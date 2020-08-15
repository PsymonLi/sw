header_type apulu_i2e_metadata_t {
    fields {
        mapping_lkp_addr    : 128;
        entropy_hash        : 16;
        mapping_lkp_type    : 2;
        flow_role           : 1;
        session_id          : 21;
        vnic_id             : 16;
        bd_id               : 16;
        vpc_id              : 16;
        mapping_lkp_id      : 16;
        nexthop_id          : 16;
        meter_enabled       : 1;
        rx_packet           : 1;
        nexthop_type        : 2;
        mapping_bypass      : 1;
        update_checksum     : 1;
        copp_policer_id     : 10;
        priority            : 5;
        src_lif             : 11;
        mirror_session      : 8;
        pad0                : 1;
        device_mode         : 2;
        is_local_to_local   : 1;
        binding_check_drop  : 1;
        skip_stats_update   : 1;
        tx_policer_id       : 10;
        copp_class          : 4;
        xlate_id            : 20;
        pad1                : 3;
        dst_tm_oq           : 5;
        rewrite_flags       : 16;
    }
}
