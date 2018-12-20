#include "capri.h"
#include "req_tx.h"
#include "sqcb.h"

struct req_tx_phv_t p;
struct sqwqe_t d;
struct req_tx_s2_t0_k k;

#define SQ_BKTRACK_P t0_s2s_sq_bktrack_info
#define SQCB0_WRITE_BACK_P t0_s2s_sqcb_write_back_info
#define SQCB2_WRITE_BACK_P t1_s2s_bktrack_sqcb2_write_back_info

#define TO_S1_BT_P to_s1_bt_info
#define TO_S2_BT_P to_s2_bt_info
#define TO_S3_BT_P to_s3_bt_info
#define TO_S4_BT_P to_s4_bt_info
#define TO_S5_BT_P to_s5_bt_info
#define TO_S6_BT_P to_s6_bt_info

#define TO_S7_BT_WB_P to_s7_bt_wb_info

#define IN_P      t0_s2s_sq_bktrack_info
#define IN_TO_S_P to_s2_bt_info

#define K_SSN        CAPRI_KEY_RANGE(IN_P, ssn_sbit0_ebit6, ssn_sbit23_ebit23)
#define K_TX_PSN     CAPRI_KEY_RANGE(IN_P, tx_psn_sbit0_ebit6, tx_psn_sbit23_ebit23)
#define K_REXMIT_PSN CAPRI_KEY_RANGE(IN_TO_S_P, rexmit_psn_sbit0_ebit3, rexmit_psn_sbit20_ebit23)
#define K_LOG_SQ_PAGE_SIZE CAPRI_KEY_RANGE(IN_TO_S_P, log_sq_page_size_sbit0_ebit2, log_sq_page_size_sbit3_ebit4)
#define K_LOG_NUM_WQES CAPRI_KEY_RANGE(IN_TO_S_P, log_num_wqes_sbit0_ebit0, log_num_wqes_sbit1_ebit4)
#define K_SQ_C_INDEX CAPRI_KEY_RANGE(IN_P, sq_c_index_sbit0_ebit7, sq_c_index_sbit8_ebit15)
#define K_SQ_P_INDEX CAPRI_KEY_RANGE(IN_P, sq_p_index_or_imm_data1_or_inv_key1_sbit0_ebit2, sq_p_index_or_imm_data1_or_inv_key1_sbit11_ebit15)
#define K_CURRENT_SGE_ID CAPRI_KEY_RANGE(IN_P, current_sge_id_sbit0_ebit6, current_sge_id_sbit7_ebit7)
#define K_CURRENT_SGE_OFFSET CAPRI_KEY_RANGE(IN_P, current_sge_offset_sbit0_ebit6, current_sge_offset_sbit31_ebit31)
#define K_NUM_SGES   CAPRI_KEY_RANGE(IN_P, num_sges_sbit0_ebit6, num_sges_sbit7_ebit7)

#define WQE_OP_TO_NUM_PKTS(_num_pkts_r, _wqe, _log_pmtu, _cf1, _cf2, _cf3)\
    slt            _cf3, OP_TYPE_FETCH_N_ADD, _wqe.base.op_type;         \
    bcf            [_cf3], _wqe_op_to_num_pkts_end;                      \
    add            _num_pkts_r, 0, 0;                                    \
    seq            _cf1,  _wqe.base.op_type, OP_TYPE_FETCH_N_ADD;        \
    seq            _cf2,  _wqe.base.op_type, OP_TYPE_CMP_N_SWAP;         \
    bcf            [_cf1 | _cf2], _wqe_op_to_num_pkts_end;               \
    add            _num_pkts_r, r0, 1;                                   \
    sll            _num_pkts_r, 1, _log_pmtu;                            \
    add            _num_pkts_r, _wqe.send.length, _num_pkts_r;           \
    sub            _num_pkts_r, _num_pkts_r, 1;                          \
    srl            _num_pkts_r, _num_pkts_r, _log_pmtu;                  \
_wqe_op_to_num_pkts_end:

%%

    .param    req_tx_bktrack_sqsge_process
    .param    req_tx_bktrack_write_back_process
    .param    req_tx_bktrack_sqcb2_write_back_process

