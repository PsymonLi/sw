#include "nic/fte/test/fte_base_test.hpp"
#include "nic/fte/fte_flow.hpp"
#include "nic/fte/fte.hpp"
#include "nic/hal/src/nw/interface.hpp"
#include "nic/hal/src/nw/endpoint.hpp"
#include "nic/hal/src/nw/session.hpp"
#include "nic/hal/src/nw/l2segment.hpp"
#include "nic/hal/src/nw/nw.hpp"
#include "nic/hal/plugins/cfg/nat/nat.hpp"
#include "nic/hal/src/firewall/nwsec_group.hpp"
#include "nic/gen/proto/hal/interface.pb.h"
#include "nic/gen/proto/hal/l2segment.pb.h"
#include "nic/gen/proto/hal/vrf.pb.h"
#include "nic/gen/proto/hal/endpoint.pb.h"
#include "nic/gen/proto/hal/nw.pb.h"
#include "nic/gen/proto/hal/nwsec.pb.h"
#include "nic/gen/proto/hal/nat.pb.h"
#include "nic/p4/iris/include/defines.h"

using namespace hal::plugins::sfw;

uint32_t fte_base_test::vrf_id_ = 0;
uint32_t fte_base_test::l2seg_id_ = 0;
uint32_t fte_base_test::intf_id_ = 0;
uint32_t fte_base_test::nwsec_id_ = 0;
uint32_t fte_base_test::nh_id_ = 0;
uint32_t fte_base_test::pool_id_ = 0;
fte::ctx_t fte_base_test::ctx_ = {};
bool  fte_base_test::ipc_logging_disable_ = false;
std::vector<dev_handle_t> fte_base_test::handles;
std::map<hal_handle_t, ep_info_t> fte_base_test::eps;

hal_handle_t fte_base_test::add_vrf()
{
    hal_ret_t ret;
    vrf::VrfSpec  spec;
    vrf::VrfResponse resp;

    spec.mutable_key_or_handle()->set_vrf_id(++fte_base_test::vrf_id_);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::vrf_create(spec, &resp);
    hal::hal_cfg_db_close();

    EXPECT_EQ(ret, HAL_RET_OK);

    return resp.mutable_vrf_status()->vrf_handle();
}

hal_handle_t fte_base_test::add_network(hal_handle_t vrfh, uint32_t v4_addr, uint8_t prefix_len, uint64_t rmac)
{
    hal_ret_t ret;
    nw::NetworkSpec spec;
    nw::NetworkResponse resp;

    hal::vrf_t *vrf = hal::vrf_lookup_by_handle(vrfh);
    EXPECT_NE(vrf, nullptr);

    spec.set_rmac(rmac);
    spec.mutable_key_or_handle()->mutable_nw_key()->mutable_vrf_key_handle()->set_vrf_id(vrf->vrf_id);
    spec.mutable_key_or_handle()->mutable_nw_key()->mutable_ip_prefix()->mutable_address()->set_ip_af(types::IP_AF_INET);
    spec.mutable_key_or_handle()->mutable_nw_key()->mutable_ip_prefix()->mutable_address()->set_v4_addr(v4_addr);
    spec.mutable_key_or_handle()->mutable_nw_key()->mutable_ip_prefix()->set_prefix_len(prefix_len);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::network_create(spec, &resp);
    hal::hal_cfg_db_close();

    EXPECT_EQ(ret, HAL_RET_OK);

    return resp.mutable_status()->nw_handle();
}

hal_handle_t fte_base_test::add_l2segment(hal_handle_t nwh, uint16_t vlan_id)
{
    hal_ret_t ret;
    l2segment::L2SegmentSpec spec;
    l2segment::L2SegmentResponse resp;

    hal::network_t *nw = hal::find_network_by_handle(nwh);
    EXPECT_NE(nw, nullptr);

    spec.mutable_vrf_key_handle()->set_vrf_id(nw->nw_key.vrf_id);
    spec.add_network_key_handle()->set_nw_handle(nwh);
    spec.mutable_key_or_handle()->set_segment_id(++l2seg_id_);
    spec.mutable_wire_encap()->set_encap_type(types::ENCAP_TYPE_DOT1Q);
    spec.mutable_wire_encap()->set_encap_value(vlan_id);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::l2segment_create(spec, &resp);
    hal::hal_cfg_db_close();
    EXPECT_EQ(ret, HAL_RET_OK);

    return resp.mutable_l2segment_status()->l2segment_handle();
}

