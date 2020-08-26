/*
 *  Implements the RX stage of the RxDMA P4+ pipeline
 */

#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s2_t0_tcp_rx_k.h"

struct phv_ p;
struct s2_t0_tcp_rx_k_ k;
struct s2_t0_tcp_rx_d d;

%%
    .param          tcp_ack_start
    .param          tcp_rx_read_rnmdr_start
    .param          tcp_rx_read_rnmdr_fc
    .param          tcp_ooo_book_keeping
    .param          tcp_ooo_book_keeping_in_order
    .param          tcp_rx_s3_t0_bubble_drop_pkt
    .param          TCP_PROXY_STATS
    .align

    /*
     * Global variables
     *
     */
#define r_consumer_ring_mask r7

tcp_rx_process_start:
#ifdef CAPRI_IGNORE_TIMESTAMP
    add             r4, r0, r0
    add             r6, r0, r0
#endif

    // Fix for Capri bug - local writes are written back when both local and
    // non-local writes are present to the same d-vector
    tblwr.l         d.{u.tcp_rx_d.need_descr_L...u.tcp_rx_d.unused_flags_L}, 0

    sll             r_consumer_ring_mask, 1, d.u.tcp_rx_d.consumer_ring_shift
    sub             r_consumer_ring_mask, r_consumer_ring_mask, 1


    
    /* FP PAWS check */   
    scwlt           c1, k.s1_s2s_rcv_tsval, d.u.tcp_rx_d.ts_recent
tcp_fastpath_chks:    
    /*
     * Fast Path checks
     */
    sne.!c1         c1, d.u.tcp_rx_d.state, TCP_ESTABLISHED
    seq.!c1         c1, k.s1_s2s_fin_sent, 1
    sne             c2, d.u.tcp_rx_d.rcv_nxt, k.s1_s2s_seq
    scwlt           c3, k.s1_s2s_snd_nxt, k.s1_s2s_ack_seq

    /*
     * c4 = serq is full
     */
#ifdef HW
    seq             c4, k.to_s2_serq_cidx[15], 1
#else
    setcf           c4, [!c0]
#endif
    add.!c4         r2, d.u.tcp_rx_d.serq_pidx, 1
    and.!c4         r2, r2, r_consumer_ring_mask
    seq.!c4         c4, r2, k.to_s2_serq_cidx[14:0]

    /*
     * Header Prediction
     *
     * Checks if hlen and flags are as expected in fast path
     *
     * TODO : Do we care about window change here? (its handled in tcp_ack)
     *
     * TODO : check for hlen difference. The issue with checking for
     * hlen is that DOL and e2e have different hlens (e2e has
     * timestamp and dol does not). Either find a way to configure
     * the CBs accordingly, or add timestamp option to DOL
     */
    and             r1, k.common_phv_flags, TCPHDR_HP_FLAG_BITS
    sne             c5, r1, d.u.tcp_rx_d.pred_flags[23:16]
    seq             c6, d.u.tcp_rx_d.ooq_not_empty, 1
    seq.!c6         c6, k.common_phv_ooq_tx2rx_pkt, 1
    seq             c7, k.to_s2_rcv_wnd_adv, 0
    bcf             [c1 | c2 | c3 | c4 | c5 | c6 | c7], tcp_rx_slow_path
    nop
     /* set ts_recent */
    scwle           c1, k.s1_s2s_seq, k.to_s2_rcv_wup
    tblwr.c1        d.u.tcp_rx_d.ts_recent, k.s1_s2s_rcv_tsval 
    /* set the TS ECR */
    phvwr           p.rx2tx_extra_rcv_tsval, d.u.tcp_rx_d.ts_recent
tcp_rx_fast_path:
    // reset keepalive timeout **********************************************
    sne             c2, d.u.tcp_rx_d.cfg_flags[TCP_OPER_FLAG_KEEPALIVES_BIT], 0
    add.c2          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_KEEPA_TO_OFFSET
#ifndef HW
    memwr.h.c2     r1, 1
#else
    memwr.h.c2     r1, TCP_DEFAULT_KEEPA_RESET_TICK 
#endif 
    /*
     * Do we have payload? If not we are done.
     */
    seq             c1, k.s1_s2s_payload_len, 0
    b.c1            flow_rx_process_done

    tblwr.!c1.l     d.u.tcp_rx_d.need_descr_L, 1
tcp_rx_fast_path_queue_serq:
    tblor.l         d.u.tcp_rx_d.flag, FLAG_DATA
    phvwri          p.common_phv_write_serq, 1
    phvwr           p.to_s6_serq_pidx, d.u.tcp_rx_d.serq_pidx
    tblmincri       d.u.tcp_rx_d.serq_pidx, d.u.tcp_rx_d.consumer_ring_shift, 1
    phvwr           p.to_s5_serq_pidx, d.u.tcp_rx_d.serq_pidx

