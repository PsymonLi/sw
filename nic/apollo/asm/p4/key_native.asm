#include "apollo.h"
#include "ingress.h"
#include "INGRESS_p.h"

struct key_native_k k;
struct key_native_d d;
struct phv_ p;

%%

nop:
    nop.e
    nop

.align
native_ipv4_packet:
    phvwr           p.key_metadata_ktype, KEY_TYPE_IPV4
    phvwr           p.key_metadata_src, k.ipv4_1_srcAddr
    phvwr           p.key_metadata_dst, k.ipv4_1_dstAddr
    seq             c1, k.udp_1_valid, TRUE
    phvwr.c1        p.key_metadata_sport, k.udp_1_srcPort
    phvwr.c1        p.key_metadata_dport, k.udp_1_dstPort
    phvwr.e         p.key_metadata_proto, k.ipv4_1_protocol
    phvwr           p.control_metadata_mapping_lkp_addr, k.ipv4_1_srcAddr

.align
native_ipv6_packet:
    phvwr           p.key_metadata_ktype, KEY_TYPE_IPV6
    phvwr           p.key_metadata_src, \
                        k.{ipv6_1_srcAddr_sbit0_ebit31...ipv6_1_srcAddr_sbit64_ebit127}
    phvwr           p.key_metadata_dst[127:16], k.ipv6_1_dstAddr_sbit0_ebit111
    phvwr           p.key_metadata_dst[15:0], k.ipv6_1_dstAddr_sbit112_ebit127
    seq             c1, k.udp_1_valid, TRUE
    phvwr.c1        p.key_metadata_sport, k.udp_1_srcPort
    phvwr.c1        p.key_metadata_dport, k.udp_1_dstPort
    phvwr           p.predicate_header_is_ipv6, 1
    phvwr.e         p.key_metadata_proto, k.ipv6_1_nextHdr
    phvwr           p.control_metadata_mapping_lkp_addr, \
                        k.{ipv6_1_srcAddr_sbit0_ebit31...ipv6_1_srcAddr_sbit64_ebit127}

.align
native_nonip_packet:
    seq             c1, k.ctag_1_valid, TRUE
    phvwr.c1        p.key_metadata_dport, k.ctag_1_etherType
    phvwr.!c1       p.key_metadata_dport, k.ethernet_1_etherType
    phvwr           p.key_metadata_ktype, KEY_TYPE_MAC
    phvwr.e         p.key_metadata_src, k.ethernet_1_srcAddr
    phvwr           p.key_metadata_dst, k.ethernet_1_dstAddr

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
key_native_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
