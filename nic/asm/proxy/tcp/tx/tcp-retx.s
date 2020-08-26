/*
 *    Implements the TX stage of the TxDMA P4+ pipeline
 */

#include "tcp-constants.h"
#include "tcp-phv.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s3_t0_tcp_tx_k.h"

struct phv_ p;
struct s3_t0_tcp_tx_k_ k;
struct s3_t0_tcp_tx_retx_d d;

%%
    .align
    .param          tcp_xmit_process_start
    .param          tcp_xmit_ack_process_start
    .param          tcp_xmit_idle_process_start
    .param          tcp_tx_read_nmdr_gc_idx_start
    .param          tcp_tx_sack_rx
    .param          tcp_xmit_rst_process_start
    .param          tcp_xmit_keepalive_probe

tcp_retx_process_start:
    bbeq            d.tx_rst_sent, 1, tcp_retx_drop

    seq             c1, k.common_phv_launch_sack_rx, 1
    bal.c1          r7, tcp_retx_launch_sack_rx
    nop
    
    bbeq           k.common_phv_pending_keepa, 1, tcp_send_keepa
    nop

    bbeq            k.common_phv_pending_ack_send, 1, tcp_retx_ack_send
    nop

    bbeq            k.common_phv_pending_tw_fw2_to, 1, tcp_retx_fw2_tw_to
    nop

    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN,
                        tcp_xmit_process_start,
                        k.common_phv_qstate_addr,
                        TCP_TCB_XMIT_OFFSET, TABLE_SIZE_512_BITS)

    seq             c1, k.common_phv_pending_rto, 1
    seq             c2, k.common_phv_pending_fast_retx, 1
    bcf             [c1 | c2], tcp_retx_retransmit

    seq             c1, k.common_phv_pending_sesq, 1
    seq.!c1         c1, k.common_phv_pending_asesq, 1
    b.!c1           end_program

    // Process pending SESQ/ASESQ
    tblwr.c1        d.last_snd_wnd, k.t0_s2s_snd_wnd

    seq             c2, k.common_phv_fin, 1
    seq             c1, k.common_phv_rst, 1
    // Nothing to do if not RST/FIN
    bcf             [!c1 & !c2], end_program
    nop
handle_fin_rst:
    // RST or FIN: mark end of SESQ, end_marker_flag. No more entries in SESQ
    tblwr           d.sesq_end_marker_flag, 1
    // If FIN, do nothing more, If RST, reschedule to cleanup retx queue
    b.!c1           end_program
tcp_retx_handle_rst:
    // Schedule Retx Cleanup for final Retx queue cleanup on sending RST
    tblwr.c1        d.tx_rst_sent, 1
    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_INC,
                        DB_SCHED_UPD_EVAL, 0, LIF_TCP)
    /* data will be in r3 */
    CAPRI_RING_DOORBELL_DATA(0, k.common_phv_fid,
                        TCP_SCHED_RING_CLEAN_RETX, 0)
    memwr.dx.e      r4, r3
    nop

end_program:
    nop.e
    nop

tcp_retx_retransmit:
    phvwr           p.t0_s2s_snd_nxt, d.retx_snd_una
    phvwri          p.to_s5_pending_tso_data, 1
    nop.e
    nop


tcp_retx_ack_send:
    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN,
                        tcp_xmit_ack_process_start,
                        k.common_phv_qstate_addr,
                        TCP_TCB_XMIT_OFFSET, TABLE_SIZE_512_BITS)
    /*
     * Check if we need to send ack, else clear table_valid
     */
    seq             c1, k.common_phv_pending_dup_ack_send, 1
    seq             c2, d.last_ack, k.t0_s2s_rcv_nxt
    setcf           c3, [!c1 & c2]
    tblwr.e         d.last_ack, k.t0_s2s_rcv_nxt
    phvwri.c3       p.{app_header_table0_valid...app_header_table3_valid}, 0

tcp_retx_fw2_tw_to:
tcp_retx_ka_to_rst:
    seq             c1, k.common_phv_rst, 1
    tblwr.c1        d.tx_rst_sent, 1
    b.!c1           tcp_retx_drop // Must be tw_to
    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN,
                        tcp_xmit_rst_process_start,
                        k.common_phv_qstate_addr,
                        TCP_TCB_XMIT_OFFSET, TABLE_SIZE_512_BITS)
    nop.e
    nop

tcp_send_keepa:
    seq             c1, k.common_phv_rst, 1
    b.c1            tcp_retx_ka_to_rst
    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN,
                        tcp_xmit_keepalive_probe,
                        k.common_phv_qstate_addr,
                        TCP_TCB_XMIT_OFFSET, TABLE_SIZE_512_BITS)
    nop.e
    nop

tcp_retx_launch_sack_rx:
    CAPRI_NEXT_TABLE_READ_OFFSET(1, TABLE_LOCK_DIS,
                        tcp_tx_sack_rx,
                        k.common_phv_qstate_addr,
                        TCP_TCB_OOO_BOOK_KEEPING_OFFSET0, TABLE_SIZE_512_BITS)
    jr              r7
    nop

tcp_retx_drop:
    phvwri.e        p.p4_intr_global_drop, 1
    CAPRI_CLEAR_TABLE_VALID(0)

