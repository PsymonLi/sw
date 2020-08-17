/*****************************************************************************/
/* Policy (IPv6 and non-IP)                                                  */
/*****************************************************************************/
@pragma capi appdatafields epoch session_index flow_role nexthop_valid nexthop_type nexthop_id priority is_local_to_local
@pragma capi hwfields_access_api
action flow_hash(epoch, session_index,
                 hash1, hint1, hash2, hint2, hash3, hint3, hash4, hint4,
                 more_hashes, more_hints, force_flow_miss, flow_role,
                 nexthop_valid, nexthop_type, nexthop_id, entry_valid,
                 priority, is_local_to_local) {
    modify_field(p4i_i2e.entropy_hash, scratch_metadata.flow_hash);
    modify_field(p4i_to_arm.flow_hash, scratch_metadata.flow_hash);
    if (entry_valid == TRUE) {
        // if hardware register indicates hit, take the results
        modify_field(control_metadata.flow_epoch, epoch);
        modify_field(scratch_metadata.session_id, session_index);
        modify_field(p4i_i2e.session_id, scratch_metadata.session_id);
        modify_field(scratch_metadata.flag, force_flow_miss);
        // entry is old or force_flow_miss is true
        if ((ingress_recirc.defunct_flow == TRUE) or
            (force_flow_miss == TRUE)) {
            modify_field(control_metadata.flow_miss, TRUE);
            modify_field(control_metadata.flow_done, TRUE);
            modify_field(p4i_to_arm.flow_hit, TRUE);
            modify_field(p4i_to_arm.flow_role, flow_role);
            modify_field(p4i_to_arm.session_id, scratch_metadata.session_id);
            modify_field(p4i_to_arm.defunct_flow, ingress_recirc.defunct_flow);
        } else {
            modify_field(p4i_i2e.is_local_to_local, is_local_to_local);
            modify_field(p4i_i2e.flow_role, flow_role);
            modify_field(control_metadata.flow_info_lkp, TRUE);
            if (flow_role == TCP_FLOW_INITIATOR) {
                modify_field(key_metadata.flow_info_id,
                             scratch_metadata.session_id);
            } else {
                modify_field(key_metadata.flow_info_id,
                             scratch_metadata.session_id +
                             (FLOW_INFO_TABLE_SIZE / 2));
            }
            modify_field(control_metadata.flow_done, TRUE);
            modify_field(scratch_metadata.flag, nexthop_valid);
            if (nexthop_valid == TRUE) {
                modify_field(p4i_i2e.nexthop_type, nexthop_type);
                if (nexthop_type == NEXTHOP_TYPE_VPC) {
                    modify_field(p4i_i2e.mapping_lkp_id, nexthop_id);
                } else {
                    modify_field(p4i_i2e.priority, priority);
                    modify_field(p4i_i2e.nexthop_id, nexthop_id);
                }
            }
            if ((arm_to_p4i.valid == FALSE) and
                (tcp.flags & (TCP_FLAG_FIN|TCP_FLAG_RST) != 0)) {
                modify_field(control_metadata.flow_miss, TRUE);
                modify_field(control_metadata.force_flow_miss, TRUE);
                modify_field(p4i_to_arm.flow_hit, TRUE);
                modify_field(p4i_to_arm.flow_role, flow_role);
                modify_field(p4i_to_arm.session_id,
                             scratch_metadata.session_id);
                if (nexthop_valid == TRUE) {
                    modify_field(p4i_to_arm.nexthop_type, nexthop_type);
                    if (nexthop_type == NEXTHOP_TYPE_VPC) {
                        modify_field(p4i_i2e.mapping_lkp_id, nexthop_id);
                    } else {
                        modify_field(p4i_i2e.mapping_bypass, TRUE);
                        modify_field(p4i_to_arm.nexthop_id, nexthop_id);
                    }
                }
            }
        }

        // if hardware register indicates miss, compare hashes with r1
        // (scratch_metadata.flow_hash) and setup lookup in overflow table
        modify_field(scratch_metadata.flow_hash, scratch_metadata.flow_hash);
        modify_field(scratch_metadata.hint_valid, FALSE);
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.flow_hash == hash1)) {
            modify_field(scratch_metadata.flow_hint, hint1);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.flow_hash == hash2)) {
            modify_field(scratch_metadata.flow_hint, hint2);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.flow_hash == hash3)) {
            modify_field(scratch_metadata.flow_hint, hint3);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.flow_hash == hash4)) {
            modify_field(scratch_metadata.flow_hint, hint4);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        modify_field(scratch_metadata.flag, more_hashes);
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.flag == TRUE)) {
            modify_field(scratch_metadata.flow_hint, more_hints);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }

        if (scratch_metadata.hint_valid == TRUE) {
            modify_field(control_metadata.flow_ohash_lkp, TRUE);
            modify_field(ingress_recirc.flow_ohash, scratch_metadata.flow_hint);
        } else {
            modify_field(control_metadata.flow_done, TRUE);
            modify_field(control_metadata.flow_miss, TRUE);
        }
    } else {
        modify_field(p4i_i2e.session_id, -1);
        modify_field(control_metadata.flow_done, TRUE);
        modify_field(control_metadata.flow_miss, TRUE);
    }

    modify_field(scratch_metadata.flag, entry_valid);
    modify_field(scratch_metadata.flow_hash, hash1);
    modify_field(scratch_metadata.flow_hash, hash2);
    modify_field(scratch_metadata.flow_hash, hash3);
    modify_field(scratch_metadata.flow_hash, hash4);
}

