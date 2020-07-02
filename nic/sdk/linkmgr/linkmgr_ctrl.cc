//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// linkmgr ctrl thread requests handling
///
//----------------------------------------------------------------------------

#include <boost/interprocess/managed_shared_memory.hpp>
#include "platform/drivers/xcvr.hpp"
#include "port_serdes.hpp"
#include "lib/pal/pal.hpp"
#include "platform/pal/include/pal.h"
#include "lib/event_thread/event_thread.hpp"
#include "lib/ipc/ipc.hpp"
#include "linkmgr_state.hpp"
#include "port.hpp"
#include "include/sdk/port_utils.hpp"
#include "lib/utils/utils.hpp"

using namespace sdk::event_thread;
using namespace sdk::ipc;
using namespace boost::interprocess;

namespace sdk {
namespace linkmgr {

// link down poll list
port *link_poll_timer_list[MAX_LOGICAL_PORTS];

static port *
port_link_poll_timer_list (uint32_t index)
{
    return link_poll_timer_list[index];
}

sdk_ret_t
port_link_poll_timer_add (void *pd_p)
{
    port *port_p = (port *)pd_p;

    link_poll_timer_list[port_p->port_num() - 1] = port_p;
    return SDK_RET_OK;
}

sdk_ret_t
port_link_poll_timer_delete (void *pd_p)
{
    port *port_p = (port *)pd_p;

    link_poll_timer_list[port_p->port_num() - 1] = NULL;
    return SDK_RET_OK;
}

static bool
port_link_poll_enabled (void)
{
    return g_linkmgr_state->link_poll_en();
}

static sdk_ret_t
port_event_enable (linkmgr_entry_data_t *data)
{
    port *port_p = (port *)data->ctxt;
    return port_p->port_enable(true);
}

static sdk_ret_t
port_event_disable (linkmgr_entry_data_t *data)
{
    port *port_p = (port *)data->ctxt;
    return port_p->port_disable();
}

static void
port_enable_req_handler (ipc_msg_ptr msg, const void *ctx)
{
    SDK_ATOMIC_STORE_BOOL(&hal_cfg, false);
    port_event_enable((linkmgr_entry_data_t *)msg->data());

    // send response back to blocked caller
    respond(msg, NULL, 0);
}

static void
port_disable_req_handler (ipc_msg_ptr msg, const void *ctx)
{
    SDK_ATOMIC_STORE_BOOL(&hal_cfg, false);
    port_event_disable((linkmgr_entry_data_t *)msg->data());

    // send response back to blocked caller
    respond(msg, NULL, 0);
}

static void
port_quiesce_req_handler (ipc_msg_ptr msg, const void *ctx)
{
    linkmgr_entry_data_t *data = (linkmgr_entry_data_t *)msg->data();
    port *port_p = (port *)data->ctxt;
    linkmgr_entry_data_t resp = *data;
    sdk_ret_t ret;

    ret = port_p->port_quiesce();
    // send response back to blocked caller
    resp.status = ret;
    respond(msg, &resp, sizeof(resp));
}

/// \brief  start the link poll and transceiver poll timers
///         in the context of linkmgr-ctrl thread
/// \return SDK_RET_OK on success, failure status code on error
static sdk_ret_t
linkmgr_ctrl_timers_start (void)
{
    timer_start(g_linkmgr_state->xcvr_poll_timer_handle());
    timer_start(g_linkmgr_state->link_poll_timer_handle());
    SDK_TRACE_DEBUG("starting poll timers");
    return SDK_RET_OK;
}

/// \brief     restore the port state during upgrade
/// \param[in] port_p pointer to port struct
/// \return    SDK_RET_OK on success, failure status code on error
static sdk_ret_t
port_restore_ (port *port_p)
{
    sdk_ret_t ret;
    uint64_t mask = 1 << (port_p->port_num() - 1);

    // init the bringup and debounce timers
    port_p->timers_init();

    if(port_p->port_type() != port_type_t::PORT_TYPE_MGMT) {
        g_linkmgr_state->set_port_bmap_mask(g_linkmgr_state->port_bmap_mask() |
                                            mask);
    }
    // set the source mac addr for pause frames
    // TODO required during upgrade?
    // port_p->port_mac_set_pause_src_addr(args->mac_addr);

    // init MAC stats hbm region address
    ret = port::port_mac_stats_init(port_p);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("port %u mac stats init failed", port_p->port_num());
        return ret;
    }
    return SDK_RET_OK;
}

/// \brief     enable port operations after upgrade
/// \param[in] port_p pointer to port struct
/// \return    SDK_RET_OK on success, failure status code on error
static sdk_ret_t
port_switchover_ (port *port_p)
{
    sdk_ret_t ret;
    uint32_t ifindex;
    uint64_t mask = 1 << (port_p->port_num() - 1);

    ifindex =
        sdk::lib::catalog::logical_port_to_ifindex(port_p->port_num());
    SDK_TRACE_DEBUG("port upgrade switchover for port %s",
                    eth_ifindex_to_str(ifindex).c_str());

    ret = port_restore_(port_p);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("port upgrade failed to restore port %s",
                      eth_ifindex_to_str(ifindex).c_str());
        return ret;
    }

