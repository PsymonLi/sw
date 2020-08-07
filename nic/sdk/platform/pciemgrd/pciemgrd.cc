/*
 * Copyright (c) 2017-2019, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <cinttypes>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "platform/pciehdevices/include/pci_ids.h"
#include "platform/pal/include/pal.h"
#include "platform/evutils/include/evutils.h"
#include "platform/misc/include/misc.h"
#include "platform/misc/include/bdf.h"
#include "platform/pciemgrutils/include/pciesys.h"
#include "platform/pciemgr/include/pciemgr.h"
#include "platform/pciemgr/include/pciehdev_event.h"
#include "platform/pcieport/include/pcieport.h"
#include "platform/pcieport/include/portmap.h"
#include "lib/catalog/catalog.hpp"
#include "platform/fru/fru.hpp"

#include "pciemgrd_impl.hpp"

static pciemgrenv_t pciemgrenv;

pciemgrenv_t *
pciemgrenv_get(void)
{
    return &pciemgrenv;
}

static void
pcie_hostdn(void)
{
    int r = evutil_system(EV_DEFAULT_ "/nic/tools/pcie_hostdn.sh", NULL, NULL);
    if (r < 0) pciesys_logerror("pcie_hostdn failed %d\n", r);
}

#ifdef __aarch64__
/*
 * Naples25 OCP and SWM with ALOM present will not reboot when the
 * hostdn event is received.  The catalog long_lived setting is used
 * to enable this behavior which is negated when the ALOM is missing
 * per the CPLD.
 */
static int swm_alom_present(void)
{
    int id;
    int cntl;

    id = cpld_reg_rd(CPLD_REGISTER_ID);
    if (id < 0) {
        pciesys_logerror("error reading cpld id\n");
        return 0;
    }

    if (id == CPLD_ID_NAPLES25_OCP)
        return 1;

    cntl = cpld_reg_rd(CPLD_REGISTER_CTRL);
    if (cntl < 0) {
        pciesys_logerror("error reading cpld ctrl reg\n");
        return 0;
    }

    if ((id == CPLD_ID_NAPLES25_SWM || id == CPLD_NAPLES25_DELL_SWM_ID) &&
        (cntl & CPLD_ALOM_PRESENT_BIT)) {
        return 1;
    } else {
        pciesys_loginfo("naples25 swm alom missing, reset for hostdn\n");
        return 0;
    }
}
#endif /* __aarch64__ */

/*
 * With long-lived SWM cards, a hostdn is a sign the host is going away.
 * This might be an opportunity for us to reset if there is a
 * pending config change.  The pcie_hostdn_swm.sh script will check
 * for pending changes and reset if necessary.
 */
static void
pcie_hostdn_swm(void)
{
    int r = evutil_system(EV_DEFAULT_ "/nic/tools/pcie_hostdn_swm.sh",
                          NULL, NULL);
    if (r < 0) {
        pciesys_logerror("pcie_hostdn_swm.sh failed %d\n", r);
    } else {
        pciesys_loginfo("pcie_hostdn_swm.sh called\n");
    }
}

/*
 * With long-lived SWM cards, a macup is a sign the host is booting.
 * This might be another opportunity for us to reset if there is a
 * pending config change, same as pcie_hostdn_swm().
 */
static void
pcie_macup_swm(void)
{
    pcie_hostdn_swm();
}

/*
 * To make the IP address of the "int_mnic0" internal management
 * interface unique we make a component of the IP address based
 * on the primary PCIe bus address of this Naples.
 *
 * Set int_mnic0 169.254.<bus>.1.
 */
