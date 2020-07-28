//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// MS LI stub HAL integration for uplink L3 interface
//---------------------------------------------------------------

#include <thread>
#include "nic/apollo/api/internal/pds_if.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_li_intf.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_linux_util.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_ifindex.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/sdk/lib/logger/logger.hpp"
#include "nic/apollo/learn/learn_api.hpp"
#include <li_fte.hpp>
#include <li_lipi_slave_join.hpp>
#include <li_port.hpp>

extern NBB_ULONG li_proc_id;

namespace pds_ms {

static int fetch_port_fault_status (ms_ifindex_t &ifindex) {
    sdk_ret_t ret;
    pds_if_info_t info = {0};

    if (PDS_MOCK_MODE()) {
        PDS_TRACE_DEBUG ("MS If 0x%x: Mock oper up in PDS MOCK MODE", ifindex);
        return ATG_FRI_FAULT_NONE;
    }

    auto eth_ifindex = ms_to_pds_eth_ifindex(ifindex);
    ret = api::pds_if_read(&eth_ifindex, &info);
    if (unlikely (ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("%s", (std::string("PDS If Get failed for Eth If ")
                    .append(std::to_string(eth_ifindex))
                    .append(" MS If ")
                    .append(std::to_string(ifindex))
                    .append(" err=").append(std::to_string(ret))).c_str());
        return ATG_FRI_FAULT_NONE;
    }
    if (info.status.state == PDS_IF_STATE_DOWN) {
        PDS_TRACE_DEBUG("MS If 0x%x: Port DOWN", ifindex);
        return ATG_FRI_FAULT_PRESENT;
    } else if (info.status.state == PDS_IF_STATE_UP) {
        PDS_TRACE_DEBUG("MS If 0x%x: Port UP", ifindex);
        return ATG_FRI_FAULT_NONE;
    }

    /* Invalid state. Should not come here. Indicate fault */
    PDS_TRACE_DEBUG("MS If 0x%x: Port state invalid", ifindex);
    return ATG_FRI_FAULT_PRESENT;
}

void li_intf_t::parse_ips_info_(ATG_LIPI_PORT_ADD_UPDATE* port_add_upd_ips) {
    ips_info_.ifindex = port_add_upd_ips->id.if_index;
    ips_info_.if_name = port_add_upd_ips->id.if_name;
}

bool li_intf_t::fetch_store_info_(pds_ms::state_t* state) {
    store_info_.phy_port_if_obj = state->if_store().get(ips_info_.ifindex);
    PDS_TRACE_DEBUG("MS If 0x%x: Fetched obj %p", ips_info_.ifindex,
                    store_info_.phy_port_if_obj);

    // intf store entry should have already been created in the mgmt stub
    // unless this is being restored from the CTM snapshot during upgrade
    if (unlikely(store_info_.phy_port_if_obj == nullptr)) {

        if (mgmt_state_t::thread_context().state()->is_upg_ht_in_progress()) {
            auto new_if_obj = new pds_ms::if_obj_t(ips_info_.ifindex);

            // this is from the CTM snapshot replay, but we dont have
            // L3 interface in HAL
            // save in store to indicate that this interface is already
            // created in MS
            state->if_store().add_upd(ips_info_.ifindex, new_if_obj);
            PDS_TRACE_DEBUG("MS 0x%x hitless upg CTM snapshot replayed interface",
                            ips_info_.ifindex);
            // force link down in MS to prevent it from downloading nexthops
            // when HAL is not ready with L3 interface yet
            init_fault_status_(new_if_obj->phy_port_properties(), true);
            return false;
        }
        throw Error(std::string("LI Port AddUpdate for unknown MS IfIndex ")
                    .append(std::to_string(ips_info_.ifindex)));
    }

    if (!store_info_.phy_port_if_obj->phy_port_properties().mgmt_spec_init) {
        PDS_TRACE_DEBUG("Graceful restart snapshot replayed LI Port Add"
                        "MS 0x%x for unknown L3 interface",
                        ips_info_.ifindex);
        return false;
    }

    // has this interface been initialized by LI stub
    return !(store_info_.phy_port_if_obj->phy_port_properties().li_stub_init);
}

void li_intf_t::init_fault_status_(if_obj_t::phy_port_properties_t& phy_port_prop,
                                   bool force_fault) {
    // Create the FRI worker
    ntl::Frl &frl = li::Fte::get().get_frl();
    auto& fw = phy_port_prop.fri_worker;
    if (fw == nullptr) {
        fw = frl.create_fri_worker(ips_info_.ifindex);
        PDS_TRACE_DEBUG("MS If 0x%x: Created FRI worker", ips_info_.ifindex);
    }
    // Set the initial interface state. The port event subscribe is already
    // done by this point. Any events after creating worker and setting
    // initial state is handled by the port event callback
    FRL_FAULT_STATE fault_state;
    frl.init_fault_state(&fault_state);
    if (force_fault) {
        fault_state.hw_link_faults = ATG_FRI_FAULT_PRESENT;
    } else {
        fault_state.hw_link_faults = fetch_port_fault_status(ips_info_.ifindex);
    }
    frl.send_fault_ind(fw, &fault_state);
}

void li_intf_t::enter_ms_ctxt_and_init_fault_status_(if_obj_t::phy_port_properties_t&
                                                     phy_port_prop) {
    NBB_CREATE_THREAD_CONTEXT
    NBS_ENTER_SHARED_CONTEXT(li_proc_id);
    NBS_GET_SHARED_DATA();

    init_fault_status_(phy_port_prop);

    NBS_RELEASE_SHARED_DATA();
    NBS_EXIT_SHARED_CONTEXT();
    NBB_DESTROY_THREAD_CONTEXT
}

static void
terminate_frl_ (state_t* state, ms_ifindex_t ms_ifindex)
{
    auto& phy_port_prop = state->if_store().get(ms_ifindex)->phy_port_properties();
    ntl::Frl &frl = li::Fte::get().get_frl();
    auto& fw = phy_port_prop.fri_worker;
    if (fw != nullptr) {
        PDS_TRACE_DEBUG("Terminate FRL worker for MS Ifndex 0x%x", ms_ifindex);
        frl.terminate_fri_worker(fw);
    }
}

static void
enter_ms_ctxt_and_terminate_frl_ (state_t* state, ms_ifindex_t ms_ifindex)
{
    NBB_CREATE_THREAD_CONTEXT
    NBS_ENTER_SHARED_CONTEXT(li_proc_id);
    NBS_GET_SHARED_DATA();

    terminate_frl_(state, ms_ifindex);

    NBS_RELEASE_SHARED_DATA();
    NBS_EXIT_SHARED_CONTEXT();
    NBB_DESTROY_THREAD_CONTEXT
}

void li_intf_t::init_if_(bool async) {

    auto& phy_port_prop = store_info_.phy_port_if_obj->phy_port_properties();
    phy_port_prop.li_stub_init = true;
    if (async) {
        // already in LI stub context
        init_fault_status_(phy_port_prop);
    } else {
        enter_ms_ctxt_and_init_fault_status_(phy_port_prop);
    }
}

NBB_BYTE li_intf_t::handle_intf_add_upd_(bool async,
                                         ATG_LIPI_PORT_ADD_UPDATE* port_add_upd_ips) {
    pds_batch_ctxt_guard_t  pds_bctxt_guard;
    NBB_BYTE rc = ATG_OK;

    { // Enter thread-safe context to access/modify global state
        auto state_ctxt = pds_ms::state_t::thread_context();
        if (!fetch_store_info_(state_ctxt.state())) {
            // no-op return
            return rc;
        }
        PDS_TRACE_INFO ("MS If 0x%x: Create IPS", ips_info_.ifindex);

        init_if_(async);
    } // End of state thread_context
      // Do Not access/modify global state after this
    return rc;
}

NBB_BYTE li_intf_t::handle_add_upd_ips(ATG_LIPI_PORT_ADD_UPDATE* port_add_upd_ips) {
    parse_ips_info_(port_add_upd_ips);

    if (ms_ifindex_to_pds_type (ips_info_.ifindex) != IF_TYPE_L3) {
        // Nothing to do for non-L3 interfaces
        return ATG_OK;
    }
    if (port_add_upd_ips->port_settings.no_switch_port == ATG_NO) {
        // Only processing L3 port creates
        return ATG_OK;
    }

    return handle_intf_add_upd_(true, port_add_upd_ips);
}

void li_intf_t::intf_delete(ms_ifindex_t ms_ifindex, bool li_ip_deleted) {
    auto state_ctxt = pds_ms::state_t::thread_context();
    auto if_obj = state_ctxt.state()->if_store().get(ms_ifindex);

    if (if_obj == nullptr) {
        return;
    }
    auto& port_prop = if_obj->phy_port_properties();

    // intf has been deleted from mgmt
    // also LI stub has deleted IP in linux
    if ((port_prop.mgmt_spec_init == false) && 
        (li_ip_deleted || (port_prop.has_ip() == false))) {
        if (li_ip_deleted) {
            // we are already in li stub
            terminate_frl_(state_ctxt.state(), ms_ifindex);
        } else {
            enter_ms_ctxt_and_terminate_frl_(state_ctxt.state(), ms_ifindex);
        }
        PDS_TRACE_DEBUG("MSIfIndex 0x%x Interface %s delete",
                        ms_ifindex, port_prop.l3_if_spec.key.str());
        // erase interface from store
        state_ctxt.state()->if_store().erase(ms_ifindex);
    }
}

NBB_BYTE li_intf_t::intf_create(ms_ifindex_t ms_ifindex) {
    ips_info_.ifindex = ms_ifindex;
    return handle_intf_add_upd_(false, nullptr);
}

} // End namespace