tcp_rcv_nxt_update:
    tbladd          d.u.tcp_rx_d.rcv_nxt, k.s1_s2s_payload_len
    phvwr           p.to_s5_rcv_nxt, d.u.tcp_rx_d.rcv_nxt

bytes_rcvd_stats_update_start:
    CAPRI_STATS_INC(bytes_rcvd, k.s1_s2s_payload_len, d.u.tcp_rx_d.bytes_rcvd, p.to_s7_bytes_rcvd)
bytes_rcvd_stats_update_end:

tcp_event_data_recv:
tcp_event_data_rcv_done:
    tblwr           d.u.tcp_rx_d.lrcv_time, r4

tcp_ack_snd_check:
    CAPRI_OPERAND_DEBUG(d.u.tcp_rx_d.ato)
    bbeq            d.u.tcp_rx_d.cfg_flags[TCP_OPER_FLAG_DELACK_BIT], 0, tcp_schedule_ack
    tblssub         d.u.tcp_rx_d.quick, 1
    // c1 = 1 ==> Start delayed ack timer
    seq             c1, d.u.tcp_rx_d.quick, 0
    b.c1            tcp_schedule_ack
    tblwr.c1        d.u.tcp_rx_d.quick, TCP_QUICKACKS

tcp_schedule_del_ack:
    // Set delayed ack timeout
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_ATO_OFFSET
    memwr.h         r1, d.u.tcp_rx_d.ato

    // Ring delay ack rx2tx doorbell to start perpetual timer
    // if needed. This can be a memwr since there are no ordering
    // issues
    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_SET,
                        DB_SCHED_UPD_EVAL, 0, LIF_TCP)
    tbladd          d.u.tcp_rx_d.del_ack_pi, 1
    /* data will be in r3 */
    CAPRI_RING_DOORBELL_DATA(0, k.common_phv_fid,
                        TCP_SCHED_RING_DEL_ACK, d.u.tcp_rx_d.del_ack_pi)
    memwr.dx        r4, r3

    seq             c1, d.u.tcp_rx_d.dont_send_ack_L, 0
    b               tcp_ack_snd_check_end
    phvwrmi.c1      p.common_phv_pending_txdma, TCP_PENDING_TXDMA_DEL_ACK, \
                        TCP_PENDING_TXDMA_DEL_ACK

tcp_schedule_ack:
    // cancel del_ack timer and schedule immediate ack
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_ATO_OFFSET
    memwr.h         r1, 0
    seq             c1, d.u.tcp_rx_d.dont_send_ack_L, 0
    phvwrmi.c1      p.common_phv_pending_txdma, TCP_PENDING_TXDMA_ACK_SEND, \
                        TCP_PENDING_TXDMA_ACK_SEND
tcp_ack_snd_check_end:

/*
 * NOTE : No tblwr beyond this point
 */
slow_path:
flow_rx_process_done:
table_read_setup_next:
    phvwr           p.rx2tx_extra_snd_wnd, d.u.tcp_rx_d.snd_wnd
    phvwr           p.rx2tx_extra_rcv_nxt, d.u.tcp_rx_d.rcv_nxt
    phvwr           p.rx2tx_extra_state, d.u.tcp_rx_d.state
    phvwr           p.to_s3_flag, d.u.tcp_rx_d.flag
#ifndef HW
    smeqb           c1, k.common_phv_debug_dol, TCP_DDOL_DONT_SEND_ACK, TCP_DDOL_DONT_SEND_ACK
    phvwrmi.c1      p.common_phv_pending_txdma, 0, TCP_PENDING_TXDMA_ACK_SEND | TCP_PENDING_TXDMA_DEL_ACK
#endif
flow_cpu_rx_process_done:
table_launch_tcp_ack:
    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN, tcp_ack_start,
                        k.common_phv_qstate_addr, TCP_TCB_RX_OFFSET,
                        TABLE_SIZE_512_BITS)

    // If RNMDR descr is pre allocated but is not needed, drop it.
    seq             c3, d.u.tcp_rx_d.need_descr_L, 0
    seq.c3          c3, k.to_s2_descr_prealloc, 1
    /*
     * Pre-alloced descr needs to be freed. Set indication so that later stages can
     * drop pkt instead of posting on SERQ
     */
    phvwri.c3.e     p.common_phv_rx_drop_pkt, 1
#if 1
    // Utilizing the Branch delay slot
    phvwr           p.to_s5_rnmdr_size, 8000
#endif
#if 0
    // TODO : Apollo doesn't support table id 3. Disable for now
    CAPRI_NEXT_TABLE_READ_i(3, TABLE_LOCK_DIS, tcp_rx_read_rnmdr_fc,
                 ASIC_SEM_RNMDPR_BIG_ALLOC_RAW_ADDR, TABLE_SIZE_64_BITS)
#endif
tcp_rx_end:
    nop.e
    nop

