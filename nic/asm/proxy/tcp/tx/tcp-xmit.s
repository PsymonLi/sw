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
#include "INGRESS_s4_t0_tcp_tx_k.h"

struct phv_ p;
struct s4_t0_tcp_tx_k_ k;
struct s4_t0_tcp_tx_xmit_d d;

%%
    .align
    .param          tcp_tso_process_start
    .param          TCP_PROXY_STATS

    /*
     * r4 = current timestamp. Do not use this register
     */

#define c_sesq c5
#define c_snd_una c6
#define c_retval c7

#define r_linkaddr r7

tcp_xmit_process_start:
    seq             c_sesq, k.common_phv_pending_sesq, 1
    seq.!c_sesq     c_sesq, k.common_phv_pending_asesq, 1
    seq             c_snd_una, k.common_phv_pending_retx_cleanup, 1

    seq             c1, k.t0_s2s_state, TCP_RST
    bcf             [c1 & !c_snd_una], tcp_tx_end_program_and_drop_rst
    nop
    seq             c1, k.to_s4_rtt_seq_req, 1
    sne             c2, d.rtt_seq_req, 1
    setcf           c1, [c1 & c2]
    tblwr.c1        d.rtt_seq_req, 1
               
    bcf             [c_snd_una], tcp_tx_xmit_snd_una_update
    nop
    seq             c1, k.common_phv_pending_per_to, 1
    slt             c2, 0, d.persist_s
    bcf             [c1 & c2], tcp_xmit_persist_probe
    nop
    bbeq            k.common_phv_pending_rto, 1, tcp_tx_retransmit
    nop

    bbeq            k.common_phv_pending_fast_retx, 1, rearm_rto

tcp_tx_enqueue:
    /*
     * If we have no window, bail out
     */
    seq             c1, d.no_window, 1
    seq             c2, k.to_s4_window_open, 1
    bcf             [c1 & !c2], tcp_tx_end_program_and_drop
    nop
    balcf           r6, [c1 & c2], tcp_xmit_stop_unstop_sesq_prod
    tblwr           d.no_window, 0
    
    phvwr           p.t0_s2s_snd_nxt, d.snd_nxt

    seq             c1, k.common_phv_fin, 1
    bal.c1          r_linkaddr, tcp_tx_handle_fin
    nop

    /* 
     * Check if window allows transmit of data.
     * Return value in c_retval
     */
    bal             r_linkaddr, tcp_snd_wnd_test
    nop
    b.!c_retval     tcp_tx_no_window
    nop

    /* Inform TSO stage following later to check for any data to
     * send from retx queue
     */
    tblwr           d.persist_s, 0
    phvwri          p.to_s5_pending_tso_data, 1

flow_read_xmit_cursor_start:
    /* Get the point where we are supposed to read from */
    seq             c1, k.t0_s2s_addr, r0
    bcf             [c1], flow_read_xmit_cursor_done

    // TODO : r1 needs to be capped by the window size
    add             r1, k.t0_s2s_len, r0
    tbladd          d.snd_nxt, r1
    seq             c2, d.rtt_seq_req, 1 
    phvwr.c2        p.tx2rx_rtt_seq, d.snd_nxt
    tblwr.c2        d.rtt_seq, d.snd_nxt
    phvwr.!c2       p.tx2rx_rtt_seq, d.rtt_seq
#ifdef HW
    srl.c2          r3, r4, 13
#else
    addi.c2          r3, r0, 0xFEEDFED0
    srl.c2          r3, r3, 13
#endif
    phvwr.c2        p.tx2rx_rtt_time, r3
    tblwr.c2        d.rtt_time, r3
    phvwr.!c2       p.tx2rx_rtt_time, d.rtt_time
    tblwr.c2        d.rtt_seq_req, r0           
    sne             c1, d.packets_out, 0
    bcf             [c1], rearm_rto_done
    nop 

rearm_rto:
#ifndef HW
    bbeq            k.common_phv_debug_dol_dont_start_retx_timer, 1, rearm_rto_done
    tblwr           d.rto_backoff, r0
#endif
    seq             c1, d.rto_backoff, TCP_MAX_RTO_COUNT
    b.c1            tcp_retrans_timeout_lim
    
    CAPRI_OPERAND_DEBUG(k.to_s4_rto)

    /*
     * r2 = rto
     *    = min(rto << backoff, TCP_RTO_MAX)
     */
    sll             r2, k.to_s4_rto, d.rto_backoff
    slt             c1, r2, TCP_RTO_MAX_TICK
    add.!c1         r2, r0, TCP_RTO_MAX_TICK

    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_RTO_OFFSET
    memwr.h         r1, r2
    add.c1          r2, r0, r0
    add.c1          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_TTYPERTO_TO_OFFSET
    memwr.h.c1      r1, r2

