//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines protobuf API for interface object
///
//----------------------------------------------------------------------------

#ifndef __AGENT_SVC_INTERFACE_SVC_HPP__
#define __AGENT_SVC_INTERFACE_SVC_HPP__

#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/interface.hpp"
#include "nic/apollo/api/include/pds_lif.hpp"
#include "nic/apollo/api/internal/pds_if.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/core/interface.hpp"

static inline types::LifType
pds_lif_type_to_proto_lif_type (lif_type_t lif_type)
{
    switch (lif_type) {
    case lif_type_t::LIF_TYPE_HOST:
        return types::LIF_TYPE_HOST;
    case lif_type_t::LIF_TYPE_HOST_MGMT:
        return types::LIF_TYPE_HOST_MGMT;
    case lif_type_t::LIF_TYPE_MNIC_OOB_MGMT:
        return types::LIF_TYPE_OOB_MGMT;
    case lif_type_t::LIF_TYPE_MNIC_INBAND_MGMT:
        return types::LIF_TYPE_INBAND_MGMT;
    case lif_type_t::LIF_TYPE_MNIC_INTERNAL_MGMT:
        return types::LIF_TYPE_INTERNAL_MGMT;
    case lif_type_t::LIF_TYPE_MNIC_CPU:
        return types::LIF_TYPE_DATAPATH;
    case lif_type_t::LIF_TYPE_LEARN:
        return types::LIF_TYPE_LEARN;
    case lif_type_t::LIF_TYPE_CONTROL:
        return types::LIF_TYPE_CONTROL;
    default:
        break;
    }
    return types::LIF_TYPE_NONE;
}

static inline pds_if_state_t
proto_admin_state_to_pds_admin_state (pds::IfStatus state)
{
    switch (state) {
    case pds::IF_STATUS_DOWN:
        return PDS_IF_STATE_DOWN;
    case pds::IF_STATUS_UP:
        return PDS_IF_STATE_UP;
    default:
        return PDS_IF_STATE_NONE;
    }
}

static inline pds::IfStatus
pds_admin_state_to_proto_admin_state (pds_if_state_t state)
{
    switch (state) {
    case PDS_IF_STATE_DOWN:
        return pds::IF_STATUS_DOWN;
    case PDS_IF_STATE_UP:
        return pds::IF_STATUS_UP;
    default:
        return pds::IF_STATUS_NONE;
    }
}

static inline pds::IfType
if_type_to_proto_if_type (if_type_t type)
{
    switch (type) {
    case IF_TYPE_UPLINK:
        return pds::IF_TYPE_UPLINK;
    case IF_TYPE_UPLINK_PC:
        return pds::IF_TYPE_UPLINK_PC;
    case IF_TYPE_L3:
        return pds::IF_TYPE_L3;
    case IF_TYPE_LOOPBACK:
        return pds::IF_TYPE_LOOPBACK;
    case IF_TYPE_CONTROL:
        return pds::IF_TYPE_CONTROL;
    case IF_TYPE_HOST:
        return pds::IF_TYPE_HOST;
    default:
        return pds::IF_TYPE_NONE;
    }
}

