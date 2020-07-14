/******************************************************************************/
/* LIF info                                                                   */
/******************************************************************************/
action lif_info(direction, lif_type, vnic_id, bd_id, vpc_id, learn_enabled,
                pinned_nexthop_type, pinned_nexthop_id, lif_vlan_mode) {
    modify_field(control_metadata.rx_packet, direction);
    modify_field(p4i_i2e.rx_packet, direction);
    modify_field(control_metadata.lif_type, lif_type);
    modify_field(vnic_metadata.vnic_id, vnic_id);
    modify_field(vnic_metadata.bd_id, bd_id);
    modify_field(vnic_metadata.vpc_id, vpc_id);
    if (arm_to_p4i.flow_lkp_id_override == FALSE) {
        modify_field(key_metadata.flow_lkp_id, bd_id);
    } else {
        modify_field(key_metadata.flow_lkp_id, arm_to_p4i.flow_lkp_id);
    }
    modify_field(p4i_i2e.skip_stats_update, arm_to_p4i.skip_stats_update);
    modify_field(p4i_to_arm.sw_meta, arm_to_p4i.sw_meta);
    modify_field(p4i_i2e.nexthop_type, pinned_nexthop_type);
    modify_field(p4i_i2e.nexthop_id, pinned_nexthop_id);
    modify_field(control_metadata.learn_enabled, learn_enabled);
    modify_field(scratch_metadata.lif_vlan_mode, lif_vlan_mode);
    if (scratch_metadata.lif_vlan_mode == APULU_LIF_VLAN_MODE_LIF_VLAN) {
        // ASM only
        // modify_field(key_metadata.lif, capri_intrinsic.lif);
        // modify_field(key_metadata.lif, arm_to_p4i.lif);
    } else {
        if (scratch_metadata.lif_vlan_mode == APULU_LIF_VLAN_MODE_QINQ) {
            modify_field(key_metadata.lif, ctag2_1.vid);
        } else {
            modify_field(key_metadata.lif, 0);
        }
    }
    // ASM only
    // modify_field(p4i_i2e.src_lif, capri_intrinsic.lif);
    // modify_field(p4i_i2e.src_lif, arm_to_p4i.lif);
}

@pragma stage 0
table lif {
    reads {
        capri_intrinsic.lif : exact;
    }
    actions {
        lif_info;
    }
    size : LIF_TABLE_SIZE;
}

@pragma stage 0
table lif2 {
    reads {
        arm_to_p4i.lif  : exact;
    }
    actions {
        lif_info;
    }
    size : LIF_TABLE_SIZE;
}

/******************************************************************************/
/* LIF/VLAN info                                                              */
/******************************************************************************/
action lif_vlan_info(vnic_id, bd_id, vpc_id) {
    //if (table_hit) {
        if (vnic_id != 0) {
            modify_field(vnic_metadata.vnic_id, vnic_id);
        }
        if (bd_id != 0) {
            modify_field(vnic_metadata.bd_id, bd_id);
            if (arm_to_p4i.flow_lkp_id_override == FALSE) {
                modify_field(key_metadata.flow_lkp_id, bd_id);
            }
        }
        if (vpc_id != 0) {
            modify_field(vnic_metadata.vpc_id, vpc_id);
        }
    //}

    // keys for local mapping lookup
    if (control_metadata.rx_packet == FALSE) {
        if ((ipv4_1.valid == TRUE) and (ipv4_1.srcAddr != 0)) {
            modify_field(key_metadata.local_mapping_lkp_type, KEY_TYPE_IPV4);
            modify_field(key_metadata.local_mapping_lkp_addr, ipv4_1.srcAddr);
            modify_field(key_metadata.local_mapping_lkp_id,
                         vnic_metadata.vpc_id);
        } else {
            if ((arp.valid == TRUE) and (arp.senderIpAddr != 0)) {
                modify_field(key_metadata.local_mapping_lkp_id,
                             vnic_metadata.vpc_id);
                modify_field(key_metadata.local_mapping_lkp_type,
                             KEY_TYPE_IPV4);
                modify_field(key_metadata.local_mapping_lkp_addr,
                             arp.senderIpAddr);
            } else {
                modify_field(key_metadata.local_mapping_lkp_id,
                             vnic_metadata.bd_id);
                modify_field(key_metadata.local_mapping_lkp_type,
                             KEY_TYPE_MAC);
                modify_field(key_metadata.local_mapping_lkp_addr,
                             ethernet_1.srcAddr);
            }
        }
    } else {
        if (ipv4_1.valid == TRUE) {
            modify_field(key_metadata.local_mapping_lkp_type, KEY_TYPE_IPV4);
            modify_field(key_metadata.local_mapping_lkp_addr, ipv4_1.dstAddr);
            modify_field(key_metadata.local_mapping_lkp_id,
                         vnic_metadata.vpc_id);
        } else {
            modify_field(key_metadata.local_mapping_lkp_type, KEY_TYPE_MAC);
            modify_field(key_metadata.local_mapping_lkp_addr,
                         ethernet_1.dstAddr);
            modify_field(key_metadata.local_mapping_lkp_id,
                         vnic_metadata.bd_id);
        }
    }

    // keys for mapping lookup
    modify_field(p4i_i2e.mapping_lkp_type, KEY_TYPE_MAC);
    modify_field(p4i_i2e.mapping_lkp_type, ethernet_1.dstAddr);
    modify_field(p4i_i2e.mapping_lkp_id, vnic_metadata.bd_id);
}

