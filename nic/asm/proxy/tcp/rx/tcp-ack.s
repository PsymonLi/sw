/*
 *  TCP ACK processing (ack received from peer)
 *
 *  Update snd_una, and inform tx pipeline
 *
 *  Handle dup_acks and fast retransmissions
 *
 *  Handle window change, and inform tx pipeline
 */


#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp-constants.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s2_t0_tcp_rx_k.h"

struct phv_ p;
struct s2_t0_tcp_rx_k_ k;
struct s2_t0_tcp_rx_tcp_ack_d d;

%%
    .align
    .param          tcp_rx_rtt_start
    .param          tcp_ack_slow

#define c_est c6

tcp_ack_start:
    bbeq            k.common_phv_ooq_tx2rx_pkt, 1, tcp_ack_skip
    /*
     * Pure data, ack_seq hasn't advanced
     *
     * fast path if
     *      state == ESTABLISHED &&
     *      (flag & FLAG_DATA) == FLAG_DATA &&
     *      ack_seq == snd_una &&
     *      not in recovery
     */
    smeqb           c1, k.to_s2_flag, FLAG_DATA, FLAG_DATA
    // and             r1, k.to_s2_flag, FLAG_DATA
    // seq             c1, r1, FLAG_DATA
    seq             c3, d.snd_una, k.s1_s2s_ack_seq
    seq             c4, d.cc_flags, 0
    seq             c_est, d.state, TCP_ESTABLISHED
    bcf             [c1 & c3 & c4 & c_est], tcp_ack_done

    /*
     * Ack has advanced
     *
     * fast path if
     *      state == ESTABLISHED &&
     *      !(flag & FLAG_SLOWPATH) &&
     *      not in recovery &&
     *      ack_seq <= snd_nxt &&
     *      ack_seq > snd_una
     */
    smeqb           c5, k.to_s2_flag, FLAG_SLOWPATH, FLAG_SLOWPATH
    scwlt           c2, k.s1_s2s_snd_nxt, k.s1_s2s_ack_seq
    /* 
     * Note: tcp_ack_slow assumes c2 will hve indication of ACK for sent data
     * So dont change it without corresponding change in tcp_ack_slow
     */
    scwle           c3, k.s1_s2s_ack_seq, d.snd_una
    sne             c4, d.cc_flags, 0
    setcf           c7, [c5 | c2 | c3 | c4 | !c_est]
    j.c7            tcp_ack_slow
    nop

tcp_ack_fast:
    /* At this point we know that ACK has incremented beyond snd_una
    /* If Data is not present (Note: c1 still holds this flag),
     * then it is valid Pure ACK
     */ 
    bcf             [c1], proc_tcp_ack_fast
    nop
pure_acks_rcvd_stats_update_start:
    CAPRI_STATS_INC(pure_acks_rcvd, 1, d.pure_acks_rcvd, p.to_s7_pure_acks_rcvd)
pure_acks_rcvd_stats_update_end:

proc_tcp_ack_fast: 
    phvwri          p.common_phv_process_ack_flag, 1
    phvwrpair       p.to_s4_cc_ack_signal, TCP_CC_ACK_SIGNAL, \
                        p.to_s4_cc_flags, d.cc_flags
tcp_update_wl_fast:
    tblwr           d.snd_wl1, k.s1_s2s_ack_seq
tcp_snd_una_update_fast:
    sub             r1, k.s1_s2s_ack_seq, d.snd_una
bytes_acked_stats_update_start:
    CAPRI_STATS_INC(bytes_acked, r1[31:0], d.bytes_acked, p.to_s7_bytes_acked)
bytes_acked_stats_update_end:
    phvwr           p.to_s4_bytes_acked, r1[31:0]
    tblwr           d.snd_una, k.s1_s2s_ack_seq

    /*
     * tell txdma we have work to do
     */
    phvwrmi         p.common_phv_pending_txdma, TCP_PENDING_TXDMA_SND_UNA_UPDATE, \
                        TCP_PENDING_TXDMA_SND_UNA_UPDATE

    /*
     * Launch next stage
     */
tcp_ack_done:
    tblwr           d.num_dup_acks, 0
    tblwr           d.limited_transmit, 0
    tblwr.f         d.snd_wnd, k.to_s2_window
tcp_ack_skip:
    phvwr           p.to_s4_snd_wnd, k.to_s2_window
    phvwrpair       p.rx2tx_extra_snd_wnd, k.to_s2_window, \
                        p.rx2tx_extra_snd_una, d.snd_una
    phvwr           p.rx2tx_extra_limited_transmit, d.limited_transmit
    CAPRI_NEXT_TABLE_READ_OFFSET_e(0, TABLE_LOCK_EN,
                        tcp_rx_rtt_start,
                        k.common_phv_qstate_addr,
                        TCP_TCB_RTT_OFFSET, TABLE_SIZE_512_BITS)
    nop