static inline void
pds_if_api_spec_to_proto (pds::InterfaceSpec *proto_spec,
                          const pds_if_spec_t *api_spec)
{
    proto_spec->set_id(api_spec->key.id, PDS_MAX_KEY_LEN);
    proto_spec->set_type(if_type_to_proto_if_type(api_spec->type));
    proto_spec->set_adminstatus(
                pds_admin_state_to_proto_admin_state(api_spec->admin_state));

    switch (api_spec->type) {
    case IF_TYPE_UPLINK:
        {
            auto proto_uplink = proto_spec->mutable_uplinkspec();
            proto_uplink->set_portid(api_spec->uplink_info.port.id, PDS_MAX_KEY_LEN);
        }
        break;
    case IF_TYPE_L3:
        {
            auto proto_l3 = proto_spec->mutable_l3ifspec();
            proto_l3->set_vpcid(api_spec->l3_if_info.vpc.id, PDS_MAX_KEY_LEN);
            proto_l3->set_portid(api_spec->l3_if_info.port.id, PDS_MAX_KEY_LEN);
            proto_l3->set_macaddress(
                      MAC_TO_UINT64(api_spec->l3_if_info.mac_addr));
            pds_encap_to_proto_encap(proto_l3->mutable_encap(),
                                     &api_spec->l3_if_info.encap);
            auto af = api_spec->l3_if_info.ip_prefix.addr.af;
            if (af == IP_AF_IPV4 || af == IP_AF_IPV6) {
                ippfx_api_spec_to_proto_spec(proto_l3->mutable_prefix(),
                                             &api_spec->l3_if_info.ip_prefix);
            }
        }
        break;
    case IF_TYPE_LOOPBACK:
        {
            auto proto_loopback = proto_spec->mutable_loopbackifspec();
            auto af = api_spec->loopback_if_info.ip_prefix.addr.af;
            if (af == IP_AF_IPV4 || af == IP_AF_IPV6) {
                ippfx_api_spec_to_proto_spec(proto_loopback->mutable_prefix(),
                                             &api_spec->loopback_if_info.ip_prefix);
            }
        }
        break;
    case IF_TYPE_CONTROL:
        {
            auto proto_control_if = proto_spec->mutable_controlifspec();
            auto af = api_spec->control_if_info.ip_prefix.addr.af;
            if (af == IP_AF_IPV4 || af == IP_AF_IPV6) {
                ippfx_api_spec_to_proto_spec(proto_control_if->mutable_prefix(),
                                             &api_spec->control_if_info.ip_prefix);
            }
            proto_control_if->set_macaddress(
                MAC_TO_UINT64(api_spec->control_if_info.mac_addr));
            if (api_spec->control_if_info.gateway.af == IP_AF_IPV4) {
                ipv4addr_api_spec_to_proto_spec(proto_control_if->mutable_gateway(),
                                                &api_spec->control_if_info.gateway.addr.v4_addr);
            } else {
                ipv6addr_api_spec_to_proto_spec(proto_control_if->mutable_gateway(),
                                                &api_spec->control_if_info.gateway.addr.v6_addr);
            }
        }
        break;
    case IF_TYPE_HOST:
        {
            auto proto_host_if = proto_spec->mutable_hostifspec();
            proto_host_if->set_txpolicer(api_spec->host_if_info.tx_policer.id,
                                         PDS_MAX_KEY_LEN);
        }
        break;
    default:
        break;
    }
}

static inline pds::LldpProtoMode
pds_lldpmode_to_proto_lldpmode (pds_if_lldp_mode_t mode)
{
    switch (mode) {
    case LLDP_MODE_LLDP:
        return pds::LLDP_MODE_LLDP;
    case LLDP_MODE_CDPV1:
        return pds::LLDP_MODE_CDPV1;
    case LLDP_MODE_CDPV2:
        return pds::LLDP_MODE_CDPV2;
    case LLDP_MODE_EDP:
        return pds::LLDP_MODE_EDP;
    case LLDP_MODE_FDP:
        return pds::LLDP_MODE_FDP;
    case LLDP_MODE_SONMP:
        return pds::LLDP_MODE_SONMP;
    default:
        return pds::LLDP_MODE_NONE;
    }
}

static inline pds::LldpIdType
pds_lldpidtype_to_proto_lldpidtype (pds_if_lldp_idtype_t idtype)
{
    switch (idtype) {
    case LLDPID_SUBTYPE_IFNAME:
        return pds::LLDPID_SUBTYPE_IFNAME;
    case LLDPID_SUBTYPE_IFALIAS:
        return pds::LLDPID_SUBTYPE_IFALIAS;
    case LLDPID_SUBTYPE_LOCAL:
        return pds::LLDPID_SUBTYPE_LOCAL;
    case LLDPID_SUBTYPE_MAC:
        return pds::LLDPID_SUBTYPE_MAC;
    case LLDPID_SUBTYPE_IP:
        return pds::LLDPID_SUBTYPE_IP;
    case LLDPID_SUBTYPE_PORT:
        return pds::LLDPID_SUBTYPE_PORT;
    case LLDPID_SUBTYPE_CHASSIS:
        return pds::LLDPID_SUBTYPE_CHASSIS;
    default:
        return pds::LLDPID_SUBTYPE_NONE;
    }
}

