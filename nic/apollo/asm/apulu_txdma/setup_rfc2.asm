#include "../../p4/include/apulu_sacl_defines.h"

#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_setup_rfc2_k.h"

struct phv_            p;
struct setup_rfc2_k_   k;

%%

setup_rfc2:
    // If a root change has been signalled
    seq              c1, r0, k.txdma_control_root_change
    // No. Stop.
    nop.c1.e
    nop

    // Yes. Check new root_number and copy tags from the correct root
    seq              c1, k.txdma_control_root_count, 1
    bcf              [c1], load1
    phvwr.c1         p.rx_to_tx_hdr_stag0_classid0, k.rx_to_tx_hdr_stag0_classid1
    seq              c1, k.txdma_control_root_count, 2
    bcf              [c1], load2
    phvwr.c1         p.rx_to_tx_hdr_stag0_classid0, k.rx_to_tx_hdr_stag0_classid2
    seq              c1, k.txdma_control_root_count, 3
    bcf              [c1], load3
    phvwr.c1         p.rx_to_tx_hdr_stag0_classid0, k.rx_to_tx_hdr_stag0_classid3
    seq              c1, k.txdma_control_root_count, 4
    bcf              [c1], load4
    phvwr.c1         p.rx_to_tx_hdr_stag0_classid0, k.rx_to_tx_hdr_stag0_classid4
    seq              c1, k.txdma_control_root_count, 5
    bcf              [c1], load5
    phvwr.c1         p.rx_to_tx_hdr_stag0_classid0, k.rx_to_tx_hdr_stag0_classid5
    seq              c1, k.txdma_control_root_count, 6
    bcf              [c1], clearall
    phvwr.c1         p.rx_to_tx_hdr_stag0_classid0, r0
    nop.e
    nop

load1:
    phvwr            p.rx_to_tx_hdr_stag1_classid0, k.rx_to_tx_hdr_stag1_classid1
    phvwr            p.rx_to_tx_hdr_stag2_classid0, k.rx_to_tx_hdr_stag2_classid1
    phvwr            p.rx_to_tx_hdr_stag3_classid0, k.rx_to_tx_hdr_stag3_classid1
    phvwr            p.rx_to_tx_hdr_stag4_classid0, k.rx_to_tx_hdr_stag4_classid1
    phvwr            p.rx_to_tx_hdr_dtag0_classid0, k.rx_to_tx_hdr_dtag0_classid1
    phvwr            p.rx_to_tx_hdr_dtag1_classid0, k.rx_to_tx_hdr_dtag1_classid1
    phvwr            p.rx_to_tx_hdr_dtag2_classid0, k.rx_to_tx_hdr_dtag2_classid1
    phvwr            p.rx_to_tx_hdr_dtag3_classid0, k.rx_to_tx_hdr_dtag3_classid1
    phvwr            p.rx_to_tx_hdr_dtag4_classid0, k.rx_to_tx_hdr_dtag4_classid1
    phvwr.e          p.txdma_control_stag_classid,  k.rx_to_tx_hdr_stag0_classid1
    phvwr            p.txdma_control_dtag_classid,  k.rx_to_tx_hdr_dtag0_classid1

load2:
    phvwr            p.rx_to_tx_hdr_stag1_classid0, k.rx_to_tx_hdr_stag1_classid2
    phvwr            p.rx_to_tx_hdr_stag2_classid0, k.rx_to_tx_hdr_stag2_classid2
    phvwr            p.rx_to_tx_hdr_stag3_classid0, k.rx_to_tx_hdr_stag3_classid2
    phvwr            p.rx_to_tx_hdr_stag4_classid0, k.rx_to_tx_hdr_stag4_classid2
    phvwr            p.rx_to_tx_hdr_dtag0_classid0, k.rx_to_tx_hdr_dtag0_classid2
    phvwr            p.rx_to_tx_hdr_dtag1_classid0, k.rx_to_tx_hdr_dtag1_classid2
    phvwr            p.rx_to_tx_hdr_dtag2_classid0, k.rx_to_tx_hdr_dtag2_classid2
    phvwr            p.rx_to_tx_hdr_dtag3_classid0, k.rx_to_tx_hdr_dtag3_classid2
    phvwr            p.rx_to_tx_hdr_dtag4_classid0[9:4], k.rx_to_tx_hdr_dtag4_classid2_s0_e5
    phvwr            p.rx_to_tx_hdr_dtag4_classid0[3:0], k.rx_to_tx_hdr_dtag4_classid2_s6_e9
    phvwr.e          p.txdma_control_stag_classid,  k.rx_to_tx_hdr_stag0_classid2
    phvwr            p.txdma_control_dtag_classid,  k.rx_to_tx_hdr_dtag0_classid2

