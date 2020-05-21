#include "nic/hal/plugins/cfg/nw/interface.hpp"
#include "nic/hal/plugins/cfg/nw/endpoint.hpp"
#include "nic/hal/plugins/cfg/nw/session.hpp"
#include "nic/hal/plugins/cfg/nw/nw.hpp"
#include "nic/hal/plugins/cfg/telemetry/telemetry.hpp"
#include "nic/hal/plugins/sfw/cfg/nwsec.hpp"
#include "gen/proto/interface.pb.h"
#include "gen/proto/l2segment.pb.h"
#include "gen/proto/telemetry.pb.h"
#include "gen/proto/vrf.pb.h"
#include "gen/proto/nwsec.pb.h"
#include "gen/proto/endpoint.pb.h"
#include "gen/proto/session.pb.h"
#include "gen/proto/nw.pb.h"
#include "nic/hal/hal.hpp"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include "nic/hal/plugins/cfg/mcast/multicast.hpp"
#include "nic/hal/iris/include/hal_state.hpp"
#include "nic/hal/test/utils/hal_base_test.hpp"
#include "nic/hal/test/hal_calls/hal_calls.hpp"

using intf::InterfaceSpec;
using intf::InterfaceResponse;
using kh::InterfaceKeyHandle;
using l2segment::L2SegmentSpec;
using l2segment::L2SegmentResponse;
using vrf::VrfSpec;
using vrf::VrfResponse;
using intf::InterfaceL2SegmentSpec;
using intf::InterfaceL2SegmentResponse;
using intf::LifSpec;
using intf::LifResponse;
using kh::LifKeyHandle;
using nwsec::SecurityProfileSpec;
using nwsec::SecurityProfileResponse;
using endpoint::EndpointSpec;
using endpoint::EndpointResponse;
using endpoint::EndpointUpdateRequest;
using session::SessionSpec;
using session::SessionResponse;
using nw::NetworkSpec;
using nw::NetworkResponse;
using multicast::MulticastEntrySpec;
using multicast::MulticastEntryResponse;
using multicast::MulticastEntryDeleteRequest;
using multicast::MulticastEntryDeleteResponse;
using telemetry::MirrorSessionSpec;
using telemetry::MirrorSessionResponse;
using telemetry::ERSpanType;

