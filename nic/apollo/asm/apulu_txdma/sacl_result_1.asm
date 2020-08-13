#include "../../p4/include/apulu_sacl_defines.h"
#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "INGRESS_sacl_result_1_k.h"

struct phv_                p;
struct sacl_result_1_k_    k;

%%

sacl_result_1:
    /* Write HBM region address for IPv6 policy counters to PHV */
    seq        c1, k.rx_to_tx_hdr_iptype, IPTYPE_IPV6
    phvwr.c1   p.txdma_control_sacl_cntr_regn_addr, r5[63:FW_ACTION_XPOSN_SHIFT]

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
    phvwr.c1   p.txdma_control_final_rule_index, k.txdma_control_rule_index
    phvwr.c1   p.txdma_control_final_policy_index, k.txdma_control_policy_index
    nop.e
    nop

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
sacl_result_1_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
