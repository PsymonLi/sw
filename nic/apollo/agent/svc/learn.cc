//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// server side implementation for Learn service
///
//----------------------------------------------------------------------------

#include "nic/apollo/agent/svc/learn_svc.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/learn/auto/ep_aging.hpp"
#include "nic/apollo/learn/ep_mac_entry.hpp"
#include "nic/apollo/learn/ep_ip_entry.hpp"
#include "nic/apollo/learn/ep_mac_state.hpp"
#include "nic/apollo/learn/ep_ip_state.hpp"
#include "nic/apollo/learn/ep_utils.hpp"
#include "nic/apollo/learn/utils.hpp"

typedef struct ep_ip_get_arg_s {
    pds_obj_key_t subnet;
    pds::LearnIPGetResponse *proto_rsp;
} ep_ip_get_args_t;

static bool
ep_mac_walk_ip_list_cb (void *entry, void *user_data)
{
    pds::LearnMACEntry *proto_entry = (pds::LearnMACEntry *)user_data;
    ep_ip_entry *ip_entry = (ep_ip_entry *)entry;

    pds::LearnIPKey *ip_key = proto_entry->add_ipinfo();
    ipaddr_api_spec_to_proto_spec(
                ip_key->mutable_ipaddr(), &ip_entry->key()->ip_addr);
    ip_key->set_vpcid(ip_entry->key()->vpc.id, PDS_MAX_KEY_LEN);
    return false; // returning false to continue iterating
}

static void
ep_mac_entry_to_proto (ep_mac_entry *mac_entry, pds::LearnMACEntry *proto_entry)
{
    pds_obj_key_t vnic_key;

    vnic_key = learn::ep_vnic_key(mac_entry);
    proto_entry->set_state(pds_learn_state_to_proto(mac_entry->state()));
    proto_entry->set_vnicid(vnic_key.id, PDS_MAX_KEY_LEN);
    proto_entry->mutable_key()->set_macaddr(
                                MAC_TO_UINT64(mac_entry->key()->mac_addr));
    proto_entry->mutable_key()->set_subnetid(mac_entry->key()->subnet.id,
                                             PDS_MAX_KEY_LEN);
    proto_entry->set_ttl(remaining_age_mac(mac_entry));
}

static sdk_ret_t
ep_mac_get (ep_mac_key_t *key, pds::LearnMACGetResponse *proto_rsp)
{
    ep_mac_entry *mac_entry;

    mac_entry = (ep_mac_entry *)learn_db()->ep_mac_db()->find(key);
    if (!mac_entry) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    pds::LearnMACEntry *proto_entry = proto_rsp->add_response();
    ep_mac_entry_to_proto(mac_entry, proto_entry);
    mac_entry->walk_ip_list(ep_mac_walk_ip_list_cb, proto_entry);
    return SDK_RET_OK;
}

static bool
ep_mac_entry_walk_cb (void *entry, void *rsp)
{
    pds::LearnMACGetResponse *proto_rsp = (pds::LearnMACGetResponse *)rsp;
    ep_mac_entry *mac_entry = (ep_mac_entry *)entry;

    pds::LearnMACEntry *proto_entry = proto_rsp->add_response();
    ep_mac_entry_to_proto(mac_entry, proto_entry);
    mac_entry->walk_ip_list(ep_mac_walk_ip_list_cb, proto_entry);
    return false;
}

static sdk_ret_t
ep_mac_get_all (pds::LearnMACGetResponse *proto_rsp)
{
    learn_db()->ep_mac_db()->walk(ep_mac_entry_walk_cb, proto_rsp);
    return SDK_RET_OK;
}

static bool
ep_ip_entry_to_proto (void *entry, void *rsp)
{
    pds::LearnIPGetResponse *proto_rsp = (pds::LearnIPGetResponse *)rsp;
    pds::LearnIPEntry *proto_entry = proto_rsp->add_response();
    ep_ip_entry *ip_entry = (ep_ip_entry *)entry;
    ep_mac_entry *mac_entry = (ep_mac_entry *)ip_entry->mac_entry();

    proto_entry->set_state(pds_learn_state_to_proto(ip_entry->state()));
    ipaddr_api_spec_to_proto_spec(
                          proto_entry->mutable_key()->mutable_ipaddr(),
                          &ip_entry->key()->ip_addr);
    proto_entry->mutable_key()->set_vpcid(ip_entry->key()->vpc.id,
                                          PDS_MAX_KEY_LEN);
    proto_entry->set_ttl(remaining_age_ip(ip_entry));
    if (mac_entry) {
        proto_entry->mutable_macinfo()->set_macaddr(
                          MAC_TO_UINT64(mac_entry->key()->mac_addr));
        proto_entry->mutable_macinfo()->set_subnetid(mac_entry->key()->subnet.id,
                                                    PDS_MAX_KEY_LEN);
    }
    return false;
}