hal_ret_t
create_uplink(uint32_t if_id, uint32_t port,
              uint32_t native_l2seg, bool is_oob)
{
    hal_ret_t            ret;
    InterfaceSpec       spec;
    InterfaceResponse   rsp;

    spec.set_type(intf::IF_TYPE_UPLINK);

    spec.mutable_key_or_handle()->set_interface_id(if_id);
    spec.mutable_if_uplink_info()->set_port_num(port);
    spec.mutable_if_uplink_info()->set_native_l2segment_id(native_l2seg);
    spec.mutable_if_uplink_info()->set_is_oob_management(is_oob);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::interface_create(spec, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
update_uplink(uint32_t if_id, uint32_t port,
              uint32_t native_l2seg, bool is_oob)
{
    hal_ret_t            ret;
    InterfaceSpec       spec;
    InterfaceResponse   rsp;

    spec.set_type(intf::IF_TYPE_UPLINK);

    spec.mutable_key_or_handle()->set_interface_id(if_id);
    spec.mutable_if_uplink_info()->set_port_num(port);
    spec.mutable_if_uplink_info()->set_native_l2segment_id(native_l2seg);
    spec.mutable_if_uplink_info()->set_is_oob_management(is_oob);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::interface_update(spec, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
create_tunnel(uint32_t if_id, uint32_t vrf, uint32_t src_ip, uint32_t dst_ip)
{
    hal_ret_t            ret;
    InterfaceSpec       spec;
    InterfaceResponse   rsp;

    spec.set_type(intf::IF_TYPE_TUNNEL);

    spec.mutable_key_or_handle()->set_interface_id(if_id);  
    spec.mutable_if_tunnel_info()->mutable_vrf_key_handle()->set_vrf_id(vrf);
    spec.mutable_if_tunnel_info()->set_encap_type(intf::IF_TUNNEL_ENCAP_TYPE_GRE);
    spec.mutable_if_tunnel_info()->mutable_gre_info()->mutable_source()->set_ip_af(types::IP_AF_INET);
    spec.mutable_if_tunnel_info()->mutable_gre_info()->mutable_source()->set_v4_addr(src_ip);
    spec.mutable_if_tunnel_info()->mutable_gre_info()->mutable_destination()->set_ip_af(types::IP_AF_INET);
    spec.mutable_if_tunnel_info()->mutable_gre_info()->mutable_destination()->set_v4_addr(dst_ip);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::interface_create(spec, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
create_collector (uint32_t cid, uint32_t vrf, uint32_t l2seg, 
                  uint32_t src_ip, uint32_t dst_ip)
{
    hal_ret_t            ret;
    CollectorSpec        spec;
    CollectorResponse    rsp;

    spec.mutable_key_or_handle()->set_collector_id(cid);
    spec.mutable_vrf_key_handle()->set_vrf_id(vrf);
    spec.mutable_l2seg_key_handle()->set_segment_id(l2seg);
    spec.mutable_src_ip()->set_ip_af(types::IP_AF_INET);
    spec.mutable_src_ip()->set_v4_addr(src_ip);
    spec.mutable_dest_ip()->set_ip_af(types::IP_AF_INET);
    spec.mutable_dest_ip()->set_v4_addr(dst_ip);
    spec.set_protocol(types::IPPROTO_UDP);
    spec.set_dest_port(1000);
    spec.set_export_interval(10);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::collector_create(spec, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
update_collector (gtest_collector_t *coll, bool create)
{
    hal_ret_t            ret;
    CollectorSpec        spec;
    CollectorResponse    rsp;

    spec.mutable_key_or_handle()->set_collector_id(coll->cid);
    spec.mutable_vrf_key_handle()->set_vrf_id(coll->vrf);
    spec.mutable_l2seg_key_handle()->set_segment_id(coll->l2seg);
    spec.mutable_src_ip()->set_ip_af(types::IP_AF_INET);
    spec.mutable_src_ip()->set_v4_addr(coll->src_ip);
    spec.mutable_dest_ip()->set_ip_af(types::IP_AF_INET);
    spec.mutable_dest_ip()->set_v4_addr(coll->dst_ip);
    spec.set_protocol(types::IPPROTO_UDP);
    spec.set_dest_port(coll->dest_port);
    spec.set_export_interval(coll->export_invl);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    if (create) {
        ret = hal::collector_create(spec, &rsp);
    } else {
        ret = hal::collector_update(spec, &rsp);
    }
    hal::hal_cfg_db_close();
    return ret;

}

hal_ret_t
delete_interface (uint32_t if_id)
{
    hal_ret_t               ret;
    InterfaceDeleteRequest  req;
    InterfaceDeleteResponse rsp;

    req.mutable_key_or_handle()->set_interface_id(if_id);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::interface_delete(req, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
create_vrf (uint32_t vrf_id, types::VrfType type, uint32_t des_if_id)
{
    hal_ret_t ret;
    VrfSpec spec;
    VrfResponse rsp;

    spec.mutable_key_or_handle()->set_vrf_id(vrf_id);
    spec.set_vrf_type(type);
    spec.mutable_designated_uplink()->set_interface_id(des_if_id);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::vrf_create(spec, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
update_l2seg(uint32_t vrf_id, uint32_t l2seg_id, uint32_t encap,
             uint32_t up_ifid[], uint32_t ifid_count,
             l2segment::MulticastFwdPolicy mcast_pol,
             l2segment::BroadcastFwdPolicy bcast_pol,
             bool is_swm,
             bool create)
{
    hal_ret_t           ret;
    L2SegmentSpec       spec;
    L2SegmentResponse   rsp;

    spec.mutable_vrf_key_handle()->set_vrf_id(vrf_id);
    spec.mutable_key_or_handle()->set_segment_id(l2seg_id);
    spec.mutable_wire_encap()->set_encap_type(types::ENCAP_TYPE_DOT1Q);
    spec.mutable_wire_encap()->set_encap_value(encap);
    spec.set_mcast_fwd_policy(mcast_pol);
    spec.set_bcast_fwd_policy(bcast_pol);
    spec.set_single_wire_management(is_swm);
    for (uint32_t i = 0; i < ifid_count; i++) {
        spec.add_if_key_handle()->set_interface_id(up_ifid[i]);
    }
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    if (create) {
        ret = hal::l2segment_create(spec, &rsp);
    } else {
        ret = hal::l2segment_update(spec, &rsp);
    }
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
delete_l2seg (uint32_t vrf_id, uint32_t l2seg_id)
{
    hal_ret_t ret;
    L2SegmentDeleteRequest  l2seg_del_req;
    L2SegmentDeleteResponse l2seg_del_rsp;

    l2seg_del_req.mutable_vrf_key_handle()->set_vrf_id(vrf_id);
    l2seg_del_req.mutable_key_or_handle()->set_segment_id(l2seg_id);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::l2segment_delete(l2seg_del_req, &l2seg_del_rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
create_lif(uint32_t lif_id, uint32_t if_id, types::LifType type, string name,
           bool is_oob, bool is_int, bool recv_bc,
           bool recv_allmc, bool recv_prom, bool vlan_strip,
           bool vlan_ins)
{
    hal_ret_t ret;
    LifSpec spec;
    LifResponse rsp;
    static uint64_t mac = 0x00AB00000001;

    spec.mutable_key_or_handle()->set_lif_id(lif_id);
    spec.set_type(type);
    spec.set_hw_lif_id(lif_id);
    spec.mutable_pinned_uplink_if_key_handle()->set_interface_id(if_id);
    spec.set_name(name);
    spec.mutable_packet_filter()->set_receive_broadcast(recv_bc);
    spec.mutable_packet_filter()->set_receive_all_multicast(recv_allmc);
    spec.mutable_packet_filter()->set_receive_promiscuous(recv_prom);
    spec.set_vlan_strip_en(vlan_strip);
    spec.set_vlan_insert_en(vlan_ins);
    spec.set_is_management(is_oob || is_int);
    spec.set_mac_address(mac);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::lif_create(spec, &rsp, NULL);
    hal::hal_cfg_db_close();

    mac++;
    return ret;
}

hal_ret_t
create_enic(uint32_t if_id, uint32_t lif_id, intf::IfEnicType type,
            uint32_t l2seg_id, uint32_t encap, // smart enic fields
            uint32_t native_l2seg_id) // classic fields
{
    hal_ret_t           ret;
    InterfaceSpec       enicif_spec;
    InterfaceResponse   enicif_rsp;

    enicif_spec.set_type(intf::IF_TYPE_ENIC);
    enicif_spec.mutable_if_enic_info()->mutable_lif_key_or_handle()->set_lif_id(lif_id);
    enicif_spec.mutable_key_or_handle()->set_interface_id(if_id);
    enicif_spec.mutable_if_enic_info()->set_enic_type(type);
    if (type == intf::IF_ENIC_TYPE_CLASSIC) {
        enicif_spec.mutable_if_enic_info()->mutable_classic_enic_info()->
            set_native_l2segment_id(native_l2seg_id);
    } else {
        enicif_spec.mutable_if_enic_info()->mutable_enic_info()->mutable_l2segment_key_handle()->set_segment_id(l2seg_id);
        enicif_spec.mutable_if_enic_info()->mutable_enic_info()->set_encap_vlan_id(encap);
    }
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::interface_create(enicif_spec, &enicif_rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
update_enic(gtest_enic_t *enic, gtest_oper_t oper)
{
    hal_ret_t           ret;
    InterfaceSpec       enicif_spec;
    InterfaceResponse   enicif_rsp;

    enicif_spec.set_type(intf::IF_TYPE_ENIC);
    enicif_spec.mutable_if_enic_info()->mutable_lif_key_or_handle()->set_lif_id(enic->lif_id);
    enicif_spec.mutable_key_or_handle()->set_interface_id(enic->if_id);
    enicif_spec.mutable_if_enic_info()->set_enic_type(enic->type);
    if (enic->type == intf::IF_ENIC_TYPE_CLASSIC) {
        enicif_spec.mutable_if_enic_info()->mutable_classic_enic_info()->
            set_native_l2segment_id(enic->native_l2seg_id);
    } else {
        enicif_spec.mutable_if_enic_info()->mutable_enic_info()->mutable_l2segment_key_handle()->set_segment_id(enic->l2seg_id);
        enicif_spec.mutable_if_enic_info()->mutable_enic_info()->set_encap_vlan_id(enic->encap);
    }
    for (uint32_t i = 0; i < enic->tx_mirr_count; i++) {
        enicif_spec.add_txmirrorsessions()->set_mirrorsession_id(enic->tx_mirr[i]);
    }
    for (uint32_t i = 0; i < enic->rx_mirr_count; i++) {
        enicif_spec.add_rxmirrorsessions()->set_mirrorsession_id(enic->rx_mirr[i]);
    }
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    if (oper == GTEST_CREATE) {
        ret = hal::interface_create(enicif_spec, &enicif_rsp);
    } else {
        ret = hal::interface_update(enicif_spec, &enicif_rsp);
    }
    hal::hal_cfg_db_close();
    return ret;
}


hal_ret_t
delete_enic (uint32_t if_id)
{
    hal_ret_t                ret;
    InterfaceDeleteRequest   req;
    InterfaceDeleteResponse  rsp;

    req.mutable_key_or_handle()->set_interface_id(if_id);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::interface_delete(req, &rsp);
    hal::hal_cfg_db_close();

    return ret;
}

hal_ret_t
create_ep(uint32_t vrf_id, uint32_t l2seg_id, uint32_t if_id, uint64_t mac)
{
    hal_ret_t ret;
    EndpointSpec             ep_spec;
    EndpointResponse         ep_rsp;

    ep_spec.mutable_vrf_key_handle()->set_vrf_id(vrf_id);
    ep_spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->mutable_l2segment_key_handle()->set_segment_id(l2seg_id);
    ep_spec.mutable_endpoint_attrs()->mutable_interface_key_handle()->set_interface_id(if_id);
    ep_spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->set_mac_address(mac);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::endpoint_create(ep_spec, &ep_rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
create_ep(uint32_t vrf_id, uint32_t l2seg_id, uint32_t if_id, uint64_t mac,
          uint32_t ip[], uint32_t ip_count)
{
    hal_ret_t ret;
    EndpointSpec             ep_spec;
    EndpointResponse         ep_rsp;

    ep_spec.mutable_vrf_key_handle()->set_vrf_id(vrf_id);
    ep_spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->mutable_l2segment_key_handle()->set_segment_id(l2seg_id);
    ep_spec.mutable_endpoint_attrs()->mutable_interface_key_handle()->set_interface_id(if_id);
    ep_spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->set_mac_address(mac);
    for (uint32_t i = 0; i < ip_count; i++) {
        ep_spec.mutable_endpoint_attrs()->add_ip_address();
        ep_spec.mutable_endpoint_attrs()->mutable_ip_address(i)->set_ip_af(types::IP_AF_INET);
        ep_spec.mutable_endpoint_attrs()->mutable_ip_address(i)->set_v4_addr(ip[i]);
    }
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::endpoint_create(ep_spec, &ep_rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
create_ep (gtest_ep_t *ep)
{
    hal_ret_t ret;
    EndpointSpec             ep_spec;
    EndpointResponse         ep_rsp;

    ep_spec.mutable_vrf_key_handle()->set_vrf_id(ep->vrf_id);
    ep_spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->mutable_l2segment_key_handle()->set_segment_id(ep->l2seg_id);
    ep_spec.mutable_endpoint_attrs()->mutable_interface_key_handle()->set_interface_id(ep->if_id);
    ep_spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->set_mac_address(ep->mac);
    for (uint32_t i = 0; i < ep->ip_count; i++) {
        ep_spec.mutable_endpoint_attrs()->add_ip_address();
        ep_spec.mutable_endpoint_attrs()->mutable_ip_address(i)->set_ip_af(types::IP_AF_INET);
        ep_spec.mutable_endpoint_attrs()->mutable_ip_address(i)->set_v4_addr(ep->ip[i]);
    }
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::endpoint_create(ep_spec, &ep_rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
update_ep(uint32_t vrf_id, uint32_t l2seg_id, uint32_t if_id, uint64_t mac,
          uint32_t ip[], uint32_t ip_count)
{
    hal_ret_t ret;
    EndpointUpdateRequest    ep_spec;
    EndpointResponse         ep_rsp;

    ep_spec.mutable_vrf_key_handle()->set_vrf_id(vrf_id);
    ep_spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->mutable_l2segment_key_handle()->set_segment_id(l2seg_id);
    ep_spec.mutable_endpoint_attrs()->mutable_interface_key_handle()->set_interface_id(if_id);
    ep_spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->set_mac_address(mac);
    for (uint32_t i = 0; i < ip_count; i++) {
        ep_spec.mutable_endpoint_attrs()->add_ip_address();
        ep_spec.mutable_endpoint_attrs()->mutable_ip_address(i)->set_ip_af(types::IP_AF_INET);
        ep_spec.mutable_endpoint_attrs()->mutable_ip_address(i)->set_v4_addr(ip[i]);
    }
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::endpoint_update(ep_spec, &ep_rsp);
    hal::hal_cfg_db_close();
    return ret;
}



hal_ret_t
delete_ep (uint32_t vrf_id, uint32_t l2seg_id, uint64_t mac)
{
    hal_ret_t ret;
    EndpointDeleteRequest       ep_del_req;
    EndpointDeleteResponse      ep_del_rsp;

    ep_del_req.mutable_vrf_key_handle()->set_vrf_id(vrf_id);
    ep_del_req.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->mutable_l2segment_key_handle()->set_segment_id(l2seg_id);
    ep_del_req.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->set_mac_address(mac);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::endpoint_delete(ep_del_req, &ep_del_rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
create_mcast (uint32_t l2seg_id, uint64_t mac, uint32_t ifid[], uint32_t ifid_count)
{
    hal_ret_t              ret;
    MulticastEntrySpec     spec;
    MulticastEntryResponse rsp;

    spec.mutable_key_or_handle()->mutable_key()->mutable_l2segment_key_handle()->set_segment_id(l2seg_id);
    for (uint32_t i = 0; i < ifid_count; i++) {
        spec.add_oif_key_handles()->set_interface_id(ifid[i]);
    }
    spec.mutable_key_or_handle()->mutable_key()->mutable_mac()->set_group(mac);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::multicastentry_create(spec, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
delete_mcast (uint32_t l2seg_id, uint64_t mac)
{
    hal_ret_t ret;
    MulticastEntryDeleteRequest  req;
    MulticastEntryDeleteResponse rsp;

    req.mutable_key_or_handle()->mutable_key()->mutable_l2segment_key_handle()->set_segment_id(l2seg_id);
    req.mutable_key_or_handle()->mutable_key()->mutable_mac()->set_group(mac);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::multicastentry_delete(req, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
create_mirror (uint32_t session_id, uint32_t vrf_id, uint32_t sip, uint32_t dip,
               bool vlan_strip_en, uint8_t erspan_type)
{
    hal_ret_t ret;
    MirrorSessionSpec spec;
    MirrorSessionResponse rsp;

    spec.mutable_vrf_key_handle()->set_vrf_id(vrf_id);
    spec.mutable_key_or_handle()->set_mirrorsession_id(session_id);
    spec.mutable_erspan_spec()->mutable_src_ip()->set_ip_af(::types::IP_AF_INET);
    spec.mutable_erspan_spec()->mutable_src_ip()->set_v4_addr(sip);
    spec.mutable_erspan_spec()->mutable_dest_ip()->set_ip_af(::types::IP_AF_INET);
    spec.mutable_erspan_spec()->mutable_dest_ip()->set_v4_addr(dip);
    spec.mutable_erspan_spec()->set_vlan_strip_en(vlan_strip_en);
    spec.mutable_erspan_spec()->set_span_id(session_id + 10);
    spec.mutable_erspan_spec()->set_type((ERSpanType)erspan_type);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::mirror_session_create(spec, &rsp);
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
update_mirror (gtest_mirror_t *mirror, bool create)
{
    hal_ret_t ret;
    MirrorSessionSpec spec;
    MirrorSessionResponse rsp;

    spec.mutable_vrf_key_handle()->set_vrf_id(mirror->vrf_id);
    spec.mutable_key_or_handle()->set_mirrorsession_id(mirror->session_id);
    spec.mutable_erspan_spec()->mutable_src_ip()->set_ip_af(::types::IP_AF_INET);
    spec.mutable_erspan_spec()->mutable_src_ip()->set_v4_addr(mirror->sip);
    spec.mutable_erspan_spec()->mutable_dest_ip()->set_ip_af(::types::IP_AF_INET);
    spec.mutable_erspan_spec()->mutable_dest_ip()->set_v4_addr(mirror->dip);
    spec.mutable_erspan_spec()->set_vlan_strip_en(mirror->vlan_strip_en);
    spec.mutable_erspan_spec()->set_span_id(mirror->span_id);
    spec.mutable_erspan_spec()->set_type((ERSpanType)mirror->erspan_type);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    if (create) {
        ret = hal::mirror_session_create(spec, &rsp);
    } else {
        ret = hal::mirror_session_update(spec, &rsp);
    }
    hal::hal_cfg_db_close();
    return ret;
}

hal_ret_t
update_flow_monitor (gtest_flow_mon_t *fmon, bool create)
{
    hal_ret_t ret;
    FlowMonitorRuleSpec spec;
    FlowMonitorRuleResponse rsp;

    spec.mutable_key_or_handle()->set_flowmonitorrule_id(fmon->rule_id);
    spec.mutable_vrf_key_handle()->set_vrf_id(fmon->vrf_id);
    auto action = spec.mutable_action();
    action->add_action(telemetry::RuleAction::MIRROR);
    for (uint32_t i = 0; i < fmon->sess_count; i++) {
        action->add_ms_key_handle()->set_mirrorsession_id(fmon->sess_id[i]);
    }

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    if (create) {
        ret = hal::flow_monitor_rule_create(spec, &rsp);
    } else {
        ret = hal::flow_monitor_rule_update(spec, &rsp, true);
    }
    hal::hal_cfg_db_close();
    return ret;
}


