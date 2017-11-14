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

struct phv_ p;
struct s4_t0_tcp_tx_k k;
struct s4_t0_tcp_tx_tcp_tx_d d;

%%
    .align
    .param          tcp_tso_process_start
    .param          TNMDR_GC_TABLE_BASE
    .param          RNMDR_GC_TABLE_BASE

tcp_tx_process_start:
    seq             c1, k.common_phv_pending_snd_una_update, 1
    bcf             [c1], tcp_tx_snd_una_update

    seq             c1, k.common_phv_pending_rto, 1
    bcf             [c1], tcp_tx_retransmit

tcp_tx_enqueue:
    phvwr           p.t0_s2s_snd_nxt, d.snd_nxt
    /* check SESQ for pending data to be transmitted */
    seq             c1, k.common_phv_pending_sesq, 1
    seq.!c1         c1, k.common_phv_pending_asesq, 1

    bal.c1          r7, tcp_retx_enqueue
    nop

    /* Check if there is retx q cleanup needed at head due
     * to ack
     */
    slt             c1, d.retx_snd_una, k.common_phv_snd_una
    bal.c1          r7, tcp_clean_retx_queue
    nop

    /* Check if cwnd allows transmit of data */
    /* Return value in r6 */
    /* cwnd permits transmit of data if r6 != 0*/
    bal             r7, tcp_cwnd_test
    nop
    sne             c1, r6, r0

    /* Check if peer rcv wnd allows transmit of data
     * Return value in r6
     * Peer rcv wnd allows transmit of data if r6 != 0
     */
    bal.c1          r7, tcp_snd_wnd_test
    nop
    sne             c2, r6, r0

    seq             c3, k.common_phv_pending_ack_send, 1
    bcf             [c3], table_read_TSO
    nop

    bcf             [!c1 | !c2], tcp_tx_end_program
    nop
    /* Inform TSO stage following later to check for any data to
     * send from retx queue
     */
    tblwr           d.pending_tso_data, 1
    phvwri          p.to_s5_pending_tso_data, 1

flow_read_xmit_cursor_start:
    /* Get the point where we are supposed to read from */
    seq             c1, d.xmit_cursor_addr, r0
    bcf             [c1], flow_read_xmit_cursor_done
    add             r1, d.xmit_cursor_addr, r0

    // TODO : r1 needs to be capped by the window size
    add             r1, d.xmit_len, r0
    phvwr           p.to_s5_xmit_cursor_addr, d.xmit_cursor_addr
    phvwr           p.to_s5_xmit_cursor_len, r1
    tbladd          d.snd_nxt, r1

rearm_rto:
    CAPRI_OPERAND_DEBUG(k.t0_s2s_rto)

    /*
     * r1 = rto
     *    = min(rto << backoff, TCP_RTO_MAX)
     *
     * TODO: rto_backoff needs to be reset upon indication from rx
     * pipeline (rx2tx_extra_pending_reset_backoff)
     */
    add             r1, r0, k.t0_s2s_rto
    sll             r1, r1, d.rto_backoff
    slt             c1, r1, TCP_RTO_MAX
    add.!c1         r1, r0, TCP_RTO_MAX

    // TODO : use slow timer just for testing purposes
    // result will be in r3
    CAPRI_TIMER_DATA(0, k.common_phv_fid, TCP_SCHED_RING_RTO, r1)

    // TODO : using slow timer just for testing
    addi            r4, r0, CAPRI_SLOW_TIMER_ADDR(LIF_TCP)
    seq             c1, k.common_phv_debug_dol_dont_start_retx_timer, 1
    memwr.dx.!c1    r4, r3
    tbladd          d.rto_pi, 1

    // TODO : this needs to account for TSO, for now assume one packet
    tbladd          d.packets_out, 1

    // TODO : Lot of stuff to do here:
    //      if window size is smaller, move xmit_cursor_addr by appropriate amount
    //      if we have more data than one descriptor, adjust next pointer and
    //      schedule ourselves again to send more data
    // TODO : right now code assumes we always send
    //        whatever is posted to sesq, so set cursor_addr back to 0
    tblwr           d.xmit_cursor_addr, r0
    phvwr           p.tx2rx_snd_nxt, d.snd_nxt

table_read_TSO:
    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN,
                        tcp_tso_process_start, k.common_phv_qstate_addr,
                        TCP_TCB_TSO_OFFSET, TABLE_SIZE_512_BITS)
    nop.e
    nop

table_read_xmit_cursor:
    /* Read the xmit cursor if we have zero xmit cursor addr */
    add.c1          r1, d.retx_xmit_cursor, r0

