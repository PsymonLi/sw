#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s0_t0_ooq_tcp_tx_k.h"
#include "nic/p4/common/defines.h"
#include "tcp-phv.h"

#define TCP_DBG_SKIP_CLEANUP_COND_BMASK     0

struct phv_ p;
struct s0_t0_ooq_tcp_tx_k_ k;
struct s0_t0_ooq_tcp_tx_load_stage0_d d;

%%
    .align
    .param          tcp_ooq_txdma_load_rx2tx_slot
    .param          tcp_ooq_txdma_process_next_descr_addr
    .param          tcp_ooq_free_queue
    .param          tcp_ooq_txdma_win_upd_phv2pkt_dma
    .param          tcp_ccq_s1_write_actl_q 
    .param          tcp_post_feedb_rst_pkt

tcp_qtype1_process:
    CAPRI_OPERAND_DEBUG(r7)
    .brbegin
       // priorities are 0 (highest) to 7 (lowest)
       brpri		r7[4:0], [4,3,2,0,1]
       nop
          .brcase 0
             b tcp_ooq_load_qstate              // prio 1
             nop
          .brcase 1
             b tcp_trigger_window_update        // prio 0
             nop
          .brcase 2
             b tcp_ccq_cond1_s0_load_qstate     // prio 2
             nop
          .brcase 3
             b tcp_ccq_cond2_s0_load_qstate     // prio 3
             nop
          .brcase 4
             b tcp_gen_feedb_rst_pkt            // prio 4
             nop
          .brcase 5
             b tcp_ooq_load_qstate_do_nothing   // prio 5
             nop
    .brend


tcp_trigger_window_update:
    tblwr.f         d.{ci_1}.hx, d.{pi_1}.hx

    phvwrpair       p.common_phv_fid, k.p4_txdma_intr_qid, \
                        p.common_phv_qstate_addr, k.p4_txdma_intr_qstate_addr
    phvwri          p.{p4_intr_global_tm_iport...p4_intr_global_tm_oport}, (TM_PORT_DMA << 4) | TM_PORT_DMA
    phvwri          p.p4_txdma_intr_dma_cmd_ptr, TCP_PHV_OOQ_TXDMA_COMMANDS_START
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tcp_ooq_txdma_win_upd_phv2pkt_dma)

    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_NOP, DB_SCHED_UPD_EVAL, TCP_OOO_RX2TX_QTYPE, LIF_TCP)
    /* data will be in r3 */
    CAPRI_RING_DOORBELL_DATA_NOP(k.p4_txdma_intr_qid)

    memwr.dx.e        r4, r3
    nop

tcp_gen_feedb_rst_pkt:
    tblwr.f         d.{ci_4}.hx, d.{pi_4}.hx
    phvwrpair       p.common_phv_fid, k.p4_txdma_intr_qid, \
                        p.common_phv_qstate_addr, k.p4_txdma_intr_qstate_addr
    phvwri          p.{p4_intr_global_tm_iport...p4_intr_global_tm_oport}, (TM_PORT_DMA << 4) | TM_PORT_DMA
    phvwri          p.p4_txdma_intr_dma_cmd_ptr, TCP_PHV_OOQ_TXDMA_COMMANDS_START
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tcp_post_feedb_rst_pkt)

    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_NOP, DB_SCHED_UPD_EVAL, TCP_OOO_RX2TX_QTYPE, LIF_TCP)
    /* data will be in r3 */
    CAPRI_RING_DOORBELL_DATA_NOP(k.p4_txdma_intr_qid)

    memwr.dx.e        r4, r3
    nop

tcp_ooq_load_qstate:
    phvwrpair       p.common_phv_fid, k.p4_txdma_intr_qid, \
                        p.common_phv_qstate_addr, k.p4_txdma_intr_qstate_addr
    seq             c1, d.ooq_work_in_progress, 0
    b.c1            tcp_ooq_load_qstate_process_new_request
    nop
    
