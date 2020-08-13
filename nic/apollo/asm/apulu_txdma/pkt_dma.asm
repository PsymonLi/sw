#include "../../p4/include/apulu_sacl_defines.h"
#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_pkt_dma_k.h"

struct pkt_dma_k_   k;
struct phv_         p;

%%

pkt_dma:
    // clear hardware error (hack)
    phvwr           p.capri_intr_hw_error, r0

    // Clear the intrinsic recirc count to prevent TTL drop
    phvwr           p.capri_p4_intr_recirc_count, r0

    // Increment the local_recirc_count.
    add             r1, k.txdma_control_recirc_count, 1
    phvwr           p.txdma_control_recirc_count, r1

    // Copy txdma control dnat_en to predicate_lpm1_enable
    phvwr           p.txdma_predicate_lpm1_enable, k.txdma_control_dnat_en

    /* Do we have more SACLs to process... ? */
    sne             c1, k.rx_to_tx_hdr_sacl_base_addr0, r0
    /* If so, enable Recirc, and stop */
    phvwr.c1.e      p.capri_p4_intr_recirc, TRUE
    phvwr           p.txdma_predicate_pass_two, TRUE

    /* Do we have DNAT to lookup... ? */
    sne             c1, k.txdma_control_dnat_en, r0
    /* If so, enable Recirc, and stop */
    phvwr.c1.e      p.capri_p4_intr_recirc, TRUE
    nop

    /* Else disable Recirc. */
    phvwr           p.capri_p4_intr_recirc, FALSE
    phvwr           p.txdma_predicate_pass_two, FALSE

    /* If policy counter is enabled, increment if policy was enabled and hit */
    add             r1, k.txdma_control_rule_priority_s4_e10, \
                        k.txdma_control_rule_priority_s0_e3, \
                        7
    sne             c1, r0, k.txdma_control_sacl_cntr_regn_addr
    sne             c2, r1, SACL_PRIORITY_INVALID
    setcf           c1, [c1 & c2]
    /* Compute and add policy offset to the counter address */
    add.c1          r1, k.txdma_control_final_policy_index_s4_e9, \
                        k.txdma_control_final_policy_index_s0_e3, \
                        6
    mul.c1          r2, r1, SACL_COUNTER_BLOCK_SIZE
    add.c1          r1, r2, k.txdma_control_sacl_cntr_regn_addr
    /* Compute and add rule offset to the counter address */
    mul.c1          r2, k.txdma_control_final_rule_index, SACL_COUNTER_SIZE
    add.c1          r1, r1, r2
    /* Compute atomic_add array index */
    addi.c1         r6, r0, ASIC_MEM_SEM_ATOMIC_ADD_START
    add.c1          r6, r6, r1[26:0]
    /* Format the atomic_add array command for adding 1 to one counter only */
    add.c1          r7, r0, 1
    add.c1          r7, r7, r1[31:27], 58
    /* Increment the counter */
    memwr.dx.c1     r6, r7

    // Setup Intrisic fields and DMA commands to generate packet to P4IG
    phvwr           p.capri_intr_tm_oport, TM_PORT_EGRESS

    // DMA Commands
    //   1) Global Intrinsic, Intrinsic P4, TXDMA Intr, TxDMA to P4E
    //   2) Packet Payload (mem2pkt)
    //   3) CI Update (consume the event) and DB Sched Eval
    //   4) Update the rxdma copy of cindex once every 64 pkts

    //  1) Global Intrinsic, Intrinsic P4, TXDMA Intr, TxDMA to P4E
    CAPRI_DMA_CMD_PHV2PKT_SETUP2(intrinsic_dma_dma_cmd,
                                 capri_intr_tm_iport, \
                                 capri_txdma_intr_txdma_rsv,
                                 txdma_to_p4e_meter_en,
                                 txdma_to_p4e_route_priority)

    // 2) DMA command for payload
    // mem2pkt has an implicit fence. all subsequent dma is blocked
    phvwr           p.payload_dma_dma_cmd_addr, k.txdma_control_payload_addr
    phvwri          p.{payload_dma_dma_cmd_cache...payload_dma_dma_cmd_type}, \
                        (1 << 4) | (1 << 6) | CAPRI_DMA_COMMAND_MEM_TO_PKT
    phvwr           p.payload_dma_dma_cmd_size, k.rx_to_tx_hdr_payload_len

    // 3) Update the rxdma copy of cindex once every 64 pkts
    seq             c1, k.txdma_control_cindex[5:0], 0
    CAPRI_DMA_CMD_PHV2MEM_SETUP_COND(rxdma_ci_update_dma_cmd, \
                                     k.txdma_control_rxdma_cindex_addr, \
                                     txdma_control_cindex, \
                                     txdma_control_cindex, c1)

    // 4) DMA command setup for Doorbell Sched Eval
    add r1, k.capri_intr_lif_s3_e10, k.capri_intr_lif_s0_e2, 8
    CAPRI_RING_DOORBELL_ADDR(0, DB_IDX_UPD_CIDX_SET, DB_SCHED_UPD_EVAL, 0, \
                             r1)
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
