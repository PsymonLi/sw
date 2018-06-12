/* TLS P4 definitions */

#include "../../common-p4+/common_txdma_dummy.p4"

#define tx_table_s0_t0_action       read_tls_stg0

#define tx_table_s1_t0_action       read_tls_stg1_7

#define tx_table_s2_t0_action       tls_rx_serq 
#define tx_table_s2_t1_action       read_tnmdr
#define tx_table_s2_t2_action       read_tnmpr
#define tx_table_s2_t3_action       tls_read_pkt_descr_aol

#define tx_table_s3_t0_action       tls_serq_consume
#define tx_table_s3_t1_action       tdesc_alloc
#define tx_table_s3_t2_action       tpage_alloc

#define tx_table_s4_t0_action       tls_bld_brq4
#define tx_table_s4_t1_action       tls_read_random_iv
#define tx_table_s4_t2_action       tls_read_barco_pi

#define tx_table_s5_t0_action       tls_queue_brq5

#define tx_table_s6_t0_action       tls_pre_crypto_stats6

#define tx_table_s7_t0_action       tls_queue_brq7

#include "../../common-p4+/common_txdma.p4"
#include "tls_txdma_common.p4"

/* Per stage D-vector Definitions */

// d for stage 2 table 1

// d for stage 2 table 2

// d for stage 3 table 1
header_type tdesc_alloc_d_t {
    fields {
        desc                    : 64;
        pad                     : 448;
    }
}

// d for stage 3 table 2
header_type tpage_alloc_d_t {
    fields {
        page                    : 64;
        pad                     : 448;
    }
}

header_type tls_stage_bld_barco_req_d_t {
    fields {
        key_desc_index                  : HBM_ADDRESS_WIDTH;
        command_core                    : 4;
        command_mode                    : 4;
        command_op                      : 4;
        command_param                   : 20;
        // TBD: Total used   : 64 bits, pending: 448
        pad                             : 448;
    }
}
#define STG_BLD_BARCO_REQ_ACTION_PARAMS                                                                 \
key_desc_index, command_core, command_mode, command_op, command_param,idesc, odesc
#

#define GENERATE_STG_BLD_BARCO_REQ_D                                                                    \
    modify_field(tls_bld_barco_req_d.key_desc_index, key_desc_index);                                   \
    modify_field(tls_bld_barco_req_d.command_core, command_core);                                       \
    modify_field(tls_bld_barco_req_d.command_mode, command_mode);                                       \
    modify_field(tls_bld_barco_req_d.command_op, command_op);                                           \
    modify_field(tls_bld_barco_req_d.command_param, command_param);

header_type tls_stage_queue_brq_d_t {
    fields {
        // 8 Bytes intrinsic header
        CAPRI_QSTATE_HEADER_COMMON
        // 4 Bytes BRQ ring
        CAPRI_QSTATE_HEADER_RING(0)

        brq_base                       : ADDRESS_WIDTH;

        // TBD: Total used   : 128 bits, pending: 384
        pad                             : 384;
    }
}

#define STG_QUEUE_BRQ_ACTION_PARAMS                                                                 \
rsvd,cosA,cosB,cos_sel,eval_last,host,total,pid, pi_0,ci_0
#

#define GENERATE_STG_QUEUE_BRQ_D                                                                    \
    modify_field(tls_queue_brq_d.rsvd, rsvd);                                                       \
    modify_field(tls_queue_brq_d.cosA, cosA);                                                       \
    modify_field(tls_queue_brq_d.cosB, cosB);                                                       \
    modify_field(tls_queue_brq_d.cos_sel, cos_sel);                                                 \
    modify_field(tls_queue_brq_d.eval_last, eval_last);                                             \
    modify_field(tls_queue_brq_d.host, host);                                                       \
    modify_field(tls_queue_brq_d.total, total);                                                     \
    modify_field(tls_queue_brq_d.pid, pid);                                                         \
    modify_field(tls_queue_brq_d.pi_0, pi_0);                                                       \
    modify_field(tls_queue_brq_d.ci_0, ci_0);



#define GENERATE_GLOBAL_K                                                                               \
        modify_field(tls_global_phv_scratch.fid, tls_global_phv.fid);                                   \
        modify_field(tls_global_phv_scratch.dec_flow, tls_global_phv.dec_flow);                         \
        modify_field(tls_global_phv_scratch.flags_do_pre_ccm_enc, tls_global_phv.flags_do_pre_ccm_enc); \
        modify_field(tls_global_phv_scratch.flags_pad0, tls_global_phv.flags_pad0);                     \
        modify_field(tls_global_phv_scratch.qstate_addr, tls_global_phv.qstate_addr);                   \
        modify_field(tls_global_phv_scratch.debug_dol, tls_global_phv.debug_dol);                       \
        modify_field(tls_global_phv_scratch.tls_hdr_type, tls_global_phv.tls_hdr_type);                 \
        modify_field(tls_global_phv_scratch.tls_hdr_version_major, tls_global_phv.tls_hdr_version_major);\
        modify_field(tls_global_phv_scratch.tls_hdr_version_minor, tls_global_phv.tls_hdr_version_minor);\
        modify_field(tls_global_phv_scratch.tls_hdr_len, tls_global_phv.tls_hdr_len);                   \
        modify_field(tls_global_phv_scratch.next_tls_hdr_offset, tls_global_phv.next_tls_hdr_offset);

