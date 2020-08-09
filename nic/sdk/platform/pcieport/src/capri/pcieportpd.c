/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>

#include "platform/pal/include/pal.h"
#include "platform/pciemgrutils/include/pciesys.h"
#include "platform/pcieport/include/pcieport.h"
#include "platform/pcieport/include/portmap.h"
#include "platform/pcieport/src/pcieport_impl.h"
#include "pcieportpd.h"

typedef struct pp_linkwidth_s {
    int port;
    int gen;
    int width;
    uint32_t usedlanes;
    uint32_t pp_linkwidth;
} pp_linkwidth_t;

int
pcieportpd_validate_hostconfig(pcieport_t *p)
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

int
pcieportpd_hostconfig(pcieport_t *p, const pciemgr_params_t *params)
{
    switch (p->cap_width) {
    case  1: /* x1 uses 2 lanes */
    case  2: p->lanemask = 0x0003 << (p->port * 2); break;
    case  4: p->lanemask = 0x000f << (p->port * 2); break;
    case  8: p->lanemask = 0x00ff << (p->port * 2); break;
    case 16: p->lanemask = 0xffff << (p->port * 2); break;
    }

    return 0;
}

/*
 * We don't get with pcieport_get(port) here.
 * This is used by some callers who are not
 * going to pcieport_open() but want to read
 * registers directly (e.g. pcieutil);
 */
int
pcieportpd_is_accessible(const int port)
{
    const u_int32_t sta_rst = pal_reg_rd32(PXC_(STA_C_PORT_RST, port));
    const u_int32_t sta_mac = pal_reg_rd32(PXC_(STA_C_PORT_MAC, port));
    const u_int8_t ltssm_st = sta_mac & 0x1f;

    /*
     * port is accessible if out of reset so pcie refclk is good and
     * ltssm_st >= 0x9 (config.complete).
     *
     * Note: this is a bit conservative, we don't need to be all the
     * way through config to have a stable refclk.  But the link might
     * still be settling and refclk could go away during the settling
     * time so we check ltssm_st to be sure we've settled.
     */
    return (sta_rst & STA_RSTF_(PERSTN)) != 0 && ltssm_st >= 0x9;
}

/*
 * Each port has a port_lanes register that reports the status of
 * each ports lanes.  The 16 port lanes are hardwired to each port
 * with this mapping:
 *
 *     port 0 = physical lanes [7:0]
 *     port 1 = physical lanes [3:2]
 *     port 2 = physical lanes [7:4]
 *     port 3 = physical lanes [7:6]
 *     port 4 = physical lanes [15:8]
 *     port 5 = physical lanes [12:10]
 *     port 6 = physical lanes [15:12]
 *     port 7 = physical lanes [15:14]
 */
int
pcieport_get_lanes(const int port,
                   u_int32_t *lanes_detected, u_int32_t *lanes_active)
{
    u_int32_t port_lanes;
    int r = 0;

    if (pal_pciepreg_rd32(PXP_(STA_P_PORT_LANES_7_0, port), &port_lanes) < 0) {
        port_lanes = 0;
        r = -1;
    }
    if (lanes_detected) *lanes_detected = port_lanes & 0xff;
    if (lanes_active)   *lanes_active   = (port_lanes >> 8) & 0xff;
    return r;
}

static int
pp_linkwidth(pp_linkwidth_t *pplw, const int maxwidth)
{
    uint32_t linkwidth, portlanes;

    if (pplw->width > maxwidth) {
        pciesys_logerror("pp_linkwidth port%d max width %d > %d\n",
                         pplw->port, pplw->width, maxwidth);
        return -1;
    }

    /*
     * 1-2 lanes: set pp_linkwidth 3
     * 4   lanes: set 2
     * 8   lanes: set 1
     * 16  lanes: set 0
     */
    switch (pplw->width) {
    case  1:
    case  2: linkwidth = 3; portlanes = 0x3    << (pplw->port * 2); break;
    case  4: linkwidth = 2; portlanes = 0xf    << (pplw->port * 2); break;
    case  8: linkwidth = 1; portlanes = 0xff   << (pplw->port * 2); break;
    case 16: linkwidth = 0; portlanes = 0xffff << (pplw->port * 2); break;
    default:
        pciesys_logerror("pp_linkwidth port%d bad width %d\n",
                         pplw->port, pplw->width);
        return -1;
    }

    /* check if someone is already using these lanes */
    if (pplw->usedlanes & portlanes) {
        pciesys_logerror("pp_linkwidth port%d gen%dx%d lane overlap\n",
                         pplw->port, pplw->gen, pplw->width);
        return -1;
    }

    /* we are using these lanes */
    pplw->usedlanes |= portlanes;
    pplw->pp_linkwidth |= linkwidth << (pplw->port * 2);
    return 0;
}

/*
 * We have 16 pcie lanes to configure across 8 ports.
 * Based on the pcie link port config for this platform
 * configure the global PP_LINKWIDTH lane configuration
 * to match the port mapping.
 *
 * Enforce these constraints on the maximum number of lanes
 * for ports, and watch for overlapping lane commitments,
 * e.g. if port0 is configured for 8 lanes then port3 cannot use 2.
 *
 *     Port0 can use 16, 8, 4, 2 lanes.
 *     Port1 can use           2 lanes.
 *     Port2 can use        4, 2 lanes.
 *     Port3 can use           2 lanes.
 *     Port4 can use     8, 4, 2 lanes.
 *     Port5 can use           2 lanes.
 *     Port6 can use        4, 2 lanes.
 *     Port7 can use           2 lanes.
 */
int
pcieportpd_pp_linkwidth(void)
{
    const uint32_t portmask = portmap_portmask();
    pp_linkwidth_t pplw;
    int port, r;

    memset(&pplw, 0, sizeof(pplw));

    for (port = 0; port < PCIEPORT_NPORTS; port++) {
        const int portbit = 1 << port;
        if (portmask & portbit) {
            pcieport_spec_t ps;

            if (portmap_getspec(port, &ps) < 0) {
                pciesys_logerror("portmap_getspec port %d failed\n", port);
                return -1;
            }
            pplw.port = port;
            pplw.gen = ps.gen;
            pplw.width = ps.width;

            switch (port) {
            case 0: r = pp_linkwidth(&pplw, 16); break;
            case 1: r = pp_linkwidth(&pplw,  2); break;
            case 2: r = pp_linkwidth(&pplw,  4); break;
            case 3: r = pp_linkwidth(&pplw,  2); break;
            case 4: r = pp_linkwidth(&pplw,  8); break;
            case 5: r = pp_linkwidth(&pplw,  2); break;
            case 6: r = pp_linkwidth(&pplw,  4); break;
            case 7: r = pp_linkwidth(&pplw,  2); break;
            default:
                pciesys_logerror("pp_linkwidth: unknown port %d\n", port);
                return -1;
            }
            if (r < 0) {
                return r;
            }
        }
    }
    pal_reg_wr32(PP_(CFG_PP_LINKWIDTH, 0), pplw.pp_linkwidth);
    return 0;
}
