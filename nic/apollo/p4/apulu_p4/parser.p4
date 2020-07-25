/******************************************************************************
 * Capri Intrinsic header definitions                                         *
 *****************************************************************************/
header cap_phv_intr_global_t capri_intrinsic;
header cap_phv_intr_p4_t capri_p4_intrinsic;
header cap_phv_intr_rxdma_t capri_rxdma_intrinsic;
header cap_phv_intr_txdma_t capri_txdma_intrinsic;

/******************************************************************************
 * Headers                                                                    *
 *****************************************************************************/
@pragma synthetic_header
@pragma pa_field_union ingress p4i_i2e.vnic_id                      vnic_metadata.vnic_id
@pragma pa_field_union ingress p4i_i2e.bd_id                        vnic_metadata.bd_id
@pragma pa_field_union ingress p4i_i2e.vpc_id                       vnic_metadata.vpc_id
header apulu_i2e_metadata_t p4i_i2e;
header apulu_i2e_metadata_t p4e_i2e;

header apulu_ingress_recirc_header_t ingress_recirc;
header apulu_egress_recirc_header_t egress_recirc;

@pragma hdr_len parser_metadata.mirror_blob_len
header mirror_blob_t mirror_blob;

@pragma synthetic_header
@pragma pa_field_union ingress p4i_to_rxdma.flow_src                key_metadata.src
@pragma pa_field_union ingress p4i_to_rxdma.flow_dst                key_metadata.dst
@pragma pa_field_union ingress p4i_to_rxdma.flow_proto              key_metadata.proto
@pragma pa_field_union ingress p4i_to_rxdma.flow_sport              key_metadata.sport
@pragma pa_field_union ingress p4i_to_rxdma.flow_dport              key_metadata.dport
@pragma pa_field_union ingress p4i_to_rxdma.vpc_id                  vnic_metadata.vpc_id
header apulu_p4i_to_rxdma_header_t p4i_to_rxdma;
header apulu_txdma_to_p4e_header_t txdma_to_p4e;

@pragma synthetic_header
@pragma pa_field_union egress p4e_to_p4plus_classic_nic.l4_sport    key_metadata.sport
@pragma pa_field_union egress p4e_to_p4plus_classic_nic.l4_dport    key_metadata.dport
header p4_to_p4plus_classic_nic_header_t p4e_to_p4plus_classic_nic;
@pragma synthetic_header
@pragma pa_field_union egress p4e_to_p4plus_ipsec.seq_no            ipsec_metadata.seq_no
header p4_to_p4plus_ipsec_header_t p4e_to_p4plus_ipsec;
@pragma synthetic_header
@pragma pa_field_union egress p4e_to_p4plus_classic_nic_ip.ip_sa    ipv6_1.srcAddr
@pragma pa_field_union egress p4e_to_p4plus_classic_nic_ip.ip_da    ipv6_1.dstAddr
header p4_to_p4plus_ip_addr_t p4e_to_p4plus_classic_nic_ip;
@pragma pa_field_union egress p4e_to_p4plus_classic_nic_ip2.ip_sa   ipv6_2.srcAddr
@pragma pa_field_union egress p4e_to_p4plus_classic_nic_ip2.ip_da   ipv6_2.dstAddr
header p4_to_p4plus_ip_addr_t p4e_to_p4plus_classic_nic_ip2;

header p4plus_to_p4_s1_t p4plus_to_p4;
header p4plus_to_p4_s2_t p4plus_to_p4_vlan;

@pragma synthetic_header
@pragma pa_field_union ingress p4i_to_arm.l2_1_offset               offset_metadata.l2_1
@pragma pa_field_union ingress p4i_to_arm.l2_2_offset               offset_metadata.l2_2
@pragma pa_field_union ingress p4i_to_arm.l3_1_offset               offset_metadata.l3_1
@pragma pa_field_union ingress p4i_to_arm.l3_2_offset               offset_metadata.l3_2
@pragma pa_field_union ingress p4i_to_arm.l4_1_offset               offset_metadata.l4_1
@pragma pa_field_union ingress p4i_to_arm.l4_2_offset               offset_metadata.l4_2
@pragma pa_field_union ingress p4i_to_arm.tcp_flags                 tcp.flags
header apulu_p4_to_arm_header_t p4i_to_arm;

header apulu_p4_to_arm_header_t p4e_to_arm;
header apulu_arm_to_p4_header_t arm_to_p4i;
header p4plus_to_p4_header_ext_t p4plus_ext;

// layer 00
header ethernet_t ethernet_00;
header vlan_tag_t ctag_00;
@pragma pa_header_union egress ipv4_00 ipv6_00
header ipv4_t ipv4_00;
header ipv6_t ipv6_00;
header udp_t udp_00;
header mpls_t mpls_00;
header vxlan_t vxlan_00;

// layer 0
header ethernet_t ethernet_0;
header vlan_tag_t ctag_0;
@pragma pa_header_union egress ipv4_0 ipv6_0
header ipv4_t ipv4_0;
header ipv6_t ipv6_0;
header udp_t udp_0;
header vxlan_t vxlan_0;
header gre_t gre_0;
header gre_opt_seq_t gre_0_opt_seq;
@pragma pa_header_union egress erspan2 erspan3
header erspan_t2_t erspan2;
header erspan_t3_t erspan3;
header erspan_t3_opt_t erspan3_opt;

