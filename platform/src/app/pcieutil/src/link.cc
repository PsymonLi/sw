/*
 * Copyright (c) 2018-2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <cinttypes>

#include "nic/sdk/platform/pal/include/pal.h"
#include "nic/sdk/platform/pcieport/include/pcieport.h"
#include "nic/sdk/platform/pcieport/include/portcfg.h"

#include "cmd.h"
#include "utils.hpp"
#include "pcieutilpd.h"

static struct timeval tv;

static inline uint64_t
lgettimestamp(void)
{
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

static const char *
ltssm_str(const unsigned int ltssm)
{
    static const char *ltssm_strs[] = {
        [0x00] = "detect.quiet",
        [0x01] = "detect.active",
        [0x02] = "polling.active",
        [0x03] = "polling.compliance",
        [0x04] = "polling.configuration",
        [0x05] = "config.linkwidthstart",
        [0x06] = "config.linkwidthaccept",
        [0x07] = "config.lanenumwait",
        [0x08] = "config.lanenumaccept",
        [0x09] = "config.complete",
        [0x0a] = "config.idle",
        [0x0b] = "recovery.receiverlock",
        [0x0c] = "recovery.equalization",
        [0x0d] = "recovery.speed",
        [0x0e] = "recovery.receiverconfig",
        [0x0f] = "recovery.idle",
        [0x10] = "L0",
        [0x11] = "L0s",
        [0x12] = "L1.entry",
        [0x13] = "L1.idle",
        [0x14] = "L2.idle",
        [0x15] = "<reserved>",
        [0x16] = "disable",
        [0x17] = "loopback.entry",
        [0x18] = "loopback.active",
        [0x19] = "loopback.exit",
        [0x1a] = "hotreset",
    };
    const int nstrs = sizeof(ltssm_strs) / sizeof(ltssm_strs[0]);
    if (ltssm >= nstrs) return "unknown";
    return ltssm_strs[ltssm];
}

static void
linkpoll(int argc, char *argv[])
{
    struct linkstate {
        unsigned int perstni:1;
        unsigned int perstn:1;
        unsigned int phystatus:1;
        unsigned int portgate:1;
        unsigned int crs:1;
        unsigned int ltssm_en:1;
        unsigned int ltssm_st:5;
        unsigned int reversed:1;
        unsigned int fifo_rd:8;
        unsigned int fifo_wr:8;
        int gen;
        int width;
        u_int8_t recovery;
    } ostbuf, *ost = &ostbuf, nstbuf, *nst = &nstbuf, *tst;
    u_int64_t otm, ntm, starttm, totaltm;
    int port, polltm_us, opt, showall, showfifos;
    char genGxW_str[16], fifo_str[16];
    struct tm *tm;

    port = default_pcieport();
    polltm_us = 0;
    totaltm = 0;
    showall = 0;
    showfifos = 0;

    optind = 0;
    while ((opt = getopt(argc, argv, "afp:t:T:")) != -1) {
        switch (opt) {
        case 'a':
            showall = 1;
            break;
        case 'f':
            showfifos = 1;
            break;
        case 'p':
            port = strtoul(optarg, NULL, 0);
            break;
        case 't':
            polltm_us = strtoul(optarg, NULL, 0);
            break;
        case 'T': {
            u_int32_t totaltm_sec = strtoul(optarg, NULL, 0);
            totaltm = totaltm_sec * 1000000;
            break;
        }
        default:
            return;
        }
    }

    const int w = 38;
    printf("%*s perstn_dn2up_interrupt\n", w, "");
    printf("%*s |perstn (pcie refclk good)\n", w, "");
    printf("%*s ||phystatus (phy out of reset)\n", w, "");
    printf("%*s |||ltssm_en (link training ready)\n", w, "");
    printf("%*s ||||portgate_open (traffic can flow)\n", w, "");
    printf("%*s |||||cfg_retry off (cfg trans allowed)\n", w, "");
    printf("%*s |||||| core-recov%s reversed lanes\n", w, "",
           showfifos ? "        " : "");
    printf("%*s |||||| |%s          |\n", w, "",
           showfifos ? "    fifo" : "");
    printf("[yyyyddmm-hh:mm:ss.usecs  +time (sec)] "
           "IPplgr ---%s genGxW r ltssm\n",
           showfifos ? "  rd/wr " : "");

    memset(ost, 0, sizeof(*ost));
    memset(nst, 0, sizeof(*nst));
    fifo_str[0] = '\0';
    otm = 0;
    ntm = 0;
    starttm = lgettimestamp();
    while (1) {
        const u_int32_t sta_rst = pal_reg_rd32(PXC_(STA_C_PORT_RST, port));
        const u_int32_t sta_mac = pal_reg_rd32(PXC_(STA_C_PORT_MAC, port));
        const u_int32_t cfg_mac = pal_reg_rd32(PXC_(CFG_C_PORT_MAC, port));
        const u_int32_t int_pp  = pal_reg_rd32(PP_(INT_PP_INTREG, port));

        nst->perstni = (int_pp & PP_INTREG_PERSTN(port)) != 0;
        nst->perstn = (sta_rst & STA_RSTF_(PERSTN)) != 0;
        nst->phystatus = (sta_rst & STA_RSTF_(PHYSTATUS_OR)) != 0;
        nst->portgate = (sta_mac & STA_C_PORT_MACF_(PORTGATE_OPEN)) != 0;
        nst->ltssm_en = (cfg_mac & CFG_MACF_(0_2_LTSSM_EN)) != 0;
        nst->crs = (cfg_mac & CFG_MACF_(0_2_CFG_RETRY_EN)) != 0;
        nst->ltssm_st = (sta_mac & 0x1f);

        // protect against mac reset events
        // gen/width registers are inaccessible when mac is reset.
        // (this is not perfect, still racy)
        if (pcieport_is_accessible(port)) {
            portcfg_read_genwidth(port, &nst->gen, &nst->width);
            nst->reversed = pcieport_get_mac_lanes_reversed(port);
            nst->recovery = pcieport_get_recovery(port);
        } else {
            nst->gen = 0;
            nst->width = 0;
            nst->reversed = 0;
            nst->recovery = 0;
        }

        if (showfifos) {
            u_int16_t portfifo[8], depths;
#ifdef ASIC_CAPRI
            // XXX ELBA-TODO - make pcieport_portfifo_depth()
            const u_int64_t pa = PXB_(STA_ITR_PORTFIFO_DEPTH);
            pal_reg_rd32w(pa, (u_int32_t *)portfifo, 4);
            depths = portfifo[port];
#else
            portfifo[0] = 0;
            depths = portfifo[0];
#endif
            nst->fifo_wr = depths;
            nst->fifo_rd = depths >> 8;
        }

        /* fold small depths to 0's */
        if (!showall && nst->fifo_wr <= 2) nst->fifo_wr = 0;
        if (!showall && nst->fifo_rd <= 2) nst->fifo_rd = 0;

        /* fold early detect states quiet/active, too many at start */
        if (!showall && nst->ltssm_st == 1) nst->ltssm_st = 0;

        if (memcmp(nst, ost, sizeof(*nst)) != 0) {
            ntm = lgettimestamp();
            if (otm == 0) otm = ntm;

            if (nst->gen || nst->width) {
                snprintf(genGxW_str, sizeof(genGxW_str), "gen%dx%-2d%c",
                         nst->gen, nst->width, nst->reversed ? 'r' : ' ');
            } else {
                genGxW_str[0] = '\0';
            }

            if (showfifos) {
                snprintf(fifo_str, sizeof(fifo_str), " %3u/%-3u",
                         nst->fifo_rd, nst->fifo_wr);
            }

            tm = localtime(&tv.tv_sec);
            printf("[%04d%02d%02d-%02d:%02d:%02d.%06ld +%010.6lf] "
                   "%c%c%c%c%c%c %-3d%s %-8s 0x%02x %s\n",
                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                   tm->tm_hour, tm->tm_min, tm->tm_sec,
                   tv.tv_usec,
                   (ntm - otm) / 1000000.0,
                   nst->perstni         ? 'I' : '-',
                   nst->perstn          ? 'P' : '-',
                   nst->phystatus == 0  ? 'p' :'-',
                   nst->ltssm_en        ? 'l' : '-',
                   nst->portgate        ? 'g' : '-',
                   nst->crs == 0        ? 'r' : '-',
                   nst->recovery,
                   fifo_str,
                   genGxW_str,
                   nst->ltssm_st,
                   ltssm_str(nst->ltssm_st));

            tst = ost;
            ost = nst;
            nst = tst;
            otm = ntm;
        }

        if (totaltm) {
            u_int64_t nowtm = lgettimestamp();
            if (nowtm - starttm >= totaltm) {
                tm = localtime(&tv.tv_sec);
                printf("[%04d%02d%02d-%02d:%02d:%02d.%06ld +%010.6lf] "
                       "stop after %" PRId64 " seconds\n",
                       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec,
                       tv.tv_usec,
                       (nowtm - otm) / 1000000.0, totaltm / 1000000);
                break;
            }
        }
        if (polltm_us) usleep(polltm_us);
    }
}
CMDFUNC(linkpoll,
"poll pcie link state",
"linkpoll [-af][-p <port>][-t <polltm>][-T <runtime>\n"
"    -a             show all state changes (default ignores small changes)\n"
"    -f             include rd/wr fifos (default off)\n"
"    -p <port>      poll port <port> (default port 0)\n"
"    -t <polltm>    <polltm> microseconds between poll events (default 0)\n"
"    -T <runtime>   run for <runtime> seconds, then exit\n"
);
