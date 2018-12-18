/***********************************************************************/
/* rdma_aq_txdma.p4 */
/***********************************************************************/

#include "../common-p4+/common_txdma_dummy.p4"

/**** table declarations ****/

#define tx_table_s0_t0 aq_tx_s0_t0
#define tx_table_s0_t1 aq_tx_s0_t1
#define tx_table_s0_t2 aq_tx_s0_t2
#define tx_table_s0_t3 aq_tx_s0_t3

#define tx_table_s1_t0 aq_tx_s1_t0
#define tx_table_s1_t1 aq_tx_s1_t1
#define tx_table_s1_t2 aq_tx_s1_t2
#define tx_table_s1_t3 aq_tx_s1_t3

#define tx_table_s2_t0 aq_tx_s2_t0
#define tx_table_s2_t1 aq_tx_s2_t1
#define tx_table_s2_t2 aq_tx_s2_t2
#define tx_table_s2_t3 aq_tx_s2_t3

#define tx_table_s3_t0 aq_tx_s3_t0
#define tx_table_s3_t1 aq_tx_s3_t1
#define tx_table_s3_t2 aq_tx_s3_t2
#define tx_table_s3_t3 aq_tx_s3_t3

#define tx_table_s4_t0 aq_tx_s4_t0
#define tx_table_s4_t1 aq_tx_s4_t1
#define tx_table_s4_t2 aq_tx_s4_t2
#define tx_table_s5_t0 aq_tx_s5_t0
#define tx_table_s5_t1 aq_tx_s5_t1
#define tx_table_s5_t2 aq_tx_s5_t2
#define tx_table_s5_t3 aq_tx_s5_t3

#define tx_table_s6_t0 aq_tx_s6_t0
#define tx_table_s6_t1 aq_tx_s6_t1
#define tx_table_s6_t2 aq_tx_s6_t2
#define tx_table_s6_t3 aq_tx_s6_t3

#define tx_table_s7_t0 aq_tx_s7_t0
#define tx_table_s7_t1 aq_tx_s7_t1
#define tx_table_s7_t2 aq_tx_s7_t2
#define tx_table_s7_t3 aq_tx_s7_t3

#define tx_table_s0_t0_action aq_tx_aqcb_process

#define tx_table_s1_t0_action aq_tx_aqwqe_process

#define tx_table_s2_t0_action aq_tx_feedback_process_s3
#define tx_table_s2_t1_action aq_tx_modify_qp_2_process

#define tx_table_s3_t0_action aq_tx_feedback_process_s3
#define tx_table_s3_t1_action aq_tx_rqcb0_process_s3
#define tx_table_s3_t2_action aq_tx_sqcb2_process_s3

#define tx_table_s4_t0_action aq_tx_feedback_process_s4
#define tx_table_s4_t1_action aq_tx_rqcb0_process_s4
#define tx_table_s4_t2_action aq_tx_sqcb2_process_s4

#define tx_table_s5_t0_action aq_tx_feedback_process_s5
#define tx_table_s5_t1_action aq_tx_rqcb0_process
#define tx_table_s5_t2_action aq_tx_sqcb2_process

#define tx_table_s6_t0_action aq_tx_feedback_process_s6
#define tx_table_s6_t2_action aq_tx_sqcb0_process

#define tx_table_s7_t0_action aq_tx_feedback_process
#define tx_table_s7_t3_action aq_tx_stats_process

#include "../common-p4+/common_txdma.p4"
#include "./rdma_txdma_headers.p4"

/**** Macros ****/

#define GENERATE_GLOBAL_K \
    modify_field(phv_global_common_scr.lif, phv_global_common.lif);\
    modify_field(phv_global_common_scr.qid, phv_global_common.qid);\
    modify_field(phv_global_common_scr.qtype, phv_global_common.qtype);\
    modify_field(phv_global_common_scr.cb_addr, phv_global_common.cb_addr);\
    modify_field(phv_global_common_scr.pt_base_addr_page_id, phv_global_common.pt_base_addr_page_id);\
    modify_field(phv_global_common_scr.log_num_pt_entries, phv_global_common.log_num_pt_entries);\
    modify_field(phv_global_common_scr.pad, phv_global_common.pad);\

/**** header definitions ****/

header_type phv_global_common_t {
    fields {
        lif                              :   11;
        qid                              :   24;
        qtype                            :    3;
        cb_addr                          :   25;
        pt_base_addr_page_id             :   22;
        log_num_pt_entries               :    5;
        pad                              :   24;
    }
}

