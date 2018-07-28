//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/include/base.hpp"
#include "nic/hal/hal.hpp"
#include "nic/include/hal_state.hpp"
#include "nic/hal/plugins/cfg/nw/interface.hpp"
#include "nic/hal/plugins/cfg/nw/endpoint.hpp"
#include "nic/include/endpoint_api.hpp"
#include "nic/hal/plugins/cfg/nw/session.hpp"
#include "nic/hal/src/utils/utils.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/include/interface_api.hpp"
#include "nic/hal/plugins/cfg/aclqos/qos.hpp"
#include "sdk/timestamp.hpp"
#include "nic/include/fte.hpp"
#include "nic/hal/plugins/cfg/firewall/nwsec_group.hpp"

using telemetry::MirrorSessionSpec;
using session::FlowInfo;
using session::FlowKeyTcpUdpInfo;
using session::FlowKeyICMPInfo;
using session::FlowData;
using session::ConnTrackInfo;
using session::FlowStats;

using namespace sdk::lib;

#define TCP_IPV4_PKT_SZ (sizeof(ether_header_t)+sizeof(ipv4_header_t)+sizeof(tcp_header_t))

namespace hal {

thread_local void *g_session_timer;

#define SESSION_SW_DEFAULT_TIMEOUT                 (3600)
#define SESSION_SW_DEFAULT_TCP_HALF_CLOSED_TIMEOUT (120 * TIME_MSECS_PER_SEC)
#define SESSION_SW_DEFAULT_TCP_CLOSE_TIMEOUT       (15 * TIME_MSECS_PER_SEC)
#define SESSION_SW_DEFAULT_TCP_CXNSETUP_TIMEOUT    (15 * TIME_MSECS_PER_SEC)
#define SESSION_DEFAULT_TCP_TICKLE_TIMEOUT         (2 * TIME_MSECS_PER_SEC)
#define HAL_SESSION_AGE_SCAN_INTVL                 (5 * TIME_MSECS_PER_SEC)
#define HAL_SESSION_BUCKETS_TO_SCAN_PER_INTVL       4
#define HAL_TCP_CLOSE_WAIT_INTVL                   (10 * TIME_MSECS_PER_SEC)
#define MAX_TCP_TICKLES                             3
#define HAL_MAX_SESSION_PER_ENQ                     5

void *
session_get_handle_key_func (void *entry)
{
    HAL_ASSERT(entry != NULL);
    return (void *)&(((session_t *)entry)->hal_handle);
}

uint32_t
session_compute_handle_hash_func (void *key, uint32_t ht_size)
{
    return hash_algo::fnv_hash(key, sizeof(hal_handle_t)) % ht_size;
}

bool
session_compare_handle_key_func (void *key1, void *key2)
{
    HAL_ASSERT((key1 != NULL) && (key2 != NULL));
    if (*(hal_handle_t *)key1 == *(hal_handle_t *)key2) {
        return true;
    }
    return false;
}

void *
session_get_iflow_key_func (void *entry)
{
    HAL_ASSERT(entry != NULL);
    return (void *)&(((session_t *)entry)->iflow->config.key);
}

uint32_t
session_compute_iflow_hash_func (void *key, uint32_t ht_size)
{
    return (hash_algo::fnv_hash(key, sizeof(flow_key_t)) % ht_size);
}

bool
session_compare_iflow_key_func (void *key1, void *key2)
{
    HAL_ASSERT((key1 != NULL) && (key2 != NULL));
    if (!memcmp(key1, key2, sizeof(flow_key_t))) {
        return true;
    }
    return false;
}

void *
session_get_rflow_key_func (void *entry)
{
    HAL_ASSERT(entry != NULL);
    return (void *)&(((session_t *)entry)->rflow->config.key);
}

uint32_t
session_compute_rflow_hash_func (void *key, uint32_t ht_size)
{
    return (hash_algo::fnv_hash(key, sizeof(flow_key_t)) % ht_size);
}

bool
session_compare_rflow_key_func (void *key1, void *key2)
{
    HAL_ASSERT((key1 != NULL) && (key2 != NULL));
    if (!memcmp(key1, key2, sizeof(flow_key_t))) {
        return true;
    }
    return false;
}

void *
flow_get_key_func (void *entry)
{
    HAL_ASSERT(entry != NULL);
    return (void *)&(((flow_t *)entry)->config.key);
}

uint32_t
flow_compute_hash_func (void *key, uint32_t ht_size)
{
    return hash_algo::fnv_hash(key, sizeof(flow_key_t)) % ht_size;
}

bool
flow_compare_key_func (void *key1, void *key2)
{
    HAL_ASSERT((key1 != NULL) && (key2 != NULL));
    if (!memcmp(key1, key2, sizeof(flow_key_t))) {
        return true;
    }
    return false;
}

session_t *
find_session_by_handle (hal_handle_t handle)
{
    return (session_t *)g_hal_state->session_hal_handle_ht()->lookup(&handle);
}

//------------------------------------------------------------------------------
// thread safe helper to stringify flow_key_t
//------------------------------------------------------------------------------
const char *
flowkey2str (const flow_key_t& key)
{
    static thread_local char       key_str[4][400];
    static thread_local uint8_t    key_str_next = 0;
    char                           *buf;

    buf = key_str[key_str_next++ & 0x3];
    fmt::ArrayWriter out(buf, 400);

    out.write("{{dir={}, ", key.dir);

    if ( key.svrf_id == key.dvrf_id) {
        out.write("svrf={}, ", key.svrf_id);
    } else {
        out.write("svrf={}, dvrf={} ", key.svrf_id, key.dvrf_id);
    }

    switch (key.flow_type) {
    case FLOW_TYPE_L2:
        out.write("l2seg={}, smac={}, dmac={} etype={}",
                  key.l2seg_id, macaddr2str(key.smac),
                  macaddr2str(key.dmac), key.ether_type);
        break;

    case FLOW_TYPE_V4:
    case FLOW_TYPE_V6:
        if (key.flow_type == FLOW_TYPE_V4) {
            out.write("sip={}, dip={}, ", ipv4addr2str(key.sip.v4_addr),
                      ipv4addr2str(key.dip.v4_addr));
        } else {
            out.write("sip={}, dip={}, ", key.sip.v6_addr, key.dip.v6_addr);
        }

        switch (key.proto) {
        case types::IPPROTO_ICMP:
        case types::IPPROTO_ICMPV6:
            out.write("proto=icmp, type={}, code={}, id={}",
                      key.icmp_type, key.icmp_code, key.icmp_id);
            break;
        case types::IPPROTO_ESP:
            out.write("proto=esp, spi={}", key.spi);
            break;
        case types::IPPROTO_TCP:
            out.write("proto=tcp, sport={}, dport={}", key.sport, key.dport);
            break;
        case types::IPPROTO_UDP:
            out.write("proto=udp, sport={}, dport={}", key.sport, key.dport);
            break;
        default:
            out.write("proto={}", key.proto);
        }
        break;

    default:
        out.write("flow-type=unknown(%d)", key.flow_type);
    }

    out.write("}}");

    buf[out.size()] = '\0';

    return buf;
}

// extract mirror sessions specified in spec into ingress and egress bitmaps
//------------------------------------------------------------------------------
static hal_ret_t
extract_mirror_sessions (const FlowSpec& spec, uint8_t *ingress, uint8_t *egress)
{
    int i;
    *ingress = 0;
    *egress = 0;
    FlowInfo flinfo = spec.flow_data().flow_info();

    for (i = 0; i < flinfo.ing_mirror_sessions_size(); ++i) {
        uint32_t id = flinfo.ing_mirror_sessions(i).mirrorsession_id();
        if (id > 7) {
            return HAL_RET_INVALID_ARG;
        }
        *ingress = *ingress | (1 << id);
        HAL_TRACE_DEBUG("  Adding ingress session {}", id);
    }
    for (i = 0; i < flinfo.egr_mirror_sessions_size(); ++i) {
        uint32_t id = flinfo.egr_mirror_sessions(i).mirrorsession_id();
        if (id > 7) {
            return HAL_RET_INVALID_ARG;
        }
        *egress = *egress | (1 << id);
        HAL_TRACE_DEBUG("  Adding egress session {}", id);
    }
    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// release all the resources and allocated ids etc. associated with a session
// TODO: if session is added to DBs, remove from them as well !!
//------------------------------------------------------------------------------
hal_ret_t
session_cleanup (session_t *session)
{
    if (session->iflow) {
        hal::delay_delete_to_slab(HAL_SLAB_FLOW, session->iflow);
    }
    if (session->rflow) {
        hal::delay_delete_to_slab(HAL_SLAB_FLOW, session->rflow);
    }
    hal::delay_delete_to_slab(HAL_SLAB_SESSION, session);

    return HAL_RET_OK;
}

hal_ret_t
extract_flow_key_from_spec (vrf_id_t tid, flow_key_t *key,
                            const FlowKey& flow_spec_key)
{
    key->svrf_id = key->dvrf_id = tid;

    if (flow_spec_key.has_l2_key()) {
        key->flow_type = hal::FLOW_TYPE_L2;
        key->l2seg_id = flow_spec_key.l2_key().l2_segment_id();
        key->ether_type = flow_spec_key.l2_key().ether_type();
        MAC_UINT64_TO_ADDR(key->smac, flow_spec_key.l2_key().smac());
        MAC_UINT64_TO_ADDR(key->dmac, flow_spec_key.l2_key().dmac());
    } else if (flow_spec_key.has_v4_key()) {
        key->flow_type = hal::FLOW_TYPE_V4;
            key->sip.v4_addr = flow_spec_key.v4_key().sip();
        key->dip.v4_addr = flow_spec_key.v4_key().dip();
        key->proto = flow_spec_key.v4_key().ip_proto();
        if ((key->proto == IP_PROTO_TCP) ||
            (key->proto == IP_PROTO_UDP)) {
            key->sport = flow_spec_key.v4_key().tcp_udp().sport();
            key->dport = flow_spec_key.v4_key().tcp_udp().dport();
        } else if (key->proto == IP_PROTO_ICMP) {
            key->icmp_type = flow_spec_key.v4_key().icmp().type();
            key->icmp_code = flow_spec_key.v4_key().icmp().code();
            if ((key->icmp_type == 0) || (key->icmp_type == 8)) {
                /* ICMP id is valid only for echo req & rep */
                key->icmp_id = flow_spec_key.v4_key().icmp().id();
            } else {
                key->icmp_id = 0;
            }
        } else if (key->proto == IPPROTO_ESP) {
            key->spi = flow_spec_key.v4_key().esp().spi();
        } else {
            key->sport = key->dport = 0;
        }
    } else if (flow_spec_key.has_v6_key()) {
        key->flow_type = hal::FLOW_TYPE_V6;
        memcpy(key->sip.v6_addr.addr8,
               flow_spec_key.v6_key().sip().v6_addr().c_str(),
               IP6_ADDR8_LEN);
        memcpy(key->dip.v6_addr.addr8,
               flow_spec_key.v6_key().dip().v6_addr().c_str(),
               IP6_ADDR8_LEN);
        key->proto = flow_spec_key.v6_key().ip_proto();
        if ((key->proto == IP_PROTO_TCP) ||
            (key->proto == IP_PROTO_UDP)) {
            key->sport = flow_spec_key.v6_key().tcp_udp().sport();
            key->dport = flow_spec_key.v6_key().tcp_udp().dport();;
        } else if (key->proto == IP_PROTO_ICMPV6) {
            key->icmp_type = flow_spec_key.v6_key().icmp().type();
            key->icmp_code = flow_spec_key.v6_key().icmp().code();
            key->icmp_id = flow_spec_key.v6_key().icmp().id();
            // only echo request and reply
            if(key->icmp_type < 128 || key->icmp_type > 136) {
                HAL_TRACE_DEBUG("fte: invalid icmp type {}", key->icmp_type);
                return HAL_RET_INVALID_ARG;
            }
        } else if (key->proto == IPPROTO_ESP) {
            key->spi = flow_spec_key.v6_key().esp().spi();
        } else {
            key->sport = key->dport = 0;
        }
    }

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// given a flow key, get the source and destination endpoint records
//------------------------------------------------------------------------------
hal_ret_t
ep_get_from_flow_key (const flow_key_t* key, ep_t **sep, ep_t **dep)
{
    *sep = *dep = NULL;

    switch  (key->flow_type) {
    case FLOW_TYPE_L2:
        *sep = find_ep_by_l2_key(key->l2seg_id, key->smac);
        *dep = find_ep_by_l2_key(key->l2seg_id, key->dmac);
        break;

    case FLOW_TYPE_V4:
    case FLOW_TYPE_V6:
        ep_l3_key_t l3key;
        l3key.ip_addr.af = key->flow_type == FLOW_TYPE_V4 ? IP_AF_IPV4 : IP_AF_IPV6;

        l3key.vrf_id = key->svrf_id;
        l3key.ip_addr.addr = key->sip;
        *sep = find_ep_by_l3_key(&l3key);

        l3key.vrf_id = key->dvrf_id;
        l3key.ip_addr.addr = key->dip;
        *dep = find_ep_by_l3_key(&l3key);
        break;
    }

    if (*sep == NULL || *dep == NULL) {
        return HAL_RET_EP_NOT_FOUND;
    }

    return HAL_RET_OK;
}


//------------------------------------------------------------------------------
// insert this session in all meta data structures
//------------------------------------------------------------------------------
static inline hal_ret_t
add_session_to_db (vrf_t *vrf, l2seg_t *l2seg_s, l2seg_t *l2seg_d,
                   ep_t *sep, ep_t *dep, if_t *sif, if_t *dif,
                   session_t *session)
{
    //session->session_id_ht_ctxt.reset();
    //g_hal_state->session_id_ht()->insert(session,
                                         //&session->session_id_ht_ctxt);
    session->hal_handle_ht_ctxt.reset();
    g_hal_state->session_hal_handle_ht()->insert(session,
                                                 &session->hal_handle_ht_ctxt);
    session->hal_iflow_ht_ctxt.reset();
    g_hal_state->session_hal_iflow_ht()->insert(session,
                                                &session->hal_iflow_ht_ctxt);
    if (session->rflow) {
        session->hal_rflow_ht_ctxt.reset();
        g_hal_state->session_hal_rflow_ht()->insert_with_key(
                         (void *)std::addressof(session->rflow->config.key),
                         session, &session->hal_rflow_ht_ctxt);
    }

    if (sep) {
        ep_add_session(sep, session);
    }
    if (dep) {
        ep_add_session(dep, session);
    }

    dllist_reset(&session->sif_session_lentry);
    if (sif) {
        HAL_SPINLOCK_LOCK(&sif->slock);
        // sdk::lib::dllist_add(&sif->session_list_head, &session->sif_session_lentry);
        HAL_SPINLOCK_UNLOCK(&sif->slock);
    }

    dllist_reset(&session->dif_session_lentry);
    if (dif && sif != dif) {
        HAL_SPINLOCK_LOCK(&dif->slock);
        // sdk::lib::dllist_add(&dif->session_list_head,
        //                  &session->dif_session_lentry);
        HAL_SPINLOCK_UNLOCK(&dif->slock);
    }

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// remove this session from all meta data structures
//------------------------------------------------------------------------------
static inline void
del_session_from_db (ep_t *sep, ep_t *dep, session_t *session)
{
    HAL_TRACE_DEBUG("Entering DEL session from DB:{}", session->hal_handle);
    g_hal_state->session_hal_handle_ht()->remove_entry(session,
                                                 &session->hal_handle_ht_ctxt);

    g_hal_state->session_hal_iflow_ht()->remove_entry(session,
                                                &session->hal_iflow_ht_ctxt);

    if (session->rflow) {
        g_hal_state->session_hal_rflow_ht()->remove_entry(session,
                                                    &session->hal_rflow_ht_ctxt);
    }

    if (sep) {
        ep_del_session(sep, session);
    }
    if (dep) {
        ep_del_session(dep, session);
    }
}


static void
flow_tcp_to_flow_tcp_spec(flow_t *flow, FlowKeyTcpUdpInfo *tcp_udp)
{
    tcp_udp->set_sport(flow->config.key.sport);
    tcp_udp->set_dport(flow->config.key.dport);
}

static void
flow_icmp_to_flow_icmp_spec(flow_t *flow, FlowKeyICMPInfo *icmp)
{
    icmp->set_type(flow->config.key.icmp_type);
    icmp->set_code(flow->config.key.icmp_code);
    icmp->set_id(flow->config.key.icmp_id);
}

static void
flow_data_to_flow_data_spec(flow_t *flow, FlowData *flow_data)
{
    FlowInfo *flow_info = flow_data->mutable_flow_info();
    flow_info->set_flow_action((session::FlowAction)(flow->config.action));
    flow_info->set_nat_type((session::NatType)(flow->config.nat_type));
    ip_addr_to_spec(flow_info->mutable_nat_sip(), &flow->config.nat_sip);
    ip_addr_to_spec(flow_info->mutable_nat_dip(), &flow->config.nat_dip);
    flow_info->set_nat_sport(flow->config.nat_sport);
    flow_info->set_nat_dport(flow->config.nat_dport);
}

static void
flow_to_flow_spec(flow_t *flow, FlowSpec *spec)
{
    spec->mutable_flow_key()->set_src_vrf_id(flow->config.key.svrf_id);
    spec->mutable_flow_key()->set_dst_vrf_id(flow->config.key.dvrf_id);

    if (flow->config.key.flow_type == FLOW_TYPE_L2) {
        FlowKeyL2 *l2_key = spec->mutable_flow_key()->mutable_l2_key();
        l2_key->set_smac(MAC_TO_UINT64(flow->config.key.smac));
        l2_key->set_dmac(MAC_TO_UINT64(flow->config.key.dmac));
        l2_key->set_ether_type(flow->config.key.ether_type);
        l2_key->set_l2_segment_id(flow->config.key.l2seg_id);
    } else if (flow->config.key.flow_type == FLOW_TYPE_V4) {
        FlowKeyV4 *v4_key = spec->mutable_flow_key()->mutable_v4_key();
        v4_key->set_sip(flow->config.key.sip.v4_addr);
        v4_key->set_dip(flow->config.key.dip.v4_addr);
        v4_key->set_ip_proto(types::IPProtocol(flow->config.key.proto));
        if ((flow->config.key.proto == types::IPPROTO_TCP) ||
            (flow->config.key.proto == types::IPPROTO_UDP)) {
            flow_tcp_to_flow_tcp_spec(flow, v4_key->mutable_tcp_udp());
        } else if (flow->config.key.proto == types::IPPROTO_ICMP) {
            flow_icmp_to_flow_icmp_spec(flow, v4_key->mutable_icmp());
        }

    } else if (flow->config.key.flow_type == FLOW_TYPE_V6) {
        FlowKeyV6 *v6_key = spec->mutable_flow_key()->mutable_v6_key();
        v6_key->mutable_sip()->set_v6_addr(&flow->config.key.sip.v6_addr, IP6_ADDR8_LEN);
        v6_key->mutable_dip()->set_v6_addr(&flow->config.key.dip.v6_addr, IP6_ADDR8_LEN);
        v6_key->set_ip_proto(types::IPProtocol(flow->config.key.proto));
        if ((flow->config.key.proto == types::IPPROTO_TCP) ||
            (flow->config.key.proto == types::IPPROTO_UDP)) {
            flow_tcp_to_flow_tcp_spec(flow, v6_key->mutable_tcp_udp());
        } else if (flow->config.key.proto == types::IPPROTO_ICMPV6) {
            flow_icmp_to_flow_icmp_spec(flow, v6_key->mutable_icmp());
        }
    }

    flow_data_to_flow_data_spec(flow, spec->mutable_flow_data());

}

static void
session_to_session_get_response (session_t *session, SessionGetResponse *response)
{
    vrf_t   *vrf = vrf_lookup_by_handle(session->vrf_handle);

    response->mutable_status()->set_session_handle(session->hal_handle);
    response->mutable_spec()->mutable_meta()->set_vrf_id(vrf->vrf_id);
    response->mutable_spec()->set_conn_track_en(session->config.conn_track_en);
    response->mutable_spec()->set_tcp_ts_option(session->config.tcp_ts_option);

    flow_to_flow_spec(session->iflow,
                      response->mutable_spec()->mutable_initiator_flow());
    if (session->rflow) {
        flow_to_flow_spec(session->rflow,
                          response->mutable_spec()->mutable_responder_flow());
    }
}

static void
flow_state_to_flow_stats_response (flow_state_t *flow_state,
                                   FlowStats *stats)
{
    stats->set_flow_permitted_packets(flow_state->packets);
    stats->set_flow_permitted_bytes(flow_state->bytes);
    stats->set_flow_dropped_packets(flow_state->drop_packets);
    stats->set_flow_dropped_bytes(flow_state->drop_bytes);
    return;
}

static void
session_state_to_session_get_response (session_state_t *session_state,
                                       SessionGetResponse *response)
{
    flow_state_to_flow_stats_response(&session_state->iflow_state,
         response->mutable_stats()->mutable_initiator_flow_stats());
    flow_state_to_flow_stats_response(&session_state->rflow_state,
         response->mutable_stats()->mutable_responder_flow_stats());
    return;
}

hal_ret_t
system_get_fill_rsp (session_t *session, SessionGetResponse *response)
{
    pd::pd_session_get_args_t   args;
    session_state_t             session_state;
    hal_ret_t                   ret = HAL_RET_OK;
    pd::pd_func_args_t          pd_func_args = {0};

    args.session = session;
    args.session_state = &session_state;
    pd_func_args.pd_session_get = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SESSION_GET, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to fetch session state for session {}",
                      session->hal_handle);
        return HAL_RET_ERR;
    }

    session_to_session_get_response(session, response);
    session_state_to_session_get_response(&session_state, response);
    response->set_api_status(types::API_STATUS_OK);
    ret = fte::session_get(session, response);

    return ret;
}

hal_ret_t
session_get (SessionGetRequest& req, SessionGetResponse *response)
{
    session_t                   *session;

    if (!req.has_meta() ||
        req.meta().vrf_id() == HAL_VRF_ID_INVALID) {
        response->set_api_status(types::API_STATUS_VRF_ID_INVALID);
        return HAL_RET_INVALID_ARG;
    }

    session = find_session_by_handle(req.session_handle());
    if (session == NULL) {
        response->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_SESSION_NOT_FOUND;
    }

    return system_get_fill_rsp(session, response);
}

//-----------------------------------------------------------------------------
// flow create FTE
//-----------------------------------------------------------------------------
static flow_t *
flow_create_fte (const flow_cfg_t *cfg,
                 const flow_cfg_t *cfg_assoc,
                 const flow_pgm_attrs_t *attrs,
                 const flow_pgm_attrs_t *attrs_assoc,
                 session_t *session)
{
    flow_t      *assoc_flow = NULL;

    flow_t *flow = (flow_t *)g_hal_state->flow_slab()->alloc();
    if (!flow) {
        return NULL;
    }

    *flow = {};
    HAL_SPINLOCK_INIT(&flow->slock, PTHREAD_PROCESS_SHARED);
    flow->flow_key_ht_ctxt.reset();

    if (cfg){
        flow->config = *cfg;
    }

    if (attrs) {
        flow->pgm_attrs = *attrs;
    }

    flow->session = session;

    // check if we have to create associated flow
    if (cfg_assoc) {
        assoc_flow = (flow_t *)g_hal_state->flow_slab()->alloc();
        if (!assoc_flow) {
            return NULL;
        }
        *assoc_flow = {};
        HAL_SPINLOCK_INIT(&assoc_flow->slock, PTHREAD_PROCESS_SHARED);
        assoc_flow->flow_key_ht_ctxt.reset();
        assoc_flow->config = *cfg_assoc;
        if (attrs_assoc) {
            assoc_flow->pgm_attrs = *attrs_assoc;
        }
        assoc_flow->session = session;

        // If its an aug flow, goto assoc flow to get all params
        assoc_flow->is_aug_flow = true;

        // Link
        flow->assoc_flow = assoc_flow;
        assoc_flow->assoc_flow = flow;
    }

    return flow;
}

hal_ret_t
session_create (const session_args_t *args, hal_handle_t *session_handle,
                session_t **session_p)
{
    hal_ret_t ret;
    nwsec_profile_t              *nwsec_prof;
    pd::pd_session_create_args_t  pd_session_args;
    session_t                    *session;
    pd::pd_func_args_t          pd_func_args = {0};

    HAL_ASSERT(args->vrf && args->iflow && args->iflow_attrs);

    // allocate a session
    session = (session_t *)g_hal_state->session_slab()->alloc();
    if (session == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }
    *session = {};

    session->fte_id = fte::fte_id();
    HAL_TRACE_DEBUG("Creating session {:p}", (void *)session);

    dllist_reset(&session->feature_list_head);
    session->config = *args->session;
    session->vrf_handle = args->vrf->hal_handle;
    session->tcp_cxntrack_timer = NULL;

    // fetch the security profile, if any
    if (args->vrf->nwsec_profile_handle != HAL_HANDLE_INVALID) {
        nwsec_prof =
            find_nwsec_profile_by_handle(args->vrf->nwsec_profile_handle);
    } else {
        nwsec_prof = NULL;
    }

    // Handle the spec info not handled in the FTE
    // TODO(goli) all these should go to appropriate fte features
    if (args->spec) {
        uint8_t  ingress, egress;
        ret = extract_mirror_sessions(args->spec->initiator_flow(), &ingress, &egress);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("session create failure extracting mirror sessions, "
                          "err : {}", ret);
            return ret;
        }
        args->iflow[0]->ing_mirror_session |= ingress;
        args->iflow[0]->eg_mirror_session |= egress;

        if(args->rflow[0] && args->spec->has_responder_flow()) {
            ret = extract_mirror_sessions(args->spec->responder_flow(),
                                          &ingress, &egress);
            if (ret != HAL_RET_OK) {
                HAL_TRACE_ERR("session create failure extracting mirror sessions, "
                              "err : {}", ret);
                return ret;
            }
            args->rflow[0]->ing_mirror_session |= ingress;
            args->rflow[0]->eg_mirror_session |= egress;
        }
    }


    // create flows
    session->iflow = flow_create_fte(args->iflow[0], args->iflow[1],
                                     args->iflow_attrs[0], args->iflow_attrs[1],
                                     session);
    if (session->iflow == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }


    if (args->valid_rflow) {
        session->rflow = flow_create_fte(args->rflow[0], args->rflow[1],
                                         args->rflow_attrs[0], args->rflow_attrs[1],
                                         session);
        if (session->rflow == NULL) {
            ret = HAL_RET_OOM;
            goto end;
        }

        session->iflow->reverse_flow = session->rflow;
        session->rflow->reverse_flow = session->iflow;
    }
    session->hal_handle = hal_alloc_handle();

    // allocate all PD resources and finish programming, if any
    pd::pd_session_create_args_init(&pd_session_args);
    pd_session_args.iflow_hash = args->flow_hash;
    pd_session_args.vrf = args->vrf;
    pd_session_args.nwsec_prof = nwsec_prof;
    pd_session_args.session = session;
    pd_session_args.session_state = args->session_state;
    pd_session_args.rsp = args->rsp;
    pd_session_args.update_iflow = true;

    pd_func_args.pd_session_create = &pd_session_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SESSION_CREATE, &pd_func_args);
    if (ret != HAL_RET_OK && ret != HAL_RET_COLL) {
        HAL_TRACE_ERR("PD session create failure, err : {}", ret);
        goto end;
    }

