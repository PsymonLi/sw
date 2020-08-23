
/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include "dtls.h"

/*
 * Configure MS to permit a WDT reset to affect a chip reset.
 */
void
wdt_enable_chip_reset(int wdt)
{
    uint32_t val;

    val = soc_readreg(CFG_WDT) | (1 << (ELB_SOC_CSR_CFG_WDT_RST_EN_LSB + wdt));
    soc_writereg(CFG_WDT, val);
}

void
wdt_pause(int wdt, int en)
{
    uint32_t val, bit;

    bit = 1 << (ELB_SOC_CSR_CFG_WDT_PAUSE_LSB + wdt);
    val = soc_readreg(CFG_WDT);
    val = en ? (val | bit) : (val & ~bit);
    soc_writereg(CFG_WDT, val);
}
