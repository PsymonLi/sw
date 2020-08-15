//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// learn thread implementation
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/lib/dpdk/dpdk.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_msg.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/api/include/pds_subnet.hpp"
#include "nic/apollo/learn/learn_impl_base.hpp"
#include "nic/apollo/learn/learn_thread.hpp"
#include "nic/apollo/learn/learn.hpp"
#include "nic/apollo/learn/learn_internal.hpp"
#include "nic/apollo/learn/ep_utils.hpp"

namespace learn {

learn_handlers_t g_handler_tbl;

static void
setup_handlers (pds_learn_mode_t mode)
{
    switch (mode) {
    case PDS_LEARN_MODE_NOTIFY:
        g_handler_tbl.parse       = parse_packet;
        g_handler_tbl.pre_process = mode_notify::pre_process;
        g_handler_tbl.validate    = mode_notify::validate;
        g_handler_tbl.process     = mode_notify::process;
        g_handler_tbl.store       = mode_notify::store;
        g_handler_tbl.notify      = mode_notify::notify;
        break;

    case PDS_LEARN_MODE_AUTO:
        g_handler_tbl.parse       = parse_packet;
        g_handler_tbl.pre_process = mode_auto::pre_process;
        g_handler_tbl.validate    = mode_auto::validate;
        g_handler_tbl.process     = mode_auto::process;
        g_handler_tbl.store       = mode_auto::store;
        g_handler_tbl.notify      = mode_auto::notify;
        break;

    case PDS_LEARN_MODE_NONE:
    default:
        memset(&g_handler_tbl, 0, sizeof(learn_handlers_t));
        break;
    }
}

bool
learning_enabled (void)
{

    // first check if learning is enabled in the device config
    // TODO: device cfg is not available, defer the check to later
#if 0
    device_entry *device = device_db()->find();
    if (device == nullptr) {
        PDS_TRACE_ERR("Unable to read device state");
        return false;
    }
    if (!device->learning_enabled()) {
        return false;
    }
#endif

    // there is no other way to get if we are running gtest without model.
    // if we start learn thread in gtest, DPDK will assert as model is not
    // running, so disable learn thread while running under gtest
    // TODO: remove this after making sure gtest setup disables learning
    if (getenv("ASIC_MOCK_MODE")) {
        return false;
    }

    // check if pipeline defines learn lif
    if (!impl::learn_lif_name()) {
        return false;
    }
    return true;
}

void
learn_thread_exit_fn (void *ctxt)
{
}

void
learn_thread_event_cb (void *msg, void *ctxt)
{
}

#define learn_lif learn_db()->learn_lif()

void
learn_thread_pkt_poll_timer_cb (event::timer_t *timer)
{
    uint16_t rx_count = 0;
    dpdk_mbuf **rx_pkts;

    rx_pkts = learn_lif->receive_packets(0, LEARN_LIF_RECV_BURST_SZ, &rx_count);
    if (unlikely(!rx_pkts || !rx_count)) {
        return;
    }
    PDS_TRACE_VERBOSE("Rcvd learn packet");
    LEARN_COUNTER_ADD(rx_pkts, rx_count);

    for (uint16_t i = 0; i < rx_count; i++) {
        process_learn_pkt((void *)rx_pkts[i]);
    }
}

static void
process_device_create (pds_learn_mode_t mode)
{
    static event::timer_t pkt_poll_timer;
    float                 pkt_poll_interval;

    learn_db()->init_oper_mode(mode);
    setup_handlers(mode);
    if (mode != PDS_LEARN_MODE_NONE) {
        // pkt poll timer handler
        pkt_poll_interval = learn_db()->pkt_poll_interval_msecs()/1000.;
        event::timer_init(&pkt_poll_timer, learn_thread_pkt_poll_timer_cb,
                          1., pkt_poll_interval);
        event::timer_start(&pkt_poll_timer);
    }
}

void
learn_thread_api_cfg_cb (sdk::ipc::ipc_msg_ptr ipc_msg, const void *ctx)
{
    sdk::sdk_ret_t    ret = SDK_RET_OK;;
    pds_msg_list_t    *msg_list  = (pds_msg_list_t *)ipc_msg->data();
    pds_cfg_msg_t     *cfg_msg;
    pds_device_spec_t *device_spec;

    msg_list = (pds_msg_list_t *)ipc_msg->data();
    for (uint32_t i = 0; i < msg_list->num_msgs; i++) {
        cfg_msg = &msg_list->msgs[i].cfg_msg;
        if ((cfg_msg->obj_id == OBJ_ID_DEVICE) &&
            (cfg_msg->op == API_OP_CREATE)) {
            PDS_TRACE_VERBOSE("Received device create message");
            device_spec = &cfg_msg->device.spec.spec;
            process_device_create(device_spec->learn_spec.learn_mode);
        }
    }
    sdk::ipc::respond(ipc_msg, (const void *)&ret, sizeof(sdk_ret_t));
}

void
learn_thread_ipc_api_cb (sdk::ipc::ipc_msg_ptr msg, const void *ctx)
{
    sdk_ret_t ret;
    api::api_msg_t *api_msg  = (api::api_msg_t *)msg->data();

    PDS_TRACE_DEBUG("Rcvd API batch message");
    SDK_ASSERT(api_msg != nullptr);
    SDK_ASSERT(api_msg->msg_id == api::API_MSG_ID_BATCH);
    SDK_ASSERT(api_msg->batch.apis.size() != 0);

    ret = process_api_batch(api_msg);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

void
learn_thread_ipc_clear_cmd_cb (sdk::ipc::ipc_msg_ptr msg, const void *ctx)
{
    sdk_ret_t         ret = SDK_RET_OK;
    learn_clear_msg_t *req = (learn_clear_msg_t *)msg->data();
    ep_mac_key_t      *mac_key = &req->mac_key;
    ep_ip_key_t       *ip_key = &req->ip_key;

    switch(req->id) {
    case LEARN_CLEAR_MAC:
        PDS_TRACE_INFO("Received clear request for MAC %s/%s",
                       macaddr2str(mac_key->mac_addr),
                       mac_key->subnet.str());
        ret = ep_mac_entry_clear(mac_key);
        break;
    case LEARN_CLEAR_MAC_ALL:
        PDS_TRACE_INFO("received request to clear all MAC");
        ret = ep_mac_entry_clear_all();
        break;
    case LEARN_CLEAR_IP:
        PDS_TRACE_INFO("Received clear request for IP %s/%s",
                       ipaddr2str(&ip_key->ip_addr), ip_key->vpc.str());
        ret = ep_ip_entry_clear(ip_key);
        break;
    case LEARN_CLEAR_IP_ALL:
        PDS_TRACE_INFO("Received request to clear all IP");
        ret = ep_ip_entry_clear_all();
        break;
    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

void
learn_thread_init_fn (void *ctxt)
{
    // initalize learn state and dpdk_device
    SDK_ASSERT(learn_db()->init() == SDK_RET_OK);

    sdk::ipc::reg_request_handler(PDS_MSG_TYPE_CFG_OBJ_SET,
                                  learn_thread_api_cfg_cb, NULL);
    // control plane message handler
    sdk::ipc::reg_request_handler(LEARN_MSG_ID_API, learn_thread_ipc_api_cb,
                                  NULL);
    sdk::ipc::reg_request_handler(LEARN_MSG_ID_CLEAR_CMD,
                                  learn_thread_ipc_clear_cmd_cb,
                                  NULL);
}

}    // namespace learn
