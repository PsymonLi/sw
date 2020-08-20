// {C} Copyright 201 Pensando Systems Inc. All rights reserved.

#ifndef _SYSMOND_CB_H_
#define _SYSMOND_CB_H_

#include "gen/proto/port.grpc.pb.h"
#include "platform/sysmon/sysmon.hpp"
#include "platform/asicerror/interrupts.hpp"
#include "nic/sdk/platform/sensor/sensor.hpp"

using port::Port;

extern sdk::lib::catalog *g_catalog;
void event_cb_init(void);
void frequency_change_event_cb(uint32_t frequency);
void cattrip_event_cb(void);
void liveness_event_cb(void);
void power_event_cb(system_power_t *power);
void temp_event_cb(system_temperature_t *temperature,
                   xcvr_temperature_t *xcvrtemp,
                   sysmon_hbm_threshold_event_t hbm_event);
void memory_event_cb(system_memory_t *system_memoty);
void pciehealth_event_cb (sysmon_pciehealth_severity_t sev, const char *reason);
void panic_event_cb(void);
void postdiag_event_cb(void);
void intr_event_cb(const intr_reg_t *reg, const intr_field_t *field);
sdk_ret_t interrupt_notify(uint64_t reg_id, uint64_t field_id);
void create_tables(void);
void sysmon_grpc_init(void);
int port_get (uint32_t port_id, port::PortOperState *port_status);

#endif    // _SYSMOND_CB_H_
