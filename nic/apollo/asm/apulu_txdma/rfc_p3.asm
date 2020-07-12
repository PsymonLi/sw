#include "../../p4/include/apulu_sacl_defines.h"
#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "INGRESS_rfc_p3_k.h"

struct phv_                p;
struct rfc_p3_k_           k;

%%

rfc_p3:
    /* Compute the index into the classid array */
    mod        r7, k.txdma_control_rfc_index, SACL_P3_ENTRIES_PER_CACHE_LINE
    mul        r7, r7, SACL_P3_ENTRY_WIDTH
    /* Access the classid at the index */
    tblrdp     r7, r7, 0, SACL_P3_ENTRY_WIDTH
    /* Priority = r7 >> SACL_P3_ENTRY_PRIORITY_SHIFT */
    srl        r1, r7, SACL_P3_ENTRY_PRIORITY_SHIFT
    /* Action = r7 & SACL_P3_ENTRY_ACTION_MASK */
    and        r2, r7, SACL_P3_ENTRY_ACTION_MASK

    /* Is this TAG combination ? */
    sne        c2, k.txdma_control_recirc_count[0:0], r0
    sne        c3, k.txdma_control_stag_count, r0
    sne        c4, k.txdma_control_dtag_count, r0
    setcf      c6, [c2 | c3 | c4]

    /* Set c2 if table constant is FW_ACTION_XPOSN_ANY_DENY */
    seq        c2, r5, FW_ACTION_XPOSN_ANY_DENY
    /* Set c3 if current action is deny */
    seq        c3, r2, SACL_P3_ENTRY_ACTION_DENY
    /* Set c4 if previous action is deny */
    seq        c4, k.txdma_to_p4e_drop, SACL_P3_ENTRY_ACTION_DENY
    /* Set c5 if current priority is higher that previous */
    slt        c5, r1, k.txdma_control_rule_priority

    /* NOT any_deny_is_deny | TAG Combination ? */
    setcf      c7, [!c2 | c6]
    /* (NOT any_deny_is_deny | TAG Combo) & better priority? */
    setcf      c1, [c7 & c5]
    /* previous is allow | better priority */
    setcf      c7, [!c4 | c5]
    /* any_deny_is_deny & current is deny & !TAG & (previous is allow | better priority) */
    orcf       c1, [c2 & c3 & !c6 & c7]
    /* any_deny_is_deny & current is allow & previous is allow & better priority & !TAG */
    orcf       c1, [c2 & !c3 & !c4 & c5 & !c6]

    /* Update PHV with new priority and action if c1 is set */
    phvwr.c1   p.txdma_control_rule_priority, r1
    phvwr.c1   p.txdma_to_p4e_drop, r2
    phvwr.c1   p.txdma_to_p4e_sacl_action, r2
    phvwr.c1   p.txdma_to_p4e_sacl_root_num, k.txdma_control_recirc_count[3:1]

    /* Load sacl base addr + SACL_P1_TABLE_OFFSET to r1 */
    add        r1, r0, k.rx_to_tx_hdr_sacl_base_addr0
    addi       r1, r1, SACL_P1_TABLE_OFFSET
    /* P1 index = STAG:SPORT. */
    add        r2, k.rx_to_tx_hdr_sport_classid0, k.txdma_control_stag_classid, \
                                                 SACL_SPORT_CLASSID_WIDTH
    /* Write P1 table index to PHV */
    phvwr      p.txdma_control_rfc_index, r2
    /* Compute the byte offset for P1 table index */
    div        r2, r2, SACL_P1_ENTRIES_PER_CACHE_LINE
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
