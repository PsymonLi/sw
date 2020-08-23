
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#include "dtls.h"

/*
 * Configure MS to permit a WDT reset to affect a chip reset.
 */
void
wdt_enable_chip_reset(int wdt)
{
    uint32_t val;

    val = ms_readreg(CFG_WDT) | (1 << (CAP_MS_CSR_CFG_WDT_RST_EN_LSB + wdt));
    ms_writereg(CFG_WDT, val);
}

void
wdt_pause(int wdt, int en)
{
    uint32_t val, bit;

    bit = 1 << (CAP_MS_CSR_CFG_WDT_PAUSE_LSB + wdt);
    val = ms_readreg(CFG_WDT);
    val = en ? (val | bit) : (val & ~bit);
    ms_writereg(CFG_WDT, val);
}
