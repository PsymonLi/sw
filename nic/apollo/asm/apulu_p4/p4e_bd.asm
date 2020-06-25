#include "apulu.h"
#include "egress.h"
#include "EGRESS_p.h"
#include "EGRESS_p4e_bd_k.h"

struct p4e_bd_k_    k;
struct p4e_bd_d     d;
struct phv_         p;

%%

egress_bd_info:
    seq             c1, k.p4e_i2e_binding_check_drop, TRUE
    phvwr.c1.e      p.capri_intrinsic_drop, 1
    phvwr.c1.f      p.control_metadata_p4e_drop_reason[P4E_DROP_MAC_IP_BINDING_FAIL], 1
    seq             c1, k.vnic_metadata_egress_bd_id, r0
    nop.c1.e
    phvwr.!c1       p.rewrite_metadata_tunnel_tos, d.egress_bd_info_d.tos
    phvwr.e         p.rewrite_metadata_vni, d.egress_bd_info_d.vni
    phvwr.f         p.rewrite_metadata_vrmac, d.egress_bd_info_d.vrmac

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
egress_bd_info_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
