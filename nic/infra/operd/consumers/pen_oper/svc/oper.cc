//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains definitions of grpc API for operational info
///
//----------------------------------------------------------------------------

#include "nic/infra/operd/consumers/pen_oper/core/state.hpp"
#include "oper.hpp"
#include "oper_svc.hpp"

static inline sdk_ret_t
handle_request (pen_oper_info_type_t info_type, uint32_t id, void *ctxt,
                bool subscribe)
{
    if (subscribe) {
        return core::pen_oper_state::state()->add_client(info_type, id, ctxt);
    }
    return core::pen_oper_state::state()->delete_client(info_type, id, ctxt);
}

static inline sdk_ret_t
handle_event_request (const operd::OperInfoSpec *proto_req,
                      void *ctxt, bool subscribe)
{
    sdk_ret_t ret;
    operd::EventFilter event_filter;
    operd::event::operd_event_type_t event_type;

    if (proto_req->has_eventfilter()) {
        // interested only in specific events
        event_filter = proto_req->eventfilter();
        for (int i = 0; i < event_filter.types_size(); i++) {
            event_type = operd_event_type_from_proto(event_filter.types(i));
            ret = handle_request(PENOPER_INFO_TYPE_EVENT, event_type,
                                ctxt, subscribe);
            if (unlikely(ret != SDK_RET_OK)) {
                return ret;
            }
        }
    } else {
        // interested in all events
        for (uint32_t event_type = operd::event::OPERD_EVENT_TYPE_MIN;
             event_type < operd::event::OPERD_EVENT_TYPE_MAX; event_type++) {
            ret = handle_request(PENOPER_INFO_TYPE_EVENT, event_type,
                                 ctxt, subscribe);
            if (unlikely(ret != SDK_RET_OK)) {
                return ret;
            }
        }
    }
    return SDK_RET_OK;
}

static inline sdk_ret_t
handle_info_request (const operd::OperInfoSpec *proto_req, void *ctxt)
{
    bool subscribe;
    pen_oper_info_type_t info_type;

    fprintf(stdout, "Received oper info request, type %u, op %u\n",
            proto_req->infotype(), proto_req->action());
    switch (proto_req->action()) {
    case operd::OPER_INFO_OP_SUBSCRIBE:
        subscribe = true;
        break;
    case operd::OPER_INFO_OP_UNSUBSCRIBE:
        subscribe = false;
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }

    info_type = pen_oper_info_type_from_proto(proto_req->infotype());
    switch (info_type) {
    case PENOPER_INFO_TYPE_EVENT:
        return handle_event_request(proto_req, ctxt, subscribe);
    default:
        break;
    }
    return SDK_RET_INVALID_ARG;
}

Status
OperSvcImpl::OperInfoSubscribe(
    ServerContext *context,
    ServerReaderWriter<OperInfoResponse, OperInfoRequest> *stream)
{
    sdk_ret_t ret;
    OperInfoRequest info_req;
    OperInfoResponse info_resp;

    do {
        while (stream->Read(&info_req)) {
            for (int i = 0; i < info_req.request_size(); i++) {
                ret = handle_info_request(&info_req.request(i), stream);
                if (unlikely(ret != SDK_RET_OK)) {
                    goto end;
                }
            }
        }

        // check if consumer is still active or not and update consumer state
        if (unlikely(!core::pen_oper_state::state()->is_client_active(stream))) {
            // consumer unsubscribed to all oper info, bail out
            break;
        }
        pthread_yield();
    } while (TRUE);
    fprintf(stdout, "Closing the consumer stream %p\n", (void *)stream);

    return Status::OK;

end:
    // invalid request - unsubscribe the client and close the stream
    fprintf(stderr, "Invalid request, closing the consumer stream %p\n",
            (void *)stream);
    core::pen_oper_state::state()->delete_client(stream);
    info_resp.set_status(types::ApiStatus::API_STATUS_INVALID_ARG);
    stream->Write(info_resp);
    return Status::CANCELLED;
}

static inline bool
oper_info_send_cb (sdk::lib::event_id_t info_type, void *info_ctxt, void *ctxt)
{
    OperInfoResponse *resp = (OperInfoResponse *)info_ctxt;
    grpc::ServerReaderWriter<OperInfoResponse, OperInfoRequest> *stream =
        (grpc::ServerReaderWriter<OperInfoResponse, OperInfoRequest> *)ctxt;

    if (unlikely(!stream->Write(*resp))) {
        // client closed connection, return false so that consumer gets removed
        fprintf(stderr, "Failed to write oper info %u - "
                "client closed connection\n", info_type);
        return false;
    }
    return true;
}

void
publish_oper_info (pen_oper_info_type_t info_type,
                   const char *info, size_t info_length)
{
    sdk_ret_t ret;
    uint32_t info_id;
    OperInfoResponse oper_info_rsp;

    ret = pen_oper_info_to_proto_info_response(info_type, info,
                                               info_length, &oper_info_rsp);
    if (unlikely(ret != SDK_RET_OK)) {
        fprintf(stderr, "Failed to convert, dropping the info of type %u\n",
                info_type);
        return;
    }

    info_id = operd_info_id_from_proto(&oper_info_rsp);
    ret = core::pen_oper_state::state()->client_mgr(info_type)->notify_event(
        info_id, &oper_info_rsp, oper_info_send_cb);
    if (unlikely(ret != SDK_RET_OK)) {
        fprintf(stderr, "Failed to notify, dropping the info of type %u\n",
                info_type);
        return;
    }
}