static inline pds::LldpCapType
pds_lldpcaptype_to_proto_lldpcaptype (pds_if_lldp_captype_t captype)
{
    switch (captype) {
    case LLDP_CAPTYPE_REPEATER:
        return pds::LLDP_CAPTYPE_REPEATER;
    case LLDP_CAPTYPE_BRIDGE:
        return pds::LLDP_CAPTYPE_BRIDGE;
    case LLDP_CAPTYPE_ROUTER:
        return pds::LLDP_CAPTYPE_ROUTER;
    case LLDP_CAPTYPE_WLAN:
        return pds::LLDP_CAPTYPE_WLAN;
    case LLDP_CAPTYPE_TELEPHONE:
        return pds::LLDP_CAPTYPE_TELEPHONE;
    case LLDP_CAPTYPE_DOCSIS:
        return pds::LLDP_CAPTYPE_DOCSIS;
    case LLDP_CAPTYPE_STATION:
        return pds::LLDP_CAPTYPE_STATION;
    default:
        return pds::LLDP_CAPTYPE_OTHER;
    }
}

static inline void
pds_if_lldp_status_to_proto (pds::UplinkIfStatus *uplink_status, uint16_t lif_id)
{
    pds_if_lldp_status_t api_lldp_status = { 0 };

    // get the latest LLDP status for the given uplink
    core::interface_lldp_status_get(lif_id, &api_lldp_status);

    auto lldp_status = uplink_status->mutable_lldpstatus();

    // fill in the lldp local interface status
    auto lldpifstatus = lldp_status->mutable_lldpifstatus();
    lldpifstatus->set_ifname(api_lldp_status.local_if_status.if_name);
    lldpifstatus->set_routerid(api_lldp_status.local_if_status.router_id);
    lldpifstatus->set_proto(pds_lldpmode_to_proto_lldpmode(api_lldp_status.local_if_status.mode));
    lldpifstatus->set_age(api_lldp_status.local_if_status.age.c_str());
    auto lldpchstatus = lldpifstatus->mutable_lldpifchassisspec();
    lldpchstatus->set_sysname(api_lldp_status.local_if_status.chassis_spec.sysname);
    lldpchstatus->set_sysdescr(api_lldp_status.local_if_status.chassis_spec.sysdescr);
    ipv4addr_api_spec_to_proto_spec(lldpchstatus->mutable_mgmtip(),
                                    &api_lldp_status.local_if_status.chassis_spec.mgmt_ip);
    auto lldpchidstatus = lldpchstatus->mutable_chassisid();
    lldpchidstatus->set_type(
        pds_lldpidtype_to_proto_lldpidtype(api_lldp_status.local_if_status.chassis_spec.chassis_id.type));
    lldpchidstatus->set_value((char *)api_lldp_status.local_if_status.chassis_spec.chassis_id.value);
    for (int i = 0; i < api_lldp_status.local_if_status.chassis_spec.num_caps; i++) {
        auto lldpchcap = lldpchstatus->add_capability();
        lldpchcap->set_captype(
         pds_lldpcaptype_to_proto_lldpcaptype(api_lldp_status.local_if_status.chassis_spec.chassis_cap_spec[i].cap_type));
        lldpchcap->set_capenabled(api_lldp_status.local_if_status.chassis_spec.chassis_cap_spec[i].cap_enabled);
    }
    auto lldpportstatus = lldpifstatus->mutable_lldpifportspec();
    auto lldppidstatus = lldpportstatus->mutable_portid();
    lldppidstatus->set_type(pds_lldpidtype_to_proto_lldpidtype(api_lldp_status.local_if_status.port_spec.port_id.type));
    lldppidstatus->set_value((char *)api_lldp_status.local_if_status.port_spec.port_id.value);
    lldpportstatus->set_portdescr(api_lldp_status.local_if_status.port_spec.port_descr);
    lldpportstatus->set_ttl(api_lldp_status.local_if_status.port_spec.ttl);

    // fill in the lldp neighbor status
    auto lldpnbrstatus = lldp_status->mutable_lldpnbrstatus();
    lldpnbrstatus->set_ifname(api_lldp_status.neighbor_status.if_name);
    lldpnbrstatus->set_routerid(api_lldp_status.neighbor_status.router_id);
    lldpnbrstatus->set_proto(pds_lldpmode_to_proto_lldpmode(api_lldp_status.neighbor_status.mode));
    lldpnbrstatus->set_age(api_lldp_status.neighbor_status.age.c_str());
    lldpchstatus = lldpnbrstatus->mutable_lldpifchassisspec();
    lldpchstatus->set_sysname(api_lldp_status.neighbor_status.chassis_spec.sysname);
    lldpchstatus->set_sysdescr(api_lldp_status.neighbor_status.chassis_spec.sysdescr);
    ipv4addr_api_spec_to_proto_spec(lldpchstatus->mutable_mgmtip(),
                                    &api_lldp_status.neighbor_status.chassis_spec.mgmt_ip);
    lldpchidstatus = lldpchstatus->mutable_chassisid();
    lldpchidstatus->set_type(
        pds_lldpidtype_to_proto_lldpidtype(api_lldp_status.neighbor_status.chassis_spec.chassis_id.type));
    lldpchidstatus->set_value((char *)api_lldp_status.neighbor_status.chassis_spec.chassis_id.value);
    for (int i = 0; i < api_lldp_status.neighbor_status.chassis_spec.num_caps; i++) {
        auto lldpchcap = lldpchstatus->add_capability();
        lldpchcap->set_captype(
         pds_lldpcaptype_to_proto_lldpcaptype(api_lldp_status.neighbor_status.chassis_spec.chassis_cap_spec[i].cap_type));
        lldpchcap->set_capenabled(api_lldp_status.neighbor_status.chassis_spec.chassis_cap_spec[i].cap_enabled);
    }
    lldpportstatus = lldpnbrstatus->mutable_lldpifportspec();
    lldppidstatus = lldpportstatus->mutable_portid();
    lldppidstatus->set_type(pds_lldpidtype_to_proto_lldpidtype(api_lldp_status.neighbor_status.port_spec.port_id.type));
    lldppidstatus->set_value((char *)api_lldp_status.neighbor_status.port_spec.port_id.value);
    lldpportstatus->set_portdescr(api_lldp_status.neighbor_status.port_spec.port_descr);
    lldpportstatus->set_ttl(api_lldp_status.neighbor_status.port_spec.ttl);
}

