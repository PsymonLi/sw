/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>

#include "platform/src/lib/pal/include/pal.h"
#include "platform/src/lib/intrutils/include/intrutils.h"
#include "platform/src/lib/pciemgrutils/include/pciesys.h"
#include "pciehw_impl.h"

static void
reset_intr_pba_cfg(pciehwdev_t *phwdev)
{
    const u_int32_t lif = phwdev->lif;
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;

    intr_pba_cfg(lif, intrb, intrc);
}

static void
reset_intr_fwcfg(pciehwdev_t *phwdev)
{
    const u_int32_t lif = phwdev->lif;
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_fwcfg_msi(intr, lif, 0);
    }
}

static void
reset_intr_pba(pciehwdev_t *phwdev)
{
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_pba_clear(intr);
    }
}

static void
reset_intr_drvcfg(pciehwdev_t *phwdev)
{
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_drvcfg(intr);
    }
}

static void
reset_intr_msixcfg(pciehwdev_t *phwdev)
{
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_msixcfg(intr, 0, 0, 1);
    }
}

void
pciehw_reset_dev(pciehwdev_t *phwdev)
{
    reset_intr_pba_cfg(phwdev);
    reset_intr_fwcfg(phwdev);
    reset_intr_pba(phwdev);
    reset_intr_drvcfg(phwdev);
    reset_intr_msixcfg(phwdev);
}
