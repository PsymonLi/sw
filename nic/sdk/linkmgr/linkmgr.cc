// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include <boost/interprocess/managed_shared_memory.hpp>
#include "asic/asic.hpp"
#include "platform/drivers/xcvr.hpp"
#include "port_serdes.hpp"
#include "lib/pal/pal.hpp"
#include "platform/pal/include/pal.h"
#include "lib/thread/thread.hpp"
#include "lib/event_thread/event_thread.hpp"
#include "lib/ipc/ipc.hpp"
#include "linkmgr.hpp"
#include "linkmgr_state.hpp"
#include "linkmgr_internal.hpp"
#include "port.hpp"
#include "include/sdk/port_utils.hpp"
#include "lib/utils/utils.hpp"
#include "lib/shmmgr/shmmgr.hpp"
#include "linkmgr_ctrl.hpp"
#include "platform/marvell/marvell.hpp"
#include "lib/runenv/runenv.h"

using namespace sdk::event_thread;
using namespace sdk::ipc;
using namespace boost::interprocess;
using namespace sdk::utils;
using namespace sdk::lib;

#define SDK_THREAD_ID_LINKMGR_MIN SDK_IPC_ID_LINKMGR_AACS_SERVER
#define SDK_THREAD_ID_LINKMGR_MAX SDK_IPC_ID_LINKMGR_CTRL