tcp_rx_slow_path:
tcp_drop_keepalive_response:
    seq            c1, k.common_phv_flags, TCPHDR_ACK_BIT
    seq            c2, k.s1_s2s_payload_len, 0
    seq            c3, k.s1_s2s_seq, d.u.tcp_rx_d.rcv_nxt
    seq            c4, k.s1_s2s_ack_seq, d.u.tcp_rx_d.snd_una
    // drop the frame
    bcf            [c1 & c2 & c3 & c4], flow_rx_drop
    nop
    tblwr           d.u.tcp_rx_d.quick, 1 
    tbladd          d.u.tcp_rx_d.slow_path_cnt, 1
    
    /* by-pass all PAWS and ts_recent tests for O-O-O packets */
    seq             c1, k.common_phv_ooq_tx2rx_pkt, 1
    b.c1            tcp_rx_no_ts_recent
    nop
    /* bypass PAWS for non-TS connections */
    seq             c2, d.u.tcp_rx_d.ts_recent, r0
    seq             c3, k.s1_s2s_rcv_tsval, r0
    setcf           c2, [c2 | c3 ]
    
tcp_paws_test:
   /* RFC 1323 PAWS: If we have a timestamp reply on this segment
    * and it's less than ts_recent, drop it.
    */
    phvwr.!c2       p.rx2tx_extra_rcv_tsval, d.u.tcp_rx_d.ts_recent
    scwlt.!c2       c1, k.s1_s2s_rcv_tsval, d.u.tcp_rx_d.ts_recent
    //TODO PAWS drop stat
    
tcp_slowpath_cont:

    /* RFC 1323 s4.2.1 p19 basic PAWS and basic RFC 793 window check */    
    sll.!c1         r1, k.to_s2_rcv_wnd_adv, d.u.tcp_rx_d.rcv_wscale
    add.!c1         r2, d.u.tcp_rx_d.rcv_nxt, r1
    scwle.!c1       c1, r2[31:0], k.s1_s2s_seq
    b.c1            tcp_rx_send_dup_ack
    nop
    b.c2            tcp_rx_no_ts_recent
    nop
tcp_ts_recent:
   /* set ts_recent (from BSD) */
    scwle           c1, k.s1_s2s_seq, k.to_s2_rcv_wup
    add             r1, k.s1_s2s_payload_len, k.s1_s2s_seq
    smeqb           c2, k.common_phv_flags, TCPHDR_FIN, TCPHDR_FIN
    add.c2          r1, r1, 1
    scwle           c2, k.to_s2_rcv_wup, r1
    setcf           c1, [c1 & c2] 
    tblwr.c1        d.u.tcp_rx_d.ts_recent, k.s1_s2s_rcv_tsval
    /* set the TS ECR */
    phvwr           p.rx2tx_extra_rcv_tsval, d.u.tcp_rx_d.ts_recent 

tcp_rx_no_ts_recent:
    // reset keepalive timeout if in ESTAB*********************************
    sne            c1, d.u.tcp_rx_d.cfg_flags[TCP_OPER_FLAG_KEEPALIVES_BIT], 0
    seq.c1         c1, d.u.tcp_rx_d.state, TCP_ESTABLISHED
    sne.c1         c1, k.s1_s2s_fin_sent, 1
    add.c1         r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_KEEPA_TO_OFFSET
#ifndef HW
    memwr.h.c1     r1, 1
#else
    memwr.h.c1     r1, TCP_DEFAULT_KEEPA_RESET_TICK 
#endif

tcp_timewait_checks: 
   /* perform timewait checks */
    seq             c1, d.u.tcp_rx_d.state, TCP_TIME_WAIT
    bal.c1          r6, tcp_rx_timewait_check 
    nop
   
    /* FIN_WAIT2 processing */ 
    seq             c1, d.u.tcp_rx_d.state, TCP_FIN_WAIT2
    seq             c2, k.s1_s2s_snd_nxt, k.s1_s2s_ack_seq
    seq             c3, k.s1_s2s_fin_sent, 1
    setcf           c2, [c3 & c2]
    setcf           c1, [c1 | c2]
#ifndef HW
    add.c1          r2, r0, 1
#else 
    add.c1          r2, r0, TCP_DEFAULT_FW2_TICK
#endif
    add.c1          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_RTO_OFFSET
    memwr.c1.h      r1, r2
    b.c1            tcp_rx_set_tw_fw2_timer
    nop.c1
    
    seq             c1, d.u.tcp_rx_d.state, TCP_CLOSING
    seq             c2, k.s1_s2s_snd_nxt, k.s1_s2s_ack_seq
    setcf           c1, [c1 & c2]
    add.c1          r2, r0, TCP_DEFAULT_TW_TICK
    add.c1          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_RTO_OFFSET
    memwr.c1.h      r1, r2
    setcf           c2, [c1]
