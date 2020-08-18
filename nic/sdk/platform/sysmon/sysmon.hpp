//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// system monitor
///
//----------------------------------------------------------------------------

#ifndef __SYSMON_H__
#define __SYSMON_H__

#include "lib/catalog/catalog.hpp"
#include "platform/sensor/sensor.hpp"
#include "platform/drivers/xcvr_qsfp.hpp"
#include "platform/pal/include/pal_types.h"
#include "include/sdk/globals.hpp"
#include "lib/ipc/ipc.hpp"

typedef enum {
   SYSMON_LED_EVENT_HII_SYSTEM_LED_ON = 0,
   SYSMON_LED_EVENT_CRITICAL,
   SYSMON_LED_EVENT_NON_CRITICAL,
   SYSMON_LED_EVENT_PROCESS_CRASHED,
   SYSMON_LED_EVENT_SYSTEM_OK,
   SYSMON_LED_EVENT_UKNOWN_STATE
} sysmon_led_event_t;

typedef enum {
   SYSMON_HBM_TEMP_NONE = 0,
   SYSMON_HBM_TEMP_ABOVE_THRESHOLD = 1,
   SYSMON_HBM_TEMP_BELOW_THRESHOLD = 2
} sysmon_hbm_threshold_event_t;

typedef enum {
    SYSMON_PCIEHEALTH_INFO = 0,
    SYSMON_PCIEHEALTH_WARN = 1,
    SYSMON_PCIEHEALTH_ERROR = 2,
} sysmon_pciehealth_severity_t;

typedef enum {
    SYSMON_FILESYSTEM_USAGE_NONE = 0,
    SYSMON_FILESYSTEM_USAGE_ABOVE_THRESHOLD,
    SYSMON_FILESYSTEM_USAGE_BELOW_THRESHOLD,
} sysmon_filesystem_threshold_event_t;

typedef struct system_led_s {
    sysmon_led_event_t event;
    pal_led_color_t color;
} system_led_t;

typedef struct system_memory_s {
    uint64_t total_mem;
    uint64_t available_mem;
    uint64_t free_mem;
} __attribute__((packed)) system_memory_t;

typedef struct sysmon_memory_threshold_cfg_s {
    std::string  path;
    uint32_t     mem_threshold_percent;
} sysmon_memory_threshold_cfg_t;

typedef enum hii_update_type {
    HII_UPDATE_UID_LED = 0,
} hii_event_type_t;

typedef struct {
    hii_event_type_t type;
    union {
        bool uid_led_on;
    };
} hii_event_t;

// callbacks
typedef void (*frequency_change_event_cb_t)(uint32_t frequency);
typedef void (*cattrip_event_cb_t)(void);
typedef void (*power_event_cb_t)(system_power_t *power);
typedef void (*temp_event_cb_t)(system_temperature_t *temperature,
                                sdk::platform::qsfp_temperature_t *xcvrtemp,
                                sysmon_hbm_threshold_event_t hbm_event);
typedef void (*memory_event_cb_t)(system_memory_t *system_memory);
typedef void (*panic_event_cb_t)(void);
typedef void (*postdiag_event_cb_t)(void);
typedef void (*liveness_event_cb_t)(void);
typedef void (*pciehealth_event_cb_t)(sysmon_pciehealth_severity_t sev,
                                      const char *reason);
typedef void (*memory_threshold_event_cb_t)(sysmon_filesystem_threshold_event_t event,
                                            const char *path,
                                            uint32_t threshold_percent);

typedef struct sysmon_cfg_s {
    frequency_change_event_cb_t   frequency_change_event_cb;
    cattrip_event_cb_t            cattrip_event_cb;
    power_event_cb_t              power_event_cb;
    temp_event_cb_t               temp_event_cb;
    memory_event_cb_t             memory_event_cb;
    panic_event_cb_t              panic_event_cb;
    postdiag_event_cb_t           postdiag_event_cb;
    liveness_event_cb_t           liveness_event_cb;
    pciehealth_event_cb_t         pciehealth_event_cb;
    sdk::lib::catalog             *catalog;
    memory_threshold_event_cb_t   memory_threshold_event_cb;
    sysmon_memory_threshold_cfg_t *memory_threshold_cfg;
    uint16_t                      num_memory_threshold_cfg;
    uint32_t                      sysmon_poll_time;
} sysmon_cfg_t;

int sysmon_init(sysmon_cfg_t *sysmon_cfg);
void system_led(system_led_t led);

#endif    // __SYSMON_H__
