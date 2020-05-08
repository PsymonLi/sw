/*****************************************************************************/
/* Inter pipe : ingress pipeline                                             */
/*****************************************************************************/
action tunnel_decap() {
    remove_header(ethernet_1);
    remove_header(ctag_1);
    remove_header(ipv4_1);
    remove_header(ipv6_1);
    remove_header(udp_1);
    remove_header(vxlan_1);
    subtract(capri_p4_intrinsic.packet_len, capri_p4_intrinsic.frame_size,
             offset_metadata.l2_2);
}

action ingress_to_egress() {
    if (control_metadata.rx_packet == FALSE) {
        modify_field(capri_intrinsic.tm_span_session, p4i_i2e.mirror_session);
    }
    add_header(capri_p4_intrinsic);
    add_header(p4i_i2e);
    remove_header(capri_txdma_intrinsic);
    remove_header(p4plus_to_p4);
    remove_header(p4plus_to_p4_vlan);
    remove_header(ingress_recirc);
    modify_field(capri_intrinsic.tm_oport, TM_PORT_EGRESS);
}

action ingress_to_rxdma() {
    remove_header(capri_txdma_intrinsic);
    remove_header(p4plus_to_p4);
    remove_header(p4plus_to_p4_vlan);
    remove_header(ingress_recirc);

    add_header(capri_p4_intrinsic);
    add_header(capri_rxdma_intrinsic);
    add_header(p4i_to_rxdma);
    add_header(p4i_i2e);
    add_header(p4i_to_arm);

    modify_field(p4i_to_arm.packet_len, capri_p4_intrinsic.packet_len);
    modify_field(p4i_to_arm.flow_hash, p4i_i2e.entropy_hash);
    modify_field(p4i_to_arm.vnic_id, vnic_metadata.vnic_id);
    modify_field(p4i_to_arm.ingress_bd_id, vnic_metadata.bd_id);
    modify_field(p4i_to_arm.vpc_id, vnic_metadata.vpc_id);

    modify_field(offset_metadata.l2_1, offset_metadata.l2_1);
    if (p4plus_to_p4.insert_vlan_tag == TRUE) {
        add(offset_metadata.l3_1, offset_metadata.l3_1, 4);
        add(offset_metadata.l4_1, offset_metadata.l4_1, 4);
        add(offset_metadata.l2_2, offset_metadata.l2_2, 4);
        add(offset_metadata.l3_2, offset_metadata.l3_2, 4);
        add(offset_metadata.l4_2, offset_metadata.l4_2, 4);
        add(p4i_to_arm.payload_offset, offset_metadata.payload_offset, 4);
    } else {
        modify_field(p4i_to_arm.payload_offset, offset_metadata.payload_offset);
    }
    modify_field(p4i_to_arm.lif, capri_intrinsic.lif);
    modify_field(p4i_to_arm.mapping_xlate_id, p4i_i2e.xlate_id);

    modify_field(scratch_metadata.cpu_flags, 0);
    if (ctag_1.valid == TRUE) {
        bit_or(scratch_metadata.cpu_flags, scratch_metadata.cpu_flags,
               APULU_CPU_FLAGS_VLAN_VALID);
    }
    if (ipv4_1.valid == TRUE) {
        bit_or(scratch_metadata.cpu_flags, scratch_metadata.cpu_flags,
               APULU_CPU_FLAGS_IPV4_1_VALID);
    } else {
        if (ipv6_1.valid == TRUE) {
            bit_or(scratch_metadata.cpu_flags, scratch_metadata.cpu_flags,
                   APULU_CPU_FLAGS_IPV6_1_VALID);
        }
    }
    if (control_metadata.tunneled_packet == TRUE) {
        if (ethernet_2.valid == TRUE) {
            bit_or(scratch_metadata.cpu_flags, scratch_metadata.cpu_flags,
                   APULU_CPU_FLAGS_ETH_2_VALID);
        }
        if (ipv4_2.valid == TRUE) {
            bit_or(scratch_metadata.cpu_flags, scratch_metadata.cpu_flags,
                   APULU_CPU_FLAGS_IPV4_2_VALID);
        } else {
            if (ipv6_2.valid == TRUE) {
                bit_or(scratch_metadata.cpu_flags, scratch_metadata.cpu_flags,
                       APULU_CPU_FLAGS_IPV6_2_VALID);
            }
        }
    } else {
        if (udp_1.valid == FALSE) {
            modify_field(offset_metadata.l4_1, offset_metadata.l4_2);
            modify_field(offset_metadata.l4_2, 0);
        }
    }
    modify_field(p4i_to_arm.flags, scratch_metadata.cpu_flags);

    modify_field(p4i_to_rxdma.apulu_p4plus, TRUE);
    if (key_metadata.ktype == KEY_TYPE_IPV4) {
        modify_field(scratch_metadata.flag, 0);
        modify_field(p4i_to_rxdma.vnic_info_en, TRUE);
    }
    if (key_metadata.ktype == KEY_TYPE_IPV6) {
        modify_field(scratch_metadata.flag, 1);
        modify_field(p4i_to_rxdma.vnic_info_en, TRUE);
    }
    if (control_metadata.force_flow_miss == TRUE) {
        modify_field(p4i_to_rxdma.vnic_info_en, FALSE);
    }
    modify_field(p4i_to_rxdma.vnic_info_key,
                 ((control_metadata.rx_packet << 11) |
                  (vnic_metadata.vnic_id << 1) | scratch_metadata.flag));

    add_to_field(capri_p4_intrinsic.packet_len,
                 (APULU_P4_TO_ARM_HDR_SZ + APULU_I2E_HDR_SZ));
    modify_field(capri_intrinsic.tm_oport, TM_PORT_DMA);
    modify_field(capri_intrinsic.lif, APULU_SERVICE_LIF);
    modify_field(capri_rxdma_intrinsic.rx_splitter_offset,
                 (ASICPD_GLOBAL_INTRINSIC_HDR_SZ +
                  ASICPD_RXDMA_INTRINSIC_HDR_SZ + APULU_P4I_TO_RXDMA_HDR_SZ));
}

