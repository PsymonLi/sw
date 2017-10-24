#ifndef __REQ_TX_ARGS_H
#define __REQ_TX_ARGS_H

#include "capri.h"
#include "common_phv.h"

struct req_tx_sqcb_to_pt_info_t {
    page_offset                    : 16;
    page_seg_offset                : 3;
    remaining_payload_bytes        : 32;
    rrq_p_index                    : 8;
    log_pmtu                       : 5;
    pd                             : 32;
    ud                             : 1;
    pad                            : 63; // pad to 160bits for S2S data
};

struct req_tx_sqcb_to_wqe_info_t {
    in_progress                    : 1;
    log_pmtu                       : 5;
    li_fence_cleared               : 1;
    current_sge_id                 : 8;
    num_valid_sges                 : 8;
    current_sge_offset             : 32;
    remaining_payload_bytes        : 16;
    rrq_p_index                    : 8;
    pd                             : 32;
    ud                             : 1;
    pad                            : 48;
};

struct req_tx_wqe_to_sge_info_t {
    in_progress                    : 1;
    op_type                        : 8;
    first                          : 1;
    current_sge_id                 : 8;
    num_valid_sges                 : 8;
    current_sge_offset             : 32;
    remaining_payload_bytes        : 16;
    payload_offset                 : 16;
    dma_cmd_start_index            : 6; // TODO Different from "C" code due to space scarcity
    imm_data                       : 32;
    union {
        inv_key                        : 32;
        ah_handle                      : 32;
    };
};

struct req_tx_sge_to_lkey_info_t {
    sge_va                        : 64;
    key_id                        : 8;
    sge_bytes                     : 16;
    dma_cmd_start_index           : 8;
    sge_index                     : 8;
    dma_cmd_eop                   : 1;
    pad                           : 55; 
};

struct req_tx_lkey_to_ptseg_info_t {
    pt_offset                    : 32;
    log_page_size                : 5;
    pt_bytes                     : 16;
    dma_cmd_start_index          : 8;
    sge_index                    : 8;
    dma_cmd_eop                  : 1;
    pad                          : 90;
};

struct atomic_t {
    struct sge_t sge;
    pad                           : 8;
};

struct rd_t {
    read_len                      : 32;
    num_sges                      : 8;
    pad                           : 96;
};

struct send_wr_t {
    current_sge_offset            : 32;
    current_sge_id                : 8;
    num_sges                      : 8;
    imm_data                      : 32;
    union {
        inv_key                       : 32;
        ah_handle                     : 32;
    };
    pad                           : 24;
};

union op_t {
    struct rd_t rd;
    struct send_wr_t send_wr;
    struct atomic_t atomic;
};

struct req_tx_rrqwqe_to_hdr_info_t {
    rsvd                          : 2;
    last                          : 1;
    //keep op_type...first contiguous
    op_type                       : 4;
    first                         : 1;
    tbl_id                        : 3;
    log_pmtu                      : 5;
    rrq_p_index                   : 8;
    union op_t op;
};

struct req_tx_sqcb_write_back_info_t {
    busy                         : 1;
    in_progress                  : 1;
    //keep op_type...first contiguous
    op_type                      : 8;
    first                        : 1;
    last                         : 1;
    tbl_id                       : 3;
    set_fence                    : 1;
    set_li_fence                 : 1;
    set_bktrack                  : 1;
    empty_rrq_bktrack            : 1;
    release_cb1_busy             : 1;
    num_sges                     : 8;
    current_sge_id               : 8;
    sq_c_index                   : 16;
    current_sge_offset           : 32;
    pad                          : 76;
};

// Note: Do not change the order of log_pmtu to num_sges as
// program uses concatenated copy like k.{log_pmtu...num_sges}
// from  req_tx_sqcb0_to_sqcb1_info to req_tx_sqcb1_to_bktrack_wqe_info
struct req_tx_sqcb0_to_sqcb1_info_t {
    sq_c_index                    : 16;
    sq_p_index                    : 16;
    in_progress                   : 1;
    bktrack_in_progress           : 1;
    current_sge_offset            : 32;
    current_sge_id                : 8;
    num_sges                      : 8;
    update_credits                : 1;
    bktrack                       : 1; 
    pad                           : 76;
};

struct req_tx_sqcb1_to_credits_info_t {
    need_credits                 : 1;
    pad                          : 159;
};

// Note: Do not change the order of sq_c_index to num_sges as
// program uses concatenated copy like k.{log_pmtu...num_sges}
// from  req_tx_sqcb0_to_sqcb1_info to req_tx_sq_bktrack_info
//
struct req_tx_sq_bktrack_info_t {
    sq_c_index                    : 16;
    sq_p_index                    : 16;
    in_progress                   : 1;
    bktrack_in_progress           : 1;
    current_sge_offset            : 32;
    current_sge_id                : 8;
    num_sges                      : 8;
    rexmit_psn                    : 24;
    tx_psn                        : 24;
    op_type                       : 5;
    pad                           : 23;
};

struct req_tx_sqcb1_write_back_info_t {
    wqe_start_psn                 : 24;
    tx_psn                        : 24;
    skip_wqe_start_psn            : 1;
    tbl_id                        : 3;
    pad                           : 108;
};

struct req_tx_to_stage_t {
    wqe_addr                     : 64;
    pt_base_addr                 : 32;
    log_pmtu                     : 5;
    log_sq_page_size             : 5;
    log_wqe_size                 : 5;
    log_num_wqes                 : 5;
    pad                          : 12;
};

/*
struct req_tx_sqcb_to_wqe_info_k_t {
    struct capri_intrinsic_k_t intrinsic;
    union {
        global_data: GLOBAL_DATA_WIDTH;
        struct req_tx_phv_global_t global;
    };
    union {
       s2s_data: S2S_DATA_WIDTH;
       struct req_tx_sqcb_to_wqe_info_t params;
    };
};
*/

union args_union_t {
    //struct req_tx_sqcb_to_wqe_info_t sqcb_to_wqe;
    dummy: 32;
};

#endif //__REQ_TX_ARGS_H
