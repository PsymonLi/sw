//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines protobuf API for device object
///
//----------------------------------------------------------------------------

#ifndef __AGENT_SVC_DEVICE_SVC_HPP__
#define __AGENT_SVC_DEVICE_SVC_HPP__

#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/device.hpp"

static inline types::FwPolicyXposn
pds_fw_policy_xposn_api_spec_to_proto (const fw_policy_xposn_t xposn)
{
    switch (xposn) {
    case FW_POLICY_XPOSN_GLOBAL_PRIORITY:
        return types::FW_POLICY_XPOSN_GLOBAL_PRIORITY;
    case FW_POLICY_XPOSN_ANY_DENY:
        return types::FW_POLICY_XPOSN_ANY_DENY;
    default:
        return types::FW_POLICY_XPOSN_NONE;
    }
}

static inline fw_policy_xposn_t
pds_fw_policy_xposn_proto_to_api_spec (types::FwPolicyXposn xposn)
{
    switch (xposn) {
    case types::FW_POLICY_XPOSN_GLOBAL_PRIORITY:
        return FW_POLICY_XPOSN_GLOBAL_PRIORITY;
    case types::FW_POLICY_XPOSN_ANY_DENY:
        return FW_POLICY_XPOSN_ANY_DENY;
    default:
        return FW_POLICY_XPOSN_NONE;
    }
}

static inline pds::LearnMode
pds_learn_mode_api_spec_to_proto (pds_learn_mode_t mode)
{
    switch (mode) {
    case PDS_LEARN_MODE_NOTIFY:
        return pds::LEARN_MODE_NOTIFY;
    case PDS_LEARN_MODE_AUTO:
        return pds::LEARN_MODE_AUTO;
    default:
        break;
    }
    return pds::LEARN_MODE_NONE;
}

static inline pds_learn_mode_t
pds_learn_mode_proto_to_api_spec (pds::LearnMode mode)
{
    switch (mode) {
    case pds::LEARN_MODE_NOTIFY:
        return PDS_LEARN_MODE_NOTIFY;
    case pds::LEARN_MODE_AUTO:
        return PDS_LEARN_MODE_AUTO;
    default:
        break;
    }
    return PDS_LEARN_MODE_NONE;
}