namespace sdk {
namespace linkmgr {

bool hal_cfg = false;

// global log buffer
char log_buf[MAX_LOG_SIZE];

// global sdk-linkmgr state
linkmgr_state *g_linkmgr_state;

// global sdk-linkmgr config
linkmgr_cfg_t g_linkmgr_cfg;

void
linkmgr_set_link_poll_enable (bool enable)
{
    g_linkmgr_state->set_link_poll_en(enable);
}

uint32_t
glbl_mode_mgmt (mac_mode_t mac_mode)
{
    return g_linkmgr_cfg.catalog->glbl_mode_mgmt(mac_mode);
}

uint32_t
ch_mode_mgmt (mac_mode_t mac_mode, uint32_t ch)
{
    return g_linkmgr_cfg.catalog->ch_mode_mgmt(mac_mode, ch);
}

uint32_t
glbl_mode (mac_mode_t mac_mode)
{
    return g_linkmgr_cfg.catalog->glbl_mode(mac_mode);
}

uint32_t
ch_mode (mac_mode_t mac_mode, uint32_t ch)
{
    return g_linkmgr_cfg.catalog->ch_mode(mac_mode, ch);
}

uint32_t
num_asic_ports(uint32_t asic)
{
    return g_linkmgr_cfg.catalog->num_asic_ports(asic);
}

uint32_t
sbus_addr_asic_port(uint32_t asic, uint32_t asic_port)
{
    return g_linkmgr_cfg.catalog->sbus_addr_asic_port(asic, asic_port);
}

uint32_t
jtag_id(void)
{
    return g_linkmgr_cfg.catalog->jtag_id();
}

uint8_t
sbm_clk_div(void)
{
    return g_linkmgr_cfg.catalog->sbm_clk_div();
}

uint8_t
num_sbus_rings(void)
{
    return g_linkmgr_cfg.catalog->num_sbus_rings();
}

uint32_t
logical_port_to_tm_port (uint32_t logical_port)
{
    return g_linkmgr_cfg.catalog->logical_port_to_tm_port(logical_port);
}

bool
aacs_server_en(void)
{
    uint8_t en = g_linkmgr_cfg.catalog->aacs_server_en();
    if (en == 1) {
        return true;
    }

    return false;
}

bool
aacs_connect(void)
{
    uint8_t connect = g_linkmgr_cfg.catalog->aacs_connect();
    if (connect == 1) {
        return true;
    }

    return false;
}

uint32_t
aacs_server_port(void)
{
    return g_linkmgr_cfg.catalog->aacs_server_port();
}

std::string
aacs_server_ip(void)
{
    return g_linkmgr_cfg.catalog->aacs_server_ip();
}

serdes_info_t*
serdes_info_get(uint32_t sbus_addr,
                uint32_t port_speed,
                uint32_t cable_type)
{
    return g_linkmgr_cfg.catalog->serdes_info_get(sbus_addr,
                                               port_speed,
                                               cable_type);
}

int
serdes_build_id (void)
{
    return g_linkmgr_cfg.catalog->serdes_build_id();
}

int
serdes_rev_id (void)
{
    return g_linkmgr_cfg.catalog->serdes_rev_id();
}

std::string
serdes_fw_file (void)
{
    return g_linkmgr_cfg.catalog->serdes_fw_file();
}

bool
is_linkmgr_ctrl_thread (void)
{
    bool is_ctrl_thread;

    sdk::lib::thread *curr_thread = sdk::lib::thread::current_thread();
    sdk::lib::thread *ctrl_thread =
        sdk::lib::thread::find(SDK_IPC_ID_LINKMGR_CTRL);

    // TODO assert once the thread store is fixed
    if (curr_thread == NULL) {
        is_ctrl_thread = false;
        goto end;
    }

    // if curr_thread/ctrl_thread is NULL, then init has failed or not invoked
    if (ctrl_thread == NULL) {
        is_ctrl_thread = false;
        SDK_ASSERT_GOTO(0, end);
    }

    if (curr_thread->thread_id() == ctrl_thread->thread_id()) {
        is_ctrl_thread = true;
        goto end;
    }

    is_ctrl_thread = false;
end:
    // SDK_TRACE_DEBUG("ctrl thread %s", is_ctrl_thread == true? "true" : "false");
    return is_ctrl_thread;
}

bool
is_linkmgr_ctrl_thread_ready (void)
{
    sdk::lib::thread *curr_thread =
        sdk::lib::thread::find(SDK_IPC_ID_LINKMGR_CTRL);

    return (curr_thread && curr_thread->ready());
}

static void
port_rsp_handler (ipc_msg_ptr msg, const void *request_cookie)
{
    linkmgr_entry_data_t *data = (linkmgr_entry_data_t *)msg->data();

    if (msg->length() && data->response_cb) {
        SDK_TRACE_DEBUG("Responding status %u", data->status);
        data->response_cb(data->status, data->response_ctxt);
    }
}

//------------------------------------------------------------------------------
// linkmgr thread notification by other threads
//------------------------------------------------------------------------------
static sdk_ret_t
linkmgr_notify (uint8_t operation, linkmgr_entry_data_t *data)
{
    // notify the ctrl thread about the config pending
    switch (operation) {
        case LINKMGR_OPERATION_PORT_ENABLE:
        case LINKMGR_OPERATION_PORT_DISABLE:
            SDK_ATOMIC_STORE_BOOL(&hal_cfg, true);
            break;

        default:
            break;
    }

    request(SDK_IPC_ID_LINKMGR_CTRL, operation, data,
            sizeof(linkmgr_entry_data_t), port_rsp_handler, NULL);
    return SDK_RET_OK;
}

static sdk_ret_t
thread_init (linkmgr_cfg_t *cfg)
{
    int thread_id = 0;
    sdk::event_thread::event_thread *new_thread;

    // init the control thread
    thread_id = SDK_IPC_ID_LINKMGR_CTRL;
    new_thread =
        sdk::event_thread::event_thread::factory(
            "linkmgr-ctrl", thread_id,
            sdk::lib::THREAD_ROLE_CONTROL,
            0x0,    // use all control cores
            linkmgr_event_thread_init,
            linkmgr_event_thread_exit,
            linkmgr_event_handler,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            true);
    SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                            "linkmgr-ctrl thread create failure");
    new_thread->start(new_thread);
    return SDK_RET_OK;
}

void
linkmgr_threads_stop (void)
{
    int thread_id;
    sdk::lib::thread *thread;

    for (thread_id = SDK_THREAD_ID_LINKMGR_MIN;
            thread_id <= SDK_THREAD_ID_LINKMGR_MAX; thread_id++) {
        thread = sdk::lib::thread::find(thread_id);
        if (thread != NULL) {
            // stop the thread
            SDK_TRACE_DEBUG("Stopping thread %s", thread->name());
            thread->stop();
        }
    }
}

void
linkmgr_threads_wait (void)
{
    int thread_id;
    sdk::lib::thread *thread;

    for (thread_id = SDK_THREAD_ID_LINKMGR_MIN;
            thread_id <= SDK_THREAD_ID_LINKMGR_MAX; thread_id++) {
        thread = sdk::lib::thread::find(thread_id);
        if (thread != NULL) {
            SDK_TRACE_DEBUG("Waiting thread %s to exit", thread->name());
            thread->wait();
            // free the allocated thread
            sdk::lib::thread::destroy(thread);
        }
    }
}

sdk_ret_t
linkmgr_threads_suspend (void)
{
    int thread_id;
    sdk_ret_t ret;
    sdk::lib::thread *thread;

    for (thread_id = SDK_THREAD_ID_LINKMGR_MIN;
            thread_id <= SDK_THREAD_ID_LINKMGR_MAX; thread_id++) {
        thread = sdk::lib::thread::find(thread_id);
        if (thread != NULL) {
            ret = thread->suspend_req(NULL);
            if (ret != SDK_RET_OK) {
                return ret;
            }
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
linkmgr_threads_resume (void)
{
    int thread_id;
    sdk_ret_t ret;
    sdk::lib::thread *thread;

    for (thread_id = SDK_THREAD_ID_LINKMGR_MIN;
            thread_id <= SDK_THREAD_ID_LINKMGR_MAX; thread_id++) {
        thread = sdk::lib::thread::find(thread_id);
        if (thread != NULL) {
            ret = thread->resume_req();
            if (ret != SDK_RET_OK) {
                return ret;
            }
        }
    }
    return SDK_RET_OK;
}

bool
linkmgr_threads_ready (void)
{
    int thread_id;
    bool ready;
    sdk::lib::thread *thread;

    for (thread_id = SDK_THREAD_ID_LINKMGR_MIN;
            thread_id <= SDK_THREAD_ID_LINKMGR_MAX; thread_id++) {
        thread = sdk::lib::thread::find(thread_id);
        if (thread != NULL) {
            ready = thread->ready();
            SDK_TRACE_DEBUG("Linkmgr thread %s ready %u",
                            thread->name(), ready);
            if (!ready) {
                return false;
            }
        }
    }
    return true;
}

// check for thread suspended or not
static bool
linkmgr_threads_suspended_ (bool status)
{
    int thread_id;
    bool suspended;
    bool rv = true;
    sdk::lib::thread *thread;

    for (thread_id = SDK_THREAD_ID_LINKMGR_MIN;
            thread_id <= SDK_THREAD_ID_LINKMGR_MAX; thread_id++) {
        thread = sdk::lib::thread::find(thread_id);
        if (thread != NULL) {
            suspended = thread->suspended();
            if (status) {
                SDK_TRACE_DEBUG("Linkmgr thread %s suspended %u",
                                thread->name(), suspended);
            } else {
                SDK_TRACE_DEBUG("Linkmgr thread %s resumed %u",
                                thread->name(), !suspended);
            }
            if ((status ? suspended : !suspended) == false) {
                rv = false;
            }
        }
    }
    return rv;
}

bool
linkmgr_threads_suspended (void)
{
    return linkmgr_threads_suspended_(true);
}

bool
linkmgr_threads_resumed (void)
{
    return linkmgr_threads_suspended_(false);
}

static sdk_ret_t
xcvr_shm_init (linkmgr_cfg_t *cfg)
{
    void *backup_mem = NULL;
    void *mem = NULL;
    size_t size;

    // transceiver info size
    size = sdk::platform::xcvr_mem_size();

    // If shared memory segment exists for transceivers:
    //     Find the memory segment in restore store
    //     If the number of ports in segment != catalog->num_fp_ports,
    //         then return error
    //     If Version change, copy from restore store to backup store
    // Else:
    //     Create the segment in backup store
    //     Store number of entries, size of each entry in meta

    if (g_linkmgr_state->port_restore_state()) {
        mem = cfg->restore_store->open_segment(XCVR_SHM_SEGMENT);
        if (mem == NULL) {
            SDK_TRACE_ERR("Error opening shared mem segment for xcvr in "
                          "restore store, size %lu", size);
            return SDK_RET_OOM;
        }
    } else {
        SDK_TRACE_DEBUG("Allocating xcvr segment size %lu", size);
        mem = cfg->backup_store->create_segment(XCVR_SHM_SEGMENT,
                                                        size);
        if (mem == NULL) {
            SDK_TRACE_ERR("Error creating shared mem segment for xcvr in "
                          "backup store, size %lu", size);
            return SDK_RET_OOM;
        }
    }
    sdk::platform::xcvr_init(cfg->xcvr_event_cb, mem, backup_mem);
    return SDK_RET_OK;
}

static uint64_t
link_shm_size (void)
{
    return sizeof(port_obj_meta_t) +
           (sizeof(port) * g_linkmgr_cfg.catalog->num_fp_ports());
}

static sdk_ret_t
link_shm_init (linkmgr_cfg_t *cfg)
{
    std::size_t size;
    port_obj_meta_t *port_obj_meta;

    // port segment size
    size = link_shm_size();

    // If shared memory segment exists for ports:
    //     Find the segment in restore store
    //     If the number of ports in segment != catalog->num_fp_ports,
    //         then return error
    //     If Version change, copy from restore store to backup store
    // Else:
    //     Create the segment in backup store
    //     Store number of entries, size of each entry in meta

    if (g_linkmgr_state->port_restore_state()) {
        g_linkmgr_state->set_mem(
            cfg->restore_store->open_segment(LINK_SHM_SEGMENT));
        if (g_linkmgr_state->mem() == NULL) {
            SDK_TRACE_ERR("Error opening shared mem segment for ports in "
                          "restore store, size %lu", size);
            return SDK_RET_OOM;
        }
        port_obj_meta = (port_obj_meta_t *)g_linkmgr_state->mem();
        SDK_TRACE_DEBUG("Found port num_ports %lu, size %lu",
                        port_obj_meta->num_ports, port_obj_meta->port_size);
        // fail the upgrade if the number of ports have changed
        if (port_obj_meta->num_ports !=
                cfg->catalog->num_fp_ports()) {
            SDK_TRACE_ERR("Number of ports mismtach, restore memory %lu, "
                          "catalog %u", port_obj_meta->num_ports,
                          cfg->catalog->num_fp_ports());
            return SDK_RET_ERR;
        }
    } else {
        SDK_TRACE_DEBUG("Allocating port segment size %lu", size);
        g_linkmgr_state->set_mem(
            cfg->backup_store->create_segment(LINK_SHM_SEGMENT, size));
        if (g_linkmgr_state->mem() == NULL) {
            SDK_TRACE_ERR("Error creating shared mem segment for ports in "
                          "backup store, size %lu", size);
            return SDK_RET_OOM;
        }
        port_obj_meta = (port_obj_meta_t *)g_linkmgr_state->mem();
        port_obj_meta->num_ports = cfg->catalog->num_fp_ports();
        port_obj_meta->port_size = sizeof(port);
        SDK_TRACE_DEBUG("Setting port num_ports %lu, size %lu",
                        port_obj_meta->num_ports, port_obj_meta->port_size);
    }
    return SDK_RET_OK;
}

sdk_ret_t
linkmgr_shm_init (linkmgr_cfg_t *cfg)
{
    sdk_ret_t ret;

    // If hitless init:
    //     restore_store != NULL
    //     If version match:
    //         backup_store = NULL
    //     Else:
    //         backup_store != NULL
    // Else:
    //     restore_store = NULL
    //     backup_store != NULL

    // if restore_store exists, then restore state from memory
    if (cfg->restore_store != NULL) {
        SDK_TRACE_DEBUG("port upgrade restoring port state");
        g_linkmgr_state->set_port_restore_state(true);
    }

    // shm init for ports
    ret = link_shm_init(cfg);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Link shm init failed, err %u", ret);
        return ret;
    }

    // shm init for transceivers
    ret = xcvr_shm_init(cfg);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Transceiver shm init failed, err %u", ret);
    }
    return ret;
}

static sdk_ret_t
xcvr_init_ (linkmgr_cfg_t *cfg)
{
    void *mem;
    size_t size;

    // transceiver info size
    size = sdk::platform::xcvr_mem_size();

    mem = SDK_CALLOC(SDK_MEM_ALLOC_ID_LINKMGR, size);
    if (mem == NULL) {
        SDK_TRACE_ERR("Failed to allocate memory for transceiver");
        return SDK_RET_OOM;
    }
    return sdk::platform::xcvr_init(cfg->xcvr_event_cb, mem, NULL);
}

sdk_ret_t
linkmgr_init (linkmgr_cfg_t *cfg)
{
    sdk_ret_t ret;

    g_linkmgr_cfg = *cfg;

    g_linkmgr_state = linkmgr_state::factory();
    if (NULL == g_linkmgr_state) {
        SDK_TRACE_ERR("linkmgr init failed");
        return SDK_RET_ERR;
    }

    // initialize the port mac and serdes functions
    port::port_init(cfg);

    if (cfg->use_shm) {
        // allocate port and transceiver struct in shared memory
        ret = linkmgr_shm_init(cfg);
    } else {
        xcvr_init_(cfg);
    }
    if ((ret = thread_init(cfg)) != SDK_RET_OK) {
        SDK_TRACE_ERR("linkmgr thread init failed");
        return ret;
    }
    return ret;
}

size_t
linkmgr_shm_size (void)
{
    size_t size;

    // size of the linkmgr shared memory
    size = link_shm_size() + sdk::platform::xcvr_mem_size();

    // segment find fails if we allocated exact memory
    size += size/2;
    return size;
}

//-----------------------------------------------------------------------------
// Validate speed against number of lanes
//-----------------------------------------------------------------------------
static bool
validate_speed_lanes (port_speed_t speed, uint32_t num_lanes)
{
    bool valid = true;

    // speed = NONE implies no update
    switch (num_lanes) {
    case 4:
        switch (speed) {
            case port_speed_t::PORT_SPEED_100G:
            case port_speed_t::PORT_SPEED_40G:
            case port_speed_t::PORT_SPEED_NONE:
                break;

            default:
                valid = false;
                break;
        }
        break;

    case 2:
        switch (speed) {
            case port_speed_t::PORT_SPEED_50G:
            case port_speed_t::PORT_SPEED_NONE:
                break;

            default:
                valid = false;
                break;
        }
        break;

    case 1:
        switch (speed) {
            case port_speed_t::PORT_SPEED_25G:
            case port_speed_t::PORT_SPEED_10G:
            case port_speed_t::PORT_SPEED_1G:
            case port_speed_t::PORT_SPEED_NONE:
                break;

            default:
                valid = false;
                break;
        }
        break;

    default:
        valid = false;
        break;
    }

    if (valid == false) {
        SDK_TRACE_ERR("Invalid speed and lanes config,"
                      " num_lanes %u, speed %u",
                      num_lanes, static_cast<uint32_t>(speed));
    }

    return valid;
}

void
port_set_leds (uint32_t port_num, port_event_t event)
{
    int phy_port = sdk::lib::catalog::logical_port_to_phy_port(port_num);

    if (phy_port == -1) {
        return;
    }

    switch (event) {
    case port_event_t::PORT_EVENT_LINK_UP:
        sdk::lib::pal_qsfp_set_led(phy_port,
                                   pal_led_color_t::LED_COLOR_GREEN);
        break;

    case port_event_t::PORT_EVENT_LINK_DOWN:
        sdk::lib::pal_qsfp_set_led(phy_port,
                                   pal_led_color_t::LED_COLOR_NONE);
        break;

    default:
        break;
    }
}

sdk_ret_t
port_update_xcvr_event (void *pd_p, xcvr_event_info_t *xcvr_event_info)
{
    sdk_ret_t   sdk_ret   = SDK_RET_OK;
    port_args_t port_args = { 0 };

    sdk::linkmgr::port_args_init(&port_args);

    sdk_ret = port_get(pd_p, &port_args);
    if (sdk_ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to get pd for port, err %u", sdk_ret);
        return sdk_ret;
    }

    int phy_port =
        sdk::lib::catalog::logical_port_to_phy_port(port_args.port_num);

    SDK_TRACE_DEBUG("port %u, phy_port %u, xcvr_event_port %u, "
                    "xcvr_state %u, user_admin %u, admin %u, "
                    "speed %u, AN_cfg %u, AN_enable %u, num_lanes_cfg %u",
                    port_args.port_num, phy_port, xcvr_event_info->phy_port,
                    static_cast<uint32_t>(xcvr_event_info->state),
                    static_cast<uint32_t>(port_args.user_admin_state),
                    static_cast<uint32_t>(port_args.admin_state),
                    static_cast<uint32_t>(port_args.port_speed),
                    port_args.auto_neg_cfg,
                    port_args.auto_neg_enable,
                    port_args.num_lanes_cfg);

    if (phy_port == -1 || phy_port != (int)xcvr_event_info->phy_port) {
        return SDK_RET_OK;
    }

    if (xcvr_event_info->state == xcvr_state_t::XCVR_SPROM_READ) {
        // update cable type
        port_args.cable_type   = xcvr_event_info->cable_type;
        port_args.port_an_args = xcvr_event_info->port_an_args;

        SDK_TRACE_DEBUG("port %u, phy_port %u, "
                        "user_cap %u, fec_ability %u, fec_request %u",
                        port_args.port_num,
                        phy_port,
                        port_args.port_an_args->user_cap,
                        port_args.port_an_args->fec_ability,
                        port_args.port_an_args->fec_request);

        // set admin_state based on user configured value
        if (port_args.user_admin_state ==
                port_admin_state_t::PORT_ADMIN_STATE_UP) {
            port_args.admin_state = port_admin_state_t::PORT_ADMIN_STATE_UP;
        }

        // set AN based on user configured value
        port_args.auto_neg_enable = port_args.auto_neg_cfg;

        // set num_lanes based on user configured value
        port_args.num_lanes = port_args.num_lanes_cfg;

        // update port_args based on the xcvr state
        port_args_set_by_xcvr_state(&port_args);

        sdk_ret = port_update(pd_p, &port_args);

        if (sdk_ret != SDK_RET_OK) {
            SDK_TRACE_ERR("Failed to update for port %u, err %u",
                          port_args.port_num, sdk_ret);
        }
    } else if (xcvr_event_info->state == xcvr_state_t::XCVR_REMOVED) {
        if (port_args.user_admin_state ==
                port_admin_state_t::PORT_ADMIN_STATE_UP) {
            port_args.admin_state = port_admin_state_t::PORT_ADMIN_STATE_DOWN;

            sdk_ret = port_update(pd_p, &port_args);

            if (sdk_ret != SDK_RET_OK) {
                SDK_TRACE_ERR("Failed to update for port %u, err %u",
                              port_args.port_num, sdk_ret);
            }
        }
    }

    return SDK_RET_OK;
}

/// \brief      derive port_speed, fec_type and num_lanes based on transceiver
//  \param[in]  phy_port port representing transceiver
//  \param[out] port_speed derived based on transceiver
//  \param[out] fec_type   derived based on port_speed
//  \param[out] num_lanes  derived based on port_speed
//  \param[out] toggle_fec toggle FEC modes during fixed speed bringup
//  \return     #SDK_RET_OK on success, failure status code on error
static sdk_ret_t
port_args_set_by_xcvr_state_ (int phy_port, port_speed_t *port_speed,
                              port_fec_type_t *fec_type, uint32_t *num_lanes,
                              bool *toggle_fec_mode)
{
    *port_speed = sdk::platform::cable_speed(phy_port-1);

    // TODO FEC assumed based on cable speed
    switch (*port_speed) {
        case port_speed_t::PORT_SPEED_100G:
            *fec_type = port_fec_type_t::PORT_FEC_TYPE_RS;
            *num_lanes = 4;
            break;

        case port_speed_t::PORT_SPEED_50G:
            *fec_type = port_fec_type_t::PORT_FEC_TYPE_RS;
            *num_lanes = 2;
            break;

        case port_speed_t::PORT_SPEED_40G:
            *fec_type = port_fec_type_t::PORT_FEC_TYPE_NONE;
            *num_lanes = 4;
            break;

        case port_speed_t::PORT_SPEED_25G:
            *toggle_fec_mode = true;
            *fec_type = sdk::platform::xcvr_fec_type(phy_port-1);
            *num_lanes = 1;
            break;

        case port_speed_t::PORT_SPEED_10G:
            *fec_type = port_fec_type_t::PORT_FEC_TYPE_NONE;
            *num_lanes = 1;
            break;

        default:
            *fec_type = port_fec_type_t::PORT_FEC_TYPE_NONE;
            SDK_TRACE_ERR("Invalid port speed %u for phy_port %d",
                          static_cast<uint32_t>(*port_speed), phy_port);
            return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

//-----------------------------------------------------------------------------
// set port args given transceiver state
//-----------------------------------------------------------------------------
sdk_ret_t
port_args_set_by_xcvr_state (port_args_t *port_args)
{
    // if xcvr is inserted:
    //     set port params based on cable
    // else:
    //     set admin_state as ADMIN_DOWN
    //     (only admin_state is down. user_admin_state as per request msg)

    int phy_port =
        sdk::lib::catalog::logical_port_to_phy_port(port_args->port_num);

    port::phy_port_mac_addr(phy_port, port_args->mac_addr);

    // default cable type is CU
    port_args->cable_type = cable_type_t::CABLE_TYPE_CU;

    if (phy_port != -1) {
        if (sdk::platform::xcvr_valid(phy_port-1) == true) {
            port_args->cable_type = sdk::platform::cable_type(phy_port-1);

            // for older boards, cable type returned is NONE.
            // Set it to CU
            if (port_args->cable_type == cable_type_t::CABLE_TYPE_NONE) {
                port_args->cable_type = cable_type_t::CABLE_TYPE_CU;
            }
            port_args->port_an_args =
                            sdk::platform::xcvr_get_an_args(phy_port - 1);
           SDK_TRACE_DEBUG("port  %u, phy_port  %u, user_cap  %u "
                           "fec ability  %u, fec request  %u, "
                           "cable_type  %u", port_args->port_num,
                           phy_port, port_args->port_an_args->user_cap,
                           port_args->port_an_args->fec_ability,
                           port_args->port_an_args->fec_request,
                           port_args->cable_type);

            port_args->toggle_neg_mode = false;
            port_args->toggle_fec_mode = false;


            // if AN=true and
            // (1) fiber cable is inserted
            //     auto_neg_enable = false
            //     port_speed based on transceiver
            //     derived_fec_type and num_lanes based on port_speed
            // (2) SFP is inserted
            //     port_speed based on transceiver
            //     derived_fec_type and num_lanes based on port_speed
            //     (2.1) CU SFP is inserted
            //           Enable toggle mode - b/w AN and Fixed
            // TODO: to extend to QSFP also after initial validations on SFPs
            if (port_args->auto_neg_enable == true) {
                if (port_args->cable_type == cable_type_t::CABLE_TYPE_FIBER) {
                    port_args->auto_neg_enable = false;
                    port_args_set_by_xcvr_state_(
                                          phy_port, &port_args->port_speed,
                                          &port_args->derived_fec_type,
                                          &port_args->num_lanes,
                                          &port_args->toggle_fec_mode);
                }
                if (sdk::platform::xcvr_type(phy_port-1) ==
                                                  xcvr_type_t::XCVR_TYPE_SFP) {
                    port_args_set_by_xcvr_state_(
                                          phy_port, &port_args->port_speed,
                                          &port_args->derived_fec_type,
                                          &port_args->num_lanes,
                                          &port_args->toggle_fec_mode);
                    if (port_args->cable_type == cable_type_t::CABLE_TYPE_CU) {
                        // for these types, toggle between force/an till link-up
                        // TOR/peer could have do either forced/an
                        port_args->toggle_neg_mode = true;
                        SDK_TRACE_DEBUG("port %u CU-SFP toggle_neg_mode "
                                        "enabled", port_args->port_num);
                    }
                }
            }
            SDK_TRACE_DEBUG("port %u speed %u num_lanes %u, auto_neg_enable %u "
                            "toggle_an %u derived_fec_type %u, toggle_fec %u",
                            port_args->port_num,
                            static_cast<uint32_t>(port_args->port_speed),
                            port_args->num_lanes, port_args->auto_neg_enable,
                            port_args->toggle_neg_mode,
                            static_cast<uint32_t>(port_args->derived_fec_type),
                            port_args->toggle_fec_mode);

        } else {
            port_args->admin_state = port_admin_state_t::PORT_ADMIN_STATE_DOWN;
        }
    }
    return SDK_RET_OK;
}

//-----------------------------------------------------------------------------
// Validate port create config
//-----------------------------------------------------------------------------
static bool
validate_port_create (port_args_t *args)
{
    if (args->port_type == port_type_t::PORT_TYPE_NONE) {
        SDK_TRACE_ERR("Invalid port type for port %d", args->port_num);
        return false;
    }

    if (args->port_speed == port_speed_t::PORT_SPEED_NONE) {
        SDK_TRACE_ERR("Invalid port speed for port %d", args->port_num);
        return false;
    }

    // validate speed against num_lanes only if AN=false
    if (args->auto_neg_enable == false) {
        return validate_speed_lanes(args->port_speed, args->num_lanes);
    }

    return true;
}

static void
port_init_num_lanes (port_args_t *args)
{
    uint32_t asic_inst = 0;

    // capri is NRZ serdes. Force num_lanes based on speed
    // to avoid upper layers setting it
    if (g_linkmgr_cfg.catalog->asic_type(asic_inst) ==
                               asic_type_t::SDK_ASIC_TYPE_CAPRI) {
        switch (args->port_speed) {
        case port_speed_t::PORT_SPEED_100G:
        case port_speed_t::PORT_SPEED_40G:
            args->num_lanes = 4;
            break;

        case port_speed_t::PORT_SPEED_50G:
            args->num_lanes = 2;
            break;

        case port_speed_t::PORT_SPEED_25G:
        case port_speed_t::PORT_SPEED_10G:
        case port_speed_t::PORT_SPEED_1G:
            args->num_lanes = 1;
            break;

        default:
            break;
        }
    }
}

static void
port_init_defaults (port_args_t *args)
{
    if (args->mtu == 0) {
        args->mtu = 9216;   // TODO define?
    }
}

// If current_thread is hal-control thread, invoke method directly
// Else trigger hal-control thread to invoke method
static sdk_ret_t
port_enable (port *port_p)
{
    sdk_ret_t ret;
    linkmgr_entry_data_t data = { 0 };

    // wait for linkmgr control thread to process port event
    while (!is_linkmgr_ctrl_thread_ready()) {
        pthread_yield();
    }
    data.ctxt  = port_p;
    ret = linkmgr_notify(LINKMGR_OPERATION_PORT_ENABLE, &data);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Error notifying control-thread for port enable");
    }
    return ret;
}

// If current_thread is hal-control thread, invoke method directly
// Else trigger hal-control thread to invoke method
static sdk_ret_t
port_disable (port *port_p)
{
    sdk_ret_t ret;
    linkmgr_entry_data_t data = { 0 };

    // wait for linkmgr control thread to process port event
    while (!is_linkmgr_ctrl_thread_ready()) {
        pthread_yield();
    }
    data.ctxt  = port_p;
    ret = linkmgr_notify(LINKMGR_OPERATION_PORT_DISABLE, &data);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Error notifying control-thread for port disable");
    }
    return ret;
}

// If current_thread is hal-control thread, invoke method directly
// Else trigger hal-control thread to invoke method
sdk_ret_t
port_quiesce (void *pd_p, linkmgr_async_response_cb_t response_cb,
              void *response_ctxt)
{
    sdk_ret_t ret;
    linkmgr_entry_data_t data = { 0 };
    port *port_p = (port *)pd_p;

    // wait for linkmgr control thread to process port event
    while (!is_linkmgr_ctrl_thread_ready()) {
        pthread_yield();
    }
    data.ctxt  = port_p;
    data.response_cb = response_cb;
    data.response_ctxt = response_ctxt;
    ret = linkmgr_notify(LINKMGR_OPERATION_PORT_QUIESCE, &data);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Error notifying control-thread for port quiesce");
    }
    return ret;
}

/// \brief     invoke quiesce on all ports
/// \param[in] response_cb callback to be invoked after quiescing
/// \param[in] response_ctxt callback context
/// \return    SDK_RET_OK on success, failure status code on error
sdk_ret_t
port_quiesce_all (linkmgr_async_response_cb_t response_cb,
                  void *response_ctxt)
{
    sdk_ret_t ret;
    linkmgr_entry_data_t data = { 0 };

    if (!g_linkmgr_cfg.use_shm) {
        SDK_TRACE_DEBUG("port quiesce all not supported for non-shm");
        return SDK_RET_ERR;
    }
    // wait for linkmgr control thread to process port event
    while (!is_linkmgr_ctrl_thread_ready()) {
        pthread_yield();
    }
    data.response_cb = response_cb;
    data.response_ctxt = response_ctxt;
    ret = linkmgr_notify(LINKMGR_OPERATION_PORT_QUIESCE_ALL, &data);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Error notifying control-thread for port quiesce all");
    }
    return ret;
}

