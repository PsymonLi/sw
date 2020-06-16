//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains core helper functions
///
//----------------------------------------------------------------------------

#include <string>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/framework/api_thread.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/api/debug.hpp"
#include "nic/apollo/nicmgr/nicmgr.hpp"
#include "nic/apollo/pciemgr/pciemgr.hpp"
#include "nic/apollo/learn/learn_thread.hpp"
#include "nic/apollo/api/internal/upgrade.hpp"
#include "nic/metaswitch/stubs/pds_ms_stubs_init.hpp"

using boost::property_tree::ptree;

#define SESSION_AGE_SCAN_INTVL          1     // in seconds
#define SYSTEM_INTR_SCAN_INTVL          5     // in seconds
#define SYSTEM_SCAN_INTVL               10    // in seconds

namespace core {

static sdk_ret_t
parse_cores_config (ptree &pt, pds_state *state)
{
    state->set_control_cores_mask(
        std::stoul(pt.get<string>("control_cores_mask"), nullptr, 16));
    state->set_num_control_cores(
        sdk::lib::count_bits_set(state->control_cores_mask()));
    state->set_data_cores_mask(
        std::stoul(pt.get<string>("data_cores_mask"), nullptr, 16));
    state->set_num_data_cores(
        sdk::lib::count_bits_set(state->data_cores_mask()));

    // update thread library so it knows which threads to pin to
    // which cores
    sdk::lib::thread::control_cores_mask_set(state->control_cores_mask());
    sdk::lib::thread::data_cores_mask_set(state->data_cores_mask());

    return SDK_RET_OK;
}

sdk_ret_t
parse_pipeline_config (string pipeline, pds_state *state)
{
    ptree     pt;
    string    cfg_file;

    cfg_file = state->cfg_path() + "/" + "pipeline.json";

    // make sure cfg file exists
    if (access(cfg_file.c_str(), R_OK) < 0) {
        PDS_TRACE_ERR("config file %s doesn't exist or not accessible",
                      cfg_file.c_str());
        return SDK_RET_ERR;
    }

    // parse the config now
    std::ifstream json_cfg(cfg_file.c_str());
    read_json(json_cfg, pt);
    parse_cores_config(pt, state);
    return SDK_RET_OK;
}

static void
sysmon_timer_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    impl_base::asic_impl()->monitor(monitor_type_t::MONITOR_TYPE_SYSTEM);
}

static void
intr_mon_timer_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    impl_base::asic_impl()->monitor(monitor_type_t::MONITOR_TYPE_INTERRUPTS);
}

sdk_ret_t
schedule_timers (pds_state *state)
{
    void *sysmon_timer;
    void *intr_timer;

    while (!sdk::lib::periodic_thread_is_running()) {
        pthread_yield();
    }

    if (state->platform_type() == platform_type_t::PLATFORM_TYPE_HW) {
        // start periodic timer for scanning system temparature, power etc.
        sysmon_timer = sdk::lib::timer_schedule(
                           PDS_TIMER_ID_SYSTEM_SCAN,
                           SYSTEM_SCAN_INTVL * TIME_MSECS_PER_SEC,
                           nullptr, sysmon_timer_cb, true);
        if (sysmon_timer == NULL) {
            PDS_TRACE_ERR("Failed to start system monitoring timer");
            return SDK_RET_ERR;
        }
        // start periodic timer for scanning system interrupts
        intr_timer = sdk::lib::timer_schedule(
                           PDS_TIMER_ID_INTR_SCAN,
                           SYSTEM_INTR_SCAN_INTVL * TIME_MSECS_PER_SEC,
                           nullptr, intr_mon_timer_cb, true);
        if (intr_timer == NULL) {
            PDS_TRACE_ERR("Failed to start system interrupt timer");
            return SDK_RET_ERR;
        }
        //PDS_TRACE_DEBUG("Started periodic system scan timer with %us intvl",
                          //SYSTEM_SCAN_INTVL);
    }
    return SDK_RET_OK;
}