// populate proto buf spec from device API spec
static inline void
pds_device_api_spec_to_proto (pds::DeviceSpec *proto_spec,
                              const pds_device_spec_t *api_spec)
{
    if ((api_spec->device_ip_addr.af == IP_AF_IPV4) ||
        (api_spec->device_ip_addr.af == IP_AF_IPV6)) {
        ipaddr_api_spec_to_proto_spec(proto_spec->mutable_ipaddr(),
                                      &api_spec->device_ip_addr);
    }
    proto_spec->set_macaddr(MAC_TO_UINT64(api_spec->device_mac_addr));
    if ((api_spec->gateway_ip_addr.af == IP_AF_IPV4) ||
        (api_spec->gateway_ip_addr.af == IP_AF_IPV6)) {
        ipaddr_api_spec_to_proto_spec(proto_spec->mutable_gatewayip(),
                                      &api_spec->gateway_ip_addr);
    }
    proto_spec->set_bridgingen(api_spec->bridging_en);
    if (api_spec->learn_spec.learn_mode != PDS_LEARN_MODE_NONE) {
        proto_spec->mutable_learnspec()->set_learnmode(
            pds_learn_mode_api_spec_to_proto(api_spec->learn_spec.learn_mode));
        proto_spec->mutable_learnspec()->set_learnagetimeout(
            api_spec->learn_spec.learn_age_timeout);
        proto_spec->mutable_learnspec()->mutable_learnsource()->
            set_arplearnen(api_spec->learn_spec.learn_source.arp_learn_en);
        proto_spec->mutable_learnspec()->mutable_learnsource()->
            set_dhcplearnen(api_spec->learn_spec.learn_source.dhcp_learn_en);
        proto_spec->mutable_learnspec()->mutable_learnsource()->
            set_datapktlearnen(api_spec->learn_spec.learn_source.data_pkt_learn_en);
    }
    proto_spec->set_overlayroutingen(api_spec->overlay_routing_en);
    proto_spec->set_symmetricroutingen(api_spec->symmetric_routing_en);
    // xlate device operational mode
    switch (api_spec->dev_oper_mode) {
    case PDS_DEV_OPER_MODE_HOST:
        proto_spec->set_devopermode(pds::DEVICE_OPER_MODE_HOST);
        break;
    case PDS_DEV_OPER_MODE_BITW_SMART_SWITCH:
        proto_spec->set_devopermode(pds::DEVICE_OPER_MODE_BITW_SMART_SWITCH);
        break;
    case PDS_DEV_OPER_MODE_BITW_SMART_SERVICE:
        proto_spec->set_devopermode(pds::DEVICE_OPER_MODE_BITW_SMART_SERVICE);
        break;
    case PDS_DEV_OPER_MODE_BITW_CLASSIC_SWITCH:
        proto_spec->set_devopermode(pds::DEVICE_OPER_MODE_BITW_CLASSIC_SWITCH);
        break;
    default:
        proto_spec->set_devopermode(pds::DEVICE_OPER_MODE_NONE);
        break;
    }
    switch (api_spec->device_profile) {
    case PDS_DEVICE_PROFILE_2PF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_2PF);
        break;
    case PDS_DEVICE_PROFILE_3PF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_3PF);
        break;
    case PDS_DEVICE_PROFILE_4PF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_4PF);
        break;
    case PDS_DEVICE_PROFILE_5PF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_5PF);
        break;
    case PDS_DEVICE_PROFILE_6PF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_6PF);
        break;
    case PDS_DEVICE_PROFILE_7PF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_7PF);
        break;
    case PDS_DEVICE_PROFILE_8PF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_8PF);
        break;
    case PDS_DEVICE_PROFILE_32VF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_32VF);
        break;
    default:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_DEFAULT);
        break;
    }
    switch (api_spec->memory_profile) {
    case PDS_MEMORY_PROFILE_ROUTER:
        proto_spec->set_memoryprofile(pds::MEMORY_PROFILE_ROUTER);
        break;
    case PDS_MEMORY_PROFILE_DEFAULT:
    default:
        proto_spec->set_memoryprofile(pds::MEMORY_PROFILE_DEFAULT);
        break;
    }
    proto_spec->set_ipmappingpriority(api_spec->ip_mapping_priority);
    proto_spec->set_fwpolicyxposnscheme(
                    pds_fw_policy_xposn_api_spec_to_proto(
                        api_spec->fw_action_xposn_scheme));
    proto_spec->set_txpolicerid(api_spec->tx_policer.id, PDS_MAX_KEY_LEN);
}

// populate proto buf status from device API status
static inline void
pds_device_api_status_to_proto (pds::DeviceStatus *proto_status,
                                const pds_device_status_t *api_status)
{
    proto_status->set_systemmacaddress(MAC_TO_UINT64(api_status->fru_mac));
    proto_status->set_memory(api_status->memory_cap);
    proto_status->set_sku(api_status->part_num);
    proto_status->set_serialnumber(api_status->serial_num);
    proto_status->set_manufacturingdate(api_status->mnfg_date);
    proto_status->set_productname(api_status->product_name);
    proto_status->set_description(api_status->description);
    proto_status->set_vendorid(api_status->vendor_id);
    switch (api_status->chip_type) {
    case sdk::platform::asic_type_t::SDK_ASIC_TYPE_CAPRI:
        proto_status->set_chiptype(types::ASIC_TYPE_CAPRI);
        break;
    case sdk::platform::asic_type_t::SDK_ASIC_TYPE_ELBA:
        proto_status->set_chiptype(types::ASIC_TYPE_ELBA);
        break;
    default:
        proto_status->set_chiptype(types::ASIC_TYPE_NONE);
        break;
    }
    proto_status->set_hardwarerevision(api_status->hardware_revision);
    proto_status->set_cpuvendor(api_status->cpu_vendor);
    proto_status->set_cpuspecification(api_status->cpu_specification);
    proto_status->set_firmwareversion(api_status->fw_version);
    proto_status->set_socosversion(api_status->soc_os_version);
    proto_status->set_socdisksize(api_status->soc_disk_size);
    proto_status->set_pciespecification(api_status->pcie_specification);
    proto_status->set_pciebusinfo(api_status->pcie_bus_info);
    proto_status->set_numpcieports(api_status->num_pcie_ports);
    proto_status->set_numports(api_status->num_ports);
    proto_status->set_vendorname(api_status->vendor_name);
    proto_status->set_pxeversion(api_status->pxe_version);
    proto_status->set_uefiversion(api_status->uefi_version);
    proto_status->set_numhostif(api_status->num_host_if);
    proto_status->set_firmwaredescription(api_status->fw_description);
    proto_status->set_firmwarebuildtime(api_status->fw_build_time);
}