static void
handle_buschg(const int port)
{
    pciehwdev_t *phwdev = pciehwdev_get_by_id(port,
                                              PCI_VENDOR_ID_PENSANDO,
                                              PCI_DEVICE_ID_PENSANDO_MGMT);

    if (phwdev) {
        const uint16_t bdf = pciehwdev_get_hostbdf(phwdev);
        const uint8_t bus = bdf_to_bus(bdf);
        /* 169.254.<bus>.1 */
        const uint32_t ip = ((169 << 24) | (254 << 16) | (bus << 8) | 1);
        const uint32_t nm = 0xffffff00;
        const char *ifname = "int_mnic0";

        if (netif_setip(ifname, ip, nm) < 0) {
            pciesys_logerror("netif_setip %s ip 0x%08x nm 0x%08x failed: %s\n",
                             ifname, ip, nm, strerror(errno));
        }
        if (netif_up(ifname) < 0) {
            pciesys_logerror("netif_up %s failed: %s\n",
                             ifname, strerror(errno));
        }
    }
}

static void
port_evhandler(pcieport_event_t *ev, void *arg)
{
    pciemgrenv_t *pme = pciemgrenv_get();

    switch (ev->type) {
    case PCIEPORT_EVENT_LINKUP: {
        pciesys_loginfo("port%d: linkup gen%dx%d%s\n",
                        ev->port, ev->linkup.gen, ev->linkup.width,
                        ev->linkup.reversed ? "r" : "");
        break;
    }
    case PCIEPORT_EVENT_LINKDN: {
        pciesys_loginfo("port%d: linkdn\n", ev->port);
        break;
    }
    case PCIEPORT_EVENT_HOSTUP: {
        pcieport_event_hostup_t *hup = &ev->hostup;
        pciesys_loginfo("port%d: hostup gen%dx%d%s\n",
                        ev->port, hup->gen, hup->width,
                        hup->reversed ? "r" : "");
        pciehw_event_hostup(ev->port, hup->gen, hup->width, hup->lnksta2);
#ifdef IRIS
        update_pcie_port_status(ev->port, PCIEMGR_UP,
                                hup->gen, hup->width, hup->reversed);
#endif
        break;
    }
    case PCIEPORT_EVENT_HOSTDN: {
        if (pme->reboot_on_hostdn) {
            if (pme->params.long_lived) {
                pcie_hostdn_swm();
            } else {
                pcie_hostdn();
            }
        }
        // if no reset on hostdn, continue...
        pciesys_loginfo("port%d: hostdn\n", ev->port);
        pciehw_event_hostdn(ev->port);
#ifdef IRIS
        update_pcie_port_status(ev->port, PCIEMGR_DOWN);
#endif
        break;
    }
    case PCIEPORT_EVENT_MACUP: {
        pciesys_loginfo("port%d: macup event\n", ev->port);
        if (pme->params.long_lived) {
            pcie_macup_swm();
        }
        break;
    }
    case PCIEPORT_EVENT_BUSCHG: {
        const u_int8_t secbus = ev->buschg.secbus;
        pciesys_loginfo("port%d: buschg 0x%02x\n", ev->port, secbus);
        pciehw_event_buschg(ev->port, secbus);
        handle_buschg(ev->port);
        break;
    }
    case PCIEPORT_EVENT_FAULT: {
        pciesys_logerror("port%d: fault %s\n", ev->port, ev->fault.reason);
#ifdef IRIS
        update_pcie_port_status(ev->port,
                                PCIEMGR_FAULT, 0, 0, 0, ev->fault.reason);
#endif
        break;
    }
    default:
        /* Some event we don't need to handle. */
        pciesys_loginfo("port%d: event %d ignored\n", ev->port, ev->type);
        break;
    }
}

