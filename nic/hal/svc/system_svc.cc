//------------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//
// sys service implementation
//------------------------------------------------------------------------------

#include "nic/include/base.hpp"
#include "nic/hal/hal_trace.hpp"
#include "nic/include/hal.hpp"
#include "nic/include/hal_cfg.hpp"
#include "nic/include/hal_cfg_db.hpp"
#include "nic/hal/svc/system_svc.hpp"
#include "gen/proto/system.pb.h"
#include "gen/proto/types.pb.h"
#include "nic/hal/src/internal/system.hpp"

Status
SystemServiceImpl::ApiStatsGet(ServerContext *context,
                               const Empty *request,
                               ApiStatsResponse *rsp)
{
    HAL_TRACE_DEBUG("Rcvd API Stats Get Request");
    hal::api_stats_get(rsp);
    return Status::OK;
}

Status
SystemServiceImpl::SystemGet(ServerContext *context,
                             const SystemGetRequest *request,
                             SystemResponse *rsp)
{
    HAL_TRACE_DEBUG("Rcvd System Get Request");
    //Revisit to see if we need this lock for system get
    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    hal::system_get(request, rsp);
    hal::hal_cfg_db_close();

    return Status::OK;
}

Status
SystemServiceImpl::SystemUuidGet(ServerContext *context,
                                 const Empty *request,
                                 SystemResponse *rsp)
{
    HAL_TRACE_DEBUG("Rcvd System UUID Get Request");
    hal::system_uuid_get(rsp);
    return Status::OK;
}


Status
SystemServiceImpl::ClearIngressDropStats(ServerContext *context,
                                         const Empty *request,
                                         Empty *rsp) 
{
    HAL_TRACE_DEBUG("Rcvd Clear Ingress Drop Stats");

    hal::clear_ingress_drop_stats();

    return Status::OK;
}

Status
SystemServiceImpl::ClearEgressDropStats(ServerContext *context,
                                        const Empty *request,
                                        Empty *rsp)
{
    HAL_TRACE_DEBUG("Rcvd Clear Egress Drop Stats");

    hal::clear_egress_drop_stats();

    return Status::OK;
}

Status
SystemServiceImpl::ClearPbDropStats(ServerContext *context,
                                    const Empty *request,
                                    Empty *rsp)
{
    HAL_TRACE_DEBUG("Rcvd Clear Pb Drop Stats");

    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    hal::hal_cfg_db_close();

    return Status::OK;
}

Status
SystemServiceImpl::ClearFteStats(ServerContext *context,
                                 const Empty *request,
                                 Empty *rsp)
{
    HAL_TRACE_DEBUG("Rcvd Clear FTE Stats");

    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    hal::hal_cfg_db_close();

    return Status::OK;
}

Status
SystemServiceImpl::ClearFteTxRxStats(ServerContext *context,
                                     const Empty *request,
                                     Empty *rsp)
{
    HAL_TRACE_DEBUG("Rcvd Clear FTE Tx-Rx Stats");

    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    hal::hal_cfg_db_close();

    return Status::OK;
}

Status
SystemServiceImpl::ClearTableStats(ServerContext *context,
                                   const Empty *request,
                                   Empty *rsp)
{
    HAL_TRACE_DEBUG("Rcvd Clear Table Stats");

    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    hal::hal_cfg_db_close();

    return Status::OK;
}

Status
SystemServiceImpl::ClearPbStats(ServerContext *context,
                                const Empty *request,
                                Empty *rsp)
{
    HAL_TRACE_DEBUG("Rcvd Clear Pb Stats");

    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    hal::clear_pb_stats();
    hal::hal_cfg_db_close();

    return Status::OK;
}

Status
SystemServiceImpl::ForwardingModeGet(ServerContext *context,
                                     const Empty *request,
                                     ForwardingModeResponse *rsp)
{
    HAL_TRACE_DEBUG("Rcvd Get Forwarding Mode");

    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    hal::forwarding_mode_get(rsp);
    hal::hal_cfg_db_close();

    return Status::OK;
}

Status
SystemServiceImpl::FeatureProfileGet(ServerContext *context,
                                     const Empty *request,
                                     FeatureProfileResponse *rsp)
{
    HAL_TRACE_DEBUG("Rcvd Get Feature Profile");

    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    hal::feature_profile_get(rsp);
    hal::hal_cfg_db_close();

    return Status::OK;
}
