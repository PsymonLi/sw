//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_FLOW_NODE_H__
#define __VPP_FLOW_NODE_H__

#include <vlib/vlib.h>
#undef TCP_FLAG_CWR
#undef TCP_FLAG_ECE
#undef TCP_FLAG_URG
#undef TCP_FLAG_ACK
#undef TCP_FLAG_PSH
#undef TCP_FLAG_RST
#undef TCP_FLAG_SYN
#undef TCP_FLAG_FIN
#include <vnet/ip/ip.h>
#include <vppinfra/tw_timer_16t_1w_2048sl.h>
#include <vnet/udp/udp_packet.h>
#include <nic/p4/common/defines.h>
#include <ftl_wrapper.h>
#include "pdsa_hdlr.h"

#define MAX_FLOWS_PER_FRAME                     (VLIB_FRAME_SIZE * 2)
#define PDS_FLOW_SESSION_POOL_COUNT_MAX         VLIB_FRAME_SIZE
// hold max supported rx queue size in deltion pool, change this value if
// we support more buffers per RX queue
#define PDS_FLOW_DEL_SESSION_POOL_COUNT_MAX     32768
// Maximum expired timers to be processed per invocation
#define PDS_FLOW_MAX_TIMER_EXPIRATIONS          VLIB_FRAME_SIZE

#define DISPLAY_BUF_SIZE                        (1*1024*1024)
#define PDS_FLOW_TIMER_TICK                     0.1
#define PDS_FLOW_SEC_TO_TIMER_TICK(X)           (X * 10)
#define PDS_FLOW_STATS_PUBLISH_INTERVAL         (60)
#define PDS_FLOW_DEFAULT_MONITOR_INTERVAL       (300)
#define TCP_KEEP_ALIVE_RETRY_COUNT_MAX          3
#define TCP_KEEP_ALIVE_TIMEOUT                  1 // in seconds

#define foreach_flow_classify_next                                  \
        _(IP4_FLOW_PROG, "pds-ip4-flow-program" )                   \
        _(IP6_FLOW_PROG, "pds-ip6-flow-program" )                   \
        _(L2_FLOW_PROG, "pds-l2-flow-program" )                     \
        _(IP4_TUN_FLOW_PROG, "pds-tunnel-ip4-flow-program" )        \
        _(IP6_TUN_FLOW_PROG, "pds-tunnel-ip6-flow-program" )        \
        _(IP4_L2L_FLOW_PROG, "pds-l2l-rx-ip4-flow-program" )        \
        _(IP4_L2L_DEF_FLOW_PROG, "pds-l2l-ip4-def-flow-program" )   \
        _(AGE_FLOW, "pds-flow-age-setup" )                          \
        _(IP4_NAT, "pds-nat44" )                                    \
        _(IP4_NAT_INVAL, "pds-nat44-inval" )                        \
        _(ICMP_VRIP, "ip4-icmp-echo-request")                       \
        _(DROP, "pds-error-drop")                                   \

#define foreach_flow_classify_counter                               \
        _(IP4_FLOW, "IPv4 flow packets" )                           \
        _(IP6_FLOW, "IPv6 flow packets" )                           \
        _(L2_FLOW, "L2 flow packets" )                              \
        _(IP4_TUN_FLOW, "IPv4 tunnel flow packets" )                \
        _(IP6_TUN_FLOW, "IPv6 tunnel flow packets" )                \
        _(IP4_L2L_FLOW, "IPv4 l2l flow packets" )                   \
        _(IP4_NAT, "NAPT flow packets" )                            \
        _(IP4_NAT_INVAL, "NAT Invalid packets" )                    \
        _(MAX_EXCEEDED, "Session count exceeded packets")           \
        _(VNIC_NOT_FOUND, "Unknown vnic")                           \
        _(VPC_NOT_FOUND, "Unknown vpc")                             \
        _(ICMP_VRIP, "VR IPv4 request packets received")            \
        _(UNKOWN, "Unknown flow packets")                           \
        _(VRIP_DROP, "Unknown VR IPv4 packets")                     \
        _(TCP_PKT, "TCP packets")                                   \
        _(TCP_PKT_NO_SES, "TCP packets with invalid session id")    \
        _(SES_NOT_FOUND, "Packets with invalid session id")         \
        _(DEFUNCT_RFLOW, "Defunct rflow")                           \
        _(L2L_NDEF_IFLOW, "Non-defunct L2l iflow")                  \
        _(VRIP_MAC_MISMATCH, "VR IPv4 packet with invalid MAC")     \