sdk_ret_t
port_upgrade_switchover (void)
{
    sdk_ret_t ret;
    linkmgr_entry_data_t data = { 0 };

    if (!g_linkmgr_cfg.use_shm) {
        SDK_TRACE_DEBUG("port switchover not supported for non-shm");
        return SDK_RET_OK;
    }
    // wait for linkmgr control thread to process port event
    while (!is_linkmgr_ctrl_thread_ready()) {
        pthread_yield();
    }
    ret = linkmgr_notify(LINKMGR_OPERATION_PORT_SWITCHOVER, &data);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Error notifying control-thread for port switchover");
    }
    return ret;
}

static port *
shm_port_alloc (port_args_t *args)
{
    void *mem;
    if_index_t ifindex;
    uint32_t parent_port;
    port_obj_meta_t *port_obj_meta;

    SDK_TRACE_DEBUG(
        "Allocate %s in shared memory", eth_ifindex_to_str(
        sdk::lib::catalog::logical_port_to_ifindex(args->port_num)).c_str());

    ifindex = sdk::lib::catalog::logical_port_to_ifindex(args->port_num);
    parent_port = ETH_IFINDEX_TO_PARENT_PORT(ifindex);
    if (g_linkmgr_state->mem() == NULL) {
        return NULL;
    }
    port_obj_meta = (port_obj_meta_t *)g_linkmgr_state->mem();
    mem = port_obj_meta->obj +
          (port_obj_meta->port_size * (parent_port - 1));
    return new (mem) port();
}

