#include "../../p4/include/apulu_sacl_defines.h"
#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "INGRESS_rfc_p2_k.h"

struct phv_                p;
struct rfc_p2_k_           k;

%%

rfc_p2:
    /* Write IPv6 policy index to PHV */
    seq        c1, k.rx_to_tx_hdr_iptype, IPTYPE_IPV6
    add.c1     r1, k.rx_to_tx_hdr_sacl_base_addr0_s32_e39, \
                   k.rx_to_tx_hdr_sacl_base_addr0_s0_e31, 8
    sub.c1     r5, r1, r5
    addi.c1    r1, r0, SACL_IPV6_BLOCK_SIZE
    div.c1     r5, r5, r1
    phvwr.c1   p.txdma_control_sacl_policy_index, r5

    /* Compute the index into the classid array */
    mod        r7, k.txdma_control_rfc_index, SACL_P2_ENTRIES_PER_CACHE_LINE
    mul        r7, r7, SACL_P2_CLASSID_WIDTH
    /* Access the classid at the index */
    tblrdp     r7, r7, 0, SACL_P2_CLASSID_WIDTH
    /* Load sacl base addr + SACL_P3_TABLE_OFFSET to r1 */
    add        r1, k.rx_to_tx_hdr_sacl_base_addr0_s32_e39, \
                   k.rx_to_tx_hdr_sacl_base_addr0_s0_e31, 8
    addi       r1, r1, SACL_P3_TABLE_OFFSET
    /* P3 table index = P1:P2. */
    add        r2, r7, k.txdma_control_rfc_p1_classid, SACL_P2_CLASSID_WIDTH
    /* Write P3 table index to PHV */
    phvwr      p.txdma_control_rfc_index, r2
    /* Compute the byte offset for P3 table index */
    div        r2, r2, SACL_P3_ENTRIES_PER_CACHE_LINE
    mul        r2, r2, SACL_CACHE_LINE_SIZE
    /* Add the byte offset to table base */
    add.e      r1, r1, r2
    /* Write the address back to phv for P3 lookup */
    phvwr      p.txdma_control_rfc_table_addr, r1

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
rfc_p2_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
