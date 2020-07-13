#include "apulu.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_lif_k.h"

struct lif_k_           k;
struct lif_d            d;
struct phv_             p;

%%

lif_info:
    phvwr           p.control_metadata_rx_packet, d.lif_info_d.direction
    phvwr           p.control_metadata_lif_type, d.lif_info_d.lif_type
    phvwr           p.control_metadata_learn_enabled, d.lif_info_d.learn_enabled
    phvwr           p.{vnic_metadata_bd_id,vnic_metadata_vpc_id}, \
                        d.{lif_info_d.bd_id,lif_info_d.vpc_id}
    or              r1, d.lif_info_d.vnic_id, d.lif_info_d.bd_id, 16
    // lif_vlan_mode
    crestore        [c2-c1], d.lif_info_d.lif_vlan_mode, 0x3
    bcf             [!c2], lif_vlan_mode_done
    cmov            r7, c1, k.ctag2_1_vid, k.capri_intrinsic_lif
    phvwr           p.key_metadata_lif, r7
lif_vlan_mode_done:
    phvwrpair.e     p.p4i_i2e_rx_packet, d.lif_info_d.direction, \
                        p.p4i_i2e_src_lif, k.capri_intrinsic_lif
    phvwr.f         p.{key_metadata_flow_lkp_id,vnic_metadata_vnic_id}, r1

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
lif_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
