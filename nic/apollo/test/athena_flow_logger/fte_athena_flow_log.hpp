
//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains all session aging test cases for athena
///
//----------------------------------------------------------------------------
#include <cstring>
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/api/include/athena/pds_flow_log.h"

namespace event = sdk::event_thread;

namespace flow_logger {
sdk_ret_t
fte_athena_spawn_flow_log_thread (int numSeconds,
                                  std::string logdir);
}