#define foreach_flow_prog_next                                      \
        _(FWD_FLOW, "pds-fwd-flow" )                                \
        _(SESSION_PROG, "pds-session-program")                      \
        _(NAT_DROP, "pds-nat44-error")                              \
        _(DROP, "pds-error-drop")                                   \

#define foreach_flow_prog_counter                                   \
        _(FLOW_SUCCESS, "Flow programming success" )                \
        _(FLOW_FAILED, "Flow programming failed")                   \
        _(FLOW_DELETE_FAILED, "Flow delete failed")                 \
        _(SESSION_ID_ALLOC_FAILED, "Session ID alloc failed")       \
        _(SESSION_ID_INVALID, "Invalid Session ID")                 \
        _(L2L_RFLOW, "L2L responder flow")                          \
        _(L2L_DUP_RFLOW, "L2L duplicate responder flow")            \

#define foreach_fwd_flow_next                                       \
        _(INTF_OUT, "interface-tx" )                                \
        _(DROP, "pds-error-drop")                                   \

#define foreach_fwd_flow_counter                                    \
        _(REWRITE_SUCCESS, "Rewrite success" )                      \
        _(REWRITE_FAILED, "Rewrite failed" )                        \

#define foreach_session_prog_next                                   \
        _(FWD_FLOW, "pds-fwd-flow" )                                \
        _(AGE_FLOW, "pds-flow-age-setup" )                          \
        _(NAT_DROP, "pds-nat44-error")                              \
        _(DROP, "pds-error-drop")                                   \

#define foreach_session_prog_counter                                \
        _(SESSION_SUCCESS, "Session programming success" )          \
        _(SESSION_FAILED, "Session programming failed")             \

#define foreach_flow_age_setup_next                                 \
        _(FWD_FLOW, "pds-fwd-flow" )                                \
        _(DROP, "pds-error-drop")                                   \

#define foreach_flow_age_setup_counter                              \
        _(SYN, "SYN packet processed" )                             \
        _(FIN, "FIN packet processed")                              \
        _(RST, "RST packet processed" )                             \
        _(OTHER, "Other packet processed")                          \
        _(DROP, "Drop packet processed")                            \
        _(SESSION_INVALID, "Session Invalid")                       \

#define foreach_flow_timer                                          \
        _(CONN_SETUP_TIMER, "Connection Setup timer expired")       \
        _(IDLE_TIMER, "Idle timer expired")                         \
        _(KEEP_ALIVE_TIMER, "Keepalive timer expired")              \
        _(HALF_CLOSE_TIMER, "Half close timer expired")             \
        _(CLOSE_TIMER, "Close timer expired")                       \
        _(DROP_TIMER, "Drop timer expired")                         \

typedef enum
{
#define _(s,n) FLOW_CLASSIFY_NEXT_##s,
    foreach_flow_classify_next
#undef _
    FLOW_CLASSIFY_N_NEXT,
} flow_classify_next_t;

typedef enum
{
#define _(n,s) FLOW_CLASSIFY_COUNTER_##n,
    foreach_flow_classify_counter
#undef _
    FLOW_CLASSIFY_COUNTER_LAST,
} flow_classify_counter_t;

typedef enum
{
#define _(s,n) FLOW_PROG_NEXT_##s,
    foreach_flow_prog_next
#undef _
    FLOW_PROG_N_NEXT,
} flow_prog_next_t;

typedef enum
{
#define _(n,s) FLOW_PROG_COUNTER_##n,
    foreach_flow_prog_counter
#undef _
    FLOW_PROG_COUNTER_LAST,
} flow_prog_counter_t;

typedef enum
{
#define _(s,n) FWD_FLOW_NEXT_##s,
    foreach_fwd_flow_next
#undef _
    FWD_FLOW_N_NEXT,
} fwd_flow_next_t;

typedef enum
{
#define _(n,s) FWD_FLOW_COUNTER_##n,
    foreach_fwd_flow_counter
#undef _
    FWD_FLOW_COUNTER_LAST,
} fwd_flow_counter_t;

typedef enum
{
#define _(s,n) SESSION_PROG_NEXT_##s,
    foreach_session_prog_next
#undef _
    SESSION_PROG_N_NEXT,
} flow_session_next_t;

