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
#include "INGRESS_s1_t0_tcp_rx_k.h"

struct phv_ p;
struct s1_t0_tcp_rx_k_ k;
struct s1_t0_tcp_rx_d d;

%%
    .param          tcp_ack_start
    .param          tcp_rx_read_rnmdr_start
    .param          tcp_ooo_book_keeping
    .align

    /*
     * Global conditional variables
     *
     * c6 = ooo in Rx Queue, skip some stages
     * c7 = Drop packet
     */
tcp_rx_process_start:
#ifdef CAPRI_IGNORE_TIMESTAMP
    add             r4, r0, r0
    add             r6, r0, r0
#endif

    /*
     * Fast Path checks
     */
    sne             c1, d.u.tcp_rx_d.state, TCP_ESTABLISHED
    seq.!c1         c1, k.s1_s2s_fin_sent, 1
    sne             c2, d.u.tcp_rx_d.rcv_nxt, k.s1_s2s_seq
    scwlt           c3, k.s1_s2s_snd_nxt, k.s1_s2s_ack_seq

    // disable timestamp checking for now
    //setcf           c4, [c0]
    //slt             c4, r4, d.u.tcp_rx_d.ts_recent

    /*
     * c4 = serq is full
     */
    add             r2, d.u.tcp_rx_d.serq_pidx, 1
    and             r2, r2, CAPRI_SERQ_RING_SLOTS - 1
    seq             c4, r2, k.to_s1_serq_cidx

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

    bcf             [c1 | c2 | c3 | c4 | c5], tcp_rx_slow_path

tcp_rx_fast_path:

tcp_store_ts_recent:
    /*
     * tp->rcv_nxt == tp->rcv_wup
     *      tcp_store_ts_recent(tp)
     *
     */
    seq             c1, k.to_s1_rcv_wup, d.u.tcp_rx_d.rcv_nxt
    tblwr.c1        d.u.tcp_rx_d.ts_recent, k.s1_s2s_rcv_tsval

    /*
     * Do we have payload? If not we are done.
     */
    seq             c1, k.s1_s2s_payload_len, 0
    b.c1            flow_rx_process_done
    tblor.!c1.l     d.u.tcp_rx_d.flag, FLAG_DATA

    phvwri          p.common_phv_write_serq, 1
    tblwr.l         d.u.tcp_rx_d.alloc_descr, 1

    phvwr           p.to_s6_serq_pidx, d.u.tcp_rx_d.serq_pidx
    tblmincri       d.u.tcp_rx_d.serq_pidx, CAPRI_SERQ_RING_SLOTS_SHIFT, 1
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
    bbeq            d.u.tcp_rx_d.cfg_flags[TCP_CFG_FLAG_DELACK_BIT], 0, tcp_schedule_ack
    tblssub         d.u.tcp_rx_d.quick, 1
    // c1 = 1 ==> Start delayed ack timer
    seq             c1, d.u.tcp_rx_d.quick, 0
    b.!c1           tcp_schedule_ack
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

    b               tcp_ack_snd_check_end
    phvwrmi         p.common_phv_pending_txdma, TCP_PENDING_TXDMA_DEL_ACK, \
                        TCP_PENDING_TXDMA_DEL_ACK

tcp_schedule_ack:
    // cancel del_ack timer and schedule immediate ack
    add             r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_ATO_OFFSET
    memwr.h         r1, 0
    phvwrmi         p.common_phv_pending_txdma, TCP_PENDING_TXDMA_ACK_SEND, \
                        TCP_PENDING_TXDMA_ACK_SEND
tcp_ack_snd_check_end:

slow_path:
flow_rx_process_done:
table_read_setup_next:
    phvwr           p.rx2tx_extra_snd_wnd, d.u.tcp_rx_d.snd_wnd
    phvwr           p.rx2tx_extra_rcv_mss, d.u.tcp_rx_d.rcv_mss
    phvwr           p.rx2tx_extra_rcv_nxt, d.u.tcp_rx_d.rcv_nxt
    phvwr           p.rx2tx_extra_state, d.u.tcp_rx_d.state
    phvwr           p.to_s2_flag, d.u.tcp_rx_d.flag
