#include "apulu.h"
#include "egress.h"
#include "EGRESS_p.h"
#include "EGRESS_nexthop_k.h"

struct nexthop_k_   k;
struct nexthop_d    d;
struct phv_         p;

%%

// r1 : packet length
// r2 : k.rewrite_metadata_flags
// c7 : tunnel_tos_override

// thread 0 : perform normal rewrites
nexthop_info:
    bne             r5, r0, nexthop2
    seq             c1, k.p4e_i2e_nexthop_id, r0
    seq.!c1         c1, d.nexthop_info_d.drop, TRUE
    bcf             [c1], nexthop_invalid
    sne             c1, d.nexthop_info_d.tunnel2_id, r0
    sne.c1          c1, k.control_metadata_erspan_copy, TRUE
    bcf             [!c1], nexthop_info2
    add             r1, k.capri_p4_intrinsic_packet_len_s6_e13, \
                        k.capri_p4_intrinsic_packet_len_s0_e5, 8

    // second tunnel info
    phvwr           p.control_metadata_apply_tunnel2, TRUE
    phvwr           p.rewrite_metadata_tunnel2_id, d.nexthop_info_d.tunnel2_id
    phvwr           p.rewrite_metadata_tunnel2_vni, d.nexthop_info_d.vlan

nexthop_info2:
    seq             c1, d.nexthop_info_d.port, TM_PORT_DMA
    bcf             [!c1], nexthop_rewrite
    phvwr           p.capri_intrinsic_tm_oport, d.nexthop_info_d.port
    phvwr           p.capri_intrinsic_lif, d.nexthop_info_d.lif
    phvwr           p.control_metadata_p4plus_app_id, d.nexthop_info_d.app_id
    phvwr           p.capri_rxdma_intrinsic_qtype, d.nexthop_info_d.qtype
    seq             c1, k.p4e_to_p4plus_classic_nic_rss_override, FALSE
    phvwr.c1        p.capri_rxdma_intrinsic_qid, d.nexthop_info_d.qid
    phvwr           p.vnic_metadata_rx_vnic_stats_id, \
                        d.nexthop_info_d.rx_vnic_id
    phvwr           p.rewrite_metadata_vlan_strip_en, \
                        d.nexthop_info_d.vlan_strip_en

nexthop_rewrite:
    sne             c1, r0, d.nexthop_info_d.rewrite_flags
    cmov            r2, c1, d.nexthop_info_d.rewrite_flags, \
                        k.rewrite_metadata_flags
    seq             c1, r2[P4_REWRITE_DMAC_BITS], P4_REWRITE_DMAC_FROM_MAPPING
    phvwr.c1        p.ethernet_1_dstAddr, k.rewrite_metadata_dmaci
    seq             c1, r2[P4_REWRITE_DMAC_BITS], P4_REWRITE_DMAC_FROM_NEXTHOP
    phvwr.c1        p.ethernet_1_dstAddr, d.nexthop_info_d.dmaci
    seq             c1, r2[P4_REWRITE_DMAC_BITS], P4_REWRITE_DMAC_FROM_TUNNEL
    phvwrpair.c1    p.ethernet_1_dstAddr[47:8], \
                        k.rewrite_metadata_tunnel_dmaci_s0_e39, \
                        p.ethernet_1_dstAddr[7:0], \
                        k.rewrite_metadata_tunnel_dmaci_s40_e47
    seq             c1, r2[P4_REWRITE_SMAC_BITS], P4_REWRITE_SMAC_FROM_VRMAC
    phvwr.c1        p.ethernet_1_srcAddr, k.rewrite_metadata_vrmac
    seq             c1, r2[P4_REWRITE_SMAC_BITS], P4_REWRITE_SMAC_FROM_NEXTHOP
    phvwr.c1        p.ethernet_1_srcAddr, d.nexthop_info_d.smaco
    seq             c1, r2[P4_REWRITE_VLAN_BITS], P4_REWRITE_VLAN_ENCAP
    bcf             [c1], vlan_encap
    seq             c1, k.ctag_1_valid, TRUE
    seq.c1          c1, r2[P4_REWRITE_VLAN_BITS], P4_REWRITE_VLAN_DECAP
    nop.!c1.e
