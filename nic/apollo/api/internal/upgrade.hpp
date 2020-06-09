//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// upgrade thread implementation
///
//----------------------------------------------------------------------------

#ifndef __INTERNAL_UPGRADE_HPP__
#define __INTERNAL_UPGRADE_HPP__

#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/lib/dpdk/dpdk.hpp"

namespace event = sdk::event_thread;

namespace api {

// callback function invoked during upgrade thread initialization
void upgrade_thread_init_fn(void *ctxt);

// callback function invoked during upgrade thread exit
void upgrade_thread_exit_fn(void *ctxt);

// callback function invoked to process events received
void upgrade_thread_event_cb(void *msg, void *ctxt);

}    // namespace api

#endif    // __INTERNAL_UPGRADE_HPP__