int
open_hostports(void)
{
    pciemgrenv_t *pme = pciemgrenv_get();
    pciemgr_params_t *params = &pme->params;
    int r, port;

    /*
     * Register for events here so we get events as we open/config ports.
     */
    r = pcieport_register_event_handler(port_evhandler, NULL);
    if (r < 0) {
        pciesys_logerror("pcieport_register_event_handler failed %d\n", r);
        return r;
    }

    /*
     * Open and configure all the ports we are going to manage.
     */
    r = 0;
    for (port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
#ifdef IRIS
            /* initialize delphi port status object */
            update_pcie_port_status(port, PCIEMGR_DOWN);
#endif
            if ((r = pcieport_open(port, params->initmode)) < 0) {
                pciesys_logerror("pcieport_open %d failed: %d\n", port, r);
                goto error_out;
            }
            if ((r = pcieport_hostconfig(port, params)) < 0) {
                pciesys_logerror("pcieport_hostconfig %d failed\n", port);
                goto close_error_out;
            }
        }
    }

    return r;

 close_error_out:
    pcieport_close(port);
 error_out:
    for (port--; port >= 0; port--) {
        pcieport_close(port);
    }
    return r;
}

void
close_hostports(void)
{
    pciemgrenv_t *pme = pciemgrenv_get();
    int port;

    /* close all the ports we opened */
    for (port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            pcieport_close(port);
        }
    }
}

//
// The port interrupt is "global", that is, it doesn't indicate
// which port needs service.  We'll just scan all ports to find
// the one that needs attention.
//
// We could stop the search if we find the port that was signaling us.
// pcieport_intr() returns 0 when it has processed the interrupts,
// or -1 if the interrupt was not from this port.  We could consider
// replacing the body with
//
//         if (pcieport_intr(port) == 0) break;
//
// But if we do that we should probably keep track of which port we
// checked last and start at the next port on the next interrupt to
// keep things fair across ports.  For now, these port interrupts
// are infrequent, and our production mode has only 1 port active
// so we'll save that optimization for another day.
//
static void
port_intr(void *arg)
{
    pciemgrenv_t *pme = (pciemgrenv_t *)arg;

    // process port events
    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            pcieport_intr(port);
        }
    }
}

static void
notify_intr(void *arg)
{
    const int port = *(int *)arg;

    pciehw_notify_intr(port);
}

static void
indirect_intr(void *arg)
{
    const int port = *(int *)arg;

    pciehw_indirect_intr(port);
}

static void
intr_poll(void *arg)
{
    pciemgrenv_t *pme = (pciemgrenv_t *)arg;

    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {

            // poll for port events
            pcieport_poll(port);

            // poll for device events
            pciehw_indirect_poll(port);
            pciehw_notify_poll(port);
        }
    }
}

/*****************************************************************
 * port interrupt - Port interrupts for pcie mac events.
 */

/*
 * Set up port for polling.
 */
static int
intr_init_port_poll(pciemgrenv_t *pme)
{
    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            if (pcieport_poll_init(port) < 0) return -1;
        }
    }
    return 0;
}

/*
 * Set up port for real interrupts.
 * Register for pcie mac interrupt.
 * Unmask pcie mac interrupt sources.
 */
static int
intr_init_port_intr(pciemgrenv_t *pme)
{
    static struct pal_int pciemacint;

    /* Register for the pcie mac intr. */
    if (pal_int_open(&pciemacint, "pciemac") < 0) {
        pciesys_logerror("intr_init pciemac open failed\n");
        return -1;
    }
    evutil_add_pal_int(EV_DEFAULT_ &pciemacint, port_intr, pme);

    /* Unmask the pcie mac intr sources. */
    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            if (pcieport_intr_init(port) < 0) return -1;
        }
    }
    return 0;
}

/*****************************************************************
 * dev interrupt - Device interrupts for notify/indirect events.
 */

/*
 * Set up pciehw dev for polling.
 */
static int
intr_init_dev_poll(pciemgrenv_t *pme)
{
    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            if (pciehw_notify_poll_init(port) < 0)   return -1;
            if (pciehw_indirect_poll_init(port) < 0) return -1;
        }
    }
    return 0;
}

