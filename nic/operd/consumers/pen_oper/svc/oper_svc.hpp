//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains protobuf utility functions for operational info
///
//----------------------------------------------------------------------------

#ifndef __PEN_OPER_SVC_OPER_SVC_HPP__
#define __PEN_OPER_SVC_OPER_SVC_HPP__

#include "gen/proto/oper.grpc.pb.h"

static inline pen_oper_info_type_t
pen_oper_info_type_from_proto (operd::OperInfoType info_type)
{
    switch (info_type) {
    case operd::OPER_INFO_TYPE_ALERT:
        return PENOPER_INFO_TYPE_ALERT;
    default:
        break;
    }
    return PENOPER_INFO_TYPE_NONE;
}

static inline operd::OperInfoType
pen_oper_info_type_to_proto (pen_oper_info_type_t info_type)
{
    switch (info_type) {
    case PENOPER_INFO_TYPE_ALERT:
        return operd::OPER_INFO_TYPE_ALERT;
    default:
        break;
    }
    return operd::OPER_INFO_TYPE_NONE;
}

static inline sdk_ret_t
pen_oper_info_to_proto_info_response (pen_oper_info_type_t info_type,
                                      const char *info, size_t info_length,
                                      OperInfoResponse *proto_rsp)
{
    operd::Alert *alert;
    bool result;

    switch (info_type) {
    case PENOPER_INFO_TYPE_ALERT:
        alert = proto_rsp->mutable_alertinfo();
        result = alert->ParseFromArray(info, info_length);
        if (result == false) {
            return SDK_RET_INVALID_OP;
        }
        fprintf(stdout, "Publishing alert %s %s %s %s\n",
                alert->name().c_str(), alert->category().c_str(),
                alert->description().c_str(), alert->message().c_str());
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    proto_rsp->set_infotype(pen_oper_info_type_to_proto(info_type));
    proto_rsp->set_status(types::ApiStatus::API_STATUS_OK);
    return SDK_RET_OK;
}

#endif    // __PEN_OPER_SVC_OPER_SVC_HPP__