@pragma stage 1
table lif_vlan {
    reads {
        key_metadata.lif    : exact;
        ctag_1.vid          : exact;
    }
    actions {
        lif_vlan_info;
    }
    size : LIF_VLAN_HASH_TABLE_SIZE;
}

@pragma stage 1
@pragma overflow_table lif_vlan
table lif_vlan_otcam {
    reads {
        key_metadata.lif    : ternary;
        ctag_1.vid          : ternary;
    }
    actions {
        lif_vlan_info;
    }
    size : LIF_VLAN_OTCAM_TABLE_SIZE;
}

/******************************************************************************/
/* VNI info                                                                   */
/******************************************************************************/
action vni_info(vnic_id, bd_id, vpc_id, is_l3_vnid) {
    modify_field(control_metadata.tunneled_packet, TRUE);
    // on miss, drop the packet

    // on hit
    if (vnic_id != 0) {
        modify_field(vnic_metadata.vnic_id, vnic_id);
    }
    if (bd_id != 0) {
        modify_field(vnic_metadata.bd_id, bd_id);
        if (arm_to_p4i.flow_lkp_id_override == FALSE) {
            modify_field(key_metadata.flow_lkp_id, bd_id);
        }
    }
    if (vpc_id != 0) {
        modify_field(vnic_metadata.vpc_id, vpc_id);
    }
    modify_field(control_metadata.tunnel_terminate, TRUE);
    modify_field(p4i_to_arm.is_l3_vnid, is_l3_vnid);

    // keys for local mapping lookup
    if (ipv4_2.valid == TRUE) {
        modify_field(key_metadata.local_mapping_lkp_type, KEY_TYPE_IPV4);
        modify_field(key_metadata.local_mapping_lkp_type, ipv4_2.dstAddr);
        modify_field(key_metadata.local_mapping_lkp_id, vnic_metadata.vpc_id);
    } else {
        modify_field(key_metadata.local_mapping_lkp_type, KEY_TYPE_MAC);
        modify_field(key_metadata.local_mapping_lkp_type, ethernet_2.dstAddr);
        modify_field(key_metadata.local_mapping_lkp_id, vnic_metadata.bd_id);
    }

    // keys for mapping lookup
    modify_field(p4i_i2e.mapping_lkp_type, KEY_TYPE_MAC);
    modify_field(p4i_i2e.mapping_lkp_type, ethernet_2.dstAddr);
    modify_field(p4i_i2e.mapping_lkp_id, vnic_metadata.bd_id);
}

@pragma stage 1
table vni {
    reads {
        vxlan_1.vni : exact;
    }
    actions {
        vni_info;
    }
    size : VNI_HASH_TABLE_SIZE;
}

@pragma stage 1
@pragma overflow_table vni
table vni_otcam {
    reads {
        vxlan_1.vni : ternary;
    }
    actions {
        vni_info;
    }
    size : VNI_OTCAM_TABLE_SIZE;
}

/******************************************************************************/
/* VNIC info (Ingress)                                                        */
/******************************************************************************/
action vnic_info(epoch, meter_enabled, rx_mirror_session, tx_mirror_session,
                 tx_policer_id, binding_check_enabled) {
    modify_field(p4i_to_arm.epoch, epoch);
    if ((control_metadata.flow_done == TRUE) and
        (control_metadata.flow_miss == FALSE) and
        (control_metadata.flow_epoch != epoch)) {
        modify_field(control_metadata.flow_done, FALSE);
        modify_field(ingress_recirc.defunct_flow, TRUE);
        // return;
    }
    modify_field(control_metadata.binding_check_enabled, binding_check_enabled);
    modify_field(p4i_i2e.meter_enabled, meter_enabled);
    if (control_metadata.rx_packet == TRUE) {
        modify_field(p4i_i2e.mirror_session, rx_mirror_session);
    } else {
        modify_field(p4i_i2e.tx_policer_id, tx_policer_id);
        modify_field(p4i_i2e.mirror_session, tx_mirror_session);
    }
}