header_type aq_tx_to_stage_wqe_info_t {
    fields {
        cqcb_base_addr_hi                :   24;
        sqcb_base_addr_hi                :   24;
        rqcb_base_addr_hi                :   24;
        log_num_cq_entries               :    4;
        ah_base_addr_page_id             :   22;        
        barmap_base                      :   10;
        barmap_size                      :    8;
        pad                              :   12;
    }
}

header_type aq_tx_to_stage_wqe2_info_t {
    fields {
        ah_base_addr_page_id             :   22;
        sqcb_base_addr_hi                :   24;
        rqcb_base_addr_hi                :   24;
        pad                              :   58;
    }
}

header_type aq_tx_to_stage_sqcb_info_t {
    fields {
        cb_addr                          :   34;
        rqcb_base_addr_hi                :   24;
        tx_psn_valid                     :    1;
        tx_psn                           :   24;
        local_ack_timeout                :    5;
        local_ack_timeout_valid          :    1;
        err_retry_count                  :    3;
        err_retry_count_valid            :    1;
        q_key                            :   32;
        q_key_valid                      :    1;
        pad                              :    2;
    }
}

header_type aq_tx_to_stage_rqcb_info_t {
    fields {
        q_key                            :    32;
        q_key_valid                      :     1;
        pad                              :    95;
    }
}

header_type aq_tx_to_stage_fb_stats_info_t {
    fields {
        cb_addr                          :   34;
        wqe_id                           :   16;
        nop                              :    1;
        create_cq                        :    1;
        create_qp                        :    1;
        reg_mr                           :    1;
        stats_hdrs                       :    1;
        stats_vals                       :    1;
        dereg_mr                         :    1;
        resize_cq                        :    1;
        destroy_cq                       :    1;
        modify_qp                        :    1;
        query_qp                         :    1;
        destroy_qp                       :    1;
        stats_dump                       :    1;
        aq_cmd_done                      :    1;
        pad                              :   64;
    }
}

header_type aq_tx_aqcb_to_modqp_t {
    fields {
        state               :  3;
        pmtu_log2           :  5;
        ah_len              :  8;
        ah_addr             : 32;
        rrq_base_addr       : 32;
        rrq_depth_log2      :  5;
        rsq_base_addr       : 32;
        rsq_depth_log2      :  5;
        state_valid         :  1;
        pmtu_valid          :  1;
        av_valid            :  1;
        rsq_valid           :  1;
        rrq_valid           :  1;
        qid                 : 24;
        pad                 :  9;
    }
}

// TODO: migrate to phv global later.
// phv global has only 25 bits for cb_addr. if we change it to 28bits, 
// lot of macros cannot be reused in aq asm code and need to be redefined 
// specific to aq. hence for now cb_addr is being populated in s2s.
header_type aq_tx_aqcb_to_wqe_t {
    fields {
        cb_addr             :  34;
        map_count_completed :  32;
        pad                 :  94;
    }
}

header_type aq_tx_stats_info_t {
    fields {
        pad                              :  160;
    }
}


/**** global header unions ****/

@pragma pa_header_union ingress common_global
metadata phv_global_common_t phv_global_common;
@pragma scratch_metadata
metadata phv_global_common_t phv_global_common_scr;

/**** to stage header unions ****/

//To-Stage-0

//To-Stage-1
@pragma pa_header_union ingress to_stage_1
metadata aq_tx_to_stage_wqe_info_t to_s1_info;
@pragma scratch_metadata
metadata aq_tx_to_stage_wqe_info_t to_s1_info_scr;

//To-Stage-2
@pragma pa_header_union ingress to_stage_2
metadata aq_tx_to_stage_wqe2_info_t to_s2_info;
@pragma scratch_metadata
metadata aq_tx_to_stage_wqe2_info_t to_s2_info_scr;

//To-Stage-3
@pragma pa_header_union ingress to_stage_3
metadata aq_tx_to_stage_sqcb_info_t to_s3_info;
@pragma scratch_metadata
metadata aq_tx_to_stage_sqcb_info_t to_s3_info_scr;

//To-Stage-4
@pragma pa_header_union ingress to_stage_4
metadata aq_tx_to_stage_sqcb_info_t to_s4_info;
@pragma scratch_metadata
metadata aq_tx_to_stage_sqcb_info_t to_s4_info_scr;

//To-Stage-5
@pragma pa_header_union ingress to_stage_5
metadata aq_tx_to_stage_sqcb_info_t to_s5_info;
@pragma scratch_metadata
metadata aq_tx_to_stage_sqcb_info_t to_s5_info_scr;

//To-Stage-6
@pragma pa_header_union ingress to_stage_6
metadata aq_tx_to_stage_sqcb_info_t to_s6_info;
@pragma scratch_metadata
metadata aq_tx_to_stage_sqcb_info_t to_s6_info_scr;

