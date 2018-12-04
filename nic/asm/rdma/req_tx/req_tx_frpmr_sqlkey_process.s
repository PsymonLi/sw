#include "capri.h"
#include "req_tx.h"
#include "sqcb.h"
#include "nic/p4/common/defines.h"

struct req_tx_phv_t p;
struct req_tx_s4_t0_k k;
struct key_entry_aligned_t d;


#define IN_P t0_s2s_sqwqe_to_lkey_frpmr_info
#define IN_TO_S_P to_s4_frpmr_sqlkey_info

#define WQE_TO_LKEY_T0      t0_s2s_sqwqe_to_lkey_frpmr_info
#define WQE_TO_LKEY_T1      t1_s2s_sqwqe_to_lkey_frpmr_info
#define FRPMR_WRITE_BACK_P  t2_s2s_frpmr_write_back_info

#define K_ACC_CTRL         CAPRI_KEY_FIELD(IN_P, acc_ctrl)
#define K_VA               CAPRI_KEY_RANGE(IN_P, base_va_sbit0_ebit7, base_va_sbit56_ebit63)
#define K_LEN              CAPRI_KEY_RANGE(IN_TO_S_P, len_sbit0_ebit7, len_sbit48_ebit63)
#define K_SPEC_CINDEX      CAPRI_KEY_FIELD(IN_TO_S_P, spec_cindex)
#define K_LOG_PAGE_SIZE    CAPRI_KEY_FIELD(IN_P, log_page_size)
#define K_USER_KEY         CAPRI_KEY_FIELD(IN_P, new_user_key)
#define K_SGE_INDEX        CAPRI_KEY_FIELD(IN_P, sge_index)
#define K_FAST_REG_ENABLE  CAPRI_KEY_FIELD(IN_P, fast_reg_rsvd_lkey_enable)
#define K_NUM_PT_ENTRIES   CAPRI_KEY_RANGE(IN_P, num_pt_entries_sbit0_ebit7,num_pt_entries_sbit24_ebit31)
#define K_LKEY_STATE_UPD   CAPRI_KEY_FIELD(IN_P, lkey_state_update)

#define K_HEADER_TEMPLATE_ADDR CAPRI_KEY_FIELD(IN_TO_S_P, header_template_addr_or_pd)

#define K_S2S_DATA              k.{common_t0_s2s_s2s_data_sbit0_ebit7...common_t0_s2s_s2s_data_sbit152_ebit159}
#define MR_COOKIE_RANDOM_NUM    r3  //r3 is pre-loaded with 32-bit pseudo-random number, based on LFSR.

%%

    .param  req_tx_dcqcn_enforce_process
    .param  req_tx_frpmr_write_back_process

.align
req_tx_frpmr_sqlkey_process:

    // Pin frpmr_sqlkey to stage 4
    mfspr         r1, spr_mpuid
    seq           c1, r1[4:2], STAGE_4
    bcf           [!c1], bubble_to_next_stage

    // Set table valid to 0.
    add           r2, K_SGE_INDEX, r0 // BD-slot
    CAPRI_SET_TABLE_I_VALID(r2, 0)

    bbeq         K_LKEY_STATE_UPD, 1, lkey_state_update
    // PD check.
    seq          c1, CAPRI_KEY_FIELD(IN_TO_S_P, header_template_addr_or_pd), d.pd    // BD-slot
    // QP FRPMR enable check.
    bbeq         K_FAST_REG_ENABLE, 0, error_completion
    CAPRI_RESET_TABLE_2_ARG() //BD-slot
    // State check. 
    seq          c2, d.state, KEY_STATE_FREE
    // num-pt-entries set to max available space in PT table during alloc_lkey.
    sle          c3, K_NUM_PT_ENTRIES, d.num_pt_entries_rsvd  
    bcf           [!c1 | !c2 | !c3], error_completion
    nop
    // Consumer lkey ownership check.
    bbeq         d.mr_flags.ukey_en, 0, error_completion
    srl          r4, K_VA, K_LOG_PAGE_SIZE //BD Slot
    andi         r4, r4, 0x7
    add          r3, K_NUM_PT_ENTRIES, r4
    

frpmr:
    // State will be updated to valid in the second pass after DMA of pt-table entries.
//    tblwr          d.state, KEY_STATE_VALID
    tblwr          d.user_key, K_USER_KEY
    tblwr          d.acc_ctrl, K_ACC_CTRL
    tblwr          d.log_page_size, K_LOG_PAGE_SIZE
    tblwr          d.base_va, K_VA
    tblwr          d.len, K_LEN
    // Not required for MR.
//    tblwr          d.qp, K_GLOBAL_QID
    tblwr          d.mr_cookie, MR_COOKIE_RANDOM_NUM

    phvwrpair CAPRI_PHV_FIELD(FRPMR_WRITE_BACK_P, pt_base), d.pt_base, \
              CAPRI_PHV_FIELD(FRPMR_WRITE_BACK_P, dma_size), r3
    phvwr     CAPRI_PHV_FIELD(FRPMR_WRITE_BACK_P, spec_cindex), K_SPEC_CINDEX

load_frpmr_wb:
    SQCB0_ADDR_GET(r2)
    CAPRI_NEXT_TABLE2_READ_PC_E(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, req_tx_frpmr_write_back_process, r2)

error_completion:
    // Set completion status to Memory-Management-Operation-Error
    phvwrpair      p.rdma_feedback.feedback_type, RDMA_COMPLETION_FEEDBACK, \
                   p.{rdma_feedback.completion.status, rdma_feedback.completion.error}, (CQ_STATUS_MEM_MGMT_OPER_ERR << 1 | 1)
    b              load_frpmr_wb
    // Set error-disable-qp. 
    phvwr         CAPRI_PHV_FIELD(phv_global_common, _error_disable_qp),  1 //BD-slot

bubble_to_next_stage:
    seq           c1, r1[4:2], STAGE_3
    // Pass sge_index to S4
    bcf           [!c1 ], exit

    /*
     * Load sqlkey in tables 0 and 1 for frpmr. 
     * Its done in both tables coz next phv should be able to see the update in bypass-cache.
     */

    //invoke the same routine, but with valid lkey entry as d[] vector
    CAPRI_GET_TABLE_0_K(req_tx_phv_t, r7) //BD-slot
    CAPRI_NEXT_TABLE_I_READ_SET_SIZE(r7, CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS)

    CAPRI_GET_TABLE_1_K(req_tx_phv_t, r7)
    CAPRI_NEXT_TABLE_I_READ_SET_SIZE_E(r7, CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS)
    nop //BD-slot


lkey_state_update:
    tblwr.e          d.state, KEY_STATE_VALID
    nop // BD-slot

exit:
    nop.e
    nop

