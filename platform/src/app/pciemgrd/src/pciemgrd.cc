/*
 * Copyright (c) 2017-2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <setjmp.h>
#include <cinttypes>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>

#include "platform/pciehdevices/include/pci_ids.h"
#include "nic/sdk/platform/pal/include/pal.h"
#include "nic/sdk/platform/evutils/include/evutils.h"
#include "nic/sdk/platform/pciemgrutils/include/pciesys.h"
#include "nic/sdk/platform/pciemgr/include/pciemgr.h"
#include "nic/sdk/platform/pcieport/include/pcieport.h"
#include "nic/sdk/lib/catalog/catalog.hpp"

#include "pciemgrd_impl.hpp"

#ifndef PCIEMGRD_GOLD
#include "delphic.h"
#endif

static pciemgrenv_t pciemgrenv;

static void
usage(void)
{
    fprintf(stderr,
"Usage: pciemgrd [-dFinrv][-e <enabled_ports>[-b <first_bus_num>]\n"
"                [-I <inherit-mode>][-P gen<G>x<W>][-R 0|1]\n"
"                [-V subvendorid][-D subdeviceid]\n"
"    -b <first_bus_num> set first bus used to <first_bus_num>\n"
"    -d                 daemon mode\n"
"    -e <enabled_ports> mask of enabled pcie ports\n"
"    -F                 no fake bios scan\n"
"    -i                 interactive mode\n"
"    -I <inherit-mode>  inherit or re-initialize hw/shmem\n"
"    -P gen<G>x<W>      spec devices as pcie gen <G>, lane width <W>\n"
"    -r                 restart after upgrade\n"
"    -R <0|1>           reboot_on_hostdn=<0|1>\n"
"    -V subvendorid     default subsystem vendor id\n"
"    -D subdeviceid     default subsystem device id\n");
}

pciemgrenv_t *
pciemgrenv_get(void)
{
    return &pciemgrenv;
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
reboot_the_system(void)
{
    int r = system("/sbin/reboot -f");
    if (r) pciesys_logerror("failed to reboot %d\n", r);
}

static void
port_evhandler(pcieport_event_t *ev, void *arg)
{
    pciemgrenv_t *pme = pciemgrenv_get();

    switch (ev->type) {
    case PCIEPORT_EVENT_HOSTUP: {
        pciesys_loginfo("port%d: hostup gen%dx%d\n",
                        ev->port, ev->hostup.gen, ev->hostup.width);
        pciehw_event_hostup(ev->port, ev->hostup.gen, ev->hostup.width);
#ifndef PCIEMGRD_GOLD
        delphi_update_pcie_port_status(ev->port, pciemgr::Up,
                                       ev->hostup.gen, ev->hostup.width);
#endif
        break;
    }
    case PCIEPORT_EVENT_HOSTDN: {
        if (pme->reboot_on_hostdn) {
            reboot_the_system();
        }
        pciesys_loginfo("port%d: hostdn\n", ev->port);
        pciehw_event_hostdn(ev->port);
#ifndef PCIEMGRD_GOLD
        delphi_update_pcie_port_status(ev->port, pciemgr::Down);
#endif
        break;
    }
    case PCIEPORT_EVENT_BUSCHG: {
        const u_int8_t secbus = ev->buschg.secbus;
        pciesys_loginfo("port%d: buschg 0x%02x\n", ev->port, secbus);
        pciehw_event_buschg(ev->port, secbus);
        break;
    }
    case PCIEPORT_EVENT_FAULT: {
        pciesys_logerror("port%d: fault %s\n", ev->port, ev->fault.reason);
#ifndef PCIEMGRD_GOLD
        delphi_update_pcie_port_status(ev->port,
                                       pciemgr::Fault, 0, 0, ev->fault.reason);
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
    pciemgr_params_t *p = &pme->params;
    int r, port;

    /*
     * Open and configure all the ports we are going to manage.
     */
    r = 0;
    for (port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            if ((r = pcieport_open(port, p->initmode)) < 0) {
                pciesys_logerror("pcieport_open %d failed: %d\n", port, r);
                goto error_out;
            }
            if ((r = pcieport_hostconfig(port, p)) < 0) {
                pciesys_logerror("pcieport_hostconfig %d failed\n", port);
                goto close_error_out;
            }
#ifndef PCIEMGRD_GOLD
            /* initialize delphi port status object */
            delphi_update_pcie_port_status(port, pciemgr::Down);
#endif
        }
    }

    if ((r = pcieport_register_event_handler(port_evhandler, NULL)) < 0) {
        goto close_error_out;
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

static void
intr_poll(void *arg)
{
    pciemgrenv_t *pme = pciemgrenv_get();

    // poll for port events
    if (pme->poll_port) {
        for (int port = 0; port < PCIEPORT_NPORTS; port++) {
            if (pme->enabled_ports & (1 << port)) {
                pcieport_poll(port);
            }
        }
    }

    // poll for device events
    if (pme->poll_dev) {
        pciehdev_poll();
    }
}

static void
notify_intr(void *arg)
{
    pciehw_notify_intr(0);
}

static void
indirect_intr(void *arg)
{
    pciehw_indirect_intr(0);
}

int
intr_init(void)
{
    pciemgrenv_t *pme = pciemgrenv_get();
    int need_poll_timer = 0;

    if (pme->poll_port) {
        need_poll_timer = 1;
    } else {
        // XXX need to get port intr connected
        need_poll_timer = 1;
    }
    if (pme->poll_dev) {
        pciehw_notify_poll_init();
        pciehw_indirect_poll_init();
        need_poll_timer = 1;
    } else {
#ifdef __aarch64__
        uint64_t msgaddr;
        uint32_t msgdata;

        static struct pal_int notifyint;
        pal_int_open_msi(&notifyint, &msgaddr, &msgdata);
        evutil_add_pal_int(&notifyint, notify_intr, NULL);
        pciesys_logdebug("intr_init notify "
                         "msgaddr 0x%" PRIx64 " msgdata 0x%" PRIx32 "\n",
                         msgaddr, msgdata);
        pciehw_notify_intr_init(0, msgaddr, msgdata);

        static struct pal_int indirectint;
        pal_int_open_msi(&indirectint, &msgaddr, &msgdata);
        evutil_add_pal_int(&indirectint, indirect_intr, NULL);
        pciesys_logdebug("intr_init indirect "
                         "msgaddr 0x%" PRIx64 " msgdata 0x%" PRIx32 "\n",
                         msgaddr, msgdata);
        pciehw_indirect_intr_init(0, msgaddr, msgdata);
#else
        if (0) notify_intr(NULL);
        if (0) indirect_intr(NULL);
#endif
    }
    if (need_poll_timer) {
        static evutil_timer timer;
        evutil_timer_start(&timer, intr_poll, NULL, 0.01, 0.01);
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

/*
 * These overrides are separated here and called by the main loop
 * *after* the logger initialization any setting overrides can be
 * included in the log.
 */
void
pciemgrd_params(pciemgrenv_t *pme)
{
    pme->reboot_on_hostdn = pciemgrd_param_ull("PCIE_REBOOT_ON_HOSTDN",
                                               pme->reboot_on_hostdn);
    pme->enabled_ports = pciemgrd_param_ull("PCIE_ENABLED_PORTS",
                                            pme->enabled_ports);
    pme->poll_port = pciemgrd_param_ull("PCIE_POLL_PORT", pme->poll_port);
    pme->poll_dev = pciemgrd_param_ull("PCIE_POLL_DEV", pme->poll_dev);

#ifndef PCIEMGRD_GOLD
    if (pme->params.restart) {
        if (upgrade_state_restore() < 0) {
            pciesys_logerror("restore failed, forcing full init\n");
            pme->params.initmode = FORCE_INIT;
        }
    }
#endif /* PCIEMGRD_GOLD */
}

void
pciemgrd_catalog_defaults(pciemgrenv_t *pme)
{
    /* on asic single port, on haps 2 ports enabled */
    pme->enabled_ports = pal_is_asic() ? 0x1 : 0x5;

    pciemgr_params_t *params = &pme->params;
    params->vendorid = PCI_VENDOR_ID_PENSANDO;
    params->subvendorid = params->vendorid;
    params->subdeviceid = PCI_SUBDEVICE_ID_PENSANDO_NAPLES100_4GB;

#ifndef PCIEMGRD_GOLD
#ifdef __aarch64__
    sdk::lib::catalog *catalog = sdk::lib::catalog::factory();
    if (catalog != NULL) {
        pme->enabled_ports  = catalog->pcie_hostport_mask();
        params->cap_gen     = catalog->pcie_gen();
        params->cap_width   = catalog->pcie_width();
        params->subdeviceid = catalog->pcie_subdeviceid();
        sdk::lib::catalog::destroy(catalog);
    } else {
        pciesys_logerror("no catalog\n");
    }
#endif /* __aarch64__ */
#endif /* PCIEMGRD_GOLD */
}

int
main(int argc, char *argv[])
{
    pciemgrenv_t *pme = pciemgrenv_get();
    pciemgr_params_t *params = &pme->params;
    int r, opt;

    pme->interactive = 1;
    pme->reboot_on_hostdn = pal_is_asic() ? 1 : 0;
    pme->poll_port = 1;
    pme->poll_dev = 0;

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

#ifndef PCIEMGRD_GOLD
    if (upgrade_in_progress()) {
        params->restart = 1;
        params->initmode = INHERIT_OK;
    }
#endif

    /*
     * Get the catalog defaults.
     * For testing, cmdline args can override below if desired.
     */
    pciemgrd_catalog_defaults(pme);

    while ((opt = getopt(argc, argv, "b:Cde:FGiI:P:V:D:rR:")) != -1) {
        switch (opt) {
        case 'b':
            params->first_bus = strtoul(optarg, NULL, 0);
            break;
        case 'C':
            params->compliance = 1;
            break;
        case 'd':
            pme->interactive = 0;
            break;
        case 'e':
            pme->enabled_ports = strtoul(optarg, NULL, 0);
            break;
        case 'F':
            params->fake_bios_scan = 0;
            break;
        case 'G':
            pme->gold = 1;
            break;
        case 'i':
            pme->interactive = 1;
            break;
        case 'I':
            if (strcmp(optarg, "inherit_only") == 0) {
                params->initmode = INHERIT_ONLY;
            } else if (strcmp(optarg, "inherit_ok") == 0) {
                params->initmode = INHERIT_OK;
            } else if (strcmp(optarg, "force_init") == 0) {
                params->initmode = FORCE_INIT;
            } else {
                printf("bad -I arg: inherit|inherit_ok|force_init\n");
                exit(1);
            }
            break;
        case 'P':
            if (!parse_linkspec(optarg,
                                &params->cap_gen, &params->cap_width)) {
                printf("bad pcie spec: want gen%%dx%%d, got %s\n", optarg);
                exit(1);
            }
            break;
        case 'r':
            params->restart = 1;
            params->initmode = INHERIT_OK;
            break;
        case 'R':
            pme->reboot_on_hostdn = strtoul(optarg, NULL, 0);
            break;
        case 'V':
            params->subvendorid = strtoul(optarg, NULL, 0);
            break;
        case 'D':
            params->subdeviceid = strtoul(optarg, NULL, 0);
            break;
        case '?':
        default:
            usage();
            exit(1);
        }
    }

#ifdef PCIEMGRD_GOLD
    r = gold_loop();
#else
    if (pme->gold) {
        r = gold_loop();
    } else if (pme->interactive) {
        r = cli_loop();
    } else {
        r = server_loop();
    }
#endif

    exit(r < 0 ? 1 : 0);
}