#ifndef HW
    smeqb           c1, k.common_phv_debug_dol, TCP_DDOL_DONT_SEND_ACK, TCP_DDOL_DONT_SEND_ACK
    phvwrmi.c1      p.common_phv_pending_txdma, 0, TCP_PENDING_TXDMA_ACK_SEND | TCP_PENDING_TXDMA_DEL_ACK
#endif
flow_cpu_rx_process_done:
    /*
     * c7 = drop
     */
    bcf             [c7], flow_rx_drop

table_launch_tcp_ack:
    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN, tcp_ack_start,
                        k.common_phv_qstate_addr, TCP_TCB_RX_OFFSET,
                        TABLE_SIZE_512_BITS)
    /*
     * c6 = OOO in Rx queue, do not allocate buffers
     */
    bcf             [c6], tcp_rx_end

table_launch_RNMDR_ALLOC_IDX:
    /*
     * Allocate page and descriptor if alloc_descr is 1.
     */
    seq             c3, d.u.tcp_rx_d.alloc_descr, 1
    bcf             [!c3], tcp_rx_end
    CAPRI_NEXT_TABLE_READ_i(1, TABLE_LOCK_DIS, tcp_rx_read_rnmdr_start,
                        RNMDPR_ALLOC_IDX, TABLE_SIZE_64_BITS)
tcp_rx_end:
    nop.e
    nop


tcp_rx_slow_path:
    tbladd          d.{u.tcp_rx_d.slow_path_cnt}.hx, 1
    seq             c1, k.s1_s2s_payload_len, r0
    tblor.!c1.l     d.u.tcp_rx_d.flag, FLAG_DATA
    tblwr.c1.l      d.u.tcp_rx_d.alloc_descr, 0
    phvwri.c1       p.common_phv_write_serq, 0

    smeqb           c1, k.common_phv_flags, TCPHDR_SYN, TCPHDR_SYN
    tblwr.c1.l      d.u.tcp_rx_d.alloc_descr, 1

    smeqb           c1, k.common_phv_flags, TCPHDR_ECE, TCPHDR_ECE
    tblor.c1.l      d.u.tcp_rx_d.flag, (FLAG_ECE | FLAG_SLOWPATH)

    bbeq            k.common_phv_flags[TCPHDR_RST_BIT], 1, tcp_rx_rst_handling

    seq             c1, d.u.tcp_rx_d.state, TCP_RST
    seq             c2, k.s1_s2s_rst_sent, 1
    tblwr.c2        d.u.tcp_rx_d.state, TCP_RST
    bcf             [c1 | c2], tcp_rx_post_rst_handling

    smeqb           c1, d.u.tcp_rx_d.parsed_state, \
                            TCP_PARSED_STATE_HANDLE_IN_CPU, \
                            TCP_PARSED_STATE_HANDLE_IN_CPU
    phvwri.c1       p.common_phv_write_arq, 1
    tblwr.c1.l      d.u.tcp_rx_d.alloc_descr, 1

    seq             c2, d.u.tcp_rx_d.state, TCP_LISTEN
    phvwri.c2       p.common_phv_write_tcp_app_hdr,1
    phvwr.c2        p.cpu_hdr2_tcp_seqNo, k.{s1_s2s_seq}.wx
    phvwr.c2        p.{cpu_hdr2_tcp_AckNo_1,cpu_hdr3_tcp_AckNo_2}, k.{s1_s2s_ack_seq}.wx
    phvwr.c2        p.cpu_hdr2_tcp_flags, k.common_phv_flags

    bcf             [c1], flow_cpu_rx_process_done
    setcf           c7, [!c0]

    /*
     * if pkt->seq != rcv_nxt, its a retransmission or OOO, send ack
     *
     * We don't partially accept unacknowledged bytes yet,
     * the entire frame is dropped if not between rcv_nxt and
     * rcv_nxt + advertised_window
     */
    sne             c1, k.s1_s2s_seq, d.u.tcp_rx_d.rcv_nxt
    phvwrmi.c1      p.common_phv_pending_txdma, TCP_PENDING_TXDMA_ACK_SEND, \
                        TCP_PENDING_TXDMA_ACK_SEND
    phvwr.c1        p.rx2tx_extra_pending_dup_ack_send, 1
    sne             c2, k.s1_s2s_payload_len, 0
    setcf           c3, [c1 & c2]
    phvwr.c3        p.common_phv_skip_pkt_dma, 1
    phvwr.c3        p.common_phv_write_serq, 0

