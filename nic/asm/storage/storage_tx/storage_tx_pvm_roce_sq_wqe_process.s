/*****************************************************************************
 *  pvm_roce_sq_wqe_process: Process the ROCE SQ WQE as pointed to by the
 *                           PVM's ROCE SQ and load the R2N buffer from the
 *                           WRID in the WQE if the operation type was
 *                           RDMA_SEND. Then post the buffer back to ROCE RQ.
 *****************************************************************************/

#include "storage_asm_defines.h"
#include "ingress.h"
#include "INGRESS_p.h"

struct s1_tbl_k k;
struct s1_tbl_pvm_roce_sq_wqe_process_d d;
struct phv_ p;

%%
   .param storage_tx_roce_rq_push_start

storage_tx_pvm_roce_sq_wqe_process_start:

   // Update the queue doorbell to clear the scheduler bit
   QUEUE_POP_DOORBELL_UPDATE

   // If operation type is send on the target side, then the buffer has to 
   // posted back
   seq		c1, d.op_type, ROCE_OP_TYPE_SEND
   seq		c2, d.op_type, ROCE_OP_TYPE_SEND_INV
   seq		c3, d.op_type, ROCE_OP_TYPE_SEND_IMM
   seq		c4, d.op_type, ROCE_OP_TYPE_SEND_INV_IMM
   bcf		![c1 | c2 | c3 | c4], exit
   nop
   
   // In DOL environment, all buffer posting is done by infrastructure to serialize
   // operations. Enable the buffer posting in production code when PVM is ready.
   b		exit
   nop
   
   // Setup the R2N buffer to post using mem2mem DMA in DMA commands 1 & 2
   R2N_BUF_POST_SETUP(d.wrid)

   
   // Set the table and the program address for the next stage
   LOAD_TABLE_FOR_ADDR_PARAM(STORAGE_KIVEC0_DST_QADDR, Q_STATE_SIZE,
                             storage_tx_roce_rq_push_start)

exit:
   // Setup the start and end DMA pointers
   DMA_PTR_SETUP(dma_p2m_0_dma_cmd_pad, dma_p2m_0_dma_cmd_eop,
                 p4_txdma_intr_dma_cmd_ptr)
   LOAD_NO_TABLES

