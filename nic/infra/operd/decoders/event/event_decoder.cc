//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file handles events translation to protobuf format
///
//----------------------------------------------------------------------------

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nic/sdk/lib/operd/decoder.h"
#include "nic/infra/operd/event/event_type.hpp"
#include "gen/event/event_defs.h"
#include "gen/proto/operd/events.pb.h"

using operd_event_type_t = operd::event::operd_event_type_t;

static inline operd::EventType
operd_event_type_to_proto_event_type (operd_event_type_t event_type)
{
    switch (event_type) {
    case operd::event::BGP_SESSION_ESTABLISHED:
        return operd::BGP_SESSION_ESTABLISHED;
    case operd::event::BGP_SESSION_DOWN:
        return operd::BGP_SESSION_DOWN;
    case operd::event::DSC_SERVICE_STARTED:
        return operd::DSC_SERVICE_STARTED;
    case operd::event::DSC_SERVICE_STOPPED:
        return operd::DSC_SERVICE_STOPPED;
    case operd::event::SYSTEM_COLDBOOT:
        return operd::SYSTEM_COLDBOOT;
    case operd::event::LINK_UP:
        return operd::LINK_UP;
    case operd::event::LINK_DOWN:
        return operd::LINK_DOWN;
    case operd::event::DSC_MEM_TEMP_ABOVE_THRESHOLD:
        return operd::DSC_MEM_TEMP_ABOVE_THRESHOLD;
    case operd::event::DSC_MEM_TEMP_BELOW_THRESHOLD:
        return operd::DSC_MEM_TEMP_BELOW_THRESHOLD;
    case operd::event::DSC_CATTRIP_INTERRUPT:
        return operd::DSC_CATTRIP_INTERRUPT;
    case operd::event::DSC_PANIC_EVENT:
        return operd::DSC_PANIC_EVENT;
    case operd::event::DSC_POST_DIAG_FAILURE_EVENT:
        return operd::DSC_POST_DIAG_FAILURE_EVENT;
    case operd::event::DSC_INFO_PCIEHEALTH_EVENT:
        return operd::DSC_INFO_PCIEHEALTH_EVENT;
    case operd::event::DSC_WARN_PCIEHEALTH_EVENT:
        return operd::DSC_WARN_PCIEHEALTH_EVENT;
    case operd::event::DSC_ERR_PCIEHEALTH_EVENT:
        return operd::DSC_ERR_PCIEHEALTH_EVENT;
    case operd::event::DSC_FILESYSTEM_USAGE_ABOVE_THRESHOLD:
        return operd::DSC_FILESYSTEM_USAGE_ABOVE_THRESHOLD;
    case operd::event::DSC_FILESYSTEM_USAGE_BELOW_THRESHOLD:
        return operd::DSC_FILESYSTEM_USAGE_BELOW_THRESHOLD;
    case operd::event::VNIC_SESSION_LIMIT_EXCEEDED:
        return operd::VNIC_SESSION_LIMIT_EXCEEDED;
    case operd::event::VNIC_SESSION_THRESHOLD_EXCEEDED:
        return operd::VNIC_SESSION_THRESHOLD_EXCEEDED;
    case operd::event::VNIC_SESSION_WITHIN_THRESHOLD:
        return operd::VNIC_SESSION_WITHIN_THRESHOLD;
    case operd::event::LEARN_PKT:
        return operd::LEARN_PKT;
    default:
        break;
    }
    return operd::DSC_EVENT_TYPE_NONE;
}

static inline operd::EventCategory
operd_event_category_to_proto_category (const char *category)
{
    if (strcmp(category, "System") == 0) {
        return operd::SYSTEM;
    } else if (strcmp(category, "Network") == 0) {
        return operd::NETWORK;
    } else if (strcmp(category, "Resource") == 0) {
        return operd::RESOURCE;
    } else if (strcmp(category, "Learn") == 0) {
        return operd::LEARN;
    } else if (strcmp(category, "Rollout") == 0) {
        return operd::ROLLOUT;
    }
    return operd::NONE;
}

static inline operd::EventSeverity
operd_event_severity_to_proto_severity (const char *severity)
{
    if (strcmp(severity, "INFO") == 0) {
        return operd::INFO;
    } else if (strcmp(severity, "WARN") == 0) {
        return operd::WARN;
    } else if (strcmp(severity, "CRITICAL") == 0) {
        return operd::CRITICAL;
    }
    return operd::DEBUG;
}

static inline void
operd_event_to_proto_event_info (operd::Event &event, const char *message)
{
    operd::EpLearnPkt *learninfo;
    learn_operd_msg_t *learn_msg;

    switch (event.type()) {
    case operd::LEARN_PKT:
        learn_msg = (learn_operd_msg_t *)message;
        learninfo = event.mutable_eplearnpktinfo();
        learninfo->set_hostif(learn_msg->host_if.id, PDS_MAX_KEY_LEN);
        learninfo->set_packet(learn_msg->pkt_data, learn_msg->pkt_len);
        break;
    // TODO: deprecate message and set event type specific info
    default:
        event.set_message(message);
        break;
    }
    return;
}

static size_t
event_decoder (uint8_t encoder, const char *data, size_t data_length,
               char *output, size_t output_size)
{
    size_t len;
    operd_event_data_t *event_data = (operd_event_data_t *)data;
    operd_event_type_t event_type = (operd_event_type_t)event_data->event_id;
    const char *message = event_data->message;
    operd_event_t evt_info = operd::event::event[event_type];
    operd::Event event;
    operd::EventType evt_type;
    operd::EventCategory evt_category;
    operd::EventSeverity evt_severity;

    evt_type = operd_event_type_to_proto_event_type(event_type);
    event.set_type(evt_type);
    evt_category = operd_event_category_to_proto_category(evt_info.category);
    event.set_category(evt_category);
    evt_severity = operd_event_severity_to_proto_severity(evt_info.severity);
    event.set_severity(evt_severity);
    // TODO: set timestamp & source which are not passed to decoder now
    // event.set_source();
    // event.set_time();
    event.set_description(evt_info.description);
    operd_event_to_proto_event_info(event, message);
    len = event.ByteSizeLong();
    assert(len <= output_size);
    bool result = event.SerializeToArray(output, output_size);
    assert(result == true);

    return len;
}

extern "C" void
decoder_lib_init (register_decoder_fn register_decoder)
{
    register_decoder(OPERD_DECODER_EVENT, event_decoder);
}
