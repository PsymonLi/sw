#include "../include/apulu_sacl_defines.h"

action read_qstate_info(PKTQ_QSTATE) {
    // in txdma
    //          check sw_cindex0, pindex0
    //          tbl-wr sw_cindex0++
    //          doorbell(dma) cindex0
    //          dma to rxdma_qstate cindex
    // d-vector
    PKTQ_QSTATE_DVEC_SCRATCH(scratch_qstate_hdr, scratch_qstate_info);

    if (scratch_qstate_info.sw_cindex0 == scratch_qstate_hdr.p_index0) {
        modify_field(capri_intr.drop, TRUE);
    } else {
        modify_field(app_header.table3_valid, TRUE);
        modify_field(txdma_control.payload_addr,
                     scratch_qstate_info.ring0_base +
                     (scratch_qstate_info.sw_cindex0 * PKTQ_PAGE_SIZE));

        modify_field(txdma_control.pktdesc_addr1,
                     scratch_qstate_info.ring1_base +
                     (scratch_qstate_info.sw_cindex0 << APULU_PKT_DESC_SHIFT));
        modify_field(txdma_control.pktdesc_addr2,
                      scratch_qstate_info.ring1_base +
                      (scratch_qstate_info.sw_cindex0 << APULU_PKT_DESC_SHIFT) + 64);
        modify_field(txdma_control.pktdesc_addr3,
                      scratch_qstate_info.ring1_base +
                      (scratch_qstate_info.sw_cindex0 << APULU_PKT_DESC_SHIFT) + 128);

        modify_field(scratch_qstate_info.sw_cindex0,
                     scratch_qstate_info.sw_cindex0 + 1);
        modify_field(txdma_control.cindex, scratch_qstate_info.sw_cindex0);
        modify_field(txdma_control.rxdma_cindex_addr,
                     scratch_qstate_info.rxdma_cindex_addr);
    }
}

@pragma stage 0
@pragma raw_index_table
@pragma table_write
table read_qstate {
    reads {
        capri_txdma_intr.qstate_addr    : exact;
    }
    actions {
        read_qstate_info;
    }
}

action read_pktdesc1(remote_ip,
                     route_base_addr,
                     sacl_base_addr0,
                     sacl_base_addr1,
                     sacl_base_addr2,
                     sacl_base_addr3,
                     sacl_base_addr4,
                     sacl_base_addr5,
                     src_bd_id,
                     pad1,
                     src_mapping_hit,
                     sip_classid0,
                     dip_classid0,
                     pad_classid0,
                     sport_classid0,
                     dport_classid0,
                     sip_classid1,
                     dip_classid1,
                     pad_classid1,
                     sport_classid1,
                     dport_classid1
                    )
{
    modify_field(rx_to_tx_hdr.remote_ip, remote_ip);
    modify_field(rx_to_tx_hdr.route_base_addr, route_base_addr);
    modify_field(rx_to_tx_hdr.sacl_base_addr0, sacl_base_addr0);
    modify_field(rx_to_tx_hdr.sacl_base_addr1, sacl_base_addr1);
    modify_field(rx_to_tx_hdr.sacl_base_addr2, sacl_base_addr2);
    modify_field(rx_to_tx_hdr.sacl_base_addr3, sacl_base_addr3);
    modify_field(rx_to_tx_hdr.sacl_base_addr4, sacl_base_addr4);
    modify_field(rx_to_tx_hdr.sacl_base_addr5, sacl_base_addr5);
    modify_field(rx_to_tx_hdr.src_bd_id, src_bd_id);
    modify_field(scratch_metadata.field7, pad1);
    modify_field(rx_to_tx_hdr.src_mapping_hit, src_mapping_hit);
    modify_field(rx_to_tx_hdr.sip_classid0, sip_classid0);
    modify_field(rx_to_tx_hdr.dip_classid0, dip_classid0);
    modify_field(scratch_metadata.field4, pad_classid0);
    modify_field(rx_to_tx_hdr.sport_classid0, sport_classid0);
    modify_field(rx_to_tx_hdr.dport_classid0, dport_classid0);
    modify_field(rx_to_tx_hdr.sip_classid1, sip_classid1);
    modify_field(rx_to_tx_hdr.dip_classid1, dip_classid1);
    modify_field(scratch_metadata.field4, pad_classid1);
    modify_field(rx_to_tx_hdr.sport_classid1, sport_classid1);
    modify_field(rx_to_tx_hdr.dport_classid1, dport_classid1);

    // Setup for route LPM lookup
    if (route_base_addr != 0) {
        modify_field(txdma_control.lpm1_key, remote_ip);
        modify_field(txdma_control.lpm1_base_addr, route_base_addr);
        modify_field(txdma_predicate.lpm1_enable, TRUE);
    }

    // Setup for RFC lookup
    if (sacl_base_addr0 != 0) {
        // Initialize the first P1 table index
        // = (sip_classid0 << SACL_SPORT_CLASSID_WIDTH) | sport_classid0
        modify_field(scratch_metadata.field20, (scratch_metadata.field10 <<
                                                SACL_SPORT_CLASSID_WIDTH) |
                                                scratch_metadata.field8);
        // Write P1 table index to PHV
        modify_field(txdma_control.rfc_index, scratch_metadata.field20);

        // Write P1 table lookup address to PHV
        modify_field(txdma_control.rfc_table_addr,              // P1 Lookup Addr =
                     sacl_base_addr0 +                          // Region Base +
                     SACL_P1_TABLE_OFFSET +                     // Table Base +
                     (((scratch_metadata.field20) /             // Index Bytes
                       SACL_P1_ENTRIES_PER_CACHE_LINE) *
                      SACL_CACHE_LINE_SIZE));

        modify_field(txdma_predicate.rfc_enable, TRUE);
        modify_field(txdma_control.rule_priority, SACL_PRIORITY_INVALID);
    }
}

