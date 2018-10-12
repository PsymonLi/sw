#include "ingress.h"
#include "INGRESS_p.h"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "nw.h"

struct input_mapping_native_k k;
struct phv_                   p;

%%

input_mapping_miss:
  phvwr.e     p.control_metadata_drop_reason[DROP_INPUT_MAPPING], 1
  phvwr.f     p.capri_intrinsic_drop, 1

.align
nop:
  nop.e
  nop

.align
native_ipv4_packet:
  bbeq          k.ethernet_dstAddr[40], 0, native_ipv4_packet_common
  phvwr         p.flow_lkp_metadata_pkt_type, PACKET_TYPE_UNICAST
  xor           r6, -1, r0
  seq           c2, k.ethernet_dstAddr, r6[47:0]
  cmov          r7, c2, PACKET_TYPE_BROADCAST, PACKET_TYPE_MULTICAST
  phvwr         p.flow_lkp_metadata_pkt_type, r7

native_ipv4_packet_common:
  add           r6, k.ipv4_ihl, k.tcp_dataOffset
  sub           r7, k.ipv4_totalLen, r6, 2

  phvwr         p.flow_lkp_metadata_lkp_type, FLOW_KEY_LOOKUP_TYPE_IPV4
  phvwrpair     p.flow_lkp_metadata_lkp_dst[31:0], k.ipv4_dstAddr, \
                    p.flow_lkp_metadata_lkp_src[31:0], k.ipv4_srcAddr

  or            r1, k.ipv4_ttl, k.ethernet_dstAddr, 8
  phvwr         p.{flow_lkp_metadata_lkp_dstMacAddr,flow_lkp_metadata_ip_ttl}, r1

  or            r1, k.ipv4_flags, k.ethernet_srcAddr, 3
  or            r1, r1, k.ipv4_ihl, 51
  phvwr         p.{flow_lkp_metadata_ipv4_hlen, \
                   flow_lkp_metadata_lkp_srcMacAddr, \
                   flow_lkp_metadata_ipv4_flags}, r1

  bbeq          k.esp_valid, TRUE, native_ipv4_esp_packet

  seq           c1, k.roce_bth_valid, TRUE
  cmov          r1, c1, r0, k.udp_srcPort
  or            r1, r1, k.udp_dstPort, 16
  seq           c1, k.ipv4_protocol, IP_PROTO_UDP
  phvwr.c1      p.{flow_lkp_metadata_lkp_dport,flow_lkp_metadata_lkp_sport}, r1
  phvwr.e       p.flow_lkp_metadata_lkp_proto, k.ipv4_protocol
  phvwr.f       p.l4_metadata_tcp_data_len, r7

native_ipv4_esp_packet:
  phvwr.e       p.flow_lkp_metadata_lkp_proto, IP_PROTO_IPSEC_ESP
  nop

.align
native_ipv6_packet:
  bbeq          k.ethernet_dstAddr[40], 0, native_ipv6_packet_common
  phvwr         p.flow_lkp_metadata_pkt_type, PACKET_TYPE_UNICAST
  xor           r6, -1, r0
  seq           c2, k.ethernet_dstAddr, r6[47:0]
  cmov          r7, c2, PACKET_TYPE_BROADCAST, PACKET_TYPE_MULTICAST
  phvwr         p.flow_lkp_metadata_pkt_type, r7

native_ipv6_packet_common:
  sub           r7, k.ipv6_payloadLen, k.tcp_dataOffset, 2
  phvwr         p.l4_metadata_tcp_data_len, r7

  seq           c1, k.roce_bth_valid, TRUE
  cmov          r1, c1, r0, k.udp_srcPort
  or            r1, r1, k.udp_dstPort, 16
  seq           c1, k.l3_metadata_ipv6_ulp, IP_PROTO_UDP
  phvwr.c1      p.{flow_lkp_metadata_lkp_dport,flow_lkp_metadata_lkp_sport}, r1

  phvwr         p.flow_lkp_metadata_lkp_src, k.{ipv6_srcAddr_sbit0_ebit31, \
                                                ipv6_srcAddr_sbit32_ebit127}
  phvwr         p.flow_lkp_metadata_lkp_dst, k.{ipv6_dstAddr_sbit0_ebit31, \
                                               ipv6_dstAddr_sbit32_ebit127}

  phvwr         p.flow_lkp_metadata_lkp_proto, k.l3_metadata_ipv6_ulp
  phvwr         p.flow_lkp_metadata_lkp_srcMacAddr, k.ethernet_srcAddr
  phvwrpair.e   p.flow_lkp_metadata_lkp_dstMacAddr, k.ethernet_dstAddr, \
                    p.flow_lkp_metadata_ip_ttl, k.ipv6_hopLimit
  phvwr.f       p.flow_lkp_metadata_lkp_type, FLOW_KEY_LOOKUP_TYPE_IPV6

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
native_non_ip_packet:
  bbeq          k.ethernet_dstAddr[40], 0, native_non_ip_packet_common
  phvwr         p.flow_lkp_metadata_pkt_type, PACKET_TYPE_UNICAST
  xor           r6, -1, r0
  seq           c2, k.ethernet_dstAddr, r6[47:0]
  cmov          r7, c2, PACKET_TYPE_BROADCAST, PACKET_TYPE_MULTICAST
  phvwr         p.flow_lkp_metadata_pkt_type, r7

native_non_ip_packet_common:
  phvwr         p.flow_lkp_metadata_lkp_type, FLOW_KEY_LOOKUP_TYPE_MAC
  seq           c1, k.vlan_tag_valid, 1
  phvwr.c1      p.flow_lkp_metadata_lkp_dport, k.vlan_tag_etherType
  phvwr.!c1     p.flow_lkp_metadata_lkp_dport, k.ethernet_etherType
  phvwrpair     p.flow_lkp_metadata_lkp_dst[47:0], k.ethernet_dstAddr, \
                    p.flow_lkp_metadata_lkp_src[47:0], k.ethernet_srcAddr
  phvwr.e       p.flow_lkp_metadata_lkp_srcMacAddr, k.ethernet_srcAddr
  phvwr.f       p.flow_lkp_metadata_lkp_dstMacAddr, k.ethernet_dstAddr

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
input_mapping_native_error:
  nop.e
  nop