hal_handle_t fte_base_test::add_uplink(uint8_t port_num)
{
    hal_ret_t ret;
    intf::InterfaceSpec spec;
    intf::InterfaceResponse resp;

    // Create an uplink
    spec.set_type(intf::IF_TYPE_UPLINK);
    spec.mutable_key_or_handle()->set_interface_id(++intf_id_);
    spec.mutable_if_uplink_info()->set_port_num(port_num);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::interface_create(spec, &resp);
    hal::hal_cfg_db_close();
    EXPECT_EQ(ret, HAL_RET_OK);

    return resp.mutable_status()->if_handle();
}

hal_handle_t fte_base_test::add_endpoint(hal_handle_t l2segh, hal_handle_t intfh,
                                         uint32_t ip, uint64_t mac, uint16_t useg_vlan,
                                         bool enable_e2e)
{
    hal_ret_t ret;
    endpoint::EndpointSpec spec;
    endpoint::EndpointResponse resp;
    ep_info_t  ep;

    hal::l2seg_t *l2seg = hal::l2seg_lookup_by_handle(l2segh);
    EXPECT_NE(l2seg, nullptr);

    hal::vrf_t *vrf = hal::vrf_lookup_by_handle(l2seg->vrf_handle);
    EXPECT_NE(vrf, nullptr);


    spec.mutable_vrf_key_handle()->set_vrf_id(vrf->vrf_id);
    spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()
        ->mutable_l2segment_key_handle()->set_l2segment_handle(l2segh);
    spec.mutable_endpoint_attrs()->mutable_interface_key_handle()->set_if_handle(intfh);
    spec.mutable_key_or_handle()->mutable_endpoint_key()->mutable_l2_key()->set_mac_address(mac);
    auto addr = spec.mutable_endpoint_attrs()->add_ip_address();
    addr->set_ip_af(types::IP_AF_INET);
    addr->set_v4_addr(ip);
    spec.mutable_endpoint_attrs()->set_useg_vlan(useg_vlan);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::endpoint_create(spec, &resp);
    hal::hal_cfg_db_close();
    EXPECT_EQ(ret, HAL_RET_OK);
    if (enable_e2e) {
        ep.ip = ip;
        ep.mac = mac;
        if (l2seg->wire_encap.type == types::ENCAP_TYPE_DOT1Q)
            ep.vlan = l2seg->wire_encap.val;
        else
            ep.vlan = 0;
        eps.insert(std::pair<hal_handle_t, ep_info_t>(resp.endpoint_status().endpoint_handle(),ep));
    }

    return resp.endpoint_status().endpoint_handle();
}

hal_handle_t fte_base_test::add_nwsec_policy(hal_handle_t vrfh, std::vector<v4_rule_t> &rules)
{
    hal_ret_t ret;
    nwsec::SecurityPolicySpec                      spec;
    nwsec::SecurityPolicyResponse                  resp;

    hal::vrf_t *vrf = hal::vrf_lookup_by_handle(vrfh);
    EXPECT_NE(vrf, nullptr);

    spec.mutable_policy_key_or_handle()->mutable_security_policy_key()->
        set_security_policy_id(++nwsec_id_);
    spec.mutable_policy_key_or_handle()->mutable_security_policy_key()->
        mutable_vrf_id_or_handle()->set_vrf_id(vrf->vrf_id);

    uint32_t rule_id = 0;
    for (auto &rule: rules) {
        nwsec::SecurityRule *rule_spec = spec.add_rule();

        rule_spec->set_rule_id(++rule_id);
        rule_spec->mutable_action()->set_sec_action(rule.action);
        types::RuleMatch *match = rule_spec->mutable_match();

        if (rule.to.addr) {
            types::IPPrefix *prefix = match->add_dst_address()->
                mutable_address()->mutable_prefix()->mutable_ipv4_subnet();

            prefix->mutable_address()->set_ip_af(types::IPAddressFamily::IP_AF_INET);
            prefix->mutable_address()->set_v4_addr(rule.to.addr);
            prefix->set_prefix_len(rule.to.plen ?: 32);
        }

        if (rule.from.addr) {
            types::IPPrefix *prefix = match->add_src_address()->
                mutable_address()->mutable_prefix()->mutable_ipv4_subnet();
            prefix->mutable_address()->set_ip_af(types::IPAddressFamily::IP_AF_INET);
            prefix->mutable_address()->set_v4_addr(rule.from.addr);
            prefix->set_prefix_len(rule.from.plen ?: 32);
        }

        types::L4PortRange *port_range = match->add_app_match()->mutable_port_info()->add_dst_port_range();
        port_range->set_port_low(rule.app.dport_low);
        port_range->set_port_high(rule.app.dport_high);

        if (rule.app.proto) {
            match->set_protocol((types::IPProtocol)rule.app.proto);
        }

        if (rule.app.alg) {
            nwsec::AppData *app_data = rule_spec->mutable_action()->add_app_data();
            app_data->set_alg(rule.app.alg);
        }
    }

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::securitypolicy_create(spec, &resp);
    hal::hal_cfg_db_close();
    EXPECT_EQ(ret, HAL_RET_OK);

    return resp.policy_status().security_policy_handle();
}