static bool
ep_mac_entry_subnet_filter_walk_cb (void *entry, void *ctxt)
{
    ep_ip_get_args_t *args = (ep_ip_get_args_t *)ctxt;
    ep_mac_entry *mac_entry = (ep_mac_entry *)entry;

    if (mac_entry->key()->subnet != args->subnet) {
        // skip this entry
        return false;
    }
    mac_entry->walk_ip_list(ep_ip_entry_to_proto, args->proto_rsp);
    return false;
}

static sdk_ret_t
ep_ip_get (ep_ip_key_t *key, pds::LearnIPGetResponse *proto_rsp)
{
    ep_ip_entry *ip_entry;

    ip_entry = (ep_ip_entry *)learn_db()->ep_ip_db()->find(key);
    if (!ip_entry) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    ep_ip_entry_to_proto(ip_entry, proto_rsp);
    return SDK_RET_OK;
}

static sdk_ret_t
ep_ip_get_all (pds_obj_key_t subnet, pds::LearnIPGetResponse *proto_rsp)
{
    ep_ip_get_args_t args;

    args.subnet = subnet;
    args.proto_rsp = proto_rsp;
    learn_db()->ep_mac_db()->walk(ep_mac_entry_subnet_filter_walk_cb, &args);
    return SDK_RET_OK;
}

static sdk_ret_t
ep_ip_get_all (pds::LearnIPGetResponse *proto_rsp)
{
    learn_db()->ep_ip_db()->walk(ep_ip_entry_to_proto, proto_rsp);
    return SDK_RET_OK;
}

#define FILL_LEARN_EVENT_COUNTERS(ctr_name, idx, proto_name, verbose)       \
{                                                                           \
    uint8_t learn_type = CTR_IDX_TO_LEARN_TYPE(idx);                        \
    if (verbose || counters->ctr_name[idx]) {                               \
        pds::LearnEvents *events =                                          \
        proto_rsp->mutable_stats()->add_##proto_name();                     \
        events->set_eventtype(pds_ep_learn_type_to_proto(learn_type));      \
        events->set_count(counters->ctr_name[idx]);                         \
    }                                                                       \
}

#define FILL_API_COUNTERS(ctr_name, idx, proto_name, verbose)               \
{                                                                           \
    if (verbose || counters->ctr_name[idx]) {                               \
        pds::LearnApiOps *ops =                                             \
        proto_rsp->mutable_stats()->add_##proto_name();                     \
        ops->set_apioptype(pds_learn_api_op_to_proto(idx));                 \
        ops->set_count(counters->ctr_name[idx]);                            \
    }                                                                       \
}

static sdk_ret_t
ep_learn_stats_fill (pds::LearnStatsGetResponse *proto_rsp)
{
    learn::learn_counters_t *counters = learn_db()->counters();
    auto stats = proto_rsp->mutable_stats();

    // packet counters
    stats->set_pktsrcvd(counters->rx_pkts);
    stats->set_pktssent(counters->tx_pkts_ok);
    stats->set_pktsenderrors(counters->tx_pkts_err);
    stats->set_arpprobessent(counters->arp_probes_ok);
    stats->set_arpprobesenderrors(counters->arp_probes_err);
    stats->set_pktbufferavailable(learn::learn_lif_avail_mbuf_count());
    stats->set_pktbufferalloc(counters->pkt_buf_alloc_ok);
    proto_rsp->mutable_stats()->set_pktbufferallocerrors(counters->pkt_buf_alloc_err);
    for (uint8_t i = (uint8_t)learn::PKT_DROP_REASON_NONE;
         i < (uint8_t)learn::PKT_DROP_REASON_MAX; i++) {
        if (counters->pkt_drop_reason[i]) {
            pds::LearnPktDropStats *drop_stats = stats->add_dropstats();
            drop_stats->set_reason(pds_learn_pkt_drop_reason_to_proto(i));
            drop_stats->set_numdrops(counters->pkt_drop_reason[i]);
        }
    }
    for (uint8_t i = (uint8_t)learn::LEARN_PKT_TYPE_NONE;
         i < (uint8_t)learn::LEARN_PKT_TYPE_MAX; i++) {
        if (counters->rx_pkt_type[i]) {
            pds::LearnPktTypeCounter *rx_pkt_types = stats->add_rcvdpkttypes();
            rx_pkt_types->set_pkttype(pds_learn_pkt_type_to_proto(i));
            rx_pkt_types->set_count(counters->rx_pkt_type[i]);
        }
    }

    // ageout counters
    stats->set_ipageouts(counters->ip_ageout_ok);
    stats->set_ipageouterrors(counters->ip_ageout_err);
    stats->set_macageouts(counters->mac_ageout_ok);
    stats->set_macageouterrors(counters->mac_ageout_err);

    // learn and move counters
    for (uint8_t i = 0; i < learn_type_ctr_sz(); i++) {
        FILL_LEARN_EVENT_COUNTERS(mac_learns_ok, i, maclearnevents, true);
        FILL_LEARN_EVENT_COUNTERS(ip_learns_ok, i, iplearnevents, true);
        FILL_LEARN_EVENT_COUNTERS(mac_learns_err, i, maclearnerrors, false);
        FILL_LEARN_EVENT_COUNTERS(ip_learns_err, i, iplearnerrors, false);
    }

    // learn validation errors
    for (uint8_t i = 0; i < LEARN_VALIDATION_MAX; i++) {
        if (counters->validation_err[i]) {
            pds::LearnValidations *lverr =
                proto_rsp->mutable_stats()->add_validationerrors();
            lverr->set_validationtype(pds_learn_validation_type_to_proto(i));
            lverr->set_count(counters->validation_err[i]);
        }
    }

    // API counters
    for (uint8_t i = 0; i < learn::OP_MAX; i++) {
        FILL_API_COUNTERS(vnic_ok, i, vnicops, true);
        FILL_API_COUNTERS(vnic_err, i, vnicoperrors, false);
        FILL_API_COUNTERS(remote_mac_map_ok, i, remotel2mappings, true);
        FILL_API_COUNTERS(remote_mac_map_err, i, remotel2mappingerrors, false);
        FILL_API_COUNTERS(local_ip_map_ok, i, locall3mappings, true);
        FILL_API_COUNTERS(local_ip_map_err, i, locall3mappingerrors, false);
        FILL_API_COUNTERS(remote_ip_map_ok, i, remotel3mappings, true);
        FILL_API_COUNTERS(remote_ip_map_err, i, remotel3mappingerrors, false);
    }
    return SDK_RET_OK;
}

