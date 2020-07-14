#include "apulu.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_p4i_bd_k.h"

struct p4i_bd_k_    k;
struct p4i_bd_d     d;
struct phv_         p;

%%

ingress_bd_info:
    bbeq            k.control_metadata_tunnel_terminate, TRUE, tunneled_pkt
    seq             c1, k.control_metadata_l2_enabled, FALSE
native_pkt:
    bbne            k.ipv4_1_valid, TRUE, ingress_bd_info_exit
    seq.!c1         c1, k.ethernet_1_dstAddr, d.ingress_bd_info_d.vrmac
    bcf             [!c1], ingress_bd_info_exit
    phvwr.c1        p.p4i_i2e_mapping_lkp_type, KEY_TYPE_IPV4
    phvwr.e         p.p4i_i2e_mapping_lkp_addr, k.ipv4_1_dstAddr
    phvwr.f         p.p4i_i2e_mapping_lkp_id, k.vnic_metadata_vpc_id

tunneled_pkt:
    bbne            k.ipv4_2_valid, TRUE, ingress_bd_info_exit
    seq.!c1         c1, k.ethernet_2_dstAddr, d.ingress_bd_info_d.vrmac
    bcf             [!c1], ingress_bd_info_exit
    phvwr.c1        p.p4i_i2e_mapping_lkp_type, KEY_TYPE_IPV4
    phvwr.e         p.p4i_i2e_mapping_lkp_addr, k.ipv4_2_dstAddr
    phvwr.f         p.p4i_i2e_mapping_lkp_id, k.vnic_metadata_vpc_id

ingress_bd_info_exit:
    nop.e
    nop

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
ingress_bd_info_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