hal_handle_t fte_base_test::add_route(hal_handle_t vrfh,
                                      uint32_t v4_addr, uint8_t prefix_len,
                                      hal_handle_t eph)
{
    hal_ret_t                          ret;
    nw::RouteSpec                   route_spec;
    nw::RouteResponse               route_rsp;
    nw::NexthopSpec                 nh_spec;
    nw::NexthopResponse             nh_rsp;

     // Create a nexthop with EP
    nh_spec.mutable_key_or_handle()->set_nexthop_id(++nh_id_);
    nh_spec.mutable_ep_key_or_handle()->set_endpoint_handle(eph);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::nexthop_create(nh_spec, &nh_rsp);
    hal::hal_cfg_db_close();
    EXPECT_EQ(ret, HAL_RET_OK);

    hal_handle_t nhh = nh_rsp.mutable_status()->nexthop_handle();

    // Create a route
    route_spec.mutable_key_or_handle()->mutable_route_key()->mutable_vrf_key_handle()->set_vrf_handle(vrfh);
    route_spec.mutable_key_or_handle()->mutable_route_key()->mutable_ip_prefix()->set_prefix_len(prefix_len);
    route_spec.mutable_key_or_handle()->mutable_route_key()->mutable_ip_prefix()->mutable_address()->set_ip_af(types::IP_AF_INET);
    route_spec.mutable_key_or_handle()->mutable_route_key()->mutable_ip_prefix()->mutable_address()->set_v4_addr(v4_addr);
    route_spec.mutable_nh_key_or_handle()->set_nexthop_handle(nhh);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::route_create(route_spec, &route_rsp);
    hal::hal_cfg_db_close();
    EXPECT_EQ(ret, HAL_RET_OK);

    return route_rsp.mutable_status()->route_handle();
}

hal_handle_t fte_base_test::add_nat_pool(hal_handle_t vrfh, uint32_t v4_addr, uint8_t prefix_len)
{
    hal_ret_t ret;
    nat::NatPoolSpec spec;
    nat::NatPoolResponse resp;

    spec.mutable_key_or_handle()->mutable_pool_key()->mutable_vrf_kh()->set_vrf_handle(vrfh);
    spec.mutable_key_or_handle()->mutable_pool_key()->set_pool_id(++pool_id_);
    types::IPPrefix *prefix = spec.add_address()->mutable_prefix()->mutable_ipv4_subnet();
    prefix->set_prefix_len(prefix_len);
    prefix->mutable_address()->set_ip_af(types::IP_AF_INET);
    prefix->mutable_address()->set_v4_addr(v4_addr);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::nat_pool_create(spec, &resp);
    hal::hal_cfg_db_close();
    EXPECT_EQ(ret, HAL_RET_OK);

    return resp.mutable_pool_status()->pool_handle();
}

