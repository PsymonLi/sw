#define GLOBAL_WIDTH 128
#define STAGE_2_STAGE_WIDTH 160
#define TO_STAGE_WIDTH 128
#define RAW_TABLE_ADDR_WIDTH 64
#define RAW_TABLE_PC_WIDTH 28
#define RAW_TABLE_SIZE_WIDTH 3

#define ADDRESS_WIDTH 64
#define HBM_ADDRESS_WIDTH 32
#define HBM_FULL_ADDRESS_WIDTH 34

#define RAW_TABLE_SIZE_1    0
#define RAW_TABLE_SIZE_2    1
#define RAW_TABLE_SIZE_4    2
#define RAW_TABLE_SIZE_8    3
#define RAW_TABLE_SIZE_16   4
#define RAW_TABLE_SIZE_32   5
#define RAW_TABLE_SIZE_64   6
#define RAW_TABLE_SIZE_128  7

#define LIF_TABLE_SIZE 2048
#define EGRESS_RATE_LIMITER_TABLE_SIZE 2048

header_type p4_2_p4plus_app_header_t {
    fields {
        app_type : 4;
        table0_valid : 1;
        table1_valid : 1;
        table2_valid : 1;
        table3_valid : 1;
        gft_flow_id : 24;
        app_data0 : 224;
        app_data1 : 88;
        app_data2 : 72;
    }
}

header_type p4_2_p4plus_ext_app_header_t {
    fields {
        app_data3 : 96;
    }
}

header_type p4plus_2_p4_app_header_t {
    fields {
        app_type : 4;
        table0_valid : 1;
        table1_valid : 1;
        table2_valid : 1;
        table3_valid : 1;
        app_data1 : 256;
    }
}

header_type p4plus_common_global_t {
    fields {
        global_data : GLOBAL_WIDTH;
    }
}

header_type p4plus_common_s2s_t {
    fields {
        s2s_data : STAGE_2_STAGE_WIDTH;
    }
}

header_type p4plus_common_to_stage_t {
    fields {
        to_stage_data : TO_STAGE_WIDTH;
    }
}

header_type p4plus_common_raw_table_engine_phv_t {
    fields {
        table_lock_en : 1;
        table_raw_table_size : RAW_TABLE_SIZE_WIDTH;
        table_pc   : RAW_TABLE_PC_WIDTH;
        table_addr : RAW_TABLE_ADDR_WIDTH;
    }
}

header_type p4plus_common_table_engine_phv_t {
    fields {
        to_stage_0 : TO_STAGE_WIDTH;
        to_stage_1 : TO_STAGE_WIDTH;
        to_stage_2 : TO_STAGE_WIDTH;
        to_stage_3 : TO_STAGE_WIDTH;

        to_stage_4 : TO_STAGE_WIDTH;
        to_stage_5 : TO_STAGE_WIDTH;
        to_stage_6 : TO_STAGE_WIDTH;
        to_stage_7 : TO_STAGE_WIDTH;

    }
}

header_type p4plus_app_specific_scratch_phv_t {
    fields {
        rsvd0 : 64;
        rsvd1 : 64;
        rsvd2 : 64;
        rsvd3 : 64;
        rsvd4 : 64;
        rsvd5 : 64;
        rsvd6 : 64;
        rsvd7 : 64;
    }
}

// this is for preserving D only
header_type common_scratch_metadata_t {
    fields {
        data0 : 64;
        data1 : 64;
        data2 : 64;
        data3 : 64;
        data4 : 64;
        data5 : 64;
        data6 : 64;
        data7 : 64;
    }
}

#define SCRATCH_METADATA_INIT(scratch) 		\
    modify_field(scratch.data0, data0);		\
    modify_field(scratch.data1, data1);		\
    modify_field(scratch.data2, data2);		\
    modify_field(scratch.data3, data3);		\
    modify_field(scratch.data4, data4);		\
    modify_field(scratch.data5, data5);		\
    modify_field(scratch.data6, data6);		\
    modify_field(scratch.data7, data7);		\

#define SCRATCH_METADATA_INIT_7(scratch) 	\
    modify_field(scratch.data0, data0);		\
    modify_field(scratch.data1, data1);		\
    modify_field(scratch.data2, data2);		\
    modify_field(scratch.data3, data3);		\
    modify_field(scratch.data4, data4);		\
    modify_field(scratch.data5, data5);		\
    modify_field(scratch.data6, data6);
#if 0
#define TABLE0_PARAMS_INIT(scratch, te)					\
   modify_field(scratch.table0_pc, te.table0_pc);			\
   modify_field(scratch.table0_raw_table_size,te.table0_raw_table_size);\
   modify_field(scratch.table0_lock_en, te.table0_lock_en);		\
   modify_field(scratch.table0_ki_global, te.table0_ki_global);		\
   modify_field(scratch.table0_ki_s2s, te.table0_ki_s2s);		\

#define TABLE1_PARAMS_INIT(scratch, te)					\
   modify_field(scratch.table0_pc, te.table1_pc);			\
   modify_field(scratch.table0_raw_table_size,te.table1_raw_table_size);\
   modify_field(scratch.table0_lock_en, te.table1_lock_en);		\
   modify_field(scratch.table0_ki_global, te.table0_ki_global);		\
   modify_field(scratch.table0_ki_s2s, te.table1_ki_s2s);		\

