#ifndef __RESP_TX_H
#define __RESP_TX_H
#include "capri.h"
#include "types.h"
#include "resp_tx_args.h"
#include "INGRESS_p.h"
#include "ingress.h"
#include "common_phv.h"
#include "p4/common/defines.h"

#define RESP_TX_MAX_DMA_CMDS            16
#define RESP_TX_DMA_CMD_START           0
#define RESP_TX_DMA_CMD_RQ_PREFETCH_WQE_RD          0
#define RESP_TX_DMA_CMD_RQ_PREFETCH_WQE_TO_HBM      1
#define RESP_TX_DMA_CMD_SET_PROXY_PINDEX            2
#define RESP_TX_DMA_CMD_INTRINSIC       3
#define RESP_TX_DMA_CMD_TXDMA_INTRINSIC 4
#define RESP_TX_DMA_CMD_COMMON_P4PLUS   5
#define RESP_TX_DMA_CMD_HDR_TEMPLATE    6
#define RESP_TX_DMA_CMD_BTH             7
#define RESP_TX_DMA_CMD_AETH            8
#define RESP_TX_DMA_CMD_CNP_RSVD        8 // CNP packets do not have AETH header. Re-using index.
#define RESP_TX_DMA_CMD_ATOMICAETH      9 
#define RESP_TX_DMA_CMD_ACK_ICRC        9
#define RESP_TX_DMA_CMD_PYLD_BASE       9 // consumes 3 DMA commands
#define RESP_TX_DMA_CMD_RDMA_ERR_FEEDBACK   13
#define RESP_TX_DMA_CMD_PAD_ICRC       (RESP_TX_MAX_DMA_CMDS - 2)
#define RESP_TX_DMA_CMD_UDP_OPTIONS    (RESP_TX_MAX_DMA_CMDS - 1)


#define RESP_TX_DMA_CMD_START_FLIT_ID   8 // flits 8-11 are used for dma cmds

// phv 
struct resp_tx_phv_t {
    // dma commands
    /* flit 11 */
    dma_cmd12 : 128;
    dma_cmd13 : 128;
    dma_cmd14 : 128;
    dma_cmd15 : 128;

    /* flit 10 */
    dma_cmd8  : 128;
    dma_cmd9  : 128;
    dma_cmd10 : 128;
    dma_cmd11 : 128;

    /* flit 9 */
    dma_cmd4 : 128;
    dma_cmd5 : 128;
    dma_cmd6 : 128;
    dma_cmd7 : 128;

    /* flit 8 */
    dma_cmd0 : 128;                               // 16B
    dma_cmd1 : 128;                               // 16B
    dma_cmd2 : 128;                               // 16B
    dma_cmd3 : 128;                               // 16B

    /* flit 7 */
    struct phv_intr_global_t p4_intr_global;        // 17B
    struct phv_intr_p4_t p4_intr;                   // 5B
    struct phv_intr_rxdma_t p4_intr_rxdma;          // 10B
    struct p4_to_p4plus_roce_header_t p4_to_p4plus; // 20B
    union {
        struct rdma_feedback_t rdma_feedback;       // 11B
        struct rq_prefetch_info_t rq_prefetch;      // 2B
    }; // rqpt is loaded after writeback
    rsvd2    : 8;                                   // 1B


    /* flit 6 */
    struct rdma_atomicaeth_t atomicaeth;         //  8B
    union {                                      // 16B
        struct rdma_cnp_rsvd_t cnp_rsvd;         // 16B
        struct rdma_aeth_t aeth;                 //  4B
    }; // CNP packets do not have aeth header.
    struct rdma_bth_t bth;                      // 12B
    struct p4plus_to_p4_header_t p4plus_to_p4;  // 20B
    rsvd1    :   8;                              // 1B
    pad      :  24;                             //  3B
    icrc     :  32;                             //  4B

    //***NOTE: Keep 4 bytes after pad within this flit so that it can be copied as ICRC data into phv
    //ICRC data does not need to be zeroed as Capri overwrites icrc after computation