    // add this session to our db
    ret = add_session_to_db(args->vrf, args->sl2seg, args->dl2seg,
                            args->sep, args->dep, args->sif, args->dif, session);
    HAL_ASSERT(ret == HAL_RET_OK);

    if (session_handle) {
        *session_handle = session->hal_handle;
    }

    if (session_p) {
        *session_p = session;
    }

 end:
    if (session && ret != HAL_RET_OK) {
        HAL_TRACE_ERR("session create failure, err={}", ret);
        session_cleanup(session);
    }

    return ret;
}

session_t *
session_lookup(flow_key_t key, flow_role_t *role)
{
    session_t *session = NULL;

    session = (session_t *)g_hal_state->session_hal_iflow_ht()->lookup(std::addressof(key));
    if (role)
        *role = FLOW_ROLE_INITIATOR;
    if (session == NULL) {
        session = (session_t *)g_hal_state->session_hal_rflow_ht()->lookup(std::addressof(key));
        if (role)
            *role = FLOW_ROLE_RESPONDER;
    }

    return session;
}

hal_ret_t
session_update(const session_args_t *args, session_t *session)
{
    hal_ret_t                       ret;
    pd::pd_session_update_args_t    pd_session_args;
    pd::pd_func_args_t              pd_func_args = {0};

    HAL_ASSERT_RETURN(session->fte_id == fte::fte_id(), HAL_RET_INVALID_ARG);

    if (args->iflow[0]) {
        session->iflow->config = *args->iflow[0];
        session->iflow->pgm_attrs = *args->iflow_attrs[0];
        if (session->iflow->assoc_flow && args->iflow[1]) {
            session->iflow->assoc_flow->config = *args->iflow[1];
            session->iflow->assoc_flow->pgm_attrs = *args->iflow_attrs[1];
        }
    }

    if (session->rflow && args->rflow[0]) {
        session->rflow->config = *args->rflow[0];
        session->rflow->pgm_attrs = *args->rflow_attrs[0];
        if(session->rflow->assoc_flow && args->rflow[1]) {
            session->rflow->assoc_flow->config = *args->rflow[1];
            session->rflow->assoc_flow->pgm_attrs = *args->rflow_attrs[1];
        }
    } else if (args->valid_rflow) {
        session->rflow = flow_create_fte(args->rflow[0], args->rflow[1],
                                         args->rflow_attrs[0], args->rflow_attrs[1],
                                         session);
        if (session->rflow == NULL) {
            return HAL_RET_OOM;
        }

        session->iflow->reverse_flow = session->rflow;
        session->rflow->reverse_flow = session->iflow;
    }

    // allocate all PD resources and finish programming, if any
    pd::pd_session_update_args_init(&pd_session_args);
    pd_session_args.vrf = args->vrf;
    pd_session_args.session = session;
    pd_session_args.session_state = args->session_state;
    pd_session_args.rsp = args->rsp;

    pd_func_args.pd_session_update = &pd_session_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SESSION_UPDATE, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD session update failure, err : {}", ret);
    }

    return ret;
}

