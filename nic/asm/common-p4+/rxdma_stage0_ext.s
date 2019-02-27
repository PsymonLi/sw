/*****************************************************************************
 *  rxdma_stage0_ext.s: This assembly program provides the common entry point to 
 *                   jump to various RXDMA P4+ programs. This is an extension to
 *                   the main program for performance reasons.
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

#include "INGRESS_p.h"

struct phv_ p;

%%
    .param      resp_rx_rqcb_process_ext
    .param      req_rx_sqcb1_process_ext
    .param      esp_ipv4_tunnel_h2n_ipsec_encap_rxdma_initial_table2 
    .param      ipsec_esp_v4_tunnel_n2h_good_pkt
    .param      tcp_rx_read_shared_stage0_start_ext


//Keep offset 0 for none to avoid invoking unrelated program when
//qstate's pc_offset is not initialized
.align
rx_dummy_ext:
    phvwr.e     p.p4_intr_global_drop, 1
    nop

//Do not change the order of this entry. 
//Keep it the first one after dummy
//This has to align with the txdma_stage0.s program
.align
rdma_resp_rx_stage0_ext:
    j resp_rx_rqcb_process_ext
    nop

//Do not change the order of this entry. 
//Keep it the second one after dummy
//This has to align with the txdma_stage0.s program
.align
rdma_req_rx_stage0_ext:
    j req_rx_sqcb1_process_ext
    nop

//Do not change the order of this entry
//This has to align with the txdma_stage0.s program
.align
eth_rx_stage0:
    nop.e
    nop

//Do not change the order of this entry
//This has to align with the txdma_stage0.s program
.align
eth_tx_stage0_dummy:
    nop.e
    nop

//Do not change the order of this entry
//This has to align with the txdma_stage0.s program
.align
rdma_cq_rx_stage0_ext:
    nop.e
    nop

//Do not change the order of this entry
//This has to align with the txdma_stage0.s program
.align
rdma_aq_rx_stage0_ext:
    nop.e
    nop

.align
tcp_rx_stage0:
    j tcp_rx_read_shared_stage0_start_ext
    nop

.align
ipsec_rx_stage0:
    j esp_ipv4_tunnel_h2n_ipsec_encap_rxdma_initial_table2 
    nop

.align
cpu_rx_stage0:
    nop.e
    nop

.align
ipsec_rx_n2h_stage0:
    j ipsec_esp_v4_tunnel_n2h_good_pkt 
    nop

.align
p4pt_rx_stage0:
    nop.e
    nop

.align
rawr_rx_stage0:
    nop.e
    nop

.align
virtio_rx_stage0:
    nop.e
    nop
