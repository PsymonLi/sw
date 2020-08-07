#include "../../p4/include/apulu_sacl_defines.h"
#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "INGRESS_sacl_result_1_k.h"

struct phv_                p;
struct sacl_result_1_k_    k;

%%

sacl_result_1:
    /* Compute the index into the results entry array */
    mod        r7, k.txdma_control_rule_index, SACL_RSLT_ENTRIES_PER_CACHE_LINE
    mul        r7, r7, SACL_RSLT_ENTRY_WIDTH
    /* Access the result entry at the index */
    tblrdp     r7, r7, 0, SACL_RSLT_ENTRY_WIDTH

    /* r1 = action */
    and        r6, r7, SACL_RSLT_ENTRY_ACTION_MASK
    srl        r1, r6, SACL_RSLT_ENTRY_ACTION_SHIFT
    /* r2 = priority */
    and        r6, r7, SACL_RSLT_ENTRY_PRIORITY_MASK
    srl        r2, r6, SACL_RSLT_ENTRY_PRIORITY_SHIFT
    /* r3 = stateful */
    and        r6, r7, SACL_RSLT_ENTRY_STATEFUL_MASK
    srl        r3, r6, SACL_RSLT_ENTRY_STATEFUL_SHIFT
    /* r4 = Alg_type */
    and        r6, r7, SACL_RSLT_ENTRY_ALG_TYPE_MASK
    srl        r4, r6, SACL_RSLT_ENTRY_ALG_TYPE_SHIFT

    /* Set c1 if current priority is higher that previous */
    slt        c1, r2, k.txdma_control_rule_priority

    /* Update PHV with new priority and action if c1 is set */
    phvwr.c1   p.txdma_to_p4e_sacl_drop, r1
    phvwr.c1   p.txdma_control_rule_priority, r2
    phvwr.c1   p.txdma_to_p4e_sacl_stateful, r3
    phvwr.c1   p.txdma_to_p4e_sacl_alg_type, r4
    phvwr.c1   p.txdma_to_p4e_sacl_root_num, k.txdma_control_root_count

    /* Load counter region address from metadata if IPv4. For IPv6, */
    /* the counter region address is already in table constant r5 */
    seq        c1, k.rx_to_tx_hdr_iptype, IPTYPE_IPV4
    add.c1     r5, r0, k.txdma_control_sacl_cntr_regn_addr
    add.!c1    r5, r0, r5[63:1]
    /* Stop if the counter region isn't configured */
    seq        c1, r0, r5
    nop.c1.e
    /* Compute and add policy offset to the counter address */
    mul        r1, k.txdma_control_sacl_policy_index, SACL_COUNTER_BLOCK_SIZE
    add        r5, r5, r1
    /* Compute and add rule offset to the counter address */
    mul        r1, k.txdma_control_rule_index, SACL_COUNTER_SIZE
    add        r5, r5, r1
    /* Compute atomic_add array index */
    addi       r6, r0, ASIC_MEM_SEM_ATOMIC_ADD_START
    add        r6, r6, r5[26:0]
    /* Format the atomic_add array command for adding 1 to one counter only */
    add        r7, r0, 1
    add.e      r7, r7, r5[31:27], 58
    /* Increment the counter */
    memwr.dx   r6, r7

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
sacl_result_1_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
