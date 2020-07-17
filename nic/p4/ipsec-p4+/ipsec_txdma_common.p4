
// p4plus_to_p4_header_t + p4plus_to_p4_header_ext_t
header_type ipsec_p4plus_to_p4_header_t {
    fields {
        p4plus_app_id           : 4;
        table0_valid            : 1;
        table1_valid            : 1;
        table2_valid            : 1;
        table3_valid            : 1;
        flow_index              : 24;
        flags                   : 8;
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
        pad                     : 7;
        nexthop_valid           : 1;
        nexthop_type            : 8;
        nexthop_id              : 16;
        nexthop_pad             : 8;
    }
}
