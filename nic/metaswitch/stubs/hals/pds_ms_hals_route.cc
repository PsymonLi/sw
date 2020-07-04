//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// LI VXLAN Tunnel HAL integration
//---------------------------------------------------------------

#include <thread>
#include <nbase.h>
#include "nic/metaswitch/stubs/hals/pds_ms_hals_timer.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hals_route.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_ip_track_hal.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_ifindex.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_tbl_idx.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hals_utils.hpp"
#include "nic/apollo/api/internal/pds_route.hpp"
#include "nic/sdk/lib/logger/logger.hpp"
#include "nic/apollo/learn/learn_api.hpp"
#include <hals_c_includes.hpp>
#include <hals_ropi_slave_join.hpp>
#include <hals_route.hpp>


namespace pds_ms {

using pds_ms::Error;

void hals_route_t::populate_route_id(ATG_ROPI_ROUTE_ID* route_id) {
    // Populate prefix
    memset(&ips_info_.pfx, 0, sizeof(ip_prefix_t));
    ATG_INET_ADDRESS& ms_addr = route_id->destination_address;
    ms_to_pds_ipaddr(ms_addr, &ips_info_.pfx.addr);
    ips_info_.pfx.len = route_id->prefix_length;
    // Populate VRF
    ips_info_.vrf_id = vrfname_2_vrfid(route_id->vrf_name,
                                       route_id->vrf_name_len);
}

bool hals_route_t::parse_ips_info_(ATG_ROPI_UPDATE_ROUTE* add_upd_route_ips) {
    populate_route_id(&add_upd_route_ips->route_id);
    // Populate the correlator
    NBB_CORR_GET_VALUE(ips_info_.pathset_id, add_upd_route_ips->
                       route_properties.pathset_id);
    NBB_CORR_GET_VALUE(ips_info_.ecmp_id, add_upd_route_ips->
                       route_properties.dp_pathset_correlator);
    return true;
}

pds_obj_key_t hals_route_t::make_pds_rttable_key_(state_t* state) {
    auto vpc_obj = state->vpc_store().get(ips_info_.vrf_id);
    if (unlikely(vpc_obj == nullptr)) {
        throw Error(std::string("Cannot find VPC store obj for id ").
                    append(std::to_string(ips_info_.vrf_id)));
    }
    if (unlikely(!vpc_obj->hal_created())) {
        PDS_TRACE_DEBUG("VRF %d has already been deleted", ips_info_.vrf_id);
        return rttbl_key_;
    }
    rttbl_key_ = vpc_obj->properties().vpc_spec.v4_route_table;
    return (rttbl_key_);
}

static void
make_pds_rttable_spec(state_t* state, pds_route_table_spec_t &rttable,
                       const pds_obj_key_t& rttable_key) {
    memset(&rttable, 0, sizeof(pds_route_table_spec_t));
    rttable.key = rttable_key;

    auto rttbl_store = state->route_table_store().get(rttable.key);
    SDK_ASSERT(rttbl_store != nullptr);
    // Get the routes pointer. PDS API will make a copy of the
    // route table and free it up once api processing is complete
    // after batch commit
    rttable.route_info = rttbl_store->routes();
    return;
}

bool
hals_route_t::update_route_store_(state_t* state,
                                  const pds_obj_key_t& rttable_key) {
    bool ret;

    // Populate the new route
    route_.attrs.prefix = ips_info_.pfx;
    route_.attrs.nh_type = PDS_NH_TYPE_OVERLAY_ECMP;
    route_.attrs.nh_group = msidx2pdsobjkey(ips_info_.ecmp_id);

    auto rttbl_store = state->route_table_store().get(rttable_key);
    SDK_ASSERT(rttbl_store != nullptr);

    if (!op_delete_) {
        auto rt = rttbl_store->get_route(route_.attrs.prefix);
        if (rt == nullptr) {
            op_create_ = true;
        }
        // Add or update the route in the store
        ret = rttbl_store->add_upd_route(route_);
    } else {
        // Delete is a route table update with the deleted route.
        // The route table is ONLY deleted when VRF gets deleted
        // Delete the route from the store
        ret = rttbl_store->del_route(route_.attrs.prefix);
    }
    if (!ret) {
        // Start the batch commit timer for this route-table
        // only if the batch is not yet ready to be committed
        timer_start(state->get_route_timer_list(),
                    rttbl_store->batch_commit_timer(),
                    PDS_MS_COMMIT_BATCH_TIMER_MS,
                    ips_info_.vrf_id);
    }

    return ret;
}

pds_batch_ctxt_guard_t
process_route_batch(state_t *state, const pds_obj_key_t& rttable_key,
                    std::unique_ptr<cookie_t>& cookie_uptr)
{
    sdk_ret_t ret;
    pds_route_table_spec_t rttbl_spec;
    pds_batch_ctxt_guard_t bctxt_guard;

    // If num routes exceeds capacity then batch commit will be skipped
    // and pushed later when the number of routes goes below the route
    // table capacity
    auto rttbl_store = state->route_table_store().get(rttable_key);
    SDK_ASSERT(rttbl_store != nullptr);

    int num_routes = rttbl_store->num_routes();
    if (num_routes > PDS_MS_MAX_NUM_ROUTES) {
        PDS_TRACE_ERR("Num routes %d is greater than max %d. Skipping "
                      "batch commit for rttable: %s", num_routes,
                       PDS_MS_MAX_NUM_ROUTES, rttable_key.str());
        // Return empty guard
        return bctxt_guard;
    }
    if (!rttbl_store->pending_batch_sz()) {
        PDS_TRACE_DEBUG("No pending routes. Skipping batch commit "
                        "for rttable: %s", rttable_key.str());
        // Return empty guard
        return bctxt_guard;
    }
    PDS_TRACE_DEBUG("Preparing batch. num_routes %d pending_count %d rttbl %s",
                    num_routes, rttbl_store->pending_batch_sz(),
                    rttable_key.str());
    make_pds_rttable_spec(state, rttbl_spec, rttable_key);

    SDK_ASSERT(cookie_uptr); // Cookie should not be empty
    pds_batch_params_t bp {PDS_BATCH_PARAMS_EPOCH, PDS_BATCH_PARAMS_ASYNC,
                           pds_ms::hal_callback,
                           cookie_uptr.get()};
    auto bctxt = pds_batch_start(&bp);
    if (unlikely (!bctxt)) {
        PDS_TRACE_ERR("PDS Batch Start failed");
        return bctxt_guard;
    }
    if (!PDS_MOCK_MODE()) {
        ret = pds_route_table_update(&rttbl_spec, bctxt);
    }
    if (unlikely (ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("PDS Route Update failed. ret %s",
                      std::to_string(ret).c_str());
        return bctxt_guard;
    }
    bctxt_guard.set(bctxt);

    return bctxt_guard;
}

pds_batch_ctxt_guard_t hals_route_t::make_batch_pds_spec_(state_t* state,
                                                          const pds_obj_key_t&
                                                          rttable_key) {
    pds_batch_ctxt_guard_t bctxt_guard;

    if (!update_route_store_(state, rttable_key)) {
        // Skip batch processing if batch is not ready to be committed
        // Return empty guard
        return bctxt_guard;
    }

    return (process_route_batch(state, rttable_key,
                                cookie_uptr_));
}

sdk_ret_t hals_route_t::underlay_route_add_upd_() {
    bool tracked = false;
    pds_obj_key_t pds_obj_key;
    ip_addr_t destip;
    obj_id_t pds_obj_id;

    {
        auto state_ctxt = state_t::thread_context();
        auto state = state_ctxt.state();

        auto it_internal = state->ip_track_internalip_store().find(ips_info_.pfx);
        if (it_internal != state->ip_track_internalip_store().end()) {
            auto ip_track_obj =
                state->ip_track_store().get(it_internal->second);
            pds_obj_key = ip_track_obj->pds_obj_key();
            destip = ip_track_obj->destip();
            pds_obj_id = ip_track_obj->pds_obj_id();
            if (ip_track_obj->deleted()) {
                PDS_TRACE_DEBUG("Ignore route update for delete ip rtacked object");
                return SDK_RET_OK;
            }
            tracked = true;

            auto nhgroup_id =
                state_lookup_indirect_ps_and_map_ip(state,ips_info_.pathset_id,
                                                    destip, false, pds_obj_key);
            ip_track_obj->set_indirect_ps_id(ips_info_.pathset_id);
            ips_info_.ecmp_id = nhgroup_id;
        }
    }

    if (ips_info_.ecmp_id == PDS_MS_ECMP_INVALID_INDEX) {
        return SDK_RET_OK;
    }
    if (tracked) {
        return ip_track_reachability_change(pds_obj_key, destip,
                                            ips_info_.ecmp_id, pds_obj_id);
    }

    // No underlay route programming to HAL.
    // Underlay IP route programming to HAL requires Squashed mode to get the
    // correct NH Group.
    // Refer comment in pds_ms_hals_l3.cc
    return SDK_RET_OK;
}

sdk_ret_t hals_route_t::underlay_route_del_() {
    {
        auto state_ctxt = state_t::thread_context();
        auto state = state_ctxt.state();

        auto it_internal = state->ip_track_internalip_store().find(ips_info_.pfx);
        if (it_internal != state->ip_track_internalip_store().end()) {
            auto ip_track_obj =
                state->ip_track_store().get(it_internal->second);
            if (ip_track_obj != nullptr) {
                auto& pds_obj_key = ip_track_obj->pds_obj_key();
                auto indirect_ps_id = ip_track_obj->indirect_ps_id();
                auto indirect_ps_obj =
                    state->indirect_ps_store().get(indirect_ps_id);
                if (indirect_ps_obj != nullptr) {
                    // Remove from back-ref:
                    // indirect pathset -> list of ip-track obj uuids
                    PDS_TRACE_DEBUG("Reset IP track Obj %s from indirect pathset %d",
                                    pds_obj_key.str(), indirect_ps_id);
                    indirect_ps_obj->del_ip_track_obj(pds_obj_key);
                    ip_track_obj->set_indirect_ps_id(PDS_MS_ECMP_INVALID_INDEX);
                    // TODO: HAL objects associated with tracked Dest IPs need to be
                    // black-holed when the underlay reachability is lost ??
                    // We can also reach here when tracking is deleted for an object
                    // in which case nothing to do here. Need to differentiate between
                    // these 2 cases.
                }
                if (!ip_track_obj->deleted()) {
                    // Reachability lost as opposed to IP track object delete.
                    // Need to blackhole the object.
                    ip_track_reachability_change(pds_obj_key, ip_track_obj->destip(),
                                                 PDS_MS_ECMP_INVALID_INDEX, 
                                                 ip_track_obj->pds_obj_id());
                }
            }
        }
    }
    return SDK_RET_OK;
}

static void
batch_commit_route_table (state_t *state, pds_batch_ctxt_guard_t &bctxt_guard,
                          pds_obj_key_t &rttbl_key,
                          std::unique_ptr<cookie_t>& cookie_uptr)
{
    auto l_rttbl_key_ = rttbl_key;
    cookie_uptr->send_ips_reply =
        [l_rttbl_key_] (bool pds_status, bool ips_mock) -> void {
            // ----------------------------------------------------------------
            // This block is executed asynchronously when PDS response is rcvd
            // ----------------------------------------------------------------
            PDS_TRACE_DEBUG ("++++ MS Route Batch Commit for Route table: %s "
                             "Async reply %s", l_rttbl_key_.str(),
                             (pds_status) ? "Success" : "Failure");
        };

    // All processing complete, only batch commit remains -
    // safe to release the cookie_uptr unique_ptr
    auto cookie = cookie_uptr.release();
    auto ret = learn::api_batch_commit(bctxt_guard.release());
    if (unlikely (ret != SDK_RET_OK)) {
        delete cookie;
        PDS_TRACE_ERR("Batch commit failed for rttbl %s err %s",
                      rttbl_key.str(), (std::to_string(ret)).c_str());
        return;
    }
    PDS_TRACE_DEBUG ("Route-Table %s PDS Batch commit successful",
                     rttbl_key.str());
    auto rttbl_store = state->route_table_store().get(rttbl_key);
    SDK_ASSERT(rttbl_store != nullptr);

    // Commit successful. Stop timers
    timer_stop(state->get_route_timer_list(),
               rttbl_store->batch_commit_timer());
    // Reset pending batch
    rttbl_store->reset_pending_batch_sz();

    if (PDS_MOCK_MODE()) {
        // Call the HAL callback in PDS mock mode
        std::thread cb(pds_ms::hal_callback, SDK_RET_OK, cookie);
        cb.detach();
    }
    return;
}

void hals_route_t::overlay_route_del_() {
    pds_obj_key_t rttable_key;
    pds_batch_ctxt_guard_t pds_bctxt_guard;

    op_delete_ = true;
    { // Enter thread-safe context to access/modify global state
        auto state_ctxt = pds_ms::state_t::thread_context();
        // Empty cookie
        cookie_uptr_.reset(new cookie_t);
        rttable_key = make_pds_rttable_key_(state_ctxt.state());
        if (is_pds_obj_key_invalid(rttable_key)) {
            PDS_TRACE_DEBUG("Ignore MS route delete for VRF %d that does not"
                            " have Route table ID", ips_info_.vrf_id);
            return;
        }
        pds_bctxt_guard = make_batch_pds_spec_(state_ctxt.state(),
                                               rttable_key);
        if (!pds_bctxt_guard) {
            PDS_TRACE_DEBUG("%s route delete added to pending batch",
                             ippfx2str(&ips_info_.pfx));
            return;
        }
        // If we have batched multiple IPS earlier flush it now
        // Cannot add Subnet Delete to an existing batch
        state_ctxt.state()->flush_outstanding_pds_batch();

        // Routes will only get committed once the batch timer expires OR
        // we accumulate PDS_MS_COMMIT_BATCH_SIZE number of routes
        if (pds_bctxt_guard) {
            batch_commit_route_table(state_ctxt.state(), pds_bctxt_guard,
                                     rttable_key, cookie_uptr_);
        }

    } // End of state thread_context
      // Do Not access/modify global state after this
    return;
}

NBB_BYTE
hals_route_t::handle_add_upd_ips(ATG_ROPI_UPDATE_ROUTE* add_upd_route_ips)
{
    pds_obj_key_t rttable_key;
    pds_batch_ctxt_guard_t pds_bctxt_guard;
    NBB_BYTE rc = ATG_OK;

    parse_ips_info_(add_upd_route_ips);

    if ((add_upd_route_ips->route_properties.type == ATG_ROPI_ROUTE_CONNECTED) ||
        (add_upd_route_ips->route_properties.type == ATG_ROPI_ROUTE_LOCAL_ADDRESS)) {
        PDS_TRACE_DEBUG("Ignore connected prefix %s route add",
                        ippfx2str(&ips_info_.pfx));
        // If overlay BGP peering is formed before subnet is configured locally
        // the GW IP address would have been advertised as Type 2 from remote DSC
        // and installed as a /32 route. Delete this route when it is identified
        // as a connected IP.
        if (ips_info_.vrf_id != PDS_MS_DEFAULT_VRF_ID) {
            overlay_route_del_();
        }
        return rc;
    }

    PDS_TRACE_DEBUG("Route Add IPS VRF %d Prefix %s Type %d Pathset %d"
                    " %s NHgroup %d",
                     ips_info_.vrf_id, ippfx2str(&ips_info_.pfx),
                     add_upd_route_ips->route_properties.type,
                     ips_info_.pathset_id,
                     (ips_info_.vrf_id == PDS_MS_DEFAULT_VRF_ID) ? "underlay" : "overlay",
                     ips_info_.ecmp_id);

    if (ips_info_.vrf_id == PDS_MS_DEFAULT_VRF_ID) {
        // We should not reach here in Overlay routing mode.
        // But Underlay only control-plane model requires user configured
        // VXLAN Tunnels to be stitched to underlay nexthop group.
        // Until the Metaswitch support for configured VXLAN tunnels
        // is available the stop-gap solution is to push underlay routes to
        // HAL API thread and have it stitch the TEPs to the Underlay NH groups.
        //
        // TODO: Special case - temporarily push underlay routes
        // to the internal API for walking and stitching TEPs.
        // This check to be removed later allowing it to fall through to
        // async batch mode when HAL API thread starts supporting
        // TEP stitching natively.
        if (underlay_route_add_upd_() != SDK_RET_OK) {
            PDS_TRACE_ERR("Underlay Route add VRF %d Prefix %s failed",
                          ips_info_.vrf_id, ippfx2str(&ips_info_.pfx));
        }
        return ATG_OK;
    }

    if (ips_info_.ecmp_id == PDS_MS_ECMP_INVALID_INDEX) {
        // Can happen when the L2F UpdateRoutersMAC from EVPN is delayed
        // because of which PSM cannot fetch ARP MAC from NAR stub
        // and the ROPI route update comes with black-holed pathset
        PDS_TRACE_DEBUG("Ignore prefix route %s with black-holed pathset %d",
                        ippfx2str(&ips_info_.pfx), ips_info_.pathset_id);
        return rc;
    }

    { // Enter thread-safe context to access/modify global state
        // Empty cookie
        cookie_uptr_.reset(new cookie_t);
        auto state_ctxt = pds_ms::state_t::thread_context();
        rttable_key = make_pds_rttable_key_(state_ctxt.state());
        if (is_pds_obj_key_invalid(rttable_key)) {
            PDS_TRACE_DEBUG("Ignore MS route for VRF %d that does not"
                            " have Route table ID", ips_info_.vrf_id);
            return rc;
        }
        pds_bctxt_guard = make_batch_pds_spec_(state_ctxt.state(),
                                               rttable_key);
        if (!pds_bctxt_guard) {
            PDS_TRACE_VERBOSE("%s route add/update added to pending batch",
                              ippfx2str(&ips_info_.pfx));
            return rc;
        }

        // Routes will only get committed once the batch timer expires OR
        // we accumulate PDS_MS_COMMIT_BATCH_SIZE number of routes
        if (pds_bctxt_guard) {
            batch_commit_route_table(state_ctxt.state(), pds_bctxt_guard,
                                     rttable_key, cookie_uptr_);
        }
    } // End of state thread_context
      // Do Not access/modify global state after this
    return rc;
}

void hals_route_t::handle_delete(ATG_ROPI_ROUTE_ID route_id) {
    op_delete_ = true;

    // MS stub Integration APIs do not support Async callback for deletes.
    // However since we should not block the MS NBase main thread
    // the HAL processing is always asynchronous even for deletes.
    // Assuming that Deletes never fail the Store is also updated
    // in a synchronous fashion for deletes so that it is in sync
    // if there is a subsequent create from MS.

    populate_route_id(&route_id);
    {
        auto state_ctxt = pds_ms::state_t::thread_context();
        if (state_ctxt.state()->reset_ignored_prefix(ips_info_.pfx)) {
            PDS_TRACE_DEBUG("Ignore connected prefix %s route delete",
                            ippfx2str(&ips_info_.pfx));
            return;
        }
    }
    PDS_TRACE_INFO ("MS Route %s: Delete IPS", ippfx2str(&ips_info_.pfx));

    if (ips_info_.vrf_id == PDS_MS_DEFAULT_VRF_ID) {
        underlay_route_del_();
        return;
    }

    overlay_route_del_();
}

// Route timer expiry callback function
void
route_timer_expiry_cb (NTL_TIMER_LIST_CB *list_cb, NTL_TIMER_CB *timer_cb)
{
    uint32_t vrf_id;
    pds_batch_ctxt_guard_t bctxt_guard;
    pds_obj_key_t rttable_key = {0};
    std::unique_ptr<cookie_t> cookie_uptr;

    // Action correlator contains the vrf-id
    vrf_id = timer_cb->action_correlator;
    PDS_TRACE_DEBUG("Route Batch commit timer expired for VRF %d", vrf_id);

    {
        auto state_ctxt = pds_ms::state_t::thread_context();
        auto vpc_obj = state_ctxt.state()->vpc_store().get(vrf_id);
        if (unlikely(vpc_obj == nullptr)) {
            PDS_TRACE_ERR("Cannot find VPC store obj for vrf_id: %d", vrf_id);
            return;
        }
        if (unlikely(!vpc_obj->hal_created())) {
            PDS_TRACE_DEBUG("VRF %d has already been deleted", vrf_id);
            return;
        }
        rttable_key = vpc_obj->properties().vpc_spec.v4_route_table;

        cookie_uptr.reset(new cookie_t);
        bctxt_guard = process_route_batch(state_ctxt.state(), rttable_key,
                                          cookie_uptr);
        if (!bctxt_guard) {
            PDS_TRACE_DEBUG("Skip batch commit for vrf_id: %d", vrf_id);
            return;
        }

        batch_commit_route_table(state_ctxt.state(), bctxt_guard,
                                 rttable_key, cookie_uptr);
    } // End of state thread_context
      // Do Not access/modify global state after this

    return;
}

} // End namespace
