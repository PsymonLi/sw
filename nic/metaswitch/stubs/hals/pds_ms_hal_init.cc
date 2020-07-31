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
#include "nic/metaswitch/stubs/hals/pds_ms_upgrade.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_l2f_mai.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hals_route.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_cfg_msg.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_upgrade.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/lib/ipc/ipc_ms.hpp"
#include "nic/apollo/include/globals.hpp"
#include "nic/apollo/core/event.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/core/msg.h"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
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
    void *worker = nullptr;

    {
        auto ctx = pds_ms::mgmt_state_t::thread_context();
        if (!(ctx.state()->nbase_thread()->ready())) {
            // If event comes before nbase thread is ready then
            // ignore that event. This can happen since the event
            // subscribe is called before nbase is ready
            return;
        }
    }

    uint32_t ifidx = pds_ms::pds_to_ms_ifindex(portev.ifindex, IF_TYPE_ETH);
    {
        auto state_ctxt = pds_ms::state_t::thread_context();
        auto obj = state_ctxt.state()->if_store().get(ifidx);

        // In case of hitless upgrade, IF store obj is created by LI stub
        // before the L3 inteface spec is replayed by gRPC. Ignore
        // link events during this window. The link state will be
        // read when the L3 interface spec is received
        if (obj != nullptr && obj->phy_port_properties().mgmt_spec_init) {
            worker = obj->phy_port_properties().fri_worker;
        }
    }
    if (worker == nullptr) {
        PDS_TRACE_DEBUG("No intf FRL worker, event %u",
                         static_cast<uint32_t>(portev.event));
        return;
    }

    NBB_CREATE_THREAD_CONTEXT
    NBS_ENTER_SHARED_CONTEXT(li_proc_id);
    NBS_GET_SHARED_DATA();

    // Get the FRL pointer
    ntl::Frl &frl = li::Fte::get().get_frl();
    frl.init_fault_state(&fault_state);

    // Init the fault state
    // Set the fault state based on current link state
    if (portev.event == port_event_t::PORT_EVENT_LINK_UP) {
        fault_state.hw_link_faults = ATG_FRI_FAULT_NONE;
    } else if (portev.event == port_event_t::PORT_EVENT_LINK_DOWN) {
        fault_state.hw_link_faults = ATG_FRI_FAULT_PRESENT;
    }
    PDS_TRACE_INFO("Sending intf fault indication, event %u",
                    static_cast<uint32_t>(portev.event));
    frl.send_fault_ind(worker, &fault_state);

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
    case EVENT_ID_HOST_IF_STATUS:
        // TODO: Need to propagate Host if events to the software-IF
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

static void
init_routing_cfg_thr (void* ctx)
{
    PDS_TRACE_DEBUG ("Started Routing Cfg thread %p", state_t::routing_cfg_thr);

    sdk::ipc::reg_request_handler(PDS_MSG_TYPE_CFG_OBJ_SET,
                                  pds_msg_cfg_callback, NULL);
    upg_ipc_init();
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
    // Initialize the Nbase timer list
    {
        auto ctx = state_t::thread_context();
        ctx.state()->init_timer_list(ctx.state()->get_route_timer_list(),
                                     route_timer_expiry_cb);
    }

    // Routing config thread receives the IP track events from
    // HAL API thread and converts them to MS CTM MIB requests
    // Need a separate thread to avoid blocking on CTM completion from
    // the main NBASE threads.
    state_t::routing_cfg_thr = sdk::event_thread::event_thread::factory(
        "routing_cfg", core::PDS_THREAD_ID_ROUTING_CFG,
        sdk::lib::THREAD_ROLE_CONTROL,
        0x0, // use all control cores
        init_routing_cfg_thr, // entry fn
        NULL, // exit fn
        NULL, // event callbak
        sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
        sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
        true /* can yield */, true /* sync ipc */);

    state_t::routing_cfg_thr->start(nullptr);
    return true;
}

void
hal_deinit (void)
{
    // Routing config thread is automatically cleaned up by HAL
    return;
}

bool
hal_hitless_upg_supp(void)
{
#define HITLESS_UPG_MIN_MEMORY 8 // 8G

    // TODO: revisit to call some higher level PDS API
    if (api::g_pds_state.catalogue()->memory_capacity() >= HITLESS_UPG_MIN_MEMORY) {
        // Hitless upgrade supported on 8G HBM
        return true;
    }
    return false;
}

bool
hal_is_upg_mode_hitless(void)
{
    if (unlikely(api::g_upg_state == nullptr)) {
        return false;
    }
    if (sdk::platform::sysinit_mode_hitless(api::g_upg_state->init_mode())) {
        // Or coming up gracefully
        return true;
    }
    return false;
}

} // End namespace
