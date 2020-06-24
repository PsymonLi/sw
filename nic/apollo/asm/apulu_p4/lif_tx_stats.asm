#include "apulu.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_lif_tx_stats_k.h"

struct lif_tx_stats_k_  k;
struct lif_tx_stats_d   d;
struct phv_             p;

%%

lif_tx_stats:
    seq             c1, k.capri_intrinsic_drop, TRUE
    seq.!c1         c1, k.control_metadata_local_mapping_done, FALSE
    seq.!c1         c1, k.control_metadata_flow_done, FALSE
    tbladd.c1.e.f   d.{lif_tx_stats_d.pad1}.dx, 0

    seq             c1, k.ethernet_1_dstAddr[40], 0
    seq             c2, k.ethernet_1_dstAddr, -1
    cmov.c1         r1, c1, PACKET_TYPE_UNICAST, 0
    cmov.!c1        r1, c2, PACKET_TYPE_BROADCAST, PACKET_TYPE_MULTICAST
    .brbegin
    br              r1[1:0]
    nop
    .brcase PACKET_TYPE_UNICAST
    tbladd.e        d.{lif_tx_stats_d.ucast_bytes}.dx, \
                        k.capri_p4_intrinsic_packet_len
    tbladd.f        d.{lif_tx_stats_d.ucast_pkts}.dx, 1
    .brcase PACKET_TYPE_MULTICAST
    tbladd.e        d.{lif_tx_stats_d.mcast_bytes}.dx, \
                        k.capri_p4_intrinsic_packet_len
    tbladd.f        d.{lif_tx_stats_d.mcast_pkts}.dx, 1
    .brcase PACKET_TYPE_BROADCAST
    tbladd.e        d.{lif_tx_stats_d.bcast_bytes}.dx, \
                        k.capri_p4_intrinsic_packet_len
    tbladd.f        d.{lif_tx_stats_d.bcast_pkts}.dx, 1
    .brcase 3
    tbladd.e.f      d.{lif_tx_stats_d.pad1}.dx, 0
    nop
    .brend

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
lif_tx_stats_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
