/*
 *      Implements the rx2tx shared state read stage of the TxDMA P4+ pipeline
 */
#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s0_t0_tcp_tx_k.h"

struct phv_ p;
struct s0_t0_tcp_tx_k_ k;
struct s0_t0_tcp_tx_read_rx2tx_d d;

%%
   .align

arm_sock_app_start:
   seq             c1, d.{ci_0}, d.{pi_0}
   b.c1            arm_sock_app_do_nothing
   nop

   tblwr.f         d.{ci_0}, d.{pi_0}

   addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_NOP, DB_SCHED_UPD_EVAL, 0, LIF_ARM_SOCKET_APP)
   CAPRI_RING_DOORBELL_DATA_NOP(k.p4_txdma_intr_qid)
   memwr.dx.e      r4, r3
   nop

arm_sock_app_do_nothing:
   nop.e
   nop