rearm_rto_done:

    tbladd.c_sesq   d.packets_out, 1

    phvwr           p.tx2rx_snd_nxt, d.snd_nxt
    phvwr           p.tx2rx_fin_sent, d.fin_sent
table_read_TSO:
    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN,
                        tcp_tso_process_start, k.common_phv_qstate_addr,
                        TCP_TCB_TSO_OFFSET, TABLE_SIZE_512_BITS)
    nop.e
    nop

flow_read_xmit_cursor_done:
    phvwri          p.p4_intr_global_drop, 1
    CAPRI_CLEAR_TABLE_VALID(0)
    nop.e
    nop


/*
 * Return true send window available
 * 1) check if snd_wnd available
 * 2) check if cwnd available
 * 3) check if limited transmit allowed if #1 is true, and #2 in false
 */
tcp_snd_wnd_test:
    
    add             r1, d.snd_nxt, k.t0_s2s_len
    sll             r2, k.t0_s2s_snd_wnd, d.snd_wscale
    add             r2, r2, k.common_phv_snd_una
    scwlt           c3, r2[31:0], r1[31:0]
    jr.c3           r_linkaddr 
    setcf.c3        c_retval, [!c3]
    add             r3, k.to_s4_snd_cwnd, k.common_phv_snd_una
    scwlt           c3, r3[31:0], r1[31:0]
    slt             c2, d.limited_transmit, k.t0_s2s_limited_transmit
    
    // window not available but limited_transmit != 0
    // incr local copy to keep track of how many packets
    // we have sent due to limited_transmit
    setcf           c1, [c3 & c2]
    tbladd.c1       d.limited_transmit, 1

    // reset limited_transmit to 0 if not set in rx pipeline
    seq             c4, k.t0_s2s_limited_transmit, 0
    tblwr.c4        d.limited_transmit, 0

    // Return TRUE if window available or can send due to limited transmit
    jr              r_linkaddr
    setcf           c_retval, [c1 | !c3]


tcp_tx_handle_fin:
    seq             c1, k.t0_s2s_state, TCP_ESTABLISHED
    seq             c2, k.t0_s2s_state, TCP_CLOSE_WAIT
    setcf           c1, [c1 | c2]
    tbladd.c1       d.snd_nxt, 1
    tblwr           d.fin_sent, 1
    jr              r_linkaddr
    nop

tcp_tx_xmit_snd_una_update:
    seq             c1, k.to_s4_sesq_ci_update, 1
    bal.c1          r6, tcp_xmit_stop_unstop_sesq_prod 
    tblwr.c1        d.clean_retx_ci, k.to_s4_clean_retx_ci
    /*
     * reset rto_backoff if snd_una advanced
     * TODO : We should reset backoff to 0 only if we receive an 
     * ACK for a packet that was not retransmitted. We can piggyback
     * this logic to RTT when that is implemented. For now reset it 
     * for all acks that advance snd_una
     */
    tblwr           d.rto_backoff, 0

    tblssub         d.packets_out, k.t0_s2s_packets_out_decr
    seq             c1, d.packets_out, 0
    seq             c2, k.t0_s2s_state, TCP_FIN_WAIT2
    seq             c3, k.t0_s2s_state, TCP_TIME_WAIT
    setcf           c2, [c2 | c3]
    setcf           c3, [!c1 & !c2]
    setcf           c1, [c1 & !c2]
    // if packets_out == 0 | state != FW2 or TW
    //      cancel retx timer
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_RTO_OFFSET
    memwr.h.c1      r1, 0
    memwr.h.c3      r1, k.to_s4_rto 

tcp_tx_end_program:
    // We have no window, wait till window opens up
    // or we are only running congestion control algorithm,
    // so no more stages
    CAPRI_CLEAR_TABLE_VALID(0)
    nop.e
    nop

tcp_tx_end_program_and_drop:
    phvwri          p.p4_intr_global_drop, 1
    CAPRI_CLEAR_TABLE_VALID(0)
    nop.e
    nop

tcp_tx_end_program_and_drop_rst:
    // drop the frame, but trigger retx queue cleanup
    CAPRI_CLEAR_TABLE_VALID(0)
    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_INC,
                        DB_SCHED_UPD_EVAL, 0, LIF_TCP)
    /* data will be in r3 */
    CAPRI_RING_DOORBELL_DATA(0, k.common_phv_fid,
                        TCP_SCHED_RING_CLEAN_RETX, 0)
    memwr.dx.e      r4, r3
    phvwri          p.p4_intr_global_drop, 1