@pragma stage 1
@pragma raw_index_table
table read_pktdesc1 {
    reads {
        txdma_control.pktdesc_addr1  : exact;
    }
    actions {
        read_pktdesc1;
    }
}

action read_pktdesc2(sip_classid2,
                     dip_classid2,
                     pad_classid2,
                     sport_classid2,
                     dport_classid2,
                     sip_classid3,
                     dip_classid3,
                     pad_classid3,
                     sport_classid3,
                     dport_classid3,
                     sip_classid4,
                     dip_classid4,
                     pad_classid4,
                     sport_classid4,
                     dport_classid4,
                     sip_classid5,
                     dip_classid5,
                     pad_classid5,
                     sport_classid5,
                     dport_classid5,
                     vpc_id,
                     vnic_id,
                     iptype,
                     rx_packet,
                     payload_len,
                     stag0_classid0,
                     stag1_classid0,
                     stag2_classid0,
                     stag3_classid0,
                     stag4_classid0,
                     dtag0_classid0,
                     dtag1_classid0,
                     dtag2_classid0,
                     dtag3_classid0,
                     dtag4_classid0,
                     stag0_classid1,
                     stag1_classid1,
                     stag2_classid1,
                     stag3_classid1,
                     stag4_classid1,
                     dtag0_classid1,
                     dtag1_classid1,
                     dtag2_classid1,
                     dtag3_classid1,
                     dtag4_classid1,
                     stag0_classid2,
                     stag1_classid2,
                     stag2_classid2,
                     stag3_classid2,
                     stag4_classid2,
                     dtag0_classid2,
                     dtag1_classid2,
                     dtag2_classid2,
                     dtag3_classid2,
                     dtag4_classid2,
                     pad2)
{
    modify_field(rx_to_tx_hdr.sip_classid2, sip_classid2);
    modify_field(rx_to_tx_hdr.dip_classid2, dip_classid2);
    modify_field(scratch_metadata.field4, pad_classid2);
    modify_field(rx_to_tx_hdr.sport_classid2, sport_classid2);
    modify_field(rx_to_tx_hdr.dport_classid2, dport_classid2);
    modify_field(rx_to_tx_hdr.sip_classid3, sip_classid3);
    modify_field(rx_to_tx_hdr.dip_classid3, dip_classid3);
    modify_field(scratch_metadata.field4, pad_classid3);
    modify_field(rx_to_tx_hdr.sport_classid3, sport_classid3);
    modify_field(rx_to_tx_hdr.dport_classid3, dport_classid3);
    modify_field(rx_to_tx_hdr.sip_classid4, sip_classid4);
    modify_field(rx_to_tx_hdr.dip_classid4, dip_classid4);
    modify_field(scratch_metadata.field4, pad_classid4);
    modify_field(rx_to_tx_hdr.sport_classid4, sport_classid4);
    modify_field(rx_to_tx_hdr.dport_classid4, dport_classid4);
    modify_field(rx_to_tx_hdr.sip_classid5, sip_classid5);
    modify_field(rx_to_tx_hdr.dip_classid5, dip_classid5);
    modify_field(scratch_metadata.field4, pad_classid5);
    modify_field(rx_to_tx_hdr.sport_classid5, sport_classid5);
    modify_field(rx_to_tx_hdr.dport_classid5, dport_classid5);
    modify_field(rx_to_tx_hdr.vpc_id, vpc_id);
    modify_field(rx_to_tx_hdr.vnic_id, vnic_id);
    modify_field(rx_to_tx_hdr.iptype, iptype);
    modify_field(rx_to_tx_hdr.rx_packet, rx_packet);
    modify_field(rx_to_tx_hdr.payload_len, payload_len);
    modify_field(rx_to_tx_hdr.stag0_classid0, stag0_classid0);
    modify_field(rx_to_tx_hdr.stag1_classid0, stag1_classid0);
    modify_field(rx_to_tx_hdr.stag2_classid0, stag2_classid0);
    modify_field(rx_to_tx_hdr.stag3_classid0, stag3_classid0);
    modify_field(rx_to_tx_hdr.stag4_classid0, stag4_classid0);
    modify_field(rx_to_tx_hdr.dtag0_classid0, dtag0_classid0);
    modify_field(rx_to_tx_hdr.dtag1_classid0, dtag1_classid0);
    modify_field(rx_to_tx_hdr.dtag2_classid0, dtag2_classid0);
    modify_field(rx_to_tx_hdr.dtag3_classid0, dtag3_classid0);
    modify_field(rx_to_tx_hdr.dtag4_classid0, dtag4_classid0);
    modify_field(rx_to_tx_hdr.stag0_classid1, stag0_classid1);
    modify_field(rx_to_tx_hdr.stag1_classid1, stag1_classid1);
    modify_field(rx_to_tx_hdr.stag2_classid1, stag2_classid1);
    modify_field(rx_to_tx_hdr.stag3_classid1, stag3_classid1);
    modify_field(rx_to_tx_hdr.stag4_classid1, stag4_classid1);
    modify_field(rx_to_tx_hdr.dtag0_classid1, dtag0_classid1);
    modify_field(rx_to_tx_hdr.dtag1_classid1, dtag1_classid1);
    modify_field(rx_to_tx_hdr.dtag2_classid1, dtag2_classid1);
    modify_field(rx_to_tx_hdr.dtag3_classid1, dtag3_classid1);
    modify_field(rx_to_tx_hdr.dtag4_classid1, dtag4_classid1);
    modify_field(rx_to_tx_hdr.stag0_classid2, stag0_classid2);
    modify_field(rx_to_tx_hdr.stag1_classid2, stag1_classid2);
    modify_field(rx_to_tx_hdr.stag2_classid2, stag2_classid2);
    modify_field(rx_to_tx_hdr.stag3_classid2, stag3_classid2);
    modify_field(rx_to_tx_hdr.stag4_classid2, stag4_classid2);
    modify_field(rx_to_tx_hdr.dtag0_classid2, dtag0_classid2);
    modify_field(rx_to_tx_hdr.dtag1_classid2, dtag1_classid2);
    modify_field(rx_to_tx_hdr.dtag2_classid2, dtag2_classid2);
    modify_field(rx_to_tx_hdr.dtag3_classid2, dtag3_classid2);
    modify_field(rx_to_tx_hdr.dtag4_classid2, dtag4_classid2);
    modify_field(scratch_metadata.field4, pad2);

    // Copy the first set of TAGs
    modify_field(txdma_control.stag_classid, stag0_classid0);
    modify_field(txdma_control.dtag_classid, dtag0_classid0);
}