tcp_rx_set_tw_fw2_timer:
    add.c1          r2, TCP_TT_TW_FW2, r0
    add.c1          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_TTYPERTO_TO_OFFSET
    memwr.c1.b      r1, r2
    /* the following starts the perpetual timer tick by using the del/ack doorbell 
           the conditional c2 is used here for the first interation of the FW2/TW timer */
    addi.c2         r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_SET,
                        DB_SCHED_UPD_EVAL, 0, LIF_TCP)
    tbladd.c2       d.u.tcp_rx_d.del_ack_pi, 1
    /* conditional  #define CAPRI_RING_DOORBELL_DATA(_pid, _qid, _ring, _idx) */
    add.c2             r3, d.u.tcp_rx_d.del_ack_pi, 0, DB_PID_SHFT
    or.c2              r3, r3, k.common_phv_fid, DB_QID_SHFT
    or.c2              r3, r3, TCP_SCHED_RING_DEL_ACK, DB_RING_SHFT
    memwr.dx.c2     r4, r3
    /** end of conditional doorbell code **/
    
    seq             c1, k.s1_s2s_payload_len, r0
    tblor.!c1.l     d.u.tcp_rx_d.flag, FLAG_DATA
    tblwr.c1.l      d.u.tcp_rx_d.need_descr_L, 0
    phvwri.c1       p.common_phv_write_serq, 0

    smeqb           c1, k.common_phv_flags, TCPHDR_SYN, TCPHDR_SYN
    tblwr.c1.l      d.u.tcp_rx_d.need_descr_L, 1

    smeqb           c1, k.common_phv_flags, TCPHDR_ECE, TCPHDR_ECE
    tblor.c1.l      d.u.tcp_rx_d.flag, (FLAG_ECE | FLAG_SLOWPATH)

    bbeq            k.common_phv_flags[TCPHDR_RST_BIT], 1, tcp_rx_rst_handling

    seq             c1, d.u.tcp_rx_d.state, TCP_RST
    seq             c2, k.s1_s2s_rst_sent, 1
    tblwr.c2        d.u.tcp_rx_d.state, TCP_RST
    setcf           c3, [c1 | c2]
    // drop the frame
    b.c3            flow_rx_drop
    tbladd.c3       d.u.tcp_rx_d.rx_drop_cnt, 1

    smeqb           c1, d.u.tcp_rx_d.parsed_state, \
                            TCP_PARSED_STATE_HANDLE_IN_CPU, \
                            TCP_PARSED_STATE_HANDLE_IN_CPU
    phvwri.c1       p.common_phv_write_arq, 1
    tblwr.c1.l      d.u.tcp_rx_d.need_descr_L, 1

    seq             c2, d.u.tcp_rx_d.state, TCP_LISTEN
    phvwri.c2       p.common_phv_write_tcp_app_hdr,1
    phvwr.c2        p.cpu_hdr2_tcp_seqNo, k.{s1_s2s_seq}.wx
    phvwr.c2        p.{cpu_hdr2_tcp_AckNo_1,cpu_hdr3_tcp_AckNo_2}, k.{s1_s2s_ack_seq}.wx
    phvwr.c2        p.cpu_hdr2_tcp_flags, k.common_phv_flags

    bcf             [c1], flow_cpu_rx_process_done

    /*
     * if pkt->seq != rcv_nxt, its a retransmission or OOO, send ack
     *
     * We don't partially accept unacknowledged bytes yet,
     * the entire frame is dropped if not between rcv_nxt and
     * rcv_nxt + advertised_window
     */
    sne             c1, k.s1_s2s_seq, d.u.tcp_rx_d.rcv_nxt

    // Don't send dupack for rx2tx packets
    seq             c3, k.common_phv_ooq_tx2rx_pkt, 1
    bcf             [c3 | !c1], tcp_rx_ooo_skip_dup_ack
    nop
    phvwrmi         p.common_phv_pending_txdma, TCP_PENDING_TXDMA_ACK_SEND, \
                        TCP_PENDING_TXDMA_ACK_SEND
    phvwr           p.rx2tx_extra_pending_dup_ack_send, 1
    phvwr           p.rx2tx_extra_dup_rcv_nxt, d.u.tcp_rx_d.rcv_nxt
    phvwr           p.common_phv_is_dupack, 1
    phvwr           p.to_s5_serq_pidx, d.u.tcp_rx_d.serq_pidx
    phvwr           p.to_s5_rcv_nxt, d.u.tcp_rx_d.rcv_nxt
tcp_rx_ooo_skip_dup_ack:
    sne             c2, k.s1_s2s_payload_len, 0

    /*
     * c3 = seq_no != rcv_nxt and has payload
     */
    setcf           c3, [c1 & c2]
    tblwr.c3.l      d.u.tcp_rx_d.need_descr_L, 0
    phvwr.c3        p.common_phv_write_serq, 0
    // if (seqnum == rcv_nxt - 1 && payload_len == 0) {it is a keep alive, add stats}
    add             r1, k.s1_s2s_seq, 1
    sne             c4, d.u.tcp_rx_d.rcv_nxt, r1
    bcf             [c4 | c2], tcp_rx_check_win_probe 
    addui           r2, r0, hiword(TCP_PROXY_STATS)
    addi            r2, r2, loword(TCP_PROXY_STATS)
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r2, TCP_PROXY_STATS_RCVD_KEEP_ALIVE, 1)

