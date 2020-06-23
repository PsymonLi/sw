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
    or              r1, d.lif_info_d.learn_enabled, d.lif_info_d.lif_type, 1
    phvwr           p.{control_metadata_lif_type,control_metadata_learn_enabled}, r1
    phvwr           p.{vnic_metadata_bd_id,vnic_metadata_vpc_id}, \
                        d.{lif_info_d.bd_id,lif_info_d.vpc_id}
    phvwr           p.vnic_metadata_vrmac, d.lif_info_d.vrmac
    or              r1, d.lif_info_d.vnic_id, d.lif_info_d.bd_id, 16
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
