/*****************************************************************************/
/* tcp_proxy_rxdma.p4
/*****************************************************************************/

#include "../common-p4+/common_rxdma_dummy.p4"

/******************************************************************************
 * Table names
 * 
 * Table names have to alphabetically be in chronological order (to match with
 * common program table names), so they need to be prefixed by stage# and
 * table#
 *****************************************************************************/
#define rx_table_s1_t0 s1_t0_tcp_rx

#define rx_table_s2_t0 s2_t0_tcp_rx

#define rx_table_s3_t0 s3_t0_tcp_rx
#define rx_table_s3_t1 s3_t1_tcp_rx
#define rx_table_s3_t2 s3_t2_tcp_rx
#define rx_table_s3_t3 s3_t3_tcp_rx

#define rx_table_s4_t0 s4_t0_tcp_rx
#define rx_table_s4_t1 s4_t1_tcp_rx
#define rx_table_s4_t2 s4_t2_tcp_rx
#define rx_table_s4_t3 s4_t3_tcp_rx

#define rx_table_s5_t0 s5_t0_tcp_rx

#define rx_table_s6_t0 s6_t0_tcp_rx
#define rx_table_s6_t1 s6_t1_tcp_rx
#define rx_table_s6_t2 s6_t2_tcp_rx
#define rx_table_s6_t3 s6_t3_tcp_rx

#define rx_table_s7_t0 s7_t0_tcp_rx



/******************************************************************************
 * Action names
 *****************************************************************************/
#define rx_table_s1_t0_action tcp_stage1_dummy

#define rx_table_s2_t0_action tcp_rx

#define rx_table_s3_t0_action1 tcp_ack
#define rx_table_s3_t0_action2 tcp_ooo_book_keeping

#define rx_table_s3_t1_action read_rnmdr

#define rx_table_s3_t2_action read_rnmpr

#define rx_table_s3_t3_action l7_read_rnmdr


#define rx_table_s4_t0_action tcp_rtt
#define rx_table_s4_t1_action rdesc_alloc
#define rx_table_s4_t2_action rpage_alloc
#define rx_table_s4_t3_action0 l7_rdesc_alloc
#define rx_table_s4_t3_action1 tcp_ooo_qbase_sem_alloc

#define rx_table_s5_t0_action tcp_fc
#define rx_table_s5_t1_action tcp_ooo_qbase_addr_get

#define rx_table_s6_t0_action write_serq
#define rx_table_s6_t1_action write_arq
#define rx_table_s6_t2_action write_l7q
#define rx_table_s6_t3_action tcp_ooo_qbase_cb_load

#define rx_table_s7_t0_action stats

#define common_p4plus_stage0_app_header_table_action_dummy read_tx2rx

#include "../common-p4+/common_rxdma.p4"
#include "../cpu-p4+/cpu_rx_common.p4"
#include "tcp_proxy_common.p4"

/******************************************************************************
 * Macros
 *****************************************************************************/
#define GENERATE_GLOBAL_K \
    modify_field(common_global_scratch.fid, common_phv.fid); \
    modify_field(common_global_scratch.qstate_addr, common_phv.qstate_addr); \
    modify_field(common_global_scratch.snd_una, common_phv.snd_una); \
    modify_field(common_global_scratch.debug_dol, common_phv.debug_dol); \
    modify_field(common_global_scratch.quick, common_phv.quick); \
    modify_field(common_global_scratch.ecn_flags, common_phv.ecn_flags); \
    modify_field(common_global_scratch.process_ack_flag, \
                            common_phv.process_ack_flag); \
    modify_field(common_global_scratch.flags, common_phv.flags); \
    modify_field(common_global_scratch.is_dupack, common_phv.is_dupack); \
    modify_field(common_global_scratch.ooo_rcv, common_phv.ooo_rcv); \
    modify_field(common_global_scratch.ooo_in_rx_q, common_phv.ooo_in_rx_q); \
    modify_field(common_global_scratch.write_serq, common_phv.write_serq); \
    modify_field(common_global_scratch.pending_del_ack_send, common_phv.pending_del_ack_send); \
    modify_field(common_global_scratch.pending_txdma, common_phv.pending_txdma); \
    modify_field(common_global_scratch.pingpong, common_phv.pingpong); \
    modify_field(common_global_scratch.fatal_error, common_phv.fatal_error); \
    modify_field(common_global_scratch.write_arq, common_phv.write_arq); \
    modify_field(common_global_scratch.write_tcp_app_hdr, common_phv.write_tcp_app_hdr); \
    modify_field(common_global_scratch.tsopt_enabled, common_phv.tsopt_enabled); \
    modify_field(common_global_scratch.tsopt_available, common_phv.tsopt_available); \
    modify_field(common_global_scratch.skip_pkt_dma, common_phv.skip_pkt_dma);

/******************************************************************************
 * D-vectors
 *****************************************************************************/

// d for stage 0
header_type read_tx2rxd_t {
    fields {
        rsvd                    : 8;
        cosA                    : 4;
        cosB                    : 4;
        cos_sel                 : 8;
        eval_last               : 8;
        host                    : 4;
        total                   : 4;
        pid                     : 16;
        rx_ts                   : 64; // 16 bytes

        serq_ring_size          : 16;
        l7_proxy_type           : 8;
        debug_dol               : 16;
        quick_acks_decr_old     : 4;
        pad2                    : 20; // 8 bytes

        serq_cidx               : 16;
        pad1                    : 48; // 8 bytes

        // written by TCP Tx P4+ program
        TX2RX_SHARED_STATE           // offset = 32
    }
}

// d for stage 1
header_type read_tls_stage0_d_t {
    fields {
        CAPRI_QSTATE_HEADER_RING(0) // TLS_SCHED_RING_SERQ
    }
}

