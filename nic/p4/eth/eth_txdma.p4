
#include "defines.h"

/***
 *  Header Type Declarations
 ***/

/***
 *  P-vector Headers
 ***/

header_type p4plus_to_p4_classic_header_t {
    fields {
        p4plus_app_id           : 4;
        table0_valid            : 1;
        table1_valid            : 1;
        table2_valid            : 1;
        table3_valid            : 1;
        flow_index              : 24;
        lkp_inst                : 1;
        compute_inner_l4_csum   : 1;
        compute_l4_csum         : 1;
        insert_vlan_tag         : 1;
        update_udp_len          : 1;
        update_tcp_seq_no       : 1;
        update_ip_len           : 1;
        update_ip_id            : 1;
        udp_opt_bytes           : 8;
        dst_lport               : 11;
        dst_lport_valid         : 1;
        pad1                    : 1;
        tso_last_segment        : 1;
        tso_first_segment       : 1;
        tso_valid               : 1;
        ip_id_delta             : 16;
        tcp_seq_delta           : 32;
        gso_start               : 14;
        compute_inner_ip_csum   : 1;
        compute_ip_csum         : 1;
        gso_offset              : 14;
        flow_index_valid        : 1;
        gso_valid               : 1;
        vlan_tag                : 16;
    }
}

header_type eth_tx_cq_desc_p {
    fields {
        status : 8;
        rsvd : 8;
        comp_index : 16;
        rsvd2 : 64;
        rsvd3 : 24;
        color : 1;
        rsvd4 : 7;
    }
}

/***
 *  D-vector Headers
 ***/

header_type eth_tx_qstate_d {
    fields {
        pc : 8;
        rsvd : 8;
        cosA : 4;
        cosB : 4;
        cos_sel : 8;
        eval_last : 8;
        host : 4;
        total : 4;
        pid : 16;

        p_index0   : 16;
        c_index0   : 16;
        comp_index : 16;
        ci_fetch   : 16;
        ci_miss : 16;

        // sta
        color : 1;
        spec_miss : 1;
        rsvd1 : 6;

        // cfg
        enable : 1;
        host_queue : 1;
        intr_enable : 1;
        rsvd2 : 5;

        ring_base : 64;
        ring_size : 16;
        cq_ring_base : 64;
        intr_assert_index : 16;
        sg_ring_base : 64;

        tso_hdr_addr : 64;
        tso_hdr_len : 10;
        tso_ipid_delta : 16;
        tso_seq_delta : 32;
        spurious_db_cnt : 6;
    }
}

#define HEADER_TX_DESC(n) \
    addr_lo##n : 48; \
    rsvd##n : 4; \
    addr_hi##n : 4; \
    opcode##n : 3; \
    num_sg_elems##n: 5; \
    len##n : 16; \
    vlan_tci##n : 16; \
    hdr_len_lo##n : 8; \
    encap##n : 1; \
    cq_entry##n : 1; \
    vlan_insert##n : 1; \
    rsvd2##n : 3; \
    hdr_len_hi##n: 2; \
    mss_or_csumoff_lo##n : 8; \
    csum_l4_or_eot##n : 1; \
    csum_l3_or_sot##n : 1; \
    mss_or_csumoff_hi##n : 6;

header_type eth_tx_desc_d {
    fields {
        HEADER_TX_DESC(0)
        HEADER_TX_DESC(1)
        HEADER_TX_DESC(2)
        HEADER_TX_DESC(3)
    }
}

#define HEADER_SG_ELEM(n) \
    addr_lo##n : 48; \
    rsvd##n : 4; \
    addr_hi##n : 4; \
    rsvd1##n : 8; \
    len##n : 16; \
    rsvd2##n : 48;

header_type eth_sg_desc_d {
    fields {
        HEADER_SG_ELEM(0)
        HEADER_SG_ELEM(1)
        HEADER_SG_ELEM(2)
        HEADER_SG_ELEM(3)
    }
}

/***
 * K+I Headers
 ***/

header_type eth_tx_global_k {
    fields {
        dma_cur_index : 6;
        sg_desc_addr : 64;
        sg_in_progress : 1;
        num_sg_elems : 5;
        tso_eot : 1;    // end of tso
        tso_sot : 1;    // start of tso
        host_queue : 1;
        cq_entry : 1;   // generate a completion
        intr_enable : 1;    // generate an interrupt
        lif : 11;
        stats : 32;
        drop : 1;
    }
}

header_type eth_tx_t0_s2s_k {
    fields {
        num_todo : 4;
        num_desc : 4;
        do_sg : 1;
        do_tso : 1;
        __pad : 6;
        cq_desc_addr : 64;
        intr_assert_index : 16;
        intr_assert_data : 32;   // Should be byte-aligned for PHV2MEM
    }
}

header_type eth_tx_t2_s2s_k {
    fields {
        tso_hdr_addr : 64;
        tso_hdr_len : 10;
        tso_ipid_delta : 16;
        tso_seq_delta : 32;
    }
}

