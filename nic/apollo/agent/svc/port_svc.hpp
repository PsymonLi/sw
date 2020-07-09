//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines protobuf API for port object
///
//----------------------------------------------------------------------------

#ifndef __AGENT_SVC_PORT_SVC_HPP__
#define __AGENT_SVC_PORT_SVC_HPP__

#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/port.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/linkmgr/port_mac.hpp"
#include "nic/sdk/platform/drivers/xcvr.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"

static inline pds::PortLinkSM
pds_fsmstate_to_proto (sdk::types::port_link_sm_t fsm_state)
{
    switch (fsm_state) {
    case port_link_sm_t::PORT_LINK_SM_DISABLED:
        return pds::PORT_LINK_FSM_DISABLED;
    case port_link_sm_t::PORT_LINK_SM_ENABLED:
        return pds::PORT_LINK_FSM_ENABLED;
    case port_link_sm_t::PORT_LINK_SM_AN_CFG:
        return pds::PORT_LINK_FSM_AN_CFG;
    case port_link_sm_t::PORT_LINK_SM_AN_DISABLED:
        return pds::PORT_LINK_FSM_AN_DISABLED;
    case port_link_sm_t::PORT_LINK_SM_AN_START:
        return pds::PORT_LINK_FSM_AN_START;
    case port_link_sm_t::PORT_LINK_SM_AN_WAIT_HCD:
        return pds::PORT_LINK_FSM_AN_WAIT_HCD;
    case port_link_sm_t::PORT_LINK_SM_AN_COMPLETE:
        return pds::PORT_LINK_FSM_AN_COMPLETE;
    case port_link_sm_t::PORT_LINK_SM_SERDES_CFG:
        return pds::PORT_LINK_FSM_SERDES_CFG;
    case port_link_sm_t::PORT_LINK_SM_WAIT_SERDES_RDY:
        return pds::PORT_LINK_FSM_WAIT_SERDES_RDY;
    case port_link_sm_t::PORT_LINK_SM_MAC_CFG:
        return pds::PORT_LINK_FSM_MAC_CFG;
    case port_link_sm_t::PORT_LINK_SM_SIGNAL_DETECT:
        return pds::PORT_LINK_FSM_SIGNAL_DETECT;
    case port_link_sm_t::PORT_LINK_SM_AN_DFE_TUNING:
        return pds::PORT_LINK_FSM_AN_DFE_TUNING;
    case port_link_sm_t::PORT_LINK_SM_DFE_TUNING:
        return pds::PORT_LINK_FSM_DFE_TUNING;
    case port_link_sm_t::PORT_LINK_SM_DFE_DISABLED:
        return pds::PORT_LINK_FSM_DFE_DISABLED;
    case port_link_sm_t::PORT_LINK_SM_DFE_START_ICAL:
        return pds::PORT_LINK_FSM_DFE_START_ICAL;
    case port_link_sm_t::PORT_LINK_SM_DFE_WAIT_ICAL:
        return pds::PORT_LINK_FSM_DFE_WAIT_ICAL;
    case port_link_sm_t::PORT_LINK_SM_DFE_START_PCAL:
        return pds::PORT_LINK_FSM_DFE_START_PCAL;
    case port_link_sm_t::PORT_LINK_SM_DFE_WAIT_PCAL:
        return pds::PORT_LINK_FSM_DFE_WAIT_PCAL;
    case port_link_sm_t::PORT_LINK_SM_DFE_PCAL_CONTINUOUS:
        return pds::PORT_LINK_FSM_DFE_PCAL_CONTINUOUS;
    case port_link_sm_t::PORT_LINK_SM_CLEAR_MAC_REMOTE_FAULTS:
        return pds::PORT_LINK_FSM_CLEAR_MAC_REMOTE_FAULTS;
    case port_link_sm_t::PORT_LINK_SM_WAIT_MAC_SYNC:
        return pds::PORT_LINK_FSM_WAIT_MAC_SYNC;
    case port_link_sm_t::PORT_LINK_SM_WAIT_MAC_FAULTS_CLEAR:
        return pds::PORT_LINK_FSM_WAIT_MAC_FAULTS_CLEAR;
    case port_link_sm_t::PORT_LINK_SM_UP:
        return pds::PORT_LINK_FSM_UP;
    default:
        return pds::PORT_LINK_FSM_DISABLED;
    }
}