tcp_rx_check_win_probe: 
    seq             c4, k.to_s2_rcv_wnd_adv, 0
    // c4: adv_win == 0
    // c1: seqnum != rcv_nxt
    // c2: payload_len != 0
    // if (win_adv == 0 && payload_len != 0 && seq_num == rcv_nxt) {zwinprobe}
    bcf             [c1 | !c2 | !c4], tcp_rx_ooo_check
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r2, TCP_PROXY_STATS_RCV_WIN_PROBE, 1)

tcp_rx_ooo_check:
     // Consider OOO only if payload_len != 0 && seq > rcv_nxt
    scwlt           c1, d.u.tcp_rx_d.rcv_nxt, k.s1_s2s_seq
    bcf             [!c1 | !c2], tcp_rx_ooo_check_done
    // check if its within advertised window and ooo processing
    // is configured
    add             r1, k.s1_s2s_seq, k.s1_s2s_payload_len
    sll             r3, k.to_s2_rcv_wnd_adv, d.u.tcp_rx_d.rcv_wscale
    add             r2, k.to_s2_rcv_wup, r3
    scwle           c1, r1[31:0], r2[31:0]
    seq             c2, d.u.tcp_rx_d.cfg_flags[TCP_OPER_FLAG_OOO_QUEUE_BIT], 1
    bcf             [c1 & c2], tcp_rx_ooo_rcv
    nop
    bcf             [c1], tcp_rx_ooo_check_done
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r2, TCP_PROXY_STATS_RCVD_PKT_AFTER_WIN, 1)

tcp_rx_ooo_check_done:
    seq             c1, k.common_phv_ooq_tx2rx_pkt, 1
    bcf             [c1 & c3], flow_rx_drop_launch_dummy
    tbladd.c3       d.u.tcp_rx_d.rx_drop_cnt, 1
    bcf             [c3], flow_rx_process_done

    /*
     * If we have received data and serq is full,
     * drop the frame
     */
    sne             c1, k.s1_s2s_payload_len, r0
#ifdef HW
    seq             c2, k.to_s2_serq_cidx[15], 1
#else
    setcf           c2, [!c0]
#endif
    add.!c2         r2, d.u.tcp_rx_d.serq_pidx, 1
    and.!c2         r2, r2, r_consumer_ring_mask
    seq.!c2         c2, r2, k.to_s2_serq_cidx[14:0]
    setcf           c7, [c1 & c2]
    tbladd.c7       d.u.tcp_rx_d.serq_full_cnt, 1
    b.c7            flow_rx_drop

    /*
     * Handle close (fin sent in tx pipeline)
     *
     * State (EST) --> FIN_WAIT_1
     * State (CLOSE_WAIT) --> LAST_ACK
     */
tcp_rx_handle_close:
    setcf           c2, [!c0] // Reset c2
    setcf           c7, [!c0] // Reset c7, Will be set for FIN non ooq pkt
    seq             c1, k.s1_s2s_fin_sent, 1
   
    seq.c1          c2, d.u.tcp_rx_d.state, TCP_ESTABLISHED
    tblwr.c2        d.u.tcp_rx_d.state, TCP_FIN_WAIT1

    seq.c1          c2, d.u.tcp_rx_d.state, TCP_CLOSE_WAIT
    tblwr.c2        d.u.tcp_rx_d.state, TCP_LAST_ACK

    /*
     * EST (recv: FIN) --> CLOSE_WAIT, increment sequence number
     * FIN_WAIT_1 (recv: FIN) --> CLOSING, increment sequence number
     * FIN_WAIT_2 (recv: FIN) --> TIME_WAIT, increment sequence number
     *
     */
    smeqb           c1, k.common_phv_flags, TCPHDR_FIN, TCPHDR_FIN
    b.!c1           tcp_rx_slow_path_post_fin_handling

    seq             c1, d.u.tcp_rx_d.state, TCP_ESTABLISHED
    tblwr.c1        d.u.tcp_rx_d.state, TCP_CLOSE_WAIT

    seq             c2, d.u.tcp_rx_d.state, TCP_FIN_WAIT1
    tblwr.c2        d.u.tcp_rx_d.state, TCP_CLOSING

    seq             c3, d.u.tcp_rx_d.state, TCP_FIN_WAIT2
    tblwr.c3        d.u.tcp_rx_d.state, TCP_TIME_WAIT
    phvwr           p.rx2tx_extra_state, d.u.tcp_rx_d.state
    balcf           r6, [c3], start_time_wait_timer
    setcf           c1, [c1 | c2 | c3]
    b.!c1           tcp_rx_slow_path_post_fin_handling
    tbladd.c1       d.u.tcp_rx_d.rcv_nxt, 1
    tblwr.l         d.u.tcp_rx_d.need_descr_L, 1
    phvwrmi         p.common_phv_pending_txdma, TCP_PENDING_TXDMA_ACK_SEND, \
                        TCP_PENDING_TXDMA_ACK_SEND
    seq             c7, k.common_phv_ooq_tx2rx_pkt, 0