typedef enum
{
#define _(n,s) SESSION_PROG_COUNTER_##n,
    foreach_session_prog_counter
#undef _
    SESSION_PROG_COUNTER_LAST,
} flow_session_counter_t;

typedef enum
{
#define _(n,s) FLOW_TYPE_COUNTER_##n,
    foreach_flow_type_counter
#undef _
    FLOW_TYPE_COUNTER_LAST,
} flow_type_counter_t;

typedef enum
{
#define _(n,s) PDS_FLOW_##n,
    foreach_flow_timer
#undef _
    PDS_FLOW_TIMER_LAST,
} pds_flow_timer_t;

typedef struct fwd_flow_trace_s {
    u32 hw_index;
} fwd_flow_trace_t;

typedef struct flow_prog_trace_s {
    ip46_address_t isrc, idst, rsrc, rdst;
    u16 isrc_port, idst_port, rsrc_port, rdst_port;
    u8 iprotocol, rprotocol;
} flow_prog_trace_t;

typedef struct flow_classify_trace_s {
    u32 l2_offset;
    u32 l3_offset;
    u32 l4_offset;
    u32 vnic;
    u32 flow_hash;
    u32 flags;
} flow_classify_trace_t;

typedef struct session_prog_trace_s {
    u32 session_id;
} session_prog_trace_t;

typedef enum
{
#define _(s,n) FLOW_AGE_SETUP_NEXT_##s,
    foreach_flow_age_setup_next
#undef _
    FLOW_AGE_SETUP_N_NEXT,
} flow_age_setup_next_t;

typedef enum
{
#define _(n,s) FLOW_AGE_SETUP_COUNTER_##n,
    foreach_flow_age_setup_counter
#undef _
    FLOW_AGE_SETUP_COUNTER_LAST,
} flow_age_setup_counter_t;

typedef CLIB_PACKED(union pds_flow_index_s_ {
    u64 handle;
}) pds_flow_index_t;

// Store iflow and rflow index for each allocated session
typedef CLIB_PACKED(struct pds_flow_hw_ctx_s {
    u32 is_in_use : 1;
    u32 proto : 2; // enum pds_flow_protocol
    u32 v4 : 1; // v4 or v6 table
    u32 flow_state : 3; // enum pds_flow_state
    u32 keep_alive_retry : 2;
    // if number of bits change then fix macro FLOW_AGE_TIMER_STOP()
    u32 timer_hdl : 23;
    u16 ingress_bd;
    pds_flow_index_t iflow;
    pds_flow_index_t rflow;
    // lock per session entry: since iflow/rflow index may get updated from
    // other threads, we need lock here.
    volatile u8 lock;
    u8 packet_type : 5; // pds_flow_pkt_type
    u8 iflow_rx : 1; // true if iflow is towards the host
    u8 monitor_seen : 1; // 1 if monitor process has seen flow
    u8 nat : 1;

    u8 drop : 1;
    u8 src_vnic_id : 7;

    u8 dst_vnic_id;

    u16 thread_id : 2;
    u16 reserved_1 : 6;
}) pds_flow_hw_ctx_t;

typedef struct pds_flow_session_id_thr_local_pool_s {
    i16     session_count;
    u32     session_ids[PDS_FLOW_SESSION_POOL_COUNT_MAX];
    i32     del_session_count;
    u32     del_session_ids[PDS_FLOW_DEL_SESSION_POOL_COUNT_MAX];
} pds_flow_session_id_thr_local_pool_t;

typedef struct pds_flow_rewrite_flags_s {
    u16 tx_rewrite;
    u16 rx_rewrite;
} pds_flow_rewrite_flags_t;

typedef struct pds_flow_stats_s {
    volatile u64 counter[FLOW_TYPE_COUNTER_LAST];
} pds_flow_stats_t;

typedef struct pds_flow_repl_stats_s {
    volatile u64 sync_success;
    volatile u64 restore_success;
    volatile u64 restore_failure_decode;
    volatile u64 restore_failure_unknown_flow_state;
    volatile u64 restore_failure_unknown_flow_type;
    volatile u64 restore_failure_unknown_src_vnic;
    volatile u64 restore_failure_unknown_dst_vnic;
    volatile u64 restore_failure_vnic_limit_exceeded;
    volatile u64 restore_failure_flow_insert;
} pds_flow_repl_stats_t;

