//------------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//
// SDK types header file
//------------------------------------------------------------------------------

#ifndef __SDK_TYPES_HPP__
#define __SDK_TYPES_HPP__

namespace sdk {
namespace types {

typedef enum mac_mode_t {
    MAC_MODE_1x100g,
    MAC_MODE_1x40g,
    MAC_MODE_1x50g,
    MAC_MODE_2x40g,
    MAC_MODE_2x50g,
    MAC_MODE_1x50g_2x25g,
    MAC_MODE_2x25g_1x50g,
    MAC_MODE_4x25g,
    MAC_MODE_4x10g,
    MAC_MODE_4x1g
};

enum class port_speed_t {
    PORT_SPEED_NONE    = 0,
    PORT_SPEED_10G     = 1,
    PORT_SPEED_25G     = 2,
    PORT_SPEED_40G     = 3,
    PORT_SPEED_50G     = 4,
    PORT_SPEED_100G    = 5,
};

enum class port_type_t {
    PORT_TYPE_NONE    = 0,
    PORT_TYPE_ETH     = 1,
};

enum class platform_type_t {
    PLATFORM_TYPE_NONE = 0,
    PLATFORM_TYPE_SIM  = 1,
    PLATFORM_TYPE_HAPS = 2,
    PLATFORM_TYPE_HW   = 3,
    PLATFORM_TYPE_MOCK = 4,
};

enum class port_admin_state_t {
    PORT_ADMIN_STATE_NONE    = 0,
    PORT_ADMIN_STATE_UP      = 1,
    PORT_ADMIN_STATE_DOWN    = 2,
};

enum class port_oper_status_t {
    PORT_OPER_STATUS_NONE = 0,
    PORT_OPER_STATUS_UP   = 1,
    PORT_OPER_STATUS_DOWN = 2,
};

enum class port_fec_type_t {
    PORT_FEC_TYPE_NONE,  // Disable FEC
    PORT_FEC_TYPE_FC,    // Enable FireCode FEC
    PORT_FEC_TYPE_RS,    // Enable ReedSolomon FEC
};

}    // namespace types
}    // namespace sdk

using sdk::types::port_speed_t;
using sdk::types::port_type_t;
using sdk::types::port_admin_state_t;
using sdk::types::port_oper_status_t;
using sdk::types::platform_type_t;
using sdk::types::port_fec_type_t;
using sdk::types::mac_mode_t;

#define MAC_MODE_1x100g mac_mode_t::MAC_MODE_1x100g
#define MAC_MODE_1x40g mac_mode_t::MAC_MODE_1x40g
#define MAC_MODE_1x50g mac_mode_t::MAC_MODE_1x50g
#define MAC_MODE_2x40g mac_mode_t::MAC_MODE_2x40g
#define MAC_MODE_2x50g mac_mode_t::MAC_MODE_2x50g
#define MAC_MODE_1x50g_2x25g mac_mode_t::MAC_MODE_1x50g_2x25g
#define MAC_MODE_2x25g_1x50g mac_mode_t::MAC_MODE_2x25g_1x50g
#define MAC_MODE_4x25g mac_mode_t::MAC_MODE_4x25g
#define MAC_MODE_4x10g mac_mode_t::MAC_MODE_4x10g
#define MAC_MODE_4x1g mac_mode_t::MAC_MODE_4x1g

#endif    // __SDK_TYPES_HPP__