    // common tx (flit 0 - 5)
    struct phv_ common;
};

// we still need to retain these structures due to the special nature backtrack logic
// this structure should be same as header_type resp_tx_to_stage_bt_info_t defined in p4
// TBD: auto-generate this structure from P4

struct resp_tx_to_stage_bt_info_t {
    log_rsq_size: 5;
    log_pmtu: 5;
    rsq_base_addr: 32;
    bt_cindex: 16;
    end_index: 16;
    search_index: 16;
    curr_read_rsp_psn: 24;
    read_rsp_in_progress: 1;
    bt_in_progress: 1;
    rsvd: 12;
};
 
#if 0
struct resp_tx_phv_global_t {
    struct phv_global_common_t common;
};

// stage to stage argument structures

//20
#if 0
struct resp_tx_rqcb_to_rqcb1_info_t {
    rsqwqe_addr: 64;
    log_pmtu: 5;
    serv_type: 3;
    timer_event_process: 1;
    curr_read_rsp_psn: 24;
    read_rsp_in_progress: 1;
    pad: 62;
};

struct resp_tx_rqcb1_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rqcb_to_rqcb1_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};
#endif

struct resp_tx_rqcb_to_rqcb2_info_t {
    rsqwqe_addr: 64;
    curr_read_rsp_psn: 24;
    log_pmtu: 5;
    serv_type: 3;
    header_template_addr: 32;
    header_template_size: 8;
    read_rsp_in_progress: 1;
    pad: 23;
};

struct resp_tx_rqcb2_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rqcb_to_rqcb2_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};

//20
struct resp_tx_rqcb_to_ack_info_t {
    header_template_addr: 32;
    pad: 128;
};

struct resp_tx_ack_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rqcb_to_ack_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};

//20
struct resp_tx_rqcb2_to_rsqwqe_info_t {
    curr_read_rsp_psn: 24;
    log_pmtu: 5;
    serv_type: 3;
    header_template_addr: 32;
    header_template_size: 8;
    read_rsp_in_progress: 1;
    pad: 87;
};

struct resp_tx_rsqwqe_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rqcb2_to_rsqwqe_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};

//20
struct resp_tx_rsqwqe_to_rkey_info_t {
    transfer_va: 64;
    header_template_addr: 32;
    curr_read_rsp_psn: 24;
    log_pmtu: 5;
    key_id: 1;
    send_aeth: 1;
    last_or_only: 1;
    transfer_bytes: 12;
    header_template_size: 8;
    pad: 12;
};

struct resp_tx_rsqrkey_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rsqwqe_to_rkey_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};

//20
struct resp_tx_rqcb0_write_back_info_t {
    curr_read_rsp_psn: 24;
    read_rsp_in_progress: 1;
    rate_enforce_failed: 1;
    pad: 134;
};

struct resp_tx_rqcb0_write_back_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rqcb0_write_back_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};

//20
struct resp_tx_rkey_to_ptseg_info_t {
    pt_seg_offset: 32;
    pt_seg_bytes: 32;
    dma_cmd_start_index: 8;
    log_page_size: 5;
    tbl_id: 2;
    pad: 81;
};

struct resp_tx_rsqptseg_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rkey_to_ptseg_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};


//20
struct resp_tx_rsq_backtrack_adjust_info_t {
    adjust_rsq_c_index: 16;
    rsq_bt_p_index: 16;
    pad: 128;
};

struct resp_tx_rsq_backtrack_adjust_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rsq_backtrack_adjust_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};

struct resp_tx_rqcb_to_cnp_info_t {
    new_c_index: 16;
    pad: 144;   
};  
    
struct resp_tx_cnp_process_k_t {
    struct capri_intrinsic_raw_k_t intrinsic;
    struct resp_tx_rqcb_to_cnp_info_t args;
    struct resp_tx_to_stage_t to_stage;
    struct phv_global_common_t global;
};  
#endif


#endif //__RESP_TX_H
