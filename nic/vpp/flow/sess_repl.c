/*
 *  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
 */

#include <vnet/buffer.h>
#include <pkt.h>
#include <sess_restore.h>
#include "node.h"
#include <flow.h>
#include <feature.h>
#include "sess_repl_state_tp.h"
#include "sess_repl.h"

static inline void
sess_iter_init (sess_iter_t *iter, bool v4)
{
    pds_flow_main_t *fm = &pds_flow_main;
    iter->v4 = v4;
    iter->read_index = -1;
    iter->pool = fm->session_index_pool;
    iter->ctx = NULL;
    iter->flow_table = v4 ? fm->table4 : fm->table6_or_l2;
}

static inline void
sess_iter_advance (sess_iter_t *iter)
{
    pds_flow_hw_ctx_t *tmp;
    while (1) {
        iter->read_index = pool_next_index(iter->pool, iter->read_index);
        if (iter->read_index != ~0) {
            tmp = pool_elt_at_index(iter->pool, iter->read_index);
            if (tmp->is_in_use && (tmp->v4 == iter->v4)) {
                break;
            }
        } else {
            tmp = NULL;
            break;
        }
    }
    iter->ctx = tmp;
}

void
pds_sess_sync_begin (sess_iter_t *iter, bool v4)
{
    sess_iter_init(iter, v4);
    pds_flow_prog_lock();
    // Advance to first valid hw_ctx we are interested in
    sess_iter_advance(iter);
}

void
pds_sess_sync_end (sess_iter_t *iter)
{
    pds_flow_prog_unlock();
    memset(iter, 0xbb, sizeof(*iter));
}

bool
pds_sess_v4_sync_cb (uint8_t *data, uint8_t *len, void *opaq)
{
    sess_iter_t *iter = (sess_iter_t *)(opaq);
    pds_flow_hw_ctx_t *hw_ctx = iter->ctx;
    sess_info_t sess;
    pds_flow_main_t *fm = &pds_flow_main;

    sess.id = iter->read_index + 1;
    sess.v4 = hw_ctx->v4;
    sess.flow_state = pds_encode_flow_state(hw_ctx->flow_state);
    sess.iflow_handle = hw_ctx->iflow.handle;
    sess.rflow_handle = hw_ctx->rflow.handle;
    sess.packet_type = pds_encode_flow_pkt_type(hw_ctx->packet_type);
    sess.iflow_rx = hw_ctx->iflow_rx;
    sess.nat = hw_ctx->nat;
    sess.drop = hw_ctx->drop;
    sess.src_vnic_id = hw_ctx->src_vnic_id;
    sess.dst_vnic_id = hw_ctx->dst_vnic_id;
    sess.ingress_bd = hw_ctx->ingress_bd;
    sess.flow_table = iter->flow_table;
    // encode
    pds_encode_one_v4_session(data, len, &sess, vlib_get_thread_index());
    clib_atomic_fetch_add(&fm->repl_stats.sync_success, 1);
    // advance to the next index
    sess_iter_advance(iter);

    return iter->ctx != NULL;
}

// NOTE: The below API's are supposed to be used only during switchover
// by vpp in the new domain
static void
pds_sess_reserve_all (void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_hw_ctx_t *ctx;
    pds_flow_prog_lock();
    for (u32 i = 0; i < fm->max_sessions; i++) {
        pool_get(fm->session_index_pool, ctx);
        pds_flow_hw_ctx_init(ctx);
        ctx->is_in_use = 0;
    }
    pds_flow_prog_unlock();
}

static void
pds_sess_free_unused (void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_hw_ctx_t *ctx;
    pds_flow_prog_lock();
    for (u32 i = 0; i < fm->max_sessions; ++i) {
        ctx = pool_elt_at_index(fm->session_index_pool, i);
        if (!ctx->is_in_use) {
            pool_put(fm->session_index_pool, ctx);
        }
    }
    pds_flow_prog_unlock();
}