/*
 * Set up device for real interrupts.
 * Allocate a msi interrupt for each active port.
 * Unmask interrupt sources.
 *
 * Note:  Once we start allocating msi interrupts for a port
 * we must allocate msi interrupts even for unused ports because
 * the pcie block interrupt hardware expects contiguous interrupts
 * for all ports.  Because we need contiguous interrupts, we allocate
 * all notify vectors first, then all indirect vectors, rather than
 * alternate them with a single loop through the ports.
 */
static int
intr_init_dev_intr(pciemgrenv_t *pme)
{
#ifdef __aarch64__
    static int portidx[PCIEPORT_NPORTS] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    static struct pal_int notifyint[PCIEPORT_NPORTS];
    static struct pal_int indirectint[PCIEPORT_NPORTS];
    uint64_t msgaddr;
    uint32_t msgdata;
    int r, nports, opened_one;

    nports = 0;
    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) nports++;
    }

    opened_one = 0;
    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            r = pal_int_open_msi(&notifyint[port], &msgaddr, &msgdata);
            if (r < 0) return r;
            opened_one = 1;
            evutil_add_pal_int(EV_DEFAULT_
                               &notifyint[port], notify_intr, &portidx[port]);
            pciesys_logdebug("intr_init port%d notify "
                             "msgaddr 0x%" PRIx64 " msgdata 0x%" PRIx32 "\n",
                             port, msgaddr, msgdata);
            r = pciehw_notify_intr_init(port, msgaddr, msgdata);
            if (r < 0) return r;
        } else if (opened_one && nports > 1) {
            r = pal_int_open_msi(&notifyint[port], &msgaddr, &msgdata);
            if (r < 0) return r;
            pciesys_logdebug("intr_init unused port%d notify "
                             "msgaddr 0x%" PRIx64 " msgdata 0x%" PRIx32 "\n",
                             port, msgaddr, msgdata);
        }
    }

    opened_one = 0;
    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            r = pal_int_open_msi(&indirectint[port], &msgaddr, &msgdata);
            if (r < 0) return r;
            opened_one = 1;
            evutil_add_pal_int(EV_DEFAULT_
                               &indirectint[port],
                               indirect_intr,
                               &portidx[port]);
            pciesys_logdebug("intr_init port%d indirect "
                             "msgaddr 0x%" PRIx64 " msgdata 0x%" PRIx32 "\n",
                             port, msgaddr, msgdata);
            r = pciehw_indirect_intr_init(port, msgaddr, msgdata);
            if (r < 0) return r;
        } else if (opened_one && nports > 1) {
            r = pal_int_open_msi(&indirectint[port], &msgaddr, &msgdata);
            if (r < 0) return r;
            pciesys_logdebug("intr_init unused port%d indirect "
                             "msgaddr 0x%" PRIx64 " msgdata 0x%" PRIx32 "\n",
                             port, msgaddr, msgdata);
        }
    }
#else
    if (0) notify_intr(NULL);
    if (0) indirect_intr(NULL);
#endif
    return 0;
}

int
intr_init(pciemgrenv_t *pme)
{
    ev_tstamp polltm = pme->poll_tm / 1000000.0;
    int r;

    if (!pme->poll_port) {
        r = intr_init_port_intr(pme);
    } else {
        r = intr_init_port_poll(pme);

        // polling for port events more frequently
        polltm = polltm ? MIN(0.01, polltm) : 0.01;
    }
    if (r < 0) return r;

    if (!pme->poll_dev) {
        r = intr_init_dev_intr(pme);
    } else {
        r = intr_init_dev_poll(pme);

        // polling for indirect more frequently for <10ms response.
        polltm = polltm ? MIN(0.001, polltm) : 0.001;
    }
    if (r < 0) return r;

    // set poll timer, if needed
    if (polltm) {
        static evutil_timer timer = {0};
        evutil_timer_start(EV_DEFAULT_ &timer, intr_poll, pme, polltm, polltm);
        pciesys_logdebug("intr_init: poll timer %.2f secs\n", polltm);
    }

    return 0;
}