@pragma stage 1
@pragma raw_index_table
table read_pktdesc2 {
    reads {
        txdma_control.pktdesc_addr2  : exact;
    }
    actions {
        read_pktdesc2;
    }
}

action read_pktdesc3(stag0_classid3,
                     stag1_classid3,
                     stag2_classid3,
                     stag3_classid3,
                     stag4_classid3,
                     dtag0_classid3,
                     dtag1_classid3,
                     dtag2_classid3,
                     dtag3_classid3,
                     dtag4_classid3,
                     stag0_classid4,
                     stag1_classid4,
                     stag2_classid4,
                     stag3_classid4,
                     stag4_classid4,
                     dtag0_classid4,
                     dtag1_classid4,
                     dtag2_classid4,
                     dtag3_classid4,
                     dtag4_classid4,
                     stag0_classid5,
                     stag1_classid5,
                     stag2_classid5,
                     stag3_classid5,
                     stag4_classid5,
                     dtag0_classid5,
                     dtag1_classid5,
                     dtag2_classid5,
                     dtag3_classid5,
                     dtag4_classid5,
                     pad_end)
{
    modify_field(rx_to_tx_hdr.stag0_classid3, stag0_classid3);
    modify_field(rx_to_tx_hdr.stag1_classid3, stag1_classid3);
    modify_field(rx_to_tx_hdr.stag2_classid3, stag2_classid3);
    modify_field(rx_to_tx_hdr.stag3_classid3, stag3_classid3);
    modify_field(rx_to_tx_hdr.stag4_classid3, stag4_classid3);
    modify_field(rx_to_tx_hdr.dtag0_classid3, dtag0_classid3);
    modify_field(rx_to_tx_hdr.dtag1_classid3, dtag1_classid3);
    modify_field(rx_to_tx_hdr.dtag2_classid3, dtag2_classid3);
    modify_field(rx_to_tx_hdr.dtag3_classid3, dtag3_classid3);
    modify_field(rx_to_tx_hdr.dtag4_classid3, dtag4_classid3);
    modify_field(rx_to_tx_hdr.stag0_classid4, stag0_classid4);
    modify_field(rx_to_tx_hdr.stag1_classid4, stag1_classid4);
    modify_field(rx_to_tx_hdr.stag2_classid4, stag2_classid4);
    modify_field(rx_to_tx_hdr.stag3_classid4, stag3_classid4);
    modify_field(rx_to_tx_hdr.stag4_classid4, stag4_classid4);
    modify_field(rx_to_tx_hdr.dtag0_classid4, dtag0_classid4);
    modify_field(rx_to_tx_hdr.dtag1_classid4, dtag1_classid4);
    modify_field(rx_to_tx_hdr.dtag2_classid4, dtag2_classid4);
    modify_field(rx_to_tx_hdr.dtag3_classid4, dtag3_classid4);
    modify_field(rx_to_tx_hdr.dtag4_classid4, dtag4_classid4);
    modify_field(rx_to_tx_hdr.stag0_classid5, stag0_classid5);
    modify_field(rx_to_tx_hdr.stag1_classid5, stag1_classid5);
    modify_field(rx_to_tx_hdr.stag2_classid5, stag2_classid5);
    modify_field(rx_to_tx_hdr.stag3_classid5, stag3_classid5);
    modify_field(rx_to_tx_hdr.stag4_classid5, stag4_classid5);
    modify_field(rx_to_tx_hdr.dtag0_classid5, dtag0_classid5);
    modify_field(rx_to_tx_hdr.dtag1_classid5, dtag1_classid5);
    modify_field(rx_to_tx_hdr.dtag2_classid5, dtag2_classid5);
    modify_field(rx_to_tx_hdr.dtag3_classid5, dtag3_classid5);
    modify_field(rx_to_tx_hdr.dtag4_classid5, dtag4_classid5);
    modify_field(scratch_metadata.field4, pad_end);
}

@pragma stage 1
@pragma raw_index_table
table read_pktdesc3 {
    reads {
        txdma_control.pktdesc_addr3  : exact;
    }
    actions {
        read_pktdesc3;
    }
}

control read_qstate {
    apply(read_qstate);
    apply(read_pktdesc1);
    apply(read_pktdesc2);
    apply(read_pktdesc3);
}
