//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#ifndef __AGENT_SVC_SPECS_HPP__
#define __AGENT_SVC_SPECS_HPP__

#include "nic/sdk/linkmgr/port_mac.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/table.hpp"
#include "nic/utils/ftlite/ftlite_ipv4_structs.hpp"
#include "nic/utils/ftlite/ftlite_ipv6_structs.hpp"
#include "nic/sdk/platform/capri/capri_tm_utils.hpp"
#include "nic/apollo/api/include/pds_debug.hpp"
#include "nic/apollo/api/include/pds_mapping.hpp"
#include "nic/apollo/api/include/pds_subnet.hpp"
#include "nic/apollo/api/include/pds_device.hpp"
#include "nic/apollo/api/include/pds_mirror.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_meter.hpp"
#include "nic/apollo/api/include/pds_tag.hpp"
#include "nic/apollo/api/include/pds_tep.hpp"
#include "nic/apollo/api/include/pds_service.hpp"
#include "nic/apollo/api/include/pds_nexthop.hpp"
#include "nic/apollo/api/include/pds_route.hpp"
#include "nic/apollo/api/include/pds_vpc.hpp"
#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/api/include/pds_policy.hpp"
#include "nic/apollo/api/include/pds_lif.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/core/meter.hpp"
#include "nic/apollo/agent/svc/util.hpp"
#include "nic/apollo/agent/svc/meter.hpp"
#include "nic/apollo/agent/svc/tag.hpp"
#include "nic/apollo/agent/svc/vnic.hpp"
#include "nic/apollo/agent/svc/vpc.hpp"
#include "nic/apollo/agent/svc/tunnel.hpp"
#include "nic/apollo/agent/svc/session.hpp"
#include "nic/apollo/agent/svc/service.hpp"
#include "nic/apollo/agent/svc/port.hpp"
#include "nic/apollo/agent/svc/policy.hpp"
#include "nic/apollo/agent/svc/nh.hpp"
#include "nic/apollo/agent/svc/route.hpp"
#include "nic/apollo/agent/svc/debug.hpp"
#include "nic/apollo/agent/svc/device.hpp"
#include "nic/apollo/agent/svc/mirror.hpp"
#include "nic/apollo/agent/svc/subnet.hpp"
#include "nic/apollo/agent/svc/mapping.hpp"
#include "nic/apollo/agent/svc/lif.hpp"
#include "gen/proto/types.pb.h"

using sdk::asic::pd::port_queue_credit_t;
using sdk::asic::pd::queue_credit_t;

static inline void
pds_queue_credits_to_proto(uint32_t port_num,
                           port_queue_credit_t *credit,
                           void *ctxt)
{
    QueueCreditsGetResponse *proto = (QueueCreditsGetResponse *)ctxt;
    auto port_credit = proto->add_portqueuecredit();
    port_credit->set_port(port_num);

    for (uint32_t j = 0; j < credit->num_queues; j++) {
        auto queue_credit = port_credit->add_queuecredit();
        queue_credit->set_queue(credit->queues[j].oq);
        queue_credit->set_credit(credit->queues[j].credit);
    }
}

// populate proto buf from lif api spec
static inline void
pds_lif_api_spec_to_proto (void *entry, void *ctxt)
{
    auto proto_spec =
        ((pds::LifGetResponse *)ctxt)->add_response()->mutable_spec();
    pds_lif_spec_t *api_spec = (pds_lif_spec_t *)entry;

    proto_spec->set_lifid(api_spec->key);
    proto_spec->set_pinnedinterfaceid(api_spec->pinned_ifidx);
    proto_spec->set_type(types::LifType(api_spec->type));
}

static inline void
pds_meter_debug_stats_to_proto (pds_meter_debug_stats_t *stats, void *ctxt)
{
    pds::MeterStatsGetResponse *rsp = (pds::MeterStatsGetResponse *)ctxt;
    auto proto_stats = rsp->add_stats();

    proto_stats->set_statsindex(stats->idx);
    proto_stats->set_rxbytes(stats->rx_bytes);
    proto_stats->set_txbytes(stats->tx_bytes);
}

// build Meter api spec from proto buf spec
static inline sdk_ret_t
pds_meter_proto_to_api_spec (pds_meter_spec_t *api_spec,
                                  const pds::MeterSpec &proto_spec)
{
    sdk_ret_t ret;

    api_spec->key.id = proto_spec.id();
    ret = pds_af_proto_spec_to_api_spec(&api_spec->af, proto_spec.af());
    if (ret != SDK_RET_OK) {
        return SDK_RET_INVALID_ARG;
    }

    api_spec->num_rules = proto_spec.rules_size();
    api_spec->rules =
        (pds_meter_rule_t *)SDK_MALLOC(PDS_MEM_ALLOC_ID_METER,
                        api_spec->num_rules * sizeof(pds_meter_rule_t));
    for (uint32_t rule = 0; rule < api_spec->num_rules; rule++) {
        const pds::MeterRuleSpec &proto_rule_spec = proto_spec.rules(rule);
        pds_meter_rule_t *api_rule_spec = &api_spec->rules[rule];
        if (proto_rule_spec.has_ppspolicer()) {
            api_rule_spec->type = PDS_METER_TYPE_PPS_POLICER;
            api_rule_spec->pps =
                            proto_rule_spec.ppspolicer().packetspersecond();
            api_rule_spec->pkt_burst = proto_rule_spec.ppspolicer().burst();
        } else if (proto_rule_spec.has_bpspolicer()) {
            api_rule_spec->type = PDS_METER_TYPE_BPS_POLICER;
            api_rule_spec->bps =
                            proto_rule_spec.bpspolicer().bytespersecond();
            api_rule_spec->byte_burst = proto_rule_spec.bpspolicer().burst();
        } else {
            api_rule_spec->type = PDS_METER_TYPE_ACCOUNTING;
        }

        api_rule_spec->priority = proto_rule_spec.priority();
        api_rule_spec->num_prefixes = proto_rule_spec.prefix_size();
        api_rule_spec->prefixes =
                 (ip_prefix_t *)SDK_MALLOC(PDS_MEM_ALLOC_ID_METER,
                            api_rule_spec->num_prefixes * sizeof(ip_prefix_t));
        for (int pfx = 0; pfx < proto_rule_spec.prefix_size(); pfx++) {
            ippfx_proto_spec_to_api_spec(
                    &api_rule_spec->prefixes[pfx], proto_rule_spec.prefix(pfx));
        }
    }
    return SDK_RET_OK;
}

// populate proto buf spec from meter API spec
static inline void
pds_meter_api_spec_to_proto (pds::MeterSpec *proto_spec,
                             const pds_meter_spec_t *api_spec)
{
    if (!api_spec || !proto_spec) {
        return;
    }
    proto_spec->set_id(api_spec->key.id);
    if (api_spec->af == IP_AF_IPV4) {
        proto_spec->set_af(types::IP_AF_INET);
    } else if (api_spec->af == IP_AF_IPV6) {
        proto_spec->set_af(types::IP_AF_INET6);
    }
    for (uint32_t rule = 0; rule < api_spec->num_rules; rule++) {
        pds::MeterRuleSpec *proto_rule_spec = proto_spec->add_rules();
        pds_meter_rule_t *api_rule_spec = &api_spec->rules[rule];
        switch (api_rule_spec->type) {
        case pds_meter_type_t::PDS_METER_TYPE_PPS_POLICER:
            proto_rule_spec->mutable_ppspolicer()->set_packetspersecond(
                                            api_rule_spec->pps);
            proto_rule_spec->mutable_ppspolicer()->set_burst(
                                            api_rule_spec->pkt_burst);
            break;

        case pds_meter_type_t::PDS_METER_TYPE_BPS_POLICER:
            proto_rule_spec->mutable_bpspolicer()->set_bytespersecond(
                                            api_rule_spec->bps);
            proto_rule_spec->mutable_bpspolicer()->set_burst(
                                            api_rule_spec->byte_burst);
            break;

        case pds_meter_type_t::PDS_METER_TYPE_ACCOUNTING:
            break;

        default:
            break;
        }
        proto_rule_spec->set_priority(api_rule_spec->priority);
        for (uint32_t pfx = 0;
                  pfx < api_rule_spec->num_prefixes; pfx++) {
            ippfx_api_spec_to_proto_spec(proto_rule_spec->add_prefix(),
                           &api_rule_spec->prefixes[pfx]);
        }
    }
    return;
}

// populate proto buf status from meter API status
static inline void
pds_meter_api_status_to_proto (pds::MeterStatus *proto_status,
                               const pds_meter_status_t *api_status)
{
}

// populate proto buf stats from meter API stats
static inline void
pds_meter_api_stats_to_proto (pds::MeterStats *proto_stats,
                              const pds_meter_stats_t *api_stats)
{
    proto_stats->set_meterid(api_stats->idx);
    proto_stats->set_rxbytes(api_stats->rx_bytes);
    proto_stats->set_txbytes(api_stats->tx_bytes);
}

// populate proto buf from meter API info
static inline void
pds_meter_api_info_to_proto (const pds_meter_info_t *api_info, void *ctxt)
{
    pds::MeterGetResponse *proto_rsp = (pds::MeterGetResponse *)ctxt;
    auto meter = proto_rsp->add_response();
    //pds::MeterSpec *proto_spec = meter->mutable_spec();
    pds::MeterStatus *proto_status = meter->mutable_status();
    pds::MeterStats *proto_stats = meter->mutable_stats();

    //pds_meter_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_meter_api_status_to_proto(proto_status, &api_info->status);
    pds_meter_api_stats_to_proto(proto_stats, &api_info->stats);
}

// populate proto buf spec from tag API spec
static inline void
pds_tag_api_spec_to_proto (pds::TagSpec *proto_spec,
                           const pds_tag_spec_t *api_spec)
{
    proto_spec->set_id(api_spec->key.id);
    proto_spec->set_af(pds_af_api_spec_to_proto_spec(api_spec->af));
#if 0 // We don't store rules in db for now
    for (uint32_t i = 0; i < api_spec->num_rules; i ++) {
        auto rule = proto_spec->add_rules();
        rule->set_tag(api_spec->rules[i].tag);
        rule->set_priority(api_spec->rules[i].priority);
        for (uint32_t j = 0; j < api_spec->rules[i].num_prefixes; j ++) {
            ippfx_api_spec_to_proto_spec(
                    rule->add_prefix(), &api_spec->rules[i].prefixes[j]);
        }
    }
#endif
}

// populate proto buf status from tag API status
static inline void
pds_tag_api_status_to_proto (const pds_tag_status_t *api_status,
                             pds::TagStatus *proto_status)
{
}

// populate proto buf stats from tag API stats
static inline void
pds_tag_api_stats_to_proto (const pds_tag_stats_t *api_stats,
                            pds::TagStats *proto_stats)
{
}

