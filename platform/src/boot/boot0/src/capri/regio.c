
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#include "dtls.h"

void apb_writereg(uint64_t addr, uint32_t val)
{
    dsb();
    *(volatile uint32_t *)addr = val;
    if (addr < 0x8000) {
        dsb();
        (void)apb_readreg(CAP_APB_READBACK(addr));
    }
}
