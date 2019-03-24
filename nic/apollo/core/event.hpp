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
#include "nic/apollo/api/pds_state.hpp"

// event identifiers
typedef enum event_id_e {
    EVENT_ID_NONE = 0,
    EVENT_ID_PORT = 1,
} event_id_t;

namespace core {

// port event specific information
typedef struct port_event_info_s {
    uint32_t        port_id;
    port_event_t    event;
    port_speed_t    speed;
} port_event_info_t;

// event structure that gets passed around for every event
typedef struct event_s {
    event_id_t               event_id;
    union {
        port_event_info_t    port;
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