Status
LearnSvcImpl::LearnMACGet(ServerContext *context,
                          const ::pds::LearnMACRequest* proto_req,
                          pds::LearnMACGetResponse *proto_rsp) {
    sdk_ret_t ret;
    ep_mac_key_t mac_key;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    if (proto_req->key_size() == 0) {
        // return all MACs learnt
        ret = ep_mac_get_all(proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    } else {
        for (int i = 0; i < proto_req->key_size(); i++) {
            pds_learn_mackey_proto_to_api(&mac_key, proto_req->key(i));
            ret = ep_mac_get(&mac_key, proto_rsp);
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            if (ret != SDK_RET_OK) {
                break;
            }
        }
    }
    return Status::OK;
}

Status
LearnSvcImpl::LearnIPGet(ServerContext* context,
                         const ::pds::LearnIPGetRequest* proto_req,
                         pds::LearnIPGetResponse* proto_rsp) {
    sdk_ret_t ret = SDK_RET_OK;
    ep_ip_key_t ip_key;
    pds_obj_key_t subnet;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    if (proto_req->filter_case() == pds::LearnIPGetRequest::kSubnetId) {
        // filter based on subnet IDs
        pds_obj_key_proto_to_api_spec(&subnet, proto_req->subnetid());
        ret = ep_ip_get_all(subnet, proto_rsp);
    } else if (proto_req->filter_case() == pds::LearnIPGetRequest::kKey) {
        // filter based on IPKey
        pds_learn_ipkey_proto_to_api(&ip_key, proto_req->key());
        ret = ep_ip_get(&ip_key, proto_rsp);
    } else {
        // return all IPs learnt
        ret = ep_ip_get_all(proto_rsp);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}

Status
LearnSvcImpl::LearnStatsGet(ServerContext* context,
                            const types::Empty* proto_req,
                            pds::LearnStatsGetResponse* proto_rsp) {
    sdk_ret_t ret;

    ret = ep_learn_stats_fill(proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}

Status
LearnSvcImpl::LearnMACClear(ServerContext *context,
                            const ::pds::LearnMACRequest* proto_req,
                            pds::LearnClearResponse *proto_rsp) {
    sdk_ret_t ret;
    ep_mac_key_t mac_key;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    if (proto_req->key_size() == 0) {
        // clear all MACs learnt
        learn_mac_clear(NULL);
    } else {
        for (int i = 0; i < proto_req->key_size(); i++) {
            pds_learn_mackey_proto_to_api(&mac_key, proto_req->key(i));
            ret = learn_mac_clear(&mac_key);
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            if (ret != SDK_RET_OK) {
                break;
            }
        }
    }
    return Status::OK;
}

Status
LearnSvcImpl::LearnIPClear(ServerContext* context,
                           const ::pds::LearnIPRequest* proto_req,
                           pds::LearnClearResponse* proto_rsp) {
    sdk_ret_t ret;
    ep_ip_key_t ip_key;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    if (proto_req->key_size() == 0) {
        // clear all IPs learnt
        learn_ip_clear(NULL);
    } else {
        for (int i = 0; i < proto_req->key_size(); i++) {
            pds_learn_ipkey_proto_to_api(&ip_key, proto_req->key(i));
            ret = learn_ip_clear(&ip_key);
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            if (ret != SDK_RET_OK) {
                break;
            }
        }
    }
    return Status::OK;
}

Status
LearnSvcImpl::LearnStatsClear(ServerContext* context,
                              const types::Empty* proto_req,
                              types::Empty* proto_rsp) {
    //TODO: make it an ipc later, since it is just about counters,
    // leaving it as a direct function call
    learn_db()->reset_counters();
    return Status::OK;
}
