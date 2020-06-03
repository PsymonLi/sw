//                                                                                                                                                                              
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// athena svc layer stubs
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "gen/proto/types.pb.h"

namespace core {
    sdk::lib::thread *g_agent_grpc_svc_thread;
}   // namespace core

sdk_ret_t
handle_svc_req (int fd, types::ServiceRequestMessage *proto_req, int cmd_fd)
{
    return SDK_RET_OK;
}
