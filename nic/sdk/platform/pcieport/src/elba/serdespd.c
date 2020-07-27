/*
 * Copyright (c) 2018-2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>

#include "platform/pal/include/pal.h"
#include "platform/pciemgrutils/include/pciesys.h"
#include "platform/pcieport/include/pcieport.h"
#include "pcieportpd.h"

unsigned int
pciesd_sbus_rd(const unsigned int addr,
               const unsigned int reg)
{
    // XXX ELBA-TODO
    assert(0);
    return 0;
}

void
pciesd_sbus_wr(const unsigned int addr,
               const unsigned int reg,
               const unsigned int data)
{
    // XXX ELBA-TODO
    assert(0);
}

static uint64_t
pp_pcsd_interrupt_addr(const int lane)
{
    // XXX ELBA-TODO
    return PP_(CFG_PP_PCSD_INTERRUPT, 0) + lane * 4;
}

static uint16_t
pciesd_poll_interrupt_in_progress(const uint16_t want)
{
    const int maxpolls = 100;
    uint16_t inprog;
    int polls = 0;

    /* check once, if not done immediately then poll loop */
    inprog = pal_reg_rd32(PP_(STA_PP_PCSD_INTERRUPT_IN_PROGRESS, 0));
    while (inprog != want && ++polls < maxpolls) {
        usleep(1000);
        inprog = pal_reg_rd32(PP_(STA_PP_PCSD_INTERRUPT_IN_PROGRESS, 0));
    }

    if (inprog != want) {
        pciesys_loginfo("interrupt timeout (want 0x%04x, got 0x%04x)\n",
                        want, inprog);
        /* continue */
    }

    return inprog;
}

int
pciesd_core_interrupt(const uint16_t lanemask,
                      const uint16_t code,
                      const uint16_t data,
                      laneinfo_t *dataout)
{
    const uint32_t codedata = ((uint32_t)data << 16) | code;
    uint16_t inprog;

    /* set up interrupt code/data */
    for (int i = 0; i < 16; i++) {
        const uint16_t lanebit = 1 << i;

        if (lanemask & lanebit) {
            pal_reg_wr32(pp_pcsd_interrupt_addr(i), codedata);
        }
    }

    /* issue interrupt request */
    /* XXX ELBA-TODO */
    pal_reg_wr32(PP_(CFG_PP_PCSD_INTERRUPT_REQUEST, 0), lanemask);

    /* wait for interrupt-in-progress */
    inprog = pciesd_poll_interrupt_in_progress(lanemask);
    if (inprog != lanemask) {
        memset(dataout->w, 0xff, sizeof(dataout->w));
        return -1;
    }

    /* clear interrupt request */
    /* XXX ELBA-TODO */
    pal_reg_wr32(PP_(CFG_PP_PCSD_INTERRUPT_REQUEST, 0), 0);

    /* wait for interrupt-complete */
    inprog = pciesd_poll_interrupt_in_progress(0);
    if (inprog != 0) {
        memset(dataout->w, 0xff, sizeof(dataout->w));
        return -1;
    }

    /* read interrupt response data */
    /* XXX ELBA-TODO */
    pal_reg_rd32w(PP_(STA_PP_PCSD_INTERRUPT_DATA_OUT, 0), dataout->w, 8);

    return 0;
}

static void
pcie_sta_pp_sd_core_status(laneinfo_t *li)
{
    /* XXX ELBA-TODO */
    pal_reg_rd32w(PP_(STA_PP_SD_CORE_STATUS, 0), li->w, 8);
}

/*
 * Read core status and figure out which pcie serdes lanes of lanemask
 * have the "ready" bit set.  See the Avago serdes documentation.
 */
uint16_t
pciesd_lanes_ready(const uint16_t lanemask)
{
    laneinfo_t st;
    uint16_t lanes_ready = 0;

    pcie_sta_pp_sd_core_status(&st);
    for (int i = 0; i < 16; i++) {
        const uint16_t lanebit = 1 << i;

        if (lanemask & lanebit) {
            if (st.lane[i] & (1 << 5)) {
                lanes_ready |= lanebit;
            }
        }
    }
    return lanes_ready;
}

int
pcieportpd_serdes_init(void)
{
    assert(0); /* XXX ELBA-TODO */
    return 0;
}

int
pcieportpd_serdes_reset(void)
{
    assert(0); /* XXX ELBA-TODO */
    return 0;
}
