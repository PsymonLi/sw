//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// system monitor implementation
///
//----------------------------------------------------------------------------

#include "lib/event_thread/event_thread.hpp"
#include "platform/sysmon/sysmon_internal.hpp"

sysmon_cfg_t g_sysmon_cfg;
static sdk::event_thread::timer_t g_sysmon_poll_timer_handle;

monfunc_t monfunclist[] = {
    { checkcattrip           },
    { checkfrequency         },
    { checkruntime           },
    { checktemperature       },
    { checkdisk              },
    { checkmemory            },
    { check_memory_threshold },
    { checkpostdiag          },
    { checkliveness          },
    { checkpower             },
    { checkpciehealth        },
};

static void
monitor_system (void)
{
    for (int i = 0; i < SDK_ARRAY_SIZE(monfunclist); i++) {
        monfunclist[i].func();
    }
    return;
}

static void
sysmon_poll_timer_cb (sdk::event_thread::timer_t *timer)
{
    // poll the system variables
    monitor_system();
}

static sdk_ret_t
sysmon_timers_init (void)
{
    // init and start the sysmon poll timer
    timer_init(&g_sysmon_poll_timer_handle, sysmon_poll_timer_cb,
               g_sysmon_cfg.sysmon_poll_time, g_sysmon_cfg.sysmon_poll_time);
    timer_start(&g_sysmon_poll_timer_handle);
    return SDK_RET_OK;
}

void
sysmon_event_thread_init (void *ctxt)
{
    sysmon_timers_init();
}

void
sysmon_event_thread_exit (void *ctxt)
{
}

void
sysmon_event_handler (void *msg, void *ctxt)
{
}
