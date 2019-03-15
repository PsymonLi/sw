# include "capri.h"
#include "req_tx.h"
#include "sqcb.h"

struct req_tx_phv_t p;
struct req_tx_s2_t2_k k;

#define IN_P t2_s2s_wqe_to_sge_info
#define IN_TO_S_P to_s2_sqwqe_info

#define WQE_TO_SGE_P t0_s2s_wqe_to_sge_info

#define K_CURRENT_SGE_ID CAPRI_KEY_RANGE(IN_P, current_sge_id_sbit0_ebit1, current_sge_id_sbit2_ebit7)
#define K_WQE_ADDR       CAPRI_KEY_FIELD(IN_TO_S_P, wqe_addr)

%%
    .param    req_tx_sqsge_process

.align
req_tx_sqsge_iterate_process:
    mfspr          r1, spr_mpuid
    add            r1, r1[4:2], r0
    
    beqi           r1, STAGE_2, trigger_stg3_sqsge_process
    nop            // Branch Delay Slot

    nop.e
    nop

trigger_stg3_sqsge_process:
     CAPRI_RESET_TABLE_0_ARG()
     CAPRI_SET_FIELD_RANGE2(WQE_TO_SGE_P, in_progress, ah_handle, \
                            CAPRI_KEY_RANGE(IN_P, in_progress, ah_handle_sbit8_ebit31))

    seq            c1, CAPRI_KEY_FIELD(IN_P, spec_enable), 1
    // sge_offset = TXWQE_SGE_OFFSET + sqcb0_p->current_sge_id * sizeof(sge_t);
    add            r1, TXWQE_SGE_OFFSET, K_CURRENT_SGE_ID, LOG_SIZEOF_SGE_T
    // If spec_enable is set, this program is loaded only in case of SGE recirc packets.
    // So wqe format can either be 8X4 or 16X2. So add 32 bytes to offset for length encoding.
    add.c1         r1, r1, 1, LOG_SIZEOF_WQE_8x4_T_BITS

    // sge_p = sqcb0_p->curr_wqe_ptr + sge_offset
    add            r1, r1, K_WQE_ADDR
    CAPRI_SET_TABLE_2_VALID(0)
    CAPRI_NEXT_TABLE0_READ_PC_E(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, req_tx_sqsge_process, r1)