tcp_rx_ooo_check:
     // Consider OOO only if payload_len != 0 && seq > rcv_nxt
    slt             c1, d.u.tcp_rx_d.rcv_nxt, k.s1_s2s_seq
    bcf             [!c1 | !c2], tcp_rx_ooo_check_done
    // check if its within advertised window and ooo processing
    // is configured
    add             r1, d.u.tcp_rx_d.rcv_nxt, k.s1_s2s_payload_len
    add             r2, k.to_s1_rcv_wup, k.to_s1_rcv_wnd_adv
    sle             c1, r1, r2
    seq             c2, d.u.tcp_rx_d.cfg_flags[TCP_CFG_FLAG_OOO_QUEUE_BIT], 1
    bcf             [c1 & c2], tcp_rx_ooo_rcv

tcp_rx_ooo_check_done:
    // set c6 so that descriptors are not allocated
    tbladd.c3       d.u.tcp_rx_d.rx_drop_cnt, 1
    setcf.c3        c6, [c0]
    bcf             [c3], flow_rx_process_done

    /*
     * If we have received data and serq is full,
     * drop the frame
     */
    sne             c1, k.s1_s2s_payload_len, r0
    add             r2, d.u.tcp_rx_d.serq_pidx, 1
    and             r2, r2, CAPRI_SERQ_RING_SLOTS - 1
    seq             c2, r2, k.to_s1_serq_cidx
    setcf           c7, [c1 & c2]
    tbladd.c7       d.{u.tcp_rx_d.serq_full_cnt}.hx, 1
    phvwri.c7       p.p4_intr_global_drop, 1
    b.c7            flow_rx_process_done

    /*
     * Handle close (fin sent in tx pipeline)
     *
     * State (EST) --> FIN_WAIT_1
     * State (CLOSE_WAIT) --> LAST_ACK
     */
    setcf           c2, [!c0]
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

    // TODO : hand off to arm or start time_wait timer
    seq             c3, d.u.tcp_rx_d.state, TCP_FIN_WAIT2
    tblwr.c3        d.u.tcp_rx_d.state, TCP_TIME_WAIT

    /*
     * TODO : Until TLS code is ready, do not send FIN packets to TLS
     * unless bypass_barco is set
     */
    smeqb           c4, k.common_phv_debug_dol, TCP_DDOL_BYPASS_BARCO, TCP_DDOL_BYPASS_BARCO
    b.!c4           tcp_rx_slow_path_post_fin_handling

    setcf           c1, [c1 | c2 | c3]
    b.!c1           tcp_rx_slow_path_post_fin_handling
    tbladd.c1       d.u.tcp_rx_d.rcv_nxt, 1
    tblwr.c1.l      d.u.tcp_rx_d.alloc_descr, 1
    phvwri.c1       p.common_phv_write_serq, 1
    phvwr.c1        p.to_s6_serq_pidx, d.u.tcp_rx_d.serq_pidx
    tblmincri.c1    d.u.tcp_rx_d.serq_pidx, CAPRI_SERQ_RING_SLOTS_SHIFT, 1
    phvwr.c1        p.to_s5_serq_pidx, d.u.tcp_rx_d.serq_pidx
    phvwr.c1        p.rx2tx_extra_pending_dup_ack_send, 1
    phvwrmi.c1      p.common_phv_pending_txdma, TCP_PENDING_TXDMA_ACK_SEND, \
                        TCP_PENDING_TXDMA_ACK_SEND

    //phvwr           p.rx2tx_state, d.u.tcp_rx_d.state

