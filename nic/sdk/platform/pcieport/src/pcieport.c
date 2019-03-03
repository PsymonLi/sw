/*
 * Copyright (c) 2017-2019, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "platform/pciehdevices/include/pci_ids.h"
#include "platform/misc/include/misc.h"
#include "platform/pal/include/pal.h"
#include "platform/pciemgrutils/include/pciesys.h"
#include "platform/pciemgr/include/pciemgr.h"
#include "pcieport.h"
#include "pcieport_impl.h"

pcieport_info_t *pcieport_info;

static pcieport_info_t *
pcieport_info_map(pciemgr_initmode_t initmode)
{
    pcieport_info_t *pi;
    char path[PATH_MAX];
    char *env;
    int oflags;

    /*
     * Find the default path, either from $PCIEPORT_DATA
     * or /var/run/pcieport_data on ARM,
     * or $HOME/.pcieport_data on x86_64.
     */
    env = getenv("PCIEPORT_DATA");
    if (env) {
        strncpy(path, env, sizeof(path));
        pciesys_loginfo("PCIEPORT_DATA override %s\n", path);
    } else {
#ifdef __aarch64__
        strncpy(path, "/var/run/pcieport_data", sizeof(path));
#else
        env = getenv("HOME");
        snprintf(path, sizeof(path), "%s/.pcieport_data", env ? env : "");
#endif
    }

    /*
     * If FORCE_INIT, just rm the datafile and we'll behave as if
     * there was no saved state.
     */
    if (initmode == FORCE_INIT) {
        (void)unlink(path);
    }

    oflags = O_RDWR;
    if (initmode == INHERIT_OK || initmode == FORCE_INIT) {
        oflags |= O_CREAT;
    }
    pi = mapfile(path, sizeof(pcieport_info_t), oflags);
    if (pi == NULL) {
        pciesys_logerror("%s: %s\n", path, strerror(errno));
        return NULL;
    }
    return pi;
}

static void
pcieport_info_unmap(void)
{
    munmap(pcieport_info, sizeof(pcieport_info_t));
    pcieport_info = NULL;
}

pcieport_info_t *
pcieport_info_get_or_map(pciemgr_initmode_t initmode)
{
    if (pcieport_info == NULL) {
        pcieport_info = pcieport_info_map(initmode);
    }
    return pcieport_info;
}


static pcieport_t *
pcieport_get(const int port)
{
    pcieport_info_t *pi = pcieport_info_get();
    pcieport_t *p = NULL;

    if (port >= 0 && port < PCIEPORT_NPORTS) {
        p = &pi->pcieport[port];
    }
    return p;
}

int
pcieport_open(const int port, pciemgr_initmode_t initmode)
{
    pcieport_info_t *pi = pcieport_info_get_or_map(initmode);
    pcieport_t *p;

    if (pi == NULL) {
        return -ENOENT;
    }
    if (pcieport_onetime_init(pi, initmode) < 0) {
        return -EIO;
    }
    if (port < 0 || port >= PCIEPORT_NPORTS) {
        pciesys_logerror("pcieport_open port %d out of range\n", port);
        return -EBADF;
    }
    p = &pi->pcieport[port];
    if (p->open) {
        if (initmode == INHERIT_ONLY) {
            /* pcieutil goes through this case */
            return 0;
        }
        if (initmode == INHERIT_OK) {
            pciesys_logdebug("pcieport_open port %d inherited open\n", port);
            return 0;
        }
        pciesys_logerror("pcieport_open port %d already open\n", port);
        return -EBUSY;
    }
    p->port = port;
    p->host = 0;
    p->config = 0;
    p->open = 1;

    return 0;
}

void
pcieport_close(const int port)
{
    pcieport_t *p = pcieport_get(port);

    if (p && p->open) {
        p->open = 0;
    }
    if (0) pcieport_info_unmap();
}

