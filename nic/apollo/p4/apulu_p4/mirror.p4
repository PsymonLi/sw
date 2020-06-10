action mirror_truncate(truncate_len, adjust_len, is_erspan) {
    if ((truncate_len != 0) and
        (truncate_len < (capri_p4_intrinsic.packet_len - 14))) {
        modify_field(capri_deparser_len.trunc_pkt_len,
                     truncate_len - adjust_len);
        subtract_from_field(capri_p4_intrinsic.packet_len, truncate_len + 14);
        if (is_erspan == TRUE) {
            modify_field(erspan3.truncated, TRUE);
        }
    }
}

action lspan(nexthop_type, nexthop_id, truncate_len, span_tm_oq) {
    modify_field(egress_recirc.mapping_done, TRUE);
    modify_field(control_metadata.mapping_done, TRUE);

    remove_header(mirror_blob);
    modify_field(capri_intrinsic.tm_span_session, 0);
    modify_field(rewrite_metadata.nexthop_type, nexthop_type);
    modify_field(p4e_i2e.nexthop_id, nexthop_id);
    modify_field(capri_intrinsic.tm_oq, span_tm_oq);

    // adjust length
    subtract(capri_p4_intrinsic.packet_len, capri_p4_intrinsic.frame_size,
             offset_metadata.l2_1);
    if (ctag_s.valid == TRUE) {
        mirror_truncate(truncate_len, 4, FALSE);
    } else {
        mirror_truncate(truncate_len, 0, FALSE);
    }
}

action rspan(nexthop_type, nexthop_id, ctag, truncate_len, span_tm_oq) {
    modify_field(egress_recirc.mapping_done, TRUE);
    modify_field(control_metadata.mapping_done, TRUE);

    remove_header(mirror_blob);
    modify_field(capri_intrinsic.tm_span_session, 0);
    modify_field(rewrite_metadata.nexthop_type, nexthop_type);
    modify_field(p4e_i2e.nexthop_id, nexthop_id);
    modify_field(capri_intrinsic.tm_oq, span_tm_oq);

    // add rspan vlan
    add_header(ctag_1);
    modify_field(ctag_1.vid, ctag);
    modify_field(ctag_1.dei, 0);
    modify_field(ctag_1.pcp, 0);
    modify_field(ctag_1.etherType, ethernet_1.etherType);
    modify_field(ethernet_1.etherType, ETHERTYPE_VLAN);

    // adjust length
    subtract(capri_p4_intrinsic.packet_len, capri_p4_intrinsic.frame_size,
             offset_metadata.l2_1);
    if (ctag_s.valid == TRUE) {
        mirror_truncate(truncate_len, 4, FALSE);
    } else {
        mirror_truncate(truncate_len, 0, FALSE);
    }
    add_to_field(capri_p4_intrinsic.packet_len, 4);
}

