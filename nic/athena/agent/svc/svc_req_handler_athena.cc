//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// service request message handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/interface_svc.hpp"
#include "nic/apollo/agent/svc/debug_svc.hpp"
#include "nic/apollo/api/include/pds_debug.hpp"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/apollo/agent/trace.hpp"

static sdk_ret_t
pds_handle_cmd (cmd_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    types::ServiceResponseMessage proto_rsp;

    PDS_TRACE_ERR("Go into svc_req_handler.cc");

    ret = debug::pds_handle_cmd(ctxt);

    proto_rsp.set_apistatus(sdk_ret_to_api_status(ret));
    if (!proto_rsp.SerializeToFileDescriptor(ctxt->sock_fd)) {
        PDS_TRACE_ERR("Serializing command message {} response to socket failed", ctxt->cmd);
    }

    return ret;
}

sdk_ret_t
handle_svc_req (int fd, types::ServiceRequestMessage *proto_req, int cmd_fd)
{
    sdk_ret_t ret;
    svc_req_ctxt_t svc_req;

    memset(&svc_req, 0, sizeof(svc_req_ctxt_t));
    // convert proto svc request to svc req ctxt
    pds_svc_req_proto_to_svc_req_ctxt(&svc_req, proto_req, fd, cmd_fd);

    switch (svc_req.type) {
    case SVC_REQ_TYPE_CFG:
        break;
    case SVC_REQ_TYPE_CMD:
        ret = pds_handle_cmd(&svc_req.cmd_ctxt);
        break;
    default:
        ret = SDK_RET_INVALID_ARG;
    }
    return ret;
}