load3:
    phvwr            p.rx_to_tx_hdr_stag1_classid0, k.rx_to_tx_hdr_stag1_classid3
    phvwr            p.rx_to_tx_hdr_stag2_classid0, k.rx_to_tx_hdr_stag2_classid3
    phvwr            p.rx_to_tx_hdr_stag3_classid0, k.rx_to_tx_hdr_stag3_classid3
    phvwr            p.rx_to_tx_hdr_stag4_classid0[9:2], k.rx_to_tx_hdr_stag4_classid3_s0_e7
    phvwr            p.rx_to_tx_hdr_stag4_classid0[1:0], k.rx_to_tx_hdr_stag4_classid3_s8_e9
    phvwr            p.rx_to_tx_hdr_dtag0_classid0, k.rx_to_tx_hdr_dtag0_classid3
    phvwr            p.rx_to_tx_hdr_dtag1_classid0, k.rx_to_tx_hdr_dtag1_classid3
    phvwr            p.rx_to_tx_hdr_dtag2_classid0, k.rx_to_tx_hdr_dtag2_classid3
    phvwr            p.rx_to_tx_hdr_dtag3_classid0, k.rx_to_tx_hdr_dtag3_classid3
    phvwr            p.rx_to_tx_hdr_dtag4_classid0, k.rx_to_tx_hdr_dtag4_classid3
    phvwr.e          p.txdma_control_stag_classid,  k.rx_to_tx_hdr_stag0_classid3
    phvwr            p.txdma_control_dtag_classid,  k.rx_to_tx_hdr_dtag0_classid3

load4:
    phvwr            p.rx_to_tx_hdr_stag1_classid0, k.rx_to_tx_hdr_stag1_classid4
    phvwr            p.rx_to_tx_hdr_stag2_classid0, k.rx_to_tx_hdr_stag2_classid4
    phvwr            p.rx_to_tx_hdr_stag3_classid0, k.rx_to_tx_hdr_stag3_classid4
    phvwr            p.rx_to_tx_hdr_stag4_classid0, k.rx_to_tx_hdr_stag4_classid4
    phvwr            p.rx_to_tx_hdr_dtag0_classid0, k.rx_to_tx_hdr_dtag0_classid4
    phvwr            p.rx_to_tx_hdr_dtag1_classid0, k.rx_to_tx_hdr_dtag1_classid4
    phvwr            p.rx_to_tx_hdr_dtag2_classid0, k.rx_to_tx_hdr_dtag2_classid4
    phvwr            p.rx_to_tx_hdr_dtag3_classid0, k.rx_to_tx_hdr_dtag3_classid4
    phvwr            p.rx_to_tx_hdr_dtag4_classid0, k.rx_to_tx_hdr_dtag4_classid4
    phvwr.e          p.txdma_control_stag_classid,  k.rx_to_tx_hdr_stag0_classid4
    phvwr            p.txdma_control_dtag_classid,  k.rx_to_tx_hdr_dtag0_classid4

load5:
    phvwr            p.rx_to_tx_hdr_stag1_classid0, k.rx_to_tx_hdr_stag1_classid5
    phvwr            p.rx_to_tx_hdr_stag2_classid0, k.rx_to_tx_hdr_stag2_classid5
    phvwr            p.rx_to_tx_hdr_stag3_classid0, k.rx_to_tx_hdr_stag3_classid5
    phvwr            p.rx_to_tx_hdr_stag4_classid0, k.rx_to_tx_hdr_stag4_classid5
    phvwr            p.rx_to_tx_hdr_dtag0_classid0, k.rx_to_tx_hdr_dtag0_classid5
    phvwr            p.rx_to_tx_hdr_dtag1_classid0, k.rx_to_tx_hdr_dtag1_classid5
    phvwr            p.rx_to_tx_hdr_dtag2_classid0, k.rx_to_tx_hdr_dtag2_classid5
    phvwr            p.rx_to_tx_hdr_dtag3_classid0, k.rx_to_tx_hdr_dtag3_classid5
    phvwr            p.rx_to_tx_hdr_dtag4_classid0[9:4], k.rx_to_tx_hdr_dtag4_classid5_s0_e5
    phvwr            p.rx_to_tx_hdr_dtag4_classid0[3:0], k.rx_to_tx_hdr_dtag4_classid5_s6_e9
    phvwr.e          p.txdma_control_stag_classid,  k.rx_to_tx_hdr_stag0_classid5
    phvwr            p.txdma_control_dtag_classid,  k.rx_to_tx_hdr_dtag0_classid5

clearall:
    phvwr            p.rx_to_tx_hdr_stag1_classid0, r0
    phvwr            p.rx_to_tx_hdr_stag2_classid0, r0
    phvwr            p.rx_to_tx_hdr_stag3_classid0, r0
    phvwr            p.rx_to_tx_hdr_stag4_classid0, r0
    phvwr            p.rx_to_tx_hdr_dtag0_classid0, r0
    phvwr            p.rx_to_tx_hdr_dtag1_classid0, r0
    phvwr            p.rx_to_tx_hdr_dtag2_classid0, r0
    phvwr            p.rx_to_tx_hdr_dtag3_classid0, r0
    phvwr            p.rx_to_tx_hdr_dtag4_classid0, r0
    phvwr.e          p.txdma_control_stag_classid,  r0
    phvwr            p.txdma_control_dtag_classid,  r0

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
setup_rfc2_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