tcp_rx_slow_path_post_fin_handling:
    bal             r6, tcp_rx_not_est_ack_proc
    nop
    /*
     * NOTE: c7 could be set only in case of the rcvd pkt being FIN,
     * else it will remain cleared
     * The following branch is applicable only in case of rx FIN pkt
     */
    bcf             [c7], tcp_rx_fast_path_queue_serq

    seq             c1, k.s1_s2s_payload_len, r0
    seq.!c1         c1, d.u.tcp_rx_d.state, TCP_CLOSE
    seq.!c1         c1, d.u.tcp_rx_d.state, TCP_CLOSE_WAIT
    seq.!c1         c1, d.u.tcp_rx_d.state, TCP_TIME_WAIT
    seq.!c1         c1, d.u.tcp_rx_d.state, TCP_LAST_ACK
    tblwr.c1.l      d.u.tcp_rx_d.need_descr_L, 0
    phvwri.c1       p.common_phv_write_serq, 0
    bcf             [c1], flow_rx_process_done
    phvwr.c1        p.common_phv_skip_pkt_dma, 1
tcp_rx_slow_path_handle_data:
    // if this is a tx2rx ooq packet, don't allocate descriptors
    seq             c2, k.common_phv_ooq_tx2rx_pkt, 1
    tblwr.c2.l      d.u.tcp_rx_d.need_descr_L, 0
    // For OOQ feedback packets, dont send ack
    tblwr.c2.l      d.u.tcp_rx_d.dont_send_ack_L, 1
    // We don't want to advance ack_seq no until ooq packets are all dequeued
    phvwr.c2        p.rx2tx_extra_pending_dup_ack_send, 1
    phvwr.c2        p.rx2tx_extra_dup_rcv_nxt, d.u.tcp_rx_d.rcv_nxt
    b.c2            tcp_rx_fast_path_queue_serq

    /*
     * Handle in-order data receipt (For example - data received during
     * FIN_WAIT, data received with ECE flag, data received with OOO packets in
     * queue etc.)
     */

    // OOO launch to see if we can dequeue any queue that has become in-order.
    // Skip the OOO launch and check if the queue is empty or if this is already
    // an OOO pkt that has been queued from Tx.
    seq             c1, d.u.tcp_rx_d.ooq_not_empty, 0
    b.c1            tcp_rx_skip_ooo_launch
    nop

tcp_rx_slow_path_launch_ooo:
    phvwr           p.t2_s2s_seq, k.s1_s2s_seq
    phvwr           p.t2_s2s_payload_len, k.s1_s2s_payload_len
    // Don't ack this packet. The last in-order packet from the OOQ
    // should generate the extended ack
    tblwr.l         d.u.tcp_rx_d.dont_send_ack_L, 1
    // We don't want to advance ack_seq no until ooq packets are all dequeued
    phvwr           p.rx2tx_extra_pending_dup_ack_send, 1
    phvwr           p.rx2tx_extra_dup_rcv_nxt, d.u.tcp_rx_d.rcv_nxt
    CAPRI_NEXT_TABLE_READ_OFFSET(2, TABLE_LOCK_EN, tcp_ooo_book_keeping_in_order,
                        k.common_phv_qstate_addr, TCP_TCB_OOO_BOOK_KEEPING_OFFSET0,
                        TABLE_SIZE_512_BITS)
tcp_rx_skip_ooo_launch:
    b               tcp_rx_fast_path
    nop

flow_rx_drop:
    seq             c1, k.common_phv_ooq_tx2rx_pkt, 1
    seq.!c1         c1, k.to_s2_descr_prealloc, 1
    b.c1            flow_rx_drop_launch_dummy
    phvwr.!c1.e     p.app_header_table0_valid, 0
    phvwri          p.p4_intr_global_drop, 1
    // If pkt is to be dropped, launch programs to free rnmdr descr and drop the pkt
flow_rx_drop_launch_dummy:
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tcp_rx_s3_t0_bubble_drop_pkt)
    nop.e
    nop