pcieport_stats_t *
pcieport_stats_get(const int port)
{
    pcieport_t *p = pcieport_get(port);

    if (p && p->open) {
        return &p->stats;
    }
    return NULL;
}

static int
pcieport_validate_hostconfig(pcieport_t *p)
{
    switch (p->cap_gen) {
    case 1:
    case 2:
    case 3:
    case 4:
        /* all good */
        break;
    default:
        pciesys_logerror("port %d unsupported gen%d\n", p->port, p->cap_gen);
        return -EFAULT;
    }

    switch (p->cap_width) {
    case 1: /* x1 uses 2 lanes */
    case 2:
        /* XXX verify peer isn't also configured to use our lanes */
        break;
    case 4:
        /* odd ports don't support x4 */
        if (p->port & 0x1) {
            goto bad_width;
        }
        /* XXX verify peer isn't also configured to use our lanes */
        break;
    case 8:
        /* only ports 0,4 can support x8 */
        if (p->port != 0 && p->port != 4) {
            goto bad_width;
        }
        break;
    case 16:
        /* only port 0 can use all 16 lanes */
        if (p->port != 0) {
            goto bad_width;
        }
        break;
    default:
        pciesys_logerror("port %d unsupported x%d\n", p->port, p->cap_width);
        return -ERANGE;
    }
    return 0;

 bad_width:
    pciesys_logerror("port %d doesn't support x%d\n", p->port, p->cap_width);
    return -EINVAL;
}

static int
pcieport_parse_cap(char *cap, int *gen, int *width)
{
    if (sscanf(cap, "gen%dx%d", gen, width) == 2) {
        if (*gen >= 1 && *gen <= 4 &&
            *width >= 1 && *width <= 16) {
            return 1;
        }
    }
    return 0;
}

static int
pcieport_getenv_cap(int port, int *gen, int *width)
{
    char envar[40];
    char *env;

    snprintf(envar, sizeof(envar), "PCIEPORT%d_CAP", port);
    env = getenv(envar);
    if (env && pcieport_parse_cap(env, gen, width)) {
        pciesys_loginfo("pcieport: $%s selects gen%dx%d\n",
                        envar, *gen, *width);
        return 1;
    }

    snprintf(envar, sizeof(envar), "PCIEPORT_CAP");
    env = getenv(envar);
    if (env && pcieport_parse_cap(env, gen, width)) {
        pciesys_loginfo("pcieport: $%s selects gen%dx%d\n",
                        envar, *gen, *width);
        return 1;
    }

    snprintf(envar, sizeof(envar), "PCIE_CAP");
    env = getenv(envar);
    if (env && pcieport_parse_cap(env, gen, width)) {
        pciesys_loginfo("pcieport: $%s selects gen%dx%d\n",
                        envar, *gen, *width);
        return 1;
    }

    return 0;
}

static void
pcieport_default_cap(pcieport_t *p, int *gen, int *width)
{
    /* check envar for override */
    if (!pcieport_getenv_cap(p->port, gen, width)) {
        /* no envar, provide defaults */
        *gen = pal_is_asic() ? 4 : 1;
        *width = pal_is_asic() ? 16 : 4;
    }
}

