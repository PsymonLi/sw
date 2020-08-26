// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#include <netinet/in.h>

#include "platform/capri/capri_tbl_rw.hpp"
#include "platform/capri/capri_common.hpp"
#include "include/sdk/platform.hpp"
#include "asic/rw/asicrw.hpp"
#include "nic/include/pds_socket.h"
#include "nic/include/tcp_common.h"
#include "tcpcb_pd.hpp"
#include "tcpcb_pd_internal.hpp"
#include "gen/p4gen/tcp_proxy_txdma/include/tcp_proxy_txdma_p4plus_ingress.h"
#include "gen/p4gen/tcp_proxy_ooq_txdma/include/tcp_proxy_ooq_txdma_p4plus_ingress.h"

namespace sdk {
namespace tcp {

//------------------------------------------------------------------------------
// Set TCP TxDMA helpers
//------------------------------------------------------------------------------
static sdk_ret_t
tcpcb_pd_set_tcp_ooo_rx2tx_entry(tcpcb_pd_t *cb)
{
    s0_t0_ooq_tcp_tx_d data = { 0 };
    uint64_t addr = cb->base_pa[TCP_QTYPE1];
    sdk_ret_t ret;
    uint8_t *ptr_u8;

    data.action_id = cb->txdma_ooq_action_id;
    data.u.load_stage0_d.total = TCP_PROXY_OOO_RX2TX_TOTAL_RINGS;
    data.u.load_stage0_d.ooo_rx2tx_qbase = htonll(cb->ooo_rx2tx_qbase);
    data.u.load_stage0_d.ooo_rx2tx_free_pi_addr =
        htonll(cb->ooo_rx2tx_free_pi_addr);
    data.u.load_stage0_d.ooo_rx2tx_producer_ci_addr =
        htonll(cb->ooo_prod_ci_addr);

    ptr_u8 = ((uint8_t *)&data) + TCP_TCB_CLEANUP_BMAPS_BYTE_OFFSET;
    if (cb->app_type == TCP_APP_TYPE_NVME ||
          cb->app_type == TCP_APP_TYPE_ARM_SOCKET) {
        //data.u.load_stage0_d.cleanup_cond_bmap = 0x3;
        *ptr_u8 = (TCP_CLEANUP_COND2_BIT | TCP_CLEANUP_COND1_BIT) <<
              TCP_TCB_CLEANUP_BMAP_BIT_SHIFT;
    } else {
        *ptr_u8 = TCP_CLEANUP_COND2_BIT << TCP_TCB_CLEANUP_BMAP_BIT_SHIFT;
    }

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_tx_rx2tx_entry(tcpcb_pd_t *cb)
{
    s0_t0_tcp_tx_d data = { 0 };
    uint64_t addr = cb->base_pa[TCP_QTYPE0];
    sdk_ret_t ret;

    data.action_id = cb->txdma_action_id; 
    data.u.read_rx2tx_d.total = TCP_PROXY_TX_TOTAL_RINGS;
    data.u.read_rx2tx_d.eval_last = 1 << TCP_SCHED_RING_FAST_TIMER;
    data.u.read_rx2tx_d.debug_dol_tx = htons(cb->debug_dol_tx);
    // get sesq address
    data.u.read_rx2tx_d.sesq_base_msb = (cb->sesq_base >> 32) & 0xff;
    data.u.read_rx2tx_d.sesq_base = htonl(cb->sesq_base & 0xffffffff);
    // set sesq_tx_ci to invalid
    // This is set to valid ci only in window full conditions
    // otherwise transmission happens from ci_0
    data.u.read_rx2tx_d.sesq_tx_ci = TCP_TX_INVALID_SESQ_TX_CI;

    SDK_TRACE_DEBUG("sesq_base = 0x%lx", cb->sesq_base);

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_tx_rx2tx_extra_entry(tcpcb_pd_t *cb)
{
    s1_t0_tcp_tx_read_rx2tx_extra_d data = { 0 };
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_RX2TX_SHARED_EXTRA_OFFSET;
    sdk_ret_t ret;

    data.snd_una = htonl(cb->snd_una);
    data.snd_wnd = htons(cb->snd_wnd);
    data.snd_cwnd = htonl(cb->snd_cwnd);
    data.rcv_wnd = htons(cb->rcv_wnd >> cb->rcv_wscale);
    data.rcv_nxt = htonl(cb->rcv_nxt);
    data.rto = htonl(cb->rto);
    data.state = (uint8_t)cb->state;
    data.rcv_tsval = htonl(cb->ts_recent);
    data.ts_offset = htonl(cb->rtt_seq_tsoffset);
    data.ts_time = 0;
    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_tx_retx_entry(tcpcb_pd_t *cb)
{
    s3_t0_tcp_tx_retx_d data = { 0 };
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_RETX_OFFSET;
    sdk_ret_t ret;

    data.retx_snd_una = htonl(cb->snd_una);
    data.gc_base = htonll(cb->gc_base_addr);
    if (cb->app_type == TCP_APP_TYPE_BYPASS) {
        // Used by window update logic to wakeup proxy peer TCP segment queue
        data.consumer_qid = htons(cb->other_qid);
    }

    SDK_TRACE_DEBUG("gc_base = 0x%lx", cb->gc_base_addr);

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_tx_xmit_entry(tcpcb_pd_t *cb)
{
    s4_t0_tcp_tx_xmit_d data = { 0 };
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_XMIT_OFFSET;
    sdk_ret_t ret;

    data.snd_nxt = htonl(cb->snd_nxt);
    data.snd_wscale = cb->snd_wscale;
    data.is_cwnd_limited = 0x00;
    data.smss = htons(cb->smss);
    data.initial_window = htonl(cb->initial_window);
    data.rtt_seq_req = 1;
    data.sesq_ci_addr = htonll(cb->sesq_prod_ci_addr);

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_tx_tso_entry(tcpcb_pd_t *cb)
{
    s5_t0_tcp_tx_tso_d data = { 0 };
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_TSO_OFFSET;
    sdk_ret_t ret;

    data.tx_stats_base = htonll(cb->stats_base + TCP_TX_PER_FLOW_STATS_OFFSET);
    data.source_lif = htons(cb->source_lif);
    data.source_port = htons(cb->source_port);
    data.dest_port = htons(cb->dest_port);
    data.header_len = htons(cb->header_len);
    data.smss = htons(cb->smss);
    data.ts_offset = htonl(cb->rtt_seq_tsoffset);
    data.ts_time   = 0;

    if (cb->sack_perm) {
        data.tcp_opt_flags |= TCP_OPT_FLAG_SACK_PERM;
    }
    if (cb->timestamps && cb->ts_recent) {
        data.tcp_opt_flags |= TCP_OPT_FLAG_TIMESTAMPS;
    }

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
tcpcb_pd_set_tcp_tx_hdr_template_entry(tcpcb_pd_t *cb)
{
    uint8_t data[CACHE_LINE_SIZE];
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_HEADER_TEMPLATE_OFFSET;
    sdk_ret_t ret;

    memcpy(data, cb->header_template, sizeof(data));

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

//------------------------------------------------------------------------------
// Set TCP TxDMA CB
//------------------------------------------------------------------------------
sdk_ret_t
tcpcb_pd_set_txdma(tcpcb_pd_t *cb)
{
    sdk_ret_t ret;

    // qtype 1
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_ooo_rx2tx_entry(cb));
    // qtype 0
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_tx_rx2tx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_tx_rx2tx_extra_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_tx_retx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_tx_xmit_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_tx_tso_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_set_tcp_tx_hdr_template_entry(cb));

    return ret;
}

//------------------------------------------------------------------------------
// Get TCP TxDMA helpers
//------------------------------------------------------------------------------
static sdk_ret_t
tcpcb_pd_get_tcp_ooo_rx2tx_entry(tcpcb_pd_t *cb)
{
    s0_t0_ooq_tcp_tx_d data;
    uint64_t addr = cb->base_pa[TCP_QTYPE1];
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->ooq_rx2tx_pi = data.u.load_stage0_d.pi_0;
    cb->ooq_rx2tx_ci = data.u.load_stage0_d.ci_0;
    cb->window_update_pi = data.u.load_stage0_d.pi_1;
    cb->window_update_ci = data.u.load_stage0_d.ci_1;

    cb->ooo_rx2tx_qbase = htonll(data.u.load_stage0_d.ooo_rx2tx_qbase);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_tx_rx2tx_entry(tcpcb_pd_t *cb)
{
    s0_t0_tcp_tx_d data;
    uint64_t addr = cb->base_pa[TCP_QTYPE0];
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->sesq_base = ntohl(data.u.read_rx2tx_d.sesq_base);
    cb->sesq_base |= ((uint64_t)data.u.read_rx2tx_d.sesq_base_msb << 32);
    cb->rto_deadline = ntohs(data.u.read_rx2tx_d.rto_deadline);
    cb->ato_deadline = ntohs(data.u.read_rx2tx_d.ato_deadline);
    cb->keepa_deadline = ntohs(data.u.read_rx2tx_d.keepa_deadline);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_tx_rx2tx_extra_entry(tcpcb_pd_t *cb)
{
    s1_t0_tcp_tx_read_rx2tx_extra_d data;
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_RX2TX_SHARED_EXTRA_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->rcv_tsval = ntohs(data.rcv_tsval);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_tx_retx_entry(tcpcb_pd_t *cb)
{
    s3_t0_tcp_tx_retx_d data;
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_RETX_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->retx_snd_una = ntohl(data.retx_snd_una);
    cb->read_notify_bytes = ntohl(data.read_notify_bytes);
    cb->read_notify_bytes_local = ntohl(data.read_notify_bytes_local);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_tx_xmit_entry(tcpcb_pd_t *cb)
{
    s4_t0_tcp_tx_xmit_d data;
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_XMIT_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->rto_backoff = data.rto_backoff;
    cb->snd_wscale = data.snd_wscale;
    cb->initial_window = ntohl(data.initial_window);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_tx_tso_entry(tcpcb_pd_t *cb)
{
    s5_t0_tcp_tx_tso_d data;
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_TSO_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->source_lif  =  ntohs(data.source_lif);
    cb->source_port = ntohs(data.source_port);
    cb->dest_port = ntohs(data.dest_port);
    cb->header_len = ntohs(data.header_len);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_tcp_tx_hdr_template_entry(tcpcb_pd_t *cb)
{
    uint8_t data[CACHE_LINE_SIZE];
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_HEADER_TEMPLATE_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    memcpy(cb->header_template, data, sizeof(cb->header_template));

    ret = tcpcb_pd_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

//------------------------------------------------------------------------------
// Get TCP TxDMA CB
//------------------------------------------------------------------------------
sdk_ret_t
tcpcb_pd_get_txdma(tcpcb_pd_t *cb)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_ooo_rx2tx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_tx_rx2tx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_tx_rx2tx_extra_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_tx_retx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_tx_xmit_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_tx_tso_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_tcp_tx_hdr_template_entry(cb));

    return ret;
}

//------------------------------------------------------------------------------
// TCPCB get stats helper routines
//------------------------------------------------------------------------------
static sdk_ret_t
tcpcb_pd_get_stats_tcp_tx_rx2tx_entry(tcpcb_stats_pd_t *cb)
{
    s0_t0_tcp_tx_d data = { 0 };
    tcp_tx_stats_t stats;
    uint64_t addr = cb->base_pa[TCP_QTYPE0];
    uint16_t *sesq_retx_ci_ptr;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    addr = cb->stats_base + TCP_TX_PER_FLOW_STATS_OFFSET;
    ret = tcpcb_pd_read_one(addr, (uint8_t *)&stats, sizeof(stats));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->sesq_pi = data.u.read_rx2tx_d.pi_0;
    cb->sesq_ci = data.u.read_rx2tx_d.ci_0;
    sesq_retx_ci_ptr = (uint16_t *)((uint64_t)&data + TCP_TCB_RX2TX_RETX_CI_OFFSET);
    /*
     * MSBit of half word at rtex_ci offset is used for End Marker flag 
     * Mask off MSBit to get sesq_retx_ci
     */
    cb->sesq_retx_ci = ntohs(*sesq_retx_ci_ptr) & ~SESQ_END_MARK_FLAG_BIT;
    cb->sesq_tx_ci = ntohs(data.u.read_rx2tx_d.sesq_tx_ci);
    cb->send_ack_ci = data.u.read_rx2tx_d.ci_1;
    cb->send_ack_pi = data.u.read_rx2tx_d.pi_1;
    cb->send_ack_ci = data.u.read_rx2tx_d.ci_1;
    cb->fast_timer_pi = data.u.read_rx2tx_d.pi_2;
    cb->fast_timer_ci = data.u.read_rx2tx_d.ci_2;
    cb->del_ack_pi = data.u.read_rx2tx_d.pi_3;
    cb->del_ack_ci = data.u.read_rx2tx_d.ci_3;
    cb->asesq_pi = data.u.read_rx2tx_d.pi_4;
    cb->asesq_ci = data.u.read_rx2tx_d.ci_4;
    cb->asesq_retx_ci = ntohs(data.u.read_rx2tx_d.asesq_retx_ci);
    cb->pending_tx_pi = data.u.read_rx2tx_d.pi_5;
    cb->pending_tx_ci = data.u.read_rx2tx_d.ci_5;
    cb->fast_retrans_pi = data.u.read_rx2tx_d.pi_6;
    cb->fast_retrans_ci = data.u.read_rx2tx_d.ci_6;
    cb->clean_retx_pi = data.u.read_rx2tx_d.pi_7;
    cb->clean_retx_ci = data.u.read_rx2tx_d.ci_7;

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_stats_tcp_tx_retx_entry(tcpcb_stats_pd_t *cb)
{
    s3_t0_tcp_tx_retx_d data;
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_RETX_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->tx_ring_pi = ntohs(data.tx_ring_pi);
    cb->partial_pkt_ack_cnt = ntohl(data.partial_pkt_ack_cnt);
    cb->tx_window_update_pi = ntohs(data.tx_window_update_pi);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_stats_tcp_tx_xmit_entry(tcpcb_stats_pd_t *cb)
{
    s4_t0_tcp_tx_xmit_d data;
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_XMIT_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->packets_out = ntohs(data.packets_out);
    cb->window_full_cnt = ntohl(data.window_full_cnt);
    cb->retx_cnt = ntohl(data.retx_cnt);

    return ret;
}

static sdk_ret_t
tcpcb_pd_get_stats_tcp_tx_tso_entry(tcpcb_stats_pd_t *cb)
{
    s5_t0_tcp_tx_tso_d data = { 0 };
    tcp_tx_stats_t stats;
    uint64_t addr = cb->base_pa[TCP_QTYPE0] + TCP_TCB_TSO_OFFSET;
    sdk_ret_t ret;

    ret = tcpcb_pd_read_one(addr, (uint8_t *)&data, sizeof(data));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    addr = cb->stats_base + TCP_TX_PER_FLOW_STATS_OFFSET;
    ret = tcpcb_pd_read_one(addr, (uint8_t *)&stats, sizeof(stats));
    if (ret != SDK_RET_OK) {
        return ret;
    }

    cb->zero_window_sent = ntohs(data.zero_window_sent);
    cb->bytes_sent = ntohs(data.bytes_sent) + stats.bytes_sent;
    cb->pkts_sent = data.pkts_sent + stats.pkts_sent;
    cb->pure_acks_sent = data.pure_acks_sent + stats.pure_acks_sent;

    return ret;
}

//------------------------------------------------------------------------------
// Get TCP TxDMA stats
//------------------------------------------------------------------------------
sdk_ret_t
tcpcb_pd_get_stats_txdma(tcpcb_stats_pd_t *cb)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(tcpcb_pd_get_stats_tcp_tx_rx2tx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_stats_tcp_tx_retx_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_stats_tcp_tx_xmit_entry(cb));
    CALL_AND_CHECK_FN(tcpcb_pd_get_stats_tcp_tx_tso_entry(cb));

    return ret;
}

} // namespace tcp
} // namespace sdk
