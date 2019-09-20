/******************************************************************************/
/* Local mapping                                                              */
/******************************************************************************/
@pragma capi appdatafields vnic_id
@pragma capi hwfields_access_api
action local_mapping_info(entry_valid, vnic_id,
                          hash1, hint1, hash2, hint2, hash3, hint3,
                          hash4, hint4, hash5, hint5, hash6, hint6,
                          hash7, hint7, hash8, hint8, hash9, hint9,
                          hash10, hint10, more_hashes, more_hints) {
    if (entry_valid == TRUE) {
        // if hardware register indicates hit, take the results
        if (vnic_id != 0) {
            modify_field(vnic_metadata.vnic_id, vnic_id);
        }
        modify_field(ingress_recirc.local_mapping_done, TRUE);

        // if hardware register indicates miss, compare hashes with r1
        // (scratch_metadata.local_mapping_hash) and setup lookup in
        // overflow table
        modify_field(scratch_metadata.local_mapping_hash,
                     scratch_metadata.local_mapping_hash);
        modify_field(scratch_metadata.hint_valid, FALSE);
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash1)) {
            modify_field(scratch_metadata.local_mapping_hint, hint1);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash2)) {
            modify_field(scratch_metadata.local_mapping_hint, hint2);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash3)) {
            modify_field(scratch_metadata.local_mapping_hint, hint3);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash4)) {
            modify_field(scratch_metadata.local_mapping_hint, hint4);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash5)) {
            modify_field(scratch_metadata.local_mapping_hint, hint5);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash6)) {
            modify_field(scratch_metadata.local_mapping_hint, hint6);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash7)) {
            modify_field(scratch_metadata.local_mapping_hint, hint7);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash8)) {
            modify_field(scratch_metadata.local_mapping_hint, hint8);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash9)) {
            modify_field(scratch_metadata.local_mapping_hint, hint9);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.local_mapping_hash == hash10)) {
            modify_field(scratch_metadata.local_mapping_hint, hint10);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        modify_field(scratch_metadata.flag, more_hashes);
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.flag == TRUE)) {
            modify_field(scratch_metadata.local_mapping_hint, more_hints);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }

        if (scratch_metadata.hint_valid == TRUE) {
            modify_field(ingress_recirc.local_mapping_ohash,
                         scratch_metadata.local_mapping_hint);
            modify_field(control_metadata.local_mapping_ohash_lkp, TRUE);
        } else {
            modify_field(ingress_recirc.local_mapping_done, TRUE);
        }
    } else {
        modify_field(ingress_recirc.local_mapping_done, TRUE);
    }

    modify_field(scratch_metadata.flag, entry_valid);
    modify_field(scratch_metadata.local_mapping_hash, hash1);
    modify_field(scratch_metadata.local_mapping_hash, hash2);
    modify_field(scratch_metadata.local_mapping_hash, hash3);
    modify_field(scratch_metadata.local_mapping_hash, hash4);
    modify_field(scratch_metadata.local_mapping_hash, hash5);
    modify_field(scratch_metadata.local_mapping_hash, hash6);
    modify_field(scratch_metadata.local_mapping_hash, hash7);
    modify_field(scratch_metadata.local_mapping_hash, hash8);
    modify_field(scratch_metadata.local_mapping_hash, hash9);
    modify_field(scratch_metadata.local_mapping_hash, hash10);
}

@pragma stage 2
@pragma hbm_table
table local_mapping {
    reads {
        key_metadata.local_mapping_lkp_type : exact;
        key_metadata.local_mapping_lkp_id   : exact;
        key_metadata.local_mapping_lkp_addr : exact;
    }
    actions {
        local_mapping_info;
    }
    size : LOCAL_MAPPING_TABLE_SIZE;
}

@pragma stage 3
@pragma hbm_table
@pragma overflow_table local_mapping
table local_mapping_ohash {
    reads {
        ingress_recirc.local_mapping_ohash  : exact;
    }
    actions {
        local_mapping_info;
    }
    size : LOCAL_MAPPING_OHASH_TABLE_SIZE;
}

control local_mapping {
    if (ingress_recirc.valid == FALSE) {
        apply(local_mapping);
    }
    if (control_metadata.local_mapping_ohash_lkp == TRUE) {
        apply(local_mapping_ohash);
    }
}