// populate proto buf from tag API info
static inline void
pds_tag_api_info_to_proto (const pds_tag_info_t *api_info, void *ctxt)
{
    pds::TagGetResponse *proto_rsp = (pds::TagGetResponse *)ctxt;
    auto tag = proto_rsp->add_response();
    pds::TagSpec *proto_spec = tag->mutable_spec();
    pds::TagStatus *proto_status = tag->mutable_status();
    pds::TagStats *proto_stats = tag->mutable_stats();

    pds_tag_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_tag_api_status_to_proto(&api_info->status, proto_status);
    pds_tag_api_stats_to_proto(&api_info->stats, proto_stats);
}

// build Tag api spec from proto buf spec
static inline sdk_ret_t
pds_tag_proto_to_api_spec (pds_tag_spec_t *api_spec,
                           const pds::TagSpec &proto_spec)
{
    sdk_ret_t ret;

    api_spec->key.id = proto_spec.id();
    ret = pds_af_proto_spec_to_api_spec(&api_spec->af, proto_spec.af());
    if (ret != SDK_RET_OK) {
        return SDK_RET_INVALID_ARG;
    }

    api_spec->num_rules = proto_spec.rules_size();
    api_spec->rules = (pds_tag_rule_t *)SDK_MALLOC(PDS_MEM_ALLOC_ID_TAG,
                                                   api_spec->num_rules *
                                                   sizeof(pds_tag_rule_t));
    for (int i = 0; i < proto_spec.rules_size(); i ++) {
        auto proto_rule = proto_spec.rules(i);

        api_spec->rules[i].tag = proto_rule.tag();
        api_spec->rules[i].priority = proto_rule.priority();
        api_spec->rules[i].num_prefixes = proto_rule.prefix_size();
        api_spec->rules[i].prefixes =
            (ip_prefix_t *)SDK_MALLOC(PDS_MEM_ALLOC_ID_TAG,
                                      api_spec->rules[i].num_prefixes *
                                          sizeof(ip_prefix_t));
        for (int j = 0; j < proto_rule.prefix_size(); j ++) {
            ippfx_proto_spec_to_api_spec(
                        &api_spec->rules[i].prefixes[j], proto_rule.prefix(j));
        }
    }

    return SDK_RET_OK;
}

// build VNIC api spec from proto buf spec
static inline sdk_ret_t
pds_vnic_proto_to_api_spec (pds_vnic_spec_t *api_spec,
                            const pds::VnicSpec &proto_spec)
{
    uint32_t msid;

    api_spec->key.id = proto_spec.vnicid();
    api_spec->vpc.id = proto_spec.vpcid();
    api_spec->subnet.id = proto_spec.subnetid();
    api_spec->vnic_encap = proto_encap_to_pds_encap(proto_spec.vnicencap());
    api_spec->fabric_encap = proto_encap_to_pds_encap(proto_spec.fabricencap());
    MAC_UINT64_TO_ADDR(api_spec->mac_addr, proto_spec.macaddress());
    //MAC_UINT64_TO_ADDR(api_spec->provider_mac_addr,
    //                   proto_spec.providermacaddress());
    api_spec->rsc_pool_id = proto_spec.resourcepoolid();
    api_spec->src_dst_check = proto_spec.sourceguardenable();
    for (int i = 0; i < proto_spec.txmirrorsessionid_size(); i++) {
        msid = proto_spec.txmirrorsessionid(i);
        if ((msid < 1) || (msid > 8)) {
            PDS_TRACE_ERR("Invalid tx mirror session id {} in vnic {} spec, "
                          "mirror session ids must be in the range [1-8]",
                          msid, api_spec->key.id);
            return SDK_RET_INVALID_ARG;
        }
        api_spec->tx_mirror_session_bmap |= (1 << (msid - 1));
    }
    for (int i = 0; i < proto_spec.rxmirrorsessionid_size(); i++) {
        msid = proto_spec.rxmirrorsessionid(i);
        if ((msid < 1) || (msid > 8)) {
            PDS_TRACE_ERR("Invalid rx mirror session id {} in vnic {} spec",
                          "mirror session ids must be in the range [1-8]",
                          msid, api_spec->key.id);
            return SDK_RET_INVALID_ARG;
        }
        api_spec->rx_mirror_session_bmap |= (1 << (msid - 1));
    }
    api_spec->v4_meter.id = proto_spec.v4meterid();
    api_spec->v6_meter.id = proto_spec.v6meterid();
    api_spec->switch_vnic = proto_spec.switchvnic();
    return SDK_RET_OK;
}

// populate proto buf spec from vnic API spec
static inline void
pds_vnic_api_spec_to_proto (pds::VnicSpec *proto_spec,
                            const pds_vnic_spec_t *api_spec)
{
    if (!api_spec || !proto_spec) {
        return;
    }
    proto_spec->set_vnicid(api_spec->key.id);
    proto_spec->set_vpcid(api_spec->vpc.id);
    proto_spec->set_subnetid(api_spec->subnet.id);
    pds_encap_to_proto_encap(proto_spec->mutable_vnicencap(),
                             &api_spec->vnic_encap);
    proto_spec->set_macaddress(MAC_TO_UINT64(api_spec->mac_addr));
    //proto_spec->set_providermacaddress(
                        //MAC_TO_UINT64(api_spec->provider_mac_addr));
    pds_encap_to_proto_encap(proto_spec->mutable_fabricencap(),
                             &api_spec->fabric_encap);
    proto_spec->set_resourcepoolid(api_spec->rsc_pool_id);
    proto_spec->set_sourceguardenable(api_spec->src_dst_check);
    proto_spec->set_v4meterid(api_spec->v4_meter.id);
    proto_spec->set_v6meterid(api_spec->v6_meter.id);
    if (api_spec->tx_mirror_session_bmap) {
        for (uint8_t i = 0; i < 8; i++) {
            if (api_spec->tx_mirror_session_bmap & (1 << i)) {
                proto_spec->add_txmirrorsessionid(i + 1);
            }
        }
    }
    if (api_spec->rx_mirror_session_bmap) {
        for (uint8_t i = 0; i < 8; i++) {
            if (api_spec->rx_mirror_session_bmap & (1 << i)) {
                proto_spec->add_rxmirrorsessionid(i + 1);
            }
        }
    }
    proto_spec->set_switchvnic(api_spec->switch_vnic);
    proto_spec->set_ingv4securitypolicyid(api_spec->ing_v4_policy.id);
    proto_spec->set_ingv6securitypolicyid(api_spec->ing_v6_policy.id);
    proto_spec->set_egv4securitypolicyid(api_spec->egr_v4_policy.id);
    proto_spec->set_egv6securitypolicyid(api_spec->egr_v6_policy.id);
}

// populate proto buf status from vnic API status
static inline void
pds_vnic_api_status_to_proto (pds::VnicStatus *proto_status,
                              const pds_vnic_status_t *api_status)
{
}

// populate proto buf stats from vnic API stats
static inline void
pds_vnic_api_stats_to_proto (pds::VnicStats *proto_stats,
                             const pds_vnic_stats_t *api_stats)
{
}

// populate proto buf from vnic API info
static inline void
pds_vnic_api_info_to_proto (const pds_vnic_info_t *api_info, void *ctxt)
{
    pds::VnicGetResponse *proto_rsp = (pds::VnicGetResponse *)ctxt;
    auto vnic = proto_rsp->add_response();
    pds::VnicSpec *proto_spec = vnic->mutable_spec();
    pds::VnicStatus *proto_status = vnic->mutable_status();
    pds::VnicStats *proto_stats = vnic->mutable_stats();

    pds_vnic_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_vnic_api_status_to_proto(proto_status, &api_info->status);
    pds_vnic_api_stats_to_proto(proto_stats, &api_info->stats);
}

// populate VPCPeer protobuf spec from VPCPeer API spec
static inline void
pds_vpc_peer_api_spec_to_proto (pds::VPCPeerSpec *proto_spec,
                                const pds_vpc_peer_spec_t *api_spec)
{
    if (!api_spec || !proto_spec) {
        return;
    }
    proto_spec->set_id(api_spec->key.id);
    proto_spec->set_vpc1(api_spec->vpc1.id);
    proto_spec->set_vpc2(api_spec->vpc2.id);
}

// populate proto buf status from vpc API status
static inline void
pds_vpc_peer_api_status_to_proto (pds::VPCPeerStatus *proto_status,
                                  const pds_vpc_peer_status_t *api_status)
{
}

// populate proto buf stats from vpc API stats
static inline void
pds_vpc_peer_api_stats_to_proto (pds::VPCPeerStats *proto_stats,
                                 const pds_vpc_peer_stats_t *api_stats)
{
}

// populate proto buf from vpc API info
static inline void
pds_vpc_peer_api_info_to_proto (const pds_vpc_peer_info_t *api_info, void *ctxt)
{
    pds::VPCPeerGetResponse *proto_rsp = (pds::VPCPeerGetResponse *)ctxt;
    auto vpc = proto_rsp->add_response();
    pds::VPCPeerSpec *proto_spec = vpc->mutable_spec();
    pds::VPCPeerStatus *proto_status = vpc->mutable_status();
    pds::VPCPeerStats *proto_stats = vpc->mutable_stats();

    pds_vpc_peer_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_vpc_peer_api_status_to_proto(proto_status, &api_info->status);
    pds_vpc_peer_api_stats_to_proto(proto_stats, &api_info->stats);
}

// populate VPCPeer api spec from VPCPeer proto spec
static inline void
pds_vpc_peer_proto_to_api_spec (pds_vpc_peer_spec_t *api_spec,
                                const pds::VPCPeerSpec &proto_spec)
{
    if (!api_spec) {
        return;
    }
    api_spec->key.id = proto_spec.id();
    api_spec->vpc1.id = proto_spec.vpc1();
    api_spec->vpc2.id = proto_spec.vpc2();
}

// populate proto buf spec from tep API spec
static inline void
pds_tep_api_spec_to_proto (pds::TunnelSpec *proto_spec,
                           const pds_tep_spec_t *api_spec)
{
    ipaddr_api_spec_to_proto_spec(proto_spec->mutable_remoteip(),
                                  &api_spec->key.ip_addr);
    ipaddr_api_spec_to_proto_spec(proto_spec->mutable_localip(),
                                  &api_spec->ip_addr);
    proto_spec->set_macaddress(MAC_TO_UINT64(api_spec->mac));
    pds_encap_to_proto_encap(proto_spec->mutable_encap(),
                             &api_spec->encap);
    proto_spec->set_nat(api_spec->nat);
    proto_spec->set_vpcid(api_spec->vpc.id);
    switch (api_spec->type) {
    case PDS_TEP_TYPE_WORKLOAD:
        proto_spec->set_type(pds::TUNNEL_TYPE_WORKLOAD);
        break;
    case PDS_TEP_TYPE_IGW:
        proto_spec->set_type(pds::TUNNEL_TYPE_IGW);
        break;
    case PDS_TEP_TYPE_SERVICE:
        proto_spec->set_type(pds::TUNNEL_TYPE_SERVICE);
        break;
    case PDS_TEP_TYPE_NONE:
    default:
        proto_spec->set_type(pds::TUNNEL_TYPE_NONE);
        break;
    }

    proto_spec->set_remoteservice(api_spec->remote_svc);
    if (api_spec->remote_svc) {
        pds_encap_to_proto_encap(proto_spec->mutable_remoteserviceencap(),
                                 &api_spec->remote_svc_encap);
        ipaddr_api_spec_to_proto_spec(
            proto_spec->mutable_remoteservicepublicip(),
            &api_spec->remote_svc_public_ip);
    }
}