// d for stage 2
header_type tcp_rx_d_t {
    fields {
        bytes_acked             : 16;   // tcp_ack stage
        slow_path_cnt           : 16;
        serq_full_cnt           : 16;
        ooo_cnt                 : 16;
        rcv_nxt                 : 32;
        ooo_rcv_bitmap          : TCP_OOO_NUM_CELLS;
        rcv_tstamp              : 32;
        ts_recent               : 32;
        lrcv_time               : 32;
        snd_una                 : 32;   // tcp_ack stage
        snd_wl1                 : 32;   // tcp_ack stage
        pred_flags              : 32;   // tcp_ack stage
        max_window              : 16;   // tcp_ack_stage
        bytes_rcvd              : 16;
        snd_wnd                 : 16;   // tcp_ack stage
        ato                     : 16;
        serq_pidx               : 16; 
        rcv_mss                 : 16;
        flag                    : 8;    // used with .l not written back
        rto                     : 8;
        ecn_flags               : 8;
        state                   : 8;
        parsed_state            : 8;
        quick                   : 4;
        snd_wscale              : 4;
        pending                 : 3;
        ca_flags                : 2;
        num_dup_acks            : 2;
        write_serq              : 1;
        fastopen_rsk            : 1;
        pingpong                : 1;
        ooo_in_rx_q             : 1;
        alloc_descr             : 1;    // used with .l not written back
    }
}

// d for stage 3 table 0 - ooo book keeping
header_type ooo_book_keeping_t {
    fields {
        start_seq0      : 32;
        end_seq0        : 32;
        tail_index0     : 16;
        start_seq1      : 32;
        end_seq1        : 32;
        tail_index1     : 16;
        start_seq2      : 32;
        end_seq2        : 32;
        tail_index2     : 16;
        start_seq3      : 32;
        end_seq3        : 32;
        tail_index3     : 16;
        ooo_alloc_fail  : 32;
        ooo_queue0_full : 16;
        ooo_queue1_full : 16;
        ooo_queue2_full : 16;
        ooo_queue3_full : 16;
        ooo_pad         : 96;
    }
}

// d for stage 3 table 1
header_type read_rnmdr_d_t {
    fields {
        rnmdr_pidx              : 32;
        rnmdr_pidx_full         : 8;
    }
}

// d for stage 3 table 2
header_type read_rnmpr_d_t {
    fields {
        rnmpr_pidx              : 32;
        rnmpr_pidx_full         : 8;
    }
}

// d for stage 3 table 3
header_type read_serq_d_t {
    fields {
        serq_pidx               : 16;
    }
}

       
// d for stage 4 table 0
header_type tcp_rtt_d_t {
    fields {
        srtt_us                 : 32;
        seq_rtt_us              : 32;
        ca_rtt_us               : 32;
        curr_ts                 : 32;
        rtt_min                 : 32;
        rttvar_us               : 32;
        mdev_us                 : 32;
        mdev_max_us             : 32;
        rtt_seq                 : 32;
        rto                     : 16;
        ts_ganularity_us        : 16;
        ts_shift                : 8;
        backoff                 : 4;
    }
}

#define RTT_D_PARAMS                                            \
    srtt_us, seq_rtt_us, ca_rtt_us, curr_ts, rtt_min, rttvar_us,\
    mdev_us, mdev_max_us, rtt_seq, rto, ts_ganularity_us,       \
    ts_shift, backoff

#define GENERATE_RTT_D                                          \
    modify_field(tcp_rtt_d.srtt_us, srtt_us);                   \
    modify_field(tcp_rtt_d.seq_rtt_us, seq_rtt_us);             \
    modify_field(tcp_rtt_d.ca_rtt_us, ca_rtt_us);               \
    modify_field(tcp_rtt_d.curr_ts, curr_ts);                   \
    modify_field(tcp_rtt_d.rtt_min, rtt_min);                   \
    modify_field(tcp_rtt_d.rttvar_us, rttvar_us);               \
    modify_field(tcp_rtt_d.mdev_us, mdev_us);                   \
    modify_field(tcp_rtt_d.mdev_max_us, mdev_max_us);           \
    modify_field(tcp_rtt_d.rtt_seq, rtt_seq);                   \
    modify_field(tcp_rtt_d.rto, rto);                           \
    modify_field(tcp_rtt_d.ts_ganularity_us, ts_ganularity_us); \
    modify_field(tcp_rtt_d.ts_shift, ts_shift);                 \
    modify_field(tcp_rtt_d.backoff, backoff);                   \
    

// d for stage 4 table 1
header_type rdesc_alloc_d_t {
    fields {
        desc                    : 64;
        pad                     : 448;
    }
}

// d for stage 4 table 2
header_type rpage_alloc_d_t {
    fields {
        page                    : 64;
        pad                     : 448;
    }
}

// for stage 4 table 3
header_type read_ooo_qbase_index_t {
    fields {
        ooo_qbase_pindex      : 32;
        ooo_qbase_pindex_full : 8;
    }
}

// d for stage5 table 0
header_type read_ooo_base_addr_t {
    fields {
        ooo_qbase_addr : 64;
    }
}

// d for stage 5 table 0
header_type tcp_fc_d_t {
    fields {
        page                    : 32;
        descr                   : 32;
        page_cnt                : 16;
        dummy                   : 16;
        l7_descr                : 32;
        rcv_wnd                 : 16;
        rcv_wscale              : 16;
        cpu_id                  : 8;
    }
}

// d for stage 6 table 0
header_type write_serq_d_t {
    fields {
        serq_base               : 32;
        nde_addr                : 64;
        nde_offset              : 16;
        nde_len                 : 16;
        curr_ts                 : 32;
        ft_pi                   : 16;
        rx2tx_send_ack_pi       : 16;
        rx2tx_clean_retx_pi     : 16;
        rx2tx_fast_retx_pi      : 16;

        // stats
        pkts_rcvd               : 8;
        pages_alloced           : 8;
        desc_alloced            : 8;
        debug_num_pkt_to_mem    : 8;
        debug_num_phv_to_mem    : 8;
    }
}

// d for stage 6 table 2
header_type write_l7q_d_t {
    fields {
        l7q_base                : 64; 
        l7q_pidx                : 16;    
    }   
} 

// d for stage 6 table 3 - ooo qbase addr
header_type ooo_qbase_addr_t {
    fields {
        ooo_qbase_addr0    : 64;
        ooo_qbase_addr1    : 64;
        ooo_qbase_addr2    : 64;
        ooo_qbase_addr3    : 64;
        ooo_qbase_pad      : 256;
    }
} 
/******************************************************************************
 * Global PHV definitions
 *****************************************************************************/
