//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// Initialize Stubs that drive the dataplane
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_cookie.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_ifindex.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_l2f_mai.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/apollo/include/globals.hpp"
#include "nic/apollo/core/event.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/core/msg.h"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include <li_fte.hpp>

extern NBB_ULONG li_proc_id;

namespace pds_ms {

void
hal_callback (sdk_ret_t status, const void *cookie)
{
    if (cookie == nullptr) return;
    std::unique_ptr<cookie_t> cookie_ptr ((cookie_t*) cookie);

#if 0 // TODO: Enable after all object commits are synchronous
    SDK_ASSERT(cookie_ptr->objs.empty());
#endif
    if (status != SDK_RET_OK) {
        PDS_TRACE_ERR("Async PDS HAL callback failure status %d, cookie %p",
                      status, cookie);
    } else {
        PDS_TRACE_VERBOSE("Async PDS HAL callback success, cookie %p", cookie);
        auto state_ctxt = state_t::thread_context();
        auto& objs = cookie_ptr->objs;
        if (objs.size() > 0) {PDS_TRACE_DEBUG ("Committing object(s) to store:");}

        for (auto& obj_uptr: objs) {
            obj_uptr->print_debug_str();
            obj_uptr->update_store (state_ctxt.state(), false);
            // New created object is saved in store.
            // Release the obj ownership from cookie
            obj_uptr.release();
        }
    }
    cookie_ptr->send_ips_reply((status == SDK_RET_OK), cookie_ptr->ips_mock);
}

static void
handle_port_event (core::port_event_info_t &portev)
{
    FRL_FAULT_STATE fault_state;
    std::string ifname;
    void *worker = NULL;

    {
        auto ctx = pds_ms::mgmt_state_t::thread_context();
        if (!(ctx.state()->nbase_thread()->ready())) {
            // If event comes before nbase thread is ready then
            // ignore that event. This can happen since the event
            // subscribe is called before nbase is ready
            return;
        }
    }

    NBB_CREATE_THREAD_CONTEXT
    NBS_ENTER_SHARED_CONTEXT(li_proc_id);
    NBS_GET_SHARED_DATA();

    // Get the FRL pointer
    ntl::Frl &frl = li::Fte::get().get_frl();
    frl.init_fault_state(&fault_state);
    uint32_t ifidx = pds_ms::pds_to_ms_ifindex(portev.ifindex, IF_TYPE_ETH);

    // Init the fault state
    // Set the fault state based on current link state
    if (portev.event == port_event_t::PORT_EVENT_LINK_UP) {
        fault_state.hw_link_faults = ATG_FRI_FAULT_NONE;
    } else if (portev.event == port_event_t::PORT_EVENT_LINK_DOWN) {
        fault_state.hw_link_faults = ATG_FRI_FAULT_PRESENT;
    }
    {
        auto state_ctxt = pds_ms::state_t::thread_context();
        auto obj = state_ctxt.state()->if_store().get(ifidx);
        if (obj != nullptr) {
            worker = obj->phy_port_properties().fri_worker;
        }
    }
    if (worker != nullptr) {
        PDS_TRACE_DEBUG("Sending intf fault indication, event %u",
                         static_cast<uint32_t>(portev.event));
        frl.send_fault_ind(worker, &fault_state);
    } else {
        PDS_TRACE_DEBUG("No intf FRL worker, event %u",
                         static_cast<uint32_t>(portev.event));
    }

    NBS_RELEASE_SHARED_DATA();
    NBS_EXIT_SHARED_CONTEXT();
    NBB_DESTROY_THREAD_CONTEXT

    return;
}

static void
handle_learn_event (core::event_t *event)
{
    static ip_addr_t zero_ip = { 0 };

    PDS_TRACE_DEBUG("Got learn event %u, VPC %s Subnet %s Ifindex 0x%x "
                    "IpAddr %s MacAddr %s", (uint32_t)event->event_id,
                    event->learn.vpc.str(), event->learn.subnet.str(),
                    event->learn.ifindex, ipaddr2str(&event->learn.ip_addr),
                     macaddr2str(event->learn.mac_addr));
    switch (event->event_id) {
    case EVENT_ID_MAC_LEARN:
        pds_ms::l2f_local_mac_ip_add(event->learn.subnet, zero_ip,
                                     event->learn.mac_addr,
                                     event->learn.ifindex);
        break;
    case EVENT_ID_IP_LEARN:
        pds_ms::l2f_local_mac_ip_add(event->learn.subnet, event->learn.ip_addr,
                                     event->learn.mac_addr,
                                     event->learn.ifindex);
        break;
    case EVENT_ID_MAC_AGE:
        pds_ms::l2f_local_mac_ip_del(event->learn.subnet, zero_ip,
                                     event->learn.mac_addr);
        break;
    case EVENT_ID_IP_AGE:
        pds_ms::l2f_local_mac_ip_del(event->learn.subnet,
                                     event->learn.ip_addr,
                                     event->learn.mac_addr);
        break;
    default:
        break;
    }
}

void
hal_event_callback (sdk::ipc::ipc_msg_ptr msg, const void *ctx)
{
    core::event_t *event = (core::event_t *)msg->data();

    if (!event) {
        return;
    }
    PDS_TRACE_DEBUG("Got event id %u", event->event_id);
    switch (event->event_id) {
    case EVENT_ID_PORT_STATUS:
        handle_port_event(event->port);
        break;
    case EVENT_ID_LIF_STATUS:
        // TODO: Need to propagate LIF events to the software-IF
        break;
    case EVENT_ID_MAC_LEARN:
    case EVENT_ID_IP_LEARN:
    case EVENT_ID_MAC_AGE:
    case EVENT_ID_IP_AGE:
        handle_learn_event(event);
        break;
    default:
        PDS_TRACE_ERR("Unknown event %u", event->event_id);
        break;
    }
    return;
}

static void
pds_msg_cfg_callback (sdk::ipc::ipc_msg_ptr ipc_msg, const void *ctxt)
{
    pds_cfg_msg_t *cfg_msg;
    pds_msg_list_t *msg_list;
    sdk_ret_t ret = SDK_RET_OK;

    PDS_TRACE_DEBUG("Rcvd PDS_MSG_TYPE_CFG_OBJ_SET IPC msg");
    msg_list = (pds_msg_list_t *)ipc_msg->data();
    for (uint32_t i = 0; i < msg_list->num_msgs; i++) {
        cfg_msg = &msg_list->msgs[i].cfg_msg;
        PDS_TRACE_DEBUG("Rcvd api obj %u, api op %u", cfg_msg->obj_id,
                        cfg_msg->op);
        // TODO: handle the msg
        ret = SDK_RET_OK;
    }
    sdk::ipc::respond(ipc_msg, (const void *)&ret, sizeof(sdk_ret_t));
}

void
ipc_init_cb (int fd, sdk::ipc::handler_ms_cb cb, void *ctx)
{
    PDS_TRACE_DEBUG("ipc init callback, fd %d", fd);
    // Register SDK ipc infra fd with metaswitch. Metaswitch calls the callback
    // function in the context of the nbase thread when there is any event
    // pending on the fd
    NBS_REGISTER_WORK_SOURCE(fd, NBS_WORK_SOURCE_FD, NBS_WORK_MASK_READ,
                             cb, ctx);
    return;
}

bool
hal_init (void)
{
    sdk::ipc::ipc_init_metaswitch(PDS_IPC_ID_ROUTING, &ipc_init_cb);
    sdk::ipc::subscribe(EVENT_ID_PORT_STATUS, &hal_event_callback, NULL);
    sdk::ipc::subscribe(EVENT_ID_MAC_LEARN, &hal_event_callback, NULL);
    sdk::ipc::subscribe(EVENT_ID_IP_LEARN, &hal_event_callback, NULL);
    sdk::ipc::subscribe(EVENT_ID_MAC_AGE, &hal_event_callback, NULL);
    sdk::ipc::subscribe(EVENT_ID_IP_AGE, &hal_event_callback, NULL);
    sdk::ipc::reg_request_handler(PDS_MSG_TYPE_CFG_OBJ_SET,
                                  pds_msg_cfg_callback, NULL);
    return true;
}

void
hal_deinit (void)
{
    return;
}

} // End namespace