action erspan(nexthop_type, nexthop_id, egress_bd_id, rewrite_flags,
              truncate_len, dmac, smac, sip, dip, span_id, dscp,
              span_tm_oq, vlan_strip_en, erspan_type, gre_seq_en,
              seq_num, npkts, nbytes) {
    subtract(capri_p4_intrinsic.packet_len, capri_p4_intrinsic.frame_size,
             offset_metadata.l2_1);

    // stats via table write
    add(scratch_metadata.in_packets, npkts, 1);
    add(scratch_metadata.in_bytes, nbytes, capri_p4_intrinsic.packet_len);

    // bump up gre sequence and release table lock
    modify_field(gre_0_opt_seq.seq_num, seq_num);

    modify_field(egress_recirc.mapping_done, TRUE);
    modify_field(control_metadata.mapping_done, TRUE);

    // vlan tag handling
    if (ctag_s.valid == TRUE) {
        modify_field(erspan3.vlan, ctag_s.vid);
        modify_field(erspan3.cos, ctag_s.pcp);
        modify_field(scratch_metadata.flag, vlan_strip_en);
        if (scratch_metadata.flag == TRUE) {
            modify_field(ethernet_1.etherType, ctag_s.etherType);
            remove_header(ctag_s);
            subtract_from_field(capri_p4_intrinsic.packet_len, 4);
            if (erspan_type == ERSPAN_TYPE_II) {
                modify_field(erspan2.encap_type, 0x2);
            }
        } else {
            if (erspan_type == ERSPAN_TYPE_II) {
                modify_field(erspan2.encap_type, 0x3);
            }
        }
    }

    // truncate (if needed)
    if (ctag_s.valid == TRUE) {
        mirror_truncate(truncate_len, 4, FALSE);
    } else {
        mirror_truncate(truncate_len, 0, FALSE);
    }

    // ethernet
    add_header(ethernet_0);
    modify_field(ethernet_0.dstAddr, dmac);
    modify_field(ethernet_0.srcAddr, smac);
    modify_field(ethernet_0.etherType, ETHERTYPE_IPV4);
    modify_field(scratch_metadata.packet_len, 14);

    // ipv4
    add_header(ipv4_0);
    modify_field(ipv4_0.version, 4);
    modify_field(ipv4_0.ihl, 5);
    modify_field(ipv4_0.diffserv, dscp);
    modify_field(ipv4_0.ttl, 64);
    modify_field(ipv4_0.protocol, IP_PROTO_GRE);
    modify_field(ipv4_0.srcAddr, sip);
    modify_field(ipv4_0.dstAddr, dip);

    // gre
    add_header(gre_0);
    modify_field(gre_0.C, 0);
    modify_field(gre_0.R, 0);
    modify_field(gre_0.K, 0);
    modify_field(gre_0.s, 0);
    modify_field(gre_0.recurse, 0);
    modify_field(gre_0.flags, 0);
    modify_field(gre_0.ver, 0);
    modify_field(scratch_metadata.flag, gre_seq_en);
    if (scratch_metadata.flag == TRUE) {
        add_header(gre_0_opt_seq);
        modify_field(gre_0.S, 1);
    } else {
        modify_field(gre_0.S, 0);
    }

    // erspan
    modify_field(erspan3.span_id, span_id);
    if ((erspan_type == 0) or (erspan_type == ERSPAN_TYPE_III)) {
        if (gre_seq_en == TRUE) {
            /* ipv4 (20) + gre (4) + gre_opt (4) erspan3 (12) */
            add(ipv4_0.totalLen, capri_p4_intrinsic.packet_len, 40);
            add_to_field(scratch_metadata.packet_len, 40);
        } else {
            /* ipv4 (20) + gre (4) + erspan3 (12) */
            add(ipv4_0.totalLen, capri_p4_intrinsic.packet_len, 36);
            add_to_field(scratch_metadata.packet_len, 36);
        }
        modify_field(gre_0.proto, GRE_PROTO_ERSPAN_T3);
        add_header(erspan3);
        modify_field(erspan3.version, 2);
        modify_field(erspan3.timestamp, 0 /* R4 */);
        modify_field(erspan3.granularity, 0x3);
        if (capri_intrinsic.tm_iport == TM_PORT_EGRESS) {
            modify_field(erspan3.direction, 1);
        }
        // TODO
        add_header(erspan3_opt);
    } else {
        modify_field(gre_0.proto, GRE_PROTO_ERSPAN);
        if (erspan_type == ERSPAN_TYPE_II) {
            if (gre_seq_en == TRUE) {
                /* ipv4 (20) + gre (4) + gre_opt (4) + erspan2 (8) */
                add(ipv4_0.totalLen, capri_p4_intrinsic.packet_len, 36);
                add_to_field(scratch_metadata.packet_len, 36);
            } else {
                /* ipv4 (20) + gre (4) + erspan2 (8) */
                add(ipv4_0.totalLen, capri_p4_intrinsic.packet_len, 32);
                add_to_field(scratch_metadata.packet_len, 32);
            }
            add_header(erspan2);
            modify_field(erspan2.version, 1);
            modify_field(erspan2.port_id, capri_intrinsic.lif);
        } else {
            /* ipv4 (20) + gre (4) */
            add(ipv4_0.totalLen, capri_p4_intrinsic.packet_len, 24);
            add_to_field(scratch_metadata.packet_len, 24);
        }
    }

    remove_header(mirror_blob);
    modify_field(capri_intrinsic.tm_span_session, 0);
    modify_field(capri_intrinsic.tm_oq, span_tm_oq);
    add_to_field(capri_p4_intrinsic.packet_len, scratch_metadata.packet_len);
    modify_field(rewrite_metadata.nexthop_type, nexthop_type);
    modify_field(p4e_i2e.nexthop_id, nexthop_id);
    modify_field(vnic_metadata.egress_bd_id, egress_bd_id);
    modify_field(rewrite_metadata.flags, rewrite_flags);
    modify_field(control_metadata.erspan_copy, TRUE);
    modify_field(scratch_metadata.erspan_type, erspan_type);
}

@pragma stage 0
@pragma table_write
table mirror {
    reads {
        capri_intrinsic.tm_span_session : exact;
    }
    actions {
        lspan;
        rspan;
        erspan;
    }
    size : MIRROR_SESSION_TABLE_SIZE;
}

control mirror {
    apply(mirror);
}
