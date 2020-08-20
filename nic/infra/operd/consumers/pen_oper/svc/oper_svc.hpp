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

#include "gen/event/event_defs.h"
#include "gen/proto/operd/oper.grpc.pb.h"

static inline pen_oper_info_type_t
pen_oper_info_type_from_proto (operd::OperInfoType info_type)
{
    switch (info_type) {
    case operd::OPER_INFO_TYPE_EVENT:
        return PENOPER_INFO_TYPE_EVENT;
    default:
        break;
    }
    return PENOPER_INFO_TYPE_NONE;
}

static inline operd::OperInfoType
pen_oper_info_type_to_proto (pen_oper_info_type_t info_type)
{
    switch (info_type) {
    case PENOPER_INFO_TYPE_EVENT:
        return operd::OPER_INFO_TYPE_EVENT;
    default:
        break;
    }
    return operd::OPER_INFO_TYPE_NONE;
}

static inline operd::event::operd_event_type_t
operd_event_type_from_proto (operd::EventType event_type)
{
    switch (event_type) {
    case operd::BGP_SESSION_ESTABLISHED:
        return operd::event::BGP_SESSION_ESTABLISHED;
    case operd::BGP_SESSION_DOWN:
        return operd::event::BGP_SESSION_DOWN;
    case operd::DSC_SERVICE_STARTED:
        return operd::event::DSC_SERVICE_STARTED;
    case operd::DSC_SERVICE_STOPPED:
        return operd::event::DSC_SERVICE_STOPPED;
    case operd::SYSTEM_COLDBOOT:
        return operd::event::SYSTEM_COLDBOOT;
    case operd::LINK_UP:
        return operd::event::LINK_UP;
    case operd::LINK_DOWN:
        return operd::event::LINK_DOWN;
    case operd::DSC_MEM_TEMP_ABOVE_THRESHOLD:
        return operd::event::DSC_MEM_TEMP_ABOVE_THRESHOLD;
    case operd::DSC_MEM_TEMP_BELOW_THRESHOLD:
        return operd::event::DSC_MEM_TEMP_BELOW_THRESHOLD;
    case operd::DSC_CATTRIP_INTERRUPT:
        return operd::event::DSC_CATTRIP_INTERRUPT;
    case operd::DSC_PANIC_EVENT:
        return operd::event::DSC_PANIC_EVENT;
    case operd::DSC_POST_DIAG_FAILURE_EVENT:
        return operd::event::DSC_POST_DIAG_FAILURE_EVENT;
    case operd::DSC_INFO_PCIEHEALTH_EVENT:
        return operd::event::DSC_INFO_PCIEHEALTH_EVENT;
    case operd::DSC_WARN_PCIEHEALTH_EVENT:
        return operd::event::DSC_WARN_PCIEHEALTH_EVENT;
    case operd::DSC_ERR_PCIEHEALTH_EVENT:
        return operd::event::DSC_ERR_PCIEHEALTH_EVENT;
    case operd::DSC_FILESYSTEM_USAGE_ABOVE_THRESHOLD:
        return operd::event::DSC_FILESYSTEM_USAGE_ABOVE_THRESHOLD;
    case operd::DSC_FILESYSTEM_USAGE_BELOW_THRESHOLD:
        return operd::event::DSC_FILESYSTEM_USAGE_BELOW_THRESHOLD;
    case operd::VNIC_SESSION_LIMIT_EXCEEDED:
        return operd::event::VNIC_SESSION_LIMIT_EXCEEDED;
    case operd::VNIC_SESSION_THRESHOLD_EXCEEDED:
        return operd::event::VNIC_SESSION_THRESHOLD_EXCEEDED;
    case operd::VNIC_SESSION_WITHIN_THRESHOLD:
        return operd::event::VNIC_SESSION_WITHIN_THRESHOLD;
    case operd::LEARN_PKT:
        return operd::event::LEARN_PKT;
    default:
        break;
    }
    return operd::event::OPERD_EVENT_TYPE_NONE;
}

static inline uint32_t
operd_info_id_from_proto (OperInfoResponse *resp)
{
    operd::Event event;

    switch (resp->oper_info_case()) {
    case operd::OperInfoResponse::kEventInfo:
        event = resp->eventinfo();
        return operd_event_type_from_proto(event.type());
    default:
        break;
    }
    assert(0);
    return 0;
}

static inline sdk_ret_t
pen_oper_info_to_proto_info_response (pen_oper_info_type_t info_type,
                                      const char *info, size_t info_length,
                                      OperInfoResponse *proto_rsp)
{
    operd::Event *event;
    bool result;

    switch (info_type) {
    case PENOPER_INFO_TYPE_EVENT:
        event = proto_rsp->mutable_eventinfo();
        result = event->ParseFromArray(info, info_length);
        if (result == false) {
            return SDK_RET_INVALID_OP;
        }
        switch (event->type()) {
        case operd::LEARN_PKT:
            break;
        default:
            fprintf(stdout, "Publishing event %u %u %s %s\n", event->type(),
                    event->category(), event->description().c_str(),
                    event->message().c_str());
        }
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    proto_rsp->set_infotype(pen_oper_info_type_to_proto(info_type));
    proto_rsp->set_status(types::ApiStatus::API_STATUS_OK);
    return SDK_RET_OK;
}

#endif    // __PEN_OPER_SVC_OPER_SVC_HPP__
