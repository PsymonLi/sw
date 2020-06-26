//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __DELPHI_EVENTS_HPP__
#define __DELPHI_EVENTS_HPP__

#include "nic/include/base.hpp"
#include "nic/hal/hal_trace.hpp"
#include "nic/include/hal.hpp"
#include "nic/utils/events/recorder/recorder.hpp"
#include "gen/proto/eventtypes.pb.h"

namespace hal {

// Initializes event recorder
hal_ret_t hal_events_recorder_init(void);
void hal_session_event_notify(eventtypes::EventTypes event_id, uint64_t session_limit);
void hal_workload_migration_event_notify(eventtypes::EventTypes event_id, const char *dir,
                                         const char *mac);

}    // namespace hal

#endif    // __DELPHI_EVENTS_HPP__

