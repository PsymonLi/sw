// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

#ifndef __PENVISOR_H__
#define __PENVISOR_H__

#include <stdint.h>

#define PENVISOR_PATH "/share/.penvisor"

typedef enum penvisor_instance_e_ {
    PENVISOR_INSTANCE_A = 0,
    PENVISOR_INSTANCE_B = 1,
} penvisor_instance_e;

typedef enum penvisor_action_e_ {
    PENVISOR_HEARTBEAT  = 0,
    PENVISOR_SYSRESET   = 1,
    PENVISOR_INSTLOAD   = 2,
    PENVISOR_INSTSWITCH = 3,
    PENVISOR_INSTUNLOAD = 4,
} penvisor_action_e;

typedef struct penvisor_command_t_ {
    penvisor_action_e   action;
    uint64_t            _reserved;
} __attribute__((packed)) penvisor_command_t;

#endif // __PENVISOR_H__