/* Global PHV definition */
header_type tls_global_phv_t {
    fields {
        fid                             : 16;
        dec_flow                        : 8;
        flags_do_pre_ccm_enc            : 1;
        flags_pad0                      : 7;
        qstate_addr                     : HBM_ADDRESS_WIDTH;
        debug_dol                       : 8;
        tls_hdr_type                    : 8;
        tls_hdr_version_major           : 8;
        tls_hdr_version_minor           : 8;
        tls_hdr_len                     : 16;
        next_tls_hdr_offset             : 16;
    }
}

header_type to_stage_2_phv_t {
    fields {
        serq_ci                         : 16;
        idesc                           : HBM_ADDRESS_WIDTH;
    }
}

header_type to_stage_3_phv_t {
    fields {
        tnmdr_pi                        : 16;
        tnmpr_pi                        : 16;
        serq_ci                         : 16;
    }
}

header_type to_stage_4_phv_t {
    fields {
        idesc                           : HBM_ADDRESS_WIDTH;
        odesc                           : HBM_ADDRESS_WIDTH;
        do_pre_mpp_enc                  : 1;
    }
}

header_type to_stage_5_phv_t {
    fields {
        idesc                           : HBM_ADDRESS_WIDTH;
        odesc                           : HBM_ADDRESS_WIDTH;
        opage                           : HBM_ADDRESS_WIDTH;
        sw_barco_pi                     : 16;
    }
}

header_type to_stage_6_phv_t {
    fields {
        tnmdr_alloc                     : 8;
        tnmpr_alloc                     : 8;
        enc_requests                    : 8;
        dec_requests                    : 8;
        debug_stage0_3_thread           : 16;
        debug_stage4_7_thread           : 16;
    }
}

header_type to_stage_7_phv_t {
    fields {
        idesc                           : HBM_ADDRESS_WIDTH;
        odesc                           : HBM_ADDRESS_WIDTH;
        opage                           : HBM_ADDRESS_WIDTH;
        cur_tls_data_len                : 16;
        next_tls_hdr_offset             : 16;
    }
}


/* PHV PI storage */
header_type barco_dbell_t {
    fields {
        pi                                  : 32;
    } 
}

header_type s3_t1_s2s_phv_t {
    fields {
        tnmdr_pidx              : 16;
    }
}

header_type s3_t2_s2s_phv_t {
    fields {
        tnmpr_pidx              : 16;
    }
}

#define s2_s5_t0_phv_t  additional_data_t
#define GENERATE_S2_S5_T0  GENERATE_AAD_FIELDS(s2_s5_t0_scratch, s2_s5_t0_phv)

@pragma scratch_metadata
metadata tlscb_0_t tlscb_0_d;

@pragma scratch_metadata
metadata tlscb_1_t tlscb_1_d;

@pragma scratch_metadata
metadata pkt_descr_aol_t PKT_DESCR_AOL_SCRATCH;

@pragma scratch_metadata
metadata tls_stage_bld_barco_req_d_t tls_bld_barco_req_d;

@pragma scratch_metadata
metadata tls_stage_queue_brq_d_t tls_queue_brq_d;

@pragma scratch_metadata
metadata tls_stage_pre_crypto_stats_d_t tls_pre_crypto_stats_d;

@pragma pa_header_union ingress to_stage_2
metadata to_stage_2_phv_t to_s2;

@pragma pa_header_union ingress to_stage_3
metadata to_stage_3_phv_t to_s3;

@pragma pa_header_union ingress to_stage_4
metadata to_stage_4_phv_t to_s4;


@pragma pa_header_union ingress to_stage_5
metadata to_stage_5_phv_t to_s5;

@pragma pa_header_union ingress to_stage_6
metadata to_stage_6_phv_t to_s6;

@pragma pa_header_union ingress to_stage_7
metadata to_stage_7_phv_t to_s7;

@pragma pa_header_union ingress common_global
metadata tls_global_phv_t tls_global_phv;

@pragma pa_header_union ingress  common_t0_s2s 
metadata s2_s5_t0_phv_t s2_s5_t0_phv;


@pragma pa_header_union ingress common_t1_s2s
metadata s3_t1_s2s_phv_t s3_t1_s2s;

