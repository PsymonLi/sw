header_type apulu_p4i_to_rxdma_header_t {
    fields {
        p4plus_app_id       : 4;
        lpm1_enable         : 1;
        lpm2_enable         : 1;
        vnic_info_en        : 1;
        apulu_p4plus        : 1;

        pad0                : 4;
        mapping_done        : 1;
        mapping_ohash_lkp   : 1;
        rx_packet           : 1;
        iptype              : 1;
        vnic_info_key       : 16;
        vpc_id              : 16;
        local_tag_idx       : 16;
        pad1                : 24;

        flow_src            : 128;
        flow_sport          : 16;
        flow_dport          : 16;
        flow_proto          : 8;
        flow_dst            : 128;
        service_tag         : 32;
    }
}

header_type apulu_txdma_to_p4e_header_t {
    fields {
        meter_en        : 1;
        sacl_stateful   : 1;
        sacl_root_num   : 3;
        nexthop_type    : 2;
        sacl_drop       : 1;
        nexthop_id      : 16;
        snat_type       : 2;
        dnat_en         : 1;
        dnat_idx        : 13;
        src_bd_id       : 16;
        sacl_alg_type   : 4;
        pad             : 3;
        src_mapping_hit : 1;
        route_priority  : 16;
    }
}

header_type apulu_p4_to_arm_header_t {
    fields {
        packet_len              : 16;
        nacl_data               : 8;
        flags                   : 8;
        ingress_bd_id           : 16;
        flow_hash               : 32;

        l2_1_offset             : 8;
        l3_1_offset             : 8;
        l4_1_offset             : 8;
        l2_2_offset             : 8;
        l3_2_offset             : 8;
        l4_2_offset             : 8;
        payload_offset          : 8;
        tcp_flags               : 8;

        session_id              : 32;
        lif                     : 16;
        egress_bd_id            : 16;
        service_xlate_id        : 32;
        mapping_xlate_id        : 32;
        tx_meter_id             : 16;
        nexthop_id              : 16;
        vpc_id                  : 16;
        vnic_id                 : 16;
        dnat_id                 : 16;

        rx_packet               : 1;
        flow_hit                : 1;
        flow_role               : 1;
        is_local                : 1;
        is_l3_vnid              : 1;
        snat_type               : 2;
        dnat_en                 : 1;

        mapping_hit             : 1;
        sacl_stateful           : 1;
        sacl_root_num           : 3;
        nexthop_type            : 2;
        sacl_drop               : 1;

        defunct_flow            : 1;
        local_mapping_ip_type   : 2;
        src_mapping_hit         : 1;
        sacl_alg_type           : 4;

        meter_en                : 1;
        pad                     : 7;

        epoch                   : 8;
        sw_meta                 : 32;

        src_bd_id               : 16;
        route_priority          : 16;
    }
}

header_type apulu_arm_to_p4_header_t {
    fields {
        pad                     : 1;
        skip_stats_update       : 1;
        flow_lkp_id_override    : 1;
        learning_done           : 1;
        nexthop_valid           : 1;
        lif                     : 11;
        nexthop_type            : 8;
        nexthop_id              : 16;
        flow_lkp_id             : 16;
        sw_meta                 : 32;
    }
}

header_type apulu_ingress_recirc_header_t {
    fields {
        pad                 : 7;
        defunct_flow        : 1;
        flow_ohash          : 32;
        local_mapping_ohash : 32;
    }
}

header_type apulu_egress_recirc_header_t {
    fields {
        mapping_ohash   : 32;
        pad             : 6;
        p4_to_arm_valid : 1;
        mapping_done    : 1;
    }
}

header_type apulu_rx_to_tx_header_t {
    fields {
        remote_ip       : 128;
        route_base_addr : 40;

        sacl_base_addr0 : 40;
        sacl_base_addr1 : 40;
        sacl_base_addr2 : 40;
        sacl_base_addr3 : 40;
        sacl_base_addr4 : 40;
        sacl_base_addr5 : 40;

        src_bd_id       : 16;
        pad1            : 7;
        src_mapping_hit : 1;

        sip_classid0    : 10;
        dip_classid0    : 10;
        pad_classid0    : 4;
        sport_classid0  : 8;
        dport_classid0  : 8;

        sip_classid1    : 10;
        dip_classid1    : 10;
        pad_classid1    : 4;
        sport_classid1  : 8;
        dport_classid1  : 8;

        /*--------------512b ---------------*/

        sip_classid2    : 10;
        dip_classid2    : 10;
        pad_classid2    : 4;
        sport_classid2  : 8;
        dport_classid2  : 8;

        sip_classid3    : 10;
        dip_classid3    : 10;
        pad_classid3    : 4;
        sport_classid3  : 8;
        dport_classid3  : 8;

        sip_classid4    : 10;
        dip_classid4    : 10;
        pad_classid4    : 4;
        sport_classid4  : 8;
        dport_classid4  : 8;

        sip_classid5    : 10;
        dip_classid5    : 10;
        pad_classid5    : 4;
        sport_classid5  : 8;
        dport_classid5  : 8;

        vpc_id          : 16;
        vnic_id         : 16;

        iptype          : 1;
        rx_packet       : 1;
        payload_len     : 14;

        stag0_classid0  : 10;
        stag1_classid0  : 10;
        stag2_classid0  : 10;
        stag3_classid0  : 10;
        stag4_classid0  : 10;
        dtag0_classid0  : 10;
        dtag1_classid0  : 10;
        dtag2_classid0  : 10;
        dtag3_classid0  : 10;
        dtag4_classid0  : 10;

        stag0_classid1  : 10;
        stag1_classid1  : 10;
        stag2_classid1  : 10;
        stag3_classid1  : 10;
        stag4_classid1  : 10;
        dtag0_classid1  : 10;
        dtag1_classid1  : 10;
        dtag2_classid1  : 10;
        dtag3_classid1  : 10;
        dtag4_classid1  : 10;

        stag0_classid2  : 10;
        stag1_classid2  : 10;
        stag2_classid2  : 10;
        stag3_classid2  : 10;
        stag4_classid2  : 10;
        dtag0_classid2  : 10;
        dtag1_classid2  : 10;
        dtag2_classid2  : 10;
        dtag3_classid2  : 10;
        dtag4_classid2  : 10;

        pad2            : 4;

        /*--------------512b ---------------*/

        stag0_classid3  : 10;
        stag1_classid3  : 10;
        stag2_classid3  : 10;
        stag3_classid3  : 10;
        stag4_classid3  : 10;
        dtag0_classid3  : 10;
        dtag1_classid3  : 10;
        dtag2_classid3  : 10;
        dtag3_classid3  : 10;
        dtag4_classid3  : 10;

        stag0_classid4  : 10;
        stag1_classid4  : 10;
        stag2_classid4  : 10;
        stag3_classid4  : 10;
        stag4_classid4  : 10;
        dtag0_classid4  : 10;
        dtag1_classid4  : 10;
        dtag2_classid4  : 10;
        dtag3_classid4  : 10;
        dtag4_classid4  : 10;

        stag0_classid5  : 10;
        stag1_classid5  : 10;
        stag2_classid5  : 10;
        stag3_classid5  : 10;
        stag4_classid5  : 10;
        dtag0_classid5  : 10;
        dtag1_classid5  : 10;
        dtag2_classid5  : 10;
        dtag3_classid5  : 10;
        dtag4_classid5  : 10;

        pad_end         : 4;
    }
}
