//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
#ifndef __HAL_CORE_EVENT_HPP__
#define __HAL_CORE_EVENT_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/hal/plugins/cfg/ncsi/ncsi_ipc.hpp"
#include "gen/proto/naples_status.pb.h"

namespace hal {
namespace core {

// event identifiers
typedef enum event_id_e {
    EVENT_ID_NONE               = SDK_IPC_EVENT_ID_MAX + 1,
    EVENT_ID_HAL_UP             = EVENT_ID_NONE + 1,
    EVENT_ID_PORT_STATUS        = EVENT_ID_NONE + 2,
    EVENT_ID_UPLINK_STATUS      = EVENT_ID_NONE + 3,
    EVENT_ID_HOST_LIF_CREATE    = EVENT_ID_NONE + 4,
    EVENT_ID_LIF_STATUS         = EVENT_ID_NONE + 5,
    EVENT_ID_UPG                = EVENT_ID_NONE + 6,
    EVENT_ID_UPG_STAGE_STATUS   = EVENT_ID_NONE + 7,
    EVENT_ID_MICRO_SEG          = EVENT_ID_NONE + 8,
    EVENT_ID_NCSI               = EVENT_ID_NONE + 9,
    EVENT_ID_NCSID              = EVENT_ID_NONE + 10,
    EVENT_ID_NICMGR_DELPHIC     = EVENT_ID_NONE + 11,
    EVENT_ID_NICMGR_DSC_STATUS  = EVENT_ID_NONE + 12,   // HAL's delphi thread -> nicmgr thread
} event_id_t;

// port event specific information
typedef struct port_event_info_s {
    uint32_t         id;
    port_event_t     event;
    port_speed_t     speed;
    port_fec_type_t  fec_type;
} port_event_info_t;

// micro segment event handle
typedef struct micro_seg_info_s {
    bool status;
    sdk_ret_t rsp_ret;
} micro_seg_info_t;

typedef struct dsc_status_s {
    nmd::DistributedServiceCardStatus_Mode mode;
} dsc_status_t;

// event structure that gets passed around for every event
typedef struct event_s {
    event_id_t              event_id;
    union {
        port_event_info_t   port;
        micro_seg_info_t    mseg;
        ncsi_ipc_msg_t      ncsi;
        dsc_status_t        dsc_status;
    };
} event_t;

}   // namespace core
}   // namespace hal

using hal::core::event_id_t;

#endif   // __HAL_CORE_EVENT_HPP__