typedef CLIB_PACKED (struct
                     {
                        ip4_header_t ip4_hdr;
                        tcp_header_t tcp_hdr;
                     }) ip4_and_tcp_header_t;

typedef struct pds_flow_fixup_data_s {
    bool valid;
    u32 ses_id;
    bool iflow;
    u16 bd_id;
    u16 vnic_id;
} pds_flow_fixup_data_t;

typedef struct pds_flow_main_s {
    volatile u32 *flow_prog_lock;
    ftlv4 *table4;
    ftlv6 *table6_or_l2;
    pds_flow_hw_ctx_t *session_index_pool;
    pds_flow_session_id_thr_local_pool_t *session_id_thr_local_pool;
    pds_flow_rewrite_flags_t *rewrite_flags;
    char *stats_buf;
    u8 *rx_vxlan_template;
    u64 tcp_con_setup_timeout;
    u64 tcp_half_close_timeout;
    u64 tcp_close_timeout;
    u64 tcp_keep_alive_timeout;
    u64 *idle_timeout;
    u64 *drop_timeout;
    u64 *idle_timeout_ticks;
    // per worker worker timer wheel
    tw_timer_wheel_16t_1w_2048sl_t *timer_wheel;
    u32 max_sessions;
    u32 monitor_interval;
    u8 no_threads;
    u8 con_track_en;
    pds_flow_stats_t stats;
    void *flow_metrics_hdl;
    u16 drop_nexthop;
    // packet template to send TCP keep alives
    vlib_packet_template_t tcp_keepalive_packet_template;
    u32 *delete_sessions;
    u32 ses_id_to_del;
    pds_flow_fixup_data_t fixup_data;
    u64 **ses_time;
    pds_flow_repl_stats_t repl_stats;
} pds_flow_main_t;

extern pds_flow_main_t pds_flow_main;

always_inline bool pds_flow_packet_l2l(u8 packet_type)
{
    return (packet_type <= PDS_FLOW_L2L_INTER_SUBNET) ? true : false;
}

always_inline void pds_flow_prog_lock(void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    if (fm->no_threads <= 2) {
        return;
    }

    while (clib_atomic_test_and_set(fm->flow_prog_lock));
}

always_inline void pds_flow_prog_unlock(void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    if (fm->no_threads <= 2) {
        return;
    }

    clib_atomic_release(fm->flow_prog_lock);
}

always_inline void * pds_flow_prog_get_table4(void)
{
    pds_flow_main_t *fm = &pds_flow_main;

    return fm->table4;
}

always_inline void * pds_flow_prog_get_table6_or_l2(void)
{
    pds_flow_main_t *fm = &pds_flow_main;

    return fm->table6_or_l2;
}

always_inline void pds_flow_hw_ctx_init (pds_flow_hw_ctx_t *ses)
{
    clib_memset(ses, 0, sizeof(*ses));
    ses->timer_hdl = ~0;
    return;
}

always_inline void pds_session_id_alloc2(u32 *ses_id0, u32 *ses_id1)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_session_id_thr_local_pool_t    *thr_local_pool =
                     &fm->session_id_thr_local_pool[vlib_get_thread_index()];
    uint16_t        refill_count;
    pds_flow_hw_ctx_t *ctx;

    if (PREDICT_TRUE(thr_local_pool->session_count >= 1)) {
        *ses_id1 = thr_local_pool->session_ids[thr_local_pool->session_count - 1];
        *ses_id0 = thr_local_pool->session_ids[thr_local_pool->session_count];
        ctx = pool_elt_at_index(fm->session_index_pool, (*ses_id0 - 1));
        ctx->is_in_use = 1;
        ctx = pool_elt_at_index(fm->session_index_pool, (*ses_id1 - 1));
        ctx->is_in_use = 1;
        thr_local_pool->session_count -= 2;
        return;
    }
    /* refill pool */
    refill_count = PDS_FLOW_SESSION_POOL_COUNT_MAX;
    if (thr_local_pool->session_count == 0) {
        refill_count--;;
    }
    pds_flow_prog_lock();
    while (refill_count) {
        if (PREDICT_FALSE(fm->max_sessions <= pool_elts(fm->session_index_pool))) {
            break;
        }
        thr_local_pool->session_count++;
        pool_get(fm->session_index_pool, ctx);
        pds_flow_hw_ctx_init(ctx);
        thr_local_pool->session_ids[thr_local_pool->session_count] =
             ctx - fm->session_index_pool + 1;
        refill_count--;
    }
    pds_flow_prog_unlock();
    if (PREDICT_TRUE(thr_local_pool->session_count >= 1)) {
        *ses_id1 = thr_local_pool->session_ids[thr_local_pool->session_count - 1];
        *ses_id0 = thr_local_pool->session_ids[thr_local_pool->session_count];
        ctx = pool_elt_at_index(fm->session_index_pool, (*ses_id0 - 1));
        ctx->is_in_use = 1;
        ctx = pool_elt_at_index(fm->session_index_pool, (*ses_id1 - 1));
        ctx->is_in_use = 1;
        thr_local_pool->session_count -= 2;
        return;
    }
    return;
}

