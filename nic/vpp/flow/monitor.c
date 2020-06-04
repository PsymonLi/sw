//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <system.h>
#include "node.h"
#include "pdsa_hdlr.h"
#include "pdsa_uds_hdlr.h"
#include "vnic.h"
#include "session.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

void
pds_flow_monitor_init (void)
{
    pds_flow_main_t *fm = &pds_flow_main;

    fm->monitor_interval = PDS_FLOW_DEFAULT_MONITOR_INTERVAL;
}

static void
flow_monitor_export_session (pds_flow_hw_ctx_t *session, uint16_t thread_id)
{
    int flow_log_enabled = 0;

    pds_vnic_flow_log_en_get(session->src_vnic_id, &flow_log_enabled);
    if (!flow_log_enabled) {
        return;
    }

    if (session->v4) {
        ftlv4 *table4 = (ftlv4 *)pds_flow_get_table4();
        ftlv4_export_with_handle(table4, session->iflow.handle,
                                 session->rflow.handle,
                                 FLOW_EXPORT_REASON_ACTIVE,
                                 session->iflow_rx,
                                 session->drop,
                                 thread_id);
    } else {
        ftl *table = (ftl *)pds_flow_get_table6_or_l2();
        ftl_export_with_handle(table, session->iflow.handle,
                               FLOW_EXPORT_REASON_ACTIVE,
                               session->drop);
    }
}

uword
flow_monitor_process (vlib_main_t * vm,
                      vlib_node_runtime_t * rt,
                      vlib_frame_t * f)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_hw_ctx_t *session;
    uint32_t publish_time = PDS_FLOW_STATS_PUBLISH_INTERVAL;
    uint32_t monitor_time = fm->monitor_interval;
    uint32_t sleep_time;
    uint32_t ses_id;

    while(1) {
        sleep_time = MIN(publish_time, monitor_time);
        vlib_process_wait_for_event_or_clock(vm, (f64 )sleep_time);

        publish_time -= sleep_time;
        monitor_time -= sleep_time;

        if (publish_time == 0) {
            pdsa_flow_stats_publish(fm->flow_metrics_hdl,
                                    (uint64_t *)fm->stats.counter);
            publish_time = PDS_FLOW_STATS_PUBLISH_INTERVAL;
        }

        if (monitor_time == 0) {
            for (ses_id = 1; ses_id <= fm->max_sessions; ses_id++) {
                session = pds_flow_get_hw_ctx(ses_id);
                if (session == NULL) {
                    continue;
                }
                pds_flow_hw_ctx_lock(session);
                if (PREDICT_FALSE(session->monitor_seen)) {
                    flow_monitor_export_session(session, vm->thread_index);
                } else {
                    session->monitor_seen = 1;
                }
                pds_flow_hw_ctx_unlock(session);
            }
            monitor_time = fm->monitor_interval;
        }
    }
    return 0;
}


/* *INDENT-OFF* */
VLIB_REGISTER_NODE (flow_monitor_node, static) =
{
    .function = flow_monitor_process,
    .type = VLIB_NODE_TYPE_PROCESS,
    .name = "flow-monitor-process",
};
/* *INDENT-ON* */