@pragma stage 4
@pragma index_table
table vnic {
    reads {
        vnic_metadata.vnic_id   : exact;
    }
    actions {
        vnic_info;
    }
    size : VNIC_TABLE_SIZE;
}

control input_properties {
    if (arm_to_p4i.valid == TRUE) {
        apply(lif2);
    } else {
        apply(lif);
    }
    if ((control_metadata.rx_packet == TRUE) and
        (control_metadata.to_device_ip == TRUE) and
        (vxlan_1.valid == TRUE)) {
        apply(key_tunneled);
        apply(vni);
        apply(vni_otcam);
        apply(service_mapping);
        apply(service_mapping_otcam);
    } else {
        apply(lif_vlan);
        apply(lif_vlan_otcam);
    }
    apply(vnic);
    apply(p4i_bd);
}

/******************************************************************************/
/* VPC info                                                                   */
/******************************************************************************/
action vpc_info(vni, vrmac, tos) {
    modify_field(rewrite_metadata.vni, vni);
    modify_field(rewrite_metadata.vrmac, vrmac);
    modify_field(rewrite_metadata.tunnel_tos, tos);

    // TTL decrement
    if (P4_REWRITE(rewrite_metadata.flags, TTL, DEC)) {
        if (ipv4_1.valid == TRUE) {
            add(ipv4_1.ttl, ipv4_1.ttl, -1);
            modify_field(control_metadata.update_checksum, TRUE);
        } else {
            if (ipv6_1.valid == TRUE) {
                add(ipv6_1.hopLimit, ipv6_1.hopLimit, -1);
            }
        }
    }
}

@pragma stage 2
@pragma index_table
table vpc {
    reads {
        p4e_i2e.vpc_id  : exact;
    }
    actions {
        vpc_info;
    }
    size : VPC_TABLE_SIZE;
}

/******************************************************************************/
/* BD info                                                                    */
/******************************************************************************/
action ingress_bd_info(vrmac) {
    modify_field(scratch_metadata.mac, vrmac);
    if (control_metadata.tunnel_terminate == TRUE) {
        if (ipv4_2.valid == TRUE) {
            if ((control_metadata.l2_enabled == FALSE) or
                (scratch_metadata.mac == ethernet_2.dstAddr)) {
                modify_field(p4i_i2e.mapping_lkp_type, KEY_TYPE_IPV4);
                modify_field(p4i_i2e.mapping_lkp_addr, ipv4_2.dstAddr);
                modify_field(p4i_i2e.mapping_lkp_id, vnic_metadata.vpc_id);
            }
        }
    } else {
        if (ipv4_1.valid == TRUE) {
            if ((control_metadata.l2_enabled == FALSE) or
                (scratch_metadata.mac == ethernet_1.dstAddr)) {
                modify_field(p4i_i2e.mapping_lkp_type, KEY_TYPE_IPV4);
                modify_field(p4i_i2e.mapping_lkp_addr, ipv4_1.dstAddr);
                modify_field(p4i_i2e.mapping_lkp_id, vnic_metadata.vpc_id);
            }
        }
    }
}

@pragma stage 4
@pragma index_table
table p4i_bd {
    reads {
        vnic_metadata.bd_id : exact;
    }
    actions {
        ingress_bd_info;
    }
    size : BD_TABLE_SIZE;
}

action egress_bd_info(vni, vrmac, tos) {
    if (p4e_i2e.binding_check_drop == TRUE) {
        egress_drop(P4E_DROP_MAC_IP_BINDING_FAIL);
    }

    if (vnic_metadata.egress_bd_id == 0) {
        // return;
    }

    modify_field(rewrite_metadata.vni, vni);
    modify_field(rewrite_metadata.vrmac, vrmac);
    modify_field(rewrite_metadata.tunnel_tos, tos);
}

@pragma stage 3
@pragma index_table
table p4e_bd {
    reads {
        vnic_metadata.egress_bd_id  : exact;
    }
    actions {
        egress_bd_info;
    }
    size : BD_TABLE_SIZE;
}

/******************************************************************************/
/* Rx VNIC info (Egress)                                                      */
/******************************************************************************/
action rx_vnic_info(rx_policer_id) {
    modify_field(vnic_metadata.rx_policer_id, rx_policer_id);
}

@pragma stage 3
@pragma index_table
table rx_vnic {
    reads {
        vnic_metadata.rx_vnic_id    : exact;
    }
    actions {
        rx_vnic_info;
    }
    size : VNIC_TABLE_SIZE;
}

control output_properties {
    apply(vpc);
    apply(p4e_bd);
    apply(rx_vnic);
}
