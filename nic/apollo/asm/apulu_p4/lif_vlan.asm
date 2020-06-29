#include "apulu.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_lif_vlan_k.h"

struct lif_vlan_k_  k;
struct lif_vlan_d   d;
struct phv_         p;

%%

lif_vlan_info:
    add             r6, r0, k.vnic_metadata_bd_id
    add             r7, r0, k.vnic_metadata_vpc_id
    bcf             [!c1], lif_vlan_local_mapping_key
    sne             c1, d.lif_vlan_info_d.vnic_id, r0
    phvwr.c1        p.vnic_metadata_vnic_id, d.lif_vlan_info_d.vnic_id
    sne             c1, d.lif_vlan_info_d.bd_id, r0
    phvwr.c1        p.vnic_metadata_bd_id, d.lif_vlan_info_d.bd_id
    add.c1          r6, r0, d.lif_vlan_info_d.bd_id
    seq.c1          c1, k.arm_to_p4i_flow_lkp_id_override, FALSE
    phvwr.c1        p.key_metadata_flow_lkp_id, d.lif_vlan_info_d.bd_id
    sne             c1, d.lif_vlan_info_d.vpc_id, r0
    phvwr.c1        p.vnic_metadata_vpc_id, d.lif_vlan_info_d.vpc_id
    add.c1          r7, r0, d.lif_vlan_info_d.vpc_id

lif_vlan_local_mapping_key:
    bbeq            k.control_metadata_rx_packet, TRUE, \
                        lif_vlan_local_mapping_key_rx
    seq             c7, k.ipv4_1_valid, TRUE
lif_vlan_local_mapping_key_tx:
    seq             c1, k.ipv4_1_srcAddr, r0
    bcf             [!c7 | c1], lif_vlan_local_mapping_key_tx_non_ipv4
    nop
    phvwr           p.key_metadata_local_mapping_lkp_type, KEY_TYPE_IPV4
    phvwr           p.key_metadata_local_mapping_lkp_addr, k.ipv4_1_srcAddr
    b               lif_vlan_mapping_key
    phvwr           p.key_metadata_local_mapping_lkp_id, r7

lif_vlan_local_mapping_key_tx_non_ipv4:
    seq             c1, k.arp_valid, TRUE
    sne.c1          c1, k.arp_senderIpAddr, r0
    bcf             [c1], lif_vlan_local_mapping_key_tx_arp
    phvwr.!c1       p.key_metadata_local_mapping_lkp_type, KEY_TYPE_MAC
    phvwr           p.key_metadata_local_mapping_lkp_addr, k.ethernet_1_srcAddr
    b               lif_vlan_mapping_key
    phvwr           p.key_metadata_local_mapping_lkp_id, r6

lif_vlan_local_mapping_key_tx_arp:
    phvwr           p.key_metadata_local_mapping_lkp_type, KEY_TYPE_IPV4
    phvwr           p.key_metadata_local_mapping_lkp_addr, k.arp_senderIpAddr
    b               lif_vlan_mapping_key
    phvwr           p.key_metadata_local_mapping_lkp_id, r7

lif_vlan_local_mapping_key_rx:
    bcf             [!c7], lif_vlan_local_mapping_key_rx_non_ipv4
    phvwr.c7        p.key_metadata_local_mapping_lkp_type, KEY_TYPE_IPV4
    phvwr           p.key_metadata_local_mapping_lkp_addr, k.ipv4_1_dstAddr
    b               lif_vlan_mapping_key
    phvwr           p.key_metadata_local_mapping_lkp_id, r7

lif_vlan_local_mapping_key_rx_non_ipv4:
    phvwr           p.key_metadata_local_mapping_lkp_type, KEY_TYPE_MAC
    phvwr           p.key_metadata_local_mapping_lkp_addr, k.ethernet_1_dstAddr
    phvwr           p.key_metadata_local_mapping_lkp_id, r6

lif_vlan_mapping_key:
    phvwr           p.p4i_i2e_mapping_lkp_type, KEY_TYPE_MAC
    phvwr.e         p.p4i_i2e_mapping_lkp_addr, k.ethernet_1_dstAddr
    phvwr.f         p.p4i_i2e_mapping_lkp_id, r6

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
lif_vlan_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