// starting point for the periodic thread loop
static void *
periodic_thread_start (void *ctxt)
{
    // initialize timer wheel
    sdk::lib::periodic_thread_init(ctxt);
    // run main loop
    sdk::lib::periodic_thread_run(ctxt);

    return NULL;
}

// wrapper API to create all threads
static sdk::lib::thread *
thread_create (const char *name, uint32_t thread_id,
               sdk::lib::thread_role_t thread_role,
               uint64_t cores_mask,
               sdk::lib::thread_entry_func_t entry_func,
               uint32_t thread_prio, int sched_policy, void *data)
{
    static sdk::lib::thread * new_thread;

    new_thread =
        sdk::lib::thread::factory(name, thread_id, thread_role, cores_mask,
                                  entry_func, thread_prio, sched_policy,
                                  (thread_role == sdk::lib::THREAD_ROLE_DATA) ?
                                       false : true);
    if (new_thread) {
        new_thread->set_data(data);
    }
    return new_thread;
}

// spawn all the necessary threads
sdk_ret_t
spawn_periodic_thread (pds_state *state)
{
    sdk::lib::thread    *new_thread;

    // spawn periodic thread that does background tasks
    new_thread =
        thread_create(std::string("periodic").c_str(),
            PDS_THREAD_ID_PERIODIC,
            sdk::lib::THREAD_ROLE_CONTROL,
            0x0,    // use all control cores
            periodic_thread_start,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            NULL);
    SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                            "Periodic thread create failure");
    new_thread->start(new_thread);
    return SDK_RET_OK;
}

sdk_ret_t
spawn_pciemgr_thread (pds_state *state)
{
    sdk::lib::thread    *new_thread;

    if (state->platform_type() != platform_type_t::PLATFORM_TYPE_RTL) {
        // spawn pciemgr thread
        new_thread =
            thread_create("pciemgr", PDS_THREAD_ID_PCIEMGR,
                sdk::lib::THREAD_ROLE_CONTROL,
                0x0,    // use all control cores
                pdspciemgr::pciemgrapi::pciemgr_thread_start,
                sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                NULL);
        SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                                "pciemgr thread create failure");
        new_thread->start(new_thread);
    }
    return SDK_RET_OK;
}

sdk_ret_t
spawn_nicmgr_thread (pds_state *state)
{
    sdk::event_thread::event_thread *new_thread;

    if (state->platform_type() != platform_type_t::PLATFORM_TYPE_RTL) {
        // spawn nicmgr thread
        new_thread =
            sdk::event_thread::event_thread::factory(
                "nicmgr", PDS_THREAD_ID_NICMGR,
                sdk::lib::THREAD_ROLE_CONTROL,
                0x0,    // use all control cores
                nicmgr::nicmgrapi::nicmgr_thread_init,
                nicmgr::nicmgrapi::nicmgr_thread_exit,
                nicmgr::nicmgrapi::nicmgr_event_handler,
                sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                true, true);
        SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                                "nicmgr thread create failure");
        new_thread->set_data(state);
        new_thread->start(new_thread);
    }
    return SDK_RET_OK;
}

bool
is_nicmgr_ready (void)
{
    return sdk::lib::thread::find(PDS_THREAD_ID_NICMGR)->ready();
}

void
stop_learn_thread (void)
{
    PDS_TRACE_INFO("Stopping learn thread");
    sdk::lib::thread::find(PDS_THREAD_ID_LEARN)->stop();
    PDS_TRACE_INFO("Waiting learn thread to stop");
    sdk::lib::thread::find(PDS_THREAD_ID_LEARN)->wait();
}

sdk_ret_t
spawn_api_thread (pds_state *state)
{
    sdk::event_thread::event_thread *new_thread;

    new_thread =
        sdk::event_thread::event_thread::factory(
            "cfg", PDS_THREAD_ID_API,
            sdk::lib::THREAD_ROLE_CONTROL,
            0x0,    // use all control cores
            api::api_thread_init_fn,
            api::api_thread_exit_fn,
            api::api_thread_event_cb,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            true);
     SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                             "cfg thread create failure");
     new_thread->set_data(state);
     new_thread->start(new_thread);
     return SDK_RET_OK;
}

