#include "capri.h"
#include "resp_rx.h"
#include "rqcb.h"
#include "common_phv.h"

struct resp_rx_phv_t p;
//struct resp_rx_rqcb1_write_back_process_k_t k;
struct resp_rx_s3_t2_k k;
struct rqcb1_t d;

#define IN_TO_S_P   to_s3_wb1_info
#define IN_P        t2_s2s_rqcb1_write_back_info

#define DMA_CMD_BASE r1
#define GLOBAL_FLAGS r2
#define TMP r3
#define DB_ADDR r4
#define DB_DATA r5
#define RQCB1_ADDR r6
#define T2_ARG r7

#define CQCB_ADDR r6
#define KT_BASE_ADDR r6
#define KEY_ADDR r6
#define NEW_RSQ_P_INDEX r6

#define K_CURR_WQE_PTR CAPRI_KEY_RANGE(IN_P, curr_wqe_ptr_sbit0_ebit7, curr_wqe_ptr_sbit48_ebit63)
%%
    .param  resp_rx_cqcb_process
    .param  resp_rx_inv_rkey_process
    .param  resp_rx_recirc_mpu_only_process

.align
resp_rx_rqcb1_write_back_process:
    //seq             c1, k.to_stage.s3.wb1.my_token_id, d.nxt_to_go_token_id
    seq             c1, CAPRI_KEY_FIELD(IN_TO_S_P, my_token_id), d.nxt_to_go_token_id
    bcf             [!c1], recirc
    crestore        [c3, c2], CAPRI_KEY_RANGE(IN_P, incr_nxt_to_go_token_id, incr_c_index), 0x3 // BD Slot
    # c3 - incr_nxt_to_go_token_id, c2 - incr_c_index
    tbladd.c3       d.nxt_to_go_token_id, 1 

    // if NAK/RNR NAK in stage 0, we set skip_completion to 1
    seq             c1, CAPRI_KEY_FIELD(IN_P, skip_completion), 1
    CAPRI_SET_TABLE_2_VALID_CE(c1, 0) 

    tblmincri.c2    PROXY_RQ_C_INDEX, d.log_num_wqes, 1 // Exit Slot
    bbeq            K_GLOBAL_FLAG(_only), 1, skip_updates_for_only
    tblwr           d.in_progress, CAPRI_KEY_FIELD(IN_P, in_progress)   //BD Slot

    // If atomic request, set busy to 0
    add             GLOBAL_FLAGS, r0, K_GLOBAL_FLAGS
    crestore        [c3, c2], GLOBAL_FLAGS, (RESP_RX_FLAG_ATOMIC_CSWAP | RESP_RX_FLAG_ATOMIC_FNA)
    # c3: cswap, c2: fna
    setcf           c1, [c3 | c2]
    # c1: atomic
    tblwr.c1        d.busy, 0

    // updates for multi-packet case
    tblwr       d.{current_sge_offset...current_sge_id}, CAPRI_KEY_RANGE(IN_P, current_sge_offset_sbit0_ebit7, current_sge_id)
    crestore    [c3, c2], CAPRI_KEY_RANGE(IN_P, update_wqe_ptr, update_num_sges), 0x3
    #c3 - update_wqe_ptr
    #c2 - update_num_sges
    tblwr.c3    d.curr_wqe_ptr, K_CURR_WQE_PTR
    tblwr.c2    d.num_sges, CAPRI_KEY_FIELD(IN_P, num_sges)

    
skip_updates_for_only:
    
    // invoke CQCB always 
    // though this is inefficient, we have to do this because
    // lkey validation happening in parallel to 
    // write back (current program) may decide to put the QP into 
    // error disable case in which case it may turn on completion.
    // We can reliably parse that info only in next stage, hence
    // always load cqcb, but update cqcb only when real completion is
    // necessary.
    // other option is to recirc the packet if lkey results in error
    // disabling qp and come back to cqcb in the next pass.


    RESP_RX_CQCB_ADDR_GET(CQCB_ADDR, d.cq_id)
    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS, resp_rx_cqcb_process, CQCB_ADDR)


    // if invalidate rkey is present, invoke it by loading appopriate
    // key entry, else load the same program as MPU only. 
    // in any case, this inv_rkey_process will eventually call stats
    bbeq            K_GLOBAL_FLAG(_inv_rkey), 0, inv_rkey_mpu_only
    KT_BASE_ADDR_GET2(KT_BASE_ADDR, TMP)
    add             TMP, r0, CAPRI_KEY_FIELD(IN_TO_S_P, inv_r_key)
    KEY_ENTRY_ADDR_GET(KEY_ADDR, KT_BASE_ADDR, TMP)

    CAPRI_NEXT_TABLE3_READ_PC(CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_256_BITS, resp_rx_inv_rkey_process, KEY_ADDR)
    nop.e
    nop

inv_rkey_mpu_only:
    CAPRI_NEXT_TABLE3_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, resp_rx_inv_rkey_process, r0)
    nop.e
    nop

recirc:
    // fire an mpu only program which will eventually set table 0 valid bit to 1 prior to recirc
    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, resp_rx_recirc_mpu_only_process, r0)

    phvwr   p.common.p4_intr_recirc, 1
    phvwr   p.common.rdma_recirc_recirc_reason, CAPRI_RECIRC_REASON_INORDER_WORK_DONE
    // invalidate table ?
    CAPRI_SET_TABLE_2_VALID(0)

exit:
    nop.e
    nop
