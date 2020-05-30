#include "apulu.h"
#include "egress.h"
#include "EGRESS_p.h"
#include "EGRESS_mirror_k.h"

struct mirror_k_    k;
struct mirror_d     d;
struct phv_         p;

%%

nop:
    sub             r5, k.capri_p4_intrinsic_frame_size, k.offset_metadata_l2_1
    phvwr           p.capri_p4_intrinsic_packet_len, r5
    phvwr           p.capri_intrinsic_tm_span_session, 0
    phvwr.e         p.egress_recirc_mapping_done, TRUE
    phvwr.f         p.control_metadata_mapping_done, TRUE

.align
lspan:
    phvwr           p.egress_recirc_mapping_done, TRUE
    phvwr           p.control_metadata_mapping_done, TRUE
    seq             c1, k.ctag_s_valid, 1
    cmov            r4, c1, 4, 0
    sub             r5, k.capri_p4_intrinsic_frame_size, k.offset_metadata_l2_1
    add             r6, r0, d.u.lspan_d.truncate_len
    bal             r7, mirror_truncate
    sne             c7, r0, r0
    phvwr           p.capri_p4_intrinsic_packet_len, r5
    phvwr           p.rewrite_metadata_nexthop_type, d.u.lspan_d.nexthop_type
    phvwr.e         p.p4e_i2e_nexthop_id, d.u.lspan_d.nexthop_id
    phvwr.f         p.mirror_blob_valid, FALSE

.align
rspan:
    phvwr           p.egress_recirc_mapping_done, TRUE
    phvwr           p.control_metadata_mapping_done, TRUE
    phvwr           p.ctag_1_valid, 1
    add             r1, k.ethernet_1_etherType, d.u.rspan_d.ctag, 16
    phvwr           p.{ctag_1_pcp,ctag_1_dei,ctag_1_vid,ctag_1_etherType}, r1
    phvwr           p.ethernet_1_etherType, ETHERTYPE_CTAG
    seq             c1, k.ctag_s_valid, 1
    cmov            r4, c1, 4, 0
    sub             r5, k.capri_p4_intrinsic_frame_size, k.offset_metadata_l2_1
    add             r6, r0, d.u.rspan_d.truncate_len
    bal             r7, mirror_truncate
    sne             c7, r0, r0
    add             r5, r5, 4
    phvwr           p.capri_p4_intrinsic_packet_len, r5
    phvwr           p.rewrite_metadata_nexthop_type, d.u.rspan_d.nexthop_type
    phvwr.e         p.p4e_i2e_nexthop_id, d.u.rspan_d.nexthop_id
    phvwr.f         p.mirror_blob_valid, FALSE

.align
erspan:
    sub             r5, k.capri_p4_intrinsic_frame_size, k.offset_metadata_l2_1

    tbladd          d.u.erspan_d.npkts, 1
    tbladd          d.u.erspan_d.nbytes, r5
    phvwr           p.gre_0_opt_seq_seq_num, d.u.erspan_d.seq_num
    tbladd.f        d.u.erspan_d.seq_num, 1

    phvwr           p.egress_recirc_mapping_done, TRUE
    phvwr           p.control_metadata_mapping_done, TRUE

    bbne            k.ctag_s_valid, TRUE, erspan_truncate
    sne             c2, r0, r0
    phvwr           p.erspan3_span_id, d.u.erspan_d.span_id
    phvwrpair       p.erspan3_vlan, k.ctag_s_vid, p.erspan3_cos, k.ctag_s_pcp
    bbne            d.u.erspan_d.vlan_strip_en, TRUE, erspan_ctag_no_strip
    seq             c6, d.u.erspan_d.erspan_type, ERSPAN_TYPE_II
    phvwr.c6        p.erspan2_encap_type, 0x2
    phvwr           p.ethernet_1_etherType, k.ctag_s_etherType
    phvwr           p.ctag_s_valid, FALSE
    b               erspan_truncate
    sub             r5, r5, 4

erspan_ctag_no_strip:
    seq             c2, r0, r0
    phvwr.c6        p.erspan2_encap_type, 0x3

erspan_truncate:
    phvwr           p.erspan3_timestamp, r4
    cmov            r4, c2, 4, 0
    add             r6, r0, d.u.erspan_d.truncate_len
    bal             r7, mirror_truncate
    seq             c7, r0, r0

    // ethernet and ctag
    seq             c1, d.u.erspan_d.ctag, r0
    cmov            r7, c1, 14, 18
    bcf             [c1], erspan_tag_done
    cmov            r2, c1, ETHERTYPE_IPV4, ETHERTYPE_CTAG
erspan_tagged:
    phvwr           p.ctag_0_valid, 1
    or              r3, ETHERTYPE_IPV4, d.u.erspan_d.ctag, 16
    phvwr           p.{ctag_0_pcp,ctag_0_dei,ctag_0_vid,ctag_0_etherType}, r3

