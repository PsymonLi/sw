//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#ifndef __HII_IPC_HPP__
#define __HII_IPC_HPP__
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"

namespace hal {
namespace core {

typedef enum hii_update_type {
    HII_UPDATE_UID_LED = 0,
} hii_event_type_t;

typedef struct {
    uint8_t  type;
    union {
        bool uid_led_on;
    };
} hii_event_info_t;

}   // namespace core
}   // namespace hal

#endif      // __HII_IPC_HPP__