@pragma pa_header_union ingress common_t2_s2s
metadata s3_t2_s2s_phv_t s3_t2_s2s;


@pragma dont_trim
metadata pkt_descr_aol_t idesc; 
@pragma dont_trim
metadata pkt_descr_aol_t odesc; 
@pragma dont_trim
metadata barco_desc_t barco_desc;
@pragma dont_trim
metadata bsq_slot_t bsq_slot;
@pragma dont_trim
metadata barco_dbell_t barco_dbell;

@pragma dont_trim
metadata crypto_iv_t crypto_iv;
@pragma dont_trim
@pragma pa_header_union ingress crypto_iv
metadata ccm_header_t ccm_header_with_aad;

@pragma dont_trim
metadata dma_cmd_phv2mem_t dma_cmd_bsq_slot;
@pragma dont_trim
metadata dma_cmd_phv2mem_t dma_cmd_odesc;
@pragma dont_trim
metadata dma_cmd_phv2mem_t dma_cmd_aad;
@pragma dont_trim
metadata dma_cmd_phv2mem_t dma_cmd_iv;
@pragma dont_trim
metadata dma_cmd_phv2mem_t dma_cmd_brq_slot;
@pragma dont_trim
metadata dma_cmd_phv2mem_t dma_cmd_idesc;
@pragma dont_trim
metadata dma_cmd_phv2mem_t dma_cmd_idesc_meta;
@pragma dont_trim
metadata dma_cmd_phv2mem_t dma_cmd_dbell;



@pragma scratch_metadata
metadata tls_global_phv_t tls_global_phv_scratch;
@pragma scratch_metadata
metadata to_stage_2_phv_t to_s2_scratch;
@pragma scratch_metadata
metadata to_stage_3_phv_t to_s3_scratch;
@pragma scratch_metadata
metadata to_stage_4_phv_t to_s4_scratch;
@pragma scratch_metadata
metadata to_stage_5_phv_t to_s5_scratch;
@pragma scratch_metadata
metadata to_stage_6_phv_t to_s6_scratch;
@pragma scratch_metadata
metadata to_stage_7_phv_t to_s7_scratch;

@pragma scratch_metadata
metadata s3_t1_s2s_phv_t s3_t1_s2s_scratch;
@pragma scratch_metadata
metadata s3_t2_s2s_phv_t s3_t2_s2s_scratch;
@pragma scratch_metadata
metadata s2_s5_t0_phv_t    s2_s5_t0_scratch;


@pragma scratch_metadata
metadata read_tnmdr_d_t read_tnmdr_d;
@pragma scratch_metadata
metadata read_tnmpr_d_t read_tnmpr_d;
@pragma scratch_metadata
metadata tdesc_alloc_d_t tdesc_alloc_d;
@pragma scratch_metadata
metadata tpage_alloc_d_t tpage_alloc_d;

@pragma scratch_metadata
metadata barco_channel_pi_ci_t tls_enc_queue_brq_d;

@pragma scratch_metadata
metadata tlscb_config_aead_t TLSCB_CONFIG_AEAD_SCRATCH;


action read_tls_stg1_7(TLSCB_1_PARAMS) {

    GENERATE_GLOBAL_K

    GENERATE_TLSCB_1_D
}



/* Stage 2 table 0 action */
action tls_rx_serq(TLSCB_CONFIG_AEAD_PARAMS) {

    GENERATE_GLOBAL_K

    /* To Stage 2 fields */
    modify_field(to_s2_scratch.serq_ci, to_s2.serq_ci);
    modify_field(to_s2_scratch.idesc, to_s2.idesc);

    GENERATE_TLSCB_CONFIG_AEAD
}

/*
 * Stage 2 table 1 action
 */
action read_tnmdr(tnmdr_pidx) {
    // d for stage 2 table 1 read-tnmdr-idx
    modify_field(read_tnmdr_d.tnmdr_pidx, tnmdr_pidx);
}

/*
 * Stage 2 table 2 action
 */
action read_tnmpr(tnmpr_pidx) {
    // d for stage 2 table 2 read-tnmpr-idx
    modify_field(read_tnmpr_d.tnmpr_pidx, tnmpr_pidx);
}

/*
 * Stage 2 table 3 action
 */
action tls_read_pkt_descr_aol(PKT_DESCR_AOL_ACTION_PARAMS) {
    GENERATE_GLOBAL_K

    // d for stage 2 table 3
    GENERATE_PKT_DESCR_AOL_D
}

/* Stage 3 table 0 action */
action tls_serq_consume(TLSCB_0_PARAMS_NON_STG0) {


    GENERATE_GLOBAL_K

    /* To Stage 3 fields */
    modify_field(to_s3_scratch.serq_ci, to_s3.serq_ci);

    /* D vector */
    GENERATE_TLSCB_0_D_NON_STG0

}

