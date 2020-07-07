//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/eventmgr/eventmgr.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/svc/event_svc.hpp"

Status
EventSvcImpl::EventSubscribe(ServerContext* context,
                             grpc::ServerReaderWriter<EventResponse,
                                                      EventRequest> *stream) {
    sdk_ret_t ret;
    EventRequest event_req;
    EventResponse event_rsp;
    pds_event_spec_t event_spec;

    do {
        while (stream->Read(&event_req)) {
            for (int i = 0; i < event_req.request_size(); i++) {
                PDS_TRACE_DEBUG("Rcvd event request, id {}, op {}",
                                event_req.request(i).eventid(),
                                event_req.request(i).action());
                memset(&event_spec, 0, sizeof(event_spec));
                pds_event_spec_proto_to_api_spec(&event_spec,
                                                 event_req.request(i));
                ret = core::handle_event_request(&event_spec, stream);
                if (unlikely(ret != SDK_RET_OK)) {
                    PDS_TRACE_ERR("Invalid event request in stream {}",
                                  (void *)stream);
                    event_rsp.set_status(
                        types::ApiStatus::API_STATUS_INVALID_ARG);
                    stream->Write(event_rsp);
                    core::agent_state::state()->event_mgr()->unsubscribe_listener(
                        (void *)stream);
                    return Status::CANCELLED;
                }
            }
        }

        // check if listener is still active or not and update listener state
        if (core::update_event_listener(stream) == SDK_RET_ENTRY_NOT_FOUND) {
            // listener unsubscribed to all events, bail out
            break;
        }
        pthread_yield();
    } while (TRUE);
    PDS_TRACE_DEBUG("Closing the listener stream 0x{:x}", (void *)stream);
    return Status::OK;
}

static inline bool
event_send_cb (sdk::lib::event_id_t event_id_t, void *event_ctxt, void *ctxt)
{
    EventResponse *rsp = (EventResponse *)event_ctxt;
    grpc::ServerReaderWriter<EventResponse, EventRequest> *stream =
        (grpc::ServerReaderWriter<EventResponse, EventRequest> *)ctxt;

    PDS_TRACE_VERBOSE("Sending event {} to stream {}", event_id_t, ctxt)
    if (!stream->Write(*rsp)) {
        PDS_TRACE_ERR("Failed to send event {} to stream {}",
                      event_id_t, ctxt);
        // broken stream, return false so that subscriber gets removed
        return false;
    }
    return true;
}

void
publish_event (const pds_event_t *event)
{
    EventResponse event_rsp;

    PDS_TRACE_VERBOSE("Publishing event {}", event->event_id);
    pds_event_to_proto_event_response(&event_rsp, event);
    if (core::agent_state::state()->event_mgr()) {
        core::agent_state::state()->event_mgr()->notify_event(event->event_id,
                                                              &event_rsp,
                                                              event_send_cb);
    } else {
        PDS_TRACE_WARN("Dropping the event {}", event->event_id);
    }
}