/******************************************************************************/
/* All mappings (local and remote)                                            */
/******************************************************************************/
@pragma capi appdatafields dmaci
@pragma capi hwfields_access_api
action mapping_info(entry_valid,
                    pad12, nexthop_valid, nexthop_type, nexthop_id, dmaci,
                    hash1, hint1, hash2, hint2, hash3, hint3,
                    hash4, hint4, hash5, hint5, hash6, hint6,
                    hash7, hint7, hash8, hint8, more_hashes, more_hints) {
    if (txdma_to_p4e.mapping_bypass == FALSE) {
        modify_field(rewrite_metadata.nexthop_type, txdma_to_p4e.nexthop_type);
        // return
    }
    if (entry_valid == TRUE) {
        // if hardware register indicates hit, take the results
        modify_field(rewrite_metadata.dmaci, dmaci);
        modify_field(scratch_metadata.flag, nexthop_valid);
        if (nexthop_valid == TRUE) {
            modify_field(rewrite_metadata.nexthop_type, nexthop_type);
            modify_field(txdma_to_p4e.nexthop_id, nexthop_id);
        }
        modify_field(egress_recirc.mapping_done, TRUE);

        // if hardware register indicates miss, compare hashes with r1
        // (scratch_metadata.mapping_hash) and setup lookup in
        // overflow table
        modify_field(scratch_metadata.mapping_hash,
                     scratch_metadata.mapping_hash);
        modify_field(scratch_metadata.hint_valid, FALSE);
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.mapping_hash == hash1)) {
            modify_field(scratch_metadata.mapping_hint, hint1);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.mapping_hash == hash2)) {
            modify_field(scratch_metadata.mapping_hint, hint2);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.mapping_hash == hash3)) {
            modify_field(scratch_metadata.mapping_hint, hint3);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.mapping_hash == hash4)) {
            modify_field(scratch_metadata.mapping_hint, hint4);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.mapping_hash == hash5)) {
            modify_field(scratch_metadata.mapping_hint, hint5);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.mapping_hash == hash6)) {
            modify_field(scratch_metadata.mapping_hint, hint6);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.mapping_hash == hash7)) {
            modify_field(scratch_metadata.mapping_hint, hint7);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.mapping_hash == hash8)) {
            modify_field(scratch_metadata.mapping_hint, hint8);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }
        modify_field(scratch_metadata.flag, more_hashes);
        if ((scratch_metadata.hint_valid == FALSE) and
            (scratch_metadata.flag == TRUE)) {
            modify_field(scratch_metadata.mapping_hint, more_hints);
            modify_field(scratch_metadata.hint_valid, TRUE);
        }

        if (scratch_metadata.hint_valid == TRUE) {
            modify_field(egress_recirc.mapping_ohash,
                         scratch_metadata.mapping_hint);
            modify_field(control_metadata.mapping_ohash_lkp, TRUE);
        } else {
            modify_field(egress_recirc.mapping_done, TRUE);
        }
    } else {
        modify_field(egress_recirc.mapping_done, TRUE);
    }

    modify_field(scratch_metadata.flag, entry_valid);
    modify_field(scratch_metadata.pad12, pad12);
    modify_field(scratch_metadata.mapping_hash, hash1);
    modify_field(scratch_metadata.mapping_hash, hash2);
    modify_field(scratch_metadata.mapping_hash, hash3);
    modify_field(scratch_metadata.mapping_hash, hash4);
    modify_field(scratch_metadata.mapping_hash, hash5);
    modify_field(scratch_metadata.mapping_hash, hash6);
    modify_field(scratch_metadata.mapping_hash, hash7);
    modify_field(scratch_metadata.mapping_hash, hash8);
}

@pragma stage 0
@pragma hbm_table
table mapping {
    reads {
        p4e_i2e.mapping_lkp_type        : exact;
        txdma_to_p4e.mapping_lkp_id     : exact;
        p4e_i2e.mapping_lkp_addr        : exact;
    }
    actions {
        mapping_info;
    }
    size : MAPPING_TABLE_SIZE;
}

@pragma stage 1
@pragma hbm_table
@pragma overflow_table mapping
table mapping_ohash {
    reads {
        egress_recirc.mapping_ohash     : exact;
    }
    actions {
        mapping_info;
    }
    size : MAPPING_OHASH_TABLE_SIZE;
}

control mapping {
    if (egress_recirc.valid == FALSE) {
        apply(mapping);
    }
    if (control_metadata.mapping_ohash_lkp == TRUE) {
        apply(mapping_ohash);
    }
}
