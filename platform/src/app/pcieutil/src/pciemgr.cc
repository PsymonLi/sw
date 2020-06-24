/*
 * Copyright (c) 2019-2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <cinttypes>

#include "platform/pciemgr/include/pciehdev_event.h"
#include "platform/pciemgr_if/include/pciemgr_if.hpp"
#include "cmd.h"

class pcieevhandler : public pciemgr::evhandler {
public:
    void hostup(const int port);
    void hostdn(const int port);
    void memrd(const int port,
               const uint32_t lif,
               const pciehdev_memrw_notify_t *n);
    void memwr(const int port,
               const uint32_t lif,
               const pciehdev_memrw_notify_t *n);
    void sriov_numvfs(const int port,
                      const uint32_t lif,
                      const uint16_t numvfs);
    void reset(const int port,
               uint32_t rsttype,
               const uint32_t lifb,
               const uint32_t lifc);
};

void
pcieevhandler::hostup(const int port)
{
    printf("hostup: port %d\n", port);
}

void
pcieevhandler::hostdn(const int port)
{
    printf("hostdn: port %d\n", port);
}

void
pcieevhandler::memrd(const int port,
                     const uint32_t lif,
                     const pciehdev_memrw_notify_t *n)
{
    printf("memrd: port %d lif %u\n"
           "    bar %d baraddr 0x%" PRIx64 " baroffset 0x%" PRIx64 " "
           "size %u localpa 0x%" PRIx64 " data 0x%" PRIx64 "\n",
           port, lif,
           n->cfgidx, n->baraddr, n->baroffset,
           n->size, n->localpa, n->data);
}

void
pcieevhandler::memwr(const int port,
                     const uint32_t lif,
                     const pciehdev_memrw_notify_t *n)
{
    printf("memwr: port %d lif %u\n"
           "    bar %d baraddr 0x%" PRIx64 " baroffset 0x%" PRIx64 " "
           "size %u localpa 0x%" PRIx64 " data 0x%" PRIx64 "\n",
           port, lif,
           n->cfgidx, n->baraddr, n->baroffset,
           n->size, n->localpa, n->data);
}

void
pcieevhandler::sriov_numvfs(const int port,
                            const uint32_t lif,
                            const uint16_t numvfs)
{
    printf("sriov_numvfs: port %d lif %u numvfs %u\n", port, lif, numvfs);
}

void
pcieevhandler::reset(const int port,
                     uint32_t rsttype,
                     const uint32_t lifb,
                     const uint32_t lifc)
{
    const char *rststrs[] = { "none", "bus", "flr", "vf" };
    const unsigned int nrststrs = sizeof(rststrs) / sizeof(rststrs[0]);
    const char *rststr;
    char rstbuf[8];

    if (rsttype < nrststrs) {
        rststr = rststrs[rsttype];
    } else {
        snprintf(rstbuf, sizeof(rstbuf), "%d", rsttype);
        rststr = rstbuf;
    }
    printf("reset: port %d rsttype %s lifb %u lifc %u\n",
           port, rststr, lifb, lifc);
}

static void
pciemgr_monitor(int argc, char *argv[])
{
    pcieevhandler pcieevh;
    pciemgr *pciemgr = new class pciemgr("pcieutil", pcieevh, EV_DEFAULT);
    printf("Monitoring pcie events, ^C to exit...\n");
    evutil_run(EV_DEFAULT);
    delete pciemgr;

}

static void
pciemgr_powermode(int argc, char *argv[])
{
    enum powermode powermode;
    if (argc <= 1) {
        fprintf(stderr, "Usage: %s low|full\n", argv[0]);
        return;
    } else if (strcmp(argv[1], "low") == 0) {
        powermode = LOW_POWER;
    } else if (strcmp(argv[1], "full") == 0) {
        powermode = FULL_POWER;
    } else {
        fprintf(stderr, "Invalid power mode: %s\n", argv[1]);
        return;
    }

    pciemgr *pciemgr = new class pciemgr("pcieutil");
    if (pciemgr->powermode(powermode) < 0) {
        fprintf(stderr, "powermode(%d) failed\n", powermode);
        return;
    }
}

static void
pciemgr(int argc, char *argv[])
{
    if (argc <= 1) {
        goto usage;
    }
    if (strcmp(argv[1], "monitor") == 0) {
        pciemgr_monitor(--argc, ++argv);
    } else if (strcmp(argv[1], "powermode") == 0) {
        pciemgr_powermode(--argc, ++argv);
    } else {
        goto usage;
    }
    return;

 usage:
    fprintf(stderr,
"Usage: %s monitor|powermode ...\n"
"    monitor   [options]        monitor events from pciemgrd\n"
"    powermode [options]        send powermode event to pciemgrd\n",
            argv[0]);
}
CMDFUNC(pciemgr,
"pciemgr interface",
"pciemgr monitor|powermode ...\n"
"    monitor            monitor events from pciemgrd\n"
"    powermode low|full send powermode event to pciemgrd\n");
