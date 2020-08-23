
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef __IOMEM_H__
#define __IOMEM_H__

#define SSRAM_BASE                  0x400000
#define SSRAM_SIZE                  0x20000

#define FLASH_BASE                  0x70000000
#define FLASH_SIZE                  0x8000000

#define CAP_MS_BASE                 0x6a000000

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

#define MS_(n)      (CAP_MS_BASE + CAP_MS_CSR_ ## n ## _BYTE_ADDRESS)

#ifndef __ASSEMBLER__

#define dsb()       asm volatile("dsb sy" ::: "memory")

static inline uint32_t readreg(uint64_t addr)
{
    uint32_t r = *(volatile uint32_t *)addr;
    dsb();
    return r;
}
#define apb_readreg readreg

/*
 * Capri APB devices have a side-effect-free readable register at offset 0xfc.
 */
#define CAP_APB_READBACK(a) (((a) & -0x100ULL) | 0xfc)

static inline void writereg(uint64_t addr, uint32_t val)
{
    dsb();
    *(volatile uint32_t *)addr = val;
}
void apb_writereg(uint64_t addr, uint32_t val);

#define wdt_writereg(c, r, v)   apb_writereg(WDT_CTR_BASE(c) + (r), v)
#define wdt_readreg(c, r)       apb_readreg(WDT_CTR_BASE(c) + (r))

#define ms_readreg(n)           readreg(MS_(n))
#define ms_writereg(n, v)       writereg(MS_(n), v)
#endif

#endif
