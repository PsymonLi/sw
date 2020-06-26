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

/// \brief callback for the port switchover event
///        in the context of the linkmgr-ctrl thread
static void
linkmgr_ctrl_switchover (ipc_msg_ptr msg, const void *ctx)
{
    linkmgr_ctrl_timers_start();

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

            sdk::linkmgr::serdes_fns.serdes_spico_upload(sbus_addr, cfg_file.c_str());

            int build_id = sdk::linkmgr::serdes_fns.serdes_get_build_id(sbus_addr);
            int rev_id   = sdk::linkmgr::serdes_fns.serdes_get_rev(sbus_addr);

            if (build_id != exp_build_id || rev_id != exp_rev_id) {
                SDK_TRACE_DEBUG("sbus_addr 0x%x,"
                                " build_id 0x%x, exp_build_id 0x%x,"
                                " rev_id 0x%x, exp_rev_id 0x%x",
                                sbus_addr, build_id, exp_build_id,
                                rev_id, exp_rev_id);
                // TODO fail if no match
            }

            sdk::linkmgr::serdes_fns.serdes_spico_status(sbus_addr);

            SDK_TRACE_DEBUG("sbus_addr 0x%x, spico_crc %d",
                            sbus_addr,
                            sdk::linkmgr::serdes_fns.serdes_spico_crc(sbus_addr));
        }
    }

    pal_wr_unlock(SBUSLOCK);

    srand(time(NULL));

    // Init the linkmgr timers
    // In regular bringup, timers are started after allocating port and
    // xcvr memory
    // In upgrade mode, timers are started during switchover
    linkmgr_ctrl_timers_init();

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
