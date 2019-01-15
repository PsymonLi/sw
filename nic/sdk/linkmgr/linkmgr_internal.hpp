// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __SDK_LINKMGR_INTERNAL_HPP__
#define __SDK_LINKMGR_INTERNAL_HPP__

#include <atomic>
#include "lib/logger/logger.hpp"
#include "lib/thread/thread.hpp"
#include "port.hpp"

namespace sdk {
namespace linkmgr {

#define MAX_PORT_LINKUP_RETRIES 100
#define XCVR_POLL_TIME          1000 // 1000 ms = 1 s
#define MAX_LOG_SIZE            1024

// max link training fail count until the busy loop is exited
#define MAX_LINK_TRAIN_FAIL_COUNT 10

extern linkmgr_cfg_t g_linkmgr_cfg;
extern char log_buf[];

#define SDK_LINKMGR_LOG(type, log_buf) { \
    if (g_linkmgr_cfg.port_log_fn) {                                      \
        g_linkmgr_cfg.port_log_fn(type, log_buf);                         \
    } else {                                                              \
        SDK_TRACE_DEBUG("%s", log_buf);                                   \
    }                                                                     \
}

#define SDK_PORT_SM_LOG(type, port, state) {                              \
    snprintf(log_buf, MAX_LOG_SIZE,                                       \
             "port: %d, MAC_ID: %d, MAC_CH: %d, state: %s",               \
             port->port_num(), port->mac_id_, port->mac_ch_, state);      \
    SDK_LINKMGR_LOG(type, log_buf);                                       \
}

#define SDK_PORT_SM_DEBUG(port, state)                                    \
    SDK_PORT_SM_LOG(sdk::lib::SDK_TRACE_LEVEL_DEBUG, port, state)

#define SDK_PORT_SM_TRACE(port, state)                                    \
    SDK_PORT_SM_LOG(sdk::lib::SDK_TRACE_LEVEL_DEBUG, port, state)

#define SDK_LINKMGR_TRACE_DEBUG(format, ...)  {                           \
    snprintf(log_buf, MAX_LOG_SIZE, format, ##__VA_ARGS__);               \
    SDK_LINKMGR_LOG(sdk::lib::SDK_TRACE_LEVEL_DEBUG, log_buf);            \
}

#define SDK_LINKMGR_TRACE_ERR(format, ...)  {                             \
    snprintf(log_buf, MAX_LOG_SIZE, format, ##__VA_ARGS__);               \
    SDK_LINKMGR_LOG(sdk::lib::SDK_TRACE_LEVEL_ERR, log_buf);              \
}

#define SDK_LINKMGR_TRACE_DEBUG_SIZE(logsize, format, ...)  {             \
    snprintf(log_buf, logsize, format, ##__VA_ARGS__);                    \
    SDK_LINKMGR_LOG(sdk::lib::SDK_TRACE_LEVEL_DEBUG, log_buf);            \
}

#define SDK_LINKMGR_TRACE_ERR_SIZE(logsize, format, ...)  {               \
    snprintf(log_buf, logsize, format, ##__VA_ARGS__);                    \
    SDK_LINKMGR_LOG(sdk::lib::SDK_TRACE_LEVEL_ERR, log_buf);              \
}

typedef enum sdk_timer_id_e {
    SDK_TIMER_ID_LINK_BRINGUP = 1000,   // TODO global unique across hal
    SDK_TIMER_ID_XCVR_POLL,
    SDK_TIMER_ID_LINK_POLL,
    SDK_TIMER_ID_LINK_DEBOUNCE
} sdk_timer_id_t;

sdk_ret_t
linkmgr_notify (uint8_t operation, linkmgr_entry_data_t *data,
                q_notify_mode_t mode);

sdk_ret_t
port_link_poll_timer_add(port *port);

sdk_ret_t
port_link_poll_timer_delete(port *port);

bool
is_linkmgr_ctrl_thread();

sdk::lib::thread *current_thread (void);

uint32_t glbl_mode_mgmt (mac_mode_t mac_mode);
uint32_t ch_mode_mgmt (mac_mode_t mac_mode, uint32_t ch);
uint32_t glbl_mode (mac_mode_t mac_mode);
uint32_t ch_mode (mac_mode_t mac_mode, uint32_t ch);

uint32_t       num_asic_ports(uint32_t asic);
uint32_t       sbus_addr_asic_port(uint32_t asic, uint32_t asic_port);
uint32_t       jtag_id(void);
uint8_t        num_sbus_rings(void);
uint8_t        sbm_clk_div   (void);
bool           aacs_server_en  (void);
bool           aacs_connect    (void);
uint32_t       aacs_server_port(void);
int            serdes_build_id (void);
int            serdes_rev_id (void);
std::string    serdes_fw_file (void);
std::string    aacs_server_ip  (void);
serdes_info_t* serdes_info_get(uint32_t sbus_addr,
                               uint32_t port_speed,
                               uint32_t cable_type);
}    // namespace linkmgr
}    // namespace sdk

#endif    // __SDK_LINKMGR_INTERNAL_HPP__