erspan_tag_done:
    phvwr           p.ethernet_0_valid, 1
    phvwr           p.ethernet_0_dstAddr, d.u.erspan_d.dmac
    or              r3, r2, d.u.erspan_d.smac, 16
    phvwr           p.{ethernet_0_srcAddr,ethernet_0_etherType}, r3

    // ipv4
    phvwr           p.{ipv4_0_version,ipv4_0_ihl}, 0x45
    phvwr           p.ipv4_0_diffserv, d.u.erspan_d.dscp
    phvwr           p.{ipv4_0_srcAddr,ipv4_0_dstAddr}, \
                        d.{u.erspan_d.sip,u.erspan_d.dip}
    phvwr           p.ipv4_0_ttl, 64
    phvwr           p.ipv4_0_protocol, IP_PROTO_GRE
    phvwr           p.capri_deparser_len_ipv4_0_hdr_len, 20

    // gre
    seq             c5, d.u.erspan_d.gre_seq_en, TRUE
    phvwr.c5        p.gre_0_opt_seq_valid, TRUE
    phvwri.c5       p.{gre_0_C...gre_0_ver}, 0x1000
    phvwri.!c5      p.{gre_0_C...gre_0_ver}, 0

    seq             c1, d.u.erspan_d.erspan_type, ERSPAN_TYPE_I
    bcf             [!c1&!c6], erspan3
    phvwr           p.mirror_blob_valid, FALSE
    bcf             [c6], erspan2
    phvwr           p.gre_0_proto, GRE_PROTO_ERSPAN
erspan1:
    add             r6, r5, 24
    add             r7, r7, 24
    b               erspan_common
    // 0000 1000 1001
    add             r1, r0, 0x089

erspan2:
    phvwr           p.erspan2_version, 0x1
    phvwr           p.erspan2_port_id, k.capri_intrinsic_lif
    cmov            r1, c5, 36, 32
    add             r6, r5, r1
    add             r7, r7, r1
    // 0011 1000 1001 or 0010 1000 1001
    b               erspan_common
    cmov            r1, c5, 0x389, 0x289

erspan3:
    phvwr           p.gre_0_proto, GRE_PROTO_ERSPAN_T3
    phvwrpair       p.erspan3_version, 0x2, p.erspan3_bso, 0
    seq             c1, k.capri_intrinsic_tm_iport, TM_PORT_EGRESS
    phvwr.c1        p.erspan3_direction, 1
    phvwrpair       p.{erspan3_sgt...erspan3_hw_id}, 0, \
                        p.{erspan3_granularity,erspan3_options}, 0x6
    cmov            r1, c5, 40, 36
    add             r6, r5, r1
    add             r7, r7, r1
    // 0101 1000 1001 or 0100 1000 1001
    cmov            r1, c5, 0x589, 0x489

erspan_common:
    phvwr           p.{erspan3_opt_valid, erspan3_valid, erspan2_valid, \
                        gre_0_opt_seq_valid, gre_0_valid, vxlan_0_valid, \
                        udp_0_valid, ipv6_0_valid, ipv4_0_valid, \
                        ipv4_0_udp_csum, ipv4_0_tcp_csum, ipv4_0_csum}, r1
    phvwr           p.ipv4_0_totalLen, r6
    add             r5, r5, r7
    phvwr           p.capri_p4_intrinsic_packet_len, r5
    phvwr           p.rewrite_metadata_nexthop_type, d.u.erspan_d.nexthop_type
    phvwr           p.p4e_i2e_nexthop_id, d.u.erspan_d.nexthop_id
    phvwr           p.control_metadata_apply_tunnel2, d.u.erspan_d.apply_tunnel2
    phvwr.e         p.rewrite_metadata_tunnel2_id, d.u.erspan_d.tunnel2_id
    phvwr.f         p.rewrite_metadata_tunnel2_vni, d.u.erspan_d.tunnel2_vni

// r4 : adjust_len, r5 : packet_len, r6 : truncate_len, r7 : return address
// c1 : is_erspan
mirror_truncate:
    sub             r1, r5, 14
    sne             c1, r6, r0
    slt.c1          c1, r6, r1
    jr.!c1          r7
    phvwr           p.capri_intrinsic_tm_span_session, 0

    add             r5, r6, 14
    sub             r6, r6, r4
    phvwr           p.capri_deparser_len_trunc_pkt_len, r6
    phvwr.c7        p.erspan3_truncated, TRUE
    jr              r7
    phvwr           p.{capri_intrinsic_payload,capri_deparser_len_trunc}, 0x1

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
mirror_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    phvwr.f         p.capri_intrinsic_tm_span_session, 0
