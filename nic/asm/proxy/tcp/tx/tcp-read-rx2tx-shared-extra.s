/*
 *	Implements the rx2tx shared extra state read stage of the TxDMA P4+ pipeline
 */

#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s1_t0_tcp_tx_k.h"

struct phv_ p;
struct s1_t0_tcp_tx_k_ k;
struct s1_t0_tcp_tx_read_rx2tx_extra_d d;

%%
    .align
    .param      tcp_tx_s2_bubble_start

tcp_tx_read_rx2tx_shared_extra_stage1_start:
#ifdef CAPRI_IGNORE_TIMESTAMP
    add             r4, r0, r0
    add             r6, r0, r0
#endif
    CAPRI_OPERAND_DEBUG(k.common_phv_pending_rx2tx)
    phvwr           p.common_phv_snd_una, d.snd_una
    or              r1, d.t_flags, TCPHDR_ACK
    phvwr           p.tcp_header_flags, r1
    phvwrpair       p.t0_s2s_snd_wnd, d.snd_wnd, \
                        p.t0_s2s_state, d.state
    // HACK: Force a timer of 100 ticks
#ifdef HW
    phvwr       p.to_s4_snd_cwnd, d.snd_cwnd
    phvwr       p.to_s4_rto, d.rto 
#else
    // Set it to 1 tick in simulation so timer immediately expires
    // on stepping it
    phvwrpair       p.to_s4_snd_cwnd, d.snd_cwnd, \
                        p.to_s4_rto, 1
#endif
    phvwr           p.to_s4_rtt_seq_req, d.rtt_seq_req_
    phvwr           p.to_s5_rcv_wnd, d.rcv_wnd
    phvwrpair       p.t0_s2s_limited_transmit, d.limited_transmit[1:0], \
                        p.t0_s2s_rcv_nxt, d.rcv_nxt

    phvwr           p.to_s5_ts_ecr, d.rcv_tsval
    phvwrpair       p.to_s5_ts_offset, d.ts_offset, \
                        p.to_s5_ts_time, d.ts_time 

    seq             c1, d.launch_sack_rx, 1
    phvwr.c1        p.common_phv_launch_sack_rx, 1

    // When dup_ack is set, use dup_rcv_nxt
    phvwr           p.common_phv_pending_dup_ack_send, d.pending_dup_ack_send
    seq             c1, d.pending_dup_ack_send, 1
    phvwr.c1        p.t0_s2s_rcv_nxt, d.dup_rcv_nxt
    seq             c1, k.common_phv_pending_tw_fw2_to, 1
    bcf             [c1], fw2_tw_pending
    nop
    seq             c1, k.common_phv_pending_rx2tx, 1
    bcf             [c1], tcp_tx_start_pending
    nop
    nop.e
    nop
tcp_tx_start_pending:
    // Debug : Don't send ack based on dol flag
    seq             c1, k.common_phv_debug_dol_dont_send_ack, 1
    seq             c3, k.common_phv_pending_ack_send, 1
    /*
     * For snd_una_update, the next stage is launched by pending stage,
     * so skip launching the next stage here. Also if we are dropping
     * the PHV, then set global_drop bit
     */
    phvwri.c1       p.app_header_table0_valid, 0
    phvwri.c1       p.p4_intr_global_drop, 1
    bcf             [!c1 & c3], pending_ack
    nop
    bcf             [c1], tcp_tx_rx2tx_extra_end
    nop

fw2_tw_pending:
    seq             c1, d.state, TCP_FIN_WAIT2
    /*
     * If TW TO, send close ind right here.
     * Note: For all cases of send RST, close ind is sent in tcp-tso stage
     * That will cover FW2 TO case
     */
    b.!c1           ring_db_ccq_close_ind
    phvwr.c1        p.common_phv_rst, 1
    phvwr           p.to_s5_int_rst_rsn, LOCAL_RST_FW2_TO
pending_ack:
    /*
     * For pending_ack_send, we need to launch the bubble stage
     */
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tcp_tx_s2_bubble_start)

tcp_tx_rx2tx_extra_end:
    nop.e
    nop

ring_db_ccq_close_ind:
    // Prep DB addr in r2 for qtype 1, TCP LIF
    addi            r2, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_SET,
                        DB_SCHED_UPD_EVAL, TCP_OOO_RX2TX_QTYPE, LIF_TCP)
    /*
     * Prep DB data for cleanup cond 2, Data will be in r3
     * Note: As long as the prod value is non zero, the actual value otherwise has
     * no significance for indicating a close on the ccq ring.
     * With that, here  we are using the prod to pass reason code
     */
    CAPRI_RING_DOORBELL_DATA(0, k.common_phv_fid, TCP_CCQ_CONN_CLOSED_IND_RING,
          ACTIVE_GRACEFUL)
    // DB write
    memwr.dx        r2, r3
    CAPRI_CLEAR_TABLE0_VALID
    b               tcp_tx_rx2tx_extra_end
    phvwri          p.p4_intr_global_drop, 1