hal_ret_t
session_delete(const session_args_t *args, session_t *session)
{
    hal_ret_t                       ret;
    pd::pd_session_delete_args_t    pd_session_args;
    pd::pd_func_args_t              pd_func_args = {0};


    HAL_ASSERT_RETURN(session->fte_id == fte::fte_id(), HAL_RET_INVALID_ARG);

    // Stop any timers that might be running
    if (session->tcp_cxntrack_timer) {
        periodic::timer_delete(session->tcp_cxntrack_timer);
        session->tcp_cxntrack_timer = NULL;
    }

    // allocate all PD resources and finish programming, if any
    pd::pd_session_delete_args_init(&pd_session_args);
    pd_session_args.vrf =
        args ? args->vrf : vrf_lookup_by_handle(session->vrf_handle);
    pd_session_args.session = session;
    pd_session_args.session_state = args ? args->session_state : NULL;
    pd_session_args.rsp = args ? args->rsp : NULL;

    pd_func_args.pd_session_delete = &pd_session_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SESSION_DELETE, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD session delete failure, err : {}", ret);
    }

    del_session_from_db(args->sep, args->dep, session);

    session_cleanup(session);

    return ret;
}

//------------------------------------------------------------------------------
// Get session idle timeout based on the flow key
// Use a default timeout in case security profile is not found
//------------------------------------------------------------------------------
static uint64_t
session_aging_timeout (session_t *session,
                       flow_t *iflow, flow_state_t *iflow_state,
                       flow_t *rflow, flow_state_t *rflow_state)
{
    uint64_t            timeout = SESSION_SW_DEFAULT_TIMEOUT;
    vrf_t              *vrf = NULL;
    nwsec_profile_t    *nwsec_prof = NULL;

    vrf = vrf_lookup_by_handle(session->vrf_handle);
    if (vrf != NULL) {
        nwsec_prof = find_nwsec_profile_by_handle(vrf->nwsec_profile_handle);
        if (nwsec_prof != NULL) {
            switch (iflow->config.key.proto) {
                case IPPROTO_TCP:
                    if (iflow->pgm_attrs.drop) {
                        timeout = nwsec_prof->tcp_drop_timeout;
                    } else {
                        timeout = nwsec_prof->tcp_timeout;
                    }
                    break;

                case IPPROTO_UDP:
                    if (iflow->pgm_attrs.drop) {
                        timeout = nwsec_prof->udp_drop_timeout;
                    } else {
                        timeout = nwsec_prof->udp_timeout;
                    }
                    break;

                case IPPROTO_ICMP:
                    if (iflow->pgm_attrs.drop) {
                        timeout = nwsec_prof->icmp_drop_timeout;
                    } else {
                        timeout = nwsec_prof->icmp_timeout;
                    }
                    break;
                default:
                    if (iflow->pgm_attrs.drop) {
                        timeout = nwsec_prof->drop_timeout;
                    } else {
                        timeout = nwsec_prof->session_idle_timeout;
                    }
            }
        }
    }

    return ((uint64_t)(timeout * TIME_NSECS_PER_SEC));
}