// populate proto buf status from tep API status
static inline void
pds_tep_api_status_to_proto (pds::TunnelStatus *proto_status,
                             const pds_tep_status_t *api_status)
{
}

// populate proto buf stats from tep API stats
static inline void
pds_tep_api_stats_to_proto (pds::TunnelStats *proto_stats,
                            const pds_tep_stats_t *api_stats)
{
}

// populate proto buf from tep API info
static inline void
pds_tep_api_info_to_proto (const pds_tep_info_t *api_info, void *ctxt)
{
    pds::TunnelGetResponse *proto_rsp = (pds::TunnelGetResponse *)ctxt;
    auto tep = proto_rsp->add_response();
    pds::TunnelSpec *proto_spec = tep->mutable_spec();
    pds::TunnelStatus *proto_status = tep->mutable_status();
    pds::TunnelStats *proto_stats = tep->mutable_stats();

    pds_tep_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_tep_api_status_to_proto(proto_status, &api_info->status);
    pds_tep_api_stats_to_proto(proto_stats, &api_info->stats);
}

// Build TEP API spec from protobuf spec
static inline void
pds_tep_proto_to_api_spec (pds_tep_spec_t *api_spec,
                            const pds::TunnelSpec &proto_spec)
{
    memset(api_spec, 0, sizeof(pds_tep_spec_t));
    ipaddr_proto_spec_to_api_spec(&api_spec->key.ip_addr,
                                  proto_spec.remoteip());
    ipaddr_proto_spec_to_api_spec(&api_spec->ip_addr,
                                  proto_spec.localip());
    MAC_UINT64_TO_ADDR(api_spec->mac, proto_spec.macaddress());
    api_spec->vpc.id = proto_spec.vpcid();

    switch (proto_spec.type()) {
    case pds::TUNNEL_TYPE_IGW:
        api_spec->type = PDS_TEP_TYPE_IGW;
        break;
    case pds::TUNNEL_TYPE_WORKLOAD:
        api_spec->type = PDS_TEP_TYPE_WORKLOAD;
        break;
    case  pds::TUNNEL_TYPE_SERVICE:
        api_spec->type = PDS_TEP_TYPE_SERVICE;
        break;
    default:
        api_spec->type = PDS_TEP_TYPE_NONE;
        break;
    }
    api_spec->encap = proto_encap_to_pds_encap(proto_spec.encap());
    api_spec->nat = proto_spec.nat();
    api_spec->remote_svc = proto_spec.remoteservice();
    if (api_spec->remote_svc) {
        api_spec->remote_svc_encap =
            proto_encap_to_pds_encap(proto_spec.remoteserviceencap());
        ipaddr_proto_spec_to_api_spec(&api_spec->remote_svc_public_ip,
                                      proto_spec.remoteservicepublicip());
    }
}

// populate proto buf spec from service API spec
static inline void
pds_service_api_spec_to_proto (pds::SvcMappingSpec *proto_spec,
                               const pds_svc_mapping_spec_t *api_spec)
{
    auto proto_key = proto_spec->mutable_key();
    proto_key->set_vpcid(api_spec->key.vpc.id);
    proto_key->set_svcport(api_spec->key.svc_port);
    ipaddr_api_spec_to_proto_spec(
                proto_key->mutable_ipaddr(), &api_spec->key.vip);
    ipaddr_api_spec_to_proto_spec(
                proto_spec->mutable_privateip(), &api_spec->backend_ip);
    ipaddr_api_spec_to_proto_spec(
                proto_spec->mutable_providerip(),
                &api_spec->backend_provider_ip);
    proto_spec->set_vpcid(api_spec->vpc.id);
    proto_spec->set_port(api_spec->svc_port);
}

// populate proto buf status from service API status
static inline void
pds_service_api_status_to_proto (pds::SvcMappingStatus *proto_status,
                                 const pds_svc_mapping_status_t *api_status)
{
}

// populate proto buf stats from service API stats
static inline void
pds_service_api_stats_to_proto (pds::SvcMappingStats *proto_stats,
                                const pds_svc_mapping_stats_t *api_stats)
{
}

// populate proto buf from service API info
static inline void
pds_service_api_info_to_proto (const pds_svc_mapping_info_t *api_info, void *ctxt)
{
    pds::SvcMappingGetResponse *proto_rsp = (pds::SvcMappingGetResponse *)ctxt;
    auto service = proto_rsp->add_response();
    pds::SvcMappingSpec *proto_spec = service->mutable_spec();
    pds::SvcMappingStatus *proto_status = service->mutable_status();
    pds::SvcMappingStats *proto_stats = service->mutable_stats();

    pds_service_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_service_api_status_to_proto(proto_status, &api_info->status);
    pds_service_api_stats_to_proto(proto_stats, &api_info->stats);
}