tcp_tx_retransmit:
    seq             c1, d.snd_nxt, k.common_phv_snd_una
    b.c1            tcp_tx_end_program_and_drop
    nop.c1

    tbladd          d.retx_cnt, 1
    // Change snd_cwnd in rx2tx CB, so we don't have a stale value there
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_EXTRA_SND_CWND_OFFSET
    memwr.w         r1, d.smss

    // indicate to rx pipeline that timeout occured
    phvwr           p.tx2rx_rto_event, 1

    // Incr retx pkt stat
    addui           r2, r0, hiword(TCP_PROXY_STATS)
    addi            r2, r2, loword(TCP_PROXY_STATS)
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r2, TCP_PROXY_STATS_RETX_PKTS, 1)
 
    b               rearm_rto
    tbladd          d.rto_backoff, 1

tcp_tx_no_window:
    tbladd          d.window_full_cnt, 1
    // Bail out, but remember the current ci in stage 0 CB
    bal             r6, tcp_xmit_stop_unstop_sesq_prod
    tblwr           d.no_window, 1
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_TX_CI_OFFSET
    memwr.h         r1, k.to_s4_sesq_tx_ci
#ifndef HW
    bbeq            k.common_phv_debug_dol_dont_start_retx_timer, 1, tcp_tx_end_program_and_drop 
#endif
    seq             c1, d.snd_nxt, k.common_phv_snd_una
    b.!c1           tcp_tx_end_program_and_drop
    nop
    tblwr           d.persist_s, 25
#ifdef HW
    sll             r2, k.to_s4_rto, d.rto_backoff
    slt             c1, r2, TCP_MAX_PERSIST_TICK
    add.!c1         r2, r0, TCP_MAX_PERSIST_TICK
#else
    add             r2, r0, 1
#endif
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_RTO_OFFSET
    memwr.h         r1, r2
    add             r2, 1, r0
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_TTYPERTO_TO_OFFSET
    memwr.b         r1, r2
    b               tcp_tx_end_program_and_drop
    nop

tcp_xmit_persist_probe:
    phvwr           p.t0_s2s_len, 1
    tblssub         d.persist_s, 1
    seq             c1, d.persist_s, 0
    /*   Once d.persist_s gets to zero, we will need to RST --- tbd */
    /* branch to RST code */
    b.c1            tcp_tx_end_program_and_drop 
    nop
#ifndef HW
    bbeq            k.common_phv_debug_dol_dont_start_retx_timer, 1, rearm_rto_done
    add             r2, r0, 1 
#else
    sll             r2, k.to_s4_rto, d.rto_backoff
    slt             c1, r2, TCP_MAX_PERSIST_TICK
    add.!c1         r2, r0, TCP_MAX_PERSIST_TICK
#endif
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_RTO_OFFSET
    memwr.h         r1, r2
    add             r2, 1, r0
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_TTYPERTO_TO_OFFSET
    memwr.b         r1, r2
    b               rearm_rto_done
    nop

tcp_retrans_timeout_lim: 
    phvwr           p.t0_s2s_snd_nxt, d.snd_nxt
    phvwr           p.tx2rx_snd_nxt, d.snd_nxt
    phvwr           p.tx2rx_rst_sent, 1
    phvwr           p.common_phv_rst, 1
    phvwr           p.to_s5_int_rst_rsn, LOCAL_RST_RTO
    phvwr           p.t0_s2s_len, r0
    phvwrmi         p.tcp_header_flags, TCPHDR_ACK | TCPHDR_RST, \
                            TCPHDR_ACK | TCPHDR_RST
    b               rearm_rto_done
    nop

tcp_xmit_stop_unstop_sesq_prod:
    add          r1, r0, d.sesq_ci_addr
    add          r2, d.clean_retx_ci, d.no_window, SESQ_END_MARK_FLAG_BIT_SHIFT
    memwr.h      r1, r2
    jr           r6
    nop
     
/*
 * Idle timer expired, reduce cwnd and quit
 */
.align
tcp_xmit_idle_process_start:
    /*
     * RFC 5681 (Restarting Idle Connections)
     * cwnd = min(cwnd, initial_window)
     */
    add             r2, r0, k.to_s4_snd_cwnd
    slt             c1, d.initial_window, r2
    add.c1          r2, r0, d.initial_window

    add             r1, k.common_phv_qstate_addr, TCP_TCB_CC_SND_CWND_OFFSET
    memwr.w         r1, r2
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_EXTRA_SND_CWND_OFFSET
    memwr.w         r1, r2
    phvwri.e        p.p4_intr_global_drop, 1
    CAPRI_CLEAR_TABLE_VALID(0)