static inline port_fec_type_t
pds_port_proto_fec_type_to_sdk_fec_type (pds::PortFecType proto_fec_type)
{
    switch (proto_fec_type) {
    case pds::PORT_FEC_TYPE_NONE:
        return port_fec_type_t::PORT_FEC_TYPE_NONE;
    case pds::PORT_FEC_TYPE_FC:
        return port_fec_type_t::PORT_FEC_TYPE_FC;
    case pds::PORT_FEC_TYPE_RS:
        return port_fec_type_t::PORT_FEC_TYPE_RS;
    default:
        return port_fec_type_t::PORT_FEC_TYPE_NONE;
    }
}

static inline pds::PortFecType
pds_port_sdk_fec_type_to_proto_fec_type (port_fec_type_t sdk_fec_type)
{
    switch (sdk_fec_type) {
    case port_fec_type_t::PORT_FEC_TYPE_FC:
        return pds::PORT_FEC_TYPE_FC;
    case port_fec_type_t::PORT_FEC_TYPE_RS:
        return pds::PORT_FEC_TYPE_RS;
    default:
        return pds::PORT_FEC_TYPE_NONE;
    }
}

static inline port_speed_t
pds_port_proto_speed_to_sdk_speed (types::PortSpeed proto_port_speed)
{
    switch (proto_port_speed) {
    case types::PORT_SPEED_NONE:
        return port_speed_t::PORT_SPEED_NONE;
    case types::PORT_SPEED_1G:
        return port_speed_t::PORT_SPEED_1G;
    case types::PORT_SPEED_10G:
        return port_speed_t::PORT_SPEED_10G;
    case types::PORT_SPEED_25G:
        return port_speed_t::PORT_SPEED_25G;
    case types::PORT_SPEED_40G:
        return port_speed_t::PORT_SPEED_40G;
    case types::PORT_SPEED_50G:
        return port_speed_t::PORT_SPEED_50G;
    case types::PORT_SPEED_100G:
        return port_speed_t::PORT_SPEED_100G;
    default:
        return port_speed_t::PORT_SPEED_NONE;
    }
}

static inline types::PortSpeed
pds_port_sdk_speed_to_proto_speed (port_speed_t sdk_port_speed)
{
    switch (sdk_port_speed) {
    case port_speed_t::PORT_SPEED_1G:
        return types::PORT_SPEED_1G;
    case port_speed_t::PORT_SPEED_10G:
        return types::PORT_SPEED_10G;
    case port_speed_t::PORT_SPEED_25G:
        return types::PORT_SPEED_25G;
    case port_speed_t::PORT_SPEED_40G:
        return types::PORT_SPEED_40G;
    case port_speed_t::PORT_SPEED_50G:
        return types::PORT_SPEED_50G;
    case port_speed_t::PORT_SPEED_100G:
        return types::PORT_SPEED_100G;
    default:
        return types::PORT_SPEED_NONE;
    }
}

static inline port_admin_state_t
pds_port_proto_admin_state_to_sdk_admin_state (
                            pds::PortAdminState proto_admin_state)
{
    switch (proto_admin_state) {
    case pds::PORT_ADMIN_STATE_NONE:
        return port_admin_state_t::PORT_ADMIN_STATE_NONE;
    case pds::PORT_ADMIN_STATE_DOWN:
        return port_admin_state_t::PORT_ADMIN_STATE_DOWN;
    case pds::PORT_ADMIN_STATE_UP:
        return port_admin_state_t::PORT_ADMIN_STATE_UP;
    default:
        return port_admin_state_t::PORT_ADMIN_STATE_NONE;
    }
}

static inline pds::PortAdminState
pds_port_sdk_admin_state_to_proto_admin_state (port_admin_state_t sdk_admin_state)
{
    switch (sdk_admin_state) {
    case port_admin_state_t::PORT_ADMIN_STATE_DOWN:
        return pds::PORT_ADMIN_STATE_DOWN;
    case port_admin_state_t::PORT_ADMIN_STATE_UP:
        return pds::PORT_ADMIN_STATE_UP;
    default:
        return pds::PORT_ADMIN_STATE_NONE;
    }
}

static inline port_pause_type_t
pds_port_proto_pause_type_to_sdk_pause_type (pds::PortPauseType proto_pause_type)
{
    switch (proto_pause_type) {
    case pds::PORT_PAUSE_TYPE_LINK:
        return port_pause_type_t::PORT_PAUSE_TYPE_LINK;
    case pds::PORT_PAUSE_TYPE_PFC:
        return port_pause_type_t::PORT_PAUSE_TYPE_PFC;
    default:
        return port_pause_type_t::PORT_PAUSE_TYPE_NONE;
    }
}

