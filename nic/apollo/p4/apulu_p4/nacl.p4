/*****************************************************************************/
/* Network ACL                                                               */
/*****************************************************************************/
action nacl_permit() {
}

action nacl_drop() {
    ingress_drop(P4I_DROP_NACL);
}

action nacl_redirect(nexthop_type, nexthop_id) {
    modify_field(p4i_i2e.mapping_bypass, TRUE);
    modify_field(p4i_i2e.nexthop_type, nexthop_type);
    modify_field(p4i_i2e.nexthop_id, nexthop_id);
    modify_field(control_metadata.flow_miss, FALSE);

    modify_field(capri_intrinsic.drop, 0);
    modify_field(control_metadata.p4i_drop_reason, 0);
}

@pragma stage 4
table nacl {
    reads {
        vnic_metadata.vnic_id               : ternary;
        key_metadata.ktype                  : ternary;
        key_metadata.src                    : ternary;
        key_metadata.dst                    : ternary;
        key_metadata.proto                  : ternary;
        key_metadata.sport                  : ternary;
        key_metadata.dport                  : ternary;
        ethernet_1.dstAddr                  : ternary;
        capri_intrinsic.lif                 : ternary;
        control_metadata.rx_packet          : ternary;
        control_metadata.tunneled_packet    : ternary;
        control_metadata.flow_miss          : ternary;
    }
    actions {
        nacl_permit;
        nacl_redirect;
        nacl_drop;
    }
    size : NACL_TABLE_SIZE;
}

control nacl {
    apply(nacl);
}
