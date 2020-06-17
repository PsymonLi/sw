//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// LI VXLAN Tunnel HAL integration
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
        PDS_TRACE_DEBUG ("MS If 0x%x: PDS MOCK MODE", ifindex);
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
    ips_info_.admin_state =
        (port_add_upd_ips->port_settings.port_enabled == ATG_YES);
    ips_info_.admin_state_updated =
        (port_add_upd_ips->port_settings.port_enabled_updated == ATG_YES);
    ips_info_.switchport =
        (port_add_upd_ips->port_settings.no_switch_port == ATG_NO);
    ips_info_.switchport_updated =
        (port_add_upd_ips->port_settings.no_switch_port_updated == ATG_YES);
}

bool li_intf_t::fetch_store_info_(pds_ms::state_t* state) {
    store_info_.phy_port_if_obj = state->if_store().get(ips_info_.ifindex);

    // If Store entry should have already been created in the Mgmt Stub 
    if (unlikely(store_info_.phy_port_if_obj == nullptr)) {

        if (mgmt_state_t::thread_context().state()->is_graceful_restart()) {
            auto new_if_obj = new pds_ms::if_obj_t(ips_info_.ifindex);
            state->if_store().add_upd(ips_info_.ifindex, new_if_obj);
            PDS_TRACE_DEBUG("Graceful restart snapshot replayed LI Port Add"
                            "MS 0x%x for unknown L3 interface",
                            ips_info_.ifindex);
            init_fault_status_(new_if_obj->phy_port_properties(), true);
            return false;
        }
        throw Error(std::string("LI Port AddUpdate for unknown MS IfIndex ")
                    .append(std::to_string(ips_info_.ifindex)));
    }

    if (!store_info_.phy_port_if_obj->phy_port_properties().spec_init) {
        PDS_TRACE_DEBUG("Graceful restart snapshot replayed LI Port Add"
                        "MS 0x%x for unknown L3 interface",
                        ips_info_.ifindex);
        return false;
    }

    if (!store_info_.phy_port_if_obj->phy_port_properties().hal_created) {
        // Obj was saved in store to cache the UUID and linux parameters
        // but not yet created in HAL - HAL PDS create now
        op_create_ = true;
    }
    return true;
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

void li_intf_t::enter_ms_ctxt_and_init_fault_status_(if_obj_t::phy_port_properties_t& phy_port_prop) {
    NBB_CREATE_THREAD_CONTEXT
    NBS_ENTER_SHARED_CONTEXT(li_proc_id);
    NBS_GET_SHARED_DATA();

    init_fault_status_(phy_port_prop);

    NBS_RELEASE_SHARED_DATA();
    NBS_EXIT_SHARED_CONTEXT();
    NBB_DESTROY_THREAD_CONTEXT
}

bool li_intf_t::cache_new_obj_in_cookie_(bool async) {
    std::unique_ptr<if_obj_t> new_if_obj;

    // Create a new object in order to store the updated fields
    // but do not save it in the Global State yet
    new_if_obj.reset(new if_obj_t(*(store_info_.phy_port_if_obj)));
    auto& phy_port_prop = new_if_obj->phy_port_properties();
    if (op_create_) {
        phy_port_prop.hal_created = true;
        phy_port_prop.admin_state = ips_info_.admin_state;
        phy_port_prop.switchport = ips_info_.switchport;
        PDS_TRACE_DEBUG ("MS If 0x%x: Admin State %d, Switchport %d",
                         ips_info_.ifindex, ips_info_.admin_state,
                         ips_info_.switchport);

        if (async) {
            init_fault_status_(phy_port_prop);
        } else {
            enter_ms_ctxt_and_init_fault_status_(phy_port_prop);
        }

    } else if (ips_info_.admin_state_updated || ips_info_.switchport_updated) {
        if (ips_info_.admin_state_updated) {
            PDS_TRACE_DEBUG ("MS If 0x%x: Admin State change to %d",
                             ips_info_.ifindex, ips_info_.admin_state);
            // Update the new admin state in the new If object
            phy_port_prop.admin_state = ips_info_.admin_state;
        }
        if (ips_info_.switchport_updated) {
            PDS_TRACE_DEBUG ("MS If 0x%x: Switchport State change to %d",
                             ips_info_.ifindex, ips_info_.switchport);
            // Update the new admin state in the new If object
            phy_port_prop.switchport = ips_info_.switchport;
        }
    } else {
        // Update request but no change in the fields we are
        // interested in
        PDS_TRACE_DEBUG("MS If 0x%x: No-op update", ips_info_.ifindex);
        return false;
    }
    // Update the local store info context so that the make_pds_spec
    // refers to the latest fields
    store_info_.phy_port_if_obj = new_if_obj.get();
    // Cache the new object in the cookie to revisit asynchronously
    // when the PDS API response is received
    cookie_uptr_->objs.push_back(std::move(new_if_obj));
    return true;
}

pds_obj_key_t li_intf_t::make_pds_if_key_(void) {
    return store_info_.phy_port_if_obj->phy_port_properties().l3_if_spec.key;
}

pds_if_spec_t li_intf_t::make_pds_if_spec_(void) {
    pds_if_spec_t spec =
        store_info_.phy_port_if_obj->phy_port_properties().l3_if_spec;
    auto& port_prop = store_info_.phy_port_if_obj->phy_port_properties();
    spec.admin_state =
        (port_prop.admin_state) ? PDS_IF_STATE_UP : PDS_IF_STATE_DOWN;
    memcpy (spec.l3_if_info.mac_addr, port_prop.mac_addr, ETH_ADDR_LEN);

    PDS_TRACE_INFO("Populate PDS IfSpec MS L3IfIndex 0x%x L3 UUID %s"
                   " Port UUID %s IPPrefix %s",
                   ips_info_.ifindex, spec.key.str(),
                   spec.l3_if_info.port.str(), ippfx2str(&spec.l3_if_info.ip_prefix));
    return spec;
}

pds_batch_ctxt_guard_t li_intf_t::make_batch_pds_spec_(bool async) {
    pds_batch_ctxt_guard_t bctxt_guard_;

    // Cookie should not be empty in case of async
    SDK_ASSERT(!async || cookie_uptr_);
    pds_batch_params_t bp {PDS_BATCH_PARAMS_EPOCH, 
                           (async) ? PDS_BATCH_PARAMS_ASYNC : false,
                           pds_ms::hal_callback,
                           (async) ? cookie_uptr_.get() : nullptr};
    auto bctxt = pds_batch_start(&bp);
    if (unlikely (!bctxt)) {
        throw Error(std::string("PDS Batch Start failed for MS If ")
                    .append(std::to_string(ips_info_.ifindex)));
    }
    bctxt_guard_.set (bctxt);

    if (op_delete_) { // Delete
        auto if_key = make_pds_if_key_();
        if (!PDS_MOCK_MODE()) {
            pds_if_delete(&if_key, bctxt);
        }
    } else { // Add or update
        auto if_spec = make_pds_if_spec_();
        sdk_ret_t ret = SDK_RET_OK;
        if (op_create_) {
            if (!PDS_MOCK_MODE()) {
                ret = pds_if_create(&if_spec, bctxt);
            }
        } else {
            if (!PDS_MOCK_MODE()) {
                ret = pds_if_update(&if_spec, bctxt);
            }
        }
        if (unlikely (ret != SDK_RET_OK)) {
            throw Error(std::string("PDS If Create or Update failed for MS If ")
                        .append(std::to_string(ips_info_.ifindex))
                        .append(" err=").append(std::to_string(ret)));
        }
    }
    return bctxt_guard_;
}

pds_batch_ctxt_guard_t li_intf_t::prepare_pds(state_t::context_t& state_ctxt,
                                             bool async) {

    auto pds_bctxt_guard = make_batch_pds_spec_(async); 

    // If we have batched multiple IPS earlier, flush it now
    // Cannot add Interface Create/Update to an existing batch
    state_ctxt.state()->flush_outstanding_pds_batch();
    return pds_bctxt_guard;
}

void li_intf_t::populate_clbk_(ATG_LIPI_PORT_ADD_UPDATE* port_add_upd_ips) {
    cookie_uptr_->send_ips_reply =
        [port_add_upd_ips] (bool pds_status, bool ips_mock) -> void {
            // ----------------------------------------------------------------
            // This block is executed asynchronously when PDS response is rcvd
            // ----------------------------------------------------------------
            if (unlikely(ips_mock)) return; // UT

            NBB_CREATE_THREAD_CONTEXT
            NBS_ENTER_SHARED_CONTEXT(li_proc_id);
            NBS_GET_SHARED_DATA();

            auto key = li::Port::get_key(*port_add_upd_ips);
            auto& port_store = li::Fte::get().get_lipi_join()->get_port_store();
            auto it = port_store.find(key);
            if (it == port_store.end()) {
                auto send_response =
                    li::Port::set_ips_rc(&port_add_upd_ips->ips_hdr,
                                         (pds_status) ? ATG_OK : ATG_UNSUCCESSFUL);
                SDK_ASSERT(send_response);
                PDS_TRACE_DEBUG("+++++++ Phy port 0x%x: Send Async IPS reply"
                                " %s stateless mode +++++++",
                                key.index, (pds_status) ? "Success" : "Failure");
                li::Fte::get().get_lipi_join()->
                    send_ips_reply(&port_add_upd_ips->ips_hdr);
            } else {
                if (pds_status) {
                    PDS_TRACE_DEBUG("Phy Port 0x%x: Send Async IPS Reply success stateful mode",
                                    key.index);
                    (*it)->update_complete(ATG_OK);
                } else {
                    PDS_TRACE_DEBUG("Phy Port 0x%x: Send Async IPS Reply failure stateful mode",
                                    key.index);
                    (*it)->update_failed(ATG_UNSUCCESSFUL);
                }
            }
            NBS_RELEASE_SHARED_DATA();
            NBS_EXIT_SHARED_CONTEXT();
            NBB_DESTROY_THREAD_CONTEXT
        };
}

NBB_BYTE li_intf_t::handle_intf_add_upd_(bool async,
                                         ATG_LIPI_PORT_ADD_UPDATE* port_add_upd_ips) {
    pds_batch_ctxt_guard_t  pds_bctxt_guard;
    NBB_BYTE rc = ATG_OK;

    // Alloc new cookie and cache IPS
    cookie_uptr_.reset (new cookie_t);

    { // Enter thread-safe context to access/modify global state
        auto state_ctxt = pds_ms::state_t::thread_context();
        if (!fetch_store_info_(state_ctxt.state())) {
            return rc;
        }

        if (op_create_) {
            PDS_TRACE_INFO ("MS If 0x%x: Create IPS", ips_info_.ifindex);
        } else {
            PDS_TRACE_INFO ("MS If 0x%x: Update IPS", ips_info_.ifindex);
        }
        if (unlikely(!cache_new_obj_in_cookie_(async))) {
            // No change
            PDS_TRACE_DEBUG ("MS If 0x%x: No-op IPS", ips_info_.ifindex);
            return rc;
        }

        pds_bctxt_guard = prepare_pds(state_ctxt, async);
    } // End of state thread_context
      // Do Not access/modify global state after this

    if (async) {
        populate_clbk_(port_add_upd_ips);
    }

    // All processing complete, only batch commit remains -
    // safe to release the cookie_uptr_ unique_ptr
    rc = ATG_ASYNC_COMPLETION;
    auto cookie = cookie_uptr_.release();
    auto ret = learn::api_batch_commit(pds_bctxt_guard.release());
    if (unlikely (ret != SDK_RET_OK)) {
        delete cookie;
        throw Error(std::string("Batch commit failed for Add-Update Port MS If ")
                    .append(std::to_string(ips_info_.ifindex))
                    .append(" err=").append(std::to_string(ret)));
    }
    PDS_TRACE_DEBUG ("MS If 0x%x: Add/Upd PDS Batch commit successful",
                     ips_info_.ifindex);
    if (!async) {
        PDS_TRACE_DEBUG("++++ Phy port 0x%x: Created in HAL ++++", ips_info_.ifindex);
    }

    if (PDS_MOCK_MODE() && async) {
        // Call the HAL callback in PDS mock mode
        std::thread cb(pds_ms::hal_callback, SDK_RET_OK, cookie);
        cb.detach();
    }
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

NBB_BYTE li_intf_t::l3_intf_create(ms_ifindex_t ms_ifindex) {
    ips_info_.ifindex = ms_ifindex;
    return handle_intf_add_upd_(false, nullptr);
}

void li_intf_t::handle_delete(NBB_ULONG ifindex) {
    pds_batch_ctxt_guard_t  pds_bctxt_guard;
    op_delete_ = true;

    // MS stub Integration APIs do not support Async callback for deletes.
    // However since we should not block the MS NBase main thread
    // the HAL processing is always asynchronous even for deletes.
    // Assuming that Deletes never fail the Store is also updated
    // in a synchronous fashion for deletes so that it is in sync
    // if there is a subsequent create from MS.

    ips_info_.ifindex = ifindex;
    PDS_TRACE_INFO ("MS If 0x%x: Delete IPS no-op", ips_info_.ifindex);
}

sdk_ret_t li_intf_t::update_pds_ipaddr(NBB_ULONG ms_ifindex) {
    pds_batch_ctxt_guard_t  pds_bctxt_guard;
    {
        // Only L3 interface address can be updated. Send the current
        // interface address to Metaswitch. If address is same then metaswitch
        // will treat it as a no-op
        auto state_ctxt = pds_ms::state_t::thread_context();
        auto if_obj = state_ctxt.state()->if_store().get(ms_ifindex);
        if (if_obj == nullptr) {
            return SDK_RET_OK;
        }
        auto& phy_port_prop = if_obj->phy_port_properties();
        if (!phy_port_prop.hal_created) {
            PDS_TRACE_DEBUG("MS IfIndex 0x%x Ignore IP Update before HAL Create", 
                            ms_ifindex);
            return SDK_RET_OK;
        }
        PDS_TRACE_DEBUG("MS IfIndex 0x%x IP update", ms_ifindex);

        ips_info_.ifindex = ms_ifindex;
        store_info_.phy_port_if_obj = if_obj;
        pds_bctxt_guard = prepare_pds(state_ctxt, false /* synchronous */);

        // This is a synchronous batch commit.
        // Ensure that state lock is released to avoid blocking NBASE thread
    } // End of state thread_context. Do Not access/modify global state

    auto ret = learn::api_batch_commit(pds_bctxt_guard.release());
    if (unlikely (ret != SDK_RET_OK)) {
        PDS_TRACE_ERR ("MS IfIndex 0x%x: Add/Upd IP address Batch commit"
                       "failed %d", ips_info_.ifindex, ret);
        return ret;
    }

    PDS_TRACE_DEBUG ("MS IfIndex 0x%x: Add/Upd IP address Batch commit successful",
                     ips_info_.ifindex);
    return SDK_RET_OK;
}

sdk_ret_t li_intf_update_pds_ipaddr (NBB_ULONG ms_ifindex)
{
    try {
        li_intf_t intf;
        return intf.update_pds_ipaddr(ms_ifindex);
    } catch (Error& e) {
        PDS_TRACE_ERR ("Interface IP address Update processing failed %s", e.what());
        return SDK_RET_ERR;
    }
}
} // End namespace