static inline pds::PortPauseType
pds_port_sdk_pause_type_to_proto_pause_type (port_pause_type_t sdk_pause_type)
{
    switch (sdk_pause_type) {
    case port_pause_type_t::PORT_PAUSE_TYPE_LINK:
        return pds::PORT_PAUSE_TYPE_LINK;
    case port_pause_type_t::PORT_PAUSE_TYPE_PFC:
        return pds::PORT_PAUSE_TYPE_PFC;
    default:
        return pds::PORT_PAUSE_TYPE_NONE;
    }
}

static inline port_loopback_mode_t
pds_port_proto_loopback_mode_to_sdk_loopback_mode (
                        pds::PortLoopBackMode proto_loopback_mode)
{
    switch (proto_loopback_mode) {
    case pds::PORT_LOOPBACK_MODE_MAC:
        return port_loopback_mode_t::PORT_LOOPBACK_MODE_MAC;
    case pds::PORT_LOOPBACK_MODE_PHY:
        return port_loopback_mode_t::PORT_LOOPBACK_MODE_PHY;
    default:
        return port_loopback_mode_t::PORT_LOOPBACK_MODE_NONE;
    }
}

static inline pds::PortLoopBackMode
pds_port_sdk_loopback_mode_to_proto_loopback_mode (
                        port_loopback_mode_t sdk_loopback_mode)
{
    switch (sdk_loopback_mode) {
    case port_loopback_mode_t::PORT_LOOPBACK_MODE_MAC:
        return pds::PORT_LOOPBACK_MODE_MAC;
    case port_loopback_mode_t::PORT_LOOPBACK_MODE_PHY:
        return pds::PORT_LOOPBACK_MODE_PHY;
    default:
        return pds::PORT_LOOPBACK_MODE_NONE;
    }
}

static inline sdk_ret_t
pds_svc_port_stats_reset (const types::Id *req)
{
    pds_obj_key_t key;
    sdk_ret_t     ret;

    if (req->id().empty()) {
        ret = api::port_stats_reset(&k_pds_obj_key_invalid);
    } else {
        pds_obj_key_proto_to_api_spec(&key, req->id());
        ret = api::port_stats_reset(&key);
    }
    return ret;
}