static u_int64_t
getenv_override_ull(const char *label, const char *name, const u_int64_t def)
{
    const char *env = getenv(name);
    if (env) {
        u_int64_t val = strtoull(env, NULL, 0);
        pciesys_loginfo("%s: $%s override %" PRIu64 " (0x%" PRIx64 ")\n",
                        label, name, val, val);
        return val;
    }
    return def;
}

static u_int64_t
pciemgrd_param_ull(const char *name, const u_int64_t def)
{
    return getenv_override_ull("pciemgrd", name, def);
}


static int
parse_linkspec(const char *s, u_int8_t *genp, u_int8_t *widthp)
{
    int gen, width;

    if (sscanf(s, "gen%dx%d", &gen, &width) != 2) {
        return 0;
    }
    if (gen < 1 || gen > 4) {
        return 0;
    }
    if (width < 1 || width > 32) {
        return 0;
    }
    if (width & (width - 1)) {
        return 0;
    }
    *genp = gen;
    *widthp = width;
    return 1;
}

static void
pciemgrd_sbus_lock(void)
{
    pal_lock_ret_t r = pal_wr_lock(SBUSLOCK);
    assert(r == LCK_SUCCESS);
}

static void
pciemgrd_sbus_unlock(void)
{
    pal_lock_ret_t r = pal_wr_unlock(SBUSLOCK);
    assert(r == LCK_SUCCESS);
}

static void
pciemgrd_sbus_locker_init(pciemgrenv_t *pme)
{
    static pciesys_sbus_locker_t sbus_locker = {
        .sbus_lock   = pciemgrd_sbus_lock,
        .sbus_unlock = pciemgrd_sbus_unlock,
    };

    pciesys_set_sbus_locker(&sbus_locker, PCIESYS_PRI_PCIEMGR);
}

static void
pciemgrd_vpd_params(pciemgrenv_t *pme)
{
    pciemgr_params_t *params = &pme->params;

#define S(field, str) \
    strncpy0(params->field, str, sizeof(params->field))

#ifdef __aarch64__
    std::string s;

#define SFRU(KEY, field) \
    do { \
        if (sdk::platform::readfrukey(KEY, s) == 0) {   \
            S(field, s.c_str());\
        } \
    } while (0)

    SFRU(BOARD_PRODUCTNAME_KEY, id);
    SFRU(BOARD_PARTNUM_KEY, partnum);
    SFRU(BOARD_SERIALNUMBER_KEY, serialnum);
    SFRU(BOARD_MANUFACTURERDATE_KEY, mfgdate);
    SFRU(BOARD_ENGCHANGELEVEL_KEY, engdate); // XXX HPE wants a date
#undef SFRU

    boost::property_tree::ptree ver;
    boost::property_tree::read_json("/nic/etc/VERSION.json", ver);
    S(fwvers, ver.get<std::string>("sw.version").c_str());

#else

    S(id, "SIM NAPLES");
    S(partnum, "SIM-123456-78");
    S(serialnum, "SIM12345678");
    S(mfgdate, "SIM-2013-08-29");
    S(engdate, "SIM-2013-08-29");
    S(pcarev, "SIM-PCA-X1");
    S(misc, "SIM 1000GHz,0.0001pW");
    S(fwvers, "1.2.3.4-SIM");
#endif
#undef S
}

/*
 * These overrides are separated here and called by the main loop
 * *after* the logger initialization so any setting overrides can be
 * included in the log.
 */