@pragma stage 2
@pragma hbm_table
@pragma capi_bitfields_struct
table flow {
    reads {
        key_metadata.flow_lkp_id    : exact;
        key_metadata.ktype          : exact;
        key_metadata.src            : exact;
        key_metadata.dst            : exact;
        key_metadata.proto          : exact;
        key_metadata.sport          : exact;
        key_metadata.dport          : exact;
    }
    actions {
        flow_hash;
    }
    size : FLOW_TABLE_SIZE;
}

@pragma stage 3
@pragma hbm_table
@pragma overflow_table flow
table flow_ohash {
    reads {
        ingress_recirc.flow_ohash   : exact;
    }
    actions {
        flow_hash;
    }
    size : FLOW_OHASH_TABLE_SIZE;
}

/*****************************************************************************/
/* Policy (IPv4)                                                             */
/*****************************************************************************/
@pragma capi appdatafields epoch session_index flow_role nexthop_valid nexthop_type nexthop_id priority is_local_to_local
@pragma capi hwfields_access_api
action ipv4_flow_hash(epoch, session_index, nexthop_type,
                      hash1, hint1, hash2, hint2, more_hashes, more_hints,
                      force_flow_miss, flow_role, nexthop_valid, nexthop_id,
                      entry_valid, priority, is_local_to_local) {
    modify_field(p4i_i2e.entropy_hash, scratch_metadata.flow_hash);
    modify_field(p4i_to_arm.flow_hash, scratch_metadata.flow_hash);
    if (entry_valid == TRUE) {
        // if hardware register indicates hit, take the results
        modify_field(control_metadata.flow_epoch, epoch);
        modify_field(scratch_metadata.session_id, session_index);
        modify_field(p4i_i2e.session_id, scratch_metadata.session_id);
        modify_field(scratch_metadata.flag, force_flow_miss);
        // entry is old or force_flow_miss is true
        if ((ingress_recirc.defunct_flow == TRUE) or
            (force_flow_miss == TRUE)) {
            modify_field(control_metadata.flow_miss, TRUE);
            modify_field(control_metadata.flow_done, TRUE);
            modify_field(p4i_to_arm.flow_hit, TRUE);
            modify_field(p4i_to_arm.flow_role, flow_role);
            modify_field(p4i_to_arm.session_id, scratch_metadata.session_id);
            modify_field(p4i_to_arm.defunct_flow, ingress_recirc.defunct_flow);
        } else {
            modify_field(p4i_i2e.is_local_to_local, is_local_to_local);
            modify_field(p4i_i2e.flow_role, flow_role);
            modify_field(control_metadata.flow_info_lkp, TRUE);
            if (flow_role == TCP_FLOW_INITIATOR) {
                modify_field(key_metadata.flow_info_id,
                             scratch_metadata.session_id);
            } else {
                modify_field(key_metadata.flow_info_id,
                             scratch_metadata.session_id +
                             (FLOW_INFO_TABLE_SIZE / 2));
            }
            modify_field(control_metadata.flow_done, TRUE);
            modify_field(scratch_metadata.flag, nexthop_valid);
            if (nexthop_valid == TRUE) {
                modify_field(p4i_i2e.nexthop_type, nexthop_type);
                if (nexthop_type == NEXTHOP_TYPE_VPC) {
                    modify_field(p4i_i2e.mapping_lkp_id, nexthop_id);
                } else {
                    modify_field(p4i_i2e.priority, priority);
                    modify_field(p4i_i2e.nexthop_id, nexthop_id);
                }
            }
            if ((arm_to_p4i.valid == FALSE) and
                (tcp.flags & (TCP_FLAG_FIN|TCP_FLAG_RST) != 0)) {
                modify_field(control_metadata.flow_miss, TRUE);
                modify_field(control_metadata.force_flow_miss, TRUE);
                modify_field(p4i_to_arm.flow_hit, TRUE);
                modify_field(p4i_to_arm.flow_role, flow_role);
                modify_field(p4i_to_arm.session_id,
                             scratch_metadata.session_id);
                if (nexthop_valid == TRUE) {
                    modify_field(p4i_to_arm.nexthop_type, nexthop_type);
                    if (nexthop_type == NEXTHOP_TYPE_VPC) {
                        modify_field(p4i_i2e.mapping_lkp_id, nexthop_id);
                    } else {
                        modify_field(p4i_i2e.mapping_bypass, TRUE);
                        modify_field(p4i_to_arm.nexthop_id, nexthop_id);
                    }
                }
            }
        }

        // if hardware register indicates miss, compare hashes with r1
        // (scratch_metadata.flow_hash) and setup lookup in overflow table
        modify_field(scratch_metadata.ipv4_flow_hash,
                     scratch_metadata.ipv4_flow_hash);
        modify_field(scratch_metadata.hint_valid, FALSE);
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.ipv4_flow_hash == hash1)) {
            modify_field(scratch_metadata.ipv4_flow_hint, hint1);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.ipv4_flow_hash == hash2)) {
            modify_field(scratch_metadata.ipv4_flow_hint, hint2);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        modify_field(scratch_metadata.flag, more_hashes);
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.flag == TRUE)) {
            modify_field(scratch_metadata.ipv4_flow_hint, more_hints);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }

        if (scratch_metadata.hint_valid == TRUE) {
            modify_field(control_metadata.flow_ohash_lkp, TRUE);
            modify_field(ingress_recirc.flow_ohash,
                         scratch_metadata.ipv4_flow_hint);
        } else {
            modify_field(control_metadata.flow_done, TRUE);
            modify_field(control_metadata.flow_miss, TRUE);
        }
    } else {
        modify_field(p4i_i2e.session_id, -1);
        modify_field(control_metadata.flow_done, TRUE);
        modify_field(control_metadata.flow_miss, TRUE);
    }

    modify_field(scratch_metadata.flag, entry_valid);
    modify_field(scratch_metadata.ipv4_flow_hash, hash1);
    modify_field(scratch_metadata.ipv4_flow_hash, hash2);
}