#define SESSION_AGED_RET(ENTRY)                                     \
    ENTRY(SESSION_AGED_NONE,   0,  "SESSION_AGED_NONE")             \
    ENTRY(SESSION_AGED_IFLOW,  1,  "SESSION_AGED_IFLOW")            \
    ENTRY(SESSION_AGED_RFLOW,  2,  "SESSION_AGED_RFLOW")            \
    ENTRY(SESSION_AGED_BOTH,   3,  "SESSION_AGED_BOTH")             \

DEFINE_ENUM(session_aged_ret_t, SESSION_AGED_RET)
#undef SESSION_AGED_RET
/*
 * TCP tickle timer context used to
 * send out TCP keepalives & RST in case
 * of session aging
 */
typedef struct tcptkle_timer_ctx_ {
    hal_handle_t        session_handle;
    session_state_t     session_state;
    session_aged_ret_t  aged_flow;
    uint8_t             num_tickles;
    dllist_ctxt_t       dllist_ctxt;
} __PACK__ tcptkle_timer_ctx_t;

static hal_ret_t
build_tcp_packet (hal::flow_key_t key, flow_state_t state,
                  pd::cpu_to_p4plus_header_t *cpu_header,
                  uint8_t *pkt, bool setrst=false)
{
    pd::pd_l2seg_get_fromcpu_vlanid_args_t   args;
    ether_header_t                          *eth_hdr;
    ipv4_header_t                           *ip_hdr;
    tcp_header_t                            *tcp_hdr;
    hal_ret_t                                ret = HAL_RET_OK;
    ep_t                                    *sep = NULL, *dep = NULL;
    l2seg_t                                 *sl2seg = NULL;
    pd::pd_func_args_t                      pd_func_args = {0};

    if (!pkt) {
        return HAL_RET_INVALID_ARG;
    }

    ret = ep_get_from_flow_key(&key, &sep, &dep);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Couldnt get SEP/DEP from session :{}", key);
        return HAL_RET_ERR;
    }
    sl2seg = hal::l2seg_lookup_by_handle(sep->l2seg_handle);
    if (sl2seg == NULL) {
        HAL_TRACE_ERR("Couldnt get source l2seg for session :{}", key);
        return HAL_RET_ERR;
    }

    args.l2seg = sl2seg;
    args.vid = &cpu_header->hw_vlan_id;

    pd_func_args.pd_l2seg_get_fromcpu_vlanid = &args;
    if (pd::hal_pd_call(hal::pd::PD_FUNC_ID_L2SEG_GET_FRCPU_VLANID,
                                      &pd_func_args) == HAL_RET_OK) {
        cpu_header->flags |= CPU_TO_P4PLUS_FLAGS_UPD_VLAN;
    }


    // get the eth type
    eth_hdr = (ether_header_t *)(pkt);
    eth_hdr->etype = htons((key.flow_type == FLOW_TYPE_V4)?ETH_TYPE_IPV4:ETH_TYPE_IPV6);
    memcpy(eth_hdr->dmac, dep->l2_key.mac_addr, ETH_ADDR_LEN);
    memcpy(eth_hdr->smac, sep->l2_key.mac_addr, ETH_ADDR_LEN);

    // fix the IP header
    if (key.flow_type == FLOW_TYPE_V4) {
        ip_hdr = (ipv4_header_t *)(pkt + sizeof(ether_header_t));
        ip_hdr->version = 4;
        ip_hdr->ihl = sizeof(ipv4_header_t)/4;
        ip_hdr->tot_len = htons((ip_hdr->ihl << 2) + TCP_HDR_MIN_LEN);
        ip_hdr->saddr = htonl(key.sip.v4_addr);
        ip_hdr->daddr = htonl(key.dip.v4_addr);
        ip_hdr->id = htons((uint16_t)0xBEAD);
        ip_hdr->protocol = IPPROTO_TCP;
        ip_hdr->frag_off = 0;
        ip_hdr->ttl = IP_DEFAULT_TTL;
        ip_hdr->check = 0;    // let P4 datapath compute checksum
    } else {
        // no IPv6 support
        return HAL_RET_INVALID_ARG;
    }

    // fix the TCP header
    tcp_hdr = (tcp_header_t *)(pkt + sizeof(ether_header_t) + sizeof(ipv4_header_t));
    tcp_hdr->sport = htons(key.sport);
    tcp_hdr->dport = htons(key.dport);
    tcp_hdr->seq = htonl((setrst)?state.tcp_seq_num:(state.tcp_seq_num-1));
    tcp_hdr->ack_seq = htonl(state.tcp_ack_num);
    tcp_hdr->doff = 5;    // TCP header size in words
    tcp_hdr->res1 = 0;
    tcp_hdr->cwr = 0;
    tcp_hdr->ece = 0;
    tcp_hdr->urg = 0;
    tcp_hdr->ack = 1;
    tcp_hdr->psh = 0;
    tcp_hdr->rst = (setrst)?1:0;
    tcp_hdr->syn = 0;
    tcp_hdr->fin = 0;
    tcp_hdr->window = 0;
    tcp_hdr->check = 0;    // let P4 datapath compute checksum
    tcp_hdr->urg_ptr = 0;

    return HAL_RET_OK;
}