//To-Stage-7
@pragma pa_header_union ingress to_stage_7 to_s7_fb_stats_info
metadata aq_tx_to_stage_fb_stats_info_t to_s7_fb_stats_info;
@pragma scratch_metadata
metadata aq_tx_to_stage_fb_stats_info_t to_s7_fb_stats_info_scr;


/**** stage to stage header unions ****/

//Table-0
@pragma pa_header_union ingress common_t0_s2s t0_s2s_aqcb_to_wqe_info
metadata aq_tx_aqcb_to_wqe_t t0_s2s_aqcb_to_wqe_info;
@pragma scratch_metadata
metadata aq_tx_aqcb_to_wqe_t t0_s2s_aqcb_to_wqe_info_scr;

//Table-1
@pragma pa_header_union ingress common_t1_s2s t1_s2s_wqe2_to_rqcb0_info 

metadata aq_tx_aqcb_to_modqp_t t1_s2s_wqe2_to_rqcb0_info;
@pragma scratch_metadata
metadata aq_tx_aqcb_to_modqp_t t1_s2s_wqe2_to_rqcb0_info_scr;

//Table-2
@pragma pa_header_union ingress common_t2_s2s t2_s2s_wqe2_to_sqcb2_info t2_s2s_sqcb2_to_sqcb0_info

metadata aq_tx_aqcb_to_modqp_t t2_s2s_wqe2_to_sqcb2_info;
@pragma scratch_metadata
metadata aq_tx_aqcb_to_modqp_t t2_s2s_wqe2_to_sqcb2_info_scr;

metadata aq_tx_aqcb_to_modqp_t t2_s2s_sqcb2_to_sqcb0_info;
@pragma scratch_metadata
metadata aq_tx_aqcb_to_modqp_t t2_s2s_sqcb2_to_sqcb0_info_scr;

//Table-3
metadata aq_tx_stats_info_t t3_s2s_stats_info;
@pragma scratch_metadata
metadata aq_tx_stats_info_t t3_s2s_stats_info_scr;

/*
 * Stage 0 table 0 action
 */
action aq_tx_aqcb_process () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage

    // stage to stage
}

action aq_tx_aqwqe_process () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage
    modify_field(to_s1_info_scr.cqcb_base_addr_hi, to_s1_info.cqcb_base_addr_hi);
    modify_field(to_s1_info_scr.sqcb_base_addr_hi, to_s1_info.sqcb_base_addr_hi);
    modify_field(to_s1_info_scr.rqcb_base_addr_hi, to_s1_info.rqcb_base_addr_hi);
    modify_field(to_s1_info_scr.log_num_cq_entries, to_s1_info.log_num_cq_entries);
    modify_field(to_s1_info_scr.ah_base_addr_page_id, to_s1_info.ah_base_addr_page_id);
    modify_field(to_s1_info_scr.barmap_base, to_s1_info.barmap_base);
    modify_field(to_s1_info_scr.barmap_size, to_s1_info.barmap_size);
    modify_field(to_s1_info_scr.pad, to_s1_info.pad);
    
    // stage to stage
    modify_field(t0_s2s_aqcb_to_wqe_info_scr.cb_addr, t0_s2s_aqcb_to_wqe_info.cb_addr);
    modify_field(t0_s2s_aqcb_to_wqe_info_scr.map_count_completed, t0_s2s_aqcb_to_wqe_info.map_count_completed);
}

action aq_tx_modify_qp_2_process () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage
    modify_field(to_s2_info_scr.ah_base_addr_page_id, to_s2_info.ah_base_addr_page_id);
    modify_field(to_s2_info_scr.sqcb_base_addr_hi, to_s2_info.sqcb_base_addr_hi);
    modify_field(to_s2_info_scr.rqcb_base_addr_hi, to_s2_info.rqcb_base_addr_hi);
    modify_field(to_s2_info_scr.pad, to_s2_info.pad);
    
    // stage to stage
}

action aq_tx_sqcb2_process_s3 () {
    // from ki global
    GENERATE_GLOBAL_K
}

action aq_tx_sqcb2_process_s4 () {
    // from ki global
    GENERATE_GLOBAL_K
}

