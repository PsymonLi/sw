//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
///
/// \file
/// apulu pipeline api info to proto and vice versa implementation for device
///
//------------------------------------------------------------------------------

#ifndef __APULU_SVC_DEVICE_SVC_HPP__
#define __APULU_SVC_DEVICE_SVC_HPP__

#include "grpc++/grpc++.h"
#include "nic/apollo/api/include/pds_device.hpp"
#include "nic/apollo/api/impl/apulu/svc/specs.hpp"
#include "gen/proto/device.grpc.pb.h"

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
pds_fw_policy_xposn_to_api_spec (types::FwPolicyXposn xposn)
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
            set_datapktlearnen(
                api_spec->learn_spec.learn_source.data_pkt_learn_en);
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
    case PDS_DEVICE_PROFILE_16PF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_16PF);
        break;
    case PDS_DEVICE_PROFILE_32VF:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_32VF);
        break;
    default:
        proto_spec->set_deviceprofile(pds::DEVICE_PROFILE_DEFAULT);
        break;
    }
    switch (api_spec->memory_profile) {
    case PDS_MEMORY_PROFILE_DEFAULT:
    default:
        proto_spec->set_memoryprofile(pds::MEMORY_PROFILE_DEFAULT);
        break;
    }
    proto_spec->set_ipmappingpriority(api_spec->ip_mapping_priority);
    proto_spec->set_fwpolicyxposnscheme(
                    pds_fw_policy_xposn_api_spec_to_proto(
                        api_spec->fw_action_xposn_scheme));
}

// populate proto buf from device API info
static inline void
pds_device_api_info_to_proto (pds_device_info_t *api_info, void *ctxt)
{
    pds::DeviceGetResponse *proto_rsp = (pds::DeviceGetResponse *)ctxt;
    pds::DeviceSpec *proto_spec = proto_rsp->mutable_response()->mutable_spec();

    pds_device_api_spec_to_proto(proto_spec, &api_info->spec);
}

// build device api spec from proto buf spec
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
        // default is set to HOST mode
        api_spec->dev_oper_mode = PDS_DEV_OPER_MODE_HOST;
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
    case pds::DEVICE_PROFILE_16PF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_16PF;
        break;
    case pds::DEVICE_PROFILE_32VF:
        api_spec->device_profile = PDS_DEVICE_PROFILE_32VF;
        break;
    default:
        api_spec->device_profile = PDS_DEVICE_PROFILE_DEFAULT;
        break;
    }
    switch (proto_spec.memoryprofile()) {
    case pds::MEMORY_PROFILE_DEFAULT:
    default:
        api_spec->memory_profile = PDS_MEMORY_PROFILE_DEFAULT;
        break;
    }
    // valid priority range is 0-1023
    api_spec->ip_mapping_priority = proto_spec.ipmappingpriority() & 0x3FF;
    api_spec->fw_action_xposn_scheme =
        pds_fw_policy_xposn_to_api_spec(proto_spec.fwpolicyxposnscheme());
    return SDK_RET_OK;
}

// populate API info from device proto response
static inline sdk_ret_t
pds_device_proto_to_api_info (pds_device_info_t *api_info,
                              pds::DeviceGetResponse *proto_rsp)
{
    pds_device_spec_t *api_spec;
    pds::DeviceSpec proto_spec;
    sdk_ret_t ret;

    SDK_ASSERT(proto_rsp->has_response() != 0);
    api_spec = &api_info->spec;
    proto_spec = proto_rsp->mutable_response()->spec();
    ret = pds_device_proto_to_api_spec(api_spec, proto_spec);
    SDK_ASSERT(ret == SDK_RET_OK);
    return ret;
}

#endif    //__AGENT_SVC_DEVICE_SVC_HPP__
