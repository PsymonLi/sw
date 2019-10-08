/*****************************************************************************/
/* Classic mode processing                                                   */
/*****************************************************************************/
action registered_macs(dst_lport, multicast_en, nic_mode) {
    modify_field(capri_intrinsic.tm_oport, TM_PORT_EGRESS);
    modify_field(qos_metadata.qos_class_id, capri_intrinsic.tm_oq);
    modify_field(control_metadata.registered_mac_nic_mode, nic_mode);

    // hit action
    if (multicast_en == TRUE) {
        modify_field(capri_intrinsic.tm_replicate_en, multicast_en);
        modify_field(capri_intrinsic.tm_replicate_ptr, dst_lport);
    } else {
        modify_field(control_metadata.dst_lport, dst_lport);
    }

    // Current assumption is for input_properties miss case we don't
    // need to honor promiscuous receivers list.
    if (flow_lkp_metadata.lkp_classic_vrf == 0) {
        modify_field(control_metadata.drop_reason, DROP_INPUT_PROPERTIES_MISS);
        drop_packet();
    }
    modify_field(control_metadata.registered_mac_miss, TRUE);

    if (flow_lkp_metadata.pkt_type == PACKET_TYPE_MULTICAST) {
        modify_field(capri_intrinsic.tm_replicate_en, TRUE);
        add(capri_intrinsic.tm_replicate_ptr,
            control_metadata.flow_miss_idx, 1);
    } else {
        if (flow_lkp_metadata.pkt_type == PACKET_TYPE_BROADCAST) {
            modify_field(capri_intrinsic.tm_replicate_en, TRUE);
            modify_field(capri_intrinsic.tm_replicate_ptr,
                         control_metadata.flow_miss_idx);
        } else {
            modify_field(capri_intrinsic.tm_replicate_en, TRUE);
            add(capri_intrinsic.tm_replicate_ptr,
                control_metadata.flow_miss_idx, 2);
        }
    }
}

@pragma stage 2
table registered_macs {
    reads {
        entry_inactive.registered_macs    : exact;
        flow_lkp_metadata.lkp_classic_vrf : exact;
        flow_lkp_metadata.lkp_dstMacAddr  : exact;
    }
    actions {
        registered_macs;
    }
    size : REGISTERED_MACS_TABLE_SIZE;
}


@pragma stage 2
@pragma overflow_table registered_macs
table registered_macs_otcam {
    reads {
        entry_inactive.registered_macs    : ternary;
        flow_lkp_metadata.lkp_classic_vrf : ternary;
        flow_lkp_metadata.lkp_dstMacAddr  : ternary;
    }
    actions {
        registered_macs;
    }
    size : REGISTERED_MACS_OTCAM_TABLE_SIZE;
}

control process_registered_macs {

    if (control_metadata.registered_mac_launch == 1) {
        apply(registered_macs);
        apply(registered_macs_otcam);
    }
}