action aq_tx_sqcb2_process () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage
    modify_field(to_s5_info_scr.rqcb_base_addr_hi, to_s5_info.rqcb_base_addr_hi);
    modify_field(to_s5_info_scr.tx_psn, to_s5_info.tx_psn);
    modify_field(to_s5_info_scr.tx_psn_valid, to_s5_info.tx_psn_valid);
    modify_field(to_s5_info_scr.local_ack_timeout_valid, to_s5_info.local_ack_timeout_valid);
    modify_field(to_s5_info_scr.local_ack_timeout, to_s5_info.local_ack_timeout);
    modify_field(to_s5_info_scr.err_retry_count, to_s5_info.err_retry_count);
    modify_field(to_s5_info_scr.err_retry_count_valid, to_s5_info.err_retry_count_valid);
    modify_field(to_s5_info_scr.q_key, to_s5_info.q_key);
    modify_field(to_s5_info_scr.q_key_valid, to_s5_info.q_key_valid);
    modify_field(to_s5_info_scr.pad, to_s5_info.pad);
    
    // stage to stage
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.state, t2_s2s_wqe2_to_sqcb2_info.state);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.pmtu_log2, t2_s2s_wqe2_to_sqcb2_info.pmtu_log2);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.ah_len, t2_s2s_wqe2_to_sqcb2_info.ah_len);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.ah_addr, t2_s2s_wqe2_to_sqcb2_info.ah_addr);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.rrq_depth_log2, t2_s2s_wqe2_to_sqcb2_info.rrq_depth_log2);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.rsq_depth_log2, t2_s2s_wqe2_to_sqcb2_info.rsq_depth_log2);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.rrq_base_addr, t2_s2s_wqe2_to_sqcb2_info.rrq_base_addr);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.rsq_base_addr, t2_s2s_wqe2_to_sqcb2_info.rsq_base_addr);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.state_valid, t2_s2s_wqe2_to_sqcb2_info.state_valid);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.state_valid, t2_s2s_wqe2_to_sqcb2_info.state_valid);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.av_valid, t2_s2s_wqe2_to_sqcb2_info.av_valid);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.rsq_valid, t2_s2s_wqe2_to_sqcb2_info.rsq_valid);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.rrq_valid, t2_s2s_wqe2_to_sqcb2_info.rrq_valid);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.pmtu_valid, t2_s2s_wqe2_to_sqcb2_info.pmtu_valid);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.qid, t2_s2s_wqe2_to_sqcb2_info.qid);
    modify_field(t2_s2s_wqe2_to_sqcb2_info_scr.pad, t2_s2s_wqe2_to_sqcb2_info.pad);
}

action aq_tx_sqcb0_process () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage
    modify_field(to_s6_info_scr.rqcb_base_addr_hi, to_s6_info.rqcb_base_addr_hi);
    modify_field(to_s6_info_scr.tx_psn, to_s6_info.tx_psn);
    modify_field(to_s6_info_scr.tx_psn_valid, to_s6_info.tx_psn_valid);
    modify_field(to_s6_info_scr.local_ack_timeout_valid, to_s6_info.local_ack_timeout_valid);
    modify_field(to_s6_info_scr.local_ack_timeout, to_s6_info.local_ack_timeout);
    modify_field(to_s6_info_scr.q_key, to_s6_info.q_key);
    modify_field(to_s6_info_scr.q_key_valid, to_s6_info.q_key_valid);
    modify_field(to_s6_info_scr.pad, to_s6_info.pad);
    
    // stage to stage
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.state, t2_s2s_sqcb2_to_sqcb0_info.state);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.state_valid, t2_s2s_sqcb2_to_sqcb0_info.state_valid);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.pmtu_log2, t2_s2s_sqcb2_to_sqcb0_info.pmtu_log2);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.pmtu_valid, t2_s2s_sqcb2_to_sqcb0_info.pmtu_valid);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.ah_len, t2_s2s_sqcb2_to_sqcb0_info.ah_len);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.ah_addr, t2_s2s_sqcb2_to_sqcb0_info.ah_addr);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.av_valid, t2_s2s_sqcb2_to_sqcb0_info.av_valid);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.rrq_depth_log2, t2_s2s_sqcb2_to_sqcb0_info.rrq_depth_log2);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.rsq_depth_log2, t2_s2s_sqcb2_to_sqcb0_info.rsq_depth_log2);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.rrq_base_addr, t2_s2s_sqcb2_to_sqcb0_info.rrq_base_addr);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.rsq_base_addr, t2_s2s_sqcb2_to_sqcb0_info.rsq_base_addr);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.rsq_valid, t2_s2s_sqcb2_to_sqcb0_info.rsq_valid);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.rrq_valid, t2_s2s_sqcb2_to_sqcb0_info.rrq_valid);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.qid, t2_s2s_sqcb2_to_sqcb0_info.qid);
    modify_field(t2_s2s_sqcb2_to_sqcb0_info_scr.pad, t2_s2s_sqcb2_to_sqcb0_info.pad);
}

