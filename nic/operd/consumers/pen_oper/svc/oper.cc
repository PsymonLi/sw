//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains definitions of protobuf API for operational info
///
//----------------------------------------------------------------------------

#include "nic/operd/consumers/pen_oper/core/state.hpp"
#include "oper.hpp"
#include "oper_svc.hpp"

static inline sdk_ret_t
handle_info_request (const operd::OperInfoSpec *proto_req, void *ctxt)
{
    pen_oper_info_type_t info_type;
    operd::OperInfoOp action;

    fprintf(stdout, "Received oper info request, type %u, op %u\n",
            proto_req->infotype(), proto_req->action());
    info_type = pen_oper_info_type_from_proto(proto_req->infotype());
    if (info_type == PENOPER_INFO_TYPE_NONE) {
        return SDK_RET_INVALID_ARG;
    }

    action = proto_req->action();
    if (action == operd::OPER_INFO_OP_SUBSCRIBE) {
        core::pen_oper_state::state()->event_mgr()->subscribe(info_type, ctxt);
    } else if (action == operd::OPER_INFO_OP_UNSUBSCRIBE) {
        core::pen_oper_state::state()->event_mgr()->unsubscribe(info_type,
                                                                ctxt);
    } else {
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

static inline sdk_ret_t
is_consumer_active (void *ctxt)
{
    if (!core::pen_oper_state::state()->event_mgr()->is_listener_active(ctxt)) {
        fprintf(stdout, "Removing inactive consumer %p\n", ctxt);
        core::pen_oper_state::state()->event_mgr()->unsubscribe_listener(ctxt);
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    return SDK_RET_OK;
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
                if (ret != SDK_RET_OK) {
                    goto end;
                }
            }
        }

        // check if consumer is still active or not and update consumer state
        if (is_consumer_active(stream) == SDK_RET_ENTRY_NOT_FOUND) {
            // consumer unsubscribed to all oper info, bail out
            break;
        }
        pthread_yield();
    } while (TRUE);
    fprintf(stdout, "Closing the consumer stream %p\n", (void *)stream);

    return Status::OK;

end:
    // invalid request - unsubscribe and close the stream
    core::pen_oper_state::state()->event_mgr()->unsubscribe_listener((void *)stream);
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

    if (!stream->Write(*resp)) {
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
    OperInfoResponse oper_info_rsp;

    ret = pen_oper_info_to_proto_info_response(info_type, info,
                                               info_length, &oper_info_rsp);
    if (ret != SDK_RET_OK) {
        fprintf(stderr, "Failed to convert, dropping the info of type %u\n",
                info_type);
        return;
    }

    ret = core::pen_oper_state::state()->event_mgr()->notify_event(info_type,
              &oper_info_rsp, oper_info_send_cb);
    if (ret != SDK_RET_OK) {
        fprintf(stderr, "Failed to notify, dropping the info of type %u\n",
                info_type);
        return;
    }
}