always_inline u32 pds_session_id_alloc(void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_session_id_thr_local_pool_t    *thr_local_pool =
       &fm->session_id_thr_local_pool[vlib_get_thread_index()];
    uint16_t        refill_count;
    pds_flow_hw_ctx_t *ctx;
    uint32_t        ret = 0;

    if (PREDICT_TRUE(thr_local_pool->session_count >= 0)) {
        ret = thr_local_pool->session_ids[thr_local_pool->session_count];
        ctx = pool_elt_at_index(fm->session_index_pool, (ret - 1));
        ctx->is_in_use = 1;
        thr_local_pool->session_count--;
        return ret;
    }
    /* refill pool */
    refill_count = PDS_FLOW_SESSION_POOL_COUNT_MAX;
    pds_flow_prog_lock();
    while (refill_count) {
        if (PREDICT_FALSE(fm->max_sessions <= pool_elts(fm->session_index_pool))) {
            break;
        }
        thr_local_pool->session_count++;
        pool_get(fm->session_index_pool, ctx);
        pds_flow_hw_ctx_init(ctx);
        thr_local_pool->session_ids[thr_local_pool->session_count] =
            ctx - fm->session_index_pool + 1;
        refill_count--;
    }
    pds_flow_prog_unlock();
    if (PREDICT_TRUE(thr_local_pool->session_count >= 0)) {
        ret = thr_local_pool->session_ids[thr_local_pool->session_count];
        ctx = pool_elt_at_index(fm->session_index_pool, (ret - 1));
        ctx->is_in_use = 1;
        thr_local_pool->session_count--;
        return ret;
    }
    return 0; //0 means invalid
}

always_inline void pds_session_id_dealloc(u32 ses_id)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_session_id_thr_local_pool_t *thr_local_pool =
           &fm->session_id_thr_local_pool[vlib_get_thread_index()];
    pds_flow_hw_ctx_t *ctx;

    // this means we can't buffer sessions anymore. free existing sessions
    if (PREDICT_FALSE(thr_local_pool->del_session_count >=
                      PDS_FLOW_DEL_SESSION_POOL_COUNT_MAX)) {
        pds_flow_prog_lock();
        for (int i = 0; i < PDS_FLOW_DEL_SESSION_POOL_COUNT_MAX; i++) {
            pool_put_index(fm->session_index_pool,
                           (thr_local_pool->del_session_ids[i] - 1));
        }
        thr_local_pool->del_session_count = 0;
        pds_flow_prog_unlock();
    }
    ctx = pool_elt_at_index(fm->session_index_pool, (ses_id - 1));
    ctx->is_in_use = 0;
    thr_local_pool->del_session_ids[thr_local_pool->del_session_count++] =
            ses_id;
    return;
}

always_inline pds_flow_hw_ctx_t *
pds_flow_get_hw_ctx_no_check (u32 index)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_hw_ctx_t *ctx;

    index -= 1;
    ctx = pool_elt_at_index(fm->session_index_pool, index);
    return ctx;
}

always_inline pds_flow_hw_ctx_t * pds_flow_get_hw_ctx (u32 ses_id)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_hw_ctx_t *ctx;

    if (PREDICT_FALSE((ses_id == 0) ||
        pool_is_free_index(fm->session_index_pool, (ses_id - 1)))) {
        return NULL;
    }
    ctx = pool_elt_at_index(fm->session_index_pool, (ses_id - 1));
    if (PREDICT_FALSE(ctx->is_in_use == 0)) {
        return NULL;
    }
    return ctx;
}

always_inline bool pds_flow_ses_id_valid (u32 ses_id)
{
    if (PREDICT_FALSE(NULL == pds_flow_get_hw_ctx(ses_id))) {
        return false;
    }
    return true;
}