// layer 1
header ethernet_t ethernet_1;
header vlan_tag_t ctag_1;
header vlan_tag_t ctag2_1;
header vlan_tag_t ctag_s;
header arp_rarp_t arp;
@pragma pa_header_union ingress ipv4_1 ipv6_1
header ipv4_t ipv4_1;
header ipv6_t ipv6_1;
header udp_t udp_1;
header vxlan_t vxlan_1;
header geneve_t geneve;

// layer 2
header ethernet_t ethernet_2;
header vlan_tag_t ctag_2;
@pragma pa_header_union ingress ipv4_2 ipv6_2
header ipv4_t ipv4_2;
header ipv6_t ipv6_2;
header udp_t udp_2;

header tcp_t tcp;
header tcp_option_eol_t tcp_option_eol;
header tcp_option_nop_t tcp_option_nop;
header tcp_option_mss_t tcp_option_mss;
header tcp_option_ws_t tcp_option_ws;
header tcp_option_generic_t tcp_option_generic;
@pragma hdr_len parser_metadata.tcp_options_len
header tcp_options_blob_t tcp_option_blob;

header icmp_t icmp;
header icmp_echo_t icmp_echo;
header esp_t esp;

/******************************************************************************
 * Parser OHI                                                                 *
 *****************************************************************************/
header_type parser_ohi_t {
    fields {
        ipv4_1_len          : 16;
        ipv4_2_len          : 16;
        l4_1_len            : 16;
        l4_2_len            : 16;
        ipv4_1___start_off  : 16;
        ipv6_1___start_off  : 16;
        ipv4_2___start_off  : 16;
        ipv6_2___start_off  : 16;
        tcp___start_off     : 16;
        udp_1___start_off   : 16;
        udp_2___start_off   : 16;
        gso_start           : 16;
        gso_offset          : 16;
    }
}
@pragma parser_write_only
metadata parser_ohi_t ohi;

/******************************************************************************
 * Parser metadata                                                            *
 *****************************************************************************/
header_type parser_metadata_t {
    fields {
        mirror_blob_len : 8;
        tcp_options_len : 8;
    }
}
@pragma pa_parser_local
metadata parser_metadata_t parser_metadata;

/******************************************************************************
 * Parser start                                                               *
 *****************************************************************************/
parser start {
    extract(capri_intrinsic);
    return select(capri_intrinsic.tm_iport) {
        TM_PORT_INGRESS : parse_ingress_recirc_header;
        TM_PORT_DMA : parse_txdma_to_ingress;
        default : parse_uplink;
        0x1 mask 0 : deparse_ingress;
        0x2 mask 0 : egress_start;
    }
}

@pragma xgress ingress
parser parse_uplink {
    return parse_ingress_packet;
}

@pragma xgress ingress
parser parse_ingress_recirc_header {
    extract(capri_p4_intrinsic);
    extract(ingress_recirc);
    return parse_ingress_packet;
}

@pragma xgress ingress
parser parse_txdma_to_ingress {
    extract(capri_txdma_intrinsic);
    extract(p4plus_to_p4);
    extract(p4plus_to_p4_vlan);
    set_metadata(ohi.gso_start, p4plus_to_p4.gso_start + 0);
    set_metadata(ohi.gso_offset, p4plus_to_p4.gso_offset + 0);
    return select(p4plus_to_p4.gso_valid, p4plus_to_p4.p4plus_app_id) {
        0x10 mask 0x10 : parse_txdma_gso;
        P4PLUS_APPTYPE_CPU : parse_cpu_packet;
        P4PLUS_APPTYPE_IPSEC : parse_p4plus_ext_header;
        default : parse_ingress_packet;
    }
}

@pragma xgress ingress
@pragma generic_checksum_start capri_gso_csum.gso_checksum
parser parse_txdma_gso {
    return parse_ingress_packet;
}

@pragma xgress ingress
parser parse_cpu_packet {
    extract(arm_to_p4i);
    return parse_ingress_packet;
}

@pragma xgress ingress
parser parse_p4plus_ext_header {
    extract(p4plus_ext);
    return parse_ingress_packet;
}

/******************************************************************************
 * Layer 1                                                                    *
 *****************************************************************************/
parser parse_ingress_packet {
    extract(ethernet_1);
    set_metadata(offset_metadata.l2_1, current + 0);
    set_metadata(key_metadata.src, ethernet_1.srcAddr);
    set_metadata(key_metadata.dst, ethernet_1.dstAddr);
    return select(latest.etherType) {
        ETHERTYPE_CTAG : parse_ctag_1;
        ETHERTYPE_IPV4 : parse_ipv4_1;
        ETHERTYPE_IPV6 : parse_ipv6_1;
        ETHERTYPE_ARP  : parse_arp;
        default : ingress;
    }
}

parser parse_ctag_1 {
    extract(ctag_1);
    return select(latest.etherType) {
        ETHERTYPE_CTAG : parse_ctag2_1;
        ETHERTYPE_IPV4 : parse_ipv4_1;
        ETHERTYPE_IPV6 : parse_ipv6_1;
        ETHERTYPE_ARP  : parse_arp;
        default : ingress;
    }
}

parser parse_ctag2_1 {
    extract(ctag2_1);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4_1;
        ETHERTYPE_IPV6 : parse_ipv6_1;
        ETHERTYPE_ARP  : parse_arp;
        default : ingress;
    }
}

parser parse_arp {
    extract(arp);
    return ingress;
}