void
pciemgrd_params(pciemgrenv_t *pme)
{
#ifdef ASIC_ELBA
    /* XXX ELBA-TODO make elba interrupts work */
    pme->poll_dev = 1;
#endif

    pme->reboot_on_hostdn = pciemgrd_param_ull("PCIE_REBOOT_ON_HOSTDN",
                                               pme->reboot_on_hostdn);
    pme->enabled_ports = pciemgrd_param_ull("PCIE_ENABLED_PORTS",
                                            pme->enabled_ports);
    pme->poll_port = pciemgrd_param_ull("PCIE_POLL_PORT", pme->poll_port);
    pme->poll_dev = pciemgrd_param_ull("PCIE_POLL_DEV", pme->poll_dev);
    pme->cpumask = pciemgrd_param_ull("PCIE_CPUMASK", pme->cpumask);
    pme->fifopri = pciemgrd_param_ull("PCIE_FIFOPRI", pme->fifopri);
    pme->mlockall = pciemgrd_param_ull("PCIE_MLOCKALL", pme->mlockall);
    pme->poll_tm = pciemgrd_param_ull("PCIE_POLL_TM", pme->poll_tm);

    pciemgr_params_t *params = &pme->params;
    params->single_pnd = pciemgrd_param_ull("PCIE_SINGLE_PND",
                                            params->single_pnd);

    char *cap = getenv("PCIE_CAP");
    if (cap) {
        if (parse_linkspec(cap, &params->cap_gen, &params->cap_width)) {
            pciesys_loginfo("pciemgrd: $PCIE_CAP override gen%dx%d\n",
                            params->cap_gen, params->cap_width);
        } else {
            pciesys_logwarn("pciemgrd: $PCIE_CAP \"%s\" parse failed\n", cap);
        }
    }

    pciemgrd_vpd_params(pme);

    if (pme->params.restart) {
        if (upgrade_state_restore() < 0) {
            pciesys_logerror("restore failed, forcing full init\n");
            pme->params.initmode = FORCE_INIT;
        }
        /*
         * If this upgrade restart was due to a rollback
         * we have consumed the state we saved for the rollback.
         * Mark this complete (removes any saved rollback state).
         */
        if (upgrade_rollback_in_progress()) {
            upgrade_rollback_complete();
        }
    }
}

static int
portmap_init_from_catalog(pciemgrenv_t *pme)
{
    portmap_init();
#ifdef __aarch64__
    sdk::lib::catalog *catalog = sdk::lib::catalog::factory();
    if (catalog == NULL) {
        pciesys_logerror("no catalog!\n");
        return -1;
    }

    pme->params.vendorid = catalog->pcie_vendorid();
    pme->params.subvendorid = catalog->pcie_subvendorid();
    pme->params.subdeviceid = catalog->pcie_subdeviceid();
    pme->params.long_lived = catalog->pcie_long_lived() && swm_alom_present();
    pme->params.clock_freq = catalog->pcie_clock_freq();

    const char *vpdfmt = catalog->pcie_vpd_format().c_str();
    pme->params.vpd_format = pciemgr_vpd_format_from_str(vpdfmt);

    // vpd_format specified but unrecognized
    if (vpdfmt && *vpdfmt && pme->params.vpd_format == VPD_FORMAT_NONE) {
        pciesys_logerror("catalog vpd_format \"%s\" unknown\n", vpdfmt);
        assert(0);
        return -1;
    }
    // default to "hpe" format if none specified
    if (pme->params.vpd_format == VPD_FORMAT_NONE) {
        pme->params.vpd_format = VPD_FORMAT_HPE;
    }

    int nportspecs = catalog->pcie_nportspecs();
    for (int i = 0; i < nportspecs; i++) {
        pcieport_spec_t ps = { 0 };
        ps.host  = catalog->pcie_host(i);
        ps.port  = catalog->pcie_port(i);
        ps.gen   = catalog->pcie_gen(i);
        ps.width = catalog->pcie_width(i);
        if (portmap_addhost(&ps) < 0) {
            pciesys_logerror("portmap_add i %d h%d p%d gen%dx%d failed\n",
                             i, ps.host, ps.port, ps.gen, ps.width);
            return -1;
        }
        /* virtual dev default cap matches first port */
        if (pme->params.cap_gen == 0) {
            pme->params.cap_gen = ps.gen;
            pme->params.cap_width = ps.width;
        }
    }
    sdk::lib::catalog::destroy(catalog);
#else
    pme->params.vendorid = PCI_VENDOR_ID_PENSANDO;
    pme->params.subvendorid = PCI_VENDOR_ID_PENSANDO;
    pme->params.subdeviceid = PCI_SUBDEVICE_ID_PENSANDO_NAPLES100_4GB;
    // default to "hpe" format if none specified
    if (pme->params.vpd_format == VPD_FORMAT_NONE) {
        pme->params.vpd_format = VPD_FORMAT_HPE;
    }

    pcieport_spec_t ps = { 0 };
    ps.host  = 0;
    ps.port  = 0;
    ps.gen   = 3;
    ps.width = 16;

    if (portmap_addhost(&ps) < 0) {
        pciesys_logerror("portmap_add h%d p%d gen%dx%d failed\n",
                         ps.host, ps.port, ps.gen, ps.width);
            return -1;
    }

    /* virtual dev default cap matches first port */
    if (pme->params.cap_gen == 0) {
        pme->params.cap_gen = ps.gen;
        pme->params.cap_width = ps.width;
    }
#endif
    pme->enabled_ports = portmap_portmask();
    return 0;
}

