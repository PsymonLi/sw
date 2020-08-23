
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#include "dtls.h"
#include "dw_wdt.h"

void
wdt_start(int wdt, int to)
{
    uint32_t torr = (to << 4) | to;

    wdt_writereg(wdt, WDT_TORR, torr);
    wdt_writereg(wdt, WDT_CRR, WDT_KICK_VAL);
    wdt_writereg(wdt, WDT_CR, WDT_CR_PCLK_256);
    wdt_writereg(wdt, WDT_CR, (WDT_CR_PCLK_256 | WDT_CR_ENABLE));
    wdt_writereg(wdt, WDT_CRR, WDT_KICK_VAL);
    wdt_pause(wdt, 0);
}

void
wdt_kick(int wdt)
{
    wdt_writereg(wdt, WDT_CRR, WDT_KICK_VAL);
}

/*
 * Use WDT0 to perform a chip reset.
 */
void
wdt_sys_reset(void)
{
    int wdt = 0;

    // Enable the WDT to reset the system
    wdt_enable_chip_reset(wdt);

    // Start the WDT with an immediate timeout
    wdt_start(wdt, 0);

    // Wait for reset
    for (;;) {
        asm volatile("wfi");
    }
}