header_type to_stage_2_phv_t {
    // tcp-rx
    fields {
        data_ofs_rsvd           : 8;
        rcv_wup                 : 32;
        seq                     : 32;
        serq_cidx               : 12;
        ip_dsfield              : 8;
    }
}

header_type to_stage_3_phv_t {
    // tcp-ack
    fields {
        flag                    : 8;
        seq                     : 32;
        payload_len             : 16;
    }
}

header_type to_stage_4_phv_t {
    // tcp-rtt, read-rnmdr, read-rnmpr, read-serq
    fields {
        snd_nxt                 : 32;
        rcv_tsval               : 32;
        rcv_tsecr               : 32;
    }
}

header_type to_stage_5_phv_t {
    // tcp-fc
    fields {
        page_count              : 16;
        page                    : 34;
        descr                   : 32;
        l7_descr                : 32;
    }
}

header_type to_stage_6_phv_t {
    // write-serq
    fields {
        page                    : 34;
        descr                   : 32;
        payload_len             : 16;
        ooo_offset              : 16;
        serq_pidx               : 12;
        ooo_queue_id            : 2;
        ooo_tail_index          : 8;
    }
}

header_type to_stage_7_phv_t {
    // stats
    fields {
        bytes_rcvd              : 16;
        pkts_rcvd               : 8;
        pages_alloced           : 8;
        desc_alloced            : 8;
        debug_num_pkt_to_mem    : 8;
        debug_num_phv_to_mem    : 8;

        bytes_acked             : 16;
        slow_path_cnt           : 16;
        serq_full_cnt           : 16;
        ooo_cnt                 : 16;
    }
}

header_type common_global_phv_t {
    fields {
        // global k (max 128)
        fid                     : 24;
        qstate_addr             : 34;
        snd_una                 : 32;
        debug_dol               : 8;
        flags                   : 8;
        quick                   : 3;
        ecn_flags               : 2;
        process_ack_flag        : 1;
        is_dupack               : 1;
        ooo_rcv                 : 1;
        ooo_in_rx_q             : 1;
        write_serq              : 1;
        pending_del_ack_send    : 1;
        pending_txdma           : 4;
        pingpong                : 1;
        fatal_error             : 1;
        write_arq               : 1;
        write_tcp_app_hdr       : 1;
        tsopt_enabled           : 1;
        tsopt_available         : 1;
        skip_pkt_dma            : 1;
    }
}

/******************************************************************************
 * Stage to stage PHV definitions
 *****************************************************************************/
header_type s1_s2s_phv_t {
    fields {
        payload_len             : 16;
        ack_seq                 : 32;
        snd_nxt                 : 32;
        rcv_tsval               : 32;
        packets_out             : 16;
        window                  : 16;
        rcv_mss_shft            : 4;
        quick_acks_decr         : 4;
        fin_sent                : 1;
        rst_sent                : 1;
    }
}

header_type s4_t1_s2s_phv_t {
    fields {
        rnmdr_pidx              : 16;
        ato                     : 16;
    }
}

header_type s4_t2_s2s_phv_t {
    fields {
        rnmpr_pidx              : 16;
    }
}

#if 0
header_type s4_s2s_phv_t {
    fields {
        payload_len             : 16;
        ato                     : 16;
        ooo_offset              : 16;
        packets_out             : 32;
        sacked_out              : 16;
        lost_out                : 8;
    }
}
#endif

header_type s6_s2s_phv_t {
    fields {
        payload_len             : 16;
    }
}

header_type s6_t1_s2s_phv_t {
    fields {
        rnmdr_pidx              : 16;
        ato                     : 16;
        cpu_id                  : 8;
    }
}

header_type s6_t2_s2s_phv_t {
    fields {
        l7_descr                : 32;
    }    
}

header_type s6_t3_s2s_phv_t {
    fields {
        ooo_qbase_addr : 64;
        ooo_pkt_descr_addr : 64;
    }
}

/******************************************************************************
 * Header unions for d-vector
 *****************************************************************************/
@pragma scratch_metadata
metadata read_tx2rxd_t read_tx2rxd;
@pragma scratch_metadata
metadata read_tls_stage0_d_t read_tls_stage0_d;
@pragma scratch_metadata
metadata tcp_rx_d_t tcp_rx_d;
@pragma scratch_metadata
metadata tcp_rtt_d_t tcp_rtt_d;
@pragma scratch_metadata
metadata read_rnmdr_d_t read_rnmdr_d;
@pragma scratch_metadata
metadata read_rnmpr_d_t read_rnmpr_d;
@pragma scratch_metadata
metadata read_rnmdr_d_t l7_read_rnmdr_d;
@pragma scratch_metadata
metadata read_ooo_qbase_index_t read_ooo_qbase_index;
@pragma scratch_metadata
metadata read_serq_d_t read_serq_d;
//@pragma scratch_metadata
//metadata tcp_fra_d_t tcp_fra_d;
//@pragma scratch_metadata
//metadata tcp_cc_d_t tcp_cc_d;
@pragma scratch_metadata
metadata tcp_fc_d_t tcp_fc_d;
@pragma scratch_metadata
metadata write_serq_d_t write_serq_d;
@pragma scratch_metadata
metadata rdesc_alloc_d_t rdesc_alloc_d;
@pragma scratch_metadata
metadata rpage_alloc_d_t rpage_alloc_d;
@pragma scratch_metadata
metadata rdesc_alloc_d_t l7_rdesc_alloc_d;
@pragma scratch_metadata
metadata arq_pi_d_t arq_rx_pi_d;
@pragma scratch_metadata
metadata write_l7q_d_t write_l7q_d;

@pragma scratch_metadata
metadata ooo_book_keeping_t ooo_book_keeping;
@pragma scratch_metadata
metadata ooo_qbase_addr_t ooo_qbase_addr;
@pragma scratch_metadata
metadata read_ooo_base_addr_t read_ooo_base_addr;

/******************************************************************************
 * Header unions for PHV layout
 *****************************************************************************/
@pragma pa_header_union ingress app_header
metadata p4_to_p4plus_tcp_proxy_base_header_t tcp_app_header;
@pragma scratch_metadata
metadata p4_to_p4plus_tcp_proxy_base_header_t tcp_scratch_app;

@pragma pa_header_union ingress to_stage_1 cpu_hdr1
metadata p4_to_p4plus_cpu_pkt_1_t cpu_hdr1;