vlan_decap:
    sub             r1, r1, 4
    phvwr           p.capri_p4_intrinsic_packet_len, r1
    phvwr.e         p.ctag_1_valid, FALSE
    phvwr.f         p.ethernet_1_etherType, k.ctag_1_etherType

vlan_encap:
    bbeq            k.control_metadata_erspan_copy, TRUE, vlan_encap2
    nop
    nop.c1.e
    phvwr           p.{ctag_1_pcp,ctag_1_dei,ctag_1_vid}, d.nexthop_info_d.vlan
    phvwr           p.ctag_1_valid, TRUE
    phvwr           p.ctag_1_etherType, k.ethernet_1_etherType
    phvwr           p.ethernet_1_etherType, ETHERTYPE_VLAN
    add.e           r1, r1, 4
    phvwr.f         p.capri_p4_intrinsic_packet_len, r1

vlan_encap2:
    phvwr           p.{ctag_0_pcp,ctag_0_dei,ctag_0_vid}, d.nexthop_info_d.vlan
    phvwr           p.ctag_0_valid, TRUE
    phvwr           p.ctag_0_etherType, k.ethernet_0_etherType
    phvwr           p.ethernet_0_etherType, ETHERTYPE_VLAN
    add.e           r1, r1, 4
    phvwr.f         p.capri_p4_intrinsic_packet_len, r1

// thread 1 : perform vxlan rewrite
nexthop2:
    seq.!c1         c1, d.nexthop_info_d.drop, TRUE
    nop.c1.e
    seq             c7, k.rewrite_metadata_tunnel_tos_override, TRUE
    phvwr           p.{ethernet_00_dstAddr,ethernet_00_srcAddr}, \
                        d.{nexthop_info_d.dmaco,nexthop_info_d.smaco}
    phvwr           p.rewrite_metadata_tunnel_tos, \
                        k.rewrite_metadata_tunnel_tos2
    sne             c1, r0, d.nexthop_info_d.rewrite_flags
    cmov            r2, c1, d.nexthop_info_d.rewrite_flags, \
                        k.rewrite_metadata_flags
    bbeq            k.control_metadata_erspan_copy, TRUE, nexthop_erspan_copy
    add             r1, k.capri_p4_intrinsic_packet_len_s6_e13, \
                        k.capri_p4_intrinsic_packet_len_s0_e5, 8
    seq             c1, r2[P4_REWRITE_ENCAP_BITS], P4_REWRITE_ENCAP_VXLAN
    nop.!c1.e
vxlan_encap:
    seq             c1, k.ctag_1_valid, TRUE
    sub.c1          r1, r1, 4
    phvwr.c1        p.ethernet_1_etherType, k.ctag_1_etherType
    phvwr           p.{ethernet_0_dstAddr,ethernet_0_srcAddr}, \
                        d.{nexthop_info_d.dmaco,nexthop_info_d.smaco}
    seq             c1, r2[P4_REWRITE_VNI_BITS], P4_REWRITE_VNI_FROM_TUNNEL
    add             r6, r0, k.rewrite_metadata_tunnel_vni
    sne.!c1         c1, r6, r0
    cmov            r7, c1, r6, k.rewrite_metadata_vni
    or              r7, r7, 0x8, 48
    or              r7, r0, r7, 8
    bbeq            k.rewrite_metadata_ip_type, IPTYPE_IPV6, ipv6_vxlan_encap
    phvwr           p.{vxlan_0_flags,vxlan_0_reserved,vxlan_0_vni, \
                        vxlan_0_reserved2}, r7
