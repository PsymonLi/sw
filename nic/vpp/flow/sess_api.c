//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#include <vlib/vlib.h>
#include "pdsa_uds_hdlr.h"
#include "node.h"
#include <sess.h>
#include <vnic.h>

always_inline uint8_t
pds_flow_get_ctr_idx (uint8_t proto, bool isv4)
{
    switch(proto) {
    case PDS_FLOW_PROTO_TCP:
        if (isv4) {
            return FLOW_TYPE_COUNTER_TCPV4;
        } else {
            return FLOW_TYPE_COUNTER_TCPV6;
        }
    case PDS_FLOW_PROTO_UDP:
        if (isv4) {
            return FLOW_TYPE_COUNTER_UDPV4;
        } else {
            return FLOW_TYPE_COUNTER_UDPV6;
        }
    case PDS_FLOW_PROTO_ICMP:
        if (isv4) {
            return FLOW_TYPE_COUNTER_ICMPV4;
        } else {
            return FLOW_TYPE_COUNTER_ICMPV6;
        }
    default:
        if (isv4) {
            return FLOW_TYPE_COUNTER_OTHERV4;
        } else {
            return FLOW_TYPE_COUNTER_OTHERV6;
        }
    }
    // TODO: what about L2?
    return 0;
}

void
pds_flow_delete_session (u32 ses_id)
{
    pds_flow_hw_ctx_t *session = pds_flow_get_hw_ctx(ses_id);
    pds_flow_main_t *fm = &pds_flow_main;
    int flow_log_enabled = 0;
    int thread = vlib_get_thread_index();
    uint8_t ctr_idx;

    if (PREDICT_FALSE(session == NULL)) {
        return;
    }

    pds_vnic_flow_log_en_get(session->src_vnic_id, &flow_log_enabled);
    FLOW_AGE_TIMER_STOP(&fm->timer_wheel[thread], session->timer_hdl);
    // Delete both iflow and rflow
    if (session->v4) {
        ftlv4 *table4 = (ftlv4 *)pds_flow_get_table4();
        if (flow_log_enabled) {
            ftlv4_export_with_handle(table4, session->iflow.handle,
                                     session->rflow.handle,
                                     FLOW_EXPORT_REASON_DEL,
                                     session->iflow_rx,
                                     session->drop,
                                     thread);
        }
        session = pds_flow_get_hw_ctx_lock(ses_id);
        if (PREDICT_FALSE(ftlv4_get_with_handle(table4, session->iflow.handle,
                                                thread) != 0)) {
            goto end;
        }
        pds_flow_hw_ctx_unlock(session);
        if (PREDICT_FALSE(session->nat)) {
            ftlv4_update_iflow_nat_session(table4, thread);
        }
        if (PREDICT_FALSE(ftlv4_remove_cached_entry(table4, thread)) != 0) {
            return;
        }

        session = pds_flow_get_hw_ctx_lock(ses_id);
        if (PREDICT_FALSE(ftlv4_get_with_handle(table4, session->rflow.handle,
                                                thread) != 0)) {
            goto end;
        }
        pds_flow_hw_ctx_unlock(session);
        if (PREDICT_FALSE(session->nat)) {
            ftlv4_update_rflow_nat_session(table4, thread);
        }
        if (PREDICT_FALSE(ftlv4_remove_cached_entry(table4, thread)) != 0) {
            return;
        }
        if (PREDICT_FALSE(session->nat)) {
            u32 vpc_id = pds_vnic_vpc_id_get(session->src_vnic_id);
            ftlv4_remove_nat_session(vpc_id, table4, thread);
        }
    } else {
        ftl *table = (ftl *)pds_flow_get_table6_or_l2();
        if (flow_log_enabled) {
            ftl_export_with_handle(table, session->iflow.handle,
                                   FLOW_EXPORT_REASON_DEL,
                                   session->drop);
        }
        session = pds_flow_get_hw_ctx_lock(ses_id);
        if (PREDICT_FALSE(ftlv6_get_with_handle(table, session->iflow.handle) != 0)) {
            goto end;
        }
        pds_flow_hw_ctx_unlock(session);
        if (PREDICT_FALSE(ftlv6_remove_cached_entry(table)) != 0) {
            return;
        }

        session = pds_flow_get_hw_ctx_lock(ses_id);
        if (PREDICT_FALSE(ftlv6_get_with_handle(table, session->rflow.handle) != 0)) {
           goto end;
        }
        pds_flow_hw_ctx_unlock(session);
        if (PREDICT_FALSE(ftlv6_remove_cached_entry(table)) != 0) {
            return;
        }
    }

    pds_vnic_active_sessions_decrement(session->src_vnic_id);
    ctr_idx = pds_flow_get_ctr_idx(session->proto, session->v4);
    clib_atomic_fetch_add(&fm->stats.counter[ctr_idx], -1);
    pds_session_id_dealloc(ses_id);
    pds_session_stats_clear(ses_id);
    return;

end:
    pds_flow_hw_ctx_unlock(session);
    return;
}