#define TABLE2_PARAMS_INIT(scratch, te)					\
   modify_field(scratch.table0_pc, te.table2_pc);			\
   modify_field(scratch.table0_raw_table_size,te.table2_raw_table_size);\
   modify_field(scratch.table0_lock_en, te.table2_lock_en);             \
   modify_field(scratch.table0_ki_global, te.table0_ki_global);         \
   modify_field(scratch.table0_ki_s2s, te.table2_ki_s2s);               \

#define TABLE3_PARAMS_INIT(scratch, te)                                 \
   modify_field(scratch.table0_pc, te.table3_pc);                       \
   modify_field(scratch.table0_raw_table_size,te.table3_raw_table_size);\
   modify_field(scratch.table0_lock_en, te.table3_lock_en);             \
   modify_field(scratch.table0_ki_global, te.table0_ki_global);         \
   modify_field(scratch.table0_ki_s2s, te.table3_ki_s2s);
#endif

header_type dma_cmd_start_pad_t {
    fields {
        dma_cmd_start_pad : 64;
    }
}

header_type pkt_descr_t {
    fields {
        scratch : 512;
        A0 : 64;
        O0 : 32;
        L0 : 32;
        A1 : 64;
        O1 : 32;
        L1 : 32;
        A2 : 64;
        O2 : 32;
        L2 : 32;
        next_addr : 64;
        next_pkt : 64;
    }
}

header_type pkt_descr_scratch_t {
    fields {
        scratch : 512;
    }
}

header_type pkt_descr_aol_t {
    fields {
        A0 : 64;
        O0 : 32;
        L0 : 32;
        A1 : 64;
        O1 : 32;
        L1 : 32;
        A2 : 64;
        O2 : 32;
        L2 : 32;
        next_addr : 64;
        next_pkt : 64;

    }
}

header_type ring_entry_t {
    fields {
        descr_addr : 64;
    }
}

/*
 * Ring entry containing both address and length
 */
header_type hbm_al_ring_entry_t {
    fields {
        pad : 16;
        len : 14;
        descr_addr : HBM_FULL_ADDRESS_WIDTH;
    }
}

header_type semaphore_ci_t {
    fields {
        index : 32;
    }
}

header_type semaphore_pi_t {
    fields {
        index : 32;
    }
}

#define CAPRI_QSTATE_HEADER_COMMON \
        rsvd                : 8;\
        cosA                : 4;\
        cosB                : 4;\
        cos_sel             : 8;\
        eval_last           : 8;\
        host                : 4;\
        total               : 4;\
        pid                 : 16;\



#define CAPRI_QSTATE_HEADER_RING(_x)		\
        pi_##_x                           : 16;\
        ci_##_x                           : 16;\

header_type tcp_header_t {
    fields {
        source_port : 16;
        dest_port : 16;
        seq_no : 32;
        ack_no : 32;
        data_ofs : 4;
        rsvd : 3;
        flags : 9;
        window : 16;
        cksum : 16;
        urg : 16;
    }
}

header_type tcp_header_pad_t {
    fields {
        kind : 8;
    }
}

header_type tcp_header_ts_option_t {
    fields {
        kind : 8;       // set to 8
        len : 8;        // set to 10
        ts_val : 32;
        ts_ecr : 32;
    }
}

header_type tcp_sack_perm_option_t {
    fields {
        kind : 8;       // set to 4
        len : 8;        // set to 2
    }
}

header_type tcp_sack_option_t {
    fields {
        kind : 8;       // set to 5
        len : 8;        // variable (8*n + 2)
    }
}

/*
 * flags bits:
 *  0       : vlan valid
 *  1       : ipv4 valid
 *  2       : ipv6 valid
 *  3       : ip options present
 *  4       : tcp options present
 *  5-7     : unused
 */
header_type p4_to_p4plus_cpu_pkt_1_t {
    fields {
        src_lif             : 16;

        lif                 : 16;
        qtype               : 8;
        qid                 : 32;

        lkp_vrf             : 16;

        pad                 : 2;
        lkp_dir             : 1;
        lkp_inst            : 1;
        lkp_type            : 4;

        flags               : 8;

        // offsets
        l2_offset           : 16;
        l3_offset_1         : 8;
    }
}

header_type p4_to_p4plus_cpu_pkt_2_t {
    fields {
        l3_offset_2         : 8;
        l4_offset           : 16;
        payload_offset      : 16;

        // qos
        src_tm_iq           : 5;
        pad_1               : 3;

        // flow hash
        flow_hash           : 32;

        // tcp
        tcp_flags           : 8;
        tcp_seqNo           : 32;
        tcp_AckNo_1         : 8;
    }
}

header_type p4_to_p4plus_cpu_pkt_3_t {
    fields {
        tcp_AckNo_2         : 24;
        tcp_window          : 16;
        tcp_options         : 8;
        tcp_mss             : 16;
        tcp_ws              : 8;
    }
}


header_type policer_scratch_metadata_t {
    fields {
        policer_valid       : 1;
        policer_pkt_rate    : 1;
        policer_rlimit_en   : 1;
        policer_rlimit_prof : 2;
        policer_color_aware : 1;
        policer_rsvd        : 1;
        policer_axi_wr_pend : 1;
        policer_burst       : 40;
        policer_rate        : 40;
        policer_tbkt        : 40;
    }
}
