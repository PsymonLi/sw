// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __SDK_LINKMGR_HPP__
#define __SDK_LINKMGR_HPP__

#include "include/sdk/base.hpp"
#include "include/sdk/types.hpp"
#include "lib/catalog/catalog.hpp"
#include "platform/utils/mpartition.hpp"
#include "include/sdk/timestamp.hpp"
#include "lib/shmstore/shmstore.hpp"
#include "lib/utils/in_mem_fsm_logger.hpp"

#define MAX_MAC_STATS 89

namespace sdk {
namespace linkmgr {

using sdk::types::xcvr_state_t;
using sdk::types::xcvr_pid_t;
using sdk::utils::in_mem_fsm_logger;

#define LINK_SHM_SEGMENT    "link_shm_segment"
#define XCVR_SHM_SEGMENT    "xcvr_shm_segment"

typedef struct port_obj_meta_s {
    uint64_t num_ports;         ///< number of stored ports
    uint64_t port_size;         ///< size of each port
    uint64_t reserved1;         ///< reserved field
    uint64_t reserved2;         ///< reserved field
    uint64_t reserved3;         ///< reserved field
    uint8_t  obj[0];            ///< start offset of structs
} __PACK__ port_obj_meta_t;

typedef void (*port_log_fn_t)(sdk_trace_level_e trace_level,
                              const char *msg);

typedef void (*xcvr_event_notify_t)(xcvr_event_info_t *xcvr_event_info);
typedef void (*linkmgr_async_response_cb_t)(sdk_ret_t status, void *ctxt);

typedef struct linkmgr_cfg_s {
    platform_type_t     platform_type;
    sdk::lib::catalog   *catalog;
    void                *server_builder;
    const char          *cfg_path;
    port_event_notify_t port_event_cb;
    xcvr_event_notify_t xcvr_event_cb;
    port_log_fn_t       port_log_fn;
    bool                process_mode;
    port_admin_state_t  admin_state;    ///< default port admin state
    mpartition          *mempartition;  ///< memory partition context from HAL
    sdk::lib::shmstore  *backup_store;  ///< shared memory backup store instance
                                        ///< from HAL for backing up the states
    sdk::lib::shmstore  *restore_store; ///< shared memory backup store instance
                                        ///< from HAL for restoring the states
                                        ///< during bringup
    module_version_t    curr_version;   ///< curent version
    module_version_t    prev_version;   ///< previous version info passed from hal,
                                        ///< valid only in upgrade scenarios
    bool                use_shm;        ///< use shared memory for port allocation
} __PACK__ linkmgr_cfg_t;

extern linkmgr_cfg_t g_linkmgr_cfg;

typedef struct port_args_s {
    void                  *port_p;                    // SDK returned port context
    uint32_t              port_num;                   // uplink port number
    port_type_t           port_type;                  // port type
    port_speed_t          port_speed;                 // port speed
    port_admin_state_t    admin_state;                // admin state of the port
    port_admin_state_t    user_admin_state;           // user configured admin state of the port
    port_oper_status_t    oper_status;                // oper status of the port
    port_fec_type_t       fec_type;                   // oper FEC type
    port_fec_type_t       user_fec_type;              // configured FEC type
    port_fec_type_t       derived_fec_type;           // fec type based on AN or user config
    bool                  toggle_fec_mode;            // toggle FEC modes during fixed speed bringup
    port_pause_type_t     pause;                      // mac pause
    bool                  tx_pause_enable;            // mac tx pause enable
    bool                  rx_pause_enable;            // mac rx pause enable
    cable_type_t          cable_type;                 // CU/Fiber cable type
    bool                  auto_neg_cfg;               // user AutoNeg config
    bool                  auto_neg_enable;            // Enable AutoNeg
    bool                  toggle_neg_mode;            // toggle between auto_neg and force modes
    bool                  mac_stats_reset;            // mac stats reset
    uint32_t              mac_id;                     // mac id associated with the port
    uint32_t              mac_ch;                     // mac channel associated with port
    uint32_t              num_lanes_cfg;              // user configured number of lanes for the port
    uint32_t              num_lanes;                  // number of lanes for the port
    uint32_t              debounce_time;              // Debounce time in ms
    uint32_t              mtu;                        // mtu
    uint64_t              *stats_data;                // MAC stats info
    xcvr_event_info_t     xcvr_event_info;            // transceiver info
    port_an_args_t        *port_an_args;              // an cfg based on xcvr
    port_link_sm_t        link_sm;                    // internal port state machine
    port_loopback_mode_t  loopback_mode;              // port loopback mode - MAC/PHY
    uint32_t              num_link_down;              // number of link down events
    char                  last_down_timestamp[TIME_STR_SIZE]; // time at which link last went down
    timespec_t            bringup_duration;           // time taken for last link bringup
    uint32_t              breakout_modes;             // supported breakout modes
    uint8_t               mac_addr[6];                // MAC addr of port
    uint32_t              sbus_addr[MAX_PORT_LANES];  // set the sbus addr for each lane
    in_mem_fsm_logger     *sm_logger;                 // state machine transition Logger
} __PACK__ port_args_t;

sdk_ret_t linkmgr_init(linkmgr_cfg_t *cfg);
void *port_create(port_args_t *port_args);

/// \brief  returns the size of the linkmgr shm store
/// \return size of the linkmgr shmstore to be allocated
size_t linkmgr_shm_size(void);

/// \brief  enable port operations after upgrade
/// \return SDK_RET_OK on success, failure status code on error
sdk_ret_t port_upgrade_switchover(void);

sdk_ret_t port_update(void *port_p, port_args_t *port_args);
sdk_ret_t port_delete(void *port_p);
sdk_ret_t port_get(void *port_p, port_args_t *port_args);
sdk_ret_t port_stats_reset(void *port_p);
port_type_t port_type(void *port_p);
void linkmgr_set_link_poll_enable(bool enable);
sdk_ret_t port_args_set_by_xcvr_state(port_args_t *port_args);
sdk_ret_t port_update_xcvr_event(
            void *port_p, xcvr_event_info_t *xcvr_event_info);
void port_set_leds(uint32_t port_num, port_event_t event);
sdk_ret_t start_aacs_server(int port);
void stop_aacs_server(void);
port_admin_state_t port_default_admin_state(void);
void linkmgr_threads_stop(void);
void linkmgr_threads_wait(void);
void port_store_user_config(port_args_t *port_args);
sdk_ret_t port_quiesce(void *port_p, linkmgr_async_response_cb_t response_cb,
                       void *reponse_ctxt);

/// \brief     invoke quiesce on all ports
/// \param[in] response_cb callback to be invoked after quiescing
/// \param[in] response_ctxt callback context
/// \return    SDK_RET_OK on success, failure status code on error
sdk_ret_t port_quiesce_all(linkmgr_async_response_cb_t response_cb,
                           void *response_ctxt);
sdk_ret_t linkmgr_threads_suspend(void);
sdk_ret_t linkmgr_threads_resume(void);
bool linkmgr_threads_suspended(void);
bool linkmgr_threads_resumed(void);
bool linkmgr_threads_ready(void);

/// \brief  returns the number of uplinks with physical link up
/// \return number of uplinks with physical link up
uint16_t num_uplinks_link_up(void);

// \@brief     get the stats base address
// \@param[in] ifindex ifindex of the port
// \@return    on success, returns stats base address for port,
//             else INVALID_MEM_ADDRESS
mem_addr_t port_stats_addr(uint32_t ifindex);

static inline void
port_args_init (port_args_t *args)
{
    memset(args, 0, sizeof(port_args_t));
}

#define SDK_THREAD_ID_LINKMGR_MIN SDK_IPC_ID_LINKMGR_AACS_SERVER
#define SDK_THREAD_ID_LINKMGR_MAX SDK_IPC_ID_LINKMGR_CTRL
#define SDK_THREAD_ID_LINKMGR_CTRL SDK_IPC_ID_LINKMGR_CTRL


}    // namespace linkmgr
}    // namespace sdk

using sdk::linkmgr::linkmgr_cfg_t;
using sdk::linkmgr::port_args_t;

#endif    // __SDK_LINKMGR_HPP__