.align
req_tx_bktrack_sqwqe_process:
    // if (in_progress == TRUE)
    //     tx_psn = k.args.tx_psn
    // else
    //     tx_psn = k.args.tx_psn - wqe_op_to_num_pkts()
    add            r6, K_SSN, r0
    seq            c1, CAPRI_KEY_FIELD(IN_P, in_progress), 1 
    bcf            [c1], wqe_bktrack
    add            r1, K_TX_PSN, r0 // Branch Delay Slot
    WQE_OP_TO_NUM_PKTS(r2, d, CAPRI_KEY_FIELD(IN_TO_S_P, log_pmtu), c1, c2, c3)
    sub            r1, K_TX_PSN, r2
    mincr          r1, 24, r0 
    sub.!c3        r6, K_SSN, 1
    mincr.!c3      r6, 24, r0

wqe_bktrack:
    // set bktrack_in_progress to true to start with
    setcf          c6, [c0]
    //if (rexmit_psn < tx_psn)
    scwlt24        c2, K_REXMIT_PSN, r1
    bcf            [!c2], wqe_match
    // set empty_rrq to false as bktracking is in progress
    // set is_op_type_read to false
    crestore       [c7, c5], r0, 0xa0 // Branch Delay Slot

    // rexmit psn range is lower than current wqe's start psn. Need to go to
    // previous wqe. Compute page_index for (cindex - 1) and see if its
    // in the same page as current wqe or previous page. If previous page,
    // complete this pass and come back to get physical address of previous
    // page using pt entry

    // log_num_wqe_per_page = log_sq_page_size - log_wqe_size
    sub            r2, K_LOG_SQ_PAGE_SIZE, CAPRI_KEY_FIELD(IN_TO_S_P, log_wqe_size)

    // prev_page_index = sq_c_index >> log_num_wqe_per_page
    srlv           r3, K_SQ_C_INDEX, r2
    // c_index = (c_index - 1) % num_wqes
    add            r4, K_SQ_C_INDEX, r0
    mincr          r4, K_LOG_NUM_WQES, -1
    seq            c2, r4, K_SQ_P_INDEX
    bcf            [c2], invalid_rexmit_psn
    // page_index = (c_index >> log_num_wqe_per_page)
    srlv           r5, r4, r2  // Branch Delay Slot

    // is previous wqe in the current page?
    //if (page_index == prev_page_index)
    bne            r5, r3, wqe_page_bktrack
    // set r5 - wqe_addr to zero to fetch wqe in a different page
    add            r5, r0, r0 // Branch Delay Slot 
    // wqe_addr = wqe_addr - (1 << log_wqe_size) 
    add            r5, CAPRI_KEY_FIELD(IN_TO_S_P, log_wqe_size), r0
    sllv           r5, 1, r5
    // check for c_index wrap around upon backtracking and based on that
    // add or subtract c_index*wqe_size to the previous wqe address
    slt            c1, K_SQ_C_INDEX, r4
    sub.!c1        r5, CAPRI_KEY_FIELD(IN_TO_S_P, wqe_addr), r5
    sll.c1         r5, r4, CAPRI_KEY_FIELD(IN_TO_S_P, log_wqe_size)
    add.c1         r5, CAPRI_KEY_FIELD(IN_TO_S_P, wqe_addr), r5

    mfspr          r7, spr_mpuid
    seq            c1, r7[4:2], CAPRI_STAGE_LAST-1
    bcf            [c1], sqcb_writeback
    
    // Upate wqe addr to the previos wqe
    phvwr CAPRI_PHV_FIELD(TO_S3_BT_P, wqe_addr), r5

    phvwrpair CAPRI_PHV_FIELD(TO_S4_BT_P, wqe_addr), r5, CAPRI_PHV_FIELD(TO_S5_BT_P, wqe_addr), r5

    phvwrpair CAPRI_PHV_FIELD(TO_S6_BT_P, wqe_addr), r5, CAPRI_PHV_FIELD(TO_S7_BT_WB_P, wqe_addr), r5

    CAPRI_RESET_TABLE_0_ARG()
    phvwrpair CAPRI_PHV_FIELD(SQ_BKTRACK_P, tx_psn), r1, CAPRI_PHV_FIELD(SQ_BKTRACK_P, ssn), r6
    phvwrpair CAPRI_PHV_FIELD(SQ_BKTRACK_P, sq_c_index), r4, CAPRI_PHV_FIELD(SQ_BKTRACK_P, sq_p_index_or_imm_data1_or_inv_key1), K_SQ_P_INDEX
  
    phvwr    CAPRI_PHV_FIELD(TO_S7_BT_WB_P, wqe_start_psn), r1

    // label and pc cannot be same, so calculate cur pc using bal 
    // and compute start pc deducting the current instruction position
    bal            r6, calculate_raw_table_pc_1
    nop            //BD Slot