static session_aged_ret_t
hal_has_session_aged (session_t *session, uint64_t ctime_ns,
                     session_state_t *session_state_p)
{
    pd::pd_conv_hw_clock_to_sw_clock_args_t    clock_args;
    flow_t                                    *iflow, *rflow = NULL;
    session_state_t                            session_state;
    uint64_t                                   last_pkt_ts;
    uint64_t                                   session_timeout;
    pd::pd_session_get_args_t                  args;
    SessionSpec                                spec;
    SessionResponse                            rsp;
    hal_ret_t                                  ret;
    session_aged_ret_t                         retval = SESSION_AGED_NONE;
    pd::pd_func_args_t                         pd_func_args = {0};

    // Check if its a TCP flow with connection tracking enabled.
    // And connection tracking timer is not NULL. This means the session
    // is one of connection establishment or connection close phase. Disable
    // aging at that time as the timer would eventually fire and clean up the
    // session anyway.
    if (session->iflow->config.key.proto == IPPROTO_TCP &&
        session->config.conn_track_en &&
        session->tcp_cxntrack_timer != NULL) {
        HAL_TRACE_DEBUG("Session {} connection tracking timer is on -- bailing aging",
                        session->hal_handle);
        return retval;
    }

    // read the initiator flow record
    iflow = session->iflow;
    if (!iflow) {
        HAL_TRACE_ERR("session {} has no iflow, ignoring ...",
                      session->hal_handle);
        return retval;
    }

    rflow = session->rflow;
    args.session = session;
    args.session_state = &session_state;
    pd_func_args.pd_session_get = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SESSION_GET, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to fetch session state for session {}",
                      session->hal_handle);
        return retval;
    }

    // check if iflow has expired now
    // If there is no timeout configured then we do not age the session
    session_timeout = session_aging_timeout(session, iflow,
                              &session_state.iflow_state,
                              rflow, rflow ? &session_state.rflow_state : NULL);
    if (!session_timeout) {
        return retval;
    }

    // Check initiator flow
    // Convert hw clock to sw clock resolving any deltas
    clock_args.hw_tick = session_state.iflow_state.last_pkt_ts;
    clock_args.sw_ns = &last_pkt_ts;
    pd_func_args.pd_conv_hw_clock_to_sw_clock = &clock_args;
    pd::hal_pd_call(pd::PD_FUNC_ID_CONV_HW_CLOCK_TO_SW_CLOCK, &pd_func_args);
    HAL_TRACE_DEBUG("Hw tick: {}", session_state.iflow_state.last_pkt_ts);
    HAL_TRACE_DEBUG("session_age_cb: last pkt ts: {} ctime_ns: {} session_timeout: {}",
                    last_pkt_ts, ctime_ns, session_timeout);
    if ((ctime_ns - last_pkt_ts) >= session_timeout) {
        // session hasn't aged yet, move on
        retval = SESSION_AGED_IFLOW;
    }

    if (session->rflow) {
        //check responder flow
        clock_args.hw_tick = session_state.rflow_state.last_pkt_ts;
        clock_args.sw_ns = &last_pkt_ts;
        pd_func_args.pd_conv_hw_clock_to_sw_clock = &clock_args;
        pd::hal_pd_call(pd::PD_FUNC_ID_CONV_HW_CLOCK_TO_SW_CLOCK, &pd_func_args);
        if ((ctime_ns - last_pkt_ts) >= session_timeout) {
            // responder flow seems to be active still
            if (retval == SESSION_AGED_IFLOW)
                retval = SESSION_AGED_BOTH;
            else
                retval = SESSION_AGED_RFLOW;
        }
    }

   *session_state_p = session_state;

   return retval;
}

void build_and_send_tcp_pkt(void *);

static void
tcp_tickle_timeout_cb (void *timer, uint32_t timer_id, void *timer_ctxt)
{
    tcptkle_timer_ctx_t *ctx = (tcptkle_timer_ctx_t *)timer_ctxt;
    timespec_t           ctime;
    uint64_t             ctime_ns;
    session_state_t      session_state;
    session_aged_ret_t   ret = SESSION_AGED_NONE;
    session_t           *session = NULL;

    session = hal::find_session_by_handle(ctx->session_handle);
    if (session == NULL) {
        // Cant find session -- bail out
        goto cleanup;
    }

    // get current time
    clock_gettime(CLOCK_MONOTONIC, &ctime);
    sdk::timestamp_to_nsecs(&ctime, &ctime_ns);
    hal_has_session_aged(session, ctime_ns, &session_state);

    /*
     * We cannot rely on the timestamp here as our tickle would have
     * also incremented the timestamp and we have no way of making sure
     * if we really got a response. Hence, look at the packet count per flow
     */
    if (session_state.iflow_state.packets <=
                              (ctx->session_state.iflow_state.packets + 1)) {
        ret = SESSION_AGED_IFLOW;
    }
    if (session_state.rflow_state.packets <=
                               (ctx->session_state.rflow_state.packets + 1)) {
        if (ret == SESSION_AGED_IFLOW)
            ret = SESSION_AGED_BOTH;
        else
            ret = SESSION_AGED_RFLOW;
    }
    ctx->session_state = session_state;
    if (ret == SESSION_AGED_NONE) {
        HAL_TRACE_DEBUG("Bailing session aging on session {}",
                                           session->iflow->config.key);
        // Session aging is stopped as we saw some packet
        goto cleanup;
    } else {
        fte::fte_softq_enqueue(session->fte_id, /* queue it on the right FTE thread */
                               build_and_send_tcp_pkt, (void *)ctx);
    }

cleanup:
    HAL_FREE(HAL_MEM_ALLOC_SESS_TIMER_CTXT, ctx);
}

