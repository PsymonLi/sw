
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef __DEFS_H__
#define __DEFS_H__
    
/*
 * Capri Clocks
 */
#define REF_CLK_ASIC                156250000
#define QSPI_CLK_ASIC               400000000

/*
 * UART configuration
 */
#define UART_CLK                    REF_CLK_ASIC
#define UART_BAUD                   115200

/*
 * Chip type
 */
#define CHIP_TYPE_ASIC  0
#define CHIP_TYPE_HAPS  1
#define CHIP_TYPE_ZEBU  2

/*
 * The active-high force-golden image GPIO locks us in to only trying to boot
 * the golden u-boot, and then golden firmware.
 */
#define GPIO_PIN_FORCE_GOLDEN       2

/*
 * GPIOs for SPI CPLD data
 */
#define CAP_GPIO_CPLD_DATA(x)       (((x) >> 6) & 0xff)

/*
 * MS Nonresettable Register used to hold state.
 * Bits [10:0] of non-resettable register 0
 */
#define BSM_STATE_REG               MS_(MSR_CFG_NONRESETTABLE)
#define BSM_STATE_REG_LSB           0

/*
 * WDT to use for high level timeouts
 */
#define BSM_WDT                     3

#endif