action ingress_recirc() {
    add_header(capri_p4_intrinsic);
    add_header(ingress_recirc);
    remove_header(capri_txdma_intrinsic);
    remove_header(p4plus_to_p4);
    remove_header(p4plus_to_p4_vlan);
    modify_field(capri_intrinsic.tm_span_session, 0);
    modify_field(capri_intrinsic.tm_oport, TM_PORT_INGRESS);
}

action p4i_inter_pipe() {
    if ((control_metadata.flow_done == FALSE) or
        (control_metadata.local_mapping_done == FALSE)) {
        ingress_recirc();
        // return
    }

    if (control_metadata.redirect_to_arm == TRUE) {
        ingress_to_rxdma();
    } else {
        if (control_metadata.tunneled_packet == TRUE) {
            tunnel_decap();
        }
        ingress_to_egress();
    }
}

@pragma stage 5
table p4i_inter_pipe {
    actions {
        p4i_inter_pipe;
    }
}

control ingress_inter_pipe {
    if (capri_intrinsic.drop == 0) {
        apply(p4i_inter_pipe);
    }
}

/*****************************************************************************/
/* Inter pipe : egress pipeline                                              */
/*****************************************************************************/
action egress_to_rxdma() {
    add_header(capri_rxdma_intrinsic);
    add_header(p4e_to_p4plus_classic_nic);

    if (p4e_to_arm.valid == TRUE) {
        add_to_field(capri_p4_intrinsic.packet_len, APULU_P4_TO_ARM_HDR_SZ);
        modify_field(p4e_to_arm.rx_packet, control_metadata.rx_packet);
        modify_field(p4e_to_arm.egress_bd_id, vnic_metadata.egress_bd_id);
        modify_field(p4e_to_arm.sacl_action, txdma_to_p4e.sacl_action);
        modify_field(p4e_to_arm.sacl_root, txdma_to_p4e.sacl_root_num);
        modify_field(p4e_to_arm.drop, txdma_to_p4e.drop);
        modify_field(p4e_to_arm.snat_type, txdma_to_p4e.snat_type);
        modify_field(p4e_to_arm.dnat_en, txdma_to_p4e.dnat_en);
        modify_field(p4e_to_arm.dnat_id, txdma_to_p4e.dnat_idx);
        modify_field(p4e_to_arm.route_priority, txdma_to_p4e.route_priority);
    } else {
        if (control_metadata.rx_packet == TRUE) {
            modify_field(capri_intrinsic.tm_span_session,
                         p4e_i2e.mirror_session);
        }
        if ((rewrite_metadata.vlan_strip_en == TRUE) and
            (ctag_1.valid == TRUE)) {
            modify_field(ethernet_1.etherType, ctag_1.etherType);
            modify_field(p4e_to_p4plus_classic_nic.vlan_pcp, ctag_1.pcp);
            modify_field(p4e_to_p4plus_classic_nic.vlan_dei, ctag_1.dei);
            modify_field(p4e_to_p4plus_classic_nic.vlan_vid, ctag_1.vid);
            modify_field(p4e_to_p4plus_classic_nic.vlan_valid, TRUE);
            remove_header(ctag_1);
            subtract(capri_p4_intrinsic.packet_len,
                     capri_p4_intrinsic.packet_len, 4);
        }

    }

    // l2 checksum code in ASM
    if (ctag_1.valid == TRUE) {
    }

    modify_field(p4e_to_p4plus_classic_nic.packet_len,
                 capri_p4_intrinsic.packet_len);
    modify_field(p4e_to_p4plus_classic_nic.p4plus_app_id,
                 P4PLUS_APPTYPE_CLASSIC_NIC);
    modify_field(capri_rxdma_intrinsic.rx_splitter_offset,
                 (ASICPD_GLOBAL_INTRINSIC_HDR_SZ + ASICPD_RXDMA_INTRINSIC_HDR_SZ +
                  P4PLUS_CLASSIC_NIC_HDR_SZ));

    // RSS
    if (ethernet_2.valid == TRUE) {
        modify_field(p4e_to_p4plus_classic_nic.encap_pkt, TRUE);
        add_header(p4e_to_p4plus_classic_nic_ip2);
        if (ipv4_2.valid == TRUE) {
            if (tcp.valid == TRUE) {
                modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                             CLASSIC_NIC_PKT_TYPE_IPV4_TCP);
            } else {
                if (udp_2.valid == TRUE) {
                    modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                                 CLASSIC_NIC_PKT_TYPE_IPV4_UDP);
                } else {
                    modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                                 CLASSIC_NIC_PKT_TYPE_IPV4);
                }
            }
        }
        if (ipv6_2.valid == TRUE) {
            if (tcp.valid == TRUE) {
                modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                             CLASSIC_NIC_PKT_TYPE_IPV6_TCP);
            } else {
                if (udp_2.valid == TRUE) {
                    modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                                 CLASSIC_NIC_PKT_TYPE_IPV6_UDP);
                } else {
                    modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                                 CLASSIC_NIC_PKT_TYPE_IPV6);
                }
            }
            // dummy code to force PHV allocation
            modify_field(ipv6_2.srcAddr, 0);
            modify_field(ipv6_2.dstAddr, 0);
        }
        if (udp_2.valid == TRUE) {
            modify_field(key_metadata.sport, udp_2.srcPort);
            modify_field(key_metadata.dport, udp_2.dstPort);
        }
    } else {
        add_header(p4e_to_p4plus_classic_nic_ip);
        if (ipv4_1.valid == TRUE) {
            if (tcp.valid == TRUE) {
                modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                             CLASSIC_NIC_PKT_TYPE_IPV4_TCP);
            } else {
                if (udp_1.valid == TRUE) {
                    modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                                 CLASSIC_NIC_PKT_TYPE_IPV4_UDP);
                } else {
                    modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                                 CLASSIC_NIC_PKT_TYPE_IPV4);
                }
            }
        }
        if (ipv6_1.valid == TRUE) {
            if (tcp.valid == TRUE) {
                modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                             CLASSIC_NIC_PKT_TYPE_IPV6_TCP);
            } else {
                if (udp_1.valid == TRUE) {
                    modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                                 CLASSIC_NIC_PKT_TYPE_IPV6_UDP);
                } else {
                    modify_field(p4e_to_p4plus_classic_nic.pkt_type,
                                 CLASSIC_NIC_PKT_TYPE_IPV6);
                }
            }
        }
    }

    // checksum bits
    modify_field(scratch_metadata.flag,
                 (((capri_intrinsic.csum_err & (1 << CSUM_HDR_IPV4_1)) >> CSUM_HDR_IPV4_1) |
                  ((capri_intrinsic.csum_err & (1 << CSUM_HDR_IPV4_2)) >> CSUM_HDR_IPV4_2)));
    modify_field(p4e_to_p4plus_classic_nic.csum_ip_ok, ~scratch_metadata.flag);
    modify_field(p4e_to_p4plus_classic_nic.csum_ip_bad, scratch_metadata.flag);
    if (tcp.valid == TRUE) {
        modify_field(scratch_metadata.flag,
                     ((capri_intrinsic.csum_err & (1 << CSUM_HDR_TCP)) >> CSUM_HDR_TCP));
        modify_field(p4e_to_p4plus_classic_nic.csum_tcp_ok, ~scratch_metadata.flag);
        modify_field(p4e_to_p4plus_classic_nic.csum_tcp_bad, scratch_metadata.flag);
    }
    if (udp_1.valid == TRUE) {
        modify_field(scratch_metadata.flag,
                     ((capri_intrinsic.csum_err & (1 << CSUM_HDR_UDP_1)) >> CSUM_HDR_UDP_1));
    }
    if (udp_2.valid == TRUE) {
        bit_or(scratch_metadata.flag, scratch_metadata.flag,
               ((capri_intrinsic.csum_err & (1 << CSUM_HDR_UDP_2)) >> CSUM_HDR_UDP_2));
    }
    if ((udp_1.valid == TRUE) or (udp_2.valid == TRUE)) {
        modify_field(p4e_to_p4plus_classic_nic.csum_udp_ok, ~scratch_metadata.flag);
        modify_field(p4e_to_p4plus_classic_nic.csum_udp_bad, scratch_metadata.flag);
    }
}

