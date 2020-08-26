// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#ifndef __TCPCB_PD_H__
#define __TCPCB_PD_H__

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define MAX_TCP_QTYPES 4
#define NUM_TCP_OOO_QUEUES_PER_FLOW     4

typedef uint64_t    tcpcb_hw_id_t;

//------------------------------------------------------------------------------
// Data structures
//------------------------------------------------------------------------------
typedef struct tcpcb_ooo_queue_entry_s {
    uint64_t                queue_addr;
    uint32_t                start_seq;
    uint32_t                end_seq;
    uint32_t                num_entries;
} tcpcb_ooq_entry_t;

typedef struct tcpcb_pd_s {
    // meta information
    uint32_t            qid;
    uint32_t            other_qid;
    uint32_t            app_qid;
    uint16_t            cpu_id;
    uint16_t            app_lif;
    uint8_t             app_type;
    uint8_t             lg2_app_num_slots;
    uint8_t             app_qtype;
    uint8_t             app_ring;
    uint8_t             app_nde_shift;
    uint8_t             app_nde_offset;
    uint8_t             rxdma_action_id;
    uint8_t             txdma_action_id;
    uint8_t             txdma_ooq_action_id;
    uint8_t             lg2_sesq_num_slots;
    uint8_t             page_headroom;
    uint64_t            base_pa[MAX_TCP_QTYPES];
    uint64_t            base_va[MAX_TCP_QTYPES];
    uint64_t            other_base_pa[MAX_TCP_QTYPES];
    uint64_t            other_base_va[MAX_TCP_QTYPES];
    uint64_t            stats_base;
    uint64_t            serq_base;
    uint64_t            sesq_base;
    uint64_t            asesq_base;
    uint64_t            sesq_prod_ci_addr;
    uint64_t            serq_prod_ci_addr;
    uint64_t            gc_base_addr;
    uint64_t            rx_gc_base_addr;
    uint64_t            ooo_rx2tx_qbase;
    uint64_t            ooo_rx2tx_free_pi_addr;
    uint64_t            ooo_prod_ci_addr;
    uint64_t            read_notify_addr;

    // configured values
    uint8_t             header_template[64];
    uint32_t            rcv_nxt;
    uint32_t            debug_dol;
    uint32_t            debug_dol_tx;
    uint32_t            snd_nxt;
    uint32_t            rcv_wup;
    uint32_t            rcv_wnd;
    uint32_t            snd_una;
    uint32_t            pred_flags;
    uint32_t            rcv_wscale;
    uint32_t            ato;
    uint32_t            snd_wnd;
    uint32_t            rtt_seq_tsoffset;
    uint32_t            rtt_time;
    uint8_t             ts_learned;
    uint8_t             cc_algo;
    bool                delay_ack;
    bool                ooo_queue;
    bool                sack_perm;
    bool                timestamps;
    bool                ecn;
    bool                keepalives;
    uint32_t            smss;
    uint32_t            snd_cwnd;
    uint32_t            snd_ssthresh;
    uint32_t            snd_wscale;
    uint32_t            abc_l_var;
    uint32_t            rcv_mss;
    uint32_t            initial_window;
    uint16_t            source_lif;
    uint16_t            source_port;
    uint16_t            dest_port;
    uint32_t            header_len;
    uint32_t            ts_offset;
    uint32_t            ts_time;
    uint32_t            ts_recent;
    uint32_t            snd_cwnd_cnt;

    // stats
    uint32_t            serq_ci;
    uint32_t            serq_pi;

    // status/updated in datapath
    uint32_t            state;
    uint64_t            rx_ts;
    uint32_t            snd_recover;
    bool                ooq_not_empty;
    tcpcb_ooq_entry_t   ooq_entry[NUM_TCP_OOO_QUEUES_PER_FLOW];
    uint32_t            cc_flags;
    uint32_t            rto;
    uint32_t            srtt_us;
    uint32_t            rttvar_us;
    uint32_t            retx_snd_una;
    uint16_t            ato_deadline;
    uint16_t            rto_deadline;
    uint16_t            keepa_deadline;
    uint16_t            rto_backoff;
    uint16_t            zero_window_sent;
    uint32_t            rcv_tsval;
    uint32_t            read_notify_bytes;
    uint32_t            read_notify_bytes_local;
    uint32_t            ooq_rx2tx_pi;
    uint32_t            ooq_rx2tx_ci;
    uint32_t            window_update_pi;
    uint32_t            window_update_ci;
} tcpcb_pd_t;

