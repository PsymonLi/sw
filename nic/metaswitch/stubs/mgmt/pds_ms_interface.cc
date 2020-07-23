// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
#include "nic/apollo/api/utils.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_interface.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_utils.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/metaswitch/stubs/mgmt/gen/mgmt/pds_ms_internal_utils_gen.hpp"
#include "gen/proto/internal.pb.h"
#include "nic/metaswitch/stubs/common/pds_ms_if_store.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_linux_util.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_ifindex.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_defs.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_li_intf.hpp"

namespace pds_ms {

static void
populate_lim_l3_intf_cfg_spec ( LimInterfaceCfgSpec& req,
                                uint32_t ifindex, uint32_t enable)
{
    req.set_entityindex (PDS_MS_LIM_ENT_INDEX);
    req.set_ifindex (ifindex);
    req.set_ifenable (enable);
    req.set_ipv4enabled (AMB_TRISTATE_TRUE);
    req.set_ipv4fwding (AMB_TRISTATE_TRUE);
    req.set_ipv6enabled (AMB_TRISTATE_TRUE);
    req.set_ipv6fwding (AMB_TRISTATE_TRUE);
    req.set_fwdingmode (AMB_LIM_FWD_MODE_L3);
}

void
populate_lim_addr_spec (ip_prefix_t           *ip_prefix,
                        LimInterfaceAddrSpec& req,
                        uint32_t              if_type,
                        uint32_t              if_id)
{
    auto ifipaddr = req.mutable_ipaddr();

    req.set_iftype ((LimIntfType)if_type);
    req.set_ifid (if_id);
    req.set_prefixlen (ip_prefix->len);

    if (ip_prefix->addr.af == IP_AF_IPV4) {
        ifipaddr->set_af(types::IP_AF_INET);
        ifipaddr->set_v4addr(ip_prefix->addr.addr.v4_addr);
    } else {
        ifipaddr->set_af(types::IP_AF_INET6);
        ifipaddr->set_v6addr(ip_prefix->addr.addr.v6_addr.addr8,
                             IP6_ADDR8_LEN);
    }
}

// In addition to interface uuid fetching, this API also marks the
// uuid object to be pending deleted in case of delete operation
static interface_uuid_obj_t::info_t
interface_uuid_fetch(const pds_obj_key_t& key, bool del_op = false)
{
    auto mgmt_ctxt = mgmt_state_t::thread_context();
    auto uuid_obj = mgmt_ctxt.state()->lookup_uuid(key);
    if (uuid_obj == nullptr) {
        throw Error(std::string("Unknown UUID in Interface Request ")
                    .append(key.str()), SDK_RET_ENTRY_NOT_FOUND);
    }
    if (uuid_obj->obj_type() != uuid_obj_type_t::INTERFACE) {
        throw Error(std::string("Wrong UUID ").append(key.str())
                    .append(" containing ").append(uuid_obj->str())
                    .append(" in Interface request"), SDK_RET_INVALID_ARG);
    }
    auto interface_uuid_obj = (interface_uuid_obj_t*) uuid_obj;
    PDS_TRACE_VERBOSE("Fetched Interface UUID %s MSIfindex 0x%X",
                     key.str(), interface_uuid_obj->ms_id());
    if (del_op) {
        mgmt_ctxt.state()->set_pending_uuid_delete(key);
    }
    return interface_uuid_obj->info();
}

static void
interface_uuid_ip_prefix (const pds_obj_key_t& key, ip_prefix_t& ip_prfx,
                          bool reset = false)
{
    auto mgmt_ctxt = mgmt_state_t::thread_context();
    auto uuid_obj = mgmt_ctxt.state()->lookup_uuid(key);
    if (uuid_obj == nullptr) {
        throw Error(std::string("Unknown UUID in Interface Request ")
                    .append(key.str()), SDK_RET_ENTRY_NOT_FOUND);
    }
    if (uuid_obj->obj_type() != uuid_obj_type_t::INTERFACE) {
        throw Error(std::string("Wrong UUID ").append(key.str())
                    .append(" containing ").append(uuid_obj->str())
                    .append(" in Interface request"), SDK_RET_INVALID_ARG);
    }
    auto interface_uuid_obj = (interface_uuid_obj_t*) uuid_obj;
    if (reset) {
        interface_uuid_obj->reset_ip();
    } else {
        interface_uuid_obj->set_ip(ip_prfx);
    }
}

static types::ApiStatus
process_interface_update (pds_if_spec_t *if_spec,
                          ms_ifindex_t ms_ifindex,
                          NBB_LONG      row_status,
                          bool update = false)
{
    LimInterfaceAddrSpec lim_addr_spec;
    PDS_MS_START_TXN(PDS_MS_CTM_GRPC_CORRELATOR);
    bool has_ip_addr = false;
    bool ip_modified = false;
    ip_prefix_t *intf_ip_prfx = NULL;

    if (if_spec->type == IF_TYPE_L3) {
        // Create L3 interfaces
        LimInterfaceCfgSpec lim_if_spec;
        uint32_t enable = (if_spec->admin_state == PDS_IF_STATE_UP) ?
                           AMB_TRISTATE_TRUE :
                           AMB_TRISTATE_FALSE;
        populate_lim_l3_intf_cfg_spec (lim_if_spec, ms_ifindex, enable);
        pds_ms_set_liminterfacecfgspec_amb_lim_if_cfg (
                                   lim_if_spec, row_status,
                                   PDS_MS_CTM_GRPC_CORRELATOR, FALSE);
        intf_ip_prfx = &if_spec->l3_if_info.ip_prefix;

        if ((row_status != AMB_ROW_DESTROY) &&
            (!ip_addr_is_zero(&intf_ip_prfx->addr))) {
            has_ip_addr = true;
            populate_lim_addr_spec (&if_spec->l3_if_info.ip_prefix,
                                    lim_addr_spec, LIM_IF_TYPE_ETH,
                                    api::objid_from_uuid(if_spec->l3_if_info.port));
        }
    } else if (if_spec->type == IF_TYPE_LOOPBACK){
        // Otherwise, its always singleton loopback
        LimInterfaceSpec lim_swif_spec;
        lim_swif_spec.set_ifid(LOOPBACK_IF_ID); lim_swif_spec.set_iftype(LIM_IF_TYPE_LOOPBACK);
        pds_ms_set_liminterfacespec_amb_lim_software_if (
                                        lim_swif_spec, row_status,
                                        PDS_MS_CTM_GRPC_CORRELATOR, FALSE);
        intf_ip_prfx = &if_spec->loopback_if_info.ip_prefix;

        if ((row_status != AMB_ROW_DESTROY) &&
            (!ip_addr_is_zero(&intf_ip_prfx->addr))) {
            has_ip_addr = true;
            populate_lim_addr_spec (&if_spec->loopback_if_info.ip_prefix,
                                    lim_addr_spec, LIM_IF_TYPE_LOOPBACK,
                                    LOOPBACK_IF_ID);
        }
    }

    if (update || row_status == AMB_ROW_DESTROY) {
        auto ifinfo = interface_uuid_fetch(if_spec->key);
        if (!IPADDR_EQ (&ifinfo.ip_prfx.addr, &intf_ip_prfx->addr)) {
            ip_modified = true;
        }

        // delete old ip
        if (!ip_addr_is_zero(&ifinfo.ip_prfx.addr)  && ip_modified) {
            LimInterfaceAddrSpec lim_del_addr_spec;
            LimIntfType iftype = LIM_IF_TYPE_LOOPBACK;
            uint32_t ifid = LOOPBACK_IF_ID;

            if (ifinfo.eth_ifindex) {
                iftype = LIM_IF_TYPE_ETH;
                ifid = ifinfo.eth_ifindex;
            }
            PDS_TRACE_INFO("Intf %s MS[[0x%x]] delete old IP address %s",
                           if_spec->key.str(), ifinfo.ms_ifindex,
                           ippfx2str(&ifinfo.ip_prfx));

            populate_lim_addr_spec (&ifinfo.ip_prfx, lim_del_addr_spec,
                                    iftype, ifid);

            if (iftype == LIM_IF_TYPE_LOOPBACK) {
                // remove redistribute connected rule
                lim_l3_if_addr_pre_set(lim_del_addr_spec, AMB_ROW_DESTROY,
                                            PDS_MS_CTM_GRPC_CORRELATOR, FALSE);
            }
            pds_ms_set_liminterfaceaddrspec_amb_lim_l3_if_addr (
                                            lim_del_addr_spec, AMB_ROW_DESTROY,
                                            PDS_MS_CTM_GRPC_CORRELATOR, FALSE);
            if (!has_ip_addr) {
                // no ip address for interface. reset the cached ip.
                interface_uuid_ip_prefix (if_spec->key, ifinfo.ip_prfx, true);
            }
        }
    }

    if (has_ip_addr) {
        PDS_TRACE_INFO("Intf %s set IP address %s",
                       if_spec->key.str(), ippfx2str(intf_ip_prfx));

        if (if_spec->type == IF_TYPE_LOOPBACK){
            // we are maintaining only one entry for redistribute connected rule
            // so delete should be completed before adding the rule for update case
            lim_l3_if_addr_pre_set(lim_addr_spec, row_status,
                                   PDS_MS_CTM_GRPC_CORRELATOR, FALSE);
        }
        // Configure IP Address
        pds_ms_set_liminterfaceaddrspec_amb_lim_l3_if_addr (
                                        lim_addr_spec, row_status,
                                        PDS_MS_CTM_GRPC_CORRELATOR, FALSE);
        if (ip_modified) {
            // ip address is modified. update cached ip
            interface_uuid_ip_prefix (if_spec->key, *intf_ip_prfx, false);
        }
    }

    PDS_MS_END_TXN(PDS_MS_CTM_GRPC_CORRELATOR);

    // blocking on response from MS
    return pds_ms::mgmt_state_t::ms_response_wait();
}

static void
interface_uuid_alloc(const pds_obj_key_t& key, uint32_t &ms_ifindex,
                     uint32_t eth_ifindex, ip_prefix_t &ip_prfx)
{
    auto mgmt_ctxt = mgmt_state_t::thread_context();
    auto uuid_obj = mgmt_ctxt.state()->lookup_uuid(key);
    if (uuid_obj != nullptr) {
        throw Error(std::string("Interface Create for existing UUID")
                    .append(key.str()).append(" containing ")
                    .append(uuid_obj->str()), SDK_RET_ENTRY_EXISTS);
    }
    interface_uuid_obj_uptr_t interface_uuid_obj
                              (new interface_uuid_obj_t
                              (key, ms_ifindex, eth_ifindex, ip_prfx));
    mgmt_ctxt.state()->set_pending_uuid_create(key,
                                               std::move(interface_uuid_obj));
    return;
}

static void
l3_intf_create (const pds_if_spec_t* if_spec, ms_ifindex_t ms_ifindex,
                uint32_t eth_ifindex)
{
    auto state_ctxt = pds_ms::state_t::thread_context();
    auto if_obj = state_ctxt.state()->if_store().get(ms_ifindex);

    if (if_obj != nullptr && if_obj->phy_port_properties().mgmt_spec_init) {
        // L3 Intf created for same Eth IfIndex for which
        // the prev L3 intf delete has not yet been seen by MS LI Stub
        throw Error(std::string("Duplicate Create for existing MS Intf ")
                    .append(std::to_string(ms_ifindex)).append(" EthIfIndex ")
                    .append(std::to_string(eth_ifindex)), SDK_RET_ENTRY_EXISTS);
        return;
    }

    if_obj_t* new_if_obj = nullptr;
    if (if_obj == nullptr) {
        new_if_obj = new pds_ms::if_obj_t(ms_ifindex, *if_spec);
    } else {
        new_if_obj = if_obj;
        new_if_obj->phy_port_properties().set_mgmt_spec(*if_spec);
    }
    auto& phy_port_prop = new_if_obj->phy_port_properties();

    auto if_name = if_index_to_ifname(eth_ifindex);
    PDS_TRACE_INFO("Fetching Linux Ifinfo for Ifname %s", if_name.c_str());
#ifdef SIM
    if (!get_linux_intf_params(if_name.c_str(),
                               &phy_port_prop.lnx_ifindex)) {
        // On sim the docker container may not have the Linux interfaces.
        PDS_TRACE_ERR("SIM: Failed to fetch Linux Ifinfo for Ifname %s",
                      if_name.c_str());
    }
#else
    if (!get_linux_intf_params(if_name.c_str(),
                               &phy_port_prop.lnx_ifindex)) {
        throw Error (std::string("Could not fetch Linux params (IfIndex, S-MAC) for ")
                     .append(if_name));
    }
#endif

    if (if_obj == nullptr) {
        state_ctxt.state()->if_store().add_upd(ms_ifindex, new_if_obj);
    } else {
        // store already has interface - must be upgrade scenario
        // invoke LI stub directly
        li_intf_t intf;
        intf.intf_create(ms_ifindex);
    }
}

sdk_ret_t
interface_create (pds_if_spec_t *spec)
{
    types::ApiStatus ret_status;
    uint32_t ms_ifindex = 0;

    // lock to allow only one grpc thread processing at a time
    std::lock_guard<std::mutex> lck(pds_ms::mgmt_state_t::grpc_lock());

    try {
        // Guard to release all pending UUIDs in case of any failures
        mgmt_uuid_guard_t uuid_guard;

        // Get PDS to MS IfIndex
        if (spec->type == IF_TYPE_L3) {
           auto eth_ifindex = api::objid_from_uuid(spec->l3_if_info.port);

            ms_ifindex = pds_to_ms_ifindex(eth_ifindex, IF_TYPE_ETH);
            PDS_TRACE_INFO("L3 Intf Create:: UUID %s Eth[0x%X] to MS[0x%X]]",
                            spec->key.str(), eth_ifindex, ms_ifindex);

            l3_intf_create(spec, ms_ifindex, eth_ifindex);

            // Cache Intf UUID to MS IfIndex
            interface_uuid_alloc(spec->key, ms_ifindex, eth_ifindex,
                                 spec->l3_if_info.ip_prefix);

        } else if (spec->type == IF_TYPE_LOOPBACK) {
            ms_ifindex = pds_to_ms_ifindex(LOOPBACK_IF_ID, IF_TYPE_LOOPBACK);
            PDS_TRACE_INFO("Loopback Intf Create:: UUID %s to MS[0x%X]]",
                            spec->key.str(),  ms_ifindex);

            // Cache Intf UUID to MS IfIndex
            interface_uuid_alloc(spec->key, ms_ifindex, 0,
                                 spec->loopback_if_info.ip_prefix);
        } else {
            PDS_TRACE_DEBUG("Ignoring unknown interface %s type %d",
                             spec->key.str(), spec->type);
            return SDK_RET_OK;
        }

        ret_status = process_interface_update (spec, ms_ifindex, AMB_ROW_ACTIVE);
        if (ret_status != types::ApiStatus::API_STATUS_OK) {
            PDS_TRACE_ERR ("Failed to process interface %s create for "
                           "MSIfIndex 0x%X err %d",
                            spec->key.str(), ms_ifindex, ret_status);
            return pds_ms_api_to_sdk_ret (ret_status);
        }

        PDS_TRACE_DEBUG ("Intf create for UUID %s successfully processed",
                          spec->key.str());
    } catch (const Error& e) {
        PDS_TRACE_ERR ("Interface %s creation failed %s",
                        spec->key.str(), e.what());
        return e.rc();
    }
    return SDK_RET_OK;
}

sdk_ret_t
interface_delete (pds_obj_key_t* key)
{
    types::ApiStatus ret_status;

    // lock to allow only one grpc thread processing at a time
    std::lock_guard<std::mutex> lck(pds_ms::mgmt_state_t::grpc_lock());

    PDS_TRACE_INFO("Interface delete %s", key->str());
    try {
        // guard to release all pending UUIDs in case of any failures
        mgmt_uuid_guard_t uuid_guard;

        // fetch MS ifindex from UUID cache
        auto ifinfo = interface_uuid_fetch(*key, true /* mark for del */);
        auto ms_ifindex = ifinfo.ms_ifindex;

        // for delete op, process_interface_update() expects iftype alone
        // to be populated in the if_spec
        pds_if_spec_t if_spec = {0};
        if_spec.key = *key;
        if (ifinfo.eth_ifindex != 0) {
            PDS_TRACE_INFO("L3 Intf Delete:: UUID %s MS[0x%X]] Eth Ifindex 0x%x",
                           key->str(), ms_ifindex, ifinfo.eth_ifindex);
            if_spec.type = IF_TYPE_L3;
        } else {
            PDS_TRACE_INFO("Loopback Intf Delete:: UUID %s MS[0x%X]]",
                           key->str(), ms_ifindex);
            if_spec.type = IF_TYPE_LOOPBACK;
        }

        // mark as deleted from mgmt
        if (if_spec.type == IF_TYPE_L3) {
            auto state_ctxt = pds_ms::state_t::thread_context();
            auto if_obj = state_ctxt.state()->if_store().get(ms_ifindex);
            if_obj->phy_port_properties().mgmt_spec_init = false;
        }
        // delete interface in MS
        ret_status = process_interface_update(&if_spec, ms_ifindex,
                                              AMB_ROW_DESTROY);
        if (ret_status != types::ApiStatus::API_STATUS_OK) {
            PDS_TRACE_ERR ("Failed to process interface UUID %s "
                           "MS-Interface 0x%X delete err %d",
                            key->str(), ms_ifindex, ret_status);
            return pds_ms_api_to_sdk_ret (ret_status);
        }
        PDS_TRACE_DEBUG ("Intf UUID %s successfully deleted in routing stack",
                         key->str());

        if (if_spec.type == IF_TYPE_L3) {
            // MS does not invoke LI stub delete for interface delete.
            // invoke interface clean up directly from here.
            li_intf_t li_intf;
            li_intf.intf_delete(ms_ifindex, false);

            PDS_TRACE_DEBUG ("L3 Intf UUID %s delete completed successfully",
                             key->str());
        }

    } catch (const Error& e) {
        PDS_TRACE_ERR ("Interface %s deletion failed %s",
                        key->str(), e.what());
        return e.rc();
    }
    return SDK_RET_OK;
}

// interface delete from pds-agent svc layer
// TODO: temporary until we stop MS hijack for loopback interface
sdk_ret_t
interface_delete (pds_obj_key_t* key, pds_batch_ctxt_t bctxt)
{
    try {
        auto ifinfo = interface_uuid_fetch(*key);
        if (ifinfo.eth_ifindex == 0) {
            // if its not attached to a underlying eth intf then it has
            // to be loopback interface since we dont store any other intf type.
            // hijack loopback intf delete alone
            return interface_delete(key);
        }
        // let other interface deletes come from API thread
        // via CFG IPC circulate
        return SDK_RET_ENTRY_NOT_FOUND;
    } catch (const Error& e) {
        PDS_TRACE_ERR ("Interface %s deletion failed %s",
                        key->str(), e.what());
        return e.rc();
    }
    return SDK_RET_OK;
}

static bool chk_and_upd_l3_spec_ (ms_ifindex_t ms_ifindex, pds_if_spec_t* spec)
{
    auto state_ctxt = pds_ms::state_t::thread_context();

    // Check for IP address change
    auto if_obj = state_ctxt.state()->if_store().get(ms_ifindex);
    if (if_obj == nullptr) {
        return false;
    }
    bool ip_change = false;
    auto& port_prop = if_obj->phy_port_properties();
    auto& old_ip_prefix = port_prop.l3_if_spec.l3_if_info.ip_prefix;
    auto& new_ip_prefix = spec->l3_if_info.ip_prefix;

    if (!ip_prefix_is_equal(&old_ip_prefix, &new_ip_prefix)) {
        PDS_TRACE_DEBUG("MSIfIndex 0x%x Change in IP", ms_ifindex);
        ip_change = true;
    } else {
        PDS_TRACE_DEBUG("MSIfIndex 0x%x No change in IP", ms_ifindex);
    }
    // Copy the spec to the store
    port_prop.l3_if_spec = *spec;
    return ip_change;
}

sdk_ret_t
interface_update (pds_if_spec_t *spec)
{
    types::ApiStatus ret_status;

    // lock to allow only one grpc thread processing at a time
    std::lock_guard<std::mutex> lck(pds_ms::mgmt_state_t::grpc_lock());

    try {
        // Guard to release all pending UUIDs in case of any failures
        mgmt_uuid_guard_t uuid_guard;

        // Fill MS IfIndex from UUID cache
        auto ifinfo = interface_uuid_fetch(spec->key);
        auto ms_ifindex = ifinfo.ms_ifindex;

        if (spec->type == IF_TYPE_L3) {
            chk_and_upd_l3_spec_(ms_ifindex, spec);
        }
        // Send the update to Metaswitch - if there is no change in
        // parameters CTM will treat it as a no-op
        ret_status = process_interface_update(spec, ms_ifindex,
                                              AMB_ROW_ACTIVE, true);
        if (ret_status != types::ApiStatus::API_STATUS_OK) {
            PDS_TRACE_ERR ("Failed to process interface UUID %s "
                           "MS-Interface 0x%X "
                           "update err %d",
                            spec->key.str(), ms_ifindex, ret_status);
            return pds_ms_api_to_sdk_ret (ret_status);
        }
        PDS_TRACE_DEBUG ("Intf update for UUID %s successfully processed",
                          spec->key.str());
    } catch (const Error& e) {
        PDS_TRACE_ERR ("Interface %s update failed %s",
                        spec->key.str(), e.what());
        return e.rc();
    }
    return SDK_RET_OK;
}

};    // namespace pds_ms