void
pciemgrd_logconfig(pciemgrenv_t *pme)
{
    pciesys_loginfo("---------------- config ----------------\n");
    pciesys_loginfo("enabled_ports 0x%x\n", pme->enabled_ports);
    pciesys_loginfo("device capabilities: gen%dx%d\n",
                    pme->params.cap_gen, pme->params.cap_width);
    pciesys_loginfo("vendorid: %04x\n", pme->params.vendorid);
    pciesys_loginfo("subvendorid: %04x\n", pme->params.subvendorid);
    pciesys_loginfo("subdeviceid: %04x\n", pme->params.subdeviceid);
    pciesys_loginfo("initmode: %d\n", pme->params.initmode);
    pciesys_loginfo("restart: %d\n", pme->params.restart);
    pciesys_loginfo("long_lived: %d\n", pme->params.long_lived);
    pciesys_loginfo("---------------- config ----------------\n");
}

void
pciemgrd_catalog_defaults(pciemgrenv_t *pme)
{
    portmap_init_from_catalog(pme);
}

/*
 * Lock our memory.  We are not swapping on our system, but the system
 * might reclaim text pages if low on memory, slowing our reaction to
 * pcie events.
 */
static void
pciemgrd_mem_init(pciemgrenv_t *pme)
{
#ifdef __aarch64__
    if (pme->mlockall) {
        if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
            pciesys_logerror("mlockall failed: %s\n", strerror(errno));
        }

        /* Fault in our stack here by writing to each stack page. */
        volatile char big_stack[64 * 1024];
        const int pagesize = getpagesize();
        for (unsigned int i = 0; i < sizeof(big_stack); i += pagesize) {
            big_stack[i] = 0;
        }
    }
#endif
}

/*
 * Set the cpu affinity mask to restrict to cpu 0.
 * Set sched policy to SCHED_FIFO to select "real-time"
 * scheduling so pciemgr can respond to pcie transactions
 * in "pcie transaction timeout" time frames, typically 50ms.
 */