always_inline void
pds_flow_age_setup_cached_sessions(u16 thread_id)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_hw_ctx_t *ctx;
    pds_flow_timer_t timer;
    u64 timeout;

    for (u32 i = 0; i < sess_info_cache_batch_get_count(thread_id); i++) {
        sess_info_t *sess = sess_info_cache_batch_get_entry_index(i, thread_id);
        ctx = pds_flow_get_session(sess->id);

        switch (ctx->flow_state) {
        case PDS_FLOW_STATE_CONN_SETUP:
            timer = PDS_FLOW_CONN_SETUP_TIMER;
            timeout = fm->tcp_con_setup_timeout;
            break;
        case PDS_FLOW_STATE_ESTABLISHED:
            timer = PDS_FLOW_IDLE_TIMER;
            timeout = fm->idle_timeout[ctx->proto];
            break;
        case PDS_FLOW_STATE_KEEPALIVE_SENT:
            ASSERT(ctx->proto == PDS_FLOW_PROTO_TCP && fm->con_track_en);
            // We have sent atleast one keep alive probe. Hence init to 1
            ctx->keep_alive_retry = 1;
            timer = PDS_FLOW_KEEP_ALIVE_TIMER;
            timeout = fm->tcp_keep_alive_timeout;
            break;
        case PDS_FLOW_STATE_HALF_CLOSE_IFLOW:
            timer = PDS_FLOW_HALF_CLOSE_TIMER;
            timeout = fm->tcp_half_close_timeout;
            break;
        case PDS_FLOW_STATE_HALF_CLOSE_RFLOW:
            timer = PDS_FLOW_HALF_CLOSE_TIMER;
            timeout = fm->tcp_half_close_timeout;
            break;
        case PDS_FLOW_STATE_CLOSE:
            timer = PDS_FLOW_CLOSE_TIMER;
            timeout = fm->tcp_keep_alive_timeout;
            break;
        default:
            continue;
        }
        FLOW_AGE_TIMER_START(&fm->timer_wheel[sess->thread_id],
                             ctx->timer_hdl, sess->id,
                             timer, timeout);
    }
}

void
pds_sess_recv_begin (void)
{
    uint16_t thread_id = vlib_get_thread_index();
    pds_sess_reserve_all();
    pds_flow_prog_lock();
    ftlv4_cache_batch_init(thread_id);
    sess_info_cache_batch_init(thread_id);
}

void
pds_sess_recv_end (void)
{
    uint16_t thread_id = vlib_get_thread_index();
    if (sess_info_cache_batch_get_count(thread_id)) {
        pds_program_cached_sessions();
        if (pds_flow_age_supported()) {
            pds_flow_age_setup_cached_sessions(thread_id);
        }
        ftlv4_cache_batch_init(thread_id);
        sess_info_cache_batch_init(thread_id);
    }
    pds_flow_prog_unlock();
    pds_sess_free_unused();
}

bool
pds_sess_v4_recv_cb (const uint8_t *data, const uint8_t len)
{
    pds_flow_main_t *fm = &pds_flow_main;
    uint16_t thread_id = vlib_get_thread_index();
    if (PREDICT_FALSE(!pds_decode_one_v4_session(data, len,
            sess_info_cache_batch_get_entry(thread_id), thread_id))) {
        clib_atomic_fetch_add(&fm->repl_stats.restore_failure_decode, 1);
        return true;
    }
    sess_info_cache_advance_count(1, thread_id);
    if (PREDICT_FALSE(sess_info_cache_full(thread_id))) {
        pds_program_cached_sessions();
        if (pds_flow_age_supported()) {
            pds_flow_age_setup_cached_sessions(thread_id);
        }
        ftlv4_cache_batch_init(thread_id);
        sess_info_cache_batch_init(thread_id);
    }
    return true;
}

void
pds_sess_cleanup_all (void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_hw_ctx_t *ctx;
    pds_flow_prog_lock();
    for (u32 i = 0; i < fm->max_sessions; ++i) {
        if (!pool_is_free_index(fm->session_index_pool, i)) {
            ctx = pool_elt_at_index(fm->session_index_pool, i);
            pool_put(fm->session_index_pool, ctx);
        }
    }
    pds_flow_prog_unlock();
}