@pragma pa_header_union ingress to_stage_2 cpu_hdr2
metadata to_stage_2_phv_t to_s2;
@pragma dont_trim
metadata p4_to_p4plus_cpu_pkt_2_t cpu_hdr2;

@pragma pa_header_union ingress to_stage_3 cpu_hdr3
@pragma dont_trim
metadata p4_to_p4plus_cpu_pkt_3_t cpu_hdr3;


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
metadata common_global_phv_t common_phv;

@pragma scratch_metadata
metadata p4_to_p4plus_cpu_pkt_1_t to_cpu1_scratch;
@pragma scratch_metadata
metadata to_stage_2_phv_t to_s2_scratch;
@pragma scratch_metadata
metadata p4_to_p4plus_cpu_pkt_2_t to_cpu2_scratch;
@pragma scratch_metadata
metadata to_stage_3_phv_t to_s3_scratch;
@pragma scratch_metadata
metadata p4_to_p4plus_cpu_pkt_3_t to_cpu3_scratch;
@pragma scratch_metadata
metadata to_stage_4_phv_t to_s4_scratch;
@pragma scratch_metadata
metadata to_stage_5_phv_t to_s5_scratch;
@pragma scratch_metadata
metadata to_stage_6_phv_t to_s6_scratch;
@pragma scratch_metadata
metadata to_stage_7_phv_t to_s7_scratch;
@pragma scratch_metadata
metadata s1_s2s_phv_t s1_s2s_scratch;
@pragma scratch_metadata
metadata s4_t1_s2s_phv_t s4_t1_s2s_scratch;
@pragma scratch_metadata
metadata s4_t2_s2s_phv_t s4_t2_s2s_scratch;
@pragma scratch_metadata
metadata s6_s2s_phv_t s6_s2s_scratch;
@pragma scratch_metadata
metadata s6_t1_s2s_phv_t s6_t1_s2s_scratch;
@pragma scratch_metadata
metadata s6_t2_s2s_phv_t s6_t2_s2s_scratch;
@pragma scratch_metadata
metadata s6_t3_s2s_phv_t s6_t3_s2s_scratch;
@pragma scratch_metadata
metadata common_global_phv_t common_global_scratch;

@pragma pa_header_union ingress common_t0_s2s s6_s2s
metadata s1_s2s_phv_t s1_s2s;
//metadata s4_s2s_phv_t s4_s2s;
metadata s6_s2s_phv_t s6_s2s;

@pragma pa_header_union ingress common_t1_s2s s6_t1_s2s
metadata s4_t1_s2s_phv_t s4_t1_s2s;
metadata s6_t1_s2s_phv_t s6_t1_s2s;

@pragma pa_header_union ingress common_t2_s2s s6_t2_s2s
metadata s4_t2_s2s_phv_t s4_t2_s2s;
metadata s6_t2_s2s_phv_t s6_t2_s2s;

@pragma pa_header_union ingress common_t3_s2s s6_t3_s2s
metadata s6_t3_s2s_phv_t s6_t3_s2s;

/******************************************************************************
 * PHV following k (for app DMA etc.)
 *****************************************************************************/
@pragma dont_trim
metadata rx2tx_t rx2tx;
@pragma dont_trim
metadata rx2tx_extra_t rx2tx_extra;
@pragma dont_trim
metadata ring_entry_t l7_ring_entry; 
@pragma dont_trim
metadata doorbell_data_t db_data;
@pragma dont_trim
metadata doorbell_data_t db_data2;
@pragma dont_trim
metadata doorbell_data_t db_data3;

header_type ring_entry_pad_t {
    fields {
        ring_entry_pad      : 24;
    }
}
@pragma dont_trim
metadata ring_entry_pad_t  ring_entry_pad;

/* ring_entry and aol needs to be contiguous in PHV */
@pragma dont_trim
metadata ring_entry_t ring_entry; 
@pragma dont_trim
metadata pkt_descr_aol_t aol;

@pragma dont_trim
metadata doorbell_data_t l7_db_data;

header_type ooq_slot_addr_t {
    fields {
        ooq_slot_addr : 64;
    }
}

@pragma dont_trim
metadata ooq_slot_addr_t ooq_slot_addr;
 


@pragma pa_align 128
@pragma dont_trim
metadata dma_cmd_pkt2mem_t pkt_dma;                 // dma cmd 1
@pragma dont_trim
@pragma pa_header_union ingress pkt_dma
metadata dma_cmd_skip_t pkt_dma_skip;

@pragma dont_trim
@pragma pa_header_union ingress pkt_dma
metadata dma_cmd_pkt2mem_t ooo_pkt_dma;

@pragma dont_trim
metadata dma_cmd_phv2mem_t pkt_descr_dma;           // dma cmd 2
@pragma dont_trim
metadata dma_cmd_phv2mem_t tcp_flags_dma;           // dma cmd 3
@pragma dont_trim
metadata dma_cmd_phv2mem_t rx2tx_or_cpu_hdr_dma;    // dma cmd 4
@pragma dont_trim
metadata dma_cmd_phv2mem_t ring_slot;               // dma cmd 5
@pragma dont_trim
metadata dma_cmd_phv2mem_t rx2tx_extra_dma;         // dma cmd 6
@pragma dont_trim
metadata dma_cmd_phv2mem_t tx_doorbell1;            // dma cmd 7
@pragma dont_trim
metadata dma_cmd_phv2mem_t tx_doorbell_or_timer;    // dma cmd 8
@pragma dont_trim
metadata dma_cmd_phv2mem_t tls_doorbell;            // dma cmd 9
@pragma dont_trim
metadata dma_cmd_phv2mem_t l7_descr;                // dma cmd 10

@pragma dont_trim
@pragma pa_header_union ingress l7_descr
metadata dma_cmd_phv2mem_t ooo_slot_addr;
 
@pragma dont_trim
metadata dma_cmd_phv2mem_t l7_ring_slot;            // dma cmd 11
@pragma dont_trim
metadata dma_cmd_phv2mem_t l7_doorbell;             // dma cmd 12

/******************************************************************************
 * Action functions to generate k_struct and d_struct
 *
 * These action functions are currently only to generate the k+i and d structs
 * and do not implement any pseudo code
 *****************************************************************************/