flow_read_xmit_cursor_done:



tcp_cwnd_test:
    /* in_flight = packets_out - (sacked_out + lost_out) + retrans_out
    */
    /* r1 = packets_out + retrans_out */
    add             r1, d.packets_out, k.t0_s2s_retrans_out
    /* r2 = left_out = sacked_out + lost_out */
    add             r2, d.sacked_out, k.t0_s2s_lost_out
    /* r1 = in_flight = (packets_out + retrans_out) - (sacked_out + lost_out)*/
    sub             r1, r1, r2
    /* c1 = (in_flight >= cwnd) */
    sle             c1, k.t0_s2s_snd_cwnd, r1
    bcf             [c1], tcp_cwnd_test_done
    /* no cwnd remaining */
    addi.c1         r6, r0, 0
    /* r6 = snd_cwnd >> 1 */
    add             r6, k.t0_s2s_snd_cwnd, r0
    srl             r6, r6, 1

    addi            r4, r0, 1
    slt             c2, r6,r4
    add.c2          r6, r4, r0
    /* at this point r6 = max(cwnd >> 1, 1) */
    /* r5 = cwnd - in_flight */
    sub             r5, k.t0_s2s_snd_cwnd, r1
    slt             c2, r5, r6
    add.c2          r6, r5, r0
    /* At this point r6 = min((max(cwnd >>1, 1) , (cwnd - in_flight)) */
tcp_cwnd_test_done:
    sne             c4, r7, r0
    jr.c4           r7
    add             r7, r0, r0

tcp_snd_wnd_test:
    /* r1 = bytes_sent = snd_nxt - snd_una */
    sub             r1, d.snd_nxt, k.common_phv_snd_una
    /* r1 = snd_wnd - bytes_sent */
    sub             r1, k.t0_s2s_snd_wnd, r1
    /* r1 = (snd_wnd - bytes_sent) / mss_cache */
    add             r5, k.to_s4_rcv_mss_shft, r0
    srlv            r6, r1, r5
    sne             c4, r7, r0
    jr.c4           r7
    add             r7, r0, r0



tcp_retx_enqueue:
    //tblwr           d.retx_tail_desc, r0
    //nop 
    seq             c1, d.retx_tail_desc, r0
    bcf             [!c1], queue_to_tail

retx_empty:
    /*
     * retx empty, update head/tail/xmit desc and cursors
     */
    tblwr           d.retx_head_desc, k.to_s4_sesq_desc_addr
    tblwr           d.retx_tail_desc, k.to_s4_sesq_desc_addr
    tblwr           d.xmit_desc, k.to_s4_sesq_desc_addr
    tblwr           d.retx_next_desc, r0
    tblwr           d.xmit_next_desc, r0

    add             r2, k.to_s4_addr, k.to_s4_offset

    tblwr           d.retx_xmit_cursor, r2
    tblwr           d.retx_head_offset, k.to_s4_offset
    tblwr           d.retx_head_len, k.to_s4_len

    tblwr           d.xmit_cursor_addr, r2
    tblwr           d.xmit_offset, k.to_s4_offset
    tblwr           d.xmit_len, k.to_s4_len
    tblwr           d.retx_snd_una, k.common_phv_snd_una
    sne             c1, k.common_phv_debug_dol_free_rnmdr, r0
    bcf             [c1], free_rnmdr
    sne             c4, r7, r0
    jr.c4           r7
    nop

queue_to_tail:
    /*
     * If retx_tail is not NULL, queue to tail, update tail and return
     */
    add             r1, d.retx_tail_desc, NIC_DESC_ENTRY_NEXT_ADDR_OFFSET
    memwr.w         r1, k.to_s4_sesq_desc_addr
    tblwr           d.retx_tail_desc, k.to_s4_sesq_desc_addr
    seq             c1, d.retx_next_desc, r0
    tblwr.c1        d.retx_next_desc, k.to_s4_sesq_desc_addr

    /*
     * If xmit_cursor_addr is NULL, update xmit_cursor_addr
     */
    seq             c1, d.xmit_cursor_addr, r0
    b.!c1           queue_to_tail_end
    add             r2, k.to_s4_addr, k.to_s4_offset
    tblwr           d.xmit_desc, k.to_s4_sesq_desc_addr
    tblwr           d.xmit_cursor_addr, r2
    tblwr           d.xmit_offset, k.to_s4_offset
    tblwr           d.xmit_len, k.to_s4_len
queue_to_tail_end:
    jr              r7

