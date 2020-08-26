// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#include <netinet/in.h>

#include "platform/capri/capri_tbl_rw.hpp"
#include "platform/capri/capri_common.hpp"
#include "include/sdk/platform.hpp"
#include "asic/rw/asicrw.hpp"
#include "nic/include/tcp_common.h"
#include "tcpcb_pd.hpp"
#include "tcpcb_pd_internal.hpp"
#include "gen/p4gen/tcp_proxy_rxdma/include/tcp_proxy_rxdma_p4plus_ingress.h"

namespace sdk {
namespace tcp {

//------------------------------------------------------------------------------
// TCPCB set helper routines
//------------------------------------------------------------------------------
static sdk_ret_t
tcpcb_pd_set_tcp_rx_tx2rx_entry(tcpcb_pd_t *cb)
{
    common_p4plus_stage0_app_header_table_d data = { 0 };
    uint64_t addr = cb->base_pa[0] + TCP_TCB_TX2RX_SHARED_OFFSET;
    sdk_ret_t ret;

    data.action_id = cb->rxdma_action_id;
    data.u.read_tx2rx_d.debug_dol = htons(cb->debug_dol);
    data.u.read_tx2rx_d.snd_nxt = htonl(cb->snd_nxt);
    data.u.read_tx2rx_d.rcv_wup = htonl(cb->rcv_wup);
    data.u.read_tx2rx_d.rcv_wnd_adv = htons(cb->rcv_wnd);
    data.u.read_tx2rx_d.serq_cidx = htons((uint16_t)cb->serq_ci);
    data.u.read_tx2rx_d.rto_event = 0;
    
    SDK_TRACE_DEBUG("snd_nxt = %u", cb->snd_nxt);

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_rx_entry(tcpcb_pd_t *cb)
{
    s2_t0_tcp_rx_tcp_rx_d data = { 0 };
    uint64_t addr = cb->base_pa[0] + TCP_TCB_RX_OFFSET;
    sdk_ret_t ret;
    data.ts_recent= htonl(cb->ts_recent);
    data.rcv_nxt = htonl(cb->rcv_nxt);
    data.snd_una = htonl(cb->snd_una);
    data.snd_wnd = htons((uint16_t)cb->snd_wnd);
    data.serq_pidx = htons((uint16_t)cb->serq_pi);
    data.state = (uint8_t)cb->state;
    data.pred_flags = htonl(cb->pred_flags);
    data.rcv_wscale = cb->rcv_wscale;
    if (cb->delay_ack) {
        data.cfg_flags |= TCP_OPER_FLAG_DELACK;
        data.ato = htons(cb->ato / TCP_TIMER_TICK);
        data.quick = TCP_QUICKACKS;
    }
    if (cb->ooo_queue) {
        data.cfg_flags |= TCP_OPER_FLAG_OOO_QUEUE;
    }
    if (cb->keepalives) {
        data.cfg_flags |= TCP_OPER_FLAG_KEEPALIVES;
    }
    switch (data.state) {
        case TCP_SYN_SENT:
        case TCP_SYN_RECV:
        case TCP_CLOSE:
        case TCP_LISTEN:
        case TCP_NEW_SYN_RECV:
            data.parsed_state |= TCP_PARSED_STATE_HANDLE_IN_CPU;
            break;
        default:
            data.parsed_state &= ~TCP_PARSED_STATE_HANDLE_IN_CPU;}
    data.consumer_ring_shift = cb->lg2_app_num_slots;
    SDK_TRACE_DEBUG("rcv_nxt = %u, snd_una = %u", cb->rcv_nxt, cb->snd_una);
    SDK_TRACE_DEBUG("rcv_wscale = %d", cb->rcv_wscale);
    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_rx_rtt_entry(tcpcb_pd_t *cb)
{
    s4_t0_tcp_rx_tcp_rtt_d data = { 0 };
    uint64_t addr = cb->base_pa[0] + TCP_TCB_RTT_OFFSET;
    sdk_ret_t ret;

    data.rto = htonl(cb->rto);  
    data.srtt_us = htonl(cb->srtt_us); 
    data.curr_ts = 0;
    data.rttvar_us = htonl(cb->rttvar_us); 
    data.rtt_updated = 0;
    data.rtt_seq_tsoffset = htonl(cb->rtt_seq_tsoffset); 
    data.rtt_time = 0;
    data.ts_learned = 0;
    SDK_TRACE_DEBUG("base_pa 0x%lx , rto = %u, srtt_us = %u, rttvar_us = %u", addr,
                    ntohl(cb->rto), ntohl(cb->srtt_us), ntohl(cb->rttvar_us));

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_rx_cc_entry(tcpcb_pd_t *cb)
{
    s5_t1_tcp_rx_d data = { 0 };
    uint64_t addr = cb->base_pa[0] + TCP_TCB_CC_OFFSET;
    sdk_ret_t ret;

    data.u.tcp_cc_d.cc_algo = TCP_CC_ALGO_NEW_RENO;
    data.u.tcp_cc_d.smss = htons(cb->smss);
    data.u.tcp_cc_d.snd_cwnd = htonl(cb->snd_cwnd);
    data.u.tcp_cc_d.snd_ssthresh = htonl(cb->snd_ssthresh);
//    SDK_TRACE_DEBUG("smss %u , snd_cwnd = %u, snd_ssthresh %u", cb->smss, cb->snd_cwnd, cb->snd_cwnd);
    if (cb->ecn) {
        data.u.tcp_cc_d.ecn_enabled = 1;
    }
    data.u.tcp_cc_d.snd_wscale = cb->snd_wscale;
    // 'L' value for ABC : Appropriate Byte Counting
    data.u.tcp_cc_d.abc_l_var = cb->abc_l_var;
    if(data.u.tcp_cc_d.cc_algo == TCP_CC_ALGO_NEW_RENO) {
        data.u.tcp_cc_new_reno_d.smss_times_abc_l = htonl(cb->smss *
            data.u.tcp_cc_d.abc_l_var);
        data.u.tcp_cc_new_reno_d.smss_squared = htonl(cb->smss *
            cb->smss);
    }
    if(data.u.tcp_cc_d.cc_algo == TCP_CC_ALGO_CUBIC) {
        data.u.tcp_cc_cubic_d.t_last_cong = 1;  //should be current 10us snapshot
        data.u.tcp_cc_cubic_d.min_rtt_ticks = 0;
        data.u.tcp_cc_cubic_d.mean_rtt_ticks = 1;
    }

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_rx_fc_entry(tcpcb_pd_t *cb)
{
    s5_t0_tcp_rx_tcp_fc_d data = { 0 };
    uint64_t addr = cb->base_pa[0] + TCP_TCB_FC_OFFSET;
    sdk_ret_t ret;

    data.rcv_wnd = htonl(cb->rcv_wnd);
    data.read_notify_addr = htonl(cb->read_notify_addr);
    data.rcv_scale = cb->rcv_wscale;
    data.cpu_id = cb->cpu_id;
    data.rcv_wup = htonl(cb->rcv_wup);
    int num_slots = 1 << cb->lg2_app_num_slots;
    data.consumer_ring_slots = htons(num_slots - 1);
    data.consumer_ring_slots_mask = htons(num_slots - 1);
    data.high_thresh1 = (uint16_t)htons(num_slots * .75);
    data.high_thresh2 = (uint16_t)htons(num_slots * .50);
    data.high_thresh3 = (uint16_t)htons(num_slots * .25);
    data.high_thresh4 = (uint16_t)htons(num_slots * .125);
    data.avg_pkt_size_shift = 7;
    data.rcv_mss = htons(cb->rcv_mss);
    data.rnmdr_gc_base = htonll(cb->rx_gc_base_addr);

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_rx_dma_entry(tcpcb_pd_t *cb)
{
    s6_t0_tcp_rx_dma_d data = { 0 };
    uint64_t addr = cb->base_pa[0] + TCP_TCB_RX_DMA_OFFSET;
    sdk_ret_t ret;

    data.rx_stats_base = htonll(cb->stats_base);
    data.app_type_cfg = cb->app_type;
    data.consumer_lif = htons(cb->app_lif);
    data.serq_base = htonll(cb->serq_base);
    data.consumer_qid = htons(cb->app_qid);
    data.consumer_qtype = cb->app_qtype;
    data.consumer_ring = cb->app_ring;
    data.nde_shift = cb->app_nde_shift;
    data.nde_offset = cb->app_nde_offset;
    data.nde_len = 1 << cb->app_nde_shift;
    data.consumer_num_slots_mask = htons((1 << cb->lg2_app_num_slots) - 1);
    data.page_headroom = cb->page_headroom;

    SDK_TRACE_DEBUG("serq_base = 0x%lx", cb->serq_base);

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_rx_ooq_cb_load_entry(tcpcb_pd_t *cb)
{
    s6_t2_tcp_rx_ooo_qbase_cb_load_d data = { 0 };
    uint64_t addr = cb->base_pa[0] + TCP_TCB_OOO_QADDR_OFFSET;
    sdk_ret_t ret;

    data.ooo_rx2tx_qbase = htonll(cb->ooo_rx2tx_qbase);

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_rx_ooq_book_keeping_entry(tcpcb_pd_t *cb)
{
    s3_t2_tcp_rx_ooo_book_keeping_d data = { 0 };
    uint64_t addr = cb->base_pa[0] + TCP_TCB_OOO_BOOK_KEEPING_OFFSET0;
    sdk_ret_t ret;

    if (cb->sack_perm) {
        data.tcp_opt_flags |= TCP_OPT_FLAG_SACK_PERM;
    }
    if (cb->timestamps) {
        data.tcp_opt_flags |= TCP_OPT_FLAG_TIMESTAMPS;
    }

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

//------------------------------------------------------------------------------
// Set TCP RxDMA CB
//------------------------------------------------------------------------------
sdk_ret_t
tcpcb_pd_set_rxdma(tcpcb_pd_t *cb)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_rx_tx2rx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_rx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_rx_rtt_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_rx_cc_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_rx_fc_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_rx_dma_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_rx_ooq_cb_load_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_rx_ooq_book_keeping_entry(cb));

    return ret;
}

//------------------------------------------------------------------------------
// TCPCB get helper routines
//------------------------------------------------------------------------------
static sdk_ret_t
tcpcb_pd_get_tcp_rx_tx2rx_entry(tcpcb_pd_t *cb)
{
    common_p4plus_stage0_app_header_table_d data;
    uint64_t addr = cb->base_pa[0] + TCP_TCB_TX2RX_SHARED_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->rx_ts = ntohll(data.u.read_tx2rx_d.rx_ts);
    cb->debug_dol = ntohs(data.u.read_tx2rx_d.debug_dol);
    cb->snd_nxt = ntohl(data.u.read_tx2rx_d.snd_nxt);
    cb->serq_ci = ntohs(data.u.read_tx2rx_d.serq_cidx);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_rx_entry(tcpcb_pd_t *cb)
{
    s2_t0_tcp_rx_tcp_rx_d data;
    uint64_t addr = cb->base_pa[0] + TCP_TCB_RX_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->rcv_nxt = ntohl(data.rcv_nxt);
    cb->snd_una = ntohl(data.snd_una);
    cb->snd_wnd = ntohs(data.snd_wnd);
    cb->serq_pi = ntohs(data.serq_pidx);
    cb->ts_recent = ntohl(data.ts_recent);
    cb->state = data.state;
    cb->pred_flags = ntohl(data.pred_flags);
    cb->snd_recover = ntohl(data.snd_recover);
    cb->ooq_not_empty = data.ooq_not_empty;
    if (data.cfg_flags & TCP_OPER_FLAG_DELACK) {
        cb->delay_ack = true;
    }
    if (data.cfg_flags & TCP_OPER_FLAG_OOO_QUEUE) {
        cb->ooo_queue = true;
    }
    cb->ato = ntohs(data.ato * TCP_TIMER_TICK);
    cb->cc_flags = data.cc_flags;

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_rx_rtt_entry(tcpcb_pd_t *cb)
{
    s4_t0_tcp_rx_tcp_rtt_d data;
    uint64_t addr = cb->base_pa[0] + TCP_TCB_RTT_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->rto = ntohl(data.rto);
    cb->srtt_us = ntohl(data.srtt_us);
    cb->rtt_seq_tsoffset = ntohl(data.rtt_seq_tsoffset);
    cb->ts_offset = ntohl(data.rtt_seq_tsoffset);
    cb->ts_time = ntohl(data.rtt_time);
    cb->rtt_time = ntohl(data.rtt_time);
    cb->ts_learned = data.ts_learned;

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_rx_cc_entry(tcpcb_pd_t *cb)
{
    s5_t1_tcp_rx_d data;
    uint64_t addr = cb->base_pa[0] + TCP_TCB_CC_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->cc_algo = data.u.tcp_cc_d.cc_algo;
    cb->snd_cwnd = ntohl(data.u.tcp_cc_d.snd_cwnd);
    cb->smss = ntohs(data.u.tcp_cc_d.smss);
    cb->snd_ssthresh = ntohl(data.u.tcp_cc_d.snd_ssthresh);
    cb->abc_l_var = data.u.tcp_cc_d.abc_l_var;
    if (data.u.tcp_cc_d.ecn_enabled) {
        cb->ecn = true;
    }

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_rx_fc_entry(tcpcb_pd_t *cb)
{
    s5_t0_tcp_rx_tcp_fc_d data;
    uint64_t addr = cb->base_pa[0] + TCP_TCB_FC_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->cpu_id = data.cpu_id;
    cb->rcv_wnd = ntohl(data.rcv_wnd);
    cb->rcv_wscale = data.rcv_scale;
    cb->rcv_mss = ntohs(data.rcv_mss);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_rx_dma_entry(tcpcb_pd_t *cb)
{
    s6_t0_tcp_rx_dma_d data;
    uint64_t addr = cb->base_pa[0] + TCP_TCB_RX_DMA_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->serq_base = ntohll(data.serq_base);
    cb->app_type = data.app_type_cfg;

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_rx_ooq_book_keeping_entry(tcpcb_pd_t *cb)
{
    s6_t2_tcp_rx_ooo_qbase_cb_load_d cb_load_d;
    s3_t2_tcp_rx_ooo_book_keeping_d bookkeeping_d;
    uint64_t addr1 = cb->base_pa[0] + TCP_TCB_OOO_BOOK_KEEPING_OFFSET0;
    uint64_t addr2 = cb->base_pa[0] + TCP_TCB_OOO_QADDR_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr1, (uint8_t *)&bookkeeping_d, sizeof(bookkeeping_d));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = tcpcb_pd_read_one(addr2, (uint8_t *)&cb_load_d, sizeof(cb_load_d));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    if (bookkeeping_d.tcp_opt_flags & TCP_OPT_FLAG_SACK_PERM) {
        cb->sack_perm = true;
    }
    if (bookkeeping_d.tcp_opt_flags & TCP_OPT_FLAG_TIMESTAMPS) {
        cb->timestamps = true;
    }

    cb->ooq_entry[0].queue_addr = ntohll(cb_load_d.ooo_qbase_addr0);
    cb->ooq_entry[0].start_seq = ntohl(bookkeeping_d.start_seq0);
    cb->ooq_entry[0].end_seq = ntohl(bookkeeping_d.end_seq0);
    cb->ooq_entry[0].num_entries = ntohs(bookkeeping_d.tail_index0);

    cb->ooq_entry[1].queue_addr = ntohll(cb_load_d.ooo_qbase_addr1);
    cb->ooq_entry[1].start_seq = ntohl(bookkeeping_d.start_seq1);
    cb->ooq_entry[1].end_seq = ntohl(bookkeeping_d.end_seq1);
    cb->ooq_entry[1].num_entries = ntohs(bookkeeping_d.tail_index1);

    cb->ooq_entry[2].queue_addr = ntohll(cb_load_d.ooo_qbase_addr2);
    cb->ooq_entry[2].start_seq = ntohl(bookkeeping_d.start_seq2);
    cb->ooq_entry[2].end_seq = ntohl(bookkeeping_d.end_seq2);
    cb->ooq_entry[2].num_entries = ntohs(bookkeeping_d.tail_index2);

    cb->ooq_entry[3].queue_addr = ntohll(cb_load_d.ooo_qbase_addr3);
    cb->ooq_entry[3].start_seq = ntohl(bookkeeping_d.start_seq3);
    cb->ooq_entry[3].end_seq = ntohl(bookkeeping_d.end_seq3);
    cb->ooq_entry[3].num_entries = ntohs(bookkeeping_d.tail_index3);

    return ret;
}

//------------------------------------------------------------------------------
// Get TCP RxDMA CB
//------------------------------------------------------------------------------
sdk_ret_t
tcpcb_pd_get_rxdma(tcpcb_pd_t *cb)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_rx_tx2rx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_rx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_rx_rtt_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_rx_cc_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_rx_fc_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_rx_dma_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_rx_ooq_book_keeping_entry(cb));

    return ret;
}

//------------------------------------------------------------------------------
// TCPCB get stats helper routines
//------------------------------------------------------------------------------
static sdk_ret_t
tcpcb_pd_get_stats_tcp_rx_entry(tcpcb_stats_pd_t *cb)
{
    s2_t0_tcp_rx_tcp_rx_d data;
    tcp_rx_stats_t stats;
    uint64_t addr = cb->base_pa[0] + TCP_TCB_RX_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    addr = cb->stats_base + TCP_RX_PER_FLOW_STATS_OFFSET;
    ret = tcpcb_pd_read_one(addr, (uint8_t *)&stats, sizeof(stats));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->bytes_rcvd = ntohs(data.bytes_rcvd) + stats.bytes_rcvd;
    cb->bytes_acked = ntohs(data.bytes_acked) + stats.bytes_acked;
    cb->pure_acks_rcvd = data.pure_acks_rcvd + stats.pure_acks_rcvd;
    cb->dup_acks_rcvd = data.dup_acks_rcvd + stats.dup_acks_rcvd;
    cb->ooo_cnt = data.ooo_cnt + stats.ooo_cnt;

    cb->slow_path_cnt = ntohs(data.slow_path_cnt);
    cb->serq_full_cnt = ntohs(data.serq_full_cnt);
    cb->rx_drop_cnt = ntohs(data.rx_drop_cnt);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_stats_tcp_rx_dma_entry(tcpcb_stats_pd_t *cb)
{
    s6_t0_tcp_rx_dma_d data;
    tcp_rx_stats_t stats;
    uint64_t addr = cb->base_pa[0] + TCP_TCB_RX_DMA_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    addr = cb->stats_base + TCP_RX_PER_FLOW_STATS_OFFSET;
    ret = tcpcb_pd_read_one(addr, (uint8_t *)&stats, sizeof(stats));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->pkts_rcvd = data.pkts_rcvd + stats.pkts_rcvd;

    return ret;
}

//------------------------------------------------------------------------------
// Get TCP RxDMA stats
//------------------------------------------------------------------------------
sdk_ret_t
tcpcb_pd_get_stats_rxdma(tcpcb_stats_pd_t *cb)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(tcpcb_pd_get_stats_tcp_rx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_stats_tcp_rx_dma_entry(cb));

    return ret;
}

} // namespace tcp
} // namespace sdk
