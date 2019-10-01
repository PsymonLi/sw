#include "apulu.h"
#include "egress.h"
#include "EGRESS_p.h"
#include "EGRESS_p4e_inter_pipe_k.h"

struct p4e_inter_pipe_k_    k;
struct phv_                 p;

%%

p4e_inter_pipe:
    sne             c1, k.capri_intrinsic_tm_oq, TM_P4_RECIRC_QUEUE
    phvwr.c1        p.capri_intrinsic_tm_iq, k.capri_intrinsic_tm_oq
    phvwr.!c1       p.capri_intrinsic_tm_oq, k.capri_intrinsic_tm_iq
    seq             c1, k.egress_recirc_mapping_done, FALSE
    bcf             [c1], egress_recirc
    phvwrpair.!c1   p.p4e_i2e_valid, FALSE, p.txdma_to_p4e_valid, FALSE
    seq             c1, k.capri_intrinsic_tm_oport, TM_PORT_DMA
    nop.!c1.e

egress_to_rxdma:
    add             r1, r0, k.capri_p4_intrinsic_packet_len
    phvwr           p.capri_rxdma_intrinsic_valid, TRUE
    phvwr           p.{p4e_to_p4plus_classic_nic_ip_valid, \
                        p4e_to_p4plus_classic_nic_valid}, 0x3
    phvwr           p.p4e_to_p4plus_classic_nic_packet_len, r1
    phvwr           p.p4e_to_p4plus_classic_nic_p4plus_app_id, \
                        P4PLUS_APPTYPE_CLASSIC_NIC
    phvwr           p.capri_rxdma_intrinsic_rx_splitter_offset, \
                        (CAPRI_GLOBAL_INTRINSIC_HDR_SZ + \
                         CAPRI_RXDMA_INTRINSIC_HDR_SZ + \
                         P4PLUS_CLASSIC_NIC_HDR_SZ)
    seq             c1, k.ipv4_1_valid, TRUE
    bcf             [c1], egress_to_rxdma_ipv4
    seq             c1, k.ipv6_1_valid, TRUE
    nop.!c1.e
egress_to_rxdma_ipv6:
    seq             c1, k.ipv6_1_nextHdr, IP_PROTO_TCP
    phvwr.c1.e      p.p4e_to_p4plus_classic_nic_pkt_type, \
                        CLASSIC_NIC_PKT_TYPE_IPV6_TCP
    seq             c1, k.ipv6_1_nextHdr, IP_PROTO_UDP
    phvwr.c1.e      p.p4e_to_p4plus_classic_nic_pkt_type, \
                        CLASSIC_NIC_PKT_TYPE_IPV6_UDP
    nop.e
    phvwr.f         p.p4e_to_p4plus_classic_nic_pkt_type, \
                        CLASSIC_NIC_PKT_TYPE_IPV6
egress_to_rxdma_ipv4:
    phvwr           p.key_metadata_src, k.ipv4_1_srcAddr
    phvwr           p.key_metadata_dst, k.ipv4_1_dstAddr
    seq             c1, k.ipv4_1_protocol, IP_PROTO_TCP
    phvwr.c1.e      p.p4e_to_p4plus_classic_nic_pkt_type, \
                        CLASSIC_NIC_PKT_TYPE_IPV4_TCP
    seq             c1, k.ipv4_1_protocol, IP_PROTO_UDP
    phvwr.c1.e      p.p4e_to_p4plus_classic_nic_pkt_type, \
                        CLASSIC_NIC_PKT_TYPE_IPV4_UDP
    phvwr.f         p.p4e_to_p4plus_classic_nic_pkt_type, \
                        CLASSIC_NIC_PKT_TYPE_IPV4

egress_recirc:
    phvwr.e         p.egress_recirc_valid, TRUE
    phvwr.f         p.capri_intrinsic_tm_oport, TM_PORT_EGRESS

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
p4e_inter_pipe_error:
    phvwr           p.capri_intrinsic_drop, 1
    sne.e           c1, k.capri_intrinsic_tm_oq, TM_P4_RECIRC_QUEUE
    phvwr.c1        p.capri_intrinsic_tm_iq, k.capri_intrinsic_tm_oq
