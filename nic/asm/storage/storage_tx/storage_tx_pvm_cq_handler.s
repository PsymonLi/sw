/*****************************************************************************
 *  pvm_cq_handler: Save the NVME command in SQ entry to PHV. DMA the
 *                  working consumer index to the consumer index in the
 *                  queue state. Check to see if we can do PRP assist and
 *                  load the address for the next stage based on that.
 *****************************************************************************/

#include "storage_asm_defines.h"
#include "ingress.h"
#include "INGRESS_p.h"

// TODO: FIXME the table numbering
struct s3_tbl_k k;
struct s3_tbl_pvm_cq_handler_d d;
struct phv_ p;

%%
   .param storage_tx_q_state_push_start

storage_tx_pvm_cq_handler_start:

   // DMA write w_ndx to c_ndx to actually pop the entry
   add		r7, r0, STORAGE_KIVEC0_SRC_QADDR
   addi		r7, r7, Q_STATE_C_NDX_OFFSET
   DMA_PHV2MEM_SETUP(storage_kivec0_w_ndx, storage_kivec0_w_ndx, r7, 
                     dma_p2m_0)

   // Initialize the remaining fields of the PVM command in the PHV
   phvwr	p.{pvm_status_cspec...pvm_status_dst_qaddr}, d.{cspec...dst_qaddr}                                           

   // Setup the DMA command to push the NVME status entry. For now keep the 
   // destination address to be 0 (in GPR r0). Set this correctly in the
   // next stage.
   DMA_PHV2MEM_SETUP(pvm_status_cspec, pvm_status_status, r0, 
                     dma_p2m_1)

   // Set the table and program address 
   LOAD_TABLE_FOR_ADDR_PARAM(STORAGE_KIVEC0_DST_QADDR, 6,
                             storage_tx_q_state_push_start)

exit:
   nop.e
   nop
