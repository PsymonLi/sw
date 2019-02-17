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

#include "platform/misc/include/bdf.h"
#include "platform/pal/include/pal.h"
#include "platform/intrutils/include/intrutils.h"
#include "platform/pciemgrutils/include/pciesys.h"
#include "pciehw_impl.h"
#include "intr.h"

static void
init_intr_pba_cfg(pciehwdev_t *phwdev)
{
    const u_int32_t lif = phwdev->lifb;
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;

    intr_pba_cfg(lif, intrb, intrc);
}

static void
init_intr_fwcfg(pciehwdev_t *phwdev)
{
    const u_int32_t lif = phwdev->lifb;
    const int port = phwdev->port;
    const u_int8_t intpin = phwdev->intpin;
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_fwcfg_legacy(intr, lif, port, intpin);
    }
}

static void
init_intr_pba(pciehwdev_t *phwdev)
{
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_pba_clear(intr);
    }
}

static void
init_intr_drvcfg(pciehwdev_t *phwdev)
{
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_drvcfg(intr);
    }
}

static void
init_intr_msixcfg(pciehwdev_t *phwdev)
{
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_msixcfg(intr, 0, 0, 1);
    }
}

void
pciehw_intr_config(pciehwdev_t *phwdev, const int legacy, const int fmask)
{
    const u_int32_t intrb = phwdev->intrb;
    const u_int32_t intrc = phwdev->intrc;
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_fwcfg_mode(intr, legacy, fmask);
    }
}

/*
 * The legacy intx interrupt gets swizzled as it progresses
 * up the pcie hierarchy according to the "device" number.
 *
 * In config space, the legacy IntA,IntB,IntC,IntD interrupts
 * are numbered 1,2,3,4 respectively.  We are doing this calculation
 * in 0-based 0,1,2,3 to make the mod math work out.  The hardware
 * int_pin register expects 0-based numbers so this matches the hw
 * register value we need to compute.
 *
 * See PCIe Base Spec Rev 4.0v1 Table 2-19 for more info.
 */
static int
swizzle_intx_pin(pciehwdev_t *phwdev)
{
    const u_int16_t d = bdf_to_dev(phwdev->bdf);
    pciehwdev_t *parent_hwdev = pciehwdev_get(phwdev->parenth);
    if (parent_hwdev == NULL) return d % 4;
    return (swizzle_intx_pin(parent_hwdev) + d) % 4;
}

/*
 * Reset intrs to default state.
 *
 *     clear any pending intrs/pba
 *     reset to legacy intr
 *     set driver mask, clear mask_on_assert
 *     msix config is reset
 */
void
pciehw_intr_reset(pciehwdev_t *phwdev)
{
    init_intr_pba(phwdev);
    init_intr_fwcfg(phwdev);
    init_intr_drvcfg(phwdev);
    init_intr_msixcfg(phwdev);
}

void
pciehw_intr_init(pciehwdev_t *phwdev)
{
    /* setup/config */
    phwdev->intpin = swizzle_intx_pin(phwdev);
    init_intr_pba_cfg(phwdev);

    /* return intr resources to default reset state */
    pciehw_intr_reset(phwdev);
}

void
pciehw_intrhw_init(void)
{
    intr_hwinit();
}