hal_handle_t fte_base_test::add_nat_mapping(hal_handle_t vrfh, uint32_t v4_addr,
                                            hal_handle_t poolh, uint32_t *mapped_ip)
{
    hal_ret_t ret;
    nat::NatMappingSpec spec;
    nat::NatMappingResponse resp;

    spec.mutable_key_or_handle()->mutable_svc()->mutable_vrf_kh()->set_vrf_handle(vrfh);
    spec.mutable_key_or_handle()->mutable_svc()->mutable_ip_addr()->set_ip_af(types::IP_AF_INET);
    spec.mutable_key_or_handle()->mutable_svc()->mutable_ip_addr()->set_v4_addr(v4_addr);
    spec.mutable_nat_pool()->set_pool_handle(poolh);
    spec.set_bidir(true);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::nat_mapping_create(spec, &resp);
    hal::hal_cfg_db_close();
    EXPECT_EQ(ret, HAL_RET_OK);

    if (mapped_ip) {
        *mapped_ip = resp.mutable_status()->mapped_ip().v4_addr();
    }

    return resp.mutable_status()->handle();
}

hal_ret_t fte_base_test::inject_pkt(fte::cpu_rxhdr_t *cpu_rxhdr,
                                    std::vector<uint8_t *> &pkts, size_t pkt_len)
{
    struct fn_ctx_t {
        fte::ctx_t *ctx;
        fte::cpu_rxhdr_t *cpu_rxhdr;
        std::vector<uint8_t *> &pkts;
        size_t pkt_len;
        hal_ret_t ret;
    } fn_ctx = { &ctx_, cpu_rxhdr, pkts, pkt_len, HAL_RET_OK };

    fte::fte_execute(FTE_ID, [](void *data) {
        fn_ctx_t *fn_ctx = (fn_ctx_t *)data;
        auto pkt = fn_ctx->pkts[0];
        fte::ctx_t *ctx = fn_ctx->ctx;

        fte::flow_t iflow[fte::ctx_t::MAX_STAGES], rflow[fte::ctx_t::MAX_STAGES];
        uint16_t num_features;
        size_t fstate_size = fte::feature_state_size(&num_features);
        fte::feature_state_t *feature_state = (fte::feature_state_t*)HAL_MALLOC(hal::HAL_MEM_ALLOC_FTE, fstate_size);

        for (int i=0; i<fte::ctx_t::MAX_STAGES; i++) {
            iflow[i]  =  {};
            rflow[i]  =  {};
        }

        for (uint32_t i=0; i<fn_ctx->pkts.size(); i++) {
            hal::hal_cfg_db_open(hal::CFG_OP_READ);
            fn_ctx->ret = ctx->init(fn_ctx->cpu_rxhdr, pkt, fn_ctx->pkt_len,
                                    iflow, rflow, feature_state, num_features);
            if (fn_ctx->ret == HAL_RET_OK) {
                fn_ctx->ret = ctx->process();
            }
            HAL_ASSERT(fn_ctx->ret == HAL_RET_OK);
            hal::hal_cfg_db_close();
        }
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, feature_state);
    }, &fn_ctx );

    return HAL_RET_OK;
}

hal_ret_t fte_base_test::inject_pkt(fte::cpu_rxhdr_t *cpu_rxhdr,
                                    uint8_t *pkt, size_t pkt_len)
{
    std::vector<uint8_t *> pkts = { pkt };

    return inject_pkt(cpu_rxhdr, pkts, pkt_len);
}

static inline ip_addr_t ep_ip(hal::ep_t *ep) {
    return dllist_entry(ep->ip_list_head.next, hal::ep_ip_entry_t, ep_ip_lentry)->ip_addr;
}