//-----------------------------------------------------------------------------
// PD If Create
//-----------------------------------------------------------------------------
void *
port_create (port_args_t *args)
{
    sdk_ret_t ret;
    void *mem;
    port *port_p;
    in_mem_fsm_logger *sm_logger = NULL;

    port_init_num_lanes(args);
    if (validate_port_create (args) == false) {
        return NULL;
    }
    port_init_defaults(args);
    if (g_linkmgr_cfg.use_shm) {
        port_p = shm_port_alloc(args);
        if (port_p == NULL) {
            return NULL;
        }
    } else {
        mem = g_linkmgr_state->port_slab()->alloc();
        if (mem == NULL) {
            return NULL;
        }
        port_p = new (mem) port();
    }

    if (g_linkmgr_state->port_restore_state()) {
        g_linkmgr_state->set_port_p(args->port_num - 1, port_p);
        return port_p;
    }

    sm_logger = in_mem_fsm_logger::factory(PORT_IN_MEM_LOGGER_CAPACITY,
                                           sizeof(port_link_sm_t));
    SDK_ASSERT(sm_logger);
    port_p->set_sm_logger(sm_logger);
    // init the bringup and debounce timers
    port_p->timers_init();
    port_p->set_port_num(args->port_num);
    port_p->set_port_type(args->port_type);
    port_p->set_port_speed(args->port_speed);
    port_p->set_mac_id(g_linkmgr_cfg.catalog->mac_id(args->port_num, 0));
    port_p->set_mac_ch(g_linkmgr_cfg.catalog->mac_ch(args->port_num, 0));
    port_p->set_num_lanes(args->num_lanes);
    port_p->set_debounce_time(args->debounce_time);
    port_p->set_auto_neg_enable(args->auto_neg_enable);
    port_p->set_toggle_neg_mode(args->toggle_neg_mode);
    port_p->set_toggle_fec_mode(args->toggle_fec_mode);
    port_p->set_mtu(args->mtu);
    port_p->set_pause(args->pause);
    port_p->set_tx_pause_enable(args->tx_pause_enable);
    port_p->set_rx_pause_enable(args->rx_pause_enable);
    port_p->set_cable_type(args->cable_type);
    port_p->set_loopback_mode(args->loopback_mode);

    // store the user configured admin state
    port_p->set_user_admin_state(args->user_admin_state);

    // store the user configured AN
    port_p->set_auto_neg_cfg(args->auto_neg_cfg);

    // store the user configured num_lanes
    port_p->set_num_lanes_cfg(args->num_lanes_cfg);

    // store the user configure fec type
    port_p->set_user_fec_type(args->user_fec_type);

    port_p->set_derived_fec_type(args->derived_fec_type);

    if(args->port_type != port_type_t::PORT_TYPE_MGMT) {
        g_linkmgr_state->set_port_bmap_mask(g_linkmgr_state->port_bmap_mask() |
                                            (1 << (args->port_num - 1)));
    }

    // use the configured num_lanes for setting sbus_addr
    for (uint32_t i = 0; i < args->num_lanes_cfg; ++i) {
        port_p->sbus_addr_set(i, g_linkmgr_cfg.catalog->sbus_addr(
                              args->port_num, i));
    }

    // set the source mac addr for pause frames
    port_p->port_mac_set_pause_src_addr(args->mac_addr);

    if (args->port_an_args != NULL) {
        port_p->set_user_cap(args->port_an_args->user_cap);
        port_p->set_fec_ability(args->port_an_args->fec_ability);
        port_p->set_fec_request(args->port_an_args->fec_request);
    }

    ret = port::port_mac_stats_init(port_p);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("port %u mac stats init failed", args->port_num);
    }

    // disable a port to invoke soft reset
    ret = port_disable(port_p);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("port %u disable failed", args->port_num);
    }

    // if admin up is set, enable the port
    if (args->admin_state == port_admin_state_t::PORT_ADMIN_STATE_UP) {
        ret = port_enable(port_p);
        if (ret != SDK_RET_OK) {
            SDK_TRACE_ERR("port %u enable failed", args->port_num);
        }
    }
    return port_p;
}