static inline void
pds_if_api_status_to_proto (pds::InterfaceStatus *proto_status,
                            const pds_if_status_t *api_status,
                            if_type_t type)
{
    proto_status->set_ifindex(api_status->ifindex);
    switch (type) {
    case IF_TYPE_UPLINK:
        {
            auto uplink_status = proto_status->mutable_uplinkifstatus();
            uplink_status->set_lifid(api_status->uplink_status.lif_id);

            // populate the uplink LLDP status
            pds_if_lldp_status_to_proto(uplink_status, api_status->uplink_status.lif_id);
        }
        break;
    case IF_TYPE_HOST:
        {
            auto host_status = proto_status->mutable_hostifstatus();
            host_status->set_name(api_status->host_if_status.name);
            host_status->set_macaddress(MAC_TO_UINT64(api_status->host_if_status.mac_addr));
            for (uint8_t i = 0; i < api_status->host_if_status.num_lifs; i ++) {
                host_status->add_lifid(&api_status->host_if_status.lifs[i],
                                       PDS_MAX_KEY_LEN);
            }
        }
        break;
    default:
        break;
    }
}

static inline void
pds_if_lldp_stats_to_proto (pds::UplinkIfStats *uplink_stats)
{
    pds_if_lldp_stats_t api_lldp_stats = { 0 };

    // get the latest LLDP stats
    core::interface_lldp_stats_get(&api_lldp_stats);
}

