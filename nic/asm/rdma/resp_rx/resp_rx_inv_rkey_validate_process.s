#include "capri.h"
#include "resp_rx.h"
#include "rqcb.h"
#include "common_phv.h"
#include "defines.h"

struct resp_rx_phv_t p;
struct resp_rx_s4_t3_k k;
struct key_entry_aligned_t d;

#define DMA_CMD_BASE r1
#define GLOBAL_FLAGS r2
#define TMP r3
#define DB_ADDR r4
#define DB_DATA r5
#define RQCB2_ADDR r6
#define KEY_P   r7

#define IN_TO_S_P to_s4_lkey_info
#define TO_S_WB1_P to_s5_wb1_info
#define TO_S_STATS_INFO_P to_s7_stats_info

#define K_USER_KEY CAPRI_KEY_RANGE(IN_TO_S_P, user_key_sbit0_ebit6, user_key_sbit7_ebit7)

%%

.align
resp_rx_inv_rkey_validate_process:

    // this program is loaded only for send with invalidate

    mfspr       r1, spr_mpuid
    seq         c1, r1[4:2], STAGE_4
    bcf         [!c1], bubble_to_next_stage

    add         GLOBAL_FLAGS, r0, K_GLOBAL_FLAGS // BD Slot

    bbeq        CAPRI_KEY_FIELD(IN_TO_S_P, rsvd_key_err), 1, rsvd_key_err

    /*  check if MR is eligible for invalidation
     *  check if state is invalid (same for MR and MW)
     *  if MR - check PD
     *  if type 1 MW - not allowed
     *  if type 2A MW - 
     *      * check QP if state is valid
     *      * check PD if state is free
     *  if any check fails - send NAK with appropriate error and move QP to error disable state
     */

    // it is an error to invalidate an MR not eligible for invalidation
    seq         c1, d.mr_flags.inv_en, 0 //BD Slot
    // it is an error to invalidate an MR/MW in INVALID state
    seq         c2, d.state, KEY_STATE_INVALID
    // invalidation is not allowed for type 1 MW
    seq         c3, d.type, MR_TYPE_MW_TYPE_1 
    bcf         [c1 | c2 | c3], flag_state_type_err

    seq         c1, d.mr_flags.ukey_en, 1 //BD Slot
    // check if user key in rkey matches user key in keytable
    seq         c2, d.user_key, K_USER_KEY
    bcf         [c1 & !c2], user_key_err
    // check PD if MR
    seq         c1, d.type, MR_TYPE_MR // BD Slot
    bcf         [c1], check_pd
    // c1: MR

    // neither MW type 1, nor MR, must be MW type 2A
    seq         c2, d.state, KEY_STATE_FREE // BD Slot
    bcf         [c2], check_pd

    // must be MW type 2A in valid state, check QP
    seq         c3, d.qp, K_GLOBAL_QID // BD Slot 
    bcf         [c3], update_state
    nop // BD Slot

    b           qp_mismatch

check_pd:
    seq         c2, d.pd, CAPRI_KEY_FIELD(IN_TO_S_P, pd) // BD Slot
    bcf         [!c2], pd_mismatch
    nop // BD Slot

update_state:
    // move state update post write_back (inv_rkey_process)
    CAPRI_SET_FIELD2(TO_S_WB1_P, inv_rkey, 1)
    // if key type is not MR, must be MW, update stats
    CAPRI_SET_FIELD2_C(TO_S_STATS_INFO_P, incr_mem_window_inv, 1, !c1)

exit:
    CAPRI_SET_TABLE_3_VALID_CE(c0, 0)
    nop // Exit Slot

rsvd_key_err:
    b           error_completion
    phvwrpair   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_disabled), 1, \
                CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_dis_inv_rkey_rsvd_key_err), 1 // BD Slot

flag_state_type_err:
    phvwrpair.c2   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_disabled), 1, \
                   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_dis_inv_rkey_state_err), 1
    phvwrpair.c3   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_disabled), 1, \
                   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_dis_type1_mw_inv_err), 1
    b           error_completion
    phvwrpair.c1   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_disabled), 1, \
                   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_dis_ineligible_mr_err), 1 // BD Slot

pd_mismatch:
    b           error_completion
    phvwrpair   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_disabled), 1, \
                CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_dis_mr_mw_pd_mismatch), 1 // BD Slot

qp_mismatch:
    b           error_completion
    phvwrpair   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_disabled), 1, \
                CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_dis_type2a_mw_qp_mismatch), 1 // BD Slot

user_key_err:
    b           error_completion
    phvwrpair   CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_disabled), 1, \
                CAPRI_PHV_FIELD(TO_S_STATS_INFO_P, qp_err_dis_user_key_err), 1 // BD Slot

error_completion:
    // As per standard, NAK is NOT required to be generated when an
    // incoming Send with Invalidate contains an invalid R_Key, 
    // or the R_Key contained in the IETH cannot be invalidated
    // However, RQ should be moved to ERROR state.
    // But it may be better to anyway generate a NAK if we can, 
    // as moving silently to error disable state is not a good thing anyway.

    // Generate DMA command to skip to payload end
    DMA_CMD_STATIC_BASE_GET(DMA_CMD_BASE, RESP_RX_DMA_CMD_START_FLIT_ID, RESP_RX_DMA_CMD_SKIP_PLD)
    DMA_SKIP_CMD_SETUP(DMA_CMD_BASE, 0 /*CMD_EOP*/, 1 /*SKIP_TO_EOP*/)

    // update cqe status and error in phv so that completion with error is generated
    phvwrpair   p.cqe.status, CQ_STATUS_LOCAL_ACC_ERR, p.cqe.error, 1
    phvwr       p.s1.ack_info.syndrome, AETH_NAK_SYNDROME_INLINE_GET(NAK_CODE_REM_ACC_ERR)
    // set error disable flag and ACK flag
    or          GLOBAL_FLAGS, GLOBAL_FLAGS, (RESP_RX_FLAG_ERR_DIS_QP|RESP_RX_FLAG_ACK_REQ)

    phvwr       CAPRI_PHV_RANGE(TO_S_STATS_INFO_P, lif_error_id_vld, lif_error_id), \
                    ((1 << 4) | LIF_STATS_RDMA_RESP_STAT(LIF_STATS_RESP_RX_CQE_ERR_OFFSET))

    CAPRI_SET_TABLE_3_VALID_CE(c0, 0)
    CAPRI_SET_FIELD_RANGE2(phv_global_common, _ud, _error_disable_qp, GLOBAL_FLAGS) // Exit Slot

bubble_to_next_stage:
    seq         c1, r1[4:2], STAGE_3
    // c1: stage_3
    bcf         [!c1], bubble_exit
    CAPRI_GET_TABLE_3_K(resp_rx_phv_t, r7)

    CAPRI_NEXT_TABLE_I_READ_SET_SIZE_E(r7, CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS)
    nop // Exit Slot

bubble_exit:
    nop.e
    nop
