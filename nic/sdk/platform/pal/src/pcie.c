/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include "pal.h"
#include "penpcie_dev.h"

/*
 * Some pcie registers are in the pcie clock domain and respond
 * with an error if accessed when the pcie clock goes away.
 * We don't get any warning when the pcie clock goes away so
 * any pcie clock domain register is a risk.  This module
 * provides special handling of these registers with kernel driver
 * assist so we can continue if we get an error response.
 */

static int
pciepreg_fd(void)
{
    static int penpciefd = -1;

    if (penpciefd < 0) {
        penpciefd = open(PENPCIE_DEV, O_RDWR);
    }
    return penpciefd;
}

int
pal_pciepreg_rd32(const uint64_t pa, uint32_t *valp)
{
    static int compat_mode;
    struct pcie_rw rw;
    int fd, r;

    memset(&rw, 0, sizeof(rw));
    rw.pciepa = pa;
    rw.size = sizeof(*valp);
    rw.rdvalp = valp;

    /*
     * compat_mode=1 when running new pal on old kernels
     * that don't have /dev/penpcie.  This happens after
     * upgrades where we update user-space on old kernels
     * (or when running x86_64 sim mode).
     */
    if (compat_mode) {
 use_compat_mode:
        *valp = pal_reg_rd32(pa);
        return 0;
    }

    fd = pciepreg_fd();
    if (fd < 0) {
        compat_mode = 1;
        goto use_compat_mode;
    }
    r = ioctl(fd, PCIE_PCIEP_REGRD, &rw);
    if (r < 0) {
        /* error value for lazy callers not checking return value */
        *valp = -1;
    }
    return r;
}

int
pal_pciepreg_wr32(const uint64_t pa, const uint32_t val)
{
    /* writes need no special handling today */
    pal_reg_wr32(pa, val);
    return 0;
}