// Build service API spec from proto buf spec
static inline void
pds_service_proto_to_api_spec (pds_svc_mapping_spec_t *api_spec,
                               const pds::SvcMappingSpec &proto_spec)
{
    api_spec->key.vpc.id = proto_spec.key().vpcid();
    api_spec->key.svc_port = proto_spec.key().svcport();
    ipaddr_proto_spec_to_api_spec(&api_spec->key.vip,
                                  proto_spec.key().ipaddr());
    ipaddr_proto_spec_to_api_spec(&api_spec->backend_ip,
                                  proto_spec.privateip());
    ipaddr_proto_spec_to_api_spec(&api_spec->backend_provider_ip,
                                  proto_spec.providerip());
    api_spec->svc_port = proto_spec.port();
    api_spec->vpc.id = proto_spec.vpcid();
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
pds_port_proto_speed_to_sdk_speed (pds::PortSpeed proto_port_speed)
{
    switch (proto_port_speed) {
    case pds::PORT_SPEED_NONE:
        return port_speed_t::PORT_SPEED_NONE;
    case pds::PORT_SPEED_1G:
        return port_speed_t::PORT_SPEED_1G;
    case pds::PORT_SPEED_10G:
        return port_speed_t::PORT_SPEED_10G;
    case pds::PORT_SPEED_25G:
        return port_speed_t::PORT_SPEED_25G;
    case pds::PORT_SPEED_40G:
        return port_speed_t::PORT_SPEED_40G;
    case pds::PORT_SPEED_50G:
        return port_speed_t::PORT_SPEED_50G;
    case pds::PORT_SPEED_100G:
        return port_speed_t::PORT_SPEED_100G;
    default:
        return port_speed_t::PORT_SPEED_NONE;
    }
}

static inline pds::PortSpeed
pds_port_sdk_speed_to_proto_speed (port_speed_t sdk_port_speed)
{
    switch (sdk_port_speed) {
    case port_speed_t::PORT_SPEED_1G:
        return pds::PORT_SPEED_1G;
    case port_speed_t::PORT_SPEED_10G:
        return pds::PORT_SPEED_10G;
    case port_speed_t::PORT_SPEED_25G:
        return pds::PORT_SPEED_25G;
    case port_speed_t::PORT_SPEED_40G:
        return pds::PORT_SPEED_40G;
    case port_speed_t::PORT_SPEED_50G:
        return pds::PORT_SPEED_50G;
    case port_speed_t::PORT_SPEED_100G:
        return pds::PORT_SPEED_100G;
    default:
        return pds::PORT_SPEED_NONE;
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
    switch(proto_pause_type) {
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
    switch(proto_loopback_mode) {
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

static inline void
pds_port_proto_to_port_args (port_args_t *port_args,
                              const pds::PortSpec &spec)
{
    port_args->user_admin_state = port_args->admin_state =
                pds_port_proto_admin_state_to_sdk_admin_state(spec.adminstate());
    port_args->port_speed = pds_port_proto_speed_to_sdk_speed(spec.speed());
    port_args->fec_type = pds_port_proto_fec_type_to_sdk_fec_type(spec.fectype());
    port_args->auto_neg_cfg = port_args->auto_neg_enable = spec.autonegen();
    port_args->debounce_time = spec.debouncetimeout();
    port_args->mtu = spec.mtu();
    port_args->pause =
                pds_port_proto_pause_type_to_sdk_pause_type(spec.pausetype());
    port_args->loopback_mode =
       pds_port_proto_loopback_mode_to_sdk_loopback_mode(spec.loopbackmode());
    port_args->num_lanes_cfg = port_args->num_lanes = spec.numlanes();
}

static inline pds::SecurityRuleAction
pds_rule_action_to_proto_action (rule_action_data_t *action_data)
{
    switch (action_data->fw_action.action) {
    case SECURITY_RULE_ACTION_ALLOW:
        return pds::SECURITY_RULE_ACTION_ALLOW;
    case SECURITY_RULE_ACTION_DENY:
        return pds::SECURITY_RULE_ACTION_DENY;
    default:
        return pds::SECURITY_RULE_ACTION_NONE;
    }
}

static inline fw_action_t
pds_proto_action_to_rule_action (pds::SecurityRuleAction action)
{
    switch (action) {
    case pds::SECURITY_RULE_ACTION_ALLOW:
        return SECURITY_RULE_ACTION_ALLOW;
    case pds::SECURITY_RULE_ACTION_DENY:
        return SECURITY_RULE_ACTION_DENY;
    default:
        return SECURITY_RULE_ACTION_DENY;
    }
}

static inline sdk_ret_t
pds_policy_dir_proto_to_api_spec (rule_dir_t *dir,
                                  const pds::SecurityPolicySpec &proto_spec)
{
    if (proto_spec.direction() == types::RULE_DIR_INGRESS) {
        *dir = RULE_DIR_INGRESS;
    } else if (proto_spec.direction() == types::RULE_DIR_EGRESS) {
        *dir = RULE_DIR_EGRESS;
    } else {
        PDS_TRACE_ERR("Invalid direction {} in policy spec {}",
                      proto_spec.direction(), proto_spec.id());
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_policy_rule_match_proto_to_api_spec (pds_policy_id_t policy_id,
                                         uint32_t rule_id, rule_match_t *match,
                                         const pds::SecurityRule &proto_rule)
{
    if (unlikely(proto_rule.has_match() == false)) {
        PDS_TRACE_ERR("Security policy {}, rule {} has no match condition, "
                      "IP protocol is a mandatory match condition",
                      policy_id, rule_id);
        return SDK_RET_INVALID_ARG;
    }

    if (unlikely(proto_rule.match().has_l3match() == false)) {
        PDS_TRACE_ERR("Security policy {}, rule {} has no L3 match condition, "
                      "IP protocol is a mandatory match condition",
                      policy_id, rule_id);
        return SDK_RET_INVALID_ARG;
    }

    const types::RuleMatch& proto_match = proto_rule.match();
    const types::RuleL3Match& proto_l3_match = proto_match.l3match();
    match->l3_match.ip_proto = proto_l3_match.protocol();
    if ((match->l3_match.ip_proto != IP_PROTO_UDP) &&
        (match->l3_match.ip_proto != IP_PROTO_TCP) &&
        (match->l3_match.ip_proto != IP_PROTO_ICMP) &&
        (match->l3_match.ip_proto != IP_PROTO_ICMPV6)) {
        PDS_TRACE_ERR("Security policy {}, rule {} with unsupported IP "
                      "protocol {}", policy_id, rule_id,
                      match->l3_match.ip_proto);
        return SDK_RET_INVALID_ARG;
    }
    if (proto_l3_match.has_srcprefix()) {
        match->l3_match.src_match_type = IP_MATCH_PREFIX;
        ippfx_proto_spec_to_api_spec(&match->l3_match.src_ip_pfx,
                                     proto_l3_match.srcprefix());
    } else if (proto_l3_match.has_srcrange()) {
        match->l3_match.src_match_type = IP_MATCH_RANGE;
        iprange_proto_spec_to_api_spec(&match->l3_match.src_ip_range,
                                       proto_l3_match.srcrange());
    } else if (proto_l3_match.srctag()) {
        match->l3_match.src_match_type = IP_MATCH_TAG;
        match->l3_match.src_tag = proto_l3_match.srctag();
    } else {
        // since the memory is zero-ed out, this is 0.0.0.0/0 or 0::0/0
        // TODO: should we set the IP_AF_XXX ?
    }
    if (proto_l3_match.has_dstprefix()) {
        match->l3_match.dst_match_type = IP_MATCH_PREFIX;
        ippfx_proto_spec_to_api_spec(&match->l3_match.dst_ip_pfx,
                                     proto_l3_match.dstprefix());
    } else if (proto_l3_match.has_dstrange()) {
        match->l3_match.dst_match_type = IP_MATCH_RANGE;
        iprange_proto_spec_to_api_spec(&match->l3_match.dst_ip_range,
                                       proto_l3_match.dstrange());
    } else if (proto_l3_match.dsttag()) {
        match->l3_match.dst_match_type = IP_MATCH_TAG;
        match->l3_match.dst_tag = proto_l3_match.dsttag();
    } else {
        // since the memory is zero-ed out, this is 0.0.0.0/0 or 0::0/0
        // TODO: should we set the IP_AF_XXX ?
    }

    if (proto_rule.match().has_l4match() &&
        (proto_rule.match().l4match().has_ports() ||
         proto_rule.match().l4match().has_typecode())) {
        const types::RuleL4Match& proto_l4_match = proto_match.l4match();
        if (proto_l4_match.has_ports()) {
            if ((match->l3_match.ip_proto != IP_PROTO_UDP) &&
                (match->l3_match.ip_proto != IP_PROTO_TCP)) {
                PDS_TRACE_ERR("Invalid port config in security policy {}, "
                              "rule {}", policy_id, rule_id);
                return SDK_RET_INVALID_ARG;
            }
            if (proto_l4_match.ports().has_srcportrange()) {
                const types::PortRange& sport_range =
                    proto_l4_match.ports().srcportrange();
                match->l4_match.sport_range.port_lo = sport_range.portlow();
                match->l4_match.sport_range.port_hi = sport_range.porthigh();
                if (unlikely(match->l4_match.sport_range.port_lo >
                             match->l4_match.sport_range.port_hi)) {
                    PDS_TRACE_ERR("Invalid src port range in security "
                                  "policy {}, rule {}", policy_id, rule_id);
                    return SDK_RET_INVALID_ARG;
                }
            } else {
                match->l4_match.sport_range.port_lo = 0;
                match->l4_match.sport_range.port_hi = 65535;
            }
            if (proto_l4_match.ports().has_dstportrange()) {
                const types::PortRange& dport_range =
                    proto_l4_match.ports().dstportrange();
                match->l4_match.dport_range.port_lo = dport_range.portlow();
                match->l4_match.dport_range.port_hi = dport_range.porthigh();
                if (unlikely(match->l4_match.dport_range.port_lo >
                             match->l4_match.dport_range.port_hi)) {
                    PDS_TRACE_ERR("Invalid dst port range in security "
                                  "policy {}, rule {}", policy_id, rule_id);
                    return SDK_RET_INVALID_ARG;
                }
            } else {
                match->l4_match.dport_range.port_lo = 0;
                match->l4_match.dport_range.port_hi = 65535;
            }
        } else if (proto_l4_match.has_typecode()) {
            if ((match->l3_match.ip_proto != IP_PROTO_ICMP) &&
                (match->l3_match.ip_proto != IP_PROTO_ICMPV6)) {
                PDS_TRACE_ERR("Invalid ICMP config in security policy {}, "
                              "rule {}", policy_id, rule_id);
                return SDK_RET_INVALID_ARG;
            }
            const types::ICMPMatch& typecode = proto_l4_match.typecode();
            match->l4_match.icmp_type = typecode.type();
            match->l4_match.icmp_code = typecode.code();
        }
    } else {
        // wildcard L4 match
        if ((match->l3_match.ip_proto == IP_PROTO_UDP) ||
            (match->l3_match.ip_proto == IP_PROTO_TCP)) {
            match->l4_match.sport_range.port_lo = 0;
            match->l4_match.sport_range.port_hi = 65535;
            match->l4_match.dport_range.port_lo = 0;
            match->l4_match.dport_range.port_hi = 65535;
        } else if ((match->l3_match.ip_proto == IP_PROTO_ICMP) ||
                   (match->l3_match.ip_proto == IP_PROTO_ICMPV6)) {
            // TODO : wildcard ICMP support will come later
        }
    }
    return SDK_RET_OK;
}

// build policy API spec from protobuf spec
static inline sdk_ret_t
pds_policy_proto_to_api_spec (pds_policy_spec_t *api_spec,
                              const pds::SecurityPolicySpec &proto_spec)
{
    uint32_t num_rules = 0;
    sdk_ret_t ret;

    api_spec->key.id = proto_spec.id();
    api_spec->policy_type = POLICY_TYPE_FIREWALL;
    ret = pds_af_proto_spec_to_api_spec(&api_spec->af, proto_spec.addrfamily());
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    ret = pds_policy_dir_proto_to_api_spec(&api_spec->direction, proto_spec);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    num_rules = proto_spec.rules_size();
    api_spec->num_rules = num_rules;
    api_spec->rules = (rule_t *)SDK_CALLOC(PDS_MEM_ALLOC_SECURITY_POLICY,
                                           sizeof(rule_t) * num_rules);
    if (unlikely(api_spec->rules == NULL)) {
        PDS_TRACE_ERR("Failed to allocate memory for security policy {}",
                      api_spec->key.id);
        return SDK_RET_OOM;
    }
    for (uint32_t i = 0; i < num_rules; i++) {
        const pds::SecurityRule &proto_rule = proto_spec.rules(i);
        api_spec->rules[i].priority = proto_rule.priority();
        api_spec->rules[i].stateful = proto_rule.stateful();
        api_spec->rules[i].action_data.fw_action.action =
                                      pds_proto_action_to_rule_action(proto_rule.action());
        ret = pds_policy_rule_match_proto_to_api_spec(api_spec->key.id,
                                                      i+ 1,
                                                      &api_spec->rules[i].match,
                                                      proto_rule);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_TRACE_ERR("Failed converting policy {} spec, err {}",
                          api_spec->key.id, ret);
            goto cleanup;
        }
    }

    return SDK_RET_OK;

cleanup :

    if (api_spec->rules) {
        SDK_FREE(PDS_MEM_ALLOC_SECURITY_POLICY, api_spec->rules);
        api_spec->rules = NULL;
    }
    return ret;
}

// populate proto buf spec from policy API spec
inline void
pds_policy_api_spec_to_proto (pds::SecurityPolicySpec *proto_spec,
                              const pds_policy_spec_t *api_spec)
{
    if (!api_spec || !proto_spec) {
        return;
    }

    proto_spec->set_id(api_spec->key.id);
    if (api_spec->af == IP_AF_IPV4) {
        proto_spec->set_addrfamily(types::IP_AF_INET);
    } else if (api_spec->af == IP_AF_IPV6) {
        proto_spec->set_addrfamily(types::IP_AF_INET6);
    }
    if (api_spec->direction == RULE_DIR_INGRESS) {
        proto_spec->set_direction(types::RULE_DIR_INGRESS);
    } else if (api_spec->direction == RULE_DIR_EGRESS) {
        proto_spec->set_direction(types::RULE_DIR_EGRESS);
    }
    for (uint32_t i = 0; i < api_spec->num_rules; i++) {
        pds::SecurityRule *proto_rule = proto_spec->add_rules();
        rule_t *api_rule = &api_spec->rules[i];
        proto_rule->set_priority(api_rule->priority);
        proto_rule->set_action(pds_rule_action_to_proto_action(&api_rule->action_data));
        proto_rule->set_stateful(api_rule->stateful);
        if (api_rule->match.l3_match.ip_proto) {
            proto_rule->mutable_match()->mutable_l3match()->set_protocol(
                                            api_rule->match.l3_match.ip_proto);
        }

        switch (api_rule->match.l3_match.src_match_type) {
        case IP_MATCH_PREFIX:
            ippfx_api_spec_to_proto_spec(
                proto_rule->mutable_match()->mutable_l3match()->mutable_srcprefix(),
                &api_rule->match.l3_match.src_ip_pfx);
            break;
        case IP_MATCH_RANGE:
            iprange_api_spec_to_proto_spec(
                proto_rule->mutable_match()->mutable_l3match()->mutable_srcrange(),
                &api_rule->match.l3_match.src_ip_range);
            break;
        case IP_MATCH_TAG:
            proto_rule->mutable_match()->mutable_l3match()->set_srctag(
                api_rule->match.l3_match.src_tag);
            break;
        default:
            break;
        }

        switch (api_rule->match.l3_match.dst_match_type) {
        case IP_MATCH_PREFIX:
            ippfx_api_spec_to_proto_spec(
                proto_rule->mutable_match()->mutable_l3match()->mutable_dstprefix(),
                &api_rule->match.l3_match.dst_ip_pfx);
            break;
        case IP_MATCH_RANGE:
            iprange_api_spec_to_proto_spec(
                proto_rule->mutable_match()->mutable_l3match()->mutable_dstrange(),
                &api_rule->match.l3_match.dst_ip_range);
            break;
        case IP_MATCH_TAG:
            proto_rule->mutable_match()->mutable_l3match()->set_dsttag(
                api_rule->match.l3_match.dst_tag);
            break;
        default:
            break;
        }

        proto_rule->mutable_match()->mutable_l4match()->mutable_ports()->mutable_srcportrange()->set_portlow(api_rule->match.l4_match.sport_range.port_lo);
        proto_rule->mutable_match()->mutable_l4match()->mutable_ports()->mutable_srcportrange()->set_porthigh(api_rule->match.l4_match.sport_range.port_hi);
        proto_rule->mutable_match()->mutable_l4match()->mutable_ports()->mutable_dstportrange()->set_portlow(api_rule->match.l4_match.dport_range.port_lo);
        proto_rule->mutable_match()->mutable_l4match()->mutable_ports()->mutable_dstportrange()->set_porthigh(api_rule->match.l4_match.dport_range.port_hi);
    }

    return;
}

// populate proto buf status from policy API status
static inline void
pds_policy_api_status_to_proto (pds::SecurityPolicyStatus *proto_status,
                                const pds_policy_status_t *api_status)
{
}

// populate proto buf stats from policy API stats
static inline void
pds_policy_api_stats_to_proto (pds::SecurityPolicyStats *proto_stats,
                                 const pds_policy_stats_t *api_stats)
{
}

// populate proto buf from policy API info
static inline void
pds_policy_api_info_to_proto (const pds_policy_info_t *api_info, void *ctxt)
{
    pds::SecurityPolicyGetResponse *proto_rsp = (pds::SecurityPolicyGetResponse *)ctxt;
    auto policy = proto_rsp->add_response();
    pds::SecurityPolicySpec *proto_spec = policy->mutable_spec();
    pds::SecurityPolicyStatus *proto_status = policy->mutable_status();
    pds::SecurityPolicyStats *proto_stats = policy->mutable_stats();

    pds_policy_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_policy_api_status_to_proto(proto_status, &api_info->status);
    pds_policy_api_stats_to_proto(proto_stats, &api_info->stats);
}

// Build nh API spec from protobuf spec
static inline void
pds_nh_proto_to_api_spec (pds_nexthop_spec_t *api_spec,
                                const pds::NexthopSpec &proto_spec)
{
    pds::NexthopType type;

    api_spec->key = proto_spec.id();
    type = proto_spec.type();
    if (type == pds::NEXTHOP_TYPE_NONE) {
        api_spec->type = PDS_NH_TYPE_NONE;
    } else if (type == pds::NEXTHOP_TYPE_IP) {
        api_spec->type = PDS_NH_TYPE_IP;
        api_spec->vpc.id = proto_spec.ipnhinfo().vpcid();
        ipaddr_proto_spec_to_api_spec(&api_spec->ip, proto_spec.ipnhinfo().ip());
        api_spec->vlan = proto_spec.ipnhinfo().vlan();
        if (proto_spec.ipnhinfo().mac() != 0) {
            MAC_UINT64_TO_ADDR(api_spec->mac, proto_spec.ipnhinfo().mac());
        }
    }
}

// populate proto buf spec from nh API spec
static inline void
pds_nh_api_spec_to_proto (pds::NexthopSpec *proto_spec,
                          const pds_nexthop_spec_t *api_spec)
{
    proto_spec->set_id(api_spec->key);
    if (api_spec->type == PDS_NH_TYPE_NONE) {
        proto_spec->set_type(pds::NEXTHOP_TYPE_NONE);
    } else if (api_spec->type == PDS_NH_TYPE_IP) {
        proto_spec->set_type(pds::NEXTHOP_TYPE_IP);
        auto ipnhinfo = proto_spec->mutable_ipnhinfo();
        ipnhinfo->set_vpcid(api_spec->vpc.id);
        ipaddr_api_spec_to_proto_spec(ipnhinfo->mutable_ip(), &api_spec->ip);
        ipnhinfo->set_vlan(api_spec->vlan);
        ipnhinfo->set_mac(MAC_TO_UINT64(api_spec->mac));
    }
}

// populate proto buf status from nh API status
static inline void
pds_nh_api_status_to_proto (pds::NexthopStatus *proto_status,
                            const pds_nexthop_status_t *api_status)
{
}

// populate proto buf stats from nh API stats
static inline void
pds_nh_api_stats_to_proto (pds::NexthopStats *proto_stats,
                           const pds_nexthop_stats_t *api_stats)
{
}

// populate proto buf from nh API info
static inline void
pds_nh_api_info_to_proto (const pds_nexthop_info_t *api_info, void *ctxt)
{
    pds::NexthopGetResponse *proto_rsp = (pds::NexthopGetResponse *)ctxt;
    auto nh = proto_rsp->add_response();
    pds::NexthopSpec *proto_spec = nh->mutable_spec();
    pds::NexthopStatus *proto_status = nh->mutable_status();
    pds::NexthopStats *proto_stats = nh->mutable_stats();

    pds_nh_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_nh_api_status_to_proto(proto_status, &api_info->status);
    pds_nh_api_stats_to_proto(proto_stats, &api_info->stats);
}

// Build nh group API spec from protobuf spec
static inline void
pds_nh_group_proto_to_api_spec (pds_nexthop_group_spec_t *api_spec,
                                const pds::NhGroupSpec &proto_spec)
{
}

// populate proto buf spec from nh API spec
static inline void
pds_nh_group_api_spec_to_proto (pds::NhGroupSpec *proto_spec,
                                const pds_nexthop_group_spec_t *api_spec)
{
}

// populate proto buf status from nh group API status
static inline void
pds_nh_group_api_status_to_proto (pds::NhGroupStatus *proto_status,
                                  const pds_nexthop_group_status_t *api_status)
{
}

// populate proto buf stats from nh group API stats
static inline void
pds_nh_group_api_stats_to_proto (pds::NhGroupStats *proto_stats,
                                 const pds_nexthop_group_stats_t *api_stats)
{
}

// populate proto buf from nh API info
static inline void
pds_nh_group_api_info_to_proto (const pds_nexthop_group_info_t *api_info, void *ctxt)
{
    pds::NhGroupGetResponse *proto_rsp = (pds::NhGroupGetResponse *)ctxt;
    auto nh = proto_rsp->add_response();
    pds::NhGroupSpec *proto_spec = nh->mutable_spec();
    pds::NhGroupStatus *proto_status = nh->mutable_status();
    pds::NhGroupStats *proto_stats = nh->mutable_stats();

    pds_nh_group_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_nh_group_api_status_to_proto(proto_status, &api_info->status);
    pds_nh_group_api_stats_to_proto(proto_stats, &api_info->stats);
}

static inline sdk_ret_t
pds_route_table_proto_to_api_spec (pds_route_table_spec_t *api_spec,
                                   const pds::RouteTableSpec &proto_spec)
{
    uint32_t num_routes = 0;

    api_spec->key.id = proto_spec.id();
    switch (proto_spec.af()) {
    case types::IP_AF_INET:
        api_spec->af = IP_AF_IPV4;
        break;

    case types::IP_AF_INET6:
        api_spec->af = IP_AF_IPV6;
        break;

    default:
        return SDK_RET_INVALID_ARG;
    }
    num_routes = proto_spec.routes_size();
    api_spec->num_routes = num_routes;
    api_spec->routes = (pds_route_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_ROUTE_TABLE,
                                                 sizeof(pds_route_t) *
                                                 num_routes);
    if (unlikely(api_spec->routes == NULL)) {
        PDS_TRACE_ERR("Failed to allocate memory for route table {}",
                      api_spec->key.id);
        return sdk::SDK_RET_OOM;
    }
    for (uint32_t i = 0; i < num_routes; i++) {
        const pds::Route &proto_route = proto_spec.routes(i);
        ippfx_proto_spec_to_api_spec(&api_spec->routes[i].prefix, proto_route.prefix());
        switch (proto_route.Nh_case()) {
        case pds::Route::kNextHop:
            ipaddr_proto_spec_to_api_spec(&api_spec->routes[i].nh_ip, proto_route.nexthop());
            api_spec->routes[i].nh_type = PDS_NH_TYPE_TEP;
            break;
        case pds::Route::kNexthopId:
            api_spec->routes[i].nh = proto_route.nexthopid();
            api_spec->routes[i].nh_type = PDS_NH_TYPE_IP;
            break;
        case pds::Route::kVPCId:
            api_spec->routes[i].vpc.id = proto_route.vpcid();
            api_spec->routes[i].nh_type = PDS_NH_TYPE_PEER_VPC;
            break;
        default:
            api_spec->routes[i].nh_type = PDS_NH_TYPE_BLACKHOLE;
            break;
        }
    }

    return SDK_RET_OK;
}

// populate proto buf spec from route table API spec
inline void
pds_route_table_api_spec_to_proto (pds::RouteTableSpec *proto_spec,
                                   const pds_route_table_spec_t *api_spec)
{
    if (!api_spec || !proto_spec) {
        return;
    }

    proto_spec->set_id(api_spec->key.id);
    if (api_spec->af == IP_AF_IPV4) {
        proto_spec->set_af(types::IP_AF_INET);
    } else if (api_spec->af == IP_AF_IPV6) {
        proto_spec->set_af(types::IP_AF_INET6);
    }
#if 0 // We don't store routes in db for now
    for (uint32_t i = 0; i < api_spec->num_routes; i++) {
        pds::Route *proto_route = proto_spec->add_routes();
    }
#endif

    return;
}

// populate proto buf status from route table API status
static inline void
pds_route_table_api_status_to_proto (pds::RouteTableStatus *proto_status,
                                     const pds_route_table_status_t *api_status)
{
}

// populate proto buf stats from route table API stats
static inline void
pds_route_table_api_stats_to_proto (pds::RouteTableStats *proto_stats,
                                    const pds_route_table_stats_t *api_stats)
{
}

// populate proto buf from route table API info
static inline void
pds_route_table_api_info_to_proto (const pds_route_table_info_t *api_info, void *ctxt)
{
    pds::RouteTableGetResponse *proto_rsp = (pds::RouteTableGetResponse *)ctxt;
    auto route_table = proto_rsp->add_response();
    pds::RouteTableSpec *proto_spec = route_table->mutable_spec();
    pds::RouteTableStatus *proto_status = route_table->mutable_status();
    pds::RouteTableStats *proto_stats = route_table->mutable_stats();

    pds_route_table_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_route_table_api_status_to_proto(proto_status, &api_info->status);
    pds_route_table_api_stats_to_proto(proto_stats, &api_info->stats);
}

static inline void
pds_table_api_stats_to_proto (pds::TableApiStats *proto_stats,
                              sdk::table::sdk_table_api_stats_t *stats)
{
    auto entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_INSERT);
    entry->set_count(stats->insert);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_INSERT_DUPLICATE);
    entry->set_count(stats->insert_duplicate);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_INSERT_FAIL);
    entry->set_count(stats->insert_fail);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_REMOVE);
    entry->set_count(stats->remove);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_REMOVE_NOT_FOUND);
    entry->set_count(stats->remove_not_found);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_REMOVE_FAIL);
    entry->set_count(stats->remove_fail);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_UPDATE);
    entry->set_count(stats->update);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_UPDATE_FAIL);
    entry->set_count(stats->update_fail);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_GET);
    entry->set_count(stats->get);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_GET_FAIL);
    entry->set_count(stats->get_fail);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_RESERVE);
    entry->set_count(stats->reserve);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_RESERVE_FAIL);
    entry->set_count(stats->reserve_fail);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_RELEASE);
    entry->set_count(stats->release);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_API_STATS_RELEASE_FAIL);
    entry->set_count(stats->release_fail);
}

