#include "apulu.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_p4i_device_info_k.h"

struct p4i_device_info_k_   k;
struct p4i_device_info_d    d;
struct phv_                 p;

%%

p4i_device_info:
    add             r1, (LIF_STATS_TX_UCAST_BYTES_OFFSET / 64), \
                        k.capri_intrinsic_lif, 4
    phvwr           p.control_metadata_lif_tx_stats_id, r1
    sub             r1, k.capri_p4_intrinsic_frame_size, \
                        k.offset_metadata_l2_1
    sne             c1, k.capri_intrinsic_tm_oq, TM_P4_RECIRC_QUEUE
    phvwr.c1        p.capri_intrinsic_tm_iq, k.capri_intrinsic_tm_oq
    phvwr           p.control_metadata_qos_class_id, k.capri_intrinsic_tm_oq
    bbeq            k.ingress_recirc_valid, FALSE, p4i_recirc_done
    phvwr           p.capri_p4_intrinsic_packet_len, r1

p4i_recirc:
    sne             c1, k.ingress_recirc_local_mapping_ohash, 0
    phvwr.c1        p.control_metadata_local_mapping_ohash_lkp, TRUE
    sne             c1, k.ingress_recirc_flow_ohash, 0
    phvwr.c1        p.control_metadata_flow_ohash_lkp, TRUE

p4i_recirc_done:
    seq             c1, k.ethernet_1_dstAddr, \
                        d.p4i_device_info_d.device_mac_addr1
    seq.!c1         c1, k.ethernet_1_dstAddr, \
                        d.p4i_device_info_d.device_mac_addr2
    crestore        [c3-c2], k.{ipv4_1_valid,ipv6_1_valid}, 0x3
    bcf             [c3], p4i_device_ip_check
    seq.c3          c3, k.ipv4_1_dstAddr, d.p4i_device_info_d.device_ipv4_addr
    seq.c2          c2, k.ipv6_1_dstAddr[127:64], \
                        d.p4i_device_info_d.device_ipv6_addr[127:64]
    seq.c2          c2, k.ipv6_1_dstAddr[63:0], \
                        d.p4i_device_info_d.device_ipv6_addr[63:0]
p4i_device_ip_check:
    andcf           c1, [c2 | c3]
    phvwr           p.control_metadata_l2_enabled, \
                        d.p4i_device_info_d.l2_enabled
    phvwr.c1        p.control_metadata_to_device_ip, TRUE
    seq             c1, k.p4plus_to_p4_insert_vlan_tag, TRUE
    phvwr.c1        p.ctag_1_vid, k.p4plus_to_p4_vlan_vid
    phvwrpair.e     p.p4i_i2e_nexthop_type, NEXTHOP_TYPE_NEXTHOP, \
                        p.p4i_i2e_priority, 0x1f
    phvwr.f         p.key_metadata_entry_valid, TRUE

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
p4i_device_info_error:
    phvwr           p.capri_intrinsic_drop, 1
    sne.e           c1, k.capri_intrinsic_tm_oq, TM_P4_RECIRC_QUEUE
    phvwr.c1        p.capri_intrinsic_tm_iq, k.capri_intrinsic_tm_oq