parser parse_ipv4_1 {
    set_metadata(offset_metadata.l3_1, current + 0);
    return parse_ipv4_1_split;
}

parser parse_ipv4_1_split {
    extract(ipv4_1);
    set_metadata(ohi.ipv4_1_len, ipv4_1.ihl << 2);
    set_metadata(ohi.l4_1_len, ipv4_1.totalLen - (ipv4_1.ihl << 2));
    return select(latest.flags, latest.fragOffset, latest.protocol) {
        IP_PROTO_ICMP mask 0x3fffff : parse_icmp;
        IP_PROTO_TCP mask 0x3fffff : parse_tcp;
        IP_PROTO_UDP mask 0x3fffff : parse_udp_1;
        IP_PROTO_IPSEC_ESP mask 0x3fffff : parse_ipsec_esp_1;
        0x0 mask 0x3fff00 : ingress;
        default : parse_ipv4_1_fragment;
    }
}

parser parse_ipv4_1_fragment {
    set_metadata(control_metadata.ip_fragment, 1);
    return ingress;
}


parser parse_ipv6_1 {
    extract(ipv6_1);
    set_metadata(offset_metadata.l3_1, current + 0);
    set_metadata(ohi.l4_1_len, ipv6_1.payloadLen + 0);
    return select(latest.nextHdr) {
        IP_PROTO_ICMPV6 : parse_icmp6;
        IP_PROTO_TCP : parse_tcp;
        IP_PROTO_UDP : parse_udp_1;
        default : ingress;
    }
}

parser parse_icmp {
    extract(icmp);
    set_metadata(offset_metadata.l4_2, current + 0);
    set_metadata(key_metadata.parsed_dport, latest.typeCode);
    return select(latest.typeCode) {
        ICMP_ECHO_REQ_TYPE_CODE : parse_icmp_echo;
        ICMP_ECHO_REPLY_TYPE_CODE : parse_icmp_echo;
        default : ingress;
    }
}

parser parse_icmp6 {
    extract(icmp);
    set_metadata(offset_metadata.l4_2, current + 0);
    set_metadata(key_metadata.parsed_dport, latest.typeCode);
    return select(latest.typeCode) {
        ICMP6_ECHO_REQ_TYPE_CODE : parse_icmp_echo;
        ICMP6_ECHO_REPLY_TYPE_CODE : parse_icmp_echo;
        default : ingress;
    }
}

parser parse_icmp_echo {
    extract(icmp_echo);
    set_metadata(key_metadata.parsed_sport, latest.identifier);
    return ingress;
}

parser parse_tcp {
    extract(tcp);
    set_metadata(offset_metadata.l4_2, current + 0);
    set_metadata(key_metadata.parsed_sport, latest.srcPort);
    set_metadata(key_metadata.parsed_dport, latest.dstPort);
    return ingress;
}

parser parse_udp_1 {
    extract(udp_1);
    set_metadata(offset_metadata.l4_1, current + 0);
    set_metadata(ohi.l4_1_len, udp_1.len + 0);
    return select(latest.dstPort) {
        UDP_PORT_GENV : parse_geneve;
        UDP_PORT_VXLAN : parse_vxlan_1;
        default : ingress;
    }
}

parser parse_ipsec_esp_1 {
    extract(esp);
    set_metadata(key_metadata.parsed_sport, latest.spi_hi);
    set_metadata(key_metadata.parsed_dport, latest.spi_lo);
    set_metadata(ipsec_metadata.seq_no, latest.seqNo);
    set_metadata(ipsec_metadata.ipsec_type, IPSEC_HEADER_ESP);
    return ingress;
}

parser parse_vxlan_1 {
    extract(vxlan_1);
    return parse_ethernet_2;
}

parser parse_geneve {
    extract(geneve);
    return parse_ethernet_2;
}

/******************************************************************************
 * Layer 2                                                                    *
 *****************************************************************************/
parser parse_ethernet_2 {
    extract(ethernet_2);
    set_metadata(offset_metadata.l2_2, current + 0);
    return select(latest.etherType) {
        ETHERTYPE_CTAG : parse_ctag_2;
        ETHERTYPE_IPV4 : parse_ipv4_2;
        ETHERTYPE_IPV6 : parse_ipv6_2;
        default : ingress;
    }
}

parser parse_ctag_2 {
    extract(ctag_2);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4_2;
        ETHERTYPE_IPV6 : parse_ipv6_2;
        default : ingress;
    }
}

parser parse_ipv4_2 {
    set_metadata(offset_metadata.l3_2, current + 0);
    return parse_ipv4_2_split;
}

parser parse_ipv4_2_split {
    extract(ipv4_2);
    set_metadata(ohi.ipv4_2_len, ipv4_2.ihl << 2);
    set_metadata(ohi.l4_2_len, ipv4_2.totalLen - (ipv4_2.ihl << 2));
    return select(latest.flags, latest.fragOffset, latest.protocol) {
        IP_PROTO_ICMP mask 0x3fffff : parse_icmp;
        IP_PROTO_TCP mask 0x3fffff : parse_tcp;
        IP_PROTO_UDP mask 0x3fffff : parse_udp_2;
        0x0 mask 0x3fff00 : ingress;
        default : parse_ipv4_2_fragment;
    }
}

parser parse_ipv4_2_fragment {
    set_metadata(control_metadata.ip2_fragment, 1);
    return ingress;
}