// populate proto buf stats from device API stats
static inline void
pds_device_api_stats_to_proto (pds::DeviceStats *proto_stats,
                               const pds_device_stats_t *api_stats)
{
    uint32_t i;
    pds::DeviceStatsEntry *entry;

    for (i = 0; i < api_stats->ing_drop_stats_count; i++) {
        entry = proto_stats->add_ingress();
        entry->set_name(api_stats->ing_drop_stats[i].name);
        entry->set_count(api_stats->ing_drop_stats[i].count);
    }

    for (i = 0; i < api_stats->egr_drop_stats_count; i++) {
        entry = proto_stats->add_egress();
        entry->set_name(api_stats->egr_drop_stats[i].name);
        entry->set_count(api_stats->egr_drop_stats[i].count);
    }
}

static inline sdk_ret_t
pds_device_proto_to_api_spec (pds_device_spec_t *api_spec,
                              const pds::DeviceSpec &proto_spec)
{
    types::IPAddress ipaddr = proto_spec.ipaddr();
    types::IPAddress gwipaddr = proto_spec.gatewayip();
    uint64_t macaddr = proto_spec.macaddr();

    memset(api_spec, 0, sizeof(pds_device_spec_t));
    if ((ipaddr.af() == types::IP_AF_INET) ||
        (ipaddr.af() == types::IP_AF_INET6)) {
        ipaddr_proto_spec_to_api_spec(&api_spec->device_ip_addr, ipaddr);
    }
    MAC_UINT64_TO_ADDR(api_spec->device_mac_addr, macaddr);
    if ((gwipaddr.af() == types::IP_AF_INET) ||
        (gwipaddr.af() == types::IP_AF_INET6)) {
        ipaddr_proto_spec_to_api_spec(&api_spec->gateway_ip_addr, gwipaddr);
    }
    api_spec->bridging_en = proto_spec.bridgingen();
    if (proto_spec.has_learnspec()) {
        api_spec->learn_spec.learn_mode =
            pds_learn_mode_proto_to_api_spec(proto_spec.learnspec().learnmode());
        if (api_spec->learn_spec.learn_mode != PDS_LEARN_MODE_NONE) {
            api_spec->learn_spec.learn_age_timeout =
                proto_spec.learnspec().learnagetimeout();
            if (api_spec->learn_spec.learn_age_timeout == 0) {
                // default timeout is 5 mins
                api_spec->learn_spec.learn_age_timeout = 300;
            }
            if (proto_spec.learnspec().has_learnsource()) {
                api_spec->learn_spec.learn_source.arp_learn_en =
                    proto_spec.learnspec().learnsource().arplearnen();
                api_spec->learn_spec.learn_source.dhcp_learn_en =
                    proto_spec.learnspec().learnsource().dhcplearnen();
                api_spec->learn_spec.learn_source.data_pkt_learn_en =
                    proto_spec.learnspec().learnsource().datapktlearnen();
            } else if (api_spec->learn_spec.learn_mode != PDS_LEARN_MODE_NONE) {
                api_spec->learn_spec.learn_source.arp_learn_en = true;
                api_spec->learn_spec.learn_source.dhcp_learn_en = true;
                api_spec->learn_spec.learn_source.data_pkt_learn_en = true;
            }
        }
    } else {
        api_spec->learn_spec.learn_mode = PDS_LEARN_MODE_NONE;
    }
    api_spec->overlay_routing_en = proto_spec.overlayroutingen();
    api_spec->symmetric_routing_en = proto_spec.symmetricroutingen();
    switch (proto_spec.devopermode()) {
    case pds::DEVICE_OPER_MODE_HOST:
        api_spec->dev_oper_mode = PDS_DEV_OPER_MODE_HOST;
        break;
    case pds::DEVICE_OPER_MODE_BITW_SMART_SWITCH:
        api_spec->dev_oper_mode = PDS_DEV_OPER_MODE_BITW_SMART_SWITCH;
        break;
    case pds::DEVICE_OPER_MODE_BITW_SMART_SERVICE:
        api_spec->dev_oper_mode = PDS_DEV_OPER_MODE_BITW_SMART_SERVICE;
        break;
    case pds::DEVICE_OPER_MODE_BITW_CLASSIC_SWITCH:
        api_spec->dev_oper_mode = PDS_DEV_OPER_MODE_BITW_CLASSIC_SWITCH;
        break;
    default:
        api_spec->dev_oper_mode = PDS_DEV_OPER_MODE_NONE;
        return SDK_RET_INVALID_ARG;
        break;
    }
    switch (proto_spec.deviceprofile()) {
    case pds::DEVICE_PROFILE_2PF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_2PF;
        break;
    case pds::DEVICE_PROFILE_3PF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_3PF;
        break;
    case pds::DEVICE_PROFILE_4PF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_4PF;
        break;
    case pds::DEVICE_PROFILE_5PF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_5PF;
        break;
    case pds::DEVICE_PROFILE_6PF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_6PF;
        break;
    case pds::DEVICE_PROFILE_7PF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_7PF;
        break;
    case pds::DEVICE_PROFILE_8PF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_8PF;
        break;
    case pds::DEVICE_PROFILE_32VF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_32VF;
        break;
    default:
        api_spec->device_profile = PDS_DEVICE_PROFILE_DEFAULT;
        break;
    }
    switch (proto_spec.memoryprofile()) {
    case pds::MEMORY_PROFILE_ROUTER:
        api_spec->memory_profile = PDS_MEMORY_PROFILE_ROUTER;
        break;
    case pds::MEMORY_PROFILE_DEFAULT:
    default:
        api_spec->memory_profile = PDS_MEMORY_PROFILE_DEFAULT;
        break;
    }
    // valid priority range is 0-1023
    api_spec->ip_mapping_priority = proto_spec.ipmappingpriority() & 0x3FF;
    api_spec->fw_action_xposn_scheme =
        pds_fw_policy_xposn_proto_to_api_spec(proto_spec.fwpolicyxposnscheme());
    pds_obj_key_proto_to_api_spec(&api_spec->tx_policer,
                                  proto_spec.txpolicerid());
    return SDK_RET_OK;
}