int
pcieport_hostconfig(const int port, const pciemgr_params_t *params)
{
    pcieport_t *p = pcieport_get(port);
    int r, default_gen, default_width;

    if (p == NULL) {
        return -EBADF;
    }

    if (p->config) {
        if (!p->host) {
            pciesys_logerror("pcieport_hostconfig port %d "
                             "configured but not host\n", port);
            return -EBUSY;
        }
        if (params->initmode == INHERIT_ONLY ||
            params->initmode == INHERIT_OK) {
            pciesys_logdebug("pcieport_hostconfig port %d "
                             "inherited host config\n", port);
            return 0;
        }
    }

    pcieport_default_cap(p, &default_gen, &default_width);

    if (params) {
        p->cap_gen = params->cap_gen;
        p->cap_width = params->cap_width;
        p->subvendorid = params->subvendorid;
        p->subdeviceid = params->subdeviceid;
        p->compliance = params->compliance;
        p->sris = params->sris;
        p->crs = params->strict_crs;
    }

    /*
     * Provide default params for any unspecified.
     */
    if (p->cap_gen == 0) {
        p->cap_gen = default_gen;
    }
    if (p->cap_width == 0) {
        p->cap_width = default_width;
    }
    if (p->subvendorid == 0) {
        p->subvendorid = PCI_VENDOR_ID_PENSANDO;
    }
    if (p->subdeviceid == 0) {
        p->subdeviceid = PCI_SUBDEVICE_ID_PENSANDO_NAPLES100_4GB;
    }

    /*
     * Verify the requested config is valid.
     */
    if ((r = pcieport_validate_hostconfig(p)) < 0) {
        return r;
    }

    switch (p->cap_width) {
    case  1: /* x1 uses 2 lanes */
    case  2: p->lanemask = 0x0003 << (p->port << 0); break;
    case  4: p->lanemask = 0x000f << (p->port << 1); break;
    case  8: p->lanemask = 0x00ff << (p->port << 2); break;
    case 16: p->lanemask = 0xffff << (p->port << 4); break;
    }

    if (pcieport_onetime_portinit(p) < 0) {
        return -EFAULT;
    }

    /*
     * We are configed in host mode, host mode gets
     * crs on by default (until _crs_off disables).
     */
    p->host = 1;
    p->config = 1;
    return 0;
}

int
pcieport_crs_off(const int port)
{
    pcieport_t *p = pcieport_get(port);

    if (p == NULL)  return -EBADF;
    if (!p->config) return -EIO;
    if (!p->host)   return -EINVAL;

#ifdef __aarch64__
    pciesys_loginfo("port%d: crs_off\n", port);
#endif
    if (p->crs) {
        p->crs = 0;
        pcieport_set_crs(p, p->crs);
    }
    return 0;
}

/******************************************************************
 * debug
 */