void
build_and_send_tcp_pkt (void *data)
{
    pd::cpu_to_p4plus_header_t   cpu_header = {};
    pd::p4plus_to_p4_header_t    p4plus_header = {};
    tcptkle_timer_ctx_t         *ctxt = (tcptkle_timer_ctx_t *)data;
    uint8_t                      pkt[TCP_IPV4_PKT_SZ];
    hal_ret_t                    ret = HAL_RET_OK;
    session_t                   *session = NULL;

    session = hal::find_session_by_handle(ctxt->session_handle);
    if (session == NULL) {
        // Cant find session -- bail out
        goto cleanup;
    }

    // Fill in P4Plus and CPU header info
    p4plus_header.flags = 0;
    p4plus_header.p4plus_app_id = P4PLUS_APPTYPE_CPU;
    cpu_header.src_lif = hal::SERVICE_LIF_CPU;
    cpu_header.l2_offset = 0;

    if (ctxt->num_tickles <= MAX_TCP_TICKLES) {
        // Send tickles to one or both flows
        if (ctxt->aged_flow == SESSION_AGED_IFLOW ||
            ctxt->aged_flow == SESSION_AGED_BOTH) {
            build_tcp_packet(session->iflow->config.key,
                             ctxt->session_state.iflow_state, &cpu_header, pkt);
            fte::fte_asq_send(&cpu_header, &p4plus_header, pkt, TCP_IPV4_PKT_SZ);
        }
        if (ctxt->aged_flow == SESSION_AGED_RFLOW ||
            ctxt->aged_flow == SESSION_AGED_BOTH) {
            build_tcp_packet(session->rflow->config.key,
                             ctxt->session_state.rflow_state, &cpu_header, pkt);
            fte::fte_asq_send(&cpu_header, &p4plus_header, pkt, TCP_IPV4_PKT_SZ);
        }

        HAL_TRACE_DEBUG("Sending another tickle and starting timer {}",
                         session->iflow->config.key);
        ctxt->num_tickles++;
        session->tcp_cxntrack_timer = hal::periodic::timer_schedule(
                                              HAL_TIMER_ID_TCP_TICKLE_WAIT,
                                              SESSION_DEFAULT_TCP_TICKLE_TIMEOUT,
                                              (void *)ctxt,
                                              tcp_tickle_timeout_cb, false);
    } else {
        // Send TCP RST to the flow that hasnt aged
        if (ctxt->aged_flow == SESSION_AGED_RFLOW) {
            build_tcp_packet(session->iflow->config.key,
                              ctxt->session_state.iflow_state, &cpu_header, pkt, true);
            fte::fte_asq_send(&cpu_header, &p4plus_header, pkt, TCP_IPV4_PKT_SZ);
        }
        if (ctxt->aged_flow == SESSION_AGED_IFLOW) {
            build_tcp_packet(session->rflow->config.key,
                             ctxt->session_state.rflow_state, &cpu_header, pkt, true);
            fte::fte_asq_send(&cpu_header, &p4plus_header, pkt, TCP_IPV4_PKT_SZ);
        }
        // time to clean up the session
        ret = fte::session_delete_in_fte(session->hal_handle);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("Failed to delete aged session {}",
                          session->iflow->config.key);
        }
    }

cleanup:
    // Free the context
    HAL_FREE(HAL_MEM_ALLOC_SESS_TIMER_CTXT, ctxt);
}

void
process_hal_periodic_tkle (void *data)
{
    tcptkle_timer_ctx_t **tctx_list = (tcptkle_timer_ctx_t **)data;

    for (uint8_t i=0; i<HAL_MAX_SESSION_PER_ENQ; i++) {
        if (tctx_list[i])
            build_and_send_tcp_pkt((void *)tctx_list[i]);
    }
    HAL_FREE(HAL_MEM_ALLOC_SESS_TIMER_CTXT, tctx_list);
}

void
process_hal_periodic_sess_delete (void *data)
{
    hal_handle_t  *session_list = (hal_handle_t *)data;

    for (uint8_t i=0; i<HAL_MAX_SESSION_PER_ENQ; i++) {
        if (session_list[i])
            fte::session_delete_in_fte(session_list[i]);
    }
    HAL_FREE(HAL_MEM_ALLOC_SESS_HANDLE_LIST, session_list);
}

//------------------------------------------------------------------------------
// determine whether a given session should be aged or not
//------------------------------------------------------------------------------
struct session_age_cb_args_t {
    uint64_t        ctime_ns;
    uint8_t         num_ctx[HAL_THREAD_ID_MAX];
    uint8_t         num_del_sess[HAL_THREAD_ID_MAX];
};

tcptkle_timer_ctx_t  **tctx_list[HAL_THREAD_ID_MAX];
hal_handle_t          *session_list[HAL_THREAD_ID_MAX];