calculate_raw_table_pc_1:
    // "$" here denores releative current instruction position
    sub            r6, r6, $
    CAPRI_GET_TABLE_0_K(req_tx_phv_t, r7)
    CAPRI_NEXT_TABLE_I_READ(r7, CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, r6, r5)

    nop.e
    nop 

read_or_sge_bktrack:
    seq            c5, d.base.op_type, OP_TYPE_READ
    bcf            [!c5], sge_bktrack
    nop            // Branch Delay Slot
    
    // num_pkts = rexmit_psn - wqe_start_psn
    sub           r3, K_REXMIT_PSN, r1
    mincr         r3, 24, r0
    // wqe_p->read.va += (num_pkts << log_pmtu)
    // wqe_p->read.len -= (num_pkts << log_pmtu)
    add           r2, CAPRI_KEY_FIELD(IN_TO_S_P, log_pmtu), r0
    sllv          r2, r3, r2

    // wqe_start_psn and tx_psn set to rexmit_psn as wqe's va
    // is modified to start from the rexmit_psn. if there's retransmission
    // again, then it should use this rexmit_psn as the wqe_start_psn
    add           r1, K_REXMIT_PSN, r0 
    // set bktrack_in_progress to false
    setcf         c6, [!c0]

    b            sqcb_writeback
    // set empty_rrq to true as bktrack is completed
    setcf          c7, [c0] // Branch Delay Slot

sge_bktrack:
    mfspr        r7, spr_mpuid
    seq          c1, r7[4:2], CAPRI_STAGE_LAST-1
    bcf          [c1], sqcb_writeback
    add          r5, CAPRI_KEY_FIELD(IN_TO_S_P, wqe_addr), r0 // Branch Delay Slot
    
    CAPRI_RESET_TABLE_0_ARG()

    // sge_addr = wqe_addr + TXWQE_SGE_OFFSET + (sizeof(sge_t) * current_sge_id)
    add          r3,  CAPRI_KEY_FIELD(IN_TO_S_P, wqe_addr), K_CURRENT_SGE_ID, LOG_SIZEOF_SGE_T
    add          r3, r3, TXWQE_SGE_OFFSET

    phvwrpair CAPRI_PHV_FIELD(SQ_BKTRACK_P, tx_psn), r1, CAPRI_PHV_FIELD(SQ_BKTRACK_P, ssn), r6
    phvwrpair CAPRI_PHV_FIELD(SQ_BKTRACK_P, sq_c_index), K_SQ_C_INDEX, CAPRI_PHV_FIELD(SQ_BKTRACK_P, in_progress), CAPRI_KEY_FIELD(IN_P, in_progress)
    phvwrpair CAPRI_PHV_FIELD(SQ_BKTRACK_P, current_sge_offset), 0, CAPRI_PHV_FIELD(SQ_BKTRACK_P, current_sge_id), 0
    phvwrpair CAPRI_PHV_FIELD(SQ_BKTRACK_P, num_sges), d.base.num_sges, CAPRI_PHV_FIELD(SQ_BKTRACK_P, op_type), d.base.op_type

    phvwr    CAPRI_PHV_FIELD(TO_S7_BT_WB_P, wqe_start_psn), r1
    // Always copy imm_data assuming op_type to be send. imm_data is ignored
    // imm_data & inv_key are unions
    // if op_type is not send
    phvwr CAPRI_PHV_RANGE(SQ_BKTRACK_P, sq_p_index_or_imm_data1_or_inv_key1, imm_data2_or_inv_key2), d.base.imm_data
     
    CAPRI_NEXT_TABLE0_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, req_tx_bktrack_sqsge_process, r3)

    nop.e
    nop
    