hal_ret_t
fte_base_test::inject_eth_pkt(const fte::lifqid_t &lifq,
                              hal_handle_t src_ifh, hal_handle_t src_l2segh,
                              std::vector<Tins::EthernetII> &pkts)
{
    hal::if_t *sif = hal::find_if_by_handle(src_ifh);
    hal::l2seg_t *l2seg = hal::l2seg_lookup_by_handle(src_l2segh);
    EXPECT_NE(l2seg, nullptr);

    // use first pkt to build cpu header
    Tins::EthernetII eth = pkts[0];

    uint8_t vlan_valid;
    uint16_t vlan_id;
    hal::if_l2seg_get_encap(sif, l2seg, &vlan_valid, &vlan_id);

    hal::pd::pd_l2seg_get_flow_lkupid_args_t args;
    hal::pd::pd_func_args_t pd_func_args = {0};
    args.l2seg = l2seg;
    pd_func_args.pd_l2seg_get_flow_lkupid = &args;
    hal::pd::hal_pd_call(hal::pd::PD_FUNC_ID_L2SEG_GET_FLOW_LKPID, &pd_func_args);
    hal::pd::l2seg_hw_id_t hwid = args.hwid;

    fte::cpu_rxhdr_t cpu_rxhdr = {};
    cpu_rxhdr.src_lif = sif->if_id;
    cpu_rxhdr.lif = lifq.lif;
    cpu_rxhdr.qtype = lifq.qtype;
    cpu_rxhdr.qid = lifq.qid;
    cpu_rxhdr.lkp_vrf = hwid;
    cpu_rxhdr.lkp_dir = sif->if_type == intf::IF_TYPE_ENIC ? FLOW_DIR_FROM_DMA :
        FLOW_DIR_FROM_UPLINK;
    cpu_rxhdr.lkp_inst = 0;
    cpu_rxhdr.lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV4;
    cpu_rxhdr.l2_offset = 0;

    cpu_rxhdr.l3_offset = eth.header_size() + eth.find_pdu<Tins::Dot1Q>()->header_size();
    cpu_rxhdr.l4_offset = cpu_rxhdr.l3_offset + eth.find_pdu<Tins::IP>()->header_size();
    cpu_rxhdr.payload_offset = cpu_rxhdr.l4_offset;

    auto l4pdu = eth.find_pdu<Tins::IP>()->inner_pdu();
    if (l4pdu) {
        cpu_rxhdr.payload_offset +=  l4pdu->header_size();
    }

    cpu_rxhdr.flags = 0;

    std::vector<uint8_t *>buffs;
    size_t buff_size;

    std::vector<uint8_t> buffer = pkts[0].serialize();
    buff_size = buffer.size();
    for (auto pkt: pkts) {
        //uint8_t *buffer = (uint8_t *)malloc(buff_size);
        //vector<uint8_t> raw = pkt.serialize();
        //memcpy(buffer, &raw[0], buff_size);
        buffs.push_back(&buffer[0]);
    }

    return inject_pkt(&cpu_rxhdr, buffs, buff_size);
}

hal_ret_t
fte_base_test::inject_eth_pkt(const fte::lifqid_t &lifq,
                              hal_handle_t src_ifh, hal_handle_t src_l2segh,
                              Tins::EthernetII &eth)
{
    ::vector<Tins::EthernetII> pkts = {eth};
    return inject_eth_pkt(lifq, src_ifh, src_l2segh, pkts);
}


hal_ret_t
fte_base_test::inject_ipv4_pkt(const fte::lifqid_t &lifq,
                               hal_handle_t deph, hal_handle_t seph,
                               Tins::PDU &l4pdu)
{
    hal::ep_t *sep = hal::find_ep_by_handle(seph);
    EXPECT_NE(sep, nullptr);
    ip_addr_t sip =  ep_ip(sep);
    hal::if_t *sif = hal::find_if_by_handle(sep->if_handle);

    hal::ep_t *dep = hal::find_ep_by_handle(deph);
    EXPECT_NE(dep, nullptr);
    ip_addr_t dip =  ep_ip(dep);

    hal::l2seg_t *l2seg = hal::l2seg_lookup_by_handle(sep->l2seg_handle);
    EXPECT_NE(l2seg, nullptr);
    uint8_t vlan_valid;
    uint16_t vlan_id;
    hal::if_l2seg_get_encap(sif, l2seg, &vlan_valid, &vlan_id);

    Tins::EthernetII eth = Tins::EthernetII(dep->l2_key.mac_addr, sep->l2_key.mac_addr) /
        Tins::Dot1Q(vlan_id) /
        Tins::IP(Tins::IPv4Address(htonl(dip.addr.v4_addr)),
                 Tins::IPv4Address(htonl(sip.addr.v4_addr))) /
        l4pdu;

    return inject_eth_pkt(lifq, sep->if_handle, sep->l2seg_handle, eth);
}