static void
pciemgrd_sched_init(pciemgrenv_t *pme)
{
#ifdef __aarch64__
    if (pme->cpumask) {
        unsigned int cpumask = pme->cpumask;
        cpu_set_t cpuset;

        CPU_ZERO(&cpuset);
        for (int cpu = 0; cpumask; cpu++) {
            int cpubit = 1 << cpu;
            if (cpumask & cpubit) {
                CPU_SET(cpu, &cpuset);
                cpumask &= ~cpubit;
            }
        }
        if (sched_setaffinity(0, sizeof(cpuset), &cpuset) < 0) {
            pciesys_logerror("sched_setaffinity 0x%x: %s\n",
                             pme->cpumask, strerror(errno));
        }
    }

    if (pme->fifopri) {
        struct sched_param param;
        const int policy = SCHED_FIFO | SCHED_RESET_ON_FORK;

        memset(&param, 0, sizeof(param));
        param.sched_priority = pme->fifopri;
        if (sched_setscheduler(0, policy, &param) < 0) {
            pciesys_logerror("sched_setscheduler FIFO pri %d: %s\n",
                             pme->fifopri, strerror(errno));
        }
    }
#endif
}

// called after libev blocks, before any io,timer handlers
static void
evmon_begin(void *arg)
{
    u_int64_t *tstamp = (u_int64_t *)arg;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    *tstamp = tv.tv_sec * 1000000 + tv.tv_usec;
}

// called before libev blocks, after any io,timer handlers
static void
evmon_end(void *arg)
{
    u_int64_t *tstamp = (u_int64_t *)arg;
    u_int64_t tnow, tdelta;
    struct timeval tv;

    // first time called is before evmon_begin
    if (*tstamp == 0) return;

    gettimeofday(&tv, NULL);
    tnow = tv.tv_sec * 1000000 + tv.tv_usec;

    tdelta = tnow - *tstamp;
    if (tdelta > 2000) {
        pciesys_logwarn("evmon %ldus delay\n", tdelta);
    }
}

//
// Monitor libev callbacks runtime.  If any callbacks take too long
// to run we'll log a msg about it.  We don't want slow callbacks
// doing blocking calls that might keep pciemgr from servicing
// pcie transactions and delivering completions before the
// pcie transaction timeout (typically 10ms-50ms).
//
static void
pciemgrd_evmon_init(pciemgrenv_t *pme)
{
    static evutil_check evmoncheck;
    static evutil_prepare evmonprep;
    static u_int64_t evmon_tstamp;

    evutil_add_check(EV_DEFAULT_ &evmoncheck, evmon_begin, &evmon_tstamp);
    evutil_add_prepare(EV_DEFAULT_ &evmonprep, evmon_end, &evmon_tstamp);
}

void
pciemgrd_sys_init(pciemgrenv_t *pme)
{
    pciemgrd_mem_init(pme);
    pciemgrd_sched_init(pme);
    pciemgrd_evmon_init(pme);
    pciemgrd_sbus_locker_init(pme);
}

void
pciemgrd_start()
{
    pciemgrenv_t *pme = pciemgrenv_get();
    pciemgr_params_t *params = &pme->params;
    int r;

    pme->reboot_on_hostdn = pal_is_asic() ? 1 : 0;
    pme->fifopri = 50;
    pme->poll_port = 1;
    pme->poll_tm = 500000; // 0.5s slow poll for non-essential events

    params->strict_crs = 1;

    /*
     * For aarch64 we can inherit a system in which we get restarted on
     * a running system (mostly for testing).
     * For x86_64 we want to FORCE_INIT to reinitialize hw/shmem on startup.
     *
     * On "real" ARM systems the upstream port bridge
     * is in hw and our first virtual device is bus 0 at 00:00.0.
     *
     * For simulation we want the virtual upstream port bridge
     * at 00:00.0 so our first virtual device is bus 1 at 01:00.0.
     */
#ifdef __aarch64__
    params->initmode = INHERIT_OK;
    params->first_bus = 0;
#else
    params->initmode = FORCE_INIT;
    params->first_bus = 1;
    params->fake_bios_scan = 1;         /* simulate bios scan to set bdf's */
#endif

    if (upgrade_in_progress()) {
        params->restart = 1;
        params->initmode = INHERIT_OK;
    }

    /* Get the catalog defaults. */
    pciemgrd_catalog_defaults(pme);

    r = server_loop(pme);

    exit(r < 0 ? 1 : 0);
}