sdk_ret_t
spawn_learn_thread (pds_state *state)
{
    sdk::event_thread::event_thread *new_thread;

    if (!learn::learning_enabled()) {
        PDS_TRACE_DEBUG("Skip spawning learn thread");
        return SDK_RET_OK;
    }

    // spawn learn thread
    PDS_TRACE_DEBUG("Spawning learn thread");
    new_thread =
        sdk::event_thread::event_thread::factory(
            "learn", PDS_THREAD_ID_LEARN,
            sdk::lib::THREAD_ROLE_CONTROL,
            0,
            learn::learn_thread_init_fn,
            learn::learn_thread_exit_fn,
            learn::learn_thread_event_cb,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            true, true);
    SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                            "learn thread create failure");
    new_thread->start(new_thread);
    return SDK_RET_OK;
}

sdk_ret_t
spawn_routing_thread (pds_state *state)
{
    sdk::lib::thread    *new_thread;

    // spawn control plane routing thread
    PDS_TRACE_DEBUG("Spawning routing thread");
    new_thread =
        thread_create("routing", PDS_THREAD_ID_ROUTING,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0,
            pds_ms::pds_ms_thread_init,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            state);
    SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                            "Routing thread create failure");
    new_thread->start(new_thread);
    return SDK_RET_OK;
}

bool
is_routing_thread_ready (void)
{
    return sdk::lib::thread::find(PDS_THREAD_ID_ROUTING)->ready();
}

sdk_ret_t
spawn_upgrade_thread (sdk::upg::upg_ev_params_t *params)
{
    sdk::event_thread::event_thread *new_thread;

    new_thread =
        sdk::event_thread::event_thread::factory(
            "upgrade", PDS_THREAD_ID_UPGRADE,
            sdk::lib::THREAD_ROLE_CONTROL,
            0x0,    // use all control cores
            api::upgrade_thread_init_fn,
            api::upgrade_thread_exit_fn,
            api::upgrade_thread_event_cb,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            true);
     SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                             "upgrade thread create failure");
     new_thread->set_data(params);
     new_thread->start(new_thread);
     return SDK_RET_OK;
}

static bool
thread_stop_cb_ (sdk::lib::thread *thr, void *ctxt)
{
    if ((thr->thread_id() > PDS_THREAD_ID_MIN) &&
        (thr->thread_id() < PDS_THREAD_ID_MAX)) {
        PDS_TRACE_DEBUG("Stopping thread %s", thr->name());
        thr->stop();
    }
    // continue the walk
    return false;
}

// stop the threads
void
threads_stop (void)
{
    sdk::lib::thread::walk(thread_stop_cb_, NULL);
}

static bool
thread_wait_cb_ (sdk::lib::thread *thr, void *ctxt)
{
    if ((thr->thread_id() > PDS_THREAD_ID_MIN) &&
        (thr->thread_id() < PDS_THREAD_ID_MAX)) {
        PDS_TRACE_DEBUG("Waiting for thread %s to exit", thr->name());
        thr->wait();
        // free the allocated thread
        sdk::lib::thread::destroy(thr);
    }
    // continue the walk
    return false;
}

void
threads_wait (void)
{
    sdk::lib::thread::walk(thread_wait_cb_, NULL);
}

// suspend the threads
static bool
thread_suspend_cb_ (sdk::lib::thread *thr, void *ctxt)
{
    sdk_ret_t *ret = (sdk_ret_t *)ctxt;

    if ((thr->thread_id() > PDS_THREAD_ID_MIN) &&
        (thr->thread_id() < PDS_THREAD_ID_MAX)) {
        // TODO periodic thread. all modules should un-register their timers
        // and g_twheel num entries should be zero
        if (thr->thread_id() != PDS_THREAD_ID_PERIODIC) {
            PDS_TRACE_DEBUG("Suspending thread %s", thr->name());
            *ret = thr->suspend();
            if (*ret != SDK_RET_OK) {
                return true; // stop the walk
            }
        }
    }
    // continue the walk
    return false;
}

sdk_ret_t
threads_suspend (void)
{
    sdk_ret_t ret = SDK_RET_OK;

    sdk::lib::thread::walk(thread_suspend_cb_, (void *)&ret);
    return ret;
}

