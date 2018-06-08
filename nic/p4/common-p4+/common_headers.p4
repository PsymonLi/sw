#include "../../p4/include/intrinsic.p4"

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

#define DMA_CMD_TYPE_MEM2PKT     1
#define DMA_CMD_TYPE_PHV2PKT     2
#define DMA_CMD_TYPE_PHV2MEM     3
#define DMA_CMD_TYPE_PKT2MEM     4
#define DMA_CMD_TYPE_SKIP        5
#define DMA_CMD_TYPE_MEM2MEM     6
#define DMA_CMD_TYPE_NOP         0

#define DMA_CMD_TYPE_MEM2MEM_TYPE_SRC        0
#define DMA_CMD_TYPE_MEM2MEM_TYPE_DST        1
#define DMA_CMD_TYPE_MEM2MEM_TYPE_PHV2MEM    2

#define DOORBELL_UPD_PID_CHECK              1
#
#define DOORBELL_IDX_CTL_NONE               0
#define DOORBELL_IDX_CTL_UPD_CIDX           1
#define DOORBELL_IDX_CTL_UPD_PIDX           2
#define DOORBELL_IDX_CTL_INC_PIDX           2
#
#define DOORBELL_SCHED_CTL_NONE             0
#define DOORBELL_SCHED_CTL_EVAL             1
#define DOORBELL_SCHED_CTL_CLEAR            2
#define DOORBELL_SCHED_CTL_SET              3

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
header_type scratch_metadata_t {
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


header_type dma_cmd_pkt2mem_t {
    fields {
        // pkt2mem - used for copying input packet to memory.
        dma_cmd_pad              : 42;
        dma_cmd_size             : 14;
        dma_cmd_pad1             : 1;
        dma_cmd_override_lif     : 11;
        dma_cmd_addr             : 52;
        dma_cmd_use_override_lif : 1;
        dma_cmd_cache            : 1;
        dma_cmd_host_addr        : 1;
        dma_cmd_round            : 1;
        dma_cmd_eop              : 1;
        dma_cmd_type             : 3;

    }
}

#define DMA_COMMAND_PKT2MEM_FILL(_dma_cmd_pkt2mem, _addr, _size, _is_host, _cache) \
    modify_field(_dma_cmd_pkt2mem.dma_cmd_type , DMA_CMD_TYPE_PKT2MEM); \
    modify_field(_dma_cmd_pkt2mem.dma_cmd_addr , _addr); \
    modify_field(_dma_cmd_pkt2mem.dma_cmd_host_addr , _is_host); \
    modify_field(_dma_cmd_pkt2mem.dma_cmd_size , _size); \
    modify_field(_dma_cmd_pkt2mem.dma_cmd_cache , _cache); 

header_type dma_cmd_phv2mem_t {
    fields {
        dma_cmd_pad              : 33;   
        dma_cmd_override_lif     : 11;
        dma_cmd_addr             : 52;
        dma_cmd_barrier          : 1;
        dma_cmd_round            : 1;
        dma_cmd_pcie_msg         : 1;
        dma_cmd_use_override_lif : 1;
        dma_cmd_phv_end_addr     : 10;
        dma_cmd_phv_start_addr   : 10;
        dma_cmd_fence_fence      : 1;
        dma_cmd_wr_fence         : 1;
        dma_cmd_cache            : 1;
        dma_cmd_host_addr        : 1;
        dma_cmd_eop              : 1;
        dma_cmd_type             : 3;
    }
}

#define DMA_COMMAND_PHV2MEM_FILL(_dma_cmd_phv2mem, _addr, _start, _end, _is_host, _cache, _barrier, _fence) \
    modify_field(_dma_cmd_phv2mem.dma_cmd_type, DMA_CMD_TYPE_PHV2MEM); \
    modify_field(_dma_cmd_phv2mem.dma_cmd_addr, _addr); \
    modify_field(_dma_cmd_phv2mem.dma_cmd_phv_start_addr, _start); \
    modify_field(_dma_cmd_phv2mem.dma_cmd_phv_end_addr, _end); \
    modify_field(_dma_cmd_phv2mem.dma_cmd_host_addr, _is_host); \
    modify_field(_dma_cmd_phv2mem.dma_cmd_cache, _cache); \
    modify_field(_dma_cmd_phv2mem.dma_cmd_barrier, _barrier); \
    modify_field(_dma_cmd_phv2mem.dma_cmd_wr_fence, _fence);

header_type dma_cmd_mem2mem_t {
    fields {
        dma_cmd_pad              : 16;
        dma_cmd_size             : 14;
        dma_cmd_pad1             : 1;
        dma_cmd_override_lif     : 11;
        dma_cmd_addr             : 52;
        dma_cmd_barrier          : 1;
        dma_cmd_round            : 1;
        dma_cmd_pcie_msg         : 1;
        dma_cmd_use_override_lif : 1;
        dma_cmd_phv_end_addr     : 10;
        dma_cmd_phv_start_addr   : 10;
        dma_cmd_fence_fence      : 1;
        dma_cmd_wr_fence         : 1;
        dma_cmd_cache            : 1;
        dma_cmd_host_addr        : 1;
        dma_cmd_mem2mem_type     : 2;
        dma_cmd_eop              : 1;
        dma_cmd_type             : 3;
    }
}

#define DMA_COMMAND_MEM2MEM_FILL(_dma_cmd_m2m_src, _dma_cmd_m2m_dst, _src_addr, _src_host, _dst_addr, _dst_host, _size, _cache, _fence, _barrier) \
    modify_field(_dma_cmd_m2m_src.dma_cmd_type, DMA_CMD_TYPE_MEM2MEM); \
    modify_field(_dma_cmd_m2m_src.dma_cmd_mem2mem_type, DMA_CMD_TYPE_MEM2MEM_TYPE_SRC); \ 
    modify_field(_dma_cmd_m2m_src.dma_cmd_addr, _src_addr); \
    modify_field(_dma_cmd_m2m_src.dma_cmd_host_addr, _src_host); \
    modify_field(_dma_cmd_m2m_src.dma_cmd_size, _size); \
    modify_field(_dma_cmd_m2m_src.dma_cmd_cache, _cache); \
    modify_field(_dma_cmd_m2m_src.dma_cmd_wr_fence, _fence); \
    modify_field(_dma_cmd_m2m_src.dma_cmd_barrier, _barrier); \
    modify_field(_dma_cmd_m2m_dst.dma_cmd_type, DMA_CMD_TYPE_MEM2MEM); \
    modify_field(_dma_cmd_m2m_src.dma_cmd_mem2mem_type, DMA_CMD_TYPE_MEM2MEM_TYPE_DST); \ 
    modify_field(_dma_cmd_m2m_dst.dma_cmd_addr, _dst_addr); \
    modify_field(_dma_cmd_m2m_dst.dma_cmd_host_addr, _dst_host); \
    modify_field(_dma_cmd_m2m_dst.dma_cmd_size, _size); \
    modify_field(_dma_cmd_m2m_dst.dma_cmd_cache, _cache); \
    modify_field(_dma_cmd_m2m_dst.dma_cmd_wr_fence, _fence); \
    modify_field(_dma_cmd_m2m_dst.dma_cmd_barrier, _barrier); 

header_type dma_cmd_generic_t {
    fields {
        dma_cmd_pad : 125;
        dma_cmd_type : 3;
    }
}

header_type dma_cmd_phv2pkt_t {
    fields {
        dma_cmd_phv2pkt_pad     : 41;  
        dma_cmd_phv_end_addr3   : 10;
        dma_cmd_phv_start_addr3 : 10;
        dma_cmd_phv_end_addr2   : 10;
        dma_cmd_phv_start_addr2 : 10;
        dma_cmd_phv_end_addr1   : 10;
        dma_cmd_phv_start_addr1 : 10;
        dma_cmd_phv_end_addr    : 10;
        dma_cmd_phv_start_addr  : 10;
        dma_cmd_cmdsize         : 2 ;
        dma_pkt_eop             : 1 ;
        dma_cmd_eop             : 1 ;
        dma_cmd_type            : 3 ;
    }
}

#define DMA_COMMAND_PHV2PKT_FILL(_dma_cmd_phv2pkt, _start, _end, _pkt_eop) \
    modify_field(_dma_cmd_phv2pkt.dma_cmd_type, DMA_CMD_TYPE_PHV2PKT); \
    modify_field(_dma_cmd_phv2pkt.dma_cmd_phv_start_addr, _start); \
    modify_field(_dma_cmd_phv2pkt.dma_cmd_phv_end_addr, _end); \
    modify_field(_dma_cmd_phv2pkt.dma_pkt_eop, _pkt_eop);

header_type dma_cmd_mem2pkt_t {
    fields {
        dma_cmd_mem2pkt_pad      : 42;
        dma_cmd_size             : 14;
        dma_cmd_mem2pkt_pad1     : 1;
        dma_cmd_override_lif     : 11;
        dma_cmd_addr             : 52;
        dma_cmd_use_override_lif : 1;
        dma_cmd_cache            : 1;
        dma_cmd_host_addr        : 1;
        dma_pkt_eop              : 1;
        dma_cmd_eop              : 1;
        dma_cmd_type             : 3;
    }
}

#define DMA_COMMAND_MEM2PKT_FILL(_dma_cmd_mem2pkt, _addr, _size, _pkt_eop, _cache, _host_addr) \
    modify_field(_dma_cmd_mem2pkt.dma_cmd_type, DMA_CMD_TYPE_MEM2PKT); \
    modify_field(_dma_cmd_mem2pkt.dma_cmd_addr, _addr); \
    modify_field(_dma_cmd_mem2pkt.dma_cmd_size, _size); \
    modify_field(_dma_cmd_mem2pkt.dma_pkt_eop, _pkt_eop); \
    modify_field(_dma_cmd_mem2pkt.dma_cmd_cache, _cache); \
    modify_field(_dma_cmd_mem2pkt.dma_cmd_host_addr, _host_addr); 

header_type dma_cmd_skip_t {
    fields {
        dma_cmd_skip_pad        : 109;
        dma_cmd_skip_to_eop     : 1;
        dma_cmd_size            : 14;
        dma_cmd_eop             : 1;
        dma_cmd_type            : 3;
    }
}

header_type doorbell_addr_t {
    fields {
        offset : 3;
        qtype : 3;
        lif : 11;
        upd_pid_chk : 1;
        upd_index_ctl : 2;
        upd_sched_ctl : 2;
    }
}

#define DOORBELL_ADDR_FILL(_doorbell, _offset, _qtype, _lif, _upd_pid_chk, \
                           _upd_index_ctl, _upd_sched_ctl) \
    modify_field(_doorbell.offset, _offset); \
    modify_field(_doorbell.qtype, _qtype); \
    modify_field(_doorbell.lif, _lif); \
    modify_field(_doorbell.upd_pid_chk, _upd_pid_chk); \
    modify_field(_doorbell.upd_index_ctl, _upd_index_ctl); \
    modify_field(_doorbell.upd_sched_ctl, _upd_sched_ctl);