static inline sdk_ret_t
pds_port_proto_to_api_spec (pds_if_spec_t *api_spec,
                            const pds::PortSpec *proto_spec)
{
    pds_obj_key_proto_to_api_spec(&api_spec->key, proto_spec->id());
    api_spec->type = IF_TYPE_ETH;
    api_spec->port_info.admin_state =
        pds_port_proto_admin_state_to_sdk_admin_state(proto_spec->adminstate());
    switch (proto_spec->type()) {
    case pds::PORT_TYPE_ETH:
        api_spec->port_info.type = port_type_t::PORT_TYPE_ETH;
        break;
    case pds::PORT_TYPE_ETH_MGMT:
        api_spec->port_info.type = port_type_t::PORT_TYPE_MGMT;
        break;
    default:
        api_spec->port_info.type = port_type_t::PORT_TYPE_NONE;
        break;
    }
    api_spec->port_info.speed =
        pds_port_proto_speed_to_sdk_speed(proto_spec->speed());
    api_spec->port_info.fec_type =
        pds_port_proto_fec_type_to_sdk_fec_type(proto_spec->fectype());
    api_spec->port_info.autoneg_en = proto_spec->autonegen();
    api_spec->port_info.debounce_timeout = proto_spec->debouncetimeout();
    api_spec->port_info.mtu = proto_spec->mtu();
    api_spec->port_info.pause_type =
        pds_port_proto_pause_type_to_sdk_pause_type(proto_spec->pausetype());
    api_spec->port_info.tx_pause_en = proto_spec->txpauseen();
    api_spec->port_info.rx_pause_en = proto_spec->rxpauseen();
    api_spec->port_info.loopback_mode =
        pds_port_proto_loopback_mode_to_sdk_loopback_mode(
            proto_spec->loopbackmode());
    api_spec->port_info.num_lanes = proto_spec->numlanes();
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_svc_port_update (const pds::PortUpdateRequest *proto_req,
                     pds::PortUpdateResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    Status status = Status::OK;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_if_spec_t *api_spec;

    if (proto_req == NULL) {
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
            PDS_TRACE_ERR("Failed to create a new batch, port update "
                          "failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    api_spec = (pds_if_spec_t *)
                core::agent_state::state()->if_slab()->alloc();
    if (api_spec == NULL) {
        ret = SDK_RET_OOM;
        goto end;
    }
    ret = pds_port_proto_to_api_spec(api_spec, &proto_req->spec());
    if (ret != SDK_RET_OK) {
        core::agent_state::state()->if_slab()->free(api_spec);
        goto end;
    }
    ret = api::port_update(api_spec, bctxt);
    if (ret != SDK_RET_OK) {
        core::agent_state::state()->if_slab()->free(api_spec);
        goto end;
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

static inline void
pds_port_api_spec_to_proto (pds::PortSpec *proto_spec,
                            const pds_if_spec_t *api_spec,
                            if_index_t ifindex)
{
    proto_spec->set_id(api_spec->key.id, PDS_MAX_KEY_LEN);
    proto_spec->set_portnumber(ETH_IFINDEX_TO_PARENT_PORT(ifindex));
    switch (api_spec->port_info.admin_state) {
    case port_admin_state_t::PORT_ADMIN_STATE_DOWN:
        proto_spec->set_adminstate(pds::PORT_ADMIN_STATE_DOWN);
        break;
    case port_admin_state_t::PORT_ADMIN_STATE_UP:
        proto_spec->set_adminstate(pds::PORT_ADMIN_STATE_UP);
        break;
    default:
        proto_spec->set_adminstate(pds::PORT_ADMIN_STATE_NONE);
        break;
    }
    switch (api_spec->port_info.type) {
    case port_type_t::PORT_TYPE_ETH:
        proto_spec->set_type(pds::PORT_TYPE_ETH);
        break;
    case port_type_t::PORT_TYPE_MGMT:
        proto_spec->set_type(pds::PORT_TYPE_ETH_MGMT);
        break;
    default:
        proto_spec->set_type(pds::PORT_TYPE_NONE);
        break;
    }
    proto_spec->set_speed(
        pds_port_sdk_speed_to_proto_speed(api_spec->port_info.speed));
    proto_spec->set_fectype(pds_port_sdk_fec_type_to_proto_fec_type
                            (api_spec->port_info.fec_type));
    proto_spec->set_autonegen(api_spec->port_info.autoneg_en);
    proto_spec->set_debouncetimeout(api_spec->port_info.debounce_timeout);
    proto_spec->set_mtu(api_spec->port_info.mtu);
    proto_spec->set_pausetype(pds_port_sdk_pause_type_to_proto_pause_type
                              (api_spec->port_info.pause_type));
    proto_spec->set_txpauseen(api_spec->port_info.tx_pause_en);
    proto_spec->set_rxpauseen(api_spec->port_info.rx_pause_en);
    proto_spec->set_loopbackmode(pds_port_sdk_loopback_mode_to_proto_loopback_mode(
                                 api_spec->port_info.loopback_mode));
    proto_spec->set_numlanes(api_spec->port_info.num_lanes);
}

static inline void
pds_port_api_status_to_proto (pds::PortStatus *proto_status,
                              const pds_if_status_t *api_status,
                              port_type_t port_type)
{
    auto link_status = proto_status->mutable_linkstatus();
    const uint8_t *xcvr_sprom;

    proto_status->set_ifindex(api_status->ifindex);
    proto_status->set_fsmstate(
        pds_fsmstate_to_proto(api_status->port_status.fsm_state));
    proto_status->set_macid(api_status->port_status.mac_id);
    proto_status->set_macch(api_status->port_status.mac_ch);

    switch (api_status->port_status.link_status.oper_state) {
    case port_oper_status_t::PORT_OPER_STATUS_UP:
        link_status->set_operstate(pds::PORT_OPER_STATUS_UP);
        break;
    case port_oper_status_t::PORT_OPER_STATUS_DOWN:
        link_status->set_operstate(pds::PORT_OPER_STATUS_DOWN);
        break;
    default:
        link_status->set_operstate(pds::PORT_OPER_STATUS_NONE);
        break;
    }

    switch (api_status->port_status.link_status.speed) {
    case port_speed_t::PORT_SPEED_1G:
        link_status->set_portspeed(types::PORT_SPEED_1G);
        break;
    case port_speed_t::PORT_SPEED_10G:
        link_status->set_portspeed(types::PORT_SPEED_10G);
        break;
    case port_speed_t::PORT_SPEED_25G:
        link_status->set_portspeed(types::PORT_SPEED_25G);
        break;
    case port_speed_t::PORT_SPEED_40G:
        link_status->set_portspeed(types::PORT_SPEED_40G);
        break;
    case port_speed_t::PORT_SPEED_50G:
        link_status->set_portspeed(types::PORT_SPEED_50G);
        break;
    case port_speed_t::PORT_SPEED_100G:
        link_status->set_portspeed(types::PORT_SPEED_100G);
        break;
    default:
        link_status->set_portspeed(types::PORT_SPEED_NONE);
        break;
    }

    link_status->set_autonegen(api_status->port_status.link_status.autoneg_en);
    link_status->set_numlanes(api_status->port_status.link_status.num_lanes);
    link_status->set_fectype(pds_port_sdk_fec_type_to_proto_fec_type
                             (api_status->port_status.link_status.fec_type));

    if (port_type != port_type_t::PORT_TYPE_MGMT) {
        auto xcvr_status = proto_status->mutable_xcvrstatus();
        xcvr_sprom = api_status->port_status.xcvr_status.xcvr_sprom;
        xcvr_status->set_port(api_status->port_status.xcvr_status.port);
        xcvr_status->set_state(pds::PortXcvrState(
                               api_status->port_status.xcvr_status.state));
        xcvr_status->set_pid(pds::PortXcvrPid(
                             api_status->port_status.xcvr_status.pid));
        xcvr_status->set_mediatype(
            pds::MediaType(api_status->port_status.xcvr_status.media_type));
        xcvr_status->set_xcvrsprom(
            &api_status->port_status.xcvr_status.xcvr_sprom, XCVR_SPROM_SIZE);
        xcvr_status->set_vendorname(
            sdk::platform::xcvr_vendor_name(xcvr_sprom));
        xcvr_status->set_vendoroui(sdk::platform::xcvr_vendor_oui(xcvr_sprom));
        xcvr_status->set_serialnumber(
            sdk::platform::xcvr_vendor_serial_number(xcvr_sprom));
        xcvr_status->set_partnumber(
            sdk::platform::xcvr_vendor_part_number(xcvr_sprom));
        xcvr_status->set_revision(
            sdk::platform::xcvr_vendor_revision(
                api_status->port_status.xcvr_status.port, xcvr_sprom));
    }
}

static inline void
pds_port_api_stats_to_proto (pds::PortStats *proto_stats,
                             pds_if_stats_t *api_stats,
                             port_type_t port_type)
{
    if (port_type == port_type_t::PORT_TYPE_ETH) {
        for (uint32_t i = 0; i < MAX_MAC_STATS; i++) {
            auto macstats = proto_stats->add_macstats();
            macstats->set_type(pds::MacStatsType(i));
            macstats->set_count(api_stats->port_stats.stats[i]);
        }
    } else if (port_type == port_type_t::PORT_TYPE_MGMT) {
        for (uint32_t i = 0; i < MAX_MGMT_MAC_STATS; i++) {
            auto macstats = proto_stats->add_mgmtmacstats();
            macstats->set_type(pds::MgmtMacStatsType(i));
            macstats->set_count(api_stats->port_stats.stats[i]);
        }
    }
    proto_stats->set_numlinkdown(api_stats->port_stats.num_linkdown);

    auto linkinfo = proto_stats->mutable_linktiminginfo();
    linkinfo->set_last_down_timestamp(api_stats->port_stats.last_down_timestamp);

    auto duration = linkinfo->mutable_bringup_duration();
    duration->set_sec(api_stats->port_stats.bringup_duration.tv_sec);
    duration->set_nsec(api_stats->port_stats.bringup_duration.tv_nsec);
}

static inline void
pds_port_api_info_to_proto (void *entry, void *ctxt)
{
    pds::Port *port;
    pds::PortGetResponse *proto_rsp = (pds::PortGetResponse *)ctxt;
    pds_if_info_t *info = (pds_if_info_t *)entry;

    if(info->spec.type != IF_TYPE_ETH) {
        return;
    }
    port = proto_rsp->add_response();
    pds_port_api_spec_to_proto(port->mutable_spec(), &info->spec,
                               info->status.ifindex);
    pds_port_api_status_to_proto(port->mutable_status(), &info->status,
                                 info->spec.port_info.type);
    pds_port_api_stats_to_proto(port->mutable_stats(), &info->stats,
                                info->spec.port_info.type);
}

static inline sdk_ret_t
pds_svc_port_get (const pds::PortGetRequest *proto_req,
                  pds::PortGetResponse *proto_rsp)
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
        ret = api::port_get(&key, &info);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        if (ret != SDK_RET_OK) {
            break;
        }
        pds_port_api_info_to_proto(&info, proto_rsp);
    }

    if (proto_req->id_size() == 0) {
        ret = api::port_get_all(pds_port_api_info_to_proto, proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }
    return ret;
}

#endif    //__AGENT_SVC_PORT_SVC_HPP__