/*
 * Stage 3 table 1 action
 */
action tdesc_alloc(desc, pad) {
    // k + i for stage 3 table 1

    // from ki global
    GENERATE_GLOBAL_K

    // from stage 2 to stage 3
    modify_field(s3_t1_s2s_scratch.tnmdr_pidx, s3_t1_s2s.tnmdr_pidx);

    // d for stage 3 table 1
    modify_field(tdesc_alloc_d.desc, desc);
    modify_field(tdesc_alloc_d.pad, pad);
}

/*
 * Stage 3 table 2 action
 */
action tpage_alloc(page, pad) {
    // k + i for stage 3 table 2

    // from to_stage 3

    // from ki global
    GENERATE_GLOBAL_K

    // from stage 2 to stage 3
    modify_field(s3_t2_s2s_scratch.tnmpr_pidx, s3_t2_s2s.tnmpr_pidx);

    // d for stage 3 table 2
    modify_field(tpage_alloc_d.page, page);
    modify_field(tpage_alloc_d.pad, pad);
}


/* Stage 3 table 3 action */
action tls_stage3(TLSCB_1_PARAMS) {

    GENERATE_GLOBAL_K

    /* To Stage 2 fields */
    modify_field(to_s2_scratch.serq_ci, to_s2.serq_ci);
    modify_field(to_s2_scratch.idesc, to_s2.idesc);

    GENERATE_TLSCB_1_D
}

/* Stage 4 table 0 action */
action tls_bld_brq4(TLSCB_CONFIG_AEAD_PARAMS) {

    GENERATE_GLOBAL_K

    /* To Stage 4 table 0 fields */
    modify_field(to_s4_scratch.idesc, to_s4.idesc);
    modify_field(to_s4_scratch.odesc, to_s4.odesc);
    modify_field(to_s4_scratch.do_pre_mpp_enc, to_s4.do_pre_mpp_enc);

    /* D vector */
    GENERATE_TLSCB_CONFIG_AEAD
}

/* Stage 4 table 1 action */
action tls_read_random_iv(TLSCB_0_PARAMS_NON_STG0) {

    GENERATE_GLOBAL_K

    /* To Stage 4 table 1 fields */

    /* D vector */
    GENERATE_TLSCB_0_D_NON_STG0
}

/* Stage 4 table 2 action */
action tls_read_barco_pi(TLSCB_0_PARAMS_NON_STG0) {

    GENERATE_GLOBAL_K

    /* To Stage 4 table 2 fields */

    /* D vector */
    GENERATE_TLSCB_0_D_NON_STG0
}

/* Stage 5 table 0 action */
action tls_queue_brq5(BARCO_CHANNEL_PARAMS) {

    GENERATE_GLOBAL_K



    /* To Stage 5 fields */
    modify_field(to_s5_scratch.idesc, to_s5.idesc);
    modify_field(to_s5_scratch.odesc, to_s5.odesc);
    modify_field(to_s5_scratch.opage, to_s5.opage);
    modify_field(to_s5_scratch.sw_barco_pi, to_s5.sw_barco_pi);

    GENERATE_S2_S5_T0

    GENERATE_BARCO_CHANNEL_D
}


/* Stage 6 action */
action tls_pre_crypto_stats6(STG_PRE_CRYPTO_STATS_ACTION_PARAMS) {

    GENERATE_GLOBAL_K


    /* To Stage 6 fields */
    modify_field(to_s6_scratch.tnmdr_alloc, to_s6.tnmdr_alloc);
    modify_field(to_s6_scratch.tnmpr_alloc, to_s6.tnmpr_alloc);
    modify_field(to_s6_scratch.enc_requests, to_s6.enc_requests);
    modify_field(to_s6_scratch.dec_requests, to_s6.dec_requests);
    modify_field(to_s6_scratch.debug_stage0_3_thread, to_s6.debug_stage0_3_thread);
    modify_field(to_s6_scratch.debug_stage4_7_thread, to_s6.debug_stage4_7_thread);


    GENERATE_STG_PRE_CRYPTO_STATS_D
}

/* Stage 7 action */
action tls_queue_brq7(STG_QUEUE_BRQ_ACTION_PARAMS) {

    GENERATE_GLOBAL_K



    /* To Stage 7 fields */
    modify_field(to_s7_scratch.idesc, to_s7.idesc);
    modify_field(to_s7_scratch.odesc, to_s7.odesc);
    modify_field(to_s7_scratch.opage, to_s7.opage);
    modify_field(to_s7_scratch.cur_tls_data_len, to_s7.cur_tls_data_len);
    modify_field(to_s7_scratch.next_tls_hdr_offset, to_s7.next_tls_hdr_offset);

    GENERATE_STG_QUEUE_BRQ_D
}