tcp_rx_rst_handling:
    // reset keepalive timeout **********************************************
    sne             c2, d.u.tcp_rx_d.cfg_flags[TCP_OPER_FLAG_KEEPALIVES_BIT], 0
    add.c2          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_KEEPA_TO_OFFSET
    memwr.h.c2      r1, 0 

    // if we are already in RST state - only schedule txdma to clean retx queue
    seq             c1, d.u.tcp_rx_d.state, TCP_RST
    b.c1            tcp_rx_rst_sched_clean_retx
    /*
     * BAL to send connection close condition indication for tcb cleanup
     * Pass close reason in r7
     */
    add             r5, r0, REMOTE_RST
    bal             r6, ring_db_ccq_close_ind
    nop // Delay-slot. May not be required here but just to be sure
    
    // We need the descr to pass RST flag to upper layer or to other flow (proxy)
    tblwr.l         d.u.tcp_rx_d.need_descr_L, 1

    // Change state so Tx pipeline cleans up retx queue
    tblwr           d.u.tcp_rx_d.state, TCP_RST

    // check for serq full
    add             r2, d.u.tcp_rx_d.serq_pidx, 1
    and             r2, r2, r_consumer_ring_mask
    seq             c7, r2, k.to_s2_serq_cidx[14:0]
    tbladd.c7       d.u.tcp_rx_d.serq_full_cnt, 1
    b.c7            flow_rx_drop

    // get serq slot
    phvwri          p.common_phv_write_serq, 1
    phvwr           p.to_s6_serq_pidx, d.u.tcp_rx_d.serq_pidx
    tblmincri       d.u.tcp_rx_d.serq_pidx, d.u.tcp_rx_d.consumer_ring_shift, 1

tcp_rx_rst_sched_clean_retx:
    b               flow_rx_process_done
    // Tell txdma we have work to do
    phvwrmi         p.common_phv_pending_txdma, TCP_PENDING_TXDMA_SND_UNA_UPDATE, \
                        TCP_PENDING_TXDMA_SND_UNA_UPDATE

tcp_rx_ooo_rcv:
    // prevent endless loop, we don't want to queue ooo packet back in OOO
    // queue
    seq             c3, k.common_phv_ooq_tx2rx_pkt, 1
    b.c3            flow_rx_drop_launch_dummy
    tbladd.c3       d.u.tcp_rx_d.rx_drop_cnt, 1

    seq             c5, d.u.tcp_rx_d.ooq_not_empty, 0
ooo_cnt_stats_update_start:
    CAPRI_STATS_INC(ooo_cnt, 1, d.u.tcp_rx_d.ooo_cnt, p.to_s7_ooo_cnt)
ooo_cnt_stats_update_end:
    tblwr.c5.f      d.u.tcp_rx_d.ooq_not_empty, 1

    CAPRI_NEXT_TABLE_READ_OFFSET(2, TABLE_LOCK_EN, tcp_ooo_book_keeping,
                        k.common_phv_qstate_addr, TCP_TCB_OOO_BOOK_KEEPING_OFFSET0,
                        TABLE_SIZE_512_BITS)
    phvwr           p.t2_s2s_seq, k.s1_s2s_seq
    phvwr           p.t2_s2s_payload_len, k.s1_s2s_payload_len
    phvwr           p.common_phv_ooo_rcv, 1
    // we need to keep the descriptor
    tblwr.l         d.u.tcp_rx_d.need_descr_L, 1
    b               flow_rx_process_done
    // when ooq_not_empty moves from 0 --> 1, use wrfence so this write is
    // guaranteed before writes in other stages (this field is reset to 0 by
    // bookkeeping stage)
    wrfence.c5
    
tcp_rx_timewait_check:
    /*
     * If the segment contains RST:
     *  Drop the segment - see Stevens, vol. 2, p. 964 and
     *      RFC 1337.
     */ 
    seq             c1, k.common_phv_flags[TCPHDR_RST_BIT], 1
    seq             c2, k.common_phv_flags[TCPHDR_SYN_BIT], 1
    seq             c3, k.common_phv_flags[TCPHDR_ACK_BIT], 0
    setcf           c1, [c1 | c2 | c3]
    b.c1            flow_rx_drop
    tbladd.c1       d.u.tcp_rx_d.rx_drop_cnt, 1

    /*
     * Reset the 2MSL timer if this is a duplicate FIN.
          if (thflags & TH_FIN) {
              seq = th->th_seq + tlen;
              if (seq + 1 == tw->rcv_nxt)
                    tcp_tw_2msl_reset(tw, 1);
          }
     */
    seq             c1, k.common_phv_flags[TCPHDR_FIN_BIT], 1
    add.c1          r1, k.s1_s2s_seq,k.s1_s2s_payload_len
    add.c1          r1, r1, 1
    seq.c1          c1, r1, d.u.tcp_rx_d.rcv_nxt
    /* if c1==1, then reset the TW timer */
    add.c1          r2, r0, TCP_DEFAULT_TW_TICK
    add.c1          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_RTO_OFFSET
    memwr.c1.h      r1, r2
    add.c1          r2, TCP_TT_TW_FW2, r0
    add.c1          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_TTYPERTO_TO_OFFSET
    memwr.c1.b      r1, r2
    
      /*
       * Acknowledge the segment if it has data or is not a duplicate ACK.
  
         if (thflags != TH_ACK || tlen != 0 ||
             th->th_seq != tw->rcv_nxt || th->th_ack != tw->snd_nxt) {
             TCP_PROBE5(receive, NULL, NULL, m, NULL, th);
             tcp_twrespond(tw, TH_ACK);
             goto dropnoprobe;
         }
      */
    sne            c1, k.common_phv_flags, TCPHDR_ACK_BIT
    sne            c2, k.s1_s2s_payload_len, 0
    sne            c3, k.s1_s2s_seq, d.u.tcp_rx_d.rcv_nxt
    sne            c4, k.s1_s2s_ack_seq, d.u.tcp_rx_d.snd_una
    setcf          c1, [c1 | c2 | c3 | c4]
    tblwr.c1       d.u.tcp_rx_d.quick, 1 
    jr.c1          r6  // respond with dup ack 
    nop.c1
    b.!c1          flow_rx_drop
    tbladd.!c1     d.u.tcp_rx_d.rx_drop_cnt, 1
    nop

