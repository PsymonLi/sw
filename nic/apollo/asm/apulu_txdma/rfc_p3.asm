#include "../../p4/include/apulu_sacl_defines.h"
#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "INGRESS_rfc_p3_k.h"

struct phv_                p;
struct rfc_p3_k_           k;

%%

rfc_p3:
    /* Write HBM region address for IPv4 policy counters to PHV */
    seq        c1, k.rx_to_tx_hdr_iptype, IPTYPE_IPV4
    phvwr.c1   p.txdma_control_sacl_cntr_regn_addr, r5

    /* Compute the index into the classid array */
    mod        r7, k.txdma_control_rfc_index, SACL_P3_ENTRIES_PER_CACHE_LINE
    mul        r7, r7, SACL_P3_ENTRY_WIDTH
    /* Access the rule index at the index */
    tblrdp     r7, r7, 0, SACL_P3_ENTRY_WIDTH

    /* Write the rule index to PHV */
    phvwr      p.txdma_control_rule_index, r7

    /* Load sacl base addr + SACL_RSLT_TABLE_OFFSET to r1 */
    add        r1, r0, k.rx_to_tx_hdr_sacl_base_addr0
    addi       r1, r1, SACL_RSLT_TABLE_OFFSET
    /* Compute the byte offset for result table index */
    div        r2, r7, SACL_RSLT_ENTRIES_PER_CACHE_LINE
    mul        r2, r2, SACL_CACHE_LINE_SIZE
    /* Add the byte offset to table base */
    add        r1, r1, r2
    /* Write the address back to phv for RSLT lookup */
    phvwr      p.txdma_control_sacl_rslt_tbl_addr, r1

    /* P1 index = STAG:SPORT. */
    add        r7, k.rx_to_tx_hdr_sport_classid0, k.txdma_control_stag_classid, \
                                                 SACL_SPORT_CLASSID_WIDTH
    /* Write P1 table index to PHV */
    phvwr      p.txdma_control_rfc_index, r7

    /* Load sacl base addr + SACL_P1_TABLE_OFFSET to r1 */
    add        r1, r0, k.rx_to_tx_hdr_sacl_base_addr0
    addi       r1, r1, SACL_P1_TABLE_OFFSET
    /* Compute the byte offset for P1 table index */
    div        r2, r7, SACL_P1_ENTRIES_PER_CACHE_LINE
    mul        r2, r2, SACL_CACHE_LINE_SIZE
    /* Add the byte offset to table base */
    add.e      r1, r1, r2
    /* Write the address back to phv for P1 lookup */
    phvwr      p.txdma_control_rfc_table_addr, r1

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
rfc_p3_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