ipv4_vxlan_encap:
    /*
    phvwr           p.vxlan_0_valid, 1
    phvwr           p.udp_0_valid, 1
    phvwr           p.ipv4_0_valid, 1
    phvwr           p.ipv4_0_csum, 1
    phvwr           p.ethernet_0_valid, 1
    bitmap ==> 1 1010 0101
    */
    phvwrpair       p.ctag_1_valid, 0, \
                      p.{vxlan_0_valid, \
                        udp_0_valid, \
                        ipv6_0_valid, \
                        ipv4_0_valid, \
                        ipv4_0_udp_csum, \
                        ipv4_0_tcp_csum, \
                        ipv4_0_csum, \
                        ctag_0_valid, \
                        ethernet_0_valid}, 0x1A5
    phvwr           p.capri_deparser_len_ipv4_0_hdr_len, 20
    phvwr           p.ethernet_0_etherType, ETHERTYPE_IPV4
    add             r1, r1, 36
    add.!c7         r7, k.rewrite_metadata_tunnel_tos, 0x45, 8
    add.c7          r7, k.rewrite_metadata_tunnel_tos2, 0x45, 8
    add             r7, r1[15:0], r7, 16
    phvwr           p.{ipv4_0_version,ipv4_0_ihl,ipv4_0_diffserv,ipv4_0_totalLen}, r7
    or              r7, IP_PROTO_UDP, 64, 8
    phvwrpair       p.{ipv4_0_ttl,ipv4_0_protocol}, r7, \
                        p.ipv4_0_srcAddr, k.rewrite_metadata_device_ipv4_addr
    sub             r1, r1, 20
    or              r7, k.p4e_i2e_entropy_hash, 0xC000
    or              r7, UDP_PORT_VXLAN, r7, 16
    or              r7, r1[15:0], r7, 16
    phvwr           p.{udp_0_srcPort,udp_0_dstPort,udp_0_len}, r7
    add.e           r1, r1, (20+14)
    phvwr.f         p.capri_p4_intrinsic_packet_len, r1

ipv6_vxlan_encap:
    /*
    phvwr           p.vxlan_0_valid, 1
    phvwr           p.udp_0_valid, 1
    phvwr           p.ipv6_0_valid, 1
    phvwr           p.ethernet_0_valid, 1
    bitmap ==> 1 1100 0001
    */
    phvwrpair       p.ctag_1_valid, 0, \
                      p.{vxlan_0_valid, \
                        udp_0_valid, \
                        ipv6_0_valid, \
                        ipv4_0_valid, \
                        ipv4_0_udp_csum, \
                        ipv4_0_tcp_csum, \
                        ipv4_0_csum, \
                        ctag_0_valid, \
                        ethernet_0_valid}, 0x1C1
    phvwr           p.ethernet_0_etherType, ETHERTYPE_IPV6
    add             r1, r1, 16
    add.!c7         r7, k.rewrite_metadata_tunnel_tos, 0x6, 8
    add.c7          r7, k.rewrite_metadata_tunnel_tos2, 0x6, 8
    phvwr           p.{ipv6_0_version,ipv6_0_trafficClass}, r7
    phvwr           p.ipv6_0_srcAddr, k.rewrite_metadata_device_ipv6_addr
    phvwr           p.{ipv6_0_nextHdr,ipv6_0_hopLimit}, (IP_PROTO_UDP << 8) | 64
    phvwr           p.ipv6_0_payloadLen, r1
    or              r7, k.p4e_i2e_entropy_hash, 0xC000
    or              r7, UDP_PORT_VXLAN, r7, 16
    phvwr           p.{udp_0_srcPort,udp_0_dstPort}, r7
    phvwr           p.udp_0_len, r1
    add.e           r1, r1, (40+14)
    phvwr.f         p.capri_p4_intrinsic_packet_len, r1

nexthop_erspan_copy:
    seq             c1, r2[P4_REWRITE_DMAC_BITS], P4_REWRITE_DMAC_FROM_NEXTHOP
    phvwr.c1        p.{ethernet_0_dstAddr,ethernet_0_srcAddr}, \
                        d.{nexthop_info_d.dmaco,nexthop_info_d.smaco}
    seq             c1, r2[P4_REWRITE_ENCAP_BITS], P4_REWRITE_ENCAP_VXLAN
    nop.!c1.e
vxlan_encap2:
    seq             c1, r2[P4_REWRITE_VNI_BITS], P4_REWRITE_VNI_FROM_TUNNEL
    add             r6, r0, k.rewrite_metadata_tunnel_vni
    sne.!c1         c1, r6, r0
    cmov            r7, c1, r6, k.rewrite_metadata_vni
    or              r7, r7, 0x8, 48
    or              r7, r0, r7, 8
    bbeq            k.rewrite_metadata_ip_type, IPTYPE_IPV6, ipv6_vxlan_encap2
    phvwr           p.{vxlan_00_flags,vxlan_00_reserved,vxlan_00_vni, \
                        vxlan_00_reserved2}, r7