/*
 * r5: Has close reason passed as param
 * r6: Has return address
 */
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
    CAPRI_RING_DOORBELL_DATA(0, k.common_phv_fid,  TCP_CCQ_CONN_CLOSED_IND_RING, r5)
    // DB write
    memwr.dx        r2, r3
    jr              r6
    nop

ring_db_ccq_sesq_cleanup_ind:
     // Prep DB addr in r2 for qtype 1, TCP LIF
    addi            r2, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_INC,
                        DB_SCHED_UPD_EVAL, TCP_OOO_RX2TX_QTYPE, LIF_TCP)
    // Prep DB data for cleanup cond 1, Data will be in r3
    CAPRI_RING_DOORBELL_DATA(0, k.common_phv_fid, TCP_CCQ_SESQ_CLEANED_IND_RING, 0)
    // DB write
    memwr.dx        r2, r3
    jr              r6
    nop

tcp_rx_send_dup_ack:
    phvwr           p.rx2tx_extra_pending_dup_ack_send, 1
    phvwr           p.rx2tx_extra_dup_rcv_nxt, d.u.tcp_rx_d.rcv_nxt
    tbladd          d.u.tcp_rx_d.rx_drop_cnt, 1
    phvwr           p.rx2tx_extra_rcv_tsval, d.u.tcp_rx_d.ts_recent
    tblwr.l         d.u.tcp_rx_d.need_descr_L, 0
    phvwri          p.common_phv_write_serq, 0
    b               flow_rx_process_done
    phvwrmi         p.common_phv_pending_txdma, TCP_PENDING_TXDMA_ACK_SEND, \
                        TCP_PENDING_TXDMA_ACK_SEND

// R6 has return address
start_time_wait_timer:
    // start TIME_WAIT TIMER
    add             r2, r0, TCP_DEFAULT_TW_TICK
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_RTO_OFFSET
    memwr.h         r1, r2
    add             r2, TCP_TT_TW_FW2, r0
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_TTYPERTO_TO_OFFSET
    memwr.b         r1, r2
    jr              r6
    nop

// Return address in R6
tcp_rx_not_est_ack_proc:
    /*
     * If state != est && !ooq_pkt && ack == snd_nxt, go process
     * else skip
     */
    sne             c1, d.u.tcp_rx_d.state, TCP_ESTABLISHED
    sne.c1          c1, k.common_phv_ooq_tx2rx_pkt, 1
    seq.c1          c1, k.s1_s2s_snd_nxt, k.s1_s2s_ack_seq
    jr.!c1          r6 // Skip processing and return
    /*
     *                    (recv ack)
     * State (LAST_ACK)   ----------> TCP_CLOSE
     * State (FIN_WAIT_1) ----------> FIN_WAIT_2
     * State (CLOSING) ----------> TIME_WAIT
     */
    seq.c1          c1, d.u.tcp_rx_d.state, TCP_LAST_ACK
    /*
     * BAL to send connection close condition indication for tcb cleanup
     * LAST_ACK -> CLOSE
     * Pass close reason in r5
     */
    add             r4, r0, r6 // Save ret addr in r4
    add             r5, r0, PASSIVE_GRACEFUL
    balcf           r6, [c1], ring_db_ccq_close_ind
    tblwr.c1        d.u.tcp_rx_d.state, TCP_CLOSE
    add             r6, r0, r4 // Restore ret addr in r6

    seq             c1, d.u.tcp_rx_d.state, TCP_FIN_WAIT1
    tblwr.c1        d.u.tcp_rx_d.state, TCP_FIN_WAIT2

    seq             c1, d.u.tcp_rx_d.state, TCP_CLOSING
    tblwr.c1        d.u.tcp_rx_d.state, TCP_TIME_WAIT
    add             r4, r0, r6 // Save ret addr in r4
    balcf           r6, [c1], start_time_wait_timer
    phvwr           p.rx2tx_extra_state, d.u.tcp_rx_d.state
    add             r6, r0, r4 // Restore ret addr in r6
    jr              r6
    nop
