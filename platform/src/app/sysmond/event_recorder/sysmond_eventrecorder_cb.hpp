#ifndef _SYSMOND_EVENTRECORDER_CB_H_
#define _SYSMOND_EVENTRECORDER_CB_H_

#include "platform/sysmon/sysmon.hpp"
#include "nic/sdk/platform/sensor/sensor.hpp"

void eventrecorder_cattrip_event_cb(void);
void eventrecorder_panic_event_cb(void);
void eventrecorder_postdiag_event_cb(void);
void eventrecorder_temp_event_cb(system_temperature_t *temperature,
                                 sysmon_hbm_threshold_event_t hbm_event);
void eventrecorder_fatal_interrupt_event_cb(const char *desc);
void eventrecorder_pciehealth_event_cb(sysmon_pciehealth_severity_t sev, const char *reason);

#endif    // _SYSMOND_EVENTRECORDER_CB_H_