parser parse_ipv6_2 {
    extract(ipv6_2);
    set_metadata(offset_metadata.l3_2, current + 0);
    set_metadata(ohi.l4_2_len, ipv6_2.payloadLen + 0);
    return select(latest.nextHdr) {
        IP_PROTO_ICMPV6 : parse_icmp;
        IP_PROTO_TCP : parse_tcp;
        IP_PROTO_UDP : parse_udp_2;
        default : ingress;
    }
}

parser parse_udp_2 {
    extract(udp_2);
    set_metadata(offset_metadata.l4_2, current + 0);
    set_metadata(ohi.l4_2_len, udp_2.len + 0);
    set_metadata(key_metadata.parsed_sport, latest.srcPort);
    set_metadata(key_metadata.parsed_dport, latest.dstPort);
    return ingress;
}

/******************************************************************************/
/* Ingress deparser                                                           */
/******************************************************************************/
@pragma deparse_only
@pragma xgress ingress
parser deparse_ingress {
    extract(capri_intrinsic);
    extract(capri_p4_intrinsic);

    extract(capri_txdma_intrinsic);
    extract(p4plus_to_p4);
    extract(p4plus_to_p4_vlan);
    extract(arm_to_p4i);

    extract(capri_rxdma_intrinsic);

    extract(ingress_recirc);
    extract(p4i_to_rxdma);
    extract(p4i_to_arm);
    extract(p4i_i2e);

    return parse_ingress_packet;
}

/******************************************************************************/
/* Egress parser                                                              */
/******************************************************************************/
@pragma xgress egress
parser egress_start {
    extract(capri_intrinsic);
    extract(capri_p4_intrinsic);
    return select(capri_intrinsic.tm_iport) {
        TM_PORT_EGRESS  : parse_egress_to_egress;
        TM_PORT_DMA : parse_txdma_to_egress;
        default : parse_egress_common;
        0x1 mask 0 : deparse_egress;
    }
}

@pragma xgress egress
parser parse_egress_to_egress {
    return select(capri_intrinsic.tm_instance_type) {
        TM_INSTANCE_TYPE_SPAN : parse_egress_span_copy;
        default : parse_egress_recirc_header;
    }
}

@pragma xgress egress
parser parse_egress_recirc_header {
    extract(egress_recirc);
    return select(egress_recirc.p4_to_arm_valid) {
        1 : parse_txdma_to_arm;
        default : parse_egress;
    }
}

@pragma xgress egress
parser parse_txdma_to_egress {
    extract(capri_txdma_intrinsic);
    return parse_txdma_to_arm;
}

@pragma xgress egress
parser parse_txdma_to_arm {
    extract(txdma_to_p4e);
    return parse_txdma_to_arm_split;
}

@pragma xgress egress
parser parse_txdma_to_arm_split {
    extract(p4e_to_arm);
    return parse_egress;
}

@pragma xgress egress
parser parse_egress_common {
    return select(capri_intrinsic.tm_instance_type) {
        TM_INSTANCE_TYPE_SPAN : parse_ingress_span_copy;
        default : parse_egress;
    }
}

@pragma xgress egress
parser parse_egress {
    return parse_i2e_metadata;
}

@pragma xgress egress
@pragma allow_set_meta control_metadata.span_copy
parser parse_egress_span_copy {
    set_metadata(control_metadata.span_copy, 1);
    return parse_ethernet_span_copy;
}

@pragma xgress egress
@pragma allow_set_meta control_metadata.span_copy
parser parse_ingress_span_copy {
    set_metadata(control_metadata.span_copy, 1);
    set_metadata(parser_metadata.mirror_blob_len, APULU_INGRESS_MIRROR_BLOB_SZ);
    return parse_span_copy_blob;
}

@pragma xgress egress
parser parse_span_copy_blob {
    set_metadata(parser_metadata.mirror_blob_len,
                 parser_metadata.mirror_blob_len + 0);
    extract(mirror_blob);
    return parse_ethernet_span_copy;
}

