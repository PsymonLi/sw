/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>

#include "pciehdevices.h"
#include "pciemgr_if.hpp"

static int verbose_flag;
static pciemgr *pciemgr;

static void
usage(void)
{
    fprintf(stderr,
"Usage: pciemgr [-Fnv][-e <enabled_ports>[-b <first_bus_num>][-P gen<G>x<W>][-s subdeviceid]\n"
"    -b <first_bus_num> set first bus used to <first_bus_num>\n"
"    -e <enabled_ports> max of enabled pcie ports\n"
"    -F                 no fake bios scan\n"
"    -h                 initializing hw\n"
"    -H                 no initializing hw\n"
"    -P gen<G>x<W>      spec devices as pcie gen <G>, lane width <W>\n"
"    -s subdeviceid     default subsystem device id\n"
"    -v                 verbose\n");
}

static void verbose(const char *fmt, ...)
    __attribute__((format (printf, 1, 2)));
static void
verbose(const char *fmt, ...)
{
    va_list arg;

    if (verbose_flag) {
        va_start(arg, fmt);
        vprintf(fmt, arg);
        va_end(arg);
    }
}

class myevhandler : public pciemgr::evhandler {
    virtual void memwr(const int port,
                       pciehdev_t *pdev,
                       const pciehdev_memrw_notify_t *n) {
    }
};

int
main(int argc, char *argv[])
{
    const char *myname = "pciemgr";
    myevhandler myevh;
    pciemgr = new class pciemgr(myname, myevh);

    pciemgr->initialize();

    pciehdevice_resources_t p;
    memset(&p, 0, sizeof(p));
    p.lif = 5;
    p.lif_valid = 1;
    p.intrb = 0;
    p.intrc = 4;
    pciehdev_t *pdev = pciehdev_eth_new("eth", &p);
    pciemgr->add_device(pdev);

    pciemgr->finalize();

    delete pciemgr;
    exit(0);

    if (0) verbose("reference");
    if (0) usage();
}
