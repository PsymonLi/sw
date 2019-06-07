#include "artemis_txdma.h"
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_pkt_dma_k.h"

struct pkt_dma_k_   k;
struct phv_         p;

%%

pkt_dma:
    // clear hardware error (hack)
    phvwr           p.capri_intr_hw_error, 0

    /* Is this the first pass? */
    seq              c1, k.capri_p4_intr_recirc_count, 0
    /* Then enable Recirc */
    phvwr.c1.e      p.capri_p4_intr_recirc, TRUE
    phvwr           p.txdma_predicate_pass_two, TRUE

    /* Else disable Recirc */
    phvwr.!c1       p.capri_p4_intr_recirc, FALSE

    CAPRI_RING_DOORBELL_ADDR(0, DB_IDX_UPD_CIDX_SET, DB_SCHED_UPD_EVAL, 0, \
                             k.capri_intr_lif)
    CAPRI_DMA_CMD_PHV2MEM_SETUP_STOP_END(doorbell_ci_update_dma_cmd, \
                                         r4, \
                                         doorbell_data_pid, \
                                         doorbell_data_index)

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
pkt_dma_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