@pragma stage 2
@pragma hbm_table
@pragma capi_bitfields_struct
table ipv4_flow {
    reads {
        key_metadata.flow_lkp_id    : exact;
        key_metadata.ipv4_src       : exact;
        key_metadata.ipv4_dst       : exact;
        key_metadata.proto          : exact;
        key_metadata.sport          : exact;
        key_metadata.dport          : exact;
    }
    actions {
        ipv4_flow_hash;
    }
    size : IPV4_FLOW_TABLE_SIZE;
}

@pragma stage 3
@pragma hbm_table
@pragma overflow_table ipv4_flow
table ipv4_flow_ohash {
    reads {
        ingress_recirc.flow_ohash   : exact;
    }
    actions {
        ipv4_flow_hash;
    }
    size : IPV4_FLOW_OHASH_TABLE_SIZE;
}

/*****************************************************************************/
/* Additional info for flow                                                  */
/*****************************************************************************/
action flow_info(is_local_to_local, pad) {
    modify_field(p4i_i2e.is_local_to_local, is_local_to_local);
    modify_field(scratch_metadata.flow_info_pad, pad);
}

@pragma stage 4
@pragma hbm_table
@pragma index_table
table flow_info {
    reads {
        key_metadata.flow_info_id   : exact;
    }
    actions {
        flow_info;
    }
    size : FLOW_INFO_TABLE_SIZE;
}

control flow_lookup {
    if (control_metadata.flow_ohash_lkp == FALSE) {
        if (key_metadata.ktype == KEY_TYPE_IPV4) {
            apply(ipv4_flow);
        } else {
            apply(flow);
        }
    }
    if (control_metadata.flow_ohash_lkp == TRUE) {
        if (key_metadata.ktype == KEY_TYPE_IPV4) {
            apply(ipv4_flow_ohash);
        } else {
            apply(flow_ohash);
        }
    }
    if (control_metadata.flow_info_lkp == TRUE) {
        apply(flow_info);
    }
}