typedef struct tcpcb_stats_pd_s {
    // meta information
    uint32_t            qid;
    uint64_t            base_pa[MAX_TCP_QTYPES];
    uint64_t            base_va[MAX_TCP_QTYPES];
    uint64_t            stats_base;

    // rx
    uint64_t            bytes_rcvd;
    uint64_t            pkts_rcvd;
    uint64_t            bytes_acked;
    uint64_t            pure_acks_rcvd;
    uint64_t            dup_acks_rcvd;
    uint64_t            slow_path_cnt;
    uint64_t            serq_full_cnt;
    uint64_t            ooo_cnt;
    uint64_t            rx_drop_cnt;

    // tx
    uint64_t            bytes_sent;
    uint64_t            pkts_sent;
    uint64_t            pure_acks_sent;
    uint32_t            tx_ring_pi;
    uint32_t            packets_out;
    uint32_t            partial_pkt_ack_cnt;
    uint32_t            window_full_cnt;
    uint32_t            retx_cnt;
    uint32_t            zero_window_sent;
    uint32_t            tx_window_update_pi;

    // rings
    uint16_t            sesq_pi;
    uint16_t            sesq_ci;
    uint16_t            sesq_retx_ci;
    uint16_t            sesq_tx_ci;
    uint16_t            send_ack_pi;
    uint16_t            send_ack_ci;
    uint16_t            del_ack_pi;
    uint16_t            del_ack_ci;
    uint16_t            fast_timer_pi;
    uint16_t            fast_timer_ci;
    uint16_t            asesq_pi;
    uint16_t            asesq_ci;
    uint16_t            asesq_retx_ci;
    uint16_t            pending_tx_pi;
    uint16_t            pending_tx_ci;
    uint16_t            fast_retrans_pi;
    uint16_t            fast_retrans_ci;
    uint16_t            clean_retx_pi;
    uint16_t            clean_retx_ci;
} tcpcb_stats_pd_t;

typedef struct tcp_proxy_global_stats_s {
    uint64_t rnmdr_full;
    uint64_t invalid_sesq_descr;
    uint64_t invalid_retx_sesq_descr;
    uint64_t partial_pkt_ack;
    uint64_t retx_nop_schedule;
    uint64_t gc_full;
    uint64_t tls_gc_full;
    uint64_t ooq_full;
    uint64_t invalid_nmdr_descr;
    uint64_t rcvd_ce_pkts;
    uint64_t ecn_reduced_congestion;
    uint64_t retx_pkts;
    uint64_t ooq_rx2tx_full;
    uint64_t rx_ack_for_unsent_data;
    uint64_t fin_sent_cnt;
    uint64_t rst_sent_cnt;
    uint64_t rx_win_probe;
    uint64_t rx_keep_alive;
    uint64_t rx_pkt_after_win;
    uint64_t rx_pure_win_upd;
    uint64_t rnmdr_avail; // dummy location in HBM, real value in CSR
    uint64_t tnmdr_avail; // dummy location in HBM, real value in CSR
    uint64_t ooq_avail; // dummy location in HBM, real value in CSR

    uint64_t tcp_debug1;
    uint64_t tcp_debug2;
    uint64_t tcp_debug3;
    uint64_t tcp_debug4;
    uint64_t tcp_debug5;
    uint64_t tcp_debug6;
    uint64_t tcp_debug7;
    uint64_t tcp_debug8;
    uint64_t tcp_debug9;
    uint64_t tcp_debug10;
} tcp_proxy_global_stats_t;

#endif // __TCPCB_PD_H__