    // If Link was UP before upgrade:
    //     Add the port to link poll timer
    //     Set port bmap to reflect link up
    // Else:
    //     Disable the port
    //     If the port admin state is up
    //         Enable the port
    if (port_p->oper_status() == port_oper_status_t::PORT_OPER_STATUS_UP) {
        // add to link status poll timer if the link was up before upgrade
        ret = port_link_poll_timer_add(port_p);

        // set bmap for link status
        g_linkmgr_state->set_port_bmap(g_linkmgr_state->port_bmap() | mask);
    } else {
        // disable a port to invoke soft reset
        ret = port_p->port_disable();
        if (ret != SDK_RET_OK) {
            SDK_TRACE_ERR("port %u disable failed", port_p->port_num());
        }
        if (port_p->admin_state() == port_admin_state_t::PORT_ADMIN_STATE_UP) {
            ret = port_p->port_enable();
            if (ret != SDK_RET_OK) {
                SDK_TRACE_ERR("port %u enable failed", port_p->port_num());
            }
        }
    }
    return ret;
}

/// \brief callback for the port switchover event
///        in the context of the linkmgr-ctrl thread
static void
linkmgr_ctrl_switchover (ipc_msg_ptr msg, const void *ctx)
{
    port *port_p;

    linkmgr_ctrl_timers_start();

    for (uint32_t i = 0; i < LINKMGR_MAX_PORTS; i++) {
        port_p = g_linkmgr_state->port_p(i);
        if (port_p) {
            port_switchover_(port_p);
        }
    }
    // send response back to blocked caller
    respond(msg, NULL, 0);
}

void
port_bringup_timer_cb (sdk::event_thread::timer_t *timer)
{
    uint32_t  retries = 0;
    port      *port_p = (port *)timer->ctx;

    // stop the repeated timer since timer_again is being used
    // to schedule timer
    timer_stop(timer);

    // if the bringup timer has expired, reset and try again
    if (port_p->bringup_timer_expired() == true) {
        retries = port_p->num_retries();

        // port disable resets num_retries
        port_p->port_disable();

        // Enable port only if max retries is not reached
        if (1 || retries < MAX_PORT_LINKUP_RETRIES) {
            port_p->set_num_retries(retries + 1);
            port_p->port_enable();
        } else {
            // TODO notify upper layers?
            SDK_TRACE_DEBUG("Max retries %d reached for port %d",
                            MAX_PORT_LINKUP_RETRIES, port_p->port_num());
        }
    } else {
        port_p->port_link_sm_process();
    }
}

void
port_debounce_timer_cb (sdk::event_thread::timer_t *timer)
{
    port *port_p = (port *)timer->ctx;

    // stop the repeated timer since timer_again is being used
    // to schedule timer
    timer_stop(timer);

    port_p->port_debounce_timer_cb();
}

