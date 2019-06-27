#include "egress.h"
#include "EGRESS_p.h"
#include "EGRESS_tunnel_encap_update_inner_k.h"
#include "nic/hal/iris/datapath/p4/include/defines.h"

struct tunnel_encap_update_inner_k_ k;
struct phv_                         p;

%%

nop:
  nop.e
  nop

.align
encap_inner_ipv4_udp_rewrite:
  phvwr       p.{inner_ipv4_version...inner_ipv4_diffserv}, k.{ipv4_version...ipv4_diffserv}
  phvwr       p.{inner_ipv4_totalLen...inner_ipv4_dstAddr}, k.{ipv4_totalLen...ipv4_dstAddr}
  phvwr       p.{inner_udp_len...inner_udp_checksum}, k.{udp_len...udp_checksum}
  phvwr       p.{inner_udp_srcPort...inner_udp_dstPort}, k.{udp_srcPort...udp_dstPort}
  phvwr       p.tunnel_metadata_inner_ip_proto, IP_PROTO_UDP
  phvwrpair   p.inner_ipv4_valid, TRUE, p.ipv4_valid, FALSE
  crestore    [c2-c1], k.{ipv4_options_blob2_valid, ipv4_options_blob_valid}, 0x3
  phvwr.c2    p.inner_ipv4_options_blob_valid, TRUE
  phvwr.c1    p.inner_ipv4_options_blob2_valid, TRUE
  phvwr       p.{ipv4_options_blob2_valid,ipv4_options_blob_valid}, r0
  phvwrpair   p.inner_udp_valid, TRUE, p.udp_valid, FALSE
  phvwrpair.e p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_IP_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_IP_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_IP_CHECKSUM], 0
  phvwrpair   p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_L4_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], 0

.align
encap_inner_ipv4_tcp_rewrite:
  phvwr       p.{inner_ipv4_version...inner_ipv4_diffserv}, k.{ipv4_version...ipv4_diffserv}
  phvwr       p.{inner_ipv4_totalLen...inner_ipv4_dstAddr}, k.{ipv4_totalLen...ipv4_dstAddr}
  phvwr       p.tunnel_metadata_inner_ip_proto, IP_PROTO_IPV4
  phvwrpair   p.inner_ipv4_valid, TRUE, p.ipv4_valid, FALSE
  crestore    [c2-c1], k.{ipv4_options_blob2_valid, ipv4_options_blob_valid}, 0x3
  phvwr.c2    p.inner_ipv4_options_blob_valid, TRUE
  phvwr.c1    p.inner_ipv4_options_blob2_valid, TRUE
  phvwr       p.{ipv4_options_blob2_valid,ipv4_options_blob_valid}, r0
  phvwrpair.e p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_IP_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_IP_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_IP_CHECKSUM], 0
  phvwrpair   p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_L4_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], 0

.align
encap_inner_ipv4_icmp_rewrite:
encap_inner_ipv4_unknown_rewrite:
  phvwr       p.{inner_ipv4_version...inner_ipv4_diffserv}, k.{ipv4_version...ipv4_diffserv}
  phvwr       p.{inner_ipv4_totalLen...inner_ipv4_dstAddr}, k.{ipv4_totalLen...ipv4_dstAddr}
  phvwr       p.tunnel_metadata_inner_ip_proto, IP_PROTO_IPV4
  phvwrpair   p.inner_ipv4_valid, TRUE, p.ipv4_valid, FALSE
  crestore    [c2-c1], k.{ipv4_options_blob2_valid, ipv4_options_blob_valid}, 0x3
  phvwr.c2    p.inner_ipv4_options_blob_valid, TRUE
  phvwr.c1    p.inner_ipv4_options_blob2_valid, TRUE
  phvwr       p.{ipv4_options_blob2_valid,ipv4_options_blob_valid}, r0
  phvwrpair.e p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_IP_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_IP_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_IP_CHECKSUM], 0
  phvwrpair   p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_L4_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], 0

.align
encap_inner_ipv6_udp_rewrite:
  phvwr       p.inner_ipv6_version, k.ipv6_version
  phvwr       p.{inner_ipv6_trafficClass...inner_ipv6_srcAddr}, k.{ipv6_trafficClass...ipv6_srcAddr}
  phvwr       p.{inner_ipv6_dstAddr}, k.{ipv6_dstAddr}
  phvwr       p.{inner_udp_len...inner_udp_checksum}, k.{udp_len...udp_checksum}
  phvwr       p.{inner_udp_srcPort...inner_udp_dstPort}, k.{udp_srcPort...udp_dstPort}
  phvwr       p.tunnel_metadata_inner_ip_proto, IP_PROTO_IPV6
  phvwrpair   p.inner_udp_valid, TRUE, p.udp_valid, FALSE
  phvwrpair.e p.inner_ipv6_valid, TRUE, p.ipv6_valid, FALSE
  phvwrpair   p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_L4_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], 0

.align
encap_inner_ipv6_tcp_rewrite:
  phvwr       p.inner_ipv6_version, k.ipv6_version
  phvwr       p.{inner_ipv6_trafficClass...inner_ipv6_srcAddr}, k.{ipv6_trafficClass...ipv6_srcAddr}
  phvwr       p.{inner_ipv6_dstAddr}, k.{ipv6_dstAddr}
  phvwr       p.tunnel_metadata_inner_ip_proto, IP_PROTO_IPV6
  phvwrpair.e p.inner_ipv6_valid, TRUE, p.ipv6_valid, FALSE
  phvwrpair   p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_L4_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], 0
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
encap_inner_ipv6_icmp_rewrite:
encap_inner_ipv6_unknown_rewrite:
  phvwr       p.inner_ipv6_version, k.ipv6_version
  phvwr       p.{inner_ipv6_trafficClass...inner_ipv6_srcAddr}, k.{ipv6_trafficClass...ipv6_srcAddr}
  phvwr       p.{inner_ipv6_dstAddr}, k.{ipv6_dstAddr}
  phvwr       p.tunnel_metadata_inner_ip_proto, IP_PROTO_IPV6
  phvwrpair.e p.inner_ipv6_valid, TRUE, p.ipv6_valid, FALSE
  phvwrpair   p.control_metadata_checksum_ctl[CHECKSUM_CTL_INNER_L4_CHECKSUM], \
                k.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], \
                p.control_metadata_checksum_ctl[CHECKSUM_CTL_L4_CHECKSUM], 0

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
tunnel_encap_update_inner_error:
  nop.e
  nop