static
int parse_v4_packet (uint8_t *pkt, int len, uint16_t *l3_offset,
                     uint16_t *l4_offset, uint16_t *payload_offset, uint8_t *proto)
{
    ether_header_t *eth;
    vlan_header_t *vlan;
    ipv4_header_t *ip;
    tcp_header_t  *tcp;
    uint16_t etype;
    eth = (ether_header_t *)pkt;
    if (ntohs(eth->etype) == ETHERTYPE_VLAN) {
        vlan = (vlan_header_t*)pkt;
        etype = ntohs(vlan->etype);
        ip = (ipv4_header_t *)(vlan+1);
        *l3_offset = sizeof(vlan_header_t);
    } else {
        etype = ntohs(eth->etype);
        ip = (ipv4_header_t *)(eth+1);
        *l3_offset = sizeof(ether_header_t);
    }

    // TBD: parse options
    if (etype == ETHERTYPE_IP) {
        *l4_offset = *l3_offset + sizeof(ipv4_header_t);
        tcp = (tcp_header_t *)(ip+1);
        if (ip->protocol == IPPROTO_TCP) {
            *payload_offset = *l4_offset + (tcp->doff*4);
            *proto = IPPROTO_TCP;
        } else if (ip->protocol == IPPROTO_UDP) {
            *payload_offset = *l4_offset + sizeof(udp_header_t);
            *proto = IPPROTO_UDP;
        } else {
            return 1;
        }
    } else {
        return 1;
    }

    return 0;
}

static
void fix_checksum(uint8_t *pkt, int len)
{
    ether_header_t *eth;
    vlan_header_t *vlan;
    ipv4_header_t *ip;
    tcp_header_t  *tcp;
    udp_header_t  *udp;
    uint16_t etype;
    int offset=0;

    eth = (ether_header_t *)pkt;
    if (ntohs(eth->etype) == ETHERTYPE_VLAN) {
        vlan = (vlan_header_t*)pkt;
        etype = ntohs(vlan->etype);
        ip = (ipv4_header_t *)(vlan+1);
        offset += sizeof(vlan_header_t);
    } else {
        etype = ntohs(eth->etype);
        ip = (ipv4_header_t *)(eth+1);
        offset += sizeof(ether_header_t);
    }

    // TBD: parse options
    if (etype == ETHERTYPE_IP) {
        ip->check = checksum((short unsigned int*)ip, sizeof(ipv4_header_t));
        offset += sizeof(ipv4_header_t);
        if (ip->protocol == IPPROTO_TCP) {
            tcp = (tcp_header_t*)(ip+1);
            tcp->check = get_tcp_checksum((void *)tcp, (len-offset), ip->saddr, ip->daddr);
        } else if (ip->protocol == IPPROTO_UDP) {
            udp = (udp_header_t*)(ip+1);
            udp->check = get_udp_checksum((void *)udp, (len-offset), ip->saddr, ip->daddr);
        }
    }
}