@pragma xgress egress
@pragma allow_set_meta offset_metadata.l2_1
parser parse_ethernet_span_copy {
    extract(ethernet_1);
    set_metadata(offset_metadata.l2_1, current + 0);
    return select(latest.etherType) {
        ETHERTYPE_CTAG : parse_ctag_span_copy;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_ctag_span_copy {
    extract(ctag_s);
    return ingress;
}

@pragma xgress egress
parser parse_i2e_metadata {
    extract(p4e_i2e);
    return parse_egress_packet;
}

@pragma xgress egress
@pragma allow_set_meta offset_metadata.l2_1
parser parse_egress_packet {
    extract(ethernet_1);
    set_metadata(offset_metadata.l2_1, current + 0);
    return select(latest.etherType) {
        ETHERTYPE_CTAG : parse_egress_ctag_1;
        ETHERTYPE_IPV4 : parse_egress_ipv4_1;
        ETHERTYPE_IPV6 : parse_egress_ipv6_1;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_egress_ctag_1 {
    extract(ctag_1);
    return select(latest.etherType) {
        ETHERTYPE_CTAG : parse_egress_ctag2_1;
        ETHERTYPE_IPV4 : parse_egress_ipv4_1;
        ETHERTYPE_IPV6 : parse_egress_ipv6_1;
        default : ingress;
        1 mask 0 : parse_ethernet_span_copy;
    }
}

@pragma xgress egress
parser parse_egress_ctag2_1 {
    extract(ctag2_1);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_egress_ipv4_1;
        ETHERTYPE_IPV6 : parse_egress_ipv6_1;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_egress_ipv4_1 {
    extract(ipv4_1);
    set_metadata(ohi.ipv4_1_len, ipv4_1.ihl << 2);
    set_metadata(ohi.l4_1_len, ipv4_1.totalLen - (ipv4_1.ihl << 2));
    return select(latest.fragOffset, latest.protocol) {
        IP_PROTO_ICMP : parse_egress_icmp;
        IP_PROTO_TCP : parse_egress_tcp;
        IP_PROTO_UDP : parse_egress_udp;
        IP_PROTO_IPSEC_ESP : parse_egress_ipsec;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_egress_ipv6_1 {
    extract(ipv6_1);
    set_metadata(ohi.l4_1_len, ipv6_1.payloadLen + 0);
    return select(latest.nextHdr) {
        IP_PROTO_ICMPV6 : parse_egress_icmp;
        IP_PROTO_TCP : parse_egress_tcp;
        IP_PROTO_UDP : parse_egress_udp;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_egress_icmp {
    extract(icmp);
    return ingress;
}

@pragma xgress egress
@pragma allow_set_meta key_metadata.sport
@pragma allow_set_meta key_metadata.dport
parser parse_egress_tcp {
    extract(tcp);
    set_metadata(key_metadata.sport, latest.srcPort);
    set_metadata(key_metadata.dport, latest.dstPort);
    set_metadata(parser_metadata.tcp_options_len, (tcp.dataOffset << 2) - 20);
    return select(parser_metadata.tcp_options_len) {
        0 : ingress;
        0x80 mask 0x80 : parse_egress_tcp_option_error;
        default : parse_egress_tcp_option_blob;
    }
}

@pragma xgress egress
@pragma allow_set_meta control_metadata.tcp_option_error
parser parse_egress_tcp_option_error {
    set_metadata(control_metadata.parse_tcp_option_error, 1);
    return ingress;
}

@pragma xgress egress
@pragma dont_advance_packet
@pragma capture_payload_offset
parser parse_egress_tcp_option_blob {
    extract(tcp_option_blob);
    set_metadata(parser_metadata.tcp_options_len,
                 parser_metadata.tcp_options_len + 0);
    return parse_egress_tcp_options;
}

@pragma xgress egress
@pragma header_ordering tcp_option_generic tcp_option_mss tcp_option_ws tcp_option_nop tcp_option_eol
parser parse_egress_tcp_options {
    return select(parser_metadata.tcp_options_len, current(0, 8)) {
        0x0000 mask 0xff00 : ingress;
        0x8000 mask 0x8000 : parse_egress_tcp_option_error;
        0x0000 mask 0x00ff : parse_egress_tcp_option_eol;
        0x0001 mask 0x00ff : parse_egress_tcp_option_nop;
        0x0002 mask 0x00ff : parse_egress_tcp_option_mss;
        0x0003 mask 0x00ff : parse_egress_tcp_option_ws;
        default : parse_egress_tcp_option_generic;
    }
}

@pragma no_extract
@pragma xgress egress
parser parse_egress_tcp_option_eol {
    extract(tcp_option_eol);
    set_metadata(parser_metadata.tcp_options_len,
                 parser_metadata.tcp_options_len - 1);
    return parse_egress_tcp_options;
}

@pragma no_extract
@pragma xgress egress
parser parse_egress_tcp_option_nop {
    extract(tcp_option_nop);
    set_metadata(parser_metadata.tcp_options_len,
                 parser_metadata.tcp_options_len - 1);
    return parse_egress_tcp_options;
}

@pragma xgress egress
parser parse_egress_tcp_option_mss {
    extract(tcp_option_mss);
    set_metadata(parser_metadata.tcp_options_len,
                 parser_metadata.tcp_options_len - 4);
    return parse_egress_tcp_options;
}

@pragma xgress egress
parser parse_egress_tcp_option_ws {
    extract(tcp_option_ws);
    set_metadata(parser_metadata.tcp_options_len,
                 parser_metadata.tcp_options_len - 3);
    return parse_egress_tcp_options;
}

@pragma no_extract
@pragma xgress egress
parser parse_egress_tcp_option_generic {
    extract(tcp_option_generic);
    set_metadata(parser_metadata.tcp_options_len,
                 parser_metadata.tcp_options_len - tcp_option_generic.optLength);
    return parse_egress_tcp_options;
}

@pragma xgress egress
@pragma allow_set_meta key_metadata.sport
@pragma allow_set_meta key_metadata.dport
parser parse_egress_udp {
    extract(udp_1);
    set_metadata(ohi.l4_1_len, udp_1.len + 0);
    set_metadata(key_metadata.sport, latest.srcPort);
    set_metadata(key_metadata.dport, latest.dstPort);
    return select(latest.dstPort) {
        UDP_PORT_GENV : parse_egress_geneve;
        default : ingress;
    }
}

@pragma xgress egress
@pragma allow_set_meta key_metadata.parsed_sport
@pragma allow_set_meta key_metadata.parsed_dport
@pragma allow_set_meta ipsec_metadata.seq_no
@pragma allow_set_meta ipsec_metadata.ipsec_type
parser parse_egress_ipsec {
    extract(esp);
    set_metadata(key_metadata.parsed_sport, latest.spi_hi);
    set_metadata(key_metadata.parsed_dport, latest.spi_lo);
    set_metadata(ipsec_metadata.seq_no, latest.seqNo);
    set_metadata(ipsec_metadata.ipsec_type, IPSEC_HEADER_ESP);
    return ingress;
}

@pragma xgress egress
parser parse_egress_geneve {
    extract(geneve);
    return parse_egress_ethernet_2;
}

@pragma xgress egress
parser parse_egress_ethernet_2 {
    extract(ethernet_2);
    return select(latest.etherType) {
        ETHERTYPE_CTAG : parse_egress_ctag_2;
        ETHERTYPE_IPV4 : parse_egress_ipv4_2;
        ETHERTYPE_IPV6 : parse_egress_ipv6_2;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_egress_ctag_2 {
    extract(ctag_2);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_egress_ipv4_2;
        ETHERTYPE_IPV6 : parse_egress_ipv6_2;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_egress_ipv4_2 {
    extract(ipv4_2);
    set_metadata(ohi.ipv4_2_len, ipv4_2.ihl << 2);
    set_metadata(ohi.l4_2_len, ipv4_2.totalLen - (ipv4_2.ihl << 2));
    return select(latest.fragOffset, latest.protocol) {
        IP_PROTO_ICMP : parse_egress_icmp;
        IP_PROTO_TCP : parse_egress_tcp;
        IP_PROTO_UDP : parse_egress_udp_2;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_egress_ipv6_2 {
    extract(ipv6_2);
    set_metadata(ohi.l4_2_len, ipv6_2.payloadLen + 0);
    return select(latest.nextHdr) {
        IP_PROTO_ICMPV6 : parse_egress_icmp;
        IP_PROTO_TCP : parse_egress_tcp;
        IP_PROTO_UDP : parse_egress_udp_2;
        default : ingress;
    }
}

@pragma xgress egress
parser parse_egress_udp_2 {
    extract(udp_2);
    set_metadata(ohi.l4_2_len, udp_2.len + 0);
    return ingress;
}

/******************************************************************************
 * Egress deparser                                                            *
 *****************************************************************************/
@pragma deparse_only
@pragma xgress egress
parser deparse_egress {
    // intrinsic headers
    extract(capri_intrinsic);
    extract(capri_p4_intrinsic);
    extract(capri_rxdma_intrinsic);

    // packet to classic NIC application
    extract(p4e_to_p4plus_classic_nic);
    extract(p4e_to_p4plus_classic_nic_ip);
    extract(p4e_to_p4plus_classic_nic_ip2);
    extract(p4e_to_p4plus_ipsec);

    // egress-to-egress recirc
    extract(egress_recirc);

    extract(txdma_to_p4e);
    extract(p4e_to_arm);
    extract(p4e_i2e);

    // layer 00
    extract(ethernet_00);
    extract(ctag_00);
    extract(ipv4_00);
    extract(ipv6_00);
    extract(udp_00);
    extract(mpls_00);
    extract(vxlan_00);

    // layer 0
    extract(ethernet_0);
    extract(ctag_0);
    extract(ipv4_0);
    extract(ipv6_0);
    extract(udp_0);
    extract(vxlan_0);
    extract(gre_0);
    extract(gre_0_opt_seq);
    extract(erspan2);
    extract(erspan3);
    extract(erspan3_opt);

    return parse_egress_packet;
}

/******************************************************************************
 * Checksums : Layer 00 (compute only, no verification)                       *
 *****************************************************************************/
field_list ipv4_00_checksum_list {
    ipv4_00.version;
    ipv4_00.ihl;
    ipv4_00.diffserv;
    ipv4_00.totalLen;
    ipv4_00.identification;
    ipv4_00.flags;
    ipv4_00.fragOffset;
    ipv4_00.ttl;
    ipv4_00.protocol;
    ipv4_00.srcAddr;
    ipv4_00.dstAddr;
}

@pragma checksum update egress
@pragma checksum update_len capri_deparser_len.ipv4_00_hdr_len
field_list_calculation ipv4_00_checksum {
    input {
        ipv4_00_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4_00.hdrChecksum  {
    update ipv4_00_checksum;
}

/******************************************************************************
 * Checksums : Layer 0 (compute only, no verification)                        *
 *****************************************************************************/
field_list ipv4_0_checksum_list {
    ipv4_0.version;
    ipv4_0.ihl;
    ipv4_0.diffserv;
    ipv4_0.totalLen;
    ipv4_0.identification;
    ipv4_0.flags;
    ipv4_0.fragOffset;
    ipv4_0.ttl;
    ipv4_0.protocol;
    ipv4_0.srcAddr;
    ipv4_0.dstAddr;
}

@pragma checksum update egress
@pragma checksum update_len capri_deparser_len.ipv4_0_hdr_len
field_list_calculation ipv4_0_checksum {
    input {
        ipv4_0_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4_0.hdrChecksum  {
    update ipv4_0_checksum;
}

/******************************************************************************
 * Checksums : Layer 1                                                        *
 *****************************************************************************/
field_list ipv4_1_checksum_list {
    ipv4_1.version;
    ipv4_1.ihl;
    ipv4_1.diffserv;
    ipv4_1.totalLen;
    ipv4_1.identification;
    ipv4_1.flags;
    ipv4_1.fragOffset;
    ipv4_1.ttl;
    ipv4_1.protocol;
    ipv4_1.srcAddr;
    ipv4_1.dstAddr;
}

@pragma checksum verify xgress
@pragma checksum update egress
@pragma checksum verify_len ohi.l4_1_len
@pragma checksum hdr_len_expr ohi.ipv4_1_len + 0
@pragma checksum update_len capri_deparser_len.ipv4_1_hdr_len
field_list_calculation ipv4_1_checksum {
    input {
        ipv4_1_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4_1.hdrChecksum  {
    verify ipv4_1_checksum;
    update ipv4_1_checksum;
}

field_list ipv4_1_tcp_checksum_list {
    ipv4_1.srcAddr;
    ipv4_1.dstAddr;
    8'0;
    ipv4_1.protocol;
    capri_deparser_len.l4_payload_len;
    tcp.srcPort;
    tcp.dstPort;
    tcp.seqNo;
    tcp.ackNo;
    tcp.dataOffset;
    tcp.res;
    tcp.flags;
    tcp.window;
    tcp.urgentPtr;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update egress
@pragma checksum verify_len ohi.l4_1_len
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share tcp.checksum udp_1.checksum icmp.hdrChecksum
field_list_calculation ipv4_1_tcp_checksum {
    input {
        ipv4_1_tcp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

field_list ipv6_1_tcp_checksum_list {
    ipv6_1.srcAddr;
    ipv6_1.dstAddr;
    8'0;
    ipv6_1.nextHdr;
    capri_deparser_len.l4_payload_len;
    tcp.srcPort;
    tcp.dstPort;
    tcp.seqNo;
    tcp.ackNo;
    tcp.dataOffset;
    tcp.res;
    tcp.flags;
    tcp.window;
    tcp.urgentPtr;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update egress
@pragma checksum verify_len ohi.l4_1_len
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share tcp.checksum udp_1.checksum icmp.hdrChecksum
field_list_calculation ipv6_1_tcp_checksum {
    input {
        ipv6_1_tcp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field tcp.checksum {
    verify ipv4_1_tcp_checksum;
    verify ipv6_1_tcp_checksum;
    update ipv4_1_tcp_checksum;
    update ipv6_1_tcp_checksum;
}

field_list ipv4_1_udp_checksum_list {
    ipv4_1.srcAddr;
    ipv4_1.dstAddr;
    8'0;
    ipv4_1.protocol;
    udp_1.len;
    udp_1.srcPort;
    udp_1.dstPort;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update egress
@pragma checksum verify_len ohi.l4_1_len
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share udp_1.checksum tcp.checksum icmp.hdrChecksum
field_list_calculation ipv4_1_udp_checksum {
    input {
        ipv4_1_udp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

field_list ipv6_1_udp_checksum_list {
    ipv6_1.srcAddr;
    ipv6_1.dstAddr;
    8'0;
    ipv6_1.nextHdr;
    udp_1.srcPort;
    udp_1.dstPort;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update egress
@pragma checksum verify_len ohi.l4_1_len
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share udp_1.checksum tcp.checksum icmp.hdrChecksum
field_list_calculation ipv6_1_udp_checksum {
    input {
        ipv6_1_udp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field udp_1.checksum {
    verify ipv4_1_udp_checksum;
    verify ipv6_1_udp_checksum;
    update ipv4_1_udp_checksum;
    update ipv6_1_udp_checksum;
}

field_list ipv4_1_icmp_checksum_list {
    payload;
}

@pragma checksum verify xgress
@pragma checksum update egress
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share icmp.hdrChecksum tcp.checksum udp_1.checksum
field_list_calculation ipv4_1_icmp_checksum {
    input {
        ipv4_1_icmp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

field_list ipv6_1_icmp_checksum_list {
    ipv6_1.srcAddr;
    ipv6_1.dstAddr;
    ipv6_1.nextHdr;
    ipv6_1.payloadLen;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update egress
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share icmp.hdrChecksum tcp.checksum udp_1.checksum
field_list_calculation ipv6_1_icmp_checksum {
    input {
        ipv6_1_icmp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}


calculated_field icmp.hdrChecksum {
    update ipv4_1_icmp_checksum;
    update ipv6_1_icmp_checksum;
}

/******************************************************************************
 * Checksums : Layer 2 (verify in ingress and egress, update in ingress)      *
 *****************************************************************************/
field_list ipv4_2_checksum_list {
    ipv4_2.version;
    ipv4_2.ihl;
    ipv4_2.diffserv;
    ipv4_2.totalLen;
    ipv4_2.identification;
    ipv4_2.flags;
    ipv4_2.fragOffset;
    ipv4_2.ttl;
    ipv4_2.protocol;
    ipv4_2.srcAddr;
    ipv4_2.dstAddr;
}

@pragma checksum verify xgress
@pragma checksum update ingress
@pragma checksum verify_len ohi.l4_2_len
@pragma checksum hdr_len_expr ohi.ipv4_2_len + 0
@pragma checksum update_len capri_deparser_len.ipv4_2_hdr_len
field_list_calculation ipv4_2_checksum {
    input {
        ipv4_2_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4_2.hdrChecksum  {
    verify ipv4_2_checksum;
    update ipv4_2_checksum;
}

field_list ipv4_2_tcp_checksum_list {
    ipv4_2.srcAddr;
    ipv4_2.dstAddr;
    8'0;
    ipv4_2.protocol;
    capri_deparser_len.l4_payload_len;
    tcp.srcPort;
    tcp.dstPort;
    tcp.seqNo;
    tcp.ackNo;
    tcp.dataOffset;
    tcp.res;
    tcp.flags;
    tcp.window;
    tcp.urgentPtr;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update ingress
@pragma checksum verify_len ohi.l4_2_len
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share tcp.checksum udp_2.checksum icmp.hdrChecksum
field_list_calculation ipv4_2_tcp_checksum {
    input {
        ipv4_2_tcp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

field_list ipv6_2_tcp_checksum_list {
    ipv6_2.srcAddr;
    ipv6_2.dstAddr;
    8'0;
    ipv6_2.nextHdr;
    capri_deparser_len.l4_payload_len;
    tcp.srcPort;
    tcp.dstPort;
    tcp.seqNo;
    tcp.ackNo;
    tcp.dataOffset;
    tcp.res;
    tcp.flags;
    tcp.window;
    tcp.urgentPtr;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update ingress
@pragma checksum verify_len ohi.l4_2_len
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share tcp.checksum udp_2.checksum icmp.hdrChecksum
field_list_calculation ipv6_2_tcp_checksum {
    input {
        ipv6_2_tcp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field tcp.checksum {
    verify ipv4_2_tcp_checksum;
    verify ipv6_2_tcp_checksum;
    update ipv4_2_tcp_checksum;
    update ipv6_2_tcp_checksum;
}

field_list ipv4_2_udp_checksum_list {
    ipv4_2.srcAddr;
    ipv4_2.dstAddr;
    8'0;
    ipv4_2.protocol;
    udp_2.len;
    udp_2.srcPort;
    udp_2.dstPort;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update ingress
@pragma checksum verify_len ohi.l4_2_len
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share udp_2.checksum tcp.checksum icmp.hdrChecksum
field_list_calculation ipv4_2_udp_checksum {
    input {
        ipv4_2_udp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

field_list ipv6_2_udp_checksum_list {
    ipv6_2.srcAddr;
    ipv6_2.dstAddr;
    8'0;
    ipv6_2.nextHdr;
    udp_2.srcPort;
    udp_2.dstPort;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update ingress
@pragma checksum verify_len ohi.l4_2_len
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share udp_2.checksum tcp.checksum icmp.hdrChecksum
field_list_calculation ipv6_2_udp_checksum {
    input {
        ipv6_2_udp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field udp_2.checksum {
    verify ipv4_2_udp_checksum;
    verify ipv6_2_udp_checksum;
    update ipv4_2_udp_checksum;
    update ipv6_2_udp_checksum;
}

field_list ipv4_2_icmp_checksum_list {
    payload;
}

@pragma checksum verify xgress
@pragma checksum update igress
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share icmp.hdrchecksum tcp.checksum udp_2.checksum
field_list_calculation ipv4_2_icmp_checksum {
    input {
        ipv4_2_icmp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

field_list ipv6_2_icmp_checksum_list {
    ipv6_2.srcAddr;
    ipv6_2.dstAddr;
    ipv6_2.nextHdr;
    ipv6_2.payloadLen;
    payload;
}

@pragma checksum verify xgress
@pragma checksum update igress
@pragma checksum update_len capri_deparser_len.l4_payload_len
@pragma checksum update_share icmp.hdrchecksum tcp.checksum udp_2.checksum
field_list_calculation ipv6_2_icmp_checksum {
    input {
        ipv6_2_icmp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field icmp.hdrChecksum {
    update ipv4_2_icmp_checksum;
    update ipv6_2_icmp_checksum;
}

/******************************************************************************
 * Checksums : L2 Complete Checksum                                           *
 *****************************************************************************/
field_list l2_checksum_ipv4_list {
    ipv4_1.srcAddr;
    payload;
}

field_list l2_checksum_ipv6_list {
    ipv6_1.srcAddr;
    payload;
}

@pragma checksum update_len capri_deparser_len.l2_checksum_len
field_list_calculation l2_checksum_ipv4 {
    input {
        l2_checksum_ipv4_list;
    }
    algorithm : l2_complete_csum;
    output_width : 16;
}

@pragma checksum update_len capri_deparser_len.l2_checksum_len
field_list_calculation l2_checksum_ipv6 {
    input {
        l2_checksum_ipv6_list;
    }
    algorithm : l2_complete_csum;
    output_width : 16;
}

field_list l2_checksum_vlan {
    ctag_1.pcp;
    payload;
}

@pragma checksum update_len capri_deparser_len.l2_checksum_len
field_list_calculation l2_checksum_vlan {
    input {
        l2_checksum_vlan;
    }
    algorithm : l2_complete_csum;
    output_width : 16;
}

calculated_field p4e_to_p4plus_classic_nic.csum {
    update l2_checksum_vlan if (valid(ctag_1));
    update l2_checksum_ipv4 if (valid(ipv4_1));
    update l2_checksum_ipv6 if (valid(ipv6_1));
}

/******************************************************************************
 * Checksums : Partial checksum                                               *
 *****************************************************************************/
field_list gso_checksum_list {
    p4plus_to_p4.gso_start;
    payload;
}

@pragma checksum update_len capri_gso_csum.gso_checksum
@pragma checksum gso_checksum_offset p4plus_to_p4.gso_offset
@pragma checksum gress ingress
field_list_calculation gso_checksum {
    input {
        gso_checksum_list;
    }
    algorithm : gso;
    output_width : 16;
}

calculated_field capri_gso_csum.gso_checksum {
    update gso_checksum;
}