tcp_ooq_load_qstate_process_next_pkt_descr:
    /*
     * TODO : Figure out a way to trim the first few bytes (d.curr_ooq_trim)
     */
    seq             c1, d.curr_ooo_qbase, 0
    b.c1            tcp_ooq_load_qstate_do_nothing
    add             r1, d.curr_ooo_qbase, d.curr_index, NIC_OOQ_ENTRY_SIZE_SHIFT
    CAPRI_NEXT_TABLE_READ(0, TABLE_LOCK_DIS, tcp_ooq_txdma_process_next_descr_addr, r1, TABLE_SIZE_64_BITS)

    // check if we are done with current OOQ
    tbladd          d.curr_index, 1
    seq             c1, d.curr_index, d.curr_ooq_num_entries
    b.!c1           tcp_ooq_skip_doorbell
    nop

    // we are done with current queue, ring doorbell if pi == ci
    tblwr           d.ooq_work_in_progress, 0
    tblmincri.f     d.{ci_0}.hx, ASIC_OOO_RX2TX_RING_SLOTS_SHIFT, 1

    // Inform producer of CI
    add             r1, r0, d.ooo_rx2tx_producer_ci_addr
    memwr.h         r1, d.{ci_0}.hx

    // launch table to free queue
    phvwr           p.to_s1_qbase_addr, d.curr_ooo_qbase
    CAPRI_NEXT_TABLE_READ(1, TABLE_LOCK_EN, tcp_ooq_free_queue, d.ooo_rx2tx_free_pi_addr, TABLE_SIZE_32_BITS)

    seq             c1, d.{ci_0}.hx, d.{pi_0}.hx
    b.!c1           tcp_ooq_skip_doorbell

    /*
     * We are done with all OOO queues
     * Ring doorbell to eval
     */
    phvwr           p.common_phv_all_ooq_done, 1
    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_NOP,
                        DB_SCHED_UPD_EVAL, TCP_OOO_RX2TX_QTYPE, LIF_TCP)
    /* data will be in r3 */
    CAPRI_RING_DOORBELL_DATA_NOP(k.p4_txdma_intr_qid)
    memwr.dx        r4, r3

tcp_ooq_skip_doorbell:
    nop.e
    nop

tcp_ooq_load_qstate_process_new_request:
    // New request
    add             r1, d.ooo_rx2tx_qbase, d.{ci_0}.hx, NIC_OOQ_RX2TX_ENTRY_SIZE_SHIFT
    //launch table with this address
    CAPRI_NEXT_TABLE_READ(0, TABLE_LOCK_DIS, tcp_ooq_txdma_load_rx2tx_slot, r1, TABLE_SIZE_64_BITS)
    tblwr           d.curr_ooo_qbase, 0
    tblwr.f         d.ooq_work_in_progress, 1

    wrfence.e
    nop

tcp_ooq_load_qstate_do_nothing:
    phvwri          p.p4_intr_global_drop, 1
    nop.e
    nop

/*
 * - Indication of Condition 2 for TCB cleanup: TCB state moved to CLOSE
 * - Check if we have already received close indication, if yes simply drop
 * - Set bit in bitmap in qstate for cond 2 
 * - If all conditions we are interested in for cleanup are done,
 *  launch ACTL semaphore for next stage to post msg to ACTL
 * Possible CLOSE scenarios/reasons:
 * LAST_ACK → CLOSED (passive graceful close)
 * PASSIVE_GRACEFUL
 * TIME_WAIT → CLOSED (active graceful close)
 * ACTIVE_GRACEFUL
 * ESTABLISHED → CLOSED (abortive close)
 * REMOTE_RST/LOCAL_RST/LOCAL_INTERNAL_RST
 */
tcp_ccq_cond2_s0_load_qstate:
    CAPRI_CLEAR_TABLE0_VALID
#if (TCP_DBG_SKIP_CLEANUP_COND_BMASK & TCP_CLEANUP_COND2_BIT)
    // Make ci = pi for marking cond2 ind as processed and FLUSH table
    tblwr.f         d.{ci_3}.hx, d.{pi_3}.hx
    phvwri          p.p4_intr_global_drop, 1
    b               write_db_for_ccq_sched_eval
    nop // Branch-delay slot