bool
session_age_cb (void *entry, void *ctxt)
{
    hal_ret_t               ret = HAL_RET_OK;
    session_t              *session = (session_t *)entry;
    session_age_cb_args_t  *args = (session_age_cb_args_t *)ctxt;
    SessionSpec             spec;
    SessionResponse         rsp;
    session_state_t         session_state;
    tcptkle_timer_ctx_t    *tklectx = NULL;
    session_aged_ret_t      retval = SESSION_AGED_NONE;

    // Check if its a TCP flow with connection tracking enabled.
    // And connection tracking timer is not NULL. This means the session
    // is one of connection establishment or connection close phase. Disable
    // aging at that time as the timer would eventually fire and clean up the
    // session anyway.
    if (session->iflow->config.key.proto == IPPROTO_TCP &&
        session->config.conn_track_en &&
        session->tcp_cxntrack_timer != NULL) {
        HAL_TRACE_DEBUG("Session connection tracking timer is on for {} -- bailing aging",
                        session->iflow->config.key);
        return false;
    }


    retval = hal_has_session_aged(session, args->ctime_ns, &session_state);
    if (retval != SESSION_AGED_NONE) {
        /*
         *  Send out TCP tickle if it is a TCP session and start a timer for 2
         *  seconds. We send 3 tickles (keepalives) before we send out TCP RST
         *  and proceed to delete the session
         */
        if (session->iflow->config.key.proto == IPPROTO_TCP) {
            tklectx = (tcptkle_timer_ctx_t *)HAL_CALLOC(HAL_MEM_ALLOC_SESS_TIMER_CTXT,
                                                     sizeof(tcptkle_timer_ctx_t));
            HAL_ASSERT_RETURN((tklectx != NULL), false);

            tklectx->session_handle = session->hal_handle;
            tklectx->num_tickles = 1;
            tklectx->session_state = session_state;
            tklectx->aged_flow = retval;
            tctx_list[session->fte_id][args->num_ctx[session->fte_id]++] = tklectx;

            // Enqueue if we have reached the threshold or there are
            // no more to process
            if (args->num_ctx[session->fte_id] == HAL_MAX_SESSION_PER_ENQ) {

                ret = fte::fte_softq_enqueue(session->fte_id,
                                    process_hal_periodic_tkle, (void *)tctx_list[session->fte_id]);
                // If the tickle is successfully queued then
                // return otherwise go ahead and cleanup
                if (ret == HAL_RET_OK) {
                    HAL_TRACE_DEBUG("Successfully enqueued TCP tickle {}",
                                                   session->iflow->config.key);
                }
                tctx_list[session->fte_id] = (tcptkle_timer_ctx_t **)HAL_CALLOC(
                          HAL_MEM_ALLOC_SESS_TIMER_CTXT,
                          sizeof(tcptkle_timer_ctx_t*)*HAL_MAX_SESSION_PER_ENQ);
                HAL_ASSERT(tctx_list[session->fte_id] == NULL);
                args->num_ctx[session->fte_id] = 0;
            }
        } else {
            // time to clean up the session, add handle to session list
            session_list[session->fte_id][args->num_del_sess[session->fte_id]++] = session->hal_handle;

            if (args->num_ctx[session->fte_id] == HAL_MAX_SESSION_PER_ENQ) {
                ret = fte::fte_softq_enqueue(session->fte_id,
                                    process_hal_periodic_sess_delete, (void *)session_list[session->fte_id]);
                HAL_ASSERT(ret == HAL_RET_OK);

                session_list[session->fte_id] = (hal_handle_t *)HAL_CALLOC(HAL_MEM_ALLOC_SESS_HANDLE_LIST,
                                sizeof(hal_handle_t)*HAL_MAX_SESSION_PER_ENQ);
                HAL_ASSERT(session_list[session->fte_id] == NULL);
                args->num_del_sess[session->fte_id] = 0;
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------------
// callback invoked by the HAL periodic thread for session aging
//------------------------------------------------------------------------------
static void
session_age_walk_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    uint32_t              i, bucket = *((uint32_t *)(&ctxt));
    timespec_t            ctime;
    hal_ret_t             ret = HAL_RET_OK;
    uint8_t               fte_id = 0;

    session_age_cb_args_t args;

    for (fte_id=0; fte_id<g_hal_state->oper_db()->max_data_threads(); fte_id++) {
        tctx_list[fte_id] = (tcptkle_timer_ctx_t **)HAL_CALLOC(HAL_MEM_ALLOC_SESS_TIMER_CTXT,
                              sizeof(tcptkle_timer_ctx_t*)*HAL_MAX_SESSION_PER_ENQ);
        HAL_ASSERT(tctx_list[fte_id] != NULL);

        session_list[fte_id] = (hal_handle_t *)HAL_CALLOC(HAL_MEM_ALLOC_SESS_HANDLE_LIST,
                                sizeof(hal_handle_t)*HAL_MAX_SESSION_PER_ENQ);
        HAL_ASSERT(session_list[fte_id] != NULL);
        args.num_ctx[fte_id] = 0;
        args.num_del_sess[fte_id] = 0;
    }

    // get current time
    clock_gettime(CLOCK_MONOTONIC, &ctime);
    sdk::timestamp_to_nsecs(&ctime, &args.ctime_ns);

    //HAL_TRACE_DEBUG("timer id {}, bucket: {}", timer_id,  bucket);
    for (i = 0; i < HAL_SESSION_BUCKETS_TO_SCAN_PER_INTVL; i++) {
        g_hal_state->session_hal_handle_ht()->walk_bucket_safe(bucket,
                                                     session_age_cb, &args);
        bucket = (bucket + 1)%g_hal_state->session_hal_handle_ht()->num_buckets();
    }

    //Check if there are pending requests that need to be queued to FTE threads
    for (fte_id=0; fte_id<g_hal_state->oper_db()->max_data_threads(); fte_id++) {
        if (args.num_ctx[fte_id]) {

            //HAL_TRACE_DEBUG("Enqueuing tickles for fte: {}", fte_id);
            ret = fte::fte_softq_enqueue(fte_id,
                                  process_hal_periodic_tkle, (void *)tctx_list[fte_id]);
            HAL_ASSERT(ret == HAL_RET_OK);
        }

        if (args.num_del_sess[fte_id]) {
            //HAL_TRACE_DEBUG("Enqueuing deletes for fte: {}", fte_id);
            ret = fte::fte_softq_enqueue(fte_id,
                                  process_hal_periodic_sess_delete, (void *)session_list[fte_id]);
            HAL_ASSERT(ret == HAL_RET_OK);

        }
    }

    // store the bucket id to resume on next invocation
    hal::periodic::timer_update(timer, HAL_SESSION_AGE_SCAN_INTVL,
                                true, reinterpret_cast<void *>(bucket));
}

//------------------------------------------------------------------------------
// initialize the session management module
//------------------------------------------------------------------------------
hal_ret_t
session_init (hal_cfg_t *hal_cfg)
{
    g_hal_state->oper_db()->set_max_data_threads(hal_cfg->num_data_threads);

    // Disable aging when FTE is disabled
    if (getenv("DISABLE_AGING")) {
        return HAL_RET_OK;
    }

    // wait until the periodic thread is ready
    while (!hal::periodic::periodic_thread_is_running()) {
        pthread_yield();
    }
    g_session_timer =
        hal::periodic::timer_schedule(HAL_TIMER_ID_SESSION_AGEOUT,            // timer_id
                                      HAL_SESSION_AGE_SCAN_INTVL,
                                      (void *)0,    // ctxt
                                      session_age_walk_cb, true);
    if (!g_session_timer) {
        return HAL_RET_ERR;
    }
    HAL_TRACE_DEBUG("Started session aging periodic timer with {} ms invl",
                    HAL_SESSION_AGE_SCAN_INTVL);
    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// callback invoked by the Session TCP close timer to cleanup session state
//------------------------------------------------------------------------------
static void
tcp_close_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    hal_ret_t     ret;
    hal_handle_t  session_handle = (hal_handle_t)ctxt;
    session_t    *session = NULL;

    session = hal::find_session_by_handle(session_handle);
    if (session == NULL) {
        HAL_TRACE_ERR("Cant find the session for handle {} -- bailing",
                                                         session_handle);
        return;
    }

    HAL_TRACE_DEBUG("TCP close timer callback -- deleting session with id {}",
                    session->iflow->config.key);

    // time to clean up the session
    ret = fte::session_delete(session);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to delte aged session {}",
                      session->hal_handle);
    }
}

typedef enum timeout_type_ {
    TCP_CXNSETUP_TIMEOUT = 1,
    TCP_HALF_CLOSED_TIMEOUT = 2,
    TCP_CLOSE_TIMEOUT = 3,
} timeout_type_t;
//------------------------------------------------------------------------------
// Get TCP timeout from nwsec profile
//------------------------------------------------------------------------------
static uint64_t
get_tcp_timeout (session_t *session, timeout_type_t timeout)
{
    vrf_t              *vrf = NULL;
    nwsec_profile_t    *nwsec_prof = NULL;

    vrf = vrf_lookup_by_handle(session->vrf_handle);
    if (vrf != NULL) {
        nwsec_prof = find_nwsec_profile_by_handle(vrf->nwsec_profile_handle);
    }

    switch (timeout) {
        case TCP_CXNSETUP_TIMEOUT:
        {
            if (nwsec_prof != NULL) {
                return ((uint64_t)(nwsec_prof->tcp_cnxn_setup_timeout * TIME_MSECS_PER_SEC));
            } else {
                return (SESSION_SW_DEFAULT_TCP_CXNSETUP_TIMEOUT);
            }
        }
        break;

        case TCP_HALF_CLOSED_TIMEOUT:
        {
            if (nwsec_prof != NULL) {
                return ((uint64_t)(nwsec_prof->tcp_half_closed_timeout * TIME_MSECS_PER_SEC));
            } else {
                return (SESSION_SW_DEFAULT_TCP_HALF_CLOSED_TIMEOUT);
            }
        }
        break;

        case TCP_CLOSE_TIMEOUT:
        {
            if (nwsec_prof != NULL) {
                return ((uint64_t)(nwsec_prof->tcp_close_timeout * TIME_MSECS_PER_SEC));
            } else {
                return (SESSION_SW_DEFAULT_TCP_CLOSE_TIMEOUT);
            }
        }
        break;

        default: break;
    }

    return 0;
}

//------------------------------------------------------------------------------
// API to start timer on TCP close when RST is received or bidirectional
// FIN is received
//------------------------------------------------------------------------------
hal_ret_t
schedule_tcp_close_timer (session_t *session)
{
    flow_key_t  key = {};


    if (getenv("DISABLE_AGING")) {
        return HAL_RET_OK;
    }

    // Delete the previous timers if any and start a new one
    if (session->tcp_cxntrack_timer != NULL) {
        periodic::timer_delete(session->tcp_cxntrack_timer);
        session->tcp_cxntrack_timer = NULL;
    }

    session->tcp_cxntrack_timer = hal::periodic::timer_schedule(
                                     HAL_TIMER_ID_TCP_CLOSE_WAIT,
                                     get_tcp_timeout(session, TCP_CLOSE_TIMEOUT),
                                     (void *)(session->hal_handle), tcp_close_cb, false);
    if (!session->tcp_cxntrack_timer) {
        return HAL_RET_ERR;
    }
    HAL_TRACE_DEBUG("TCP Close timer started for session {}",
                    (session->iflow)?session->iflow->config.key:key);

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// Callback invoked when TCP half closed timer fires
//------------------------------------------------------------------------------
void
tcp_half_close_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    hal_ret_t                 ret;
    hal_handle_t              session_handle = (hal_handle_t)ctxt;
    session_t                *session = NULL;
    pd::pd_session_get_args_t args;
    session_state_t           state;
    pd::pd_func_args_t        pd_func_args = {0};

    session = hal::find_session_by_handle(session_handle);
    if (session == NULL) {
        HAL_TRACE_ERR("Cant find the session for handle {} -- bailing",
                                                         session_handle);
        return;
    }

    args.session = session;
    args.session_state = &state;
    pd_func_args.pd_session_get = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SESSION_GET, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to fetch iflow record of session {}",
                       session->config.session_id);
    }

    HAL_TRACE_DEBUG("IFlow State: {}", state.iflow_state.state);

    if (session->iflow)
        session->iflow->state = state.iflow_state.state;
    if (session->rflow)
        session->rflow->state = state.rflow_state.state;

    // If we havent received bidir FIN by now then we go ahead and cleanup
    // the session
    if (state.iflow_state.state != session::FLOW_TCP_STATE_BIDIR_FIN_RCVD) {
        tcp_close_cb(timer, timer_id, (void *)(session->hal_handle));
    }
}