always_inline pds_flow_hw_ctx_t * pds_flow_get_hw_ctx_lock (u32 ses_id)
{
    pds_flow_hw_ctx_t *ctx;

    ctx = pds_flow_get_hw_ctx(ses_id);
    if (PREDICT_FALSE(NULL == ctx)) {
        return NULL;
    }
    while (clib_atomic_test_and_set(&ctx->lock));
    return ctx;
}

always_inline void pds_flow_hw_ctx_lock (pds_flow_hw_ctx_t *ctx)
{
    while (clib_atomic_test_and_set(&ctx->lock));
}

always_inline void pds_flow_hw_ctx_unlock (pds_flow_hw_ctx_t *ctx)
{
    clib_atomic_release(&ctx->lock);
}

always_inline pds_flow_protocol pds_flow_trans_proto(u8 proto)
{
    if (PREDICT_TRUE(proto == IP_PROTOCOL_TCP)) {
        return PDS_FLOW_PROTO_TCP;
    }
    if (proto == IP_PROTOCOL_UDP) {
        return PDS_FLOW_PROTO_UDP;
    }
    if (proto == IP_PROTOCOL_ICMP) {
        return PDS_FLOW_PROTO_ICMP;
    }
    return PDS_FLOW_PROTO_OTHER;
}

always_inline void pds_session_set_data(u32 ses_id, u64 i_handle, u64 r_handle,
                                        pds_flow_protocol proto,
                                        uint8_t vnic_id, bool v4,
                                        bool host_origin, u8 packet_type,
                                        bool drop, u8 thread_id)
{
    pds_flow_main_t *fm = &pds_flow_main;

    //pds_flow_prog_lock();
    pds_flow_hw_ctx_t *data = pool_elt_at_index(fm->session_index_pool,
                                                (ses_id - 1));
    data->iflow.handle = i_handle;
    data->rflow.handle = r_handle;
    data->proto = proto;
    data->v4 = v4;
    data->is_in_use = 1;
    data->src_vnic_id = vnic_id;
    data->iflow_rx = host_origin;
    data->packet_type = packet_type;
    if (PREDICT_FALSE(data->packet_type == PDS_FLOW_L2N_ASYMMETRIC_ROUTE_NAT ||
        data->packet_type == PDS_FLOW_L2N_SYMMETRIC_ROUTE_NAPT)) {
        data->nat = 1;
    } else {
        data->nat = 0;
    }
    data->drop = drop;
    data->thread_id = thread_id;
    //pds_flow_prog_unlock();
    return;
}

extern void pds_flow_expired_timers_dispatch(u32 * expired_timers);
extern void pds_flow_delete_session(u32 session_id);

always_inline void pds_session_id_flush(void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    tw_timer_wheel_16t_1w_2048sl_t *tw;

    foreach_vlib_main (({
        if (ii == 0) {
            continue;
        }
        tw = vec_elt_at_index(fm->timer_wheel, ii);
        // free timer wheel to stop all timers
        tw_timer_wheel_free_16t_1w_2048sl(tw);
        tw_timer_wheel_init_16t_1w_2048sl(tw, pds_flow_expired_timers_dispatch,
                                          PDS_FLOW_TIMER_TICK, ~0);
        tw->last_run_time = vlib_time_now(this_vlib_main);
    }));
    pds_flow_prog_lock();
    pool_free(fm->session_index_pool);
    fm->session_index_pool = NULL;
    pool_init_fixed(fm->session_index_pool, fm->max_sessions);
    for (u32 i = 0; i < vec_len(fm->session_id_thr_local_pool); i++) {
        fm->session_id_thr_local_pool[i].session_count = -1;
        fm->session_id_thr_local_pool[i].del_session_count = 0;
    }
    pds_flow_prog_unlock();
    return;
}

// timer_hdl is 23 bits, so check against 0x7fffff
#define FLOW_AGE_TIMER_STOP(_tw, _hdl)                  \
{                                                       \
    if (_hdl != 0x7fffff) {                             \
        tw_timer_stop_16t_1w_2048sl(_tw, _hdl);         \
        _hdl = ~0;                                      \
    }                                                   \
}                                                       \

void pds_session_update_data(u32 ses_id, u64 new_handle,
                             bool iflow, bool move_complete, bool lock);

#endif    // __VPP_FLOW_NODE_H__