/*
 * Stage 0 table 0 action
 */
action read_tx2rx(rsvd, cosA, cosB, cos_sel, eval_last, host, total, pid, rx_ts,
                  serq_ring_size, l7_proxy_type, debug_dol, quick_acks_decr_old,
                  pad2, serq_cidx, pad1, prr_out, snd_nxt, rcv_wup, packets_out,
                  ecn_flags_tx, quick_acks_decr, fin_sent, rst_sent, pad1_tx2rx) {
    // k + i for stage 0

    // from intrinsic
    modify_field(p4_intr_global_scratch.lif, p4_intr_global.lif);
    modify_field(p4_intr_global_scratch.tm_iq, p4_intr_global.tm_iq);
    modify_field(p4_rxdma_intr_scratch.qid, p4_rxdma_intr.qid);
    modify_field(p4_rxdma_intr_scratch.qtype, p4_rxdma_intr.qtype);
    modify_field(p4_rxdma_intr_scratch.qstate_addr, p4_rxdma_intr.qstate_addr);

    // from app header
    modify_field(tcp_scratch_app.p4plus_app_id, tcp_app_header.p4plus_app_id);
    modify_field(tcp_scratch_app.table0_valid, tcp_app_header.table0_valid);
    modify_field(tcp_scratch_app.table1_valid, tcp_app_header.table1_valid);
    modify_field(tcp_scratch_app.table2_valid, tcp_app_header.table2_valid);
    modify_field(tcp_scratch_app.table3_valid, tcp_app_header.table3_valid);

    modify_field(tcp_scratch_app.num_sack_blocks, tcp_app_header.num_sack_blocks);
    modify_field(tcp_scratch_app.payload_len, tcp_app_header.payload_len);
    modify_field(tcp_scratch_app.srcPort, tcp_app_header.srcPort);
    modify_field(tcp_scratch_app.dstPort, tcp_app_header.dstPort);
    modify_field(tcp_scratch_app.seqNo, tcp_app_header.seqNo);
    modify_field(tcp_scratch_app.ackNo, tcp_app_header.ackNo);
    modify_field(tcp_scratch_app.dataOffset, tcp_app_header.dataOffset);
    modify_field(tcp_scratch_app.flags, tcp_app_header.flags);
    modify_field(tcp_scratch_app.window, tcp_app_header.window);
    modify_field(tcp_scratch_app.urgentPtr, tcp_app_header.urgentPtr);
    modify_field(tcp_scratch_app.ts, tcp_app_header.ts);
    modify_field(tcp_scratch_app.prev_echo_ts, tcp_app_header.prev_echo_ts);

    // d for stage 0
    modify_field(read_tx2rxd.rsvd, rsvd);
    modify_field(read_tx2rxd.cosA, cosA);
    modify_field(read_tx2rxd.cosB, cosB);
    modify_field(read_tx2rxd.cos_sel, cos_sel);
    modify_field(read_tx2rxd.eval_last, eval_last);
    modify_field(read_tx2rxd.host, host);
    modify_field(read_tx2rxd.total, total);
    modify_field(read_tx2rxd.pid, pid);
    modify_field(read_tx2rxd.rx_ts, rx_ts);
    modify_field(read_tx2rxd.serq_ring_size, serq_ring_size);
    modify_field(read_tx2rxd.l7_proxy_type, l7_proxy_type);
    modify_field(read_tx2rxd.debug_dol, debug_dol);
    modify_field(read_tx2rxd.quick_acks_decr_old, quick_acks_decr_old);
    modify_field(read_tx2rxd.pad2, pad2);
    modify_field(read_tx2rxd.serq_cidx, serq_cidx);
    modify_field(read_tx2rxd.pad1, pad1);
    modify_field(read_tx2rxd.prr_out, prr_out);
    modify_field(read_tx2rxd.snd_nxt, snd_nxt);
    modify_field(read_tx2rxd.rcv_wup, rcv_wup);
    modify_field(read_tx2rxd.packets_out, packets_out);
    modify_field(read_tx2rxd.ecn_flags_tx, ecn_flags_tx);
    modify_field(read_tx2rxd.quick_acks_decr, quick_acks_decr);
    modify_field(read_tx2rxd.fin_sent, fin_sent);
    modify_field(read_tx2rxd.rst_sent, rst_sent);
    modify_field(read_tx2rxd.pad1_tx2rx, pad1_tx2rx);
}

/*
 * Stage 1 table 0 action
 */
action tcp_stage1_dummy() {
    // from ki global
    GENERATE_GLOBAL_K

    // k + i for stage 1

	modify_field(to_cpu1_scratch.src_lif, cpu_hdr1.src_lif);
	modify_field(to_cpu1_scratch.lif, cpu_hdr1.lif);
	modify_field(to_cpu1_scratch.qtype, cpu_hdr1.qtype);
	modify_field(to_cpu1_scratch.qid, cpu_hdr1.qid);
	modify_field(to_cpu1_scratch.lkp_vrf, cpu_hdr1.lkp_vrf);
	modify_field(to_cpu1_scratch.pad, cpu_hdr1.pad);
	modify_field(to_cpu1_scratch.lkp_dir, cpu_hdr1.lkp_dir);
	modify_field(to_cpu1_scratch.lkp_inst, cpu_hdr1.lkp_inst);
	modify_field(to_cpu1_scratch.lkp_type, cpu_hdr1.lkp_type);
	modify_field(to_cpu1_scratch.flags, cpu_hdr1.flags);
	modify_field(to_cpu1_scratch.l2_offset, cpu_hdr1.l2_offset);
	modify_field(to_cpu1_scratch.l3_offset_1, cpu_hdr1.l3_offset_1);
}

#define TCP_RX_CB_PARAMS \
        bytes_acked, slow_path_cnt, serq_full_cnt, ooo_cnt, \
        rcv_nxt, ooo_rcv_bitmap, rcv_tstamp, ts_recent, lrcv_time, \
        snd_una, snd_wl1, pred_flags, max_window, bytes_rcvd, \
        snd_wnd, rcv_mss, rto, ecn_flags, state, \
        parsed_state, flag, ato, serq_pidx, quick, snd_wscale, pending, ca_flags, \
        num_dup_acks, write_serq, fastopen_rsk, pingpong, \
        ooo_in_rx_q, alloc_descr

