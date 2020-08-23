
/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

        #include "defs.h"
        #include "elb_top_csr_defines.h"
        #include "elb_ap_c_hdr.h"
        #include "elb_cc0_c_hdr.h"
        #include "elb_cc1_c_hdr.h"
        #include "iomem.h"
        #include "uboot.h"
        #include "version.h"

        .section .text.init, "ax", @progbits
        .globl  _start
_start:
        b       0f
        .word   UBOOT_SIZE_MAGIC
        .word   0               // will be filled in with the file size
        .word   BOOT0_MAGIC     // identify this as boot0 rather than uboot
        .word   UBOOT_CRC32_MAGIC;
        .word   0               // will be filled in with image crc32
        .word   BOOT0_VERSION

0:
        // Take the clusters out of reset and enable
        // the DVM snoop fix.
        ldr     x0, =(ELB_AP_BASE + ELB_AP_CSR_APM1_CFG_ARM_1_BYTE_ADDRESS)
        bl      cluster_up
        ldr     x0, =(ELB_AP_BASE + ELB_AP_CSR_APM2_CFG_ARM_2_BYTE_ADDRESS)
        bl      cluster_up
        ldr     x0, =(ELB_AP_BASE + ELB_AP_CSR_APM3_CFG_ARM_3_BYTE_ADDRESS)
        bl      cluster_up
        bl      dvm_snoop_fix

        // Enable FP for varargs
        msr     cptr_el3, xzr

        isb

        // Set up the stack pointer
        ldr     x1, =(SSRAM_BASE + SSRAM_SIZE)
        mov     sp, x1

        // Relocate down to SSRAM
        adr     x0, _start              // src: current PC-rel address in flash
        ldr     x1, =_start             // dst: symbol address in SSRAM
        ldr     x2, =_end               // dst: symbol address in SSRAM
0:
        ldp     x3, x4, [x0], #16
        stp     x3, x4, [x1], #16
        cmp     x1, x2
        bne     0b

        // Zero BSS
        ldr     x0, =__bss_start__
        ldr     x1, =__bss_end__
        b       1f
0:
        str     xzr, [x0], #8
1:
        cmp     x0, x1
        bne     0b

        // Setup exception vectors to catch pre-u-boot errors
        ldr     x0, =xcpt_vectors
        msr     vbar_el3, x0

        // Continue to main
        isb
        ldr     x9, =main
        br      x9

/*
 * Take a cluster out of reset (leave the cores themselves in reset)
 * Need to do this before issuing any DSBs or DVMs.
 * In:
 *      x0: AP_CFG_ARM_[1-3]
 */
#define CFG_OV_(n)       (1 << ELB_APM0_CSR_CFG_ARM_0_OV_##n##_LSB)
#define CFG_ARM_(n)      (1 << ELB_APM0_CSR_CFG_ARM_0_CFG_##n##_LSB)

cluster_up:
        ldr     w1, [x0]
        orr     w1, w1, #(CFG_OV_(NRESETPOR) | CFG_ARM_(OVERRIDE))
        str     w1, [x0]
        orr     w1, w1, #CFG_OV_(NL2RESET)
        str     w1, [x0]
        orr     w1, w1, #(CFG_OV_(NRESETTS) | CFG_OV_(NRESETICCT) | \
                          CFG_OV_(NRESETICDT) | CFG_OV_(NRESETRDL) | \
                          CFG_OV_(NRESETGIC) | CFG_OV_(NRESETEDBG) | \
                          CFG_OV_(NRESETCNT) | CFG_OV_(NRESETATB) | \
                          CFG_OV_(NRESETACP) | CFG_OV_(NRESETACE) | \
                          CFG_OV_(NPRESETDBG))
        str     w1, [x0]
        ldr     w1, [x0]        /* read-back */
        ret

#define CFG_NIS_(f, v)  ((v) << ELB_CC0_CSR_NS_CPU0_M_MINI_CSR_CFG_NIS_CFG_NIS_0_2_ ## f ## _LSB)
#define CFG_NIS_REG(b, c)   (ELB_## b ## _BASE + \
               ELB_ ## b ## _CSR_ ## c ## _M_MINI_CSR_CFG_NIS_BYTE_ADDRESS)

/*
 * Enable DVM snoop fix
 */
dvm_snoop_fix:
        ldr     w1, =CFG_NIS_(DVM_SNOOP_FILTER_SNOOP_MATCH, 0x1) | \
                     CFG_NIS_(DVM_SNOOP_FILTER_OS_MATCH, 0x3f) | \
                     CFG_NIS_(DVM_SNOOP_FILTER_TYPE_MATCH, 0xf) | \
                     CFG_NIS_(DVM_SNOOP_FILTER_DATA, 0x5);
        ldr     x0, =CFG_NIS_REG(CC0, CPU0)
        str     w1, [x0]
        ldr     x0, =CFG_NIS_REG(CC0, CPU1)
        str     w1, [x0]
        ldr     x0, =CFG_NIS_REG(CC1, CPU2)
        str     w1, [x0]
        ldr     x0, =CFG_NIS_REG(CC1, CPU3)
        ret