header_type eth_tx_to_s1_k {
    fields {
        qstate_addr : 64;
    }
}

header_type eth_tx_to_s2_k {
    fields {
        qtype : 3;
        qid : 24;
        my_ci : 16;
        tso_hdr_addr : 64;
        tso_hdr_len : 10;
    }
}

header_type eth_tx_to_s3_k {
    fields {
        HEADER_TX_DESC(0)
    }
}

header_type eth_tx_to_s4_k {
    fields {
        HEADER_TX_DESC(1)
    }
}

header_type eth_tx_to_s5_k {
    fields {
        HEADER_TX_DESC(2)
    }
}

header_type eth_tx_to_s6_k {
    fields {
        HEADER_TX_DESC(3)
    }
}

/*****************************************************************************
 *  D-vector
 *****************************************************************************/
@pragma scratch_metadata
metadata eth_tx_qstate_d eth_tx_qstate;

@pragma scratch_metadata
metadata eth_tx_desc_d eth_tx_desc;

@pragma scratch_metadata
metadata eth_sg_desc_d eth_sg_desc;

/*****************************************************************************
 *  K-vector
 *****************************************************************************/

// Union with Common-TXDMA PHV headers

// Global PHV headers (Available in STAGES=ALL, MPUS=ALL)
@pragma pa_header_union ingress common_global
metadata eth_tx_global_k eth_tx_global;
@pragma scratch_metadata
metadata eth_tx_global_k eth_tx_global_scratch;

// To Stage N PHV headers (Available in STAGE=N, MPUS=ALL)
@pragma pa_header_union ingress to_stage_1
metadata eth_tx_to_s1_k eth_tx_to_s1;
@pragma scratch_metadata
metadata eth_tx_to_s1_k eth_tx_to_s1_scratch;

@pragma pa_header_union ingress to_stage_2
metadata eth_tx_to_s2_k eth_tx_to_s2;
@pragma scratch_metadata
metadata eth_tx_to_s2_k eth_tx_to_s2_scratch;

@pragma pa_header_union ingress to_stage_3
metadata eth_tx_to_s3_k eth_tx_to_s3;
@pragma scratch_metadata
metadata eth_tx_to_s3_k eth_tx_to_s3_scratch;

@pragma pa_header_union ingress to_stage_4
metadata eth_tx_to_s4_k eth_tx_to_s4;
@pragma scratch_metadata
metadata eth_tx_to_s4_k eth_tx_to_s4_scratch;

@pragma pa_header_union ingress to_stage_5
metadata eth_tx_to_s5_k eth_tx_to_s5;
@pragma scratch_metadata
metadata eth_tx_to_s5_k eth_tx_to_s5_scratch;

@pragma pa_header_union ingress to_stage_6
metadata eth_tx_to_s6_k eth_tx_to_s6;
@pragma scratch_metadata
metadata eth_tx_to_s6_k eth_tx_to_s6_scratch;

// Stage to Stage headers (Available in STAGES=ALL, MPUS=N)
@pragma pa_header_union ingress common_t0_s2s
metadata eth_tx_t0_s2s_k eth_tx_t0_s2s;
@pragma scratch_metadata
metadata eth_tx_t0_s2s_k eth_tx_t0_s2s_scratch;

@pragma pa_header_union ingress common_t2_s2s
metadata eth_tx_t2_s2s_k eth_tx_t2_s2s;
@pragma scratch_metadata
metadata eth_tx_t2_s2s_k eth_tx_t2_s2s_scratch;


/*****************************************************************************
 * P-vector
 *****************************************************************************/

@pragma pa_align 128
@pragma dont_trim
metadata eth_tx_cq_desc_p eth_tx_cq_desc;

// Use the App Header from Flit0 for the first packet
@pragma dont_trim
@pragma pa_header_union ingress app_header
metadata p4plus_to_p4_classic_header_t eth_tx_app_hdr0;

// Additional APP Headers for other packets
@pragma dont_trim
@pragma pa_align 512
metadata p4plus_to_p4_classic_header_t eth_tx_app_hdr1;
@pragma dont_trim
metadata p4plus_to_p4_classic_header_t eth_tx_app_hdr2;
@pragma dont_trim
metadata p4plus_to_p4_classic_header_t eth_tx_app_hdr3;

// DMA headers
@pragma pa_align 512
@pragma dont_trim
metadata dma_cmd_mem2pkt_t mem2pkt;
@pragma dont_trim
metadata dma_cmd_phv2pkt_t phv2pkt;
@pragma dont_trim
metadata dma_cmd_phv2mem_t phv2mem;
@pragma dont_trim
metadata dma_cmd_pkt2mem_t pkt2mem;
@pragma dont_trim
metadata dma_cmd_mem2mem_t mem2mem;
@pragma dont_trim
@pragma pa_header_union ingress mem2pkt phv2pkt phv2mem pkt2mem mem2mem
metadata dma_cmd_generic_t dma;