static inline void
pds_if_api_stats_to_proto (pds::InterfaceStats *proto_stats,
                           const pds_if_stats_t *api_stats,
                           if_type_t type)
{
    switch (type) {
    case IF_TYPE_UPLINK:
        {
            auto uplink_stats = proto_stats->mutable_uplinkifstats();

            // populate the uplink LLDP stats
            pds_if_lldp_stats_to_proto(uplink_stats);
        }
        break;
    default:
        break;
    }
}

static inline void
pds_if_api_info_to_proto (void *entry, void *ctxt)
{
    pds::InterfaceGetResponse *proto_rsp = (pds::InterfaceGetResponse *)ctxt;
    auto intf = proto_rsp->add_response();
    pds_if_info_t *info = (pds_if_info_t *)entry;

    pds_if_api_spec_to_proto(intf->mutable_spec(), &info->spec);
    pds_if_api_status_to_proto(intf->mutable_status(),
                               &info->status, info->spec.type);
    pds_if_api_stats_to_proto(intf->mutable_stats(), &info->stats, info->spec.type);
}

static inline sdk_ret_t
pds_if_proto_to_api_spec (pds_if_spec_t *api_spec,
                          const pds::InterfaceSpec &proto_spec)
{
    pds_obj_key_proto_to_api_spec(&api_spec->key, proto_spec.id());
    api_spec->admin_state =
        proto_admin_state_to_pds_admin_state(proto_spec.adminstatus());
    switch (proto_spec.type()) {
    case pds::IF_TYPE_L3:
        if (proto_spec.txmirrorsessionid_size()) {
            PDS_TRACE_ERR("Tx Mirroring not supported on L3 interface {}",
                          api_spec->key.str());
            return SDK_RET_INVALID_ARG;
        }
        if (proto_spec.rxmirrorsessionid_size()) {
            PDS_TRACE_ERR("Rx Mirroring not supported on L3 interface {}",
                          api_spec->key.str());
            return SDK_RET_INVALID_ARG;
        }
        api_spec->type = IF_TYPE_L3;
        pds_obj_key_proto_to_api_spec(&api_spec->l3_if_info.vpc,
                                      proto_spec.l3ifspec().vpcid());
        pds_obj_key_proto_to_api_spec(&api_spec->l3_if_info.port,
                                      proto_spec.l3ifspec().portid());
        api_spec->l3_if_info.encap =
            proto_encap_to_pds_encap(proto_spec.l3ifspec().encap());
        MAC_UINT64_TO_ADDR(api_spec->l3_if_info.mac_addr,
                           proto_spec.l3ifspec().macaddress());
        ippfx_proto_spec_to_api_spec(&api_spec->l3_if_info.ip_prefix,
                                     proto_spec.l3ifspec().prefix());
        break;

    case pds::IF_TYPE_LOOPBACK:
        if (proto_spec.txmirrorsessionid_size()) {
            PDS_TRACE_ERR("Tx Mirroring not supported on loopback interface {}",
                          api_spec->key.str());
            return SDK_RET_INVALID_ARG;
        }
        if (proto_spec.rxmirrorsessionid_size()) {
            PDS_TRACE_ERR("Rx Mirroring not supported on loopback interface {}",
                          api_spec->key.str());
            return SDK_RET_INVALID_ARG;
        }
        api_spec->type = IF_TYPE_LOOPBACK;
        ippfx_proto_spec_to_api_spec(&api_spec->loopback_if_info.ip_prefix,
                                     proto_spec.loopbackifspec().prefix());
        break;

    case pds::IF_TYPE_UPLINK:
        pds_obj_key_proto_to_api_spec(&api_spec->uplink_info.port,
                                      proto_spec.uplinkspec().portid());
        break;

    case pds::IF_TYPE_CONTROL:
        if (proto_spec.txmirrorsessionid_size()) {
            PDS_TRACE_ERR("Tx Mirroring not supported on inband interface {}",
                          api_spec->key.str());
            return SDK_RET_INVALID_ARG;
        }
        if (proto_spec.rxmirrorsessionid_size()) {
            PDS_TRACE_ERR("Rx Mirroring not supported on inband interface {}",
                          api_spec->key.str());
            return SDK_RET_INVALID_ARG;
        }
        api_spec->type = IF_TYPE_CONTROL;
        ippfx_proto_spec_to_api_spec(&api_spec->control_if_info.ip_prefix,
                                     proto_spec.controlifspec().prefix());
        MAC_UINT64_TO_ADDR(api_spec->control_if_info.mac_addr,
                           proto_spec.controlifspec().macaddress());
        ipaddr_proto_spec_to_api_spec(&api_spec->control_if_info.gateway,
                                      proto_spec.controlifspec().gateway());
        break;

    case pds::IF_TYPE_HOST:
        api_spec->type = IF_TYPE_HOST;
        pds_obj_key_proto_to_api_spec(&api_spec->host_if_info.tx_policer,
                                      proto_spec.hostifspec().txpolicer());
        break;

    default:
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_lif_api_status_to_proto (pds::LifStatus *proto_status,
                             const pds_lif_status_t *api_status)
{
    proto_status->set_name(api_status->name);
    proto_status->set_ifindex(api_status->ifindex);
    proto_status->set_adminstate(pds_admin_state_to_proto_admin_state(api_status->admin_state));
    proto_status->set_status(pds_admin_state_to_proto_admin_state(api_status->state));
    proto_status->set_nhindex(api_status->nh_idx);
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_lif_api_spec_to_proto_spec (pds::LifSpec *proto_spec,
                                const pds_lif_spec_t *api_spec)
{
    sdk_ret_t ret;
    pds_if_info_t pinned_ifinfo = { 0 };

    if (!api_spec || !proto_spec) {
        return SDK_RET_INVALID_ARG;
    }
    proto_spec->set_id(api_spec->key.id, PDS_MAX_KEY_LEN);
    if (api_spec->pinned_ifidx != IFINDEX_INVALID) {
        ret = api::pds_if_read(&api_spec->pinned_ifidx, &pinned_ifinfo);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_TRACE_ERR("Failed to find if for {}, err {}",
                          api_spec->pinned_ifidx, ret);
        } else {
            proto_spec->set_pinnedinterface(pinned_ifinfo.spec.key.id,
                                            PDS_MAX_KEY_LEN);
        }
    }
    proto_spec->set_type(pds_lif_type_to_proto_lif_type(api_spec->type));
    proto_spec->set_macaddress(MAC_TO_UINT64(api_spec->mac));
    return SDK_RET_OK;
}

// populate proto buf from lif api spec
static inline void
pds_lif_api_info_to_proto (void *entry, void *ctxt)
{
    auto rsp = ((pds::LifGetResponse *)ctxt)->add_response();
    auto proto_spec = rsp->mutable_spec();
    auto proto_status = rsp->mutable_status();
    pds_lif_info_t *api_info = (pds_lif_info_t *)entry;

    pds_lif_api_spec_to_proto_spec(proto_spec, &api_info->spec);
    pds_lif_api_status_to_proto(proto_status, &api_info->status);
}

static inline sdk_ret_t
pds_svc_interface_create (const pds::InterfaceRequest *proto_req,
                          pds::InterfaceResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    Status status = Status::OK;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_if_spec_t *api_spec;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, interface creation "
                          "failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        api_spec = (pds_if_spec_t *)
                    core::agent_state::state()->if_slab()->alloc();
        if (api_spec == NULL) {
            ret = SDK_RET_OOM;
            goto end;
        }
        auto request = proto_req->request(i);
        ret = pds_if_proto_to_api_spec(api_spec, request);
        if (ret != SDK_RET_OK) {
            core::agent_state::state()->if_slab()->free(api_spec);
            goto end;
        }
        ret = core::interface_create(api_spec, bctxt);
        if (ret != SDK_RET_OK) {
            core::agent_state::state()->if_slab()->free(api_spec);
            goto end;
        }
    }
    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;

end:
    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
pds_svc_interface_update (const pds::InterfaceRequest *proto_req,
                          pds::InterfaceResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    Status status = Status::OK;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_if_spec_t *api_spec;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, interface update "
                          "failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }
    for (int i = 0; i < proto_req->request_size(); i ++) {
        api_spec = (pds_if_spec_t *)
                    core::agent_state::state()->if_slab()->alloc();
        if (api_spec == NULL) {
            ret = SDK_RET_OOM;
            goto end;
        }
        auto request = proto_req->request(i);
        ret = pds_if_proto_to_api_spec(api_spec, request);
        if (ret != SDK_RET_OK) {
            core::agent_state::state()->if_slab()->free(api_spec);
            goto end;
        }
        ret = core::interface_update(api_spec, bctxt);
        if (ret != SDK_RET_OK) {
            core::agent_state::state()->if_slab()->free(api_spec);
            goto end;
        }
    }
    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;

end:
    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
pds_svc_interface_delete (const pds::InterfaceDeleteRequest *proto_req,
                          pds::InterfaceDeleteResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_obj_key_t key = { 0 };
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, interface delete "
                          "failed");
            proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = core::interface_delete(&key, bctxt);
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    return ret;

end:

    // destroy the internal batch
    if (batched_internally) {
        pds_batch_destroy(bctxt);
    }
    proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
pds_svc_interface_get (const pds::InterfaceGetRequest *proto_req,
                       pds::InterfaceGetResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_if_info_t info;
    pds_obj_key_t key = { 0 };

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        memset(&info, 0, sizeof(info));
        ret = core::interface_get(&key, &info);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        if (ret != SDK_RET_OK) {
            break;
        }
        pds_if_api_info_to_proto(&info, proto_rsp);
    }

    if (proto_req->id_size() == 0) {
        ret = core::interface_get_all(pds_if_api_info_to_proto, proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }
    return ret;
}

static inline sdk_ret_t
pds_svc_lif_get (const pds::LifGetRequest *proto_req,
                 pds::LifGetResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_obj_key_t key;

    if (proto_req) {
        for (int i = 0; i < proto_req->id_size(); i ++) {
            pds_lif_info_t info = { 0 };
            pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
            ret = pds_lif_read(&key, &info);
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            pds_lif_api_info_to_proto(&info, proto_rsp);
        }
        if (proto_req->id_size() == 0) {
            ret = pds_lif_read_all(pds_lif_api_info_to_proto, proto_rsp);
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        }
    }
    return ret;
}

static inline sdk_ret_t
pds_svc_lif_stats_reset (const types::Id *req)
{
    sdk_ret_t ret;
    pds_obj_key_t key;

    if (req->id().empty()) {
        ret = pds_lif_stats_reset(NULL);
    } else {
        pds_obj_key_proto_to_api_spec(&key, req->id());
        ret = pds_lif_stats_reset(&key);
    }

    return ret;
}

static inline sdk_ret_t
pds_svc_interface_handle_cfg (cfg_ctxt_t *ctxt, google::protobuf::Any *any_resp)
{
    sdk_ret_t ret;
    google::protobuf::Any *any_req = (google::protobuf::Any *)ctxt->req;

    switch (ctxt->cfg) {
    case CFG_MSG_INTERFACE_CREATE:
        {
            pds::InterfaceRequest req;
            pds::InterfaceResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_interface_create(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_INTERFACE_UPDATE:
        {
            pds::InterfaceRequest req;
            pds::InterfaceResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_interface_update(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_INTERFACE_DELETE:
        {
            pds::InterfaceDeleteRequest req;
            pds::InterfaceDeleteResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_interface_delete(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_INTERFACE_GET:
        {
            pds::InterfaceGetRequest req;
            pds::InterfaceGetResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_interface_get(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_LIF_GET:
        {
            pds::LifGetRequest req;
            pds::LifGetResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_lif_get(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    default:
        ret = SDK_RET_INVALID_ARG;
        break;
    }

    return ret;
}

#endif    //__AGENT_SVC_INTERFACE_SVC_HPP__