// resume the threads
static bool
thread_resume_cb_ (sdk::lib::thread *thr, void *ctxt)
{
    sdk_ret_t *ret = (sdk_ret_t *)ctxt;

    if ((thr->thread_id() > PDS_THREAD_ID_MIN) &&
        (thr->thread_id() < PDS_THREAD_ID_MAX)) {
        // TODO periodic thread. all modules should un-register their timers
        // and g_twheel num entries should be zero
        if (thr->thread_id() != PDS_THREAD_ID_PERIODIC) {
            PDS_TRACE_DEBUG("Resuming thread %s", thr->name());
            *ret = thr->resume();
            if (*ret != SDK_RET_OK) {
                return true; // stop the walk
            }
        }
    }
    // continue the walk
    return false;
}

sdk_ret_t
threads_resume (void)
{
    sdk_ret_t ret = SDK_RET_OK;

    sdk::lib::thread::walk(thread_resume_cb_, (void *)&ret);
    return ret;
}

// check if the threads are ready
static bool
thread_ready_cb_ (sdk::lib::thread *thr, void *ctxt)
{
    bool *rv = (bool *)ctxt;
    bool ready;

    if ((thr->thread_id() > PDS_THREAD_ID_MIN) &&
        (thr->thread_id() < PDS_THREAD_ID_MAX)) {
            ready = thr->ready();
            SDK_TRACE_DEBUG("Thread %s ready %u", thr->name(), ready);
            if (!ready) {
                *rv = false;
            }
    }
    // continue the walk
    return false;
}

bool
threads_ready (void)
{
    bool rv = true;

    sdk::lib::thread::walk(thread_ready_cb_, (void *)&rv);
    return rv;
}

// check if the threads are suspended
static bool
thread_suspended_cb_ (sdk::lib::thread *thr, void *ctxt)
{
    bool *rv = (bool *)ctxt;
    bool suspended;

    if ((thr->thread_id() > PDS_THREAD_ID_MIN) &&
        (thr->thread_id() < PDS_THREAD_ID_MAX)) {
        // TODO periodic thread. all modules should un-register their timers
        // and g_twheel num entries should be zero
        if (thr->thread_id() != PDS_THREAD_ID_PERIODIC) {
            suspended = thr->suspended();
            SDK_TRACE_DEBUG("Thread %s suspended %u", thr->name(), suspended);
            if (!suspended) {
                *rv = false;
            }
        }
    }
    // continue the walk
    return false;
}

bool
threads_suspended (void)
{
    bool rv = true;

    sdk::lib::thread::walk(thread_suspended_cb_, (void *)&rv);
    return rv;
}

// check if the threads are resumed
static bool
thread_resumed_cb_ (sdk::lib::thread *thr, void *ctxt)
{
    bool *rv = (bool *)ctxt;
    bool suspended;

    if ((thr->thread_id() > PDS_THREAD_ID_MIN) &&
        (thr->thread_id() < PDS_THREAD_ID_MAX)) {
        // TODO periodic thread. all modules should un-register their timers
        // and g_twheel num entries should be zero
        if (thr->thread_id() != PDS_THREAD_ID_PERIODIC) {
            suspended = thr->suspended();
            SDK_TRACE_DEBUG("Thread %s resumed %u", thr->name(), !suspended);
            if (suspended) {   // still in suspended state
                *rv = false;
            }
        }
    }
    // continue the walk
    return false;
}

bool
threads_resumed (void)
{
    bool rv = true;

    sdk::lib::thread::walk(thread_resumed_cb_, (void *)&rv);
    return rv;
}

// install signal handler for given signal
sdk_ret_t
sig_init (int signal, sig_handler_t sig_handler)
{
    struct sigaction    act;

    if (sig_handler == NULL) {
        return SDK_RET_ERR;
    }

    PDS_TRACE_DEBUG("Installing signal handler for signal %d", signal);
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = sig_handler;
    act.sa_flags = SA_SIGINFO;
    sigaction(signal, &act, NULL);

    return SDK_RET_OK;
}

}    // namespace core