//-----------------------------------------------------------------------------
// Validate port update config
//-----------------------------------------------------------------------------
static bool
validate_port_update (port *port_p, port_args_t *args)
{
    if (args->port_type != port_p->port_type()) {
        SDK_TRACE_ERR("port_type update not supported for port %d",
                        args->port_num);
        return false;
    }

    // TODO skip num_lanes check to support auto speed set for QSA
#if 0
    if (args->num_lanes != port_p->num_lanes()) {
        SDK_TRACE_ERR("num_lanes update not supported for port %d",
                        args->port_num);
        return false;
    }
#endif

    // validate speed against num_lanes only if AN=false
    if (args->auto_neg_enable == false) {
        if (args->num_lanes == port_p->num_lanes()) {
            return validate_speed_lanes(args->port_speed, args->num_lanes);
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// update given port
//-----------------------------------------------------------------------------
sdk_ret_t
port_update (void *pd_p, port_args_t *args)
{
    sdk_ret_t          ret           = SDK_RET_OK;
    bool               configured    = false;
    port               *port_p       = (port *)pd_p;
    port_admin_state_t prev_admin_st = port_p->admin_state();

    port_init_num_lanes(args);
    if (validate_port_update (port_p, args) == false) {
        // TODO return codes
        return SDK_RET_ERR;
    }

    // check if any properties have changed

    if (args->port_speed != port_speed_t::PORT_SPEED_NONE &&
        args->port_speed != port_p->port_speed()) {
        SDK_TRACE_DEBUG("speed updated, new %d, old %d",
                        static_cast<int>(args->port_speed),
                        static_cast<int>(port_p->port_speed()));

        port_p->port_deinit();
        port_p->set_port_speed(args->port_speed);
        configured = true;
    }

    if (args->admin_state != port_admin_state_t::PORT_ADMIN_STATE_NONE &&
        args->admin_state != port_p->admin_state()) {
        SDK_TRACE_DEBUG("admin_state updated, new %d, old %d",
                        static_cast<int>(args->admin_state),
                        static_cast<int>(port_p->admin_state()));

        // Do not update admin_state here
        // port_disable following configured=true will update it
        configured = true;
    }

    if (args->user_admin_state != port_admin_state_t::PORT_ADMIN_STATE_NONE &&
        args->user_admin_state != port_p->user_admin_state()) {
        SDK_TRACE_DEBUG("user_admin_state updated, new %d, old %d",
                        static_cast<int>(args->user_admin_state),
                        static_cast<int>(port_p->user_admin_state()));
        port_p->set_user_admin_state(args->user_admin_state);
    }

    if (args->cable_type != cable_type_t::CABLE_TYPE_NONE &&
        args->cable_type != port_p->cable_type()) {
        SDK_TRACE_DEBUG("cable_type updated, new %d, old %d",
                        args->cable_type, port_p->cable_type());
        port_p->set_cable_type(args->cable_type);
        configured = true;
    }

    // FEC_TYPE_NONE is valid
    if (args->user_fec_type != port_p->user_fec_type()) {
        SDK_TRACE_DEBUG("fec updated, new %d, old %d",
                        static_cast<int>(args->user_fec_type),
                        static_cast<int>(port_p->user_fec_type()));
        port_p->set_user_fec_type(args->user_fec_type);
        configured = true;
    }
    port_p->set_derived_fec_type(args->derived_fec_type);
    port_p->set_toggle_fec_mode(args->toggle_fec_mode);

    // MTU 0 is invalid
    if (args->mtu != 0 &&
        args->mtu != port_p->mtu()) {
        SDK_TRACE_DEBUG("mtu updated, new %d, old %d",
                        args->mtu, port_p->mtu());
        port_p->set_mtu(args->mtu);
        configured = true;
    }

    if (args->debounce_time != port_p->debounce_time()) {
        SDK_TRACE_DEBUG("Debounce updated, new %d, old %d",
                        args->debounce_time, port_p->debounce_time());
        port_p->set_debounce_time(args->debounce_time);
        configured = true;
    }

    // toggle_neg_mode overrides auto_neg_enable
    // check auto_neg_enable only if toggle_neg_mode is false
    if (args->toggle_neg_mode == false) {
        if (args->auto_neg_enable != port_p->auto_neg_enable()) {
            SDK_TRACE_DEBUG("AN updated, new %d, old %d",
                            args->auto_neg_enable, port_p->auto_neg_enable());
            port_p->set_auto_neg_enable(args->auto_neg_enable);
            configured = true;
        }
    }
    port_p->set_toggle_neg_mode(args->toggle_neg_mode);

    // TODO required?
    // if (args->toggle_neg_mode != port_p->toggle_neg_mode()) {
    //     SDK_TRACE_DEBUG("port %u toggle neg mode updated, new %d, old %d",
    //                     args->port_num, args->toggle_neg_mode, port_p->toggle_neg_mode());
    //     port_p->set_toggle_neg_mode(args->toggle_neg_mode);
    //     configured = true;
    // }

    if (args->auto_neg_cfg != port_p->auto_neg_cfg()) {
        SDK_TRACE_DEBUG("AN cfg updated, new %d, old %d",
                        args->auto_neg_cfg, port_p->auto_neg_cfg());
        port_p->set_auto_neg_cfg(args->auto_neg_cfg);
        configured = true;
    }

    // TODO update num_lanes to support auto speed set for QSA
    if (args->num_lanes != port_p->num_lanes()) {
        SDK_TRACE_DEBUG("num_lanes updated, new %d, old %d",
                        args->num_lanes, port_p->num_lanes());
        port_p->set_num_lanes(args->num_lanes);
        configured = true;
    }

    if (args->num_lanes_cfg != port_p->num_lanes_cfg()) {
        SDK_TRACE_DEBUG("num_lanes_cfg updated, new %d, old %d",
                        args->num_lanes_cfg, port_p->num_lanes_cfg());
        port_p->set_num_lanes_cfg(args->num_lanes_cfg);
        configured = true;
    }

    if (args->loopback_mode != port_p->loopback_mode()) {
        SDK_TRACE_DEBUG(
            "Loopback updated, new %s, old %s",
            args->loopback_mode ==
                port_loopback_mode_t::PORT_LOOPBACK_MODE_MAC? "mac" :
            args->loopback_mode ==
                port_loopback_mode_t::PORT_LOOPBACK_MODE_PHY? "phy" : "none",
            port_p->loopback_mode() ==
                port_loopback_mode_t::PORT_LOOPBACK_MODE_MAC? "mac" :
            port_p->loopback_mode() ==
                port_loopback_mode_t::PORT_LOOPBACK_MODE_PHY? "phy" : "none");

        port_p->set_loopback_mode(args->loopback_mode);
        configured = true;
    }

    if (args->pause != port_p->pause()) {
        SDK_TRACE_DEBUG("Pause updated, new %d, old %d",
                        static_cast<int>(args->pause),
                        static_cast<int>(port_p->pause()));
        port_p->set_pause(args->pause);
        configured = true;
    }

    if (args->tx_pause_enable != port_p->tx_pause_enable()) {
        SDK_TRACE_DEBUG("Tx Pause updated, new %d, old %d",
                        args->tx_pause_enable, port_p->tx_pause_enable());
        port_p->set_tx_pause_enable(args->tx_pause_enable);
        configured = true;
    }

    if (args->rx_pause_enable != port_p->rx_pause_enable()) {
        SDK_TRACE_DEBUG("Rx Pause updated, new %d, old %d",
                        args->rx_pause_enable, port_p->rx_pause_enable());
        port_p->set_rx_pause_enable(args->rx_pause_enable);
        configured = true;
    }

    if (args->mac_stats_reset == true) {
        port_stats_reset(port_p);
    }

    if (args->port_an_args != NULL) {
        port_p->set_user_cap(args->port_an_args->user_cap);
        port_p->set_fec_ability(args->port_an_args->fec_ability);
        port_p->set_fec_request(args->port_an_args->fec_request);
    }

    // Disable the port if any config has changed
    if (configured == true) {
        ret = port_disable(port_p);
    }

    // Enable the port if -
    //      admin-up state is set in request msg OR
    //      admin state is not set in request msg, but port was admin up
    switch(args->admin_state) {
    case port_admin_state_t::PORT_ADMIN_STATE_NONE:
        if (prev_admin_st == port_admin_state_t::PORT_ADMIN_STATE_UP) {
            ret = port_enable(port_p);
        }
        break;

    case port_admin_state_t::PORT_ADMIN_STATE_DOWN:
        ret = port_disable(port_p);
        break;

    case port_admin_state_t::PORT_ADMIN_STATE_UP:
        ret = port_enable(port_p);
        break;

    default:
        break;
    }

    return ret;
}

sdk_ret_t
port_stats_reset (void *pd_p)
{
    sdk_ret_t ret     = SDK_RET_OK;
    port      *port_p = (port *)pd_p;

    SDK_TRACE_DEBUG("Resetting MAC stats");
    // Put in reset
    ret = port_p->port_mac_stats_reset(true);
    // Release from reset
    ret = port_p->port_mac_stats_reset(false);
    return ret;
}

//-----------------------------------------------------------------------------
// delete given port by disabling the port and then deleting the port instance
//-----------------------------------------------------------------------------
sdk_ret_t
port_delete (void *pd_p)
{
    sdk_ret_t    ret = SDK_RET_OK;
    port         *port_p = (port *)pd_p;

    SDK_TRACE_DEBUG("port delete");

    port_p->port_deinit();

    ret = port_disable(port_p);
    port_p->~port();
    g_linkmgr_state->port_slab()->free(port_p);

    return ret;
}

//-----------------------------------------------------------------------------
// PD Port get
//-----------------------------------------------------------------------------
sdk_ret_t
port_get (void *pd_p, port_args_t *args)
{
    port      *port_p = (port *)pd_p;
    sdk::lib::catalog *catalog = g_linkmgr_cfg.catalog;
    uint16_t status;
    bool up;

    args->port_num    = port_p->port_num();
    args->port_type   = port_p->port_type();
    args->port_speed  = port_p->port_speed();
    args->admin_state = port_p->admin_state();
    args->mac_id      = port_p->mac_id();
    args->mac_ch      = port_p->mac_ch();
    args->num_lanes   = port_p->num_lanes();
    args->fec_type    = port_p->fec_type();
    args->user_fec_type = port_p->user_fec_type();
    args->derived_fec_type = port_p->derived_fec_type();
    args->mtu         = port_p->mtu();
    args->debounce_time    = port_p->debounce_time();
    args->auto_neg_enable  = port_p->auto_neg_enable();
    args->auto_neg_cfg     = port_p->auto_neg_cfg();
    args->user_admin_state = port_p->user_admin_state();
    args->num_lanes_cfg    = port_p->num_lanes_cfg();
    args->pause       = port_p->pause();
    args->tx_pause_enable = port_p->tx_pause_enable();
    args->rx_pause_enable = port_p->rx_pause_enable();
    args->link_sm     = port_p->port_link_sm();
    args->loopback_mode = port_p->loopback_mode();
    args->num_link_down = port_p->num_link_down();
    args->bringup_duration.tv_sec = port_p->bringup_duration_sec();
    args->bringup_duration.tv_nsec = port_p->bringup_duration_nsec();
    args->sm_logger = port_p->sm_logger();
    strncpy(args->last_down_timestamp, port_p->last_down_timestamp(), TIME_STR_SIZE);

    if (args->link_sm == port_link_sm_t::PORT_LINK_SM_AN_CFG &&
        port_p->auto_neg_enable() == true) {
        args->link_sm = port_p->port_link_an_sm();
    } else if (args->link_sm == port_link_sm_t::PORT_LINK_SM_DFE_TUNING) {
        args->link_sm = port_p->port_link_dfe_sm();
    }

    // send live link status from HW
    if (port_p->port_link_status() == true) {
        // for mgmt port, check marvell phy status if NCSI feature is disabled.
        // marvel phy status is always down if NCSI feature is enabled
        if ((port_p->port_type() == port_type_t::PORT_TYPE_MGMT) &&
            (runenv::is_feature_enabled(NCSI_FEATURE) != FEATURE_ENABLED)) {
            args->oper_status = port_oper_status_t::PORT_OPER_STATUS_DOWN;
            for (uint32_t i = 1; i <= catalog->num_logical_oob_ports(); i++) {
                if (catalog->oob_mgmt_port(i) == true) {
                    status = 0;
                    sdk::marvell::marvell_get_hwport_status(
                        catalog->oob_hw_port(i), &status);
                    sdk::marvell::marvell_get_status_updown(status, &up);
                    if (up == true) {
                        args->oper_status =
                            port_oper_status_t::PORT_OPER_STATUS_UP;
                    }
                }
            }
        } else {
            args->oper_status = port_oper_status_t::PORT_OPER_STATUS_UP;
        }
    } else {
        args->oper_status = port_oper_status_t::PORT_OPER_STATUS_DOWN;
    }

    if (NULL != args->stats_data) {
        port_p->port_mac_stats_get (args->stats_data);
    }

    return SDK_RET_OK;
}

port_type_t
port_type (void *port_p)
{
    return ((port *)port_p)->port_type();
}

//-----------------------------------------------------------------------------
// PD Port mem free
//-----------------------------------------------------------------------------
sdk_ret_t
port_mem_free (port_args_t *args)
{
    sdk_ret_t    ret = SDK_RET_OK;
    port         *port_p = (port *)args->port_p;

    g_linkmgr_state->port_slab()->free(port_p);

    return ret;
}

bool
port_has_speed_changed (port_args_t *args)
{
    port *port_p = (port *)args->port_p;
    return (args->port_speed != port_p->port_speed());
}

bool
port_has_admin_state_changed (port_args_t *args)
{
    port *port_p = (port *)args->port_p;
    return (args->admin_state != port_p->admin_state());
}

//-----------------------------------------
// AACS server
//-----------------------------------------
static void*
linkmgr_aacs_start (void* ctxt)
{
    if (g_linkmgr_state->aacs_server_port_num() != -1) {
        serdes_fns()->serdes_aacs_start(
            g_linkmgr_state->aacs_server_port_num());
    }

    return NULL;
}

sdk_ret_t
start_aacs_server (int port)
{
    int thread_prio  = 0;
    int thread_id    = 0;
    int sched_policy = SCHED_OTHER;

    if (g_linkmgr_state->aacs_server_port_num() != -1) {
        SDK_TRACE_DEBUG("AACS server already started on port %d",
                        g_linkmgr_state->aacs_server_port_num());
        return SDK_RET_OK;
    }

    thread_prio = sched_get_priority_max(sched_policy);
    if (thread_prio < 0) {
        return SDK_RET_ERR;
    }

    thread_id = SDK_IPC_ID_LINKMGR_AACS_SERVER;
    sdk::lib::thread *thread =
        sdk::lib::thread::factory(
                        std::string("aacs-server").c_str(),
                        thread_id,
                        sdk::lib::THREAD_ROLE_CONTROL,
                        0x0 /* use all control cores */,
                        linkmgr_aacs_start,
                        thread_prio,
                        sched_policy,
                        true);
    if (thread == NULL) {
        SDK_TRACE_ERR("Failed to create linkmgr aacs server thread");
        return SDK_RET_ERR;
    }
    g_linkmgr_state->set_aacs_server_port_num(port);
    thread->start(NULL);
    return SDK_RET_OK;
}

void
stop_aacs_server (void)
{
    uint32_t thread_id = SDK_IPC_ID_LINKMGR_AACS_SERVER;
    sdk::lib::thread *thread = sdk::lib::thread::find(thread_id);

    if (thread != NULL) {
        // stop the thread
        thread->stop();
        // free the allocated thread
        sdk::lib::thread::destroy(thread);
    }

    // reset the server port
    g_linkmgr_state->set_aacs_server_port_num(-1);
}

port_admin_state_t
port_default_admin_state (void)
{
    return g_linkmgr_cfg.admin_state;
}

mem_addr_t
port_stats_addr (uint32_t ifindex)
{
    sdk::types::mem_addr_t port_stats_base;

    if (g_linkmgr_cfg.mempartition == NULL) {
        SDK_TRACE_ERR("port %s stats_init NULL mempartition port stats not supported",
                      eth_ifindex_to_str(ifindex).c_str());
        return INVALID_MEM_ADDRESS;
    }
    port_stats_base = g_linkmgr_cfg.mempartition->start_addr(ASIC_HBM_REG_PORT_STATS);

    if ((port_stats_base == 0) || (port_stats_base == INVALID_MEM_ADDRESS)) {
        SDK_TRACE_ERR("port %s stats_init port_stats_base 0x%lx port stats not supported",
                      eth_ifindex_to_str(ifindex).c_str(), port_stats_base);
        return INVALID_MEM_ADDRESS;
    }

    return port_stats_base + port_stats_addr_offset(ifindex);
}

/// \brief     helper method to store the user configured values into separate
///            variables in port_args
/// \param[in] port_args port arguments to be programmed
void
port_store_user_config (port_args_t *port_args)
{
    // store user configured admin_state in another variable to be used
    // during xcvr insert/remove events
    port_args->user_admin_state = port_args->admin_state;

    // store user configured AN in another variable to be used
    // during xcvr insert/remove events
    port_args->auto_neg_cfg = port_args->auto_neg_enable;

    // store user configured num_lanes in another variable to be used
    // during xcvr insert/remove events
    port_args->num_lanes_cfg = port_args->num_lanes;

    // store user configured fec type
    port_args->user_fec_type = port_args->derived_fec_type =
                                                  port_args->fec_type;
}

uint16_t
num_uplinks_link_up (void)
{
    return sdk::lib::count_bits_set(g_linkmgr_state->port_bmap() &
                                    g_linkmgr_state->port_bmap_mask());
}

}    // namespace linkmgr
}    // namespace sdk