free_rnmdr:
    // TODO: just for testing, fix this once retx is implemented
    sub             r3, k.to_s4_sesq_desc_addr, NIC_DESC_ENTRY_0_OFFSET
    phvwr           p.ring_entry_descr_addr, r3
    addui           r1, r0, hiword(RNMDR_GC_TABLE_BASE)
    addi            r1, r0, loword(RNMDR_GC_TABLE_BASE)
    add             r1, r1, RNMDR_GC_PRODUCER_TCP, RNMDR_GC_PER_PRODUCER_SHIFT
    CAPRI_DMA_CMD_PHV2MEM_SETUP(ringentry_dma_dma_cmd, r1, ring_entry_descr_addr, ring_entry_descr_addr)
    CAPRI_DMA_CMD_RING_DOORBELL2(doorbell_dma_dma_cmd, LIF_GC, 0,
                    CAPRI_HBM_GC_RNMDR_QID, CAPRI_RNMDR_GC_TCP_RING_PRODUCER,
                    0, db_data_pid, db_data_index)
    sne             c4, r7, r0
    jr.c4           r7
    nop

free_tnmdr:
    // TODO: just for testing, fix this once retx is implemented
    sub             r3, k.to_s4_sesq_desc_addr, NIC_DESC_ENTRY_0_OFFSET
    phvwr           p.ring_entry_descr_addr, r3
    addui           r1, r0, hiword(TNMDR_GC_TABLE_BASE)
    addi            r1, r0, loword(TNMDR_GC_TABLE_BASE)
    add             r1, r1, TNMDR_GC_PRODUCER_TCP, TNMDR_GC_PER_PRODUCER_SHIFT
    CAPRI_DMA_CMD_PHV2MEM_SETUP(ringentry_dma_dma_cmd, r1, ring_entry_descr_addr, ring_entry_descr_addr)
    CAPRI_DMA_CMD_RING_DOORBELL2(doorbell_dma_dma_cmd, LIF_GC, 0,
                    CAPRI_HBM_GC_TNMDR_QID, CAPRI_TNMDR_GC_TCP_RING_PRODUCER,
                    0, db_data_pid, db_data_index)
    sne             c4, r7, r0
    jr.c4           r7
    nop

tcp_clean_retx_queue:
    // TODO: schedule ourselves to clean retx
    jr              r7
    nop

tcp_tx_end_program:
    // We have no window, wait till window opens up
    CAPRI_CLEAR_TABLE_VALID(0)
    nop.e
    nop

tcp_tx_snd_una_update:
    CAPRI_CLEAR_TABLE_VALID(0)
    // TODO : if this exceeds mtu, we need to decrement packets_out
    sub             r1, k.common_phv_snd_una, d.retx_snd_una
    slt             c1, r1, d.retx_head_len
    b.!c1           tcp_tx_snd_una_update_free_head
    tbladd          d.retx_snd_una, r1
    tbladd.e        d.retx_head_offset, r1
    tblsub          d.retx_head_len, r1
    tbladd          d.retx_xmit_cursor, r1

tcp_tx_snd_una_update_free_head:
    // TODO : free d.retx_head_desc

    // write new descriptor address, offset and length
    tblwr           d.retx_head_desc, d.retx_next_desc
    tblwr           d.retx_next_desc, k.t0_s2s_next_addr
    tblwr           d.retx_head_offset, k.to_s4_offset
    tblwr           d.retx_head_len, k.to_s4_len
    add             r2, k.to_s4_addr, k.to_s4_offset
    tblwr           d.retx_xmit_cursor, r2
    
    // TODO : this needs to account for MTU
    tblsub          d.packets_out, 1
    // This effectively cancels retransmission timer
    tbladd          d.rto_pi, 1
    
    // If we have completely cleaned up, set tail to NULL
    seq             c1, d.retx_head_desc, r0
    tblwr.c1        d.retx_tail_desc, r0

    /*
     * if we still have more data to be cleaned up,
     * schedule ourselves again
     */
    sub             r1, k.common_phv_snd_una, d.retx_snd_una
    slt             c1, r1, d.retx_head_len
    // TODO : schedule again
    nop.e
    nop

tcp_tx_retransmit:
    seq             c1, d.rto_pi, k.t0_s2s_rto_pi
    b.!c1           tcp_tx_end_program // old timer, ignore it
    phvwr           p.t0_s2s_snd_nxt, d.retx_snd_una
    phvwr           p.to_s5_xmit_cursor_addr, d.retx_xmit_cursor
    phvwr           p.to_s5_xmit_cursor_len, d.retx_head_len
    phvwri          p.to_s5_pending_tso_data, 1
    b               rearm_rto
    tbladd          d.rto_backoff, 1
