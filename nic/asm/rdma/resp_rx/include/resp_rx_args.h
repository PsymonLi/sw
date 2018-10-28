#ifndef __RESP_RX_ARGS_H
#define __RESP_RX_ARGS_H

// we still need to retain these structures due to the special nature of wqe/pt 
// functions which deal with Table 0 and/or Table 1.
// TBD: either auto-generate this structure from P4 or migrate to regular phvwr
struct resp_rx_to_stage_wb1_info_t {
    curr_wqe_ptr            :   64;
    my_token_id             :    8;
    cqcb_base_addr_hi       :   24;
    log_num_cq_entries      :    4;
    in_progress             :    1;
    incr_nxt_to_go_token_id :    1;
    incr_c_index            :    1;
    update_wqe_ptr          :    1;
    update_num_sges         :    1;
    current_sge_id          :    8;
    num_sges                :    8;
    skip_completion         :    1;
    pad                     :    6;
};

struct resp_rx_to_stage_cqpt_info_t {
    cqcb_base_addr_hi       : 24;
    log_num_cq_entries      : 4;
    pad                     : 100;
};

#if 0
#include "capri.h"
#include "common_phv.h"

struct resp_rx_to_stage_backtrack_info_t {
    va: 64;
    r_key: 32;
    len: 32;
};

struct resp_rx_to_stage_stats_info_t {
    bytes: 16;
    pad: 112;
};

struct resp_rx_to_stage_recirc_info_t {
    remaining_payload_bytes: 16;
    curr_wqe_ptr: 64;
    current_sge_offset: 32;
    current_sge_id: 8;
    num_sges: 8;
};

struct resp_rx_to_stage_atomic_info_t {
    rsqwqe_ptr: 64;
    pad: 64; 
};

//Right now, worst case RDMA headers are overflowing by 64bits at maximum
//from stage0 app_data table to rdma_params_table.
//That data is passed blindly by rdma_params_table in stage0 to stage2 of
//regular program
struct resp_rx_to_stage_ext_hdr_info_t {
    ext_hdr_data: 96;
    pad: 32;
};

struct resp_rx_s0_info_t {
    union {
        struct resp_rx_to_stage_backtrack_info_t backtrack;
    };
};

struct resp_rx_s1_info_t {
    union {
        struct resp_rx_to_stage_backtrack_info_t backtrack;
        struct resp_rx_to_stage_recirc_info_t recirc;
        struct resp_rx_to_stage_atomic_info_t atomic;
    };
};

struct resp_rx_s2_info_t {
    union {
        struct resp_rx_to_stage_backtrack_info_t backtrack;
        struct resp_rx_to_stage_ext_hdr_info_t ext_hdr;
    };
};

struct resp_rx_s3_info_t {
    union {
        struct resp_rx_to_stage_backtrack_info_t backtrack;
        struct resp_rx_to_stage_wb1_info_t wb1;
    };
};

struct resp_rx_s4_info_t {
    union {
        struct resp_rx_to_stage_backtrack_info_t backtrack;
    };
};

struct resp_rx_s5_info_t {
    union {
        struct resp_rx_to_stage_backtrack_info_t backtrack;
        struct resp_rx_to_stage_cqpt_info_t cqpt;
    };
};

struct resp_rx_s6_info_t {
    union {
        struct resp_rx_to_stage_backtrack_info_t  backtrack;
    };
};

struct resp_rx_s7_info_t {
    union {
        struct resp_rx_to_stage_backtrack_info_t backtrack;
        struct resp_rx_to_stage_stats_info_t stats;
    };
};

struct resp_rx_to_stage_t {
    union {
        struct resp_rx_s0_info_t s0;
        struct resp_rx_s1_info_t s1;
        struct resp_rx_s2_info_t s2;
        struct resp_rx_s3_info_t s3;
        struct resp_rx_s4_info_t s4;
        struct resp_rx_s5_info_t s5;
        struct resp_rx_s6_info_t s6;
        struct resp_rx_s7_info_t s7;
    };
};
#endif

#endif //__RESP_RX_ARGS_H
