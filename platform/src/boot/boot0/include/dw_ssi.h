
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef __DW_SSI_H__
#define __DW_SSI_H__

#define SSI_CTRLR0              0x00
#define SSI_CTRLR1              0x04
#define SSI_SSIENR              0x08
#define SSI_SER                 0x10
#define SSI_BAUDR               0x14
#define SSI_RXFLR               0x24
#define SSI_SR                  0x28
#define SSI_DR                  0x60

#define SSI_SR_BUSY             0x1
#define SSI_SR_RFNE             0x8

#define SSI_MOD_TX              0x1
#define SSI_MOD_EEPROM          0x3

#endif