ipv4_vxlan_encap2:
    /*
    phvwr           p.vxlan_00_valid, 1
    phvwr           p.udp_00_valid, 1
    phvwr           p.ipv4_00_valid, 1
    phvwr           p.ipv4_00_csum, 1
    phvwr           p.ethernet_00_valid, 1
    bitmap ==> 10 1010 0101
    */
    phvwr           p.{vxlan_00_valid, \
                        mpls_00_valid, \
                        udp_00_valid, \
                        ipv6_00_valid, \
                        ipv4_00_valid, \
                        ipv4_00_udp_csum, \
                        ipv4_00_tcp_csum, \
                        ipv4_00_csum, \
                        ctag_00_valid, \
                        ethernet_00_valid}, 0x2A5
    phvwr           p.capri_deparser_len_ipv4_00_hdr_len, 20
    phvwr           p.ethernet_00_etherType, ETHERTYPE_IPV4
    add             r1, r1, 36
    add.!c7         r7, k.rewrite_metadata_tunnel_tos, 0x45, 8
    add.c7          r7, k.rewrite_metadata_tunnel_tos2, 0x45, 8
    add             r7, r1[15:0], r7, 16
    phvwr           p.{ipv4_00_version,ipv4_00_ihl,ipv4_00_diffserv,\
                        ipv4_00_totalLen}, r7
    or              r7, IP_PROTO_UDP, 64, 8
    phvwrpair       p.{ipv4_00_ttl,ipv4_00_protocol}, r7, \
                        p.ipv4_00_srcAddr, k.rewrite_metadata_device_ipv4_addr
    sub             r1, r1, 20
    or              r7, k.p4e_i2e_entropy_hash, 0xC000
    or              r7, UDP_PORT_VXLAN, r7, 16
    or              r7, r1[15:0], r7, 16
    phvwr           p.{udp_00_srcPort,udp_00_dstPort,udp_00_len}, r7
    add.e           r1, r1, (20+14)
    phvwr.f         p.capri_p4_intrinsic_packet_len, r1

ipv6_vxlan_encap2:
    /*
    phvwr           p.vxlan_00_valid, 1
    phvwr           p.udp_00_valid, 1
    phvwr           p.ipv6_00_valid, 1
    phvwr           p.ethernet_00_valid, 1
    bitmap ==> 10 1100 0001
    */
    phvwr           p.{vxlan_00_valid, \
                        mpls_00_valid, \
                        udp_00_valid, \
                        ipv6_00_valid, \
                        ipv4_00_valid, \
                        ipv4_00_udp_csum, \
                        ipv4_00_tcp_csum, \
                        ipv4_00_csum, \
                        ctag_00_valid, \
                        ethernet_00_valid}, 0x2C1
    phvwr           p.ethernet_00_etherType, ETHERTYPE_IPV6
    add             r1, r1, 16
    add.!c7         r7, k.rewrite_metadata_tunnel_tos, 0x6, 8
    add.c7          r7, k.rewrite_metadata_tunnel_tos2, 0x6, 8
    phvwr           p.{ipv6_00_version,ipv6_00_trafficClass}, r7
    phvwr           p.ipv6_00_srcAddr, k.rewrite_metadata_device_ipv6_addr
    phvwr           p.{ipv6_00_nextHdr,ipv6_00_hopLimit}, \
                        (IP_PROTO_UDP << 8) | 64
    phvwr           p.ipv6_00_payloadLen, r1
    or              r7, k.p4e_i2e_entropy_hash, 0xC000
    or              r7, UDP_PORT_VXLAN, r7, 16
    phvwr           p.{udp_00_srcPort,udp_00_dstPort}, r7
    phvwr           p.udp_00_len, r1
    add.e           r1, r1, (40+14)
    phvwr.f         p.capri_p4_intrinsic_packet_len, r1

nexthop_invalid:
    phvwr.e         p.control_metadata_p4e_drop_reason[P4E_DROP_NEXTHOP_INVALID], 1
    phvwr.f         p.capri_intrinsic_drop, 1

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
nexthop_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