static inline void
pds_table_stats_to_proto (pds::TableStats *proto_stats,
                          sdk::table::sdk_table_stats_t *stats)
{
    auto entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_STATS_ENTRIES);
    entry->set_count(stats->entries);

    entry = proto_stats->add_entry();
    entry->set_type(pds::TABLE_STATS_COLLISIONS);
    entry->set_count(stats->collisions);
}

static inline void
pds_table_stats_entry_to_proto (pds_table_stats_t *stats, void *ctxt)
{
    pds::TableStatsGetResponse *rsp = (pds::TableStatsGetResponse *)ctxt;
    auto response = rsp->add_response();
    auto api_stats = response->mutable_apistats();
    auto table_stats = response->mutable_tablestats();

    response->set_tablename(stats->table_name);
    pds_table_api_stats_to_proto(api_stats, &stats->api_stats);
    pds_table_stats_to_proto(table_stats, &stats->table_stats);
    rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
}

static inline void
pds_pb_stats_port_to_proto (pds::PacketBufferPort *buf_port, uint32_t port)
{
    if ((port >= TM_UPLINK_PORT_BEGIN) && (port <= TM_UPLINK_PORT_END)) {
        buf_port->set_porttype(pds::PACKET_BUFFER_PORT_TYPE_UPLINK);
        buf_port->set_portnum(port-TM_UPLINK_PORT_BEGIN);
    } else if ((port >= TM_DMA_PORT_BEGIN) && (port <= TM_DMA_PORT_END)) {
        buf_port->set_porttype(pds::PACKET_BUFFER_PORT_TYPE_DMA);
        buf_port->set_portnum(TM_PORT_DMA);
    } else if (port == TM_PORT_INGRESS) {
        buf_port->set_porttype(pds::PACKET_BUFFER_PORT_TYPE_P4IG);
        buf_port->set_portnum(TM_PORT_INGRESS);
    } else if (port == TM_PORT_EGRESS) {
        buf_port->set_porttype(pds::PACKET_BUFFER_PORT_TYPE_P4EG);
        buf_port->set_portnum(TM_PORT_EGRESS);
    }
}

