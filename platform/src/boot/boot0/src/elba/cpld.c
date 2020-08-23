
/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include "dtls.h"
#include "dw_ssi.h"

static inline uint32_t
ssi_readreg(int reg)
{
    return apb_readreg(SSI_BASE + reg);
}

static inline void
ssi_writereg(int reg, uint32_t val)
{
    apb_writereg(SSI_BASE + reg, val);
}

/*
 * pin:      3            2        |       1            0
 * bit:  7------6------5------4----|---3------2------1------0
 *      cs1  cs1_ovr  cs0  cs0_ovr |  cs1  cs1_ovr  cs0  cs0_ovr
 *                 ssi1            |             ssi0
 */
#define SPICS_PIN_SHIFT(pin)    (2 * (pin))
#define SPICS_MASK(pin)         (0x3 << SPICS_PIN_SHIFT(pin))
#define SPICS_SET(pin, val)     ((((val) << 1) | 0x1) << SPICS_PIN_SHIFT(pin))

static
void set_cs(int val)
{
    uint32_t tmp;

    tmp = soc_readreg(SCH_CFG_SSI);
    tmp = (tmp & ~SPICS_MASK(0)) | SPICS_SET(0, val);
    soc_writereg(SCH_CFG_SSI, tmp);
}

// Initialize the SPI controller for a transaction
static void
ssi_init(int scpol, int scph, int tmod, int rxlen)
{
    // Disable master and output-enable
    ssi_writereg(SSI_SSIENR, 0);
    ssi_writereg(SSI_SER, 0);

    // The target SPI frequency is 12.5MHz with a 400MHz input clock.
    // A fixed /32 divider is used. 
    ssi_writereg(SSI_CTRLR0, (tmod << 8) | (scpol << 7) | (scph << 6) | 0x7);
    ssi_writereg(SSI_BAUDR, 32);

    // Set len Rx data bytes
    if (tmod >= 2) {
	ssi_writereg(SSI_CTRLR1, rxlen - 1);
    }

    // Enable controller
    ssi_writereg(SSI_SSIENR, 1);
}

static void
ssi_enable_cs(int cs)
{
    ssi_writereg(SSI_SER, 1 << cs);
}

static void
ssi_wait_done(void)
{
    while (ssi_readreg(SSI_SR) & SSI_SR_BUSY) {
        ; /* spin */
    }
}

uint8_t
cpld_read(int reg)
{
    uint8_t val;

    ssi_init(0, 0, SSI_MOD_EEPROM, 2);
    ssi_writereg(SSI_DR, 0x0b);
    ssi_writereg(SSI_DR, reg);
    set_cs(0);          // assert cs0
    ssi_enable_cs(0);   // go go go
    ssi_wait_done();
    set_cs(1);          // release cs0
    (void)ssi_readreg(SSI_DR); /* discard dummy byte */
    val = ssi_readreg(SSI_DR); /* real data */
    ssi_writereg(SSI_SSIENR, 0);
    return val;
}