action aq_tx_rqcb0_process_s3 () {
    // from ki global
    GENERATE_GLOBAL_K
}

action aq_tx_rqcb0_process_s4 () {
    // from ki global
    GENERATE_GLOBAL_K
}

action aq_tx_rqcb0_process () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage
    modify_field(to_s5_info_scr.q_key, to_s5_info.q_key);
    modify_field(to_s5_info_scr.q_key_valid, to_s5_info.q_key_valid);
    modify_field(to_s5_info_scr.pad, to_s5_info.pad);
    
    // stage to stage
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.state, t1_s2s_wqe2_to_rqcb0_info.state);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.pmtu_log2, t1_s2s_wqe2_to_rqcb0_info.pmtu_log2);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.ah_len, t1_s2s_wqe2_to_rqcb0_info.ah_len);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.ah_addr, t1_s2s_wqe2_to_rqcb0_info.ah_addr);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.rrq_depth_log2, t1_s2s_wqe2_to_rqcb0_info.rrq_depth_log2);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.rsq_depth_log2, t1_s2s_wqe2_to_rqcb0_info.rsq_depth_log2);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.rrq_base_addr, t1_s2s_wqe2_to_rqcb0_info.rrq_base_addr);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.rsq_base_addr, t1_s2s_wqe2_to_rqcb0_info.rsq_base_addr);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.state_valid, t1_s2s_wqe2_to_rqcb0_info.state_valid);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.pmtu_valid, t1_s2s_wqe2_to_rqcb0_info.pmtu_valid);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.av_valid, t1_s2s_wqe2_to_rqcb0_info.av_valid);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.rsq_valid, t1_s2s_wqe2_to_rqcb0_info.rsq_valid);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.rrq_valid, t1_s2s_wqe2_to_rqcb0_info.rrq_valid);
    modify_field(t1_s2s_wqe2_to_rqcb0_info_scr.pad, t1_s2s_wqe2_to_rqcb0_info.pad);
}

action aq_tx_feedback_process_s3 () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage

    // stage to stage
}

action aq_tx_feedback_process_s4 () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage

    // stage to stage
}

action aq_tx_feedback_process_s5 () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage

    // stage to stage
}

action aq_tx_feedback_process_s6 () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage

    // stage to stage
}

action aq_tx_feedback_process () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage
    modify_field(to_s7_fb_stats_info_scr.cb_addr, to_s7_fb_stats_info.cb_addr);
    modify_field(to_s7_fb_stats_info_scr.wqe_id, to_s7_fb_stats_info.wqe_id);
    modify_field(to_s7_fb_stats_info_scr.aq_cmd_done, to_s7_fb_stats_info.aq_cmd_done);    
    modify_field(to_s7_fb_stats_info_scr.pad, to_s7_fb_stats_info.pad);                                                   
    // stage to stage
    
}

action aq_tx_stats_process () {
    // from ki global
    GENERATE_GLOBAL_K

    // to stage
    modify_field(to_s7_fb_stats_info_scr.nop, to_s7_fb_stats_info.nop);
    modify_field(to_s7_fb_stats_info_scr.create_cq, to_s7_fb_stats_info.create_cq);
    modify_field(to_s7_fb_stats_info_scr.create_qp, to_s7_fb_stats_info.create_qp);
    modify_field(to_s7_fb_stats_info_scr.reg_mr, to_s7_fb_stats_info.reg_mr);
    modify_field(to_s7_fb_stats_info_scr.stats_hdrs, to_s7_fb_stats_info.stats_hdrs);
    modify_field(to_s7_fb_stats_info_scr.stats_vals, to_s7_fb_stats_info.stats_vals);
    modify_field(to_s7_fb_stats_info_scr.dereg_mr, to_s7_fb_stats_info.dereg_mr);
    modify_field(to_s7_fb_stats_info_scr.resize_cq, to_s7_fb_stats_info.resize_cq);
    modify_field(to_s7_fb_stats_info_scr.destroy_cq, to_s7_fb_stats_info.destroy_cq);
    modify_field(to_s7_fb_stats_info_scr.modify_qp, to_s7_fb_stats_info.modify_qp);
    modify_field(to_s7_fb_stats_info_scr.query_qp, to_s7_fb_stats_info.query_qp);
    modify_field(to_s7_fb_stats_info_scr.destroy_qp, to_s7_fb_stats_info.destroy_qp);
    modify_field(to_s7_fb_stats_info_scr.stats_dump, to_s7_fb_stats_info.stats_dump);

    // stage to stage
    //modify_field(t3_s2s_stats_info_scr.pad, t3_s2s_stats_info.pad);
}
