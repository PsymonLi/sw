
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef __IOMEM_H__
#define __IOMEM_H__

#define SSRAM_BASE              0x400000
#define SSRAM_SIZE              0x20000

#define FLASH_BASE              0x70000000
#define FLASH_SIZE              0x8000000

#define ELB_AP_BASE             ELB_ADDR_BASE_MS_AP_OFFSET
#define ELB_SOC_BASE            ELB_ADDR_BASE_MS_SOC_OFFSET
#define ELB_CC0_BASE            ELB_ADDR_BASE_NX_CC0_OFFSET
#define ELB_CC1_BASE            ELB_ADDR_BASE_NX_CC1_OFFSET

#define GPIO_BASE               0x4000ULL

#define UART0_BASE              0x4800UL
#define UART_STRIDE             0x400

#define SSI_BASE                0x2800ULL

#define WDT_BASE                0x1400ULL
#define WDT_STRIDE              0x400

#define QSPI_BASE               0x2400ULL
#define QSPI_AHB_BASE           0x70000000ULL
#define QSPI_AHB_LEN            0x08000000ULL
#define QSPI_TRIGGER            0x7fff0000ULL
#define QSPI_TRIGGER_FAKE       (QSPI_BASE + 0xf0)      /* dac2 FastModel */

#define SOC_(n)             (ELB_SOC_BASE + ELB_SOC_CSR_ ## n ## _BYTE_ADDRESS)

#ifndef __ASSEMBLER__

#define dsb()       asm volatile("dsb sy" ::: "memory")

static inline uint32_t readreg(uint64_t addr)
{
    uint32_t r = *(volatile uint32_t *)addr;
    dsb();
    return r;
}
#define apb_readreg readreg

static inline void writereg(uint64_t addr, uint32_t val)
{
    dsb();
    *(volatile uint32_t *)addr = val;
}
#define apb_writereg writereg

#define wdt_writereg(c, r, v)   apb_writereg(WDT_CTR_BASE(c) + (r), v)
#define wdt_readreg(c, r)       apb_readreg(WDT_CTR_BASE(c) + (r))

#define soc_readreg(n)          readreg(SOC_(n))
#define soc_writereg(n, v)      writereg(SOC_(n), v)
#endif

#endif
