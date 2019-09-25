#include "apulu.h"
#include "egress.h"
#include "EGRESS_p.h"
#include "EGRESS_p4e_device_info_k.h"

struct p4e_device_info_k_   k;
struct p4e_device_info_d    d;
struct phv_                 p;

%%

p4e_device_info:
    phvwr.e         p.rewrite_metadata_device_ipv4_addr, \
                        d.p4e_device_info_d.device_ipv4_addr
    phvwr.f         p.rewrite_metadata_device_ipv6_addr, \
                        d.p4e_device_info_d.device_ipv6_addr

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
p4e_device_info_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