action egress_recirc() {
    add_header(egress_recirc);
    modify_field(egress_recirc.p4_to_arm_valid, p4e_to_arm.valid);
    modify_field(capri_intrinsic.tm_span_session, 0);
    modify_field(capri_intrinsic.tm_oport, TM_PORT_EGRESS);
}

action p4e_inter_pipe() {
    remove_header(capri_txdma_intrinsic);
    if (capri_intrinsic.tm_oq != TM_P4_RECIRC_QUEUE) {
        modify_field(capri_intrinsic.tm_iq, capri_intrinsic.tm_oq);
    } else {
        modify_field(capri_intrinsic.tm_oq, capri_intrinsic.tm_iq);
    }

    if (egress_recirc.mapping_done == FALSE) {
        egress_recirc();
        //return
    }

    remove_header(p4e_i2e);
    remove_header(txdma_to_p4e);
    remove_header(egress_recirc);
    if (capri_intrinsic.tm_oport == TM_PORT_DMA) {
        egress_to_rxdma();
    }
}

@pragma stage 5
table p4e_inter_pipe {
    actions {
        p4e_inter_pipe;
    }
}

control egress_inter_pipe {
    if (capri_intrinsic.drop == 0) {
        apply(p4e_inter_pipe);
    } else {
        apply(p4e_drop_stats);
    }
}
