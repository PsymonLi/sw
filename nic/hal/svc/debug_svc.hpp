// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __DEBUG_SVC_HPP__
#define __DEBUG_SVC_HPP__

#include "nic/include/base.hpp"
#include "grpc++/grpc++.h"
#include "gen/proto/types.pb.h"
#include "gen/proto/debug.grpc.pb.h"

using grpc::ServerContext;
using grpc::Status;

using types::Empty;
using debug::Debug;
using debug::MemTrackGetRequest;
using debug::MemTrackGetRequestMsg;
using debug::MemTrackGetResponse;
using debug::MemTrackGetResponseMsg;
using debug::SlabGetRequestMsg;
using debug::SlabGetResponseMsg;
using debug::MpuTraceRequestMsg;
using debug::MpuTraceRequest;
using debug::MpuTraceResponseMsg;
using debug::MpuTraceResponse;
using debug::TraceSpec;
using debug::TraceRequestMsg;
using debug::TraceResponse;
using debug::TraceResponseMsg;
using debug::RegisterRequestMsg;
using debug::RegisterResponseMsg;
using debug::MemoryRequestMsg;
using debug::MemoryRequest;
using debug::MemoryResponseMsg;
using debug::MemoryResponse;
using debug::MemoryRawRequestMsg;
using debug::MemoryRawRequest;
using debug::MemoryRawResponseMsg;
using debug::MemoryRawResponse;
using debug::MemoryRawUpdateRequestMsg;
using debug::MemoryRawUpdateRequest;
using debug::MemoryRawUpdateResponseMsg;
using debug::MemoryRawUpdateResponse;
using debug::FlushLogsResponseMsg;
using debug::ThreadResponseMsg;
using debug::GenericOpnRequest;
using debug::GenericOpnResponse;
using debug::GenericOpnRequestMsg;
using debug::GenericOpnResponseMsg;
using debug::FteSpanRequestMsg;
using debug::FteSpanResponseMsg;
using debug::FteSpanRequest;
using debug::FteSpanResponse;
using debug::ClockResponse;
using debug::HbmBwGetRequest;
using debug::HbmBwGetRequestMsg;
using debug::HbmBwGetResponse;
using debug::HbmBwGetResponseMsg;
using debug::LlcSetupRequest;
using debug::LlcSetupRequestMsg;
using debug::LlcSetupResponse;
using debug::LlcSetupResponseMsg;
using debug::LlcGetResponse;
using debug::LlcGetResponseMsg;
using debug::HbmCacheRequest;
using debug::HbmCacheRequestMsg;
using debug::HbmCacheResponse;
using debug::HbmCacheResponseMsg;

class DebugServiceImpl final : public Debug::Service {
public:
    Status RegisterGet(ServerContext *context,
                       const RegisterRequestMsg *req,
                       RegisterResponseMsg *rsp);
    Status RegisterUpdate(ServerContext *context,
                          const RegisterRequestMsg *req,
                          RegisterResponseMsg *rsp);
    Status MemoryGet(ServerContext *context,
                     const MemoryRequestMsg *req,
                     MemoryResponseMsg *rsp);
    Status MemoryUpdate(ServerContext *context,
                        const MemoryRequestMsg *req,
                        MemoryResponseMsg *rsp);
    Status MemoryRawGet(ServerContext *context,
                            const MemoryRawRequestMsg *req_msg,
                            MemoryRawResponseMsg *rsp_msg);
    Status MemoryRawUpdate(ServerContext *context,
                               const MemoryRawUpdateRequestMsg *req_msg,
                               MemoryRawUpdateResponseMsg *rsp_msg);
    Status MemTrackGet(ServerContext *context,
                       const MemTrackGetRequestMsg *req,
                       MemTrackGetResponseMsg *rsp) override;
    Status SlabGet(ServerContext *context,
                   const SlabGetRequestMsg *req,
                   SlabGetResponseMsg *rsp) override;
    Status MpuTraceUpdate(ServerContext *context,
                          const MpuTraceRequestMsg *req,
                          MpuTraceResponseMsg *rsp) override;
    Status TraceUpdate(ServerContext *context,
                       const TraceRequestMsg *req,
                       TraceResponseMsg *rsp) override;
    Status TraceGet(ServerContext *context,
                    const Empty *req,
                    TraceResponseMsg *rsp) override;
    Status GenericOpn(ServerContext *context,
                      const GenericOpnRequestMsg *req_msg,
                      GenericOpnResponseMsg *rsp_msg);
    Status FlushLogs(ServerContext *context,
                     const Empty *req,
                     FlushLogsResponseMsg *rsp) override;
    Status ThreadGet(ServerContext *context,
                     const Empty *req,
                     ThreadResponseMsg *rsp) override;
    Status FteSpanGet(ServerContext *context,
                      const Empty *req,
                      FteSpanResponseMsg *rsp_msg) override;
    Status FteSpanUpdate(ServerContext *context,
                         const FteSpanRequestMsg *req_msg,
                         FteSpanResponseMsg *rsp_msg);
    Status ClockGet(ServerContext *context,
                    const Empty *req,
                    ClockResponse *rsp);
    Status HbmBwGet(ServerContext *context,
                    const HbmBwGetRequestMsg *req,
                    HbmBwGetResponseMsg *rsp);
    Status LlcSetup(ServerContext *context,
                    const LlcSetupRequestMsg *req,
                    LlcSetupResponseMsg *rsp);
    Status LlcGet(ServerContext *context,
                  const Empty *req,
                  LlcGetResponseMsg *rsp);
    Status LlcClear(ServerContext *context,
                    const Empty *request,
                    Empty *rsp);
};

#endif  // __DEBUG_SVC_HPP__