#define TCP_RX_CB_D \
    modify_field(tcp_rx_d.bytes_acked, bytes_acked); \
    modify_field(tcp_rx_d.slow_path_cnt, slow_path_cnt); \
    modify_field(tcp_rx_d.serq_full_cnt, serq_full_cnt); \
    modify_field(tcp_rx_d.ooo_cnt, ooo_cnt); \
    modify_field(tcp_rx_d.rcv_nxt, rcv_nxt); \
    modify_field(tcp_rx_d.ooo_rcv_bitmap, ooo_rcv_bitmap); \
    modify_field(tcp_rx_d.rcv_tstamp, rcv_tstamp); \
    modify_field(tcp_rx_d.ts_recent, ts_recent); \
    modify_field(tcp_rx_d.lrcv_time, lrcv_time); \
    modify_field(tcp_rx_d.snd_una, snd_una); \
    modify_field(tcp_rx_d.snd_wl1, snd_wl1); \
    modify_field(tcp_rx_d.pred_flags, pred_flags); \
    modify_field(tcp_rx_d.max_window, max_window); \
    modify_field(tcp_rx_d.bytes_rcvd, bytes_rcvd); \
    modify_field(tcp_rx_d.snd_wnd, snd_wnd); \
    modify_field(tcp_rx_d.rcv_mss, rcv_mss); \
    modify_field(tcp_rx_d.rto, rto); \
    modify_field(tcp_rx_d.ecn_flags, ecn_flags); \
    modify_field(tcp_rx_d.state, state); \
    modify_field(tcp_rx_d.parsed_state, parsed_state); \
    modify_field(tcp_rx_d.flag, flag); \
    modify_field(tcp_rx_d.ato, ato); \
    modify_field(tcp_rx_d.serq_pidx, serq_pidx); \
    modify_field(tcp_rx_d.quick, quick); \
    modify_field(tcp_rx_d.snd_wscale, snd_wscale); \
    modify_field(tcp_rx_d.pending, pending); \
    modify_field(tcp_rx_d.ca_flags, ca_flags); \
    modify_field(tcp_rx_d.num_dup_acks, num_dup_acks); \
    modify_field(tcp_rx_d.write_serq, write_serq); \
    modify_field(tcp_rx_d.fastopen_rsk, fastopen_rsk); \
    modify_field(tcp_rx_d.pingpong, pingpong); \
    modify_field(tcp_rx_d.ooo_in_rx_q, ooo_in_rx_q); \
    modify_field(tcp_rx_d.alloc_descr, alloc_descr);

/*
 * Stage 2 table 0 action
 */
action tcp_rx(TCP_RX_CB_PARAMS) {
    // k + i for stage 1


    // from to_stage 2
    if (write_serq == 1) {
        modify_field(to_s2_scratch.data_ofs_rsvd, to_s2.data_ofs_rsvd);
        modify_field(to_s2_scratch.rcv_wup, to_s2.rcv_wup);
        modify_field(to_s2_scratch.seq, to_s2.seq);
        modify_field(to_s2_scratch.serq_cidx, to_s2.serq_cidx);
        modify_field(to_s2_scratch.ip_dsfield, to_s2.ip_dsfield);
    }

    if (write_serq == 0) {
        modify_field(to_cpu2_scratch.l3_offset_2, cpu_hdr2.l3_offset_2);
        modify_field(to_cpu2_scratch.l4_offset, cpu_hdr2.l4_offset);
        modify_field(to_cpu2_scratch.payload_offset, cpu_hdr2.payload_offset);
        modify_field(to_cpu2_scratch.tcp_flags, cpu_hdr2.tcp_flags);
        modify_field(to_cpu2_scratch.tcp_seqNo, cpu_hdr2.tcp_seqNo);
        modify_field(to_cpu2_scratch.tcp_AckNo_1, cpu_hdr2.tcp_AckNo_1);
    }


    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage
    modify_field(s1_s2s_scratch.payload_len, s1_s2s.payload_len);
    modify_field(s1_s2s_scratch.ack_seq, s1_s2s.ack_seq);
    modify_field(s1_s2s_scratch.snd_nxt, s1_s2s.snd_nxt);
    modify_field(s1_s2s_scratch.packets_out, s1_s2s.packets_out);
    modify_field(s1_s2s_scratch.window, s1_s2s.window);
    modify_field(s1_s2s_scratch.rcv_mss_shft, s1_s2s.rcv_mss_shft);
    modify_field(s1_s2s_scratch.rcv_tsval, s1_s2s.rcv_tsval);
    modify_field(s1_s2s_scratch.quick_acks_decr, s1_s2s.quick_acks_decr);
    modify_field(s1_s2s_scratch.fin_sent, s1_s2s.fin_sent);

    // d for stage 2 tcp-rx
    TCP_RX_CB_D
}

/*
 * Stage 3 table 0 action2
 */
action tcp_ooo_book_keeping (start_seq0, end_seq0, tail_index0,
                             start_seq1, end_seq1, tail_index1,
                             start_seq2, end_seq2, tail_index2,
                             start_seq3, end_seq3, tail_index3, 
                             ooo_alloc_fail, ooo_queue0_full, 
                             ooo_queue1_full, ooo_queue2_full, 
                             ooo_queue3_full, ooo_pad)
{
    GENERATE_GLOBAL_K
    modify_field(ooo_book_keeping.start_seq0, start_seq0);
    modify_field(ooo_book_keeping.end_seq0, end_seq0);
    modify_field(ooo_book_keeping.tail_index0, tail_index0);
    modify_field(ooo_book_keeping.start_seq1, start_seq1);
    modify_field(ooo_book_keeping.end_seq1, end_seq1);
    modify_field(ooo_book_keeping.tail_index1, tail_index1);
    modify_field(ooo_book_keeping.start_seq2, start_seq2);
    modify_field(ooo_book_keeping.end_seq2, end_seq2);
    modify_field(ooo_book_keeping.tail_index2, tail_index2);
    modify_field(ooo_book_keeping.start_seq3, start_seq3);
    modify_field(ooo_book_keeping.end_seq3, end_seq3);
    modify_field(ooo_book_keeping.tail_index3, tail_index3);
    modify_field(ooo_book_keeping.ooo_queue0_full, ooo_queue0_full);
    modify_field(ooo_book_keeping.ooo_queue1_full, ooo_queue1_full);
    modify_field(ooo_book_keeping.ooo_queue2_full, ooo_queue2_full);
    modify_field(ooo_book_keeping.ooo_queue3_full, ooo_queue3_full);
    modify_field(ooo_book_keeping.ooo_alloc_fail, ooo_alloc_fail);
    modify_field(ooo_book_keeping.ooo_pad, ooo_pad);
}

                             
/*
 * Stage 3 table 0 action1
 */