void
fte_base_test::process_e2e_packets (void)
{
    int       maxfd, ret;
    uint16_t  nread, nwrite;
    char      inpktbuf[PKTBUF_LEN];
    char      *inp = inpktbuf;
    char      outpktbuf[PKTBUF_LEN];
    char      *outp = outpktbuf;
    uint16_t  l3_offset=0, l4_offset=0, payload_offset=0;
    fte::lifqid_t lifq = fte::FLOW_MISS_LIFQ;
    hal_ret_t rc;
    uint8_t   proto = 0;
    int       pkt_len = 0;

    while (1) {
        fd_set rd_set;
        FD_ZERO(&rd_set);
        maxfd = -1;
        for (uint32_t i = 0 ; i < handles.size(); i++) {
           FD_SET(handles[i].fd, &rd_set);
           if (handles[i].fd > maxfd) {
               maxfd = handles[i].fd;
           }
        }

        ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
        if (ret < 0 && errno == EINTR){
            continue;
        }

        for (uint32_t i = 0 ; i < handles.size(); i++) {
            if (FD_ISSET(handles[i].fd, &rd_set)) {
                dev_handle_t hdl = handles[i];
                if ((nread = read(hdl.fd, inp, PKTBUF_LEN)) < 0) {
                    continue;
                }

                /*---------- FTE PROCESSING ---------*/
                /*
                 * Get the EP information to fill the cpu header
                 * and parse the packet to get the offsets
                 */
                hal::ep_t *sep = hal::find_ep_by_handle(hdl.ep);
                EXPECT_NE(sep, nullptr);
                hal::if_t *sif = hal::find_if_by_handle(sep->if_handle);
                EXPECT_NE(sif, nullptr);

                hal::l2seg_t *l2seg = hal::l2seg_lookup_by_handle(sep->l2seg_handle);
                EXPECT_NE(l2seg, nullptr);
                hal::pd::pd_l2seg_get_flow_lkupid_args_t args;
                hal::pd::pd_func_args_t          pd_func_args = {0};
                args.l2seg = l2seg;
                pd_func_args.pd_l2seg_get_flow_lkupid = &args;
                hal::pd::hal_pd_call(hal::pd::PD_FUNC_ID_L2SEG_GET_FLOW_LKPID, &pd_func_args);
                hal::pd::l2seg_hw_id_t hwid = args.hwid;

                if (!parse_v4_packet((uint8_t *)inp, nread, &l3_offset, &l4_offset, &payload_offset, &proto)) {
                    fte::cpu_rxhdr_t cpu_rxhdr = {};
                    cpu_rxhdr.src_lif = sif->if_id;
                    cpu_rxhdr.lif = lifq.lif;
                    cpu_rxhdr.qtype = lifq.qtype;
                    cpu_rxhdr.qid = lifq.qid;
                    cpu_rxhdr.lkp_vrf = hwid;
                    cpu_rxhdr.lkp_dir = sif->if_type == intf::IF_TYPE_ENIC ? FLOW_DIR_FROM_DMA :
                                                     FLOW_DIR_FROM_UPLINK;
                    cpu_rxhdr.lkp_inst = 0;
                    cpu_rxhdr.lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV4;
                    cpu_rxhdr.l2_offset = 0;
                    cpu_rxhdr.l3_offset = l3_offset;
                    cpu_rxhdr.l4_offset = l4_offset;
                    cpu_rxhdr.payload_offset = payload_offset;
                    cpu_rxhdr.flags = 0;
                    rc = inject_pkt(&cpu_rxhdr, (uint8_t *)inp, nread);
                    EXPECT_EQ(rc, HAL_RET_OK);
                    EXPECT_FALSE(ctx_.drop());
#ifdef DEBUG
                    printf("Received packet:");
                    for (int i = 0; i< nread; i++) {
                        if (i % 16 == 0) {
                            printf("\n");
                        }
                        printf(" 0x%02x", (unsigned char)inp[i]);
                    }
                    printf("\n");
#endif

                    fte::flow_t *iflow = ctx_.flow(hal::FLOW_ROLE_INITIATOR);
                    if (proto == IPPROTO_TCP) {
                        pkt_len = net_sfw_build_tcp_pkt(ctx_, (uint8_t *)outp, PKTBUF_LEN, iflow->header_rewrite_info(),
                                                        iflow->header_push_info());
                    } else {
                        pkt_len = net_sfw_build_udp_pkt(ctx_, (uint8_t *)outp, PKTBUF_LEN, iflow->header_rewrite_info(),
                                                        iflow->header_push_info());
                    }
                }

                fix_checksum((uint8_t *)outp, pkt_len);
#ifdef DEBUG 
                printf("Received packet:");
                for (int i = 0; i< pkt_len; i++) {
                    if (i % 16 == 0) {
                        printf("\n");
                    }
                    printf(" 0x%02x", (unsigned char)outp[i]);
                }
                printf("\n");
#endif
                nwrite = write(handles[hdl.other_hdl].fd, outp, pkt_len);
                if (nwrite < 0) {
                    continue;
                }
            }
        }
    }
}

void
fte_base_test::run_service(hal_handle_t ep_h, std::string service)
{
    int idx = std::distance(eps.begin(), eps.find(ep_h));
    std::string prefix_cmd = "ip netns exec EP" + to_string(idx+1);

    std::string cmd = prefix_cmd + " " + service;
    std::system(cmd.c_str());
}

std::string 
fte_base_test::prefix_cmd(hal_handle_t ep_h)
{
    int idx = std::distance(eps.begin(), eps.find(ep_h));
    std::string prefix_cmd = "ip netns exec EP" + to_string(idx+1);

    return prefix_cmd;
}
