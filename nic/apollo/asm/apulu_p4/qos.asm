#include "apulu.h"
#include "ingress.h"
#include "INGRESS_p.h"

struct qos_d    d;
struct phv_     p;

%%
qos_info:
    /* output queue selection */
    phvwr.e         p.capri_intrinsic_tm_oq, d.qos_info_d.tm_oq
    phvwr.f         p.p4i_i2e_dst_tm_oq, d.qos_info_d.dst_tm_oq

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
qos_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
