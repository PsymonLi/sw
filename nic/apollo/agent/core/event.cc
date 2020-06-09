//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/apollo/agent/core/event.hpp"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/trace.hpp"

extern void publish_event(const pds_event_t *event);

namespace core {

sdk_ret_t
handle_event_request (const pds_event_spec_t *spec, void *ctxt)
{
    if (spec->event_op == PDS_EVENT_OP_SUBSCRIBE) {
        core::agent_state::state()->event_mgr()->subscribe(spec->event_id,
                                                           ctxt);
    } else if (spec->event_op == PDS_EVENT_OP_UNSUBSCRIBE) {
        core::agent_state::state()->event_mgr()->subscribe(spec->event_id,
                                                           ctxt);
    } else {
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

sdk_ret_t
update_event_listener (void *ctxt)
{
    if (!core::agent_state::state()->event_mgr()->is_listener_active(ctxt)) {
        PDS_TRACE_DEBUG("Listener {} is not active, removing ...", ctxt);
        core::agent_state::state()->event_mgr()->unsubscribe_listener(ctxt);
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    return SDK_RET_OK;
}

static sdk_ret_t
grpc_svc_suspend_resume (bool suspend)
{
    bool status;
    sdk_ret_t ret;
    sdk::lib::thread *thr;
    uint32_t wait_in_ms = 10 * 1000;    // 10 seconds

    thr = sdk::lib::thread::find(PDS_AGENT_THREAD_ID_GRPC_SVC);
    if (thr) {
        ret = suspend ? thr->suspend() : thr->resume();
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Agent gRPC service suspend/resume request failed, "
                          "err %u", ret);
            return ret;
        }
        // wait for it to be suspended
        while (wait_in_ms > 0) {
            status = suspend ? thr->suspended() : !thr->suspended();
            if (status) {
                break;
            }
            usleep(1000);
            wait_in_ms--;
        }
        if (wait_in_ms == 0) {
            PDS_TRACE_ERR("Agent gRPC service suspend/resume failed due "
                          "to timeout");
            return SDK_RET_ERR;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
handle_event_ntfn (const pds_event_t *event)
{
    sdk_ret_t ret;

    PDS_TRACE_DEBUG("Rcvd event {} ntfn", event->event_id);
    switch (event->event_id) {
    case PDS_EVENT_ID_UPGRADE_START:
        ret = grpc_svc_suspend_resume(true);
        break;
    case PDS_EVENT_ID_UPGRADE_ABORT:
        ret = grpc_svc_suspend_resume(false);
        break;
    default:
        publish_event(event);
        ret = SDK_RET_OK;
        break;
    }
    return ret;
}

}    // namespace core