#endif
    seq             c1, d.{ci_3}, 0
    tblor           d.cleanup_cond_done, TCP_CLEANUP_COND2_BIT
    /*
     * The producer has used the prod to pass reason code
     * The prod value otherwise has no significance for indicating a close
     * on the ccq ring 
     */
    tblwr.c1        d.close_reason, d.{pi_3}.hx
    // Make ci = pi for marking cond2 ind as processed and FLUSH table
    tblwr.f         d.{ci_3}.hx, d.{pi_3}.hx
    /*
     * Launch ACTL if all conditions are met for cleanup AND if cond1 is not
     * already processed (ci_3 == 0), Else drop
     */
    seq.c1          c1, d.cleanup_cond_done, d.cleanup_cond_bmap
    b.!c1           write_db_for_ccq_sched_eval
    phvwri.!c1      p.p4_intr_global_drop, 1
    // All conditions are met for cleanup. Launch ACTL
    b               launch_stage_actl_write
    nop
/*
 * - Indication of Condition 1 for TCB cleanup: SESQ Final Cleanup Complete
 * - Check if cond 1 is one of the conditions we are interested in, else simply drop
 * - If the above is true, Set bit in bitmap in qstate for cond 1
 * - If all the conditions are are interested in for cleanup are done,
 *  launch ACTL semaphore for next stage to post msg to ACTL
 */
tcp_ccq_cond1_s0_load_qstate:
    CAPRI_CLEAR_TABLE0_VALID
#if (TCP_DBG_SKIP_CLEANUP_COND_BMASK & TCP_CLEANUP_COND1_BIT)
    // Make ci = pi for marking cond2 ind as processed and FLUSH table
    tblwr.f         d.{ci_2}.hx, d.{pi_2}.hx
    phvwri          p.p4_intr_global_drop, 1
    b               write_db_for_ccq_sched_eval
    nop // Branch-delay slot
#endif

    // Make ci = pi for marking cond1 ind as processed and FLUSH table
    add             r2, r0, d.cleanup_cond_bmap
    smneb           c1, r2, TCP_CLEANUP_COND1_BIT, 0
    tblor.c1        d.cleanup_cond_done, TCP_CLEANUP_COND1_BIT
    tblwr.f         d.{ci_2}.hx, d.{pi_2}.hx
    seq.c1          c1, d.cleanup_cond_done, d.cleanup_cond_bmap
    b.!c1           write_db_for_ccq_sched_eval
    phvwri.!c1      p.p4_intr_global_drop, 1
    // All conditions are met for cleanup. Launch ACTL

launch_stage_actl_write:
    // We are all set to send TCB cleanup msg to ACTL
    // Setup txdma dma ptr to the dma cmd offset in phv for phv2mem dma of actl entry
    phvwr       p.common_phv_fid, k.p4_txdma_intr_qid
    phvwri      p.p4_txdma_intr_dma_cmd_ptr, TCP_PHV_ACTL_TXDMA_COMMANDS_START
    phvwr       p.to_s1_close_reason, d.close_reason
    // Launch TCP ACTL Semaphore, for ACTL ringnum/cpu 0
    CPU_TCP_ACTL_Q_SEM_INF_ADDR(0, r3)
    // Resultant Sem addr is in r3
    CAPRI_NEXT_TABLE_READ(0,
                          TABLE_LOCK_DIS,
                          tcp_ccq_s1_write_actl_q,
                          r3,
                          TABLE_SIZE_64_BITS)

write_db_for_ccq_sched_eval:    
    // phvwr           p.p4_intr_global_debug_trace, 1 
    /*
     * Ring DB for scehduler to EVAL pi/ci and stop rescheduling
     */
    // Create DB addr in r4 with TCP LIF and Type 1
    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_NOP, DB_SCHED_UPD_EVAL, TCP_OOO_RX2TX_QTYPE, LIF_TCP) 
    CAPRI_RING_DOORBELL_DATA_NOP(k.p4_txdma_intr_qid)
    /* Above macro creates DB data in r3 */
    memwr.dx        r4, r3
    nop.e
    nop

