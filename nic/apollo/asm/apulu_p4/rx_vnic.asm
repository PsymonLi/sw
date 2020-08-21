#include "apulu.h"
#include "egress.h"
#include "EGRESS_p.h"
#include "EGRESS_rx_vnic_k.h"

struct rx_vnic_k_   k;
struct rx_vnic_d    d;
struct phv_         p;

%%

rx_vnic_info:
    sne             c1, d.rx_vnic_info_d.private_mac, r0
    phvwr.e         p.vnic_metadata_rx_policer_id, \
                        d.rx_vnic_info_d.rx_policer_id
    phvwr.c1        p.ethernet_1_dstAddr, d.rx_vnic_info_d.private_mac

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
rx_vnic_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
