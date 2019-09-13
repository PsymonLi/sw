/*****************************************************************************/
/* Policy (IPv6 and non-IP)                                                  */
/*****************************************************************************/
action flow_hash(epoch, session_id, flow_role, pad8,
                 hash1, hint1, hash2, hint2, hash3, hint3,
                 hash4, hint4,  more_hashes, more_hints, more_hints_pad,
                 entry_valid) {
    modify_field(p4i_i2e.entropy_hash, scratch_metadata.flow_hash);
    if (entry_valid == TRUE) {
        // if hardware register indicates hit, take the results
        modify_field(scratch_metadata.epoch, epoch);
        if (scratch_metadata.epoch < control_metadata.epoch) {
            // entry is old
            modify_field(ingress_recirc.flow_done, TRUE);
        } else {
            modify_field(ingress_recirc.flow_done, TRUE);
            modify_field(p4i_i2e.session_id, session_id);
            modify_field(p4i_i2e.flow_role, flow_role);
        }

        // if hardware register indicates miss, compare hashes with r1
        // (scratch_metadata.flow_hash) and setup lookup in overflow table
        modify_field(scratch_metadata.flow_hash,
                     scratch_metadata.flow_hash);
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
            modify_field(ingress_recirc.flow_done, TRUE);
            modify_field(control_metadata.flow_miss, TRUE);
        }
    } else {
        modify_field(ingress_recirc.flow_done, TRUE);
        modify_field(control_metadata.flow_miss, TRUE);
    }

    modify_field(scratch_metadata.flag, entry_valid);
    modify_field(scratch_metadata.flow_hash, hash1);
    modify_field(scratch_metadata.flow_hash, hash2);
    modify_field(scratch_metadata.flow_hash, hash3);
    modify_field(scratch_metadata.flow_hash, hash4);

    modify_field(scratch_metadata.pad8, pad8);
    modify_field(scratch_metadata.more_hints_pad, more_hints_pad);
}

@pragma stage 2
@pragma hbm_table
table flow {
    reads {
        vnic_metadata.bd_id     : exact;
        key_metadata.ktype      : exact;
        key_metadata.src        : exact;
        key_metadata.dst        : exact;
        key_metadata.proto      : exact;
        key_metadata.sport      : exact;
        key_metadata.dport      : exact;
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
action ipv4_flow_hash(epoch, session_id, flow_role, pad8,
                      hash1, hint1, hash2, hint2, more_hashes, more_hints,
                      more_hints_pad, entry_valid) {
    modify_field(p4i_i2e.entropy_hash, scratch_metadata.flow_hash);
    if (entry_valid == TRUE) {
        // if hardware register indicates hit, take the results
        modify_field(scratch_metadata.epoch, epoch);
        if (scratch_metadata.epoch < control_metadata.epoch) {
            // entry is old
            modify_field(ingress_recirc.flow_done, TRUE);
        } else {
            modify_field(ingress_recirc.flow_done, TRUE);
            modify_field(p4i_i2e.session_id, session_id);
            modify_field(p4i_i2e.flow_role, flow_role);
        }

        // if hardware register indicates miss, compare hashes with r1
        // (scratch_metadata.flow_hash) and setup lookup in overflow table
        modify_field(scratch_metadata.flow_hash,
                     scratch_metadata.flow_hash);
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
            modify_field(ingress_recirc.flow_done, TRUE);
            modify_field(control_metadata.flow_miss, TRUE);
        }
    } else {
        modify_field(ingress_recirc.flow_done, TRUE);
        modify_field(control_metadata.flow_miss, TRUE);
    }

    modify_field(scratch_metadata.flag, entry_valid);
    modify_field(scratch_metadata.flow_hash, hash1);
    modify_field(scratch_metadata.flow_hash, hash2);

    modify_field(scratch_metadata.pad8, pad8);
    modify_field(scratch_metadata.more_hints_pad, more_hints_pad);
}

@pragma stage 2
@pragma hbm_table
table ipv4_flow {
    reads {
        vnic_metadata.bd_id     : exact;
        key_metadata.ipv4_src   : exact;
        key_metadata.ipv4_dst   : exact;
        key_metadata.proto      : exact;
        key_metadata.sport      : exact;
        key_metadata.dport      : exact;
    }
    actions {
        ipv4_flow_hash;
    }
    size : FLOW_TABLE_SIZE;
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
    size : FLOW_OHASH_TABLE_SIZE;
}

control flow_lookup {
    if (ingress_recirc.valid == FALSE) {
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
}
