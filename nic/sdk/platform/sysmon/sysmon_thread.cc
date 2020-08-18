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

static void
hii_ev_led_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    hii_event_t *event = (hii_event_t *)msg->data();
    system_led_t led;

    if (event->type == HII_UPDATE_UID_LED) {
        SDK_HMON_TRACE_INFO("HII Event LED value is %u", event->uid_led_on);
        if (event->uid_led_on) {
            led.event = SYSMON_LED_EVENT_HII_SYSTEM_LED_ON;
            system_led(led);
        } else {
            system_led(g_hii_prev_status);
        }
    }
}

void
sysmon_event_thread_init (void *ctxt)
{
    sysmon_timers_init();

    // subscribe for SDK_IPC_EVENT_ID_HII_UPDATE
    sdk::ipc::subscribe(sdk_ipc_event_id_t::SDK_IPC_EVENT_ID_HII_UPDATE,
                        hii_ev_led_hdlr, NULL);
}

void
sysmon_event_thread_exit (void *ctxt)
{
}

void
sysmon_event_handler (void *msg, void *ctxt)
{
}