//------------------------------------------------------------------------------
// API to start timer when TCP FIN is seen for the first time on the session
// This is to give enough time for other side to send the FIN
//------------------------------------------------------------------------------
hal_ret_t
schedule_tcp_half_closed_timer (session_t *session)
{
    if (getenv("DISABLE_AGING")) {
        return HAL_RET_OK;
    }

    // Delete the previous timers if any and start a new one
    if (session->tcp_cxntrack_timer != NULL) {
        periodic::timer_delete(session->tcp_cxntrack_timer);
        session->tcp_cxntrack_timer = NULL;
    }

    session->tcp_cxntrack_timer = hal::periodic::timer_schedule(
                                     HAL_TIMER_ID_TCP_HALF_CLOSED_WAIT,
                                     get_tcp_timeout(session, TCP_HALF_CLOSED_TIMEOUT),
                                     (void *)(session->hal_handle),
                                     tcp_half_close_cb, false);

    if (!session->tcp_cxntrack_timer) {
        return HAL_RET_ERR;
    }
    HAL_TRACE_DEBUG("TCP Half Closed timer started for session {}",
                     session->hal_handle);

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// callback invoked by the Session TCP CXN setup timer. If the session is not
// in established state then cleanup the session
//------------------------------------------------------------------------------
static void
tcp_cxnsetup_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    hal_ret_t                 ret;
    hal_handle_t              session_handle = (hal_handle_t)ctxt;
    pd::pd_session_get_args_t args;
    session_state_t           state;
    session_t                *session = NULL;
    pd::pd_func_args_t       pd_func_args = {0};

    session = hal::find_session_by_handle(session_handle);
    if (session == NULL) {
        HAL_TRACE_ERR("Cant find the session for handle {} -- bailing", session_handle);
        return;
    }

    args.session = session;
    args.session_state = &state;
    pd_func_args.pd_session_get = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SESSION_GET, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to fetch iflow record of session {}",
                       session->hal_handle);
    }

    HAL_TRACE_DEBUG("IFlow State: {}", state.iflow_state.state);

    if (session->iflow)
        session->iflow->state = state.iflow_state.state;
    if (session->rflow)
        session->rflow->state = state.rflow_state.state;

    if (state.iflow_state.state != session::FLOW_TCP_STATE_ESTABLISHED ||
        state.rflow_state.state != session::FLOW_TCP_STATE_ESTABLISHED) {
        // session is not in established state yet.
        // Cleanup the session
        ret = fte::session_delete(session);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("Failed to delete session {}",
                          session->hal_handle);
        }
    }
}

//------------------------------------------------------------------------------
// API to start timer when TCP SYN received to make sure the session
// goes to established state within the given timeout
//------------------------------------------------------------------------------
hal_ret_t
schedule_tcp_cxnsetup_timer (session_t *session)
{
    if (getenv("DISABLE_AGING")) {
        return HAL_RET_OK;
    }

    session->tcp_cxntrack_timer = hal::periodic::timer_schedule(
                                        HAL_TIMER_ID_TCP_CXNSETUP_WAIT,
                                        get_tcp_timeout(session, TCP_CXNSETUP_TIMEOUT),
                                        (void *)(session->hal_handle),
                                        tcp_cxnsetup_cb, false);

    if (!session->tcp_cxntrack_timer) {
        return HAL_RET_ERR;
    }
    HAL_TRACE_DEBUG("TCP Cxn Setup timer started for session {}",
                     session->hal_handle);

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// API to set the runtime TCP state when FTE sees TCP close packets
//------------------------------------------------------------------------------
void
session_set_tcp_state (session_t *session, hal::flow_role_t role,
                       FlowTCPState tcp_state)
{
    hal::flow_t *flow = NULL;

    if (role == hal::FLOW_ROLE_INITIATOR) {
        flow = session->iflow;
    } else {
        flow = session->rflow;
    }

    if (flow)
        flow->state = tcp_state;

    HAL_TRACE_DEBUG("Updated tcp state to {}", tcp_state);
}

hal_ret_t
session_get_all(SessionGetResponseMsg *rsp)
{
    auto walk_func = [](void *entry, void *ctxt) {
        hal::session_t  *session = (session_t *)entry;
        SessionGetResponseMsg *rsp = (SessionGetResponseMsg *)ctxt;
        system_get_fill_rsp(session, rsp->add_response());
        return false;
    };

    sdk_ret_t ret = g_hal_state->session_hal_handle_ht()->walk_safe(walk_func, rsp);

    return hal_sdk_ret_to_hal_ret(ret);
}

hal_ret_t
session_delete_list (dllist_ctxt_t *session_list, bool async)
{
    // delete all sessions
    hal_ret_t ret = HAL_RET_OK;
    dllist_ctxt_t  *curr = NULL, *next = NULL;
    dllist_for_each_safe(curr, next, session_list) {
        hal_handle_id_list_entry_t  *entry =
            dllist_entry(curr, hal_handle_id_list_entry_t, dllist_ctxt);
        hal::session_t *session = hal::find_session_by_handle(entry->handle_id);
        if (session) {
            if (async) {
                ret = fte::session_delete_async(session);
            } else {
                ret = fte::session_delete(session);
            }
        }
        // Remove from list
        dllist_del(&entry->dllist_ctxt);
        g_hal_state->hal_handle_id_list_entry_slab()->free(entry);
    }
    return ret;
}

hal_ret_t
session_delete_all (SessionDeleteResponseMsg *rsp)
{

    auto walk_func = [](void *entry, void *ctxt) {
        hal::session_t  *session = (session_t *)entry;
        dllist_ctxt_t   *list_head = (dllist_ctxt_t  *)ctxt;

        hal_handle_id_list_entry_t *list_entry = (hal_handle_id_list_entry_t *)g_hal_state->
                hal_handle_id_list_entry_slab()->alloc();

        if (entry == NULL) {
            HAL_TRACE_ERR("Out of memory - skipping delete session {}", session->hal_handle);
            return false;
        }

        list_entry->handle_id = session->hal_handle;
        dllist_add(list_head, &list_entry->dllist_ctxt);
        return false;
    };

    // build list of session_ids
    dllist_ctxt_t session_list;
    dllist_reset(&session_list);
    g_hal_state->session_hal_handle_ht()->walk_safe(walk_func, &session_list);

	hal_ret_t ret;
	ret = session_delete_list(&session_list);
	rsp->add_response()->set_api_status(hal::hal_prepare_rsp(ret));

    return HAL_RET_OK;
}

bool
check_session_match (session_match_t *match, hal::session_t *session)
{
    hal::flow_key_t *key = &session->iflow->config.key;
    if (match->match_fields & SESSION_MATCH_SVRF) {
        /** Match on vrf **/
        if (key->svrf_id != match->key.svrf_id) { return FALSE; }
    }

    // Extend this function to add more filter conditions
    return TRUE;
}

hal_ret_t
session_eval_matching_session (session_match_t  *match)
{
    auto walk_func = [](void *entry, void *ctxt) {
        hal::session_t  *session = (session_t *)entry;
        session_match_t *match = (session_match_t *) ctxt;
        dllist_ctxt_t   *list_head = match->session_list;

        if (check_session_match(match, session)) {
            hal_handle_id_list_entry_t *list_entry = (hal_handle_id_list_entry_t *)g_hal_state->
                    hal_handle_id_list_entry_slab()->alloc();
            if (list_entry == NULL) {
                HAL_TRACE_ERR("Out of memory - skipping delete session {}", session->hal_handle);
                return false;
            }
            hal::flow_key_t *key = &session->iflow->config.key;
            hal::ipv4_tuple acl_key = {};
            //fte::extract_acl_key_from_flow_key(&acl_key, key);
            bool allow = securitypolicy_is_allow(key->svrf_id, &acl_key);
            if (!allow) {
                HAL_TRACE_DEBUG("add the handle {}", session->hal_handle);
                list_entry->handle_id = session->hal_handle;
                dllist_add(list_head, &list_entry->dllist_ctxt);
            }
        }
        return false;
    };

    // build list of session_ids
    dllist_ctxt_t session_list;
    dllist_reset(&session_list);

    match->session_list = &session_list;
    HAL_TRACE_DEBUG("calling walk func");
    g_hal_state->session_hal_handle_ht()->walk_safe(walk_func, match);

    HAL_TRACE_DEBUG("delete session");
    session_delete_list(&session_list, true /*async:true*/);
    return HAL_RET_OK;
}

}    // namespace hal
