/*****************************************************************************
 *  txdma_stage0.s: This assembly program provides the common entry point to 
 *                   jump to various TXDMA P4+ programs. 
 *                   1. Include the start labels used in your stage0 programs 
 *                      in both the parameter and in the jump instrunction. 
 *                   2. Use <this_program_name, stage0_label> when invoking 
 *                      Capri MPU loader API capri_program_label_to_offset() 
 *                      to derive the relative offset to program the queue 
 *                      context structure.
 *                   3. Use <this_program_name> when invoking Capri MPU
 *                      loader API capri_program_to_base_addr() to derive
 *                      the base address for the stage 0 P4+ programs in the
 *                      table config engine.
 *****************************************************************************/


%%

    .param      req_tx_sqcb_process
    .param      storage_tx_q_state_pop_start
    .param      storage_tx_pri_q_state_pop_start
    .param      tcp_tx_stage0_process
    .param      tls_stage0
    .param      eth_tx_fetch_desc
    .param      esp_ipv4_tunnel_h2n_txdma_stage0
    .param      cpu_tx_stage0_start
    .param      ipfix_start
    .param      resp_tx_rqcb_process
    .param      esp_ipv4_tunnel_n2h_txdma_stage0
    .param      adminq_fetch_desc

//Keep offset 0 for none to avoid invoking unrelated program when
//qstate's pc_offset is not initialized
.align
dummy:
   nop.e
   nop

//Do not change the order of this entry. 
//Keep it the first one after dummy
//This has to align with the txdma_stage0.s program
.align
rdma_resp_tx_stage0:
    j resp_tx_rqcb_process
    nop

//Do not change the order of this entry. 
//Keep it the second one after dummy
//This has to align with the txdma_stage0.s program
.align
rdma_req_tx_stage0:
    j req_tx_sqcb_process
    nop

.align
eth_tx_stage0:
   j    eth_tx_fetch_desc
   nop

.align
tcp_tx_stage0:
    j   tcp_tx_stage0_process
    nop

.align
ipsec_tx_stage0:
    j esp_ipv4_tunnel_h2n_txdma_stage0
    nop

.align
tls_tx_stage0:
    j   tls_stage0
    nop

.align 
ipsec_tx_n2h_stage0:
    j esp_ipv4_tunnel_n2h_txdma_stage0
    nop

.align
storage_tx_stage0:
   j    storage_tx_q_state_pop_start
   nop

.align
storage_tx_pri_stage0:
   j    storage_tx_pri_q_state_pop_start
   nop

.align
cpu_tx_stage0:
    j   cpu_tx_stage0_start
    nop

.align
ipfix_tx_stage0:
    j   ipfix_start
    nop

.align
adminq_stage0:
    j   adminq_fetch_desc
    nop
