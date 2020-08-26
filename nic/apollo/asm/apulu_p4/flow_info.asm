#include "apulu.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_flow_info_k.h"

struct flow_info_k_ k;
struct flow_info_d  d;
struct phv_         p;

%%

flow_info:
    phvwr.e.f       p.p4i_i2e_is_local_to_local, d.flow_info_d.is_local_to_local
    nop

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
flow_info_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