action tcp_ack(TCP_RX_CB_PARAMS) {
    // k + i for stage 3

    // from ki global
    GENERATE_GLOBAL_K

    // from to_stage 3
    if (flag == 0) {
        modify_field(to_s3_scratch.flag, to_s3.flag);
    }
    if (flag == 1) {
        modify_field(to_cpu3_scratch.tcp_AckNo_2, cpu_hdr3.tcp_AckNo_2);
        modify_field(to_cpu3_scratch.tcp_window, cpu_hdr3.tcp_window);
        modify_field(to_cpu3_scratch.tcp_options, cpu_hdr3.tcp_options);
        modify_field(to_cpu3_scratch.tcp_mss, cpu_hdr3.tcp_mss);
        modify_field(to_cpu3_scratch.tcp_ws, cpu_hdr3.tcp_ws);
    }

    // from stage to stage

    // d for stage 3 (reuse tcp-rx cb)
    TCP_RX_CB_D
}

/*
 * Stage 3 table 1 action
 */
action read_rnmdr(rnmdr_pidx, rnmdr_pidx_full) {
    // d for stage 2 table 1 read-rnmdr-idx
    modify_field(read_rnmdr_d.rnmdr_pidx, rnmdr_pidx);
    modify_field(read_rnmdr_d.rnmdr_pidx_full, rnmdr_pidx_full);
}

/*
 * Stage 3 table 2 action
 */
action read_rnmpr(rnmpr_pidx, rnmpr_pidx_full) {
    // d for stage 2 table 2 read-rnmpr-idx
    modify_field(read_rnmpr_d.rnmpr_pidx, rnmpr_pidx);
    modify_field(read_rnmpr_d.rnmpr_pidx_full, rnmpr_pidx_full);
}

/*
 * Stage 3 table 3 action
 */
action l7_read_rnmdr(rnmdr_pidx, rnmdr_pidx_full) {
    // d for stage 2 table 3 read-serq-idx
    modify_field(l7_read_rnmdr_d.rnmdr_pidx, rnmdr_pidx);
    modify_field(l7_read_rnmdr_d.rnmdr_pidx_full, rnmdr_pidx_full);

}

/*
 * Stage 4 table 0 action
 */
action tcp_rtt(RTT_D_PARAMS) {
    // k + i for stage 4

    // from to_stage
    modify_field(to_s4_scratch.snd_nxt, to_s4.snd_nxt);
    modify_field(to_s4_scratch.rcv_tsval, to_s4.rcv_tsval);
    modify_field(to_s4_scratch.rcv_tsecr, to_s4.rcv_tsecr);

    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage

    // d for stage 4 tcp-rtt
    GENERATE_RTT_D
}

/*
 * Stage 4 table 1 action
 */
action rdesc_alloc(desc, pad) {
    // k + i for stage 3 table 1

    // from to_stage 4

    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage
    modify_field(s4_t1_s2s_scratch.rnmdr_pidx, s4_t1_s2s.rnmdr_pidx);
    modify_field(s4_t1_s2s_scratch.ato, s4_t1_s2s.ato);

    // d for stage 4 table 1
    modify_field(rdesc_alloc_d.desc, desc);
    modify_field(rdesc_alloc_d.pad, pad);
}

/*
 * Stage 4 table 2 action
 */
action rpage_alloc(page, pad) {
    // k + i for stage 3 table 2

    // from to_stage 4

    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage
    modify_field(s4_t2_s2s_scratch.rnmpr_pidx, s4_t2_s2s.rnmpr_pidx);

    // d for stage 4 table 2
    modify_field(rpage_alloc_d.page, page);
    modify_field(rpage_alloc_d.pad, pad);
}

/*
 * Stage 4 table 3 action1
 */
action l7_rdesc_alloc(desc, pad) {
    // k + i for stage 3 table 1

    // from to_stage 4

    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage
    //modify_field(s4_t1_s2s_scratch.rnmdr_pidx, s4_t1_s2s.rnmdr_pidx);

    // d for stage 4 table 1
    modify_field(l7_rdesc_alloc_d.desc, desc);
    modify_field(l7_rdesc_alloc_d.pad, pad);
}


/*
 * Stage 4 table 3 action2
 */
action tcp_ooo_qbase_sem_alloc(ooo_qbase_pindex, ooo_qbase_pindex_full)
{
    modify_field(read_ooo_qbase_index.ooo_qbase_pindex, ooo_qbase_pindex);
    modify_field(read_ooo_qbase_index.ooo_qbase_pindex_full, ooo_qbase_pindex_full);
}

/*
 * Stage 5 table 0 action1
 */
action tcp_fc(page, descr, page_cnt, l7_descr, rcv_wnd, rcv_wscale, cpu_id) {
    // k + i for stage 5

    // from to_stage 5
    modify_field(to_s5_scratch.page, to_s5.page);
    modify_field(to_s5_scratch.descr, to_s5.descr);
    modify_field(to_s5_scratch.l7_descr, to_s5.l7_descr);

    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage

    // d for stage 5 table 0
    modify_field(tcp_fc_d.page, page);
    modify_field(tcp_fc_d.descr, descr);
    modify_field(tcp_fc_d.page_cnt, page_cnt);
    modify_field(tcp_fc_d.l7_descr, l7_descr);
    modify_field(tcp_fc_d.rcv_wnd, rcv_wnd);
    modify_field(tcp_fc_d.rcv_wscale, rcv_wscale);
    modify_field(tcp_fc_d.cpu_id, cpu_id);
}

/*
 * Stage 5 table 1 action
 */
action tcp_ooo_qbase_addr_get(ooo_qbase_addr)
{
    modify_field(read_ooo_base_addr.ooo_qbase_addr, ooo_qbase_addr);
}