static inline void
pds_session_debug_stats_to_proto (uint32_t idx, pds_session_debug_stats_t *stats, void *ctxt)
{
    pds::SessionStatsGetResponse *rsp = (pds::SessionStatsGetResponse *)ctxt;
    auto proto_stats = rsp->add_stats();

    proto_stats->set_statsindex(idx);
    proto_stats->set_initiatorflowpkts(stats->iflow_packet_count);
    proto_stats->set_initiatorflowbytes(stats->iflow_bytes_count);
    proto_stats->set_responderflowpkts(stats->rflow_packet_count);
    proto_stats->set_responderflowbytes(stats->rflow_bytes_count);
}

static inline void
pds_session_to_proto (void *ctxt)
{
}

static inline void
pds_ipv4_flow_to_proto (ftlite::internal::ipv4_entry_t *ipv4_entry,
                        void *ctxt)
{
    flow_get_t *fget = (flow_get_t *)ctxt;
    auto flow = fget->msg.add_flow();
    auto key = flow->mutable_key();
    auto srcaddr = key->mutable_srcaddr();
    auto dstaddr = key->mutable_dstaddr();

    srcaddr->set_af(types::IP_AF_INET);
    srcaddr->set_v4addr(ipv4_entry->src);
    dstaddr->set_af(types::IP_AF_INET);
    dstaddr->set_v4addr(ipv4_entry->dst);
    key->set_srcport(ipv4_entry->sport);
    key->set_dstport(ipv4_entry->dport);
    key->set_ipproto(ipv4_entry->proto);

    flow->set_vpc(ipv4_entry->vpc_id);
    flow->set_flowrole(ipv4_entry->flow_role);
    flow->set_sessionidx(ipv4_entry->session_index);
    flow->set_epoch(ipv4_entry->epoch);

    fget->count ++;
    if (fget->count == FLOW_GET_STREAM_COUNT) {
        fget->msg.set_apistatus(types::ApiStatus::API_STATUS_OK);
        fget->writer->Write(fget->msg);
        fget->msg.Clear();
        fget->count = 0;
    }
}

static inline void
pds_ipv6_flow_to_proto (ftlite::internal::ipv6_entry_t *ipv6_entry,
                        void *ctxt)
{
    flow_get_t *fget = (flow_get_t *)ctxt;
    auto flow = fget->msg.add_flow();
    auto key = flow->mutable_key();
    auto srcaddr = key->mutable_srcaddr();
    auto dstaddr = key->mutable_dstaddr();

    srcaddr->set_af(types::IP_AF_INET6);
    srcaddr->set_v6addr(std::string((const char *)(ipv6_entry->src),
                        IP6_ADDR8_LEN));
    dstaddr->set_af(types::IP_AF_INET6);
    srcaddr->set_v6addr(std::string((const char *)(ipv6_entry->dst),
                        IP6_ADDR8_LEN));
    key->set_srcport(ipv6_entry->sport);
    key->set_dstport(ipv6_entry->dport);
    key->set_ipproto(ipv6_entry->proto);

    flow->set_vpc(ipv6_entry->vpc_id);
    flow->set_flowrole(ipv6_entry->flow_role);
    flow->set_sessionidx(ipv6_entry->session_index);
    flow->set_epoch(ipv6_entry->epoch);

    fget->count ++;
    if (fget->count == FLOW_GET_STREAM_COUNT) {
        fget->msg.set_apistatus(types::ApiStatus::API_STATUS_OK);
        fget->writer->Write(fget->msg);
        fget->msg.Clear();
        fget->count = 0;
    }
}

static inline void
pds_flow_to_proto (ftlite::internal::ipv4_entry_t *ipv4_entry,
                   ftlite::internal::ipv6_entry_t *ipv6_entry,
                   void *ctxt)
{
    if (ipv4_entry) {
        pds_ipv4_flow_to_proto(ipv4_entry, ctxt);
    } else if (ipv6_entry) {
        pds_ipv6_flow_to_proto(ipv6_entry, ctxt);
    } else {
        flow_get_t *fget = (flow_get_t *)ctxt;
        if (fget->count) {
            fget->msg.set_apistatus(types::ApiStatus::API_STATUS_OK);
            fget->writer->Write(fget->msg);
            fget->msg.Clear();
            fget->count = 0;
        }
    }
}

static inline void
pds_pb_stats_entry_to_proto (pds_pb_debug_stats_t *pds_stats, void *ctxt)
{
    sdk::platform::capri::tm_pb_debug_stats_t *stats = &pds_stats->stats;
    sdk::platform::capri::capri_queue_stats_t *qos_queue_stats =
        &pds_stats->qos_queue_stats;
    pds::PbStatsGetResponse *rsp = (pds::PbStatsGetResponse *)ctxt;
    auto pb_stats = rsp->mutable_pbstats()->add_portstats();
    auto port = pb_stats->mutable_packetbufferport();
    auto buffer_stats = pb_stats->mutable_bufferstats();
    auto oflow_fifo_stats = pb_stats->mutable_oflowfifostats();
    auto q_stats = pb_stats->mutable_qosqueuestats();

    pds_pb_stats_port_to_proto(port, pds_stats->port);

    buffer_stats->set_sopcountin(stats->buffer_stats.sop_count_in);
    buffer_stats->set_eopcountin(stats->buffer_stats.eop_count_in);
    buffer_stats->set_sopcountout(stats->buffer_stats.sop_count_out);
    buffer_stats->set_eopcountout(stats->buffer_stats.eop_count_out);

    auto drop_stats = buffer_stats->mutable_dropcounts();
    for (int i = sdk::platform::capri::BUFFER_INTRINSIC_DROP; i < sdk::platform::capri::BUFFER_DROP_MAX; i ++) {
        auto drop_stats_entry = drop_stats->add_statsentries();
        drop_stats_entry->set_reasons(pds::BufferDropReasons(i));
        drop_stats_entry->set_dropcount(stats->buffer_stats.drop_counts[i]);
    }

    oflow_fifo_stats->set_sopcountin(stats->oflow_fifo_stats.sop_count_in);
    oflow_fifo_stats->set_eopcountin(stats->oflow_fifo_stats.eop_count_in);
    oflow_fifo_stats->set_sopcountout(stats->oflow_fifo_stats.sop_count_out);
    oflow_fifo_stats->set_eopcountout(stats->oflow_fifo_stats.eop_count_out);

    auto drop_counts = oflow_fifo_stats->mutable_dropcounts();
    drop_counts->add_entry()->set_type(pds::OflowFifoDropType::OCCUPANCY_DROP);
    drop_counts->mutable_entry(0)->set_count(stats->oflow_fifo_stats.drop_counts.occupancy_drop_count);
    drop_counts->add_entry()->set_type(pds::OflowFifoDropType::EMERGENCY_STOP_DROP);
    drop_counts->mutable_entry(1)->set_count(stats->oflow_fifo_stats.drop_counts.emergency_stop_drop_count);
    drop_counts->add_entry()->set_type(pds::OflowFifoDropType::WRITE_BUFFER_ACK_FILL_UP_DROP);
    drop_counts->mutable_entry(2)->set_count(stats->oflow_fifo_stats.drop_counts.write_buffer_ack_fill_up_drop_count);
    drop_counts->add_entry()->set_type(pds::OflowFifoDropType::WRITE_BUFFER_ACK_FULL_DROP);
    drop_counts->mutable_entry(3)->set_count(stats->oflow_fifo_stats.drop_counts.write_buffer_ack_full_drop_count);
    drop_counts->add_entry()->set_type(pds::OflowFifoDropType::WRITE_BUFFER_FULL_DROP);
    drop_counts->mutable_entry(4)->set_count(stats->oflow_fifo_stats.drop_counts.write_buffer_full_drop_count);
    drop_counts->add_entry()->set_type(pds::OflowFifoDropType::CONTROL_FIFO_FULL_DROP);
    drop_counts->mutable_entry(5)->set_count(stats->oflow_fifo_stats.drop_counts.control_fifo_full_drop_count);

    for (int i = 0; i < CAPRI_TM_MAX_QUEUES; i++) {
        if (qos_queue_stats->iq_stats[i].iq.valid) {
            auto input_stats = q_stats->add_inputqueuestats();
            auto iq_stats = &qos_queue_stats->iq_stats[i].stats;
            input_stats->set_inputqueueidx(qos_queue_stats->iq_stats[i].iq.queue);
            input_stats->mutable_oflowfifostats()->set_goodpktsin(iq_stats->oflow.good_pkts_in);
            input_stats->mutable_oflowfifostats()->set_goodpktsout(iq_stats->oflow.good_pkts_out);
            input_stats->mutable_oflowfifostats()->set_erroredpktsin(iq_stats->oflow.errored_pkts_in);
            input_stats->mutable_oflowfifostats()->set_fifodepth(iq_stats->oflow.fifo_depth);
            input_stats->mutable_oflowfifostats()->set_maxfifodepth(iq_stats->oflow.max_fifo_depth);
            input_stats->set_bufferoccupancy(iq_stats->buffer_occupancy);
            input_stats->set_peakoccupancy(iq_stats->peak_occupancy);
            input_stats->set_portmonitor(iq_stats->port_monitor);
        }
        if (qos_queue_stats->oq_stats[i].oq.valid) {
            auto output_stats = q_stats->add_outputqueuestats();
            auto oq_stats = &qos_queue_stats->oq_stats[i].stats;
            output_stats->set_outputqueueidx(qos_queue_stats->oq_stats[i].oq.queue);
            output_stats->set_queuedepth(oq_stats->queue_depth);
            output_stats->set_portmonitor(oq_stats->port_monitor);
        }
    }

    rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
}