sdk_ret_t pds_svc_device_create(const pds::DeviceRequest *proto_req,
                                pds::DeviceResponse *proto_rsp);
sdk_ret_t pds_svc_device_update(const pds::DeviceRequest *proto_req,
                                pds::DeviceResponse *proto_rsp);
sdk_ret_t pds_svc_device_delete(const pds::DeviceDeleteRequest *proto_req,
                                pds::DeviceDeleteResponse *proto_rsp);
sdk_ret_t pds_svc_device_get(const pds::DeviceGetRequest *proto_req,
                             pds::DeviceGetResponse *proto_rsp);

static inline sdk_ret_t
pds_svc_device_handle_cfg (cfg_ctxt_t *ctxt, google::protobuf::Any *any_resp)
{
    sdk_ret_t ret;
    google::protobuf::Any *any_req = (google::protobuf::Any *)ctxt->req;

    switch (ctxt->cfg) {
    case CFG_MSG_DEVICE_CREATE:
        {
            pds::DeviceRequest req;
            pds::DeviceResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_device_create(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_DEVICE_UPDATE:
        {
            pds::DeviceRequest req;
            pds::DeviceResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_device_update(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_DEVICE_DELETE:
        {
            pds::DeviceDeleteRequest req;
            pds::DeviceDeleteResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_device_delete(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_DEVICE_GET:
        {
            pds::DeviceGetRequest req;
            pds::DeviceGetResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_device_get(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    default:
        ret = SDK_RET_INVALID_ARG;
        break;
    }

    return ret;
}

#endif    //__AGENT_SVC_DEVICE_SVC_HPP__
