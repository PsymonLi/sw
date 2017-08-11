header_type cap_phv_intr_global_t {
    fields {
        tm_iport           : 4;
        tm_oport           : 4;
        tm_iq              : 5;
        lif                : 11;
        timestamp          : 48;
        tm_span_session    : 8;
        error_bits         : 4;
        tm_instance_type   : 4;
        tm_replicate_ptr   : 16;
        bypass             : 1;
        tm_replicate_en    : 1;
        tm_q_depth         : 14;
        drop               : 1;
        hw_error           : 1;
        tm_cpu             : 1;
        tm_oq              : 5;
        debug_trace        : 1;
        glb_rsv            : 7;
    }
}

header_type cap_phv_intr_p4_t {
    fields {
        no_data            : 1;
        recirc             : 1;
        frame_size         : 14;
        packet_len         : 14;
        recirc_count       : 3;
        p4_pad             : 7;
    }
}

header_type cap_phv_intr_txdma_t {
    fields {
        qid                : 24;
        dma_cmd_ptr        : 6;
        qstate_addr        : 34;
        qtype              : 3;
        txdma_rsv          : 5;
    }
}

header_type cap_phv_intr_rxdma_t {
    fields {
        qid                : 24;
        dma_cmd_ptr        : 6;
        qstate_addr        : 34;
        qtype              : 3;
        rx_splitter_offset : 10;
        rxdma_rsv          : 3;
    }
}

header_type p4_to_p4plus_ipsec_header_t {
    fields {
        p4plus_app_id       : 4;
        table0_valid        : 1;
        table1_valid        : 1;
        table2_valid        : 1;
        table3_valid        : 1;
        ipsec_payload_start : 16;
        ipsec_payload_end   : 16;
        l4_protocol         : 8;
        ip_hdr_size         : 8;
        seqNo               : 32;
    }
}

header_type p4_to_p4plus_roce_header_t {
    fields {
        p4plus_app_id        : 4;
        flags                : 4;  // ecn : 2, cnp : 1, v6 : 0
        rdma_hdr_len         : 8;  // copied directly from p4 rdma table
        opcode               : 8;  // from rdma.bth
        raw_flags            : 16; // copied directly from p4 rdma table
        mac_da_inner         : 48; // from inner l2-hdr
        mac_sa_inner         : 48; // from inner l2-hdr
        payload_len          : 16;
    }
}

header_type p4_to_p4plus_tcp_proxy_base_header_t {
    fields {
        p4plus_app_id       : 4;
        num_sack_blocks     : 4;
        payload_len         : 16;
        srcPort             : 16;
        dstPort             : 16;
        seqNo               : 32;
        ackNo               : 32;
        dataOffset          : 4;
        res                 : 4;
        flags               : 8;
        window              : 16;
        urgentPtr           : 16;
        ts                  : 32;
        prev_echo_ts        : 32;
    }
}

header_type p4_to_p4plus_tcp_proxy_sack_header_t {
    fields {
        start_seq0       : 32;
        end_seq0         : 32;
        start_seq1       : 32;
        end_seq1         : 32;
        start_seq2       : 32;
        end_seq2         : 32;
        start_seq3       : 32;
        end_seq3         : 32;
    }
}

/*
 * flag bits:
 *  0 : forwarding and rewrite complete, just enqueue the packet
 *  1 : perform forwarding lookup, no rewrites
 *  2 : perform forwarding and rewrites
 *  3 : update stats
 */
header_type cpu_to_p4_header_t {
    fields {
        p4plus_app_id    : 4;
        flags            : 4;
        src_lif          : 11;
        dst_lif          : 11;
        iclass           : 4;
        oclass           : 4;
        dest_type        : 2;
        dest             : 16;
        oqueue           : 5;
    }
}

/*
 * flags bits:
 *  0 : fcs ok
 *  1 : tunnel terminate
 *  2 : ip options are present
 *  3 : tcp options are present
 *
 * flags_outer bits:
 *  0     : vlan valid
 *  1     : ipv4 valid
 *  2     : ipv6 valid
 *  3     : checksum verified by hardware
 *  4     : l3 checksum ok
 *  5     : l4 checksum ok
 *  [8-6] : ipv4 flags bits
 *
 * flags_inner bits:
 *  0     : vlan valid
 *  1     : ipv4 valid
 *  2     : ipv6 valid
 *  3     : checksum verified by hardware
 *  4     : l3 checksum ok
 *  5     : l4 checksum ok
 *  [8-6] : ipv4 flags bits
 *  9     : vlan or vni
 */
header_type p4_to_p4plus_header_t {
    fields {
        p4plus_app_id    : 4;
        src_iport        : 4;
        reason           : 8;
        packet_len       : 16;
        pad              : 5;
        src_lif          : 11;

        flags            : 4;
        lkp_type         : 4;
        lkp_vrf          : 16;

        // outer
        flags_outer      : 16;
        mac_sa_outer     : 48;
        mac_da_outer     : 48;
        vlan_pcp_outer   : 3;
        vlan_dei_outer   : 1;
        vlan_id_outer    : 12;
        ip_sa_outer      : 128;
        ip_da_outer      : 128;
        ip_proto_outer   : 8;
        ip_ttl_outer     : 8;
        l4_sport_outer   : 16;
        l4_dport_outer   : 16;

        // inner
        flags_inner      : 16;
        mac_sa_inner     : 48;
        mac_da_inner     : 48;
        vlan_pcp_inner   : 3;
        vlan_dei_inner   : 1;
        vlan_id_inner    : 28;
        ip_sa_inner      : 128;
        ip_da_inner      : 128;
        ip_proto_inner   : 8;
        ip_ttl_inner     : 8;
        l4_sport_inner   : 16;
        l4_dport_inner   : 16;

        // tcp
        tcp_flags        : 8;
        tcp_seqNo        : 32;
        tcp_AckNo        : 32;
        tcp_window       : 16;
        tcp_options      : 8;
        tcp_mss          : 16;
        tcp_ws           : 8;
    }
}

/*
 * flag bits:
 *  0 : update ip id
 *  1 : update ip len
 *  2 : update tcp seq number
 *  3 : update udp len
 *  4 : insert vlan header
 *  5 : computer outer checksums
 *  6 : computer inner checksums
 */
header_type p4plus_to_p4_header_t {
    fields {
        p4plus_app_id    : 4;
        flags            : 8;
        ip_id            : 16;
        ip_len           : 16;
        udp_len          : 16;
        tcp_seq_no       : 32;
        vlan_pcp         : 3;
        vlan_dei         : 1;
        vlan_id          : 12;
    }
}

header_type tm_replication_data_t {
    fields {
        qtype                  : 3;
        lif                    : 11;
        tunnel_rewrite_index   : 10;
        qid_or_vnid            : 24;
        pad_1                  : 4;
        rewrite_index          : 12;
    }
}