// populate proto buf spec from mirror session API spec
static inline sdk_ret_t
pds_mirror_session_api_spec_to_proto (pds::MirrorSessionSpec *proto_spec,
                                      const pds_mirror_session_spec_t *api_spec)
{
    proto_spec->set_id(api_spec->key.id);
    proto_spec->set_snaplen(api_spec->snap_len);
    switch (api_spec->type) {
    case PDS_MIRROR_SESSION_TYPE_RSPAN:
        {
            pds::RSpanSpec *proto_rspan = proto_spec->mutable_rspanspec();
            pds_encap_to_proto_encap(proto_rspan->mutable_encap(),
                                     &api_spec->rspan_spec.encap);
            proto_rspan->set_interfaceid(api_spec->rspan_spec.interface);
        }
        break;

    case PDS_MIRROR_SESSION_TYPE_ERSPAN:
        {
            pds::ERSpanSpec *proto_erspan = proto_spec->mutable_erspanspec();
            ipaddr_api_spec_to_proto_spec(proto_erspan->mutable_dstip(),
                                          &api_spec->erspan_spec.dst_ip);
            ipaddr_api_spec_to_proto_spec(proto_erspan->mutable_srcip(),
                                          &api_spec->erspan_spec.src_ip);
            proto_erspan->set_dscp(api_spec->erspan_spec.dscp);
            proto_erspan->set_spanid(api_spec->erspan_spec.span_id);
            proto_erspan->set_vpcid(api_spec->erspan_spec.vpc.id);
        }
        break;

    default:
        PDS_TRACE_ERR("Unknown mirror session type {}", api_spec->type);
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

// populate proto buf status from mirror session API status
static inline void
pds_mirror_session_api_status_to_proto (pds::MirrorSessionStatus *proto_status,
                                        const pds_mirror_session_status_t *api_status)
{
}

// populate proto buf stats from mirror session API stats
static inline void
pds_mirror_session_api_stats_to_proto (pds::MirrorSessionStats *proto_stats,
                                       const pds_mirror_session_stats_t *api_stats)
{
}

// populate proto buf from mirror session API info
static inline void
pds_mirror_session_api_info_to_proto (const pds_mirror_session_info_t *api_info,
                                      void *ctxt)
{
    pds::MirrorSessionGetResponse *proto_rsp =
        (pds::MirrorSessionGetResponse *)ctxt;
    auto mirror_session = proto_rsp->add_response();
    pds::MirrorSessionSpec *proto_spec = mirror_session->mutable_spec();
    pds::MirrorSessionStatus *proto_status = mirror_session->mutable_status();
    pds::MirrorSessionStats *proto_stats = mirror_session->mutable_stats();

    pds_mirror_session_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_mirror_session_api_status_to_proto(proto_status, &api_info->status);
    pds_mirror_session_api_stats_to_proto(proto_stats, &api_info->stats);
}

// build mirror session API spec from protobuf spec
static inline sdk_ret_t
pds_mirror_session_proto_to_api_spec (pds_mirror_session_spec_t *api_spec,
                                      const pds::MirrorSessionSpec &proto_spec)
{
    api_spec->key.id = proto_spec.id();
    api_spec->snap_len = proto_spec.snaplen();
    if (proto_spec.has_rspanspec()) {
        api_spec->type = PDS_MIRROR_SESSION_TYPE_RSPAN;
        api_spec->rspan_spec.encap =
            proto_encap_to_pds_encap(proto_spec.rspanspec().encap());
        if (api_spec->rspan_spec.encap.type != PDS_ENCAP_TYPE_DOT1Q) {
            PDS_TRACE_ERR("Invalid encap type {} in RSPAN mirror session {} "
                          "spec, only PDS_ENCAP_TYPE_DOT1Q encap type is valid",
                          api_spec->rspan_spec.encap.type, api_spec->key.id);
            return SDK_RET_INVALID_ARG;
        }
        if (api_spec->rspan_spec.encap.val.vlan_tag == 0) {
            PDS_TRACE_ERR("Invalid vlan tag 0 in RSPAN mirror session {} spec",
                          api_spec->key.id);
            return SDK_RET_INVALID_ARG;
        }
        api_spec->rspan_spec.interface =
            proto_spec.rspanspec().interfaceid();
    } else if (proto_spec.has_erspanspec()) {
        if (!proto_spec.erspanspec().has_dstip() ||
            !proto_spec.erspanspec().has_srcip()) {
            PDS_TRACE_ERR("src IP or dst IP missing in mirror session {} spec",
                          api_spec->key.id);
            return SDK_RET_INVALID_ARG;
        }
        types::IPAddress dstip = proto_spec.erspanspec().dstip();
        types::IPAddress srcip = proto_spec.erspanspec().srcip();
        api_spec->type = PDS_MIRROR_SESSION_TYPE_ERSPAN;
        ipaddr_proto_spec_to_api_spec(&api_spec->erspan_spec.dst_ip, dstip);
        ipaddr_proto_spec_to_api_spec(&api_spec->erspan_spec.src_ip, srcip);
        api_spec->erspan_spec.dscp = proto_spec.erspanspec().dscp();
        api_spec->erspan_spec.span_id = proto_spec.erspanspec().spanid();
        api_spec->erspan_spec.vpc.id = proto_spec.erspanspec().vpcid();
    } else {
        PDS_TRACE_ERR("rspan & erspan config missing in mirror session {} spec",
                      api_spec->key.id);
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

// populate proto buf spec from device API spec
static inline void
pds_device_api_spec_to_proto (pds::DeviceSpec *proto_spec,
                              const pds_device_spec_t *api_spec)
{
    ipaddr_api_spec_to_proto_spec(proto_spec->mutable_ipaddr(),
                                  &api_spec->device_ip_addr);
    proto_spec->set_macaddr(MAC_TO_UINT64(api_spec->device_mac_addr));
    ipaddr_api_spec_to_proto_spec(proto_spec->mutable_gatewayip(),
                                  &api_spec->gateway_ip_addr);
}

// populate proto buf status from device API status
static inline void
pds_device_api_status_to_proto (pds::DeviceStatus *proto_status,
                                const pds_device_status_t *api_status)
{
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
    } else {
        return SDK_RET_INVALID_ARG;
    }
    if ((gwipaddr.af() == types::IP_AF_INET) ||
        (gwipaddr.af() == types::IP_AF_INET6)) {
        ipaddr_proto_spec_to_api_spec(&api_spec->gateway_ip_addr, gwipaddr);
    } else {
        return SDK_RET_INVALID_ARG;
    }
    MAC_UINT64_TO_ADDR(api_spec->device_mac_addr, macaddr);
    return SDK_RET_OK;
}

// populate proto buf spec from subnet API spec
static inline void
pds_subnet_api_spec_to_proto (pds::SubnetSpec *proto_spec,
                              const pds_subnet_spec_t *api_spec)
{
    proto_spec->set_id(api_spec->key.id);
    proto_spec->set_vpcid(api_spec->vpc.id);
    ipv4pfx_api_spec_to_proto_spec(
                    proto_spec->mutable_v4prefix(), &api_spec->v4_prefix);
    ippfx_api_spec_to_proto_spec(
                    proto_spec->mutable_v6prefix(), &api_spec->v6_prefix);
    proto_spec->set_ipv4virtualrouterip(api_spec->v4_vr_ip);
    proto_spec->set_ipv6virtualrouterip(&api_spec->v6_vr_ip.addr.v6_addr.addr8,
                                        IP6_ADDR8_LEN);
    proto_spec->set_virtualroutermac(MAC_TO_UINT64(api_spec->vr_mac));
    proto_spec->set_v4routetableid(api_spec->v4_route_table.id);
    proto_spec->set_v6routetableid(api_spec->v6_route_table.id);
    proto_spec->set_ingv4securitypolicyid(api_spec->ing_v4_policy.id);
    proto_spec->set_ingv6securitypolicyid(api_spec->ing_v6_policy.id);
    proto_spec->set_egv4securitypolicyid(api_spec->egr_v4_policy.id);
    proto_spec->set_egv6securitypolicyid(api_spec->egr_v6_policy.id);
    pds_encap_to_proto_encap(proto_spec->mutable_fabricencap(),
                             &api_spec->fabric_encap);
}

// populate proto buf status from subnet API status
static inline void
pds_subnet_api_status_to_proto (pds::SubnetStatus *proto_status,
                                const pds_subnet_status_t *api_status)
{
}

// populate proto buf stats from subnet API stats
static inline void
pds_subnet_api_stats_to_proto (pds::SubnetStats *proto_stats,
                               const pds_subnet_stats_t *api_stats)
{
}

// populate proto buf from subnet API info
static inline void
pds_subnet_api_info_to_proto (const pds_subnet_info_t *api_info, void *ctxt)
{
    pds::SubnetGetResponse *proto_rsp = (pds::SubnetGetResponse *)ctxt;
    auto subnet = proto_rsp->add_response();
    pds::SubnetSpec *proto_spec = subnet->mutable_spec();
    pds::SubnetStatus *proto_status = subnet->mutable_status();
    pds::SubnetStats *proto_stats = subnet->mutable_stats();

    pds_subnet_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_subnet_api_status_to_proto(proto_status, &api_info->status);
    pds_subnet_api_stats_to_proto(proto_stats, &api_info->stats);
}

// Build subnet API spec from proto buf spec
static inline void
pds_subnet_proto_to_api_spec (pds_subnet_spec_t *api_spec,
                              const pds::SubnetSpec &proto_spec)
{
    api_spec->key.id = proto_spec.id();
    api_spec->vpc.id = proto_spec.vpcid();
    ipv4pfx_proto_spec_to_api_spec(&api_spec->v4_prefix, proto_spec.v4prefix());
    ippfx_proto_spec_to_api_spec(&api_spec->v6_prefix, proto_spec.v6prefix());
    api_spec->v4_vr_ip = proto_spec.ipv4virtualrouterip();
    api_spec->v6_vr_ip.af = IP_AF_IPV6;
    memcpy(api_spec->v6_vr_ip.addr.v6_addr.addr8,
           proto_spec.ipv6virtualrouterip().c_str(), IP6_ADDR8_LEN);
    MAC_UINT64_TO_ADDR(api_spec->vr_mac, proto_spec.virtualroutermac());
    api_spec->v4_route_table.id = proto_spec.v4routetableid();
    api_spec->v6_route_table.id = proto_spec.v6routetableid();
    api_spec->ing_v4_policy.id = proto_spec.ingv4securitypolicyid();
    api_spec->ing_v6_policy.id = proto_spec.ingv6securitypolicyid();
    api_spec->egr_v4_policy.id = proto_spec.egv4securitypolicyid();
    api_spec->egr_v6_policy.id = proto_spec.egv6securitypolicyid();
    api_spec->fabric_encap = proto_encap_to_pds_encap(proto_spec.fabricencap());
}

// Build VPC API spec from protobuf spec
static inline void
pds_vpc_proto_to_api_spec (pds_vpc_spec_t *api_spec,
                           const pds::VPCSpec &proto_spec)
{
    pds::VPCType type;

    api_spec->key.id = proto_spec.id();
    type = proto_spec.type();
    if (type == pds::VPC_TYPE_TENANT) {
        api_spec->type = PDS_VPC_TYPE_TENANT;
    } else if (type == pds::VPC_TYPE_SUBSTRATE) {
        api_spec->type = PDS_VPC_TYPE_SUBSTRATE;
    }
    ipv4pfx_proto_spec_to_api_spec(&api_spec->v4_prefix, proto_spec.v4prefix());
    ippfx_proto_spec_to_api_spec(&api_spec->v6_prefix, proto_spec.v6prefix());
    ippfx_proto_spec_to_api_spec(&api_spec->nat46_prefix, proto_spec.nat46prefix());
    api_spec->fabric_encap = proto_encap_to_pds_encap(proto_spec.fabricencap());
    MAC_UINT64_TO_ADDR(api_spec->vr_mac, proto_spec.virtualroutermac());
    api_spec->v4_route_table.id = proto_spec.v4routetableid();
    api_spec->v6_route_table.id = proto_spec.v6routetableid();
}

// populate proto buf spec from vpc API spec
static inline void
pds_vpc_api_spec_to_proto (pds::VPCSpec *proto_spec,
                           const pds_vpc_spec_t *api_spec)
{
    proto_spec->set_id(api_spec->key.id);
    if (api_spec->type == PDS_VPC_TYPE_TENANT) {
        proto_spec->set_type(pds::VPC_TYPE_TENANT);
    } else if (api_spec->type == PDS_VPC_TYPE_SUBSTRATE) {
        proto_spec->set_type(pds::VPC_TYPE_SUBSTRATE);
    }
    ipv4pfx_api_spec_to_proto_spec(proto_spec->mutable_v4prefix(), &api_spec->v4_prefix);
    ippfx_api_spec_to_proto_spec(proto_spec->mutable_v6prefix(), &api_spec->v6_prefix);
    ippfx_api_spec_to_proto_spec(proto_spec->mutable_nat46prefix(), &api_spec->nat46_prefix);
    pds_encap_to_proto_encap(proto_spec->mutable_fabricencap(), &api_spec->fabric_encap);
    proto_spec->set_virtualroutermac(MAC_TO_UINT64(api_spec->vr_mac));
    proto_spec->set_v4routetableid(api_spec->v4_route_table.id);
    proto_spec->set_v6routetableid(api_spec->v6_route_table.id);
}

// populate proto buf status from vpc API status
static inline void
pds_vpc_api_status_to_proto (pds::VPCStatus *proto_status,
                             const pds_vpc_status_t *api_status)
{
}

// populate proto buf stats from vpc API stats
static inline void
pds_vpc_api_stats_to_proto (pds::VPCStats *proto_stats,
                            const pds_vpc_stats_t *api_stats)
{
}

// populate proto buf from vpc API info
static inline void
pds_vpc_api_info_to_proto (const pds_vpc_info_t *api_info, void *ctxt)
{
    pds::VPCGetResponse *proto_rsp = (pds::VPCGetResponse *)ctxt;
    auto vpc = proto_rsp->add_response();
    pds::VPCSpec *proto_spec = vpc->mutable_spec();
    pds::VPCStatus *proto_status = vpc->mutable_status();
    pds::VPCStats *proto_stats = vpc->mutable_stats();

    pds_vpc_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_vpc_api_status_to_proto(proto_status, &api_info->status);
    pds_vpc_api_stats_to_proto(proto_stats, &api_info->stats);
}

// Build VPC API spec from protobuf spec
static inline void
pds_local_mapping_proto_to_api_spec (pds_local_mapping_spec_t *local_spec,
                                     const pds::MappingSpec &proto_spec)
{
    pds::MappingKey key;

    key = proto_spec.id();
    local_spec->key.vpc.id = key.vpcid();
    ipaddr_proto_spec_to_api_spec(&local_spec->key.ip_addr, key.ipaddr());
    local_spec->subnet.id = proto_spec.subnetid();
    local_spec->vnic.id = proto_spec.vnicid();
    if (proto_spec.has_publicip()) {
        if (proto_spec.publicip().af() == types::IP_AF_INET ||
            proto_spec.publicip().af() == types::IP_AF_INET6) {
            local_spec->public_ip_valid = true;
            ipaddr_proto_spec_to_api_spec(&local_spec->public_ip,
                                          proto_spec.publicip());
        }
    }
    if (proto_spec.has_providerip()) {
        if (proto_spec.providerip().af() == types::IP_AF_INET ||
            proto_spec.providerip().af() == types::IP_AF_INET6) {
            local_spec->provider_ip_valid = true;
            ipaddr_proto_spec_to_api_spec(&local_spec->provider_ip,
                                          proto_spec.providerip());
        }
    }
    local_spec->svc_tag = proto_spec.servicetag();
    MAC_UINT64_TO_ADDR(local_spec->vnic_mac, proto_spec.macaddr());
    local_spec->fabric_encap = proto_encap_to_pds_encap(proto_spec.encap());
}

static inline void
pds_remote_mapping_proto_to_api_spec (pds_remote_mapping_spec_t *remote_spec,
                                      const pds::MappingSpec &proto_spec)
{
    pds::MappingKey key;

    key = proto_spec.id();
    remote_spec->key.vpc.id = key.vpcid();
    ipaddr_proto_spec_to_api_spec(&remote_spec->key.ip_addr, key.ipaddr());
    remote_spec->subnet.id = proto_spec.subnetid();
    ipaddr_proto_spec_to_api_spec(&remote_spec->tep.ip_addr,
                                  proto_spec.tunnelip());
    MAC_UINT64_TO_ADDR(remote_spec->vnic_mac, proto_spec.macaddr());
    remote_spec->fabric_encap = proto_encap_to_pds_encap(proto_spec.encap());
    if (proto_spec.has_providerip()) {
        if (proto_spec.providerip().af() == types::IP_AF_INET ||
            proto_spec.providerip().af() == types::IP_AF_INET6) {
            remote_spec->provider_ip_valid = true;
            ipaddr_proto_spec_to_api_spec(&remote_spec->provider_ip,
                                          proto_spec.providerip());
        }
    }
}

static inline void
pds_port_stats_to_proto (pds::PortStats *stats,
                         sdk::linkmgr::port_args_t *port_info)
{
    if (port_info->port_type == port_type_t::PORT_TYPE_ETH) {
        for (uint32_t i = 0; i < MAX_MAC_STATS; i++) {
            auto macstats = stats->add_macstats();
            macstats->set_type(pds::MacStatsType(i));
            macstats->set_count(port_info->stats_data[i]);
        }
    } else if (port_info->port_type == port_type_t::PORT_TYPE_MGMT) {
        for (uint32_t i = 0; i < MAX_MGMT_MAC_STATS; i++) {
            auto macstats = stats->add_mgmtmacstats();
            macstats->set_type(pds::MgmtMacStatsType(i));
            macstats->set_count(port_info->stats_data[i]);
        }
    }
}

static inline void
pds_port_spec_to_proto (pds::PortSpec *spec, sdk::linkmgr::port_args_t *port_info)
{
    spec->set_id(port_info->port_num);
    switch(port_info->admin_state) {
    case port_admin_state_t::PORT_ADMIN_STATE_DOWN:
        spec->set_adminstate(pds::PORT_ADMIN_STATE_DOWN);
        break;
    case port_admin_state_t::PORT_ADMIN_STATE_UP:
        spec->set_adminstate(pds::PORT_ADMIN_STATE_UP);
        break;
    default:
        spec->set_adminstate(pds::PORT_ADMIN_STATE_NONE);
        break;
    }
    spec->set_speed(pds_port_sdk_speed_to_proto_speed(
                                      port_info->port_speed));
    spec->set_fectype(pds_port_sdk_fec_type_to_proto_fec_type
                                      (port_info->fec_type));
    spec->set_autonegen(port_info->auto_neg_cfg);
    spec->set_debouncetimeout(port_info->debounce_time);
    spec->set_mtu(port_info->mtu);
    spec->set_pausetype(pds_port_sdk_pause_type_to_proto_pause_type
                                      (port_info->pause));
    spec->set_loopbackmode(pds_port_sdk_loopback_mode_to_proto_loopback_mode(
                                     port_info->loopback_mode));
    spec->set_numlanes(port_info->num_lanes_cfg);
}

static inline void
pds_port_status_to_proto (pds::PortStatus *status,
                              sdk::linkmgr::port_args_t *port_info)
{
    auto link_status = status->mutable_linkstatus();
    auto xcvr_status = status->mutable_xcvrstatus();

    switch (port_info->oper_status) {
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

    switch (port_info->port_speed) {
    case port_speed_t::PORT_SPEED_1G:
        link_status->set_portspeed(pds::PORT_SPEED_1G);
        break;
    case port_speed_t::PORT_SPEED_10G:
        link_status->set_portspeed(pds::PORT_SPEED_10G);
        break;
    case port_speed_t::PORT_SPEED_25G:
        link_status->set_portspeed(pds::PORT_SPEED_25G);
        break;
    case port_speed_t::PORT_SPEED_40G:
        link_status->set_portspeed(pds::PORT_SPEED_40G);
        break;
    case port_speed_t::PORT_SPEED_50G:
        link_status->set_portspeed(pds::PORT_SPEED_50G);
        break;
    case port_speed_t::PORT_SPEED_100G:
        link_status->set_portspeed(pds::PORT_SPEED_100G);
        break;
    default:
        link_status->set_portspeed(pds::PORT_SPEED_NONE);
        break;
    }
    link_status->set_autonegen(port_info->auto_neg_enable);
    link_status->set_numlanes(port_info->num_lanes);

    xcvr_status->set_port(port_info->xcvr_event_info.phy_port);
    xcvr_status->set_state(pds::PortXcvrState(port_info->xcvr_event_info.state));
    xcvr_status->set_pid(pds::PortXcvrPid(port_info->xcvr_event_info.pid));
    xcvr_status->set_mediatype(pds::MediaType(port_info->xcvr_event_info.cable_type));
    xcvr_status->set_xcvrsprom(std::string((char*)&port_info->xcvr_event_info.xcvr_sprom));
}

static inline void
pds_port_to_proto (sdk::linkmgr::port_args_t *port_info, void *ctxt)
{
    pds::PortGetResponse *rsp = (pds::PortGetResponse *)ctxt;
    pds::Port *port = rsp->add_response();
    pds::PortSpec *spec = port->mutable_spec();
    pds::PortStats *stats = port->mutable_stats();
    pds::PortStatus *status = port->mutable_status();

    pds_port_spec_to_proto(spec, port_info);
    pds_port_stats_to_proto(stats, port_info);
    pds_port_status_to_proto(status, port_info);
}

#endif    // __AGENT_SVC_SPECS_HPP__
