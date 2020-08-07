//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// system monitor
///
//----------------------------------------------------------------------------

#include "lib/thread/thread.hpp"
#include "lib/event_thread/event_thread.hpp"
#include "platform/sysmon/sysmon_internal.hpp"
#include "platform/sysmon/sysmon_thread.hpp"

#define HEALTH_OK 1
#define HEALTH_NOK 0

systemled_t currentstatus = {UKNOWN_STATE, LED_COLOR_NONE};

void
sysmgrsystemled (systemled_t led)
{

    //Check if already at max warning level.
    if (led.event >= currentstatus.event) {
        return;
    }
    switch (led.event) {
        case CRITICAL_EVENT:
            currentstatus.event = CRITICAL_EVENT;
            currentstatus.color = LED_COLOR_YELLOW;
            pal_system_set_led(LED_COLOR_YELLOW, LED_FREQUENCY_0HZ);
            //health not ok
            pal_cpld_set_card_status(SYSMOND_HEALTH_NOT_OK);
            break;
        case NON_CRITICAL_EVENT:
            currentstatus.event = NON_CRITICAL_EVENT;
            currentstatus.color = LED_COLOR_YELLOW;
            pal_system_set_led(LED_COLOR_YELLOW, LED_FREQUENCY_0HZ);
            //health not ok
            pal_cpld_set_card_status(SYSMOND_HEALTH_NOT_OK);
            break;
        case PROCESS_CRASHED_EVENT:
            currentstatus.event = PROCESS_CRASHED_EVENT;
            currentstatus.color = LED_COLOR_YELLOW;
            pal_system_set_led(LED_COLOR_YELLOW, LED_FREQUENCY_0HZ);
            //health not ok
            pal_cpld_set_card_status(SYSMOND_HEALTH_NOT_OK);
            break;
        case EVERYTHING_WORKING:
            currentstatus.event = EVERYTHING_WORKING;
            currentstatus.color = LED_COLOR_GREEN;
            pal_system_set_led(LED_COLOR_GREEN, LED_FREQUENCY_0HZ);
            //health ok
            pal_cpld_set_card_status(SYSMOND_HEALTH_OK);
            break;
        default:
            return;
    }
    return;
}

static sdk_ret_t
thread_init (void)
{
    int thread_id;
    sdk::event_thread::event_thread *new_thread;

    thread_id = SDK_IPC_ID_SYSMON;
    new_thread =
        sdk::event_thread::event_thread::factory(
            "sysmon", thread_id,
            sdk::lib::THREAD_ROLE_CONTROL,
            0x0,    // use all control cores
            sysmon_event_thread_init,
            sysmon_event_thread_exit,
            sysmon_event_handler,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread_flags_t::THREAD_YIELD_ENABLE);
    SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                            "sysmon thread create failure");
    new_thread->start(new_thread);
    return SDK_RET_OK;
}

int
sysmon_init (sysmon_cfg_t *sysmon_cfg)
{
    systemled_t led;

    if (sysmon_cfg == NULL) {
        SDK_HMON_TRACE_ERR("Invalid params, cfg is NULL");
        return -1;
    }

    g_sysmon_cfg = *sysmon_cfg;

    memory_threshold_cfg_init();

    SDK_HMON_TRACE_INFO("Monitoring system events");

    // check for panic dump
    checkpanicdump();

    // update the firmware version in cpld
    updatefwversion();

    SDK_HMON_TRACE_INFO("HBM Threshold temperature is %u",
                   g_sysmon_cfg.catalog->hbmtemperature_threshold());

    if (configurefrequency() == 0) {
        SDK_HMON_TRACE_INFO("Frequency set from file");
    } else {
        SDK_HMON_TRACE_INFO("Failed to set frequency from file");
    }
    led.event = EVERYTHING_WORKING;
    sysmgrsystemled(led);

    thread_init();
    return 0;
}