static void
port_link_poll_timer_cb (sdk::event_thread::timer_t *timer)
{
    if (port_link_poll_enabled() == true) {
        for (int i = 0; i < MAX_LOGICAL_PORTS; ++i) {
            port *port_p = port_link_poll_timer_list(i);

            if (port_p != NULL) {
                if(port_p->port_link_status() == false) {
                    SDK_TRACE_DEBUG("%d: Link DOWN", port_p->port_num());
                    port_p->port_link_dn_handler();
                }
            }
        }
    }
}

static void
xcvr_poll_timer_cb (sdk::event_thread::timer_t *timer)
{
    sdk::platform::xcvr_poll_timer();
}

static sdk_ret_t
linkmgr_ctrl_timers_init (void)
{
    // init the transceiver poll timer
    timer_init(g_linkmgr_state->xcvr_poll_timer_handle(), xcvr_poll_timer_cb,
               XCVR_POLL_TIME, XCVR_POLL_TIME);

    // init the link poll timer
    timer_init(g_linkmgr_state->link_poll_timer_handle(),
               port_link_poll_timer_cb, LINKMGR_LINK_POLL_TIME,
               LINKMGR_LINK_POLL_TIME);
    return SDK_RET_OK;
}

void
linkmgr_event_thread_init (void *ctxt)
{
    int         exp_build_id = serdes_build_id();
    int         exp_rev_id   = serdes_rev_id();
    std::string cfg_file     = "fw/" + serdes_fw_file();
    serdes_fn_t *serdes_fn = serdes_fns();

    cfg_file = std::string(g_linkmgr_cfg.cfg_path) + "/" + cfg_file;

    pal_wr_lock(SBUSLOCK);

    // TODO move back to serdes_fn_init
    serdes_get_ip_info(1);

    serdes_sbm_set_sbus_clock_divider(sbm_clk_div());

    // download serdes fw only if port state is not restored from memory
    if (g_linkmgr_state->port_restore_state() == false) {
        for (uint32_t asic_port = 0; asic_port < num_asic_ports(0); ++asic_port) {
            uint32_t sbus_addr = sbus_addr_asic_port(0, asic_port);

            if (sbus_addr == 0) {
                continue;
            }

            serdes_fn->serdes_spico_upload(sbus_addr, cfg_file.c_str());

            int build_id = serdes_fn->serdes_get_build_id(sbus_addr);
            int rev_id   = serdes_fn->serdes_get_rev(sbus_addr);

            if (build_id != exp_build_id || rev_id != exp_rev_id) {
                SDK_TRACE_DEBUG("sbus_addr 0x%x,"
                                " build_id 0x%x, exp_build_id 0x%x,"
                                " rev_id 0x%x, exp_rev_id 0x%x",
                                sbus_addr, build_id, exp_build_id,
                                rev_id, exp_rev_id);
                // TODO fail if no match
            }

            serdes_fn->serdes_spico_status(sbus_addr);

            SDK_TRACE_DEBUG("sbus_addr 0x%x, spico_crc %d",
                            sbus_addr,
                            serdes_fn->serdes_spico_crc(sbus_addr));
        }
    }

    pal_wr_unlock(SBUSLOCK);

    srand(time(NULL));

    // Init the linkmgr timers
    // In regular bringup, timers are started after allocating port and
    // xcvr memory
    // In upgrade mode, timers are started during switchover
    linkmgr_ctrl_timers_init();
    if (g_linkmgr_state->port_restore_state() == false) {
        linkmgr_ctrl_timers_start();
    }
    reg_request_handler(LINKMGR_OPERATION_PORT_ENABLE,
                        port_enable_req_handler, NULL);
    reg_request_handler(LINKMGR_OPERATION_PORT_DISABLE,
                        port_disable_req_handler, NULL);
    reg_request_handler(LINKMGR_OPERATION_PORT_QUIESCE,
                        port_quiesce_req_handler, NULL);
    reg_request_handler(LINKMGR_OPERATION_PORT_SWITCHOVER,
                        linkmgr_ctrl_switchover, NULL);
}

void
linkmgr_event_thread_exit (void *ctxt)
{
}

void
linkmgr_event_handler (void *msg, void *ctxt)
{
}

}    // namespace linkmgr
}    // namespace sdk