tcp_rx_slow_path_post_fin_handling:
    /*   /* If PAWS failed, check it more carefully in slow path */
    /* if ((s32)(tp->rx_opt.rcv_tsval - tp->rx_opt.ts_recent) < 0) {

           /* DO NOT update ts_recent here, if checksum fails
        * and timestamp was corrupted part, it will result
        * in a hung connection since we will drop all
                * future packets due to the PAWS test.
                *
            goto slow_path  ;
    }
    */
    //sub             r1, r4, d.u.tcp_rx_d.ts_recent
    //slt             c1, r1, r0
    //bcf             [c1],slow_path
    //nop

    seq             c1, k.s1_s2s_payload_len, r0
    // We have to handle data in FIN_WAIT states or for example
    // when ECE bit was set
    bcf             [!c1], tcp_rx_fast_path
    nop

    b               flow_rx_process_done
    nop

flow_rx_drop:
    CAPRI_CLEAR_TABLE0_VALID
    nop.e
    nop

tcp_rx_rst_handling:
    /*
     * TODO : Until TLS code is ready, do not send RST packets to TLS
     * unless bypass_barco is set
     */
    smeqb           c4, k.common_phv_debug_dol, TCP_DDOL_BYPASS_BARCO, TCP_DDOL_BYPASS_BARCO
    phvwri.!c4      p.p4_intr_global_drop, 1
    b.!c4           flow_rx_drop
    tbladd.!c4      d.u.tcp_rx_d.rx_drop_cnt, 1

    // We need to pass RST flag to other flow
    tblwr.l         d.u.tcp_rx_d.alloc_descr, 1

    // Change state so Tx pipeline cleans up retx queue
    tblwr           d.u.tcp_rx_d.state, TCP_RST

    // check for serq full
    add             r2, d.u.tcp_rx_d.serq_pidx, 1
    and             r2, r2, CAPRI_SERQ_RING_SLOTS - 1
    seq             c7, r2, k.to_s1_serq_cidx
    tbladd.c7       d.{u.tcp_rx_d.serq_full_cnt}.hx, 1
    phvwri.c7       p.p4_intr_global_drop, 1
    b.c7            flow_rx_process_done

    // get serq slot
    phvwri          p.common_phv_write_serq, 1
    phvwr           p.to_s6_serq_pidx, d.u.tcp_rx_d.serq_pidx
    tblmincri       d.u.tcp_rx_d.serq_pidx, CAPRI_SERQ_RING_SLOTS_SHIFT, 1

    b               flow_rx_process_done
    // Tell txdma we have work to do
    phvwrmi         p.common_phv_pending_txdma, TCP_PENDING_TXDMA_SND_UNA_UPDATE, \
                        TCP_PENDING_TXDMA_SND_UNA_UPDATE

tcp_rx_post_rst_handling:
    // drop the frame
    setcf           c7, [c0]
    tbladd.!c4      d.u.tcp_rx_d.rx_drop_cnt, 1
    b               flow_rx_process_done
    phvwri          p.p4_intr_global_drop, 1

tcp_rx_ooo_rcv:
    CAPRI_NEXT_TABLE_READ_OFFSET(2, TABLE_LOCK_EN, tcp_ooo_book_keeping,
                        k.common_phv_qstate_addr, TCP_TCB_OOO_BOOK_KEEPING_OFFSET0,
                        TABLE_SIZE_512_BITS)
    phvwr           p.t2_s2s_seq, k.s1_s2s_seq
    phvwr           p.t2_s2s_payload_len, k.s1_s2s_payload_len
    phvwr           p.common_phv_ooo_rcv, 1
    // we need to allocate a descriptor
    tblwr.l         d.u.tcp_rx_d.alloc_descr, 1
    b               flow_rx_process_done
    tbladd          d.{u.tcp_rx_d.ooo_cnt}.hx, 1