header_type doorbell_data_t {
    fields {
        pid : 16;
        qid : 24;
        pad : 5;
        ring : 3;
        index : 16;
    }
}

header_type doorbell_data_raw_t {
    fields {
        data : 64;
    }
}

header_type dma_cmd_start_pad_t {
    fields {
        dma_cmd_start_pad : 64;
    }
}

#define DOORBELL_DATA_FILL(_doorbell, _index, _ring, _qid, _pid) \
    modify_field(_doorbell.index, _index); \
    modify_field(_doorbell.ring, _ring); \
    modify_field(_doorbell.pad, 0); \
    modify_field(_doorbell.qid, _qid); \
    modify_field(_doorbell.pid, _pid);

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

header_type rdma_scratch_metadata_t {
    fields {
        // All the page-ids are encoded as 22-bit, assuming HBM page
        // size is 4K, with appropriate shift it would form 22+12=34-bit
        // hbm address.
        // Where as CQCB base addr is encoded as 24-bit, so to get 1K alignment,
        // with appropriate shift it would form 24+10=34-bit hbm address.
        //QTYPEs on LIF can be allocated differently on different LIFs
        //This mask will have the respective bit set for all qtype values
        //where RDMA is enabled (if enabled)
        //For case where RDMA is not enabled, it should be set to 0
        rdma_en_qtype_mask : 8;

        //Per LIF PageTranslationTable and MemoryRegionWindowTable
        //are allocated adjacent to each other in HBM in that order.
        //This is the base page_id of that allocation.
        //Assumption is that it is 4K Byte(page) aligned and
        //number of PT entries are in power of 2s.
        pt_base_addr_page_id: 22;
        log_num_pt_entries: 7;

        //Per LIF CQCB and EQCB tables
        //are allocated adjacent to each other in HBM in that order.
        //CQCB address is 1K aligned, so store only 24 bits
        cqcb_base_addr_hi: 24;
        log_num_cq_entries: 5;

        //RQCB prefetch uses per LIF global ring
        //This is the base address of that ring
        prefetch_pool_base_addr_page_id: 22;
        log_num_prefetch_pool_entries:5;
        sq_qtype: 3;
        rq_qtype: 3;

        reserved: 101;

    }
}

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
