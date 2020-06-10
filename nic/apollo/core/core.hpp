// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains core helper functions
///
//----------------------------------------------------------------------------

#ifndef __CORE_CORE_HPP__
#define __CORE_CORE_HPP__

#include <signal.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/sdk/lib/periodic/periodic.hpp"
#include "nic/sdk/upgrade/include/ev.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/include/globals.hpp"

namespace core {

// threads inside the PDS HAL library (these ids are also used as IPC endpoints)
enum {
    PDS_THREAD_ID_NONE = pds_thread_id_t::PDS_THREAD_ID_MIN,
    PDS_THREAD_ID_CFG,
    PDS_THREAD_ID_API,
    PDS_THREAD_ID_PERIODIC,
    PDS_THREAD_ID_NICMGR,
    PDS_THREAD_ID_PCIEMGR,
    PDS_THREAD_ID_LEARN,
    PDS_THREAD_ID_ROUTING,
    PDS_THREAD_ID_UPGRADE,
    PDS_THREAD_ID_FTE,
    PDS_THREAD_ID_MAX = pds_thread_id_t::PDS_THREAD_ID_MAX,
};

enum {
    PDS_TIMER_ID_NONE         = 0,
    PDS_TIMER_ID_SESSION_AGE  = 1,
    PDS_TIMER_ID_SYSTEM_SCAN  = 2,
    PDS_TIMER_ID_INTR_SCAN    = 3,
};

sdk_ret_t spawn_periodic_thread(pds_state *state);
sdk_ret_t spawn_nicmgr_thread(pds_state *state);
sdk_ret_t spawn_pciemgr_thread(pds_state *state);
sdk_ret_t spawn_api_thread(pds_state *state);
sdk_ret_t spawn_learn_thread(pds_state *state);
sdk_ret_t spawn_routing_thread(pds_state *state);
sdk_ret_t spawn_upgrade_thread(sdk::upg::upg_ev_params_t *params);
bool is_nicmgr_ready(void);
bool is_routing_thread_ready(void);
void stop_learn_thread(void);
void threads_stop(void);
void threads_wait(void);
sdk_ret_t threads_suspend(void);
sdk_ret_t threads_resume(void);
bool threads_ready(void);
bool threads_suspended(void);
bool threads_resumed(void);
sdk::lib::thread *thread_get(uint32_t thread_id);
sdk_ret_t parse_global_config(string pipeline, string cfg_file,
                              pds_state *state);
sdk_ret_t parse_pipeline_config(string pipeline, pds_state *state);
typedef void (*sig_handler_t)(int sig, siginfo_t *info, void *ptr);
sdk_ret_t sig_init(int signal, sig_handler_t sig_handler);
sdk_ret_t schedule_timers(pds_state *state);

}    // namespace core

#endif    // __CORE_CORE_HPP__
