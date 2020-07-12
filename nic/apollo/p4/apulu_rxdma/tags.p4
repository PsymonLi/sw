/******************************************************************************/
/* derive tags based on local_tag_idx                                         */
/******************************************************************************/
action local_mapping_tag_info(tag0, tag1, tag2, tag3, tag4) {
    if (p4_to_rxdma.rx_packet == FALSE) {
        // Tx packet
        modify_field(lpm_metadata.stag0, tag0);
        modify_field(lpm_metadata.stag1, tag1);
        modify_field(lpm_metadata.stag2, tag2);
        modify_field(lpm_metadata.stag3, tag3);
        modify_field(lpm_metadata.stag4, tag4);
    } else {
        // Rx packet
        modify_field(lpm_metadata.dtag0, tag0);
        modify_field(lpm_metadata.dtag1, tag1);
        modify_field(lpm_metadata.dtag2, tag2);
        modify_field(lpm_metadata.dtag3, tag3);
        modify_field(lpm_metadata.dtag4, tag4);
    }
}

@pragma stage 6
@pragma hbm_table
@pragma index_table
@pragma capi_bitfields_struct
table local_mapping_tag {
    reads {
        p4_to_rxdma.local_tag_idx   : exact;
    }
    actions {
        local_mapping_tag_info;
    }
    size : LOCAL_MAPPING_TAG_TABLE_SIZE;
}

/******************************************************************************/
/* derive tags based on mapping_tag_idx                                       */
/******************************************************************************/
action mapping_tag_info(tag0, tag1, tag2, tag3, tag4) {
    if (p4_to_rxdma.rx_packet == FALSE) {
        // Tx packet
        modify_field(lpm_metadata.dtag0, tag0);
        modify_field(lpm_metadata.dtag1, tag1);
        modify_field(lpm_metadata.dtag2, tag2);
        modify_field(lpm_metadata.dtag3, tag3);
        modify_field(lpm_metadata.dtag4, tag4);
    } else {
        // Rx packet
        modify_field(lpm_metadata.stag0, tag0);
        modify_field(lpm_metadata.stag1, tag1);
        modify_field(lpm_metadata.stag2, tag2);
        modify_field(lpm_metadata.stag3, tag3);
        modify_field(lpm_metadata.stag4, tag4);
    }
}

@pragma stage 6
@pragma hbm_table
@pragma index_table
@pragma capi_bitfields_struct
table mapping_tag {
    reads {
        lpm_metadata.mapping_tag_idx    : exact;
    }
    actions {
        mapping_tag_info;
    }
    size : MAPPING_TAG_TABLE_SIZE;
}

control derive_tags {
    if (p4_to_rxdma.mapping_done == TRUE) {
        apply(local_mapping_tag);
        apply(mapping_tag);
    }
}
