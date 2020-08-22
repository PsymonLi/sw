//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains event identifiers, event data definitions and related
/// APIs
///
//----------------------------------------------------------------------------

#ifndef __CORE_EVENT_HPP__
#define __CORE_EVENT_HPP__

#include <signal.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_if.hpp"
#include "nic/apollo/include/globals.hpp"

// event identifiers
typedef enum event_id_e {
    EVENT_ID_NONE                 = PDS_IPC_EVENT_ID_HAL_MIN,
    EVENT_ID_PDS_HAL_UP           = (PDS_IPC_EVENT_ID_HAL_MIN + 1),
    EVENT_ID_PORT_STATUS          = (PDS_IPC_EVENT_ID_HAL_MIN + 2),
    EVENT_ID_UPLINK_STATUS        = (PDS_IPC_EVENT_ID_HAL_MIN + 3),
    EVENT_ID_HOST_LIF_CREATE      = (PDS_IPC_EVENT_ID_HAL_MIN + 4),
    EVENT_ID_HOST_IF_STATUS       = (PDS_IPC_EVENT_ID_HAL_MIN + 5),
    EVENT_ID_VNIC_UPD             = (PDS_IPC_EVENT_ID_HAL_MIN + 6),
    EVENT_ID_MAC_LEARN            = (PDS_IPC_EVENT_ID_HAL_MIN + 7),
    EVENT_ID_IP_LEARN             = (PDS_IPC_EVENT_ID_HAL_MIN + 8),
    EVENT_ID_MAC_AGE              = (PDS_IPC_EVENT_ID_HAL_MIN + 9),
    EVENT_ID_IP_AGE               = (PDS_IPC_EVENT_ID_HAL_MIN + 10),
    EVENT_ID_IP_DELETE            = (PDS_IPC_EVENT_ID_HAL_MIN + 11),
    EVENT_ID_MAC_DELETE           = (PDS_IPC_EVENT_ID_HAL_MIN + 12),
    EVENT_ID_MAC_MOVE_L2R         = (PDS_IPC_EVENT_ID_HAL_MIN + 13),
    EVENT_ID_IP_MOVE_L2R          = (PDS_IPC_EVENT_ID_HAL_MIN + 14),
    EVENT_ID_MAC_MOVE_R2L         = (PDS_IPC_EVENT_ID_HAL_MIN + 15),
    EVENT_ID_IP_MOVE_R2L          = (PDS_IPC_EVENT_ID_HAL_MIN + 16),
    EVENT_ID_PDS_CFG_OBJ_SET      = (PDS_IPC_EVENT_ID_HAL_MIN + 17),
    EVENT_ID_PDS_API_BATCH_COMMIT = (PDS_IPC_EVENT_ID_HAL_MIN + 18),
    EVENT_ID_PDS_API_BATCH_ABORT  = (PDS_IPC_EVENT_ID_HAL_MIN + 19),
} event_id_t;

namespace core {

// port event specific information
typedef struct port_event_info_s {
    if_index_t       ifindex;
    port_event_t     event;
    port_speed_t     speed;
    port_fec_type_t  fec_type;
} port_event_info_t;

// uplink interface event specific information {
typedef struct uplink_event_info_s {
    if_index_t        ifindex;
    pds_if_state_t    state;
} uplink_event_info_t;

// host interface event specific information
typedef struct host_if_event_info_s {
    if_index_t       ifindex;
    char             name[SDK_MAX_NAME_LEN];
    mac_addr_t       mac;
    pds_if_state_t   state;
} host_if_event_info_t;

// MAC, IP learn specific information
typedef struct learn_event_info_s {
    pds_obj_key_t   vpc;
    pds_obj_key_t   subnet;
    if_index_t      ifindex;
    ip_addr_t       ip_addr;
    mac_addr_t      mac_addr;
    // vnic hw id is needed by VPP for flow fixup, valid for move events
    uint16_t        vnic_hw_id;
} learn_event_info_t;

// API batch commit/abort event information
typedef struct api_batch_event_info_s {
    // epoch being committed or aborted
    pds_epoch_t epoch;
} api_batch_event_info_t;

// host dev information
typedef struct host_dev_event_info_s {
    pds_lif_id_t id;
} host_dev_event_info_t;

typedef struct vnic_event_info_s {
    pds_obj_key_t vnic;
    bool conn_track_en;
} vnic_event_info_t;

// event structure that gets passed around for every event
typedef struct event_s {
    event_id_t                 event_id;
    union {
        port_event_info_t      port;
        uplink_event_info_t    uplink;
        host_if_event_info_t   intf;
        learn_event_info_t     learn;
        api_batch_event_info_t batch;
        host_dev_event_info_t  host_dev;
        vnic_event_info_t      vnic;
    };
} event_t;

///< \brief    allocate event memory
///< \return    allocated event instance
event_t *event_alloc(void);

///< \brief    free event memory
///< \param[in] event    event to be freed back
void event_free(event_t *event);

///< enqueue event to a given thread
///< event    event to be enqueued
///< \param[in] thread_id    id of the thread to enqueue the event to
///< \return    true if the operation succeeded or else false
bool event_enqueue(event_t *event, uint32_t thread_id);

///< \brief    dequeue event from given thread
///< \param[in] thread_id    id of the thread from which event needs
///<                         to be dequeued from
event_t *event_dequeue(uint32_t thread_id);

}    // namespace core

#endif    // __CORE_EVENT_HPP__
