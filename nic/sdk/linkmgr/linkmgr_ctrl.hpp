//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// linkmgr ctrl thread requests handling
///
//----------------------------------------------------------------------------

#include "lib/event_thread/event_thread.hpp"

namespace sdk {
namespace linkmgr {

void port_bringup_timer_cb(sdk::event_thread::timer_t *timer);
void port_debounce_timer_cb(sdk::event_thread::timer_t *timer);

sdk_ret_t port_link_poll_timer_add(void *pd_p);
sdk_ret_t port_link_poll_timer_delete(void *pd_p);

void linkmgr_event_thread_init(void *ctxt);
void linkmgr_event_thread_exit(void *ctxt);
void linkmgr_event_handler(void *msg, void *ctxt);

}    // namespace linkmgr
}    // namespace sdk