void
pcieport_showport(const int port)
{
    pcieport_info_t *pi = pcieport_info_get();
    pcieport_t *p = pcieport_get(port);
    const int w = 20;

    if (p == NULL) {
        pciesys_logerror("port %d out of range\n", port);
        return;
    }

#define P(name, fmt, args, ...) \
    pciesys_loginfo("%-*s " fmt "\n", w, name, args, ##__VA_ARGS__)

    /* global info */
    P("version", "0x%04x", pi->version);
    P("init", "%d", pi->init);
    P("serdes_init", "%d", pi->serdes_init);
    P("serdes_init_always", "%d", pi->serdes_init_always);
    P("inherited_init", "%d", pi->inherited_init);

    /* port info */
    P("config",   "gen%dx%d", p->cap_gen, p->cap_width);
    P("current",  "gen%dx%d", p->cur_gen, p->cur_width);
    P("required", "gen%dx%d", p->req_gen, p->req_width);
    P("lanemask", "0x%04x", p->lanemask);
    P("subvendorid", "0x%04x", p->subvendorid);
    P("subdeviceid", "0x%04x", p->subdeviceid);
    P("init", "%d", p->init);
    P("open", "%d", p->open);
    P("host", "%d", p->host);
    P("config", "%d", p->config);
    P("crs", "%d", p->crs);
    P("compliance", "%d", p->compliance);
    P("aer_common", "%d", p->aer_common);
    P("sris", "%d", p->sris);
    P("vga_support", "%d", p->vga_support);
    P("reduce_rx_cred", "%d", p->reduce_rx_cred);
    P("state", "%s", pcieport_stname(p->state));
    P("event", "%s", pcieport_evname(p->event));
    P("fault_reason", "%s", p->fault_reason);
    P("last_fault_reason", "%s", p->last_fault_reason);

#undef P
}

void
pcieport_showports(void)
{
    int port;

    pciesys_loginfo("%-4s %-7s %-4s %s\n",
                    "port", "link", "bus", "state");

    for (port = 0; port < PCIEPORT_NPORTS; port++) {
        pcieport_t *p = pcieport_get(port);

        if (!p->open) continue;

        pciesys_loginfo("%-4d gen%dx%-2d 0x%02x %s\n",
                        port,
                        p->cur_gen, p->cur_width,
                        p->pribus,
                        pcieport_stname(p->state));
    }
}

void
pcieport_showportstats(const int port, const unsigned int flags)
{
    pcieport_t *p = pcieport_get(port);
    const int w = 20;

    if (p == NULL) {
        pciesys_logerror("port %d out of range\n", port);
        return;
    }

#define PCIEPORT_STATS_DEF(S) \
    if (flags & PSF_ALL || p->stats.S) \
        pciesys_loginfo("%-*s %" PRIi64 "\n", w, #S, p->stats.S);
#include "pcieport_stats_defs.h"
}

void
pcieport_clearportstats(const int port, const unsigned int flags)
{
    pcieport_t *p = pcieport_get(port);

    if (p == NULL) {
        pciesys_logerror("port %d out of range\n", port);
        return;
    }

#define PCIEPORT_STATS_DEF(S) \
    if (p->stats.S) p->stats.S = 0;
#include "pcieport_stats_defs.h"
}

static void
cmd_fsm(int argc, char *argv[])
{
    pcieport_fsm_dbg(argc, argv);
}

static void
cmd_port(int argc, char *argv[])
{
    int port = 0;

    if (argc > 1) {
        port = strtoul(argv[1], NULL, 0);
    }
    if (port < 0 || port >= PCIEPORT_NPORTS) {
        pciesys_logerror("port %d out of range\n", port);
        return;
    }
    pcieport_showport(port);
}

static void
cmd_ports(int argc, char *argv[])
{
    pcieport_showports();
}

static void
cmd_portstats(int argc, char *argv[])
{
    int opt, port;
    unsigned int flags;

    flags = PSF_NONE;
    port = 0;
    optind = 0;
    while ((opt = getopt(argc, argv, "ap:")) != -1) {
        switch (opt) {
        case 'a':
            flags |= PSF_ALL;
            break;
        case 'p':
            port = strtoul(optarg, NULL, 0);
            break;
        default:
            return;
        }
    }

    if (port < 0 || port >= PCIEPORT_NPORTS) {
        pciesys_logerror("port %d out of range\n", port);
        return;
    }
    pcieport_showportstats(port, flags);
}

typedef struct cmd_s {
    const char *name;
    void (*f)(int argc, char *argv[]);
    const char *desc;
    const char *helpstr;
} cmd_t;

static cmd_t cmdtab[] = {
#define CMDENT(name, desc, helpstr) \
    { #name, cmd_##name, desc, helpstr }
    CMDENT(fsm, "fsm", ""),
    CMDENT(port, "port", ""),
    CMDENT(ports, "ports", ""),
    CMDENT(portstats, "portstats", ""),
    { NULL, NULL }
};

static cmd_t *
cmd_lookup(cmd_t *cmdtab, const char *name)
{
    cmd_t *c;

    for (c = cmdtab; c->name; c++) {
        if (strcmp(c->name, name) == 0) {
            return c;
        }
    }
    return NULL;
}

void
pcieport_dbg(int argc, char *argv[])
{
    cmd_t *c;

    if (argc < 2) {
        pciesys_loginfo("Usage: pcieport <subcmd>\n");
        return;
    }

    c = cmd_lookup(cmdtab, argv[1]);
    if (c == NULL) {
        pciesys_logerror("%s: %s not found\n", argv[0], argv[1]);
        return;
    }
    c->f(argc - 1, argv + 1);
}