/*
 * Stage 6 table 0 action
 */
action write_serq(serq_base, nde_addr, nde_offset, nde_len, curr_ts,
        ato, ooo_offset,
        pkts_rcvd, pages_alloced, desc_alloced, debug_num_pkt_to_mem,
        debug_num_phv_to_mem, ft_pi, rx2tx_send_ack_pi,
        rx2tx_clean_retx_pi, rx2tx_fast_retx_pi) {
    // k + i for stage 6

    // from to_stage 6
    modify_field(to_s6_scratch.page, to_s6.page);
    modify_field(to_s6_scratch.descr, to_s6.descr);
    modify_field(to_s6_scratch.payload_len, to_s6.payload_len);
    modify_field(to_s6_scratch.serq_pidx, to_s6.serq_pidx);
    modify_field(to_s6_scratch.ooo_offset, to_s6.ooo_offset);

    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage
    modify_field(s6_s2s_scratch.payload_len, s6_s2s.payload_len);

    // d for stage 5 table 0
    modify_field(write_serq_d.serq_base, serq_base);
    modify_field(write_serq_d.nde_addr, nde_addr);
    modify_field(write_serq_d.nde_offset, nde_offset);
    modify_field(write_serq_d.nde_len, nde_len);
    modify_field(write_serq_d.curr_ts, curr_ts);
    modify_field(write_serq_d.pkts_rcvd, pkts_rcvd);
    modify_field(write_serq_d.pages_alloced, pages_alloced);
    modify_field(write_serq_d.desc_alloced, desc_alloced);
    modify_field(write_serq_d.debug_num_pkt_to_mem, debug_num_pkt_to_mem);
    modify_field(write_serq_d.debug_num_phv_to_mem, debug_num_phv_to_mem);
    modify_field(write_serq_d.ft_pi, ft_pi);
    modify_field(write_serq_d.rx2tx_send_ack_pi, rx2tx_send_ack_pi);
    modify_field(write_serq_d.rx2tx_clean_retx_pi, rx2tx_clean_retx_pi);
    modify_field(write_serq_d.rx2tx_fast_retx_pi, rx2tx_fast_retx_pi);
}

/*
 * Stage 6 table 1 action
 */
action write_arq(ARQ_PI_PARAMS) {

    // k + i for stage 6

    // from to_stage 6
    modify_field(to_s6_scratch.page, to_s6.page);
    modify_field(to_s6_scratch.descr, to_s6.descr);
    modify_field(to_s6_scratch.payload_len, to_s6.payload_len);


    // from stage to stage
    modify_field(s6_t1_s2s_scratch.rnmdr_pidx, s6_t1_s2s.rnmdr_pidx);
    modify_field(s6_t1_s2s_scratch.ato, s6_t1_s2s.ato);
    modify_field(s6_t1_s2s_scratch.cpu_id, s6_t1_s2s.cpu_id);

    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage

    // d for stage 6 table 1
    GENERATE_ARQ_PI_D(arq_rx_pi_d) 
}

/*
 * Stage 6 table 2 action
 */
action write_l7q(l7q_base, l7q_pidx) {
    
    // from to_stage 6
    modify_field(to_s6_scratch.page, to_s6.page);
    modify_field(to_s6_scratch.descr, to_s6.descr);
    modify_field(to_s6_scratch.payload_len, to_s6.payload_len);

    // from ki global
    GENERATE_GLOBAL_K
    
    // from stage to stage
    modify_field(s6_t2_s2s_scratch.l7_descr, s6_t2_s2s.l7_descr);

    // d for stage 6 table 2
    modify_field(write_l7q_d.l7q_base, l7q_base);
    modify_field(write_l7q_d.l7q_pidx, l7q_pidx);
}


/*
 * Stage 6 table 3 action
 */
action tcp_ooo_qbase_cb_load(ooo_qbase_addr0, ooo_qbase_addr1, 
                           ooo_qbase_addr2, ooo_qbase_addr3, ooo_qbase_pad)
{
    GENERATE_GLOBAL_K

    // from to_stage 6
    modify_field(to_s6_scratch.page, to_s6.page);
    modify_field(to_s6_scratch.descr, to_s6.descr);
    modify_field(to_s6_scratch.payload_len, to_s6.payload_len);
    modify_field(to_s6_scratch.serq_pidx, to_s6.serq_pidx);
    modify_field(to_s6_scratch.ooo_offset, to_s6.ooo_offset);
    modify_field(to_s6_scratch.ooo_queue_id, to_s6.ooo_queue_id);
    modify_field(to_s6_scratch.ooo_tail_index, to_s6.ooo_tail_index);

    modify_field(s6_t3_s2s_scratch.ooo_qbase_addr, s6_t3_s2s.ooo_qbase_addr);

    modify_field(ooo_qbase_addr.ooo_qbase_addr0, ooo_qbase_addr0);
    modify_field(ooo_qbase_addr.ooo_qbase_addr1, ooo_qbase_addr1);
    modify_field(ooo_qbase_addr.ooo_qbase_addr2, ooo_qbase_addr2);
    modify_field(ooo_qbase_addr.ooo_qbase_addr3, ooo_qbase_addr3);
    modify_field(ooo_qbase_addr.ooo_qbase_pad, ooo_qbase_pad);

}

/*
 * Stage 7 table 0 action
 */
action stats() {
    // k + i for stage 7

    // from to_stage 7
    modify_field(to_s7_scratch.bytes_rcvd, to_s7.bytes_rcvd);
    modify_field(to_s7_scratch.pkts_rcvd, to_s7.pkts_rcvd);
    modify_field(to_s7_scratch.pages_alloced, to_s7.pages_alloced);
    modify_field(to_s7_scratch.desc_alloced, to_s7.desc_alloced);
    modify_field(to_s7_scratch.debug_num_pkt_to_mem, to_s7.debug_num_pkt_to_mem);
    modify_field(to_s7_scratch.debug_num_phv_to_mem, to_s7.debug_num_phv_to_mem);
    modify_field(to_s7_scratch.bytes_acked, to_s7.bytes_acked);

    // from ki global
    GENERATE_GLOBAL_K

    // from stage to stage

    // d for stage 7 table 0
}