wqe_match:
    // rexmit psn is in the current wqe's psn range. It can either match
    // start psn or somewhere in between, which requires forward sge
    // walk to setup current_sge_id, current_sge_offset in the sqcb
    seq            c2, K_REXMIT_PSN, r1
    bcf            [!c2], read_or_sge_bktrack
    // first wqe itself matches, no change to c_index
    add            r4, r0, K_SQ_C_INDEX // Branch Delay Slot

    // set empty_rrq to true as bktrack is completed
    setcf          c7, [c0]

    // bktrack_in_progres = False
    setcf          c6, [!c0]

    // set r5 - wqe_addr to zero on match
    add            r5, r0, r0

    // Fall throguh to retransmit from this matching wqe's start psn

wqe_page_bktrack:
    // need to go to previous page, drop the packet and reschedule
    // fall through to cb writeback

sqcb_writeback:
    phvwr          p.common.p4_intr_global_drop, 1

    phvwr     CAPRI_PHV_FIELD(TO_S3_BT_P, wqe_addr), r5
    phvwrpair CAPRI_PHV_FIELD(TO_S4_BT_P, wqe_addr), r5, CAPRI_PHV_FIELD(TO_S5_BT_P, wqe_addr), r5
    phvwrpair CAPRI_PHV_FIELD(TO_S6_BT_P, wqe_addr), r5, CAPRI_PHV_FIELD(TO_S7_BT_WB_P, wqe_addr), r5

    CAPRI_RESET_TABLE_1_ARG()
    phvwrpair CAPRI_PHV_FIELD(SQCB2_WRITE_BACK_P, tx_psn), r1, CAPRI_PHV_FIELD(SQCB2_WRITE_BACK_P, ssn), r6
    // Assume send and copy imm_data, inv_key. These fields are looked into
    // only if op_type is send/write, imm_data and inv_key are union members
    phvwr     CAPRI_PHV_FIELD(SQCB2_WRITE_BACK_P, imm_data_or_inv_key), d.base.imm_data
    phvwrpair CAPRI_PHV_FIELD(SQCB2_WRITE_BACK_P, op_type), d.base.op_type, CAPRI_PHV_FIELD(SQCB2_WRITE_BACK_P, sq_cindex), r4
    phvwr.c6 CAPRI_PHV_FIELD(SQCB2_WRITE_BACK_P, bktrack_in_progress), 1
    phvwr.c5 CAPRI_PHV_FIELD(SQCB2_WRITE_BACK_P, msg_psn), r3

    phvwr    CAPRI_PHV_FIELD(TO_S7_BT_WB_P, wqe_start_psn), r1

    SQCB2_ADDR_GET(r5)
    CAPRI_NEXT_TABLE1_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, req_tx_bktrack_sqcb2_write_back_process, r5)

    CAPRI_RESET_TABLE_0_ARG()
    phvwr.c6 CAPRI_PHV_FIELD(SQCB0_WRITE_BACK_P, bktrack_in_progress), 1
    phvwr.c5 CAPRI_PHV_FIELD(SQCB0_WRITE_BACK_P, current_sge_offset), r2
    phvwrpair  CAPRI_PHV_FIELD(SQCB0_WRITE_BACK_P, num_sges), d.base.num_sges, CAPRI_PHV_FIELD(SQCB0_WRITE_BACK_P, sq_c_index), r4
    phvwr.c7 CAPRI_PHV_FIELD(SQCB0_WRITE_BACK_P, empty_rrq_bktrack), 1

    SQCB0_ADDR_GET(r5)
    CAPRI_NEXT_TABLE0_READ_PC_E(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, req_tx_bktrack_write_back_process, r5)

invalid_rexmit_psn:
    phvwr        p.{rdma_feedback.completion.status, rdma_feedback.completion.error}, (CQ_STATUS_LOCAL_QP_OPER_ERR << 1 | 1)
    phvwr          CAPRI_PHV_FIELD(phv_global_common, _error_disable_qp),  1

    SQCB0_ADDR_GET(r5)
    CAPRI_NEXT_TABLE0_READ_PC_E(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, req_tx_bktrack_write_back_process, r5)
