/*
 *  {C} Copyright 2019 Pensando Systems Inc. All rights reserved.
 */

#include <stdint.h>
#include <cstddef>
#include <cstring>

#include <nic/sdk/include/sdk/table.hpp>
#include <lib/table/ftl/ftl_base.hpp>
#include <nic/sdk/lib/p4/p4_api.hpp>
#include <nic/sdk/platform/capri/capri_tbl_rw.hpp>
#include <nic/sdk/lib/p4/p4_utils.hpp>
#include "gen/p4gen/p4/include/ftl_table.hpp"
#include "nic/vpp/infra/operd/flow_export.h"
#include <pd_utils.h>
#include <ftl_wrapper.h>
#include <sess.h>
#include "nic/operd/decoders/vpp/flow_decoder.h"
#include <nat.h>
#include <pds_table.h>

#define VPP_FTLV4_MAX_THREADS 4

using namespace sdk;
using namespace sdk::table;
using namespace sdk::platform;

extern "C" {

extern session_update_cb g_ses_cb;
extern nat_flow_dealloc_cb g_nat_dealloc_cb;


typedef struct ftlv4_cache_s {
    ipv4_flow_hash_entry_t ip4_flow[MAX_FLOW_ENTRIES_PER_BATCH];
    ipv4_flow_hash_entry_t ip4_last_read_flow;
    ipv4_flow_hash_entry_t ip4_last_read_nat_session;
    uint32_t ip4_hash[MAX_FLOW_ENTRIES_PER_BATCH];
    flow_flags_t flags[MAX_FLOW_ENTRIES_PER_BATCH];
    uint16_t count;
} ftlv4_cache_t;

ftlv4_cache_t g_ip4_flow_cache[VPP_FTLV4_MAX_THREADS];

ftlv4 *
ftlv4_create (void *key2str,
              void *appdata2str)
{
    sdk_table_factory_params_t factory_params = {0};

    factory_params.key2str = (key2str_t) (key2str);
    factory_params.appdata2str = (appdata2str_t) (appdata2str);

    return ipv4_flow_hash::factory(&factory_params);
}

static int
ftlv4_insert (ftlv4 *obj, ipv4_flow_hash_entry_t *entry, uint32_t hash,
              uint64_t *handle, uint8_t update, uint16_t thread_id)
{
    sdk_table_api_params_t params = {0};

    if (get_skip_ftl_program()) {
        return 0;
    }

    if (hash) {
        params.hash_32b = hash;
        params.hash_valid = 1;
    }

    params.thread_id = thread_id;
    params.entry = entry;
    if (unlikely(update)) {
        if (SDK_RET_OK != obj->update(&params)) {
            return -1;
        }
        goto done;
    }

    if (SDK_RET_OK != obj->insert(&params)) {
        return -1;
    }

done:
    *handle = params.handle.tou64();
    return 0;
}

static void
ftlv4_move_cb (base_table_entry_t *entry, handle_t old_handle,
               handle_t new_handle)
{
    ipv4_flow_hash_entry_t *v4entry = (ipv4_flow_hash_entry_t *)entry;
    uint32_t ses_id = v4entry->get_session_index();

    if (v4entry->get_flow_role() == TCP_FLOW_INITIATOR) {
        g_ses_cb (ses_id, new_handle.tou64(), true, true);
    } else {
        g_ses_cb (ses_id, new_handle.tou64(), false, true);
    }
}

int
ftlv4_remove (ftlv4 *obj, ipv4_flow_hash_entry_t *entry,
              uint32_t hash, uint16_t thread_id)
{
    sdk_table_api_params_t params = {0};

    if (get_skip_ftl_program()) {
        return 0;
    }

    if (hash) {
        params.hash_32b = hash;
        params.hash_valid = 1;
    }
    params.thread_id = thread_id;
    params.movecb = ftlv4_move_cb;
    params.entry = entry;
    if (SDK_RET_OK != obj->remove(&params)) {
        return -1;
    }
    return 0;
}

int
ftlv4_get_with_handle (ftlv4 *obj, uint64_t handle,
                       uint16_t thread_id)
{
    sdk_table_api_params_t params = {0};
    ipv4_flow_hash_entry_t *v4entry =
        &g_ip4_flow_cache[thread_id].ip4_last_read_flow;

    v4entry->clear();

    if (get_skip_ftl_program()) {
        return 0;
    }

    params.thread_id = thread_id;
    params.handle.tohandle(handle);
    params.entry = v4entry;

    FTL_RETRY_API(obj->get_with_handle, &params, FTL_MAX_API_RETRY_COUNT, 0);

    return 0;
}

int
ftlv4_update_cached_entry (ftlv4 *obj, uint16_t thread_id)
{
    uint64_t handle;
    return ftlv4_insert(obj, &g_ip4_flow_cache[thread_id].ip4_last_read_flow,
                        0, &handle, 1, thread_id);
}

int
ftlv4_remove_cached_entry (ftlv4 *obj, uint16_t thread_id)
{
    return ftlv4_remove(obj,
                        &g_ip4_flow_cache[thread_id].ip4_last_read_flow,
                        0, thread_id);
}

void
ftlv4_update_iflow_nat_session (ftlv4 *obj, uint16_t thread_id)
{
    ipv4_flow_hash_entry_t *v4entry =
        &g_ip4_flow_cache[thread_id].ip4_last_read_flow;
    ipv4_flow_hash_entry_t *v4natentry =
        &g_ip4_flow_cache[thread_id].ip4_last_read_nat_session;
    v4natentry->set_key_metadata_ipv4_dst(v4entry->get_key_metadata_ipv4_dst());
    v4natentry->set_key_metadata_dport(v4entry->get_key_metadata_dport());
    v4natentry->set_key_metadata_proto(v4entry->get_key_metadata_proto());
    if (unlikely(v4entry->get_key_metadata_proto() == IP_PROTO_ICMP)) {
        v4natentry->set_key_metadata_sport(v4entry->get_key_metadata_sport());
        v4natentry->set_key_metadata_dport(0);
    }
}

void
ftlv4_update_rflow_nat_session (ftlv4 *obj, uint16_t thread_id)
{
    ipv4_flow_hash_entry_t *v4entry =
                        &g_ip4_flow_cache[thread_id].ip4_last_read_flow;
    ipv4_flow_hash_entry_t *v4natentry =
                        &g_ip4_flow_cache[thread_id].ip4_last_read_nat_session;
    v4natentry->set_key_metadata_ipv4_src(v4entry->get_key_metadata_ipv4_dst());
    if (likely(v4entry->get_key_metadata_proto() != IP_PROTO_ICMP)) {
        v4natentry->set_key_metadata_sport(v4entry->get_key_metadata_dport());
    }
}

int
ftlv4_remove_nat_session (uint32_t vpc_id, ftlv4 *obj, uint16_t thread_id)
{
    uint32_t sip, dip;
    uint16_t sport, dport;
    uint8_t proto;

    ipv4_flow_hash_entry_t *v4entry =
                        &g_ip4_flow_cache[thread_id].ip4_last_read_nat_session;
    sip = v4entry->get_key_metadata_ipv4_src();
    dip = v4entry->get_key_metadata_ipv4_dst();
    sport = v4entry->get_key_metadata_sport();
    dport = v4entry->get_key_metadata_dport();
    proto = v4entry->get_key_metadata_proto();
    if (g_nat_dealloc_cb) {
        return g_nat_dealloc_cb(vpc_id, dip, dport, proto, sip, sport);
    }

    return 0;
}

static inline sdk::sdk_ret_t
ftlv4_read_with_handle (ftlv4 *obj, uint64_t handle,
                        ipv4_flow_hash_entry_t &entry, uint16_t thread_id)
{
    sdk::sdk_ret_t ret = SDK_RET_OK;
    sdk_table_api_params_t params = {0};

    params.thread_id = thread_id;
    params.handle.tohandle(handle);
    params.entry = &entry;

    FTL_RETRY_API_WITH_RET(obj->get_with_handle, &params, 
                           FTL_MAX_API_RETRY_COUNT, ret);
 
    return ret;
}

int
ftlv4_dump_entry_with_handle (ftlv4 *obj, uint64_t handle,
                              v4_flow_info_t *flow_info, uint16_t thread_id)
{
    sdk::sdk_ret_t ret;
    ipv4_flow_hash_entry_t entry;

    ret = ftlv4_read_with_handle(obj, handle, entry, thread_id);
    if (ret != SDK_RET_OK) {
        return -1;
    }

    flow_info->key_metadata_ipv4_src = entry.key_metadata_ipv4_src;
    flow_info->key_metadata_ipv4_dst = entry.key_metadata_ipv4_dst;
    flow_info->key_metadata_flow_lkp_id = ftlv4_get_key_lookup_id(&entry);
    flow_info->key_metadata_dport = entry.key_metadata_dport;
    flow_info->key_metadata_sport = entry.key_metadata_sport;
    flow_info->key_metadata_proto = entry.key_metadata_proto;
    flow_info->epoch = ftlv4_get_entry_epoch(&entry);
    flow_info->session_index = entry.session_index;
    flow_info->nexthop_valid = ftlv4_get_entry_nexthop_valid(&entry);
    flow_info->nexthop_type = ftlv4_get_entry_nexthop_type(&entry);
    flow_info->nexthop_id = ftlv4_get_entry_nexthop_id(&entry);
    flow_info->nexthop_priority = ftlv4_get_entry_nexthop_priority(&entry);
    return 0;
}

static inline int
ftlv4_read_snat_info (uint32_t session_id, bool host_origin,
                      const operd_flow_key_v4_t &flow_key,
                      operd_flow_nat_data_t &nat_data)
{
    uint16_t rx_xlate_id, tx_xlate_id;
    uint16_t rx_xlate_id2, tx_xlate_id2;
    int ret;

    pds_session_get_xlate_ids(session_id, &rx_xlate_id, &tx_xlate_id,
                               &rx_xlate_id2, &tx_xlate_id2);

    if (host_origin) {
        if (tx_xlate_id) {
            ret = pds_snat_tbl_read_ip4(tx_xlate_id, &nat_data.src_nat_addr,
                                        &nat_data.src_nat_port);
            if (ret != 0) {
                return -1;
            }
        }

        if (tx_xlate_id2) {
            ret = pds_snat_tbl_read_ip4(tx_xlate_id2, &nat_data.dst_nat_addr,
                                        &nat_data.dst_nat_port);
            if (ret != 0) {
                return -1;
            }
        }
    } else {
        if (rx_xlate_id) {
            ret = pds_snat_tbl_read_ip4(rx_xlate_id, &nat_data.dst_nat_addr,
                                        &nat_data.dst_nat_port);
            if (ret != 0) {
                return -1;
            }
        }

        if (rx_xlate_id2) {
            ret = pds_snat_tbl_read_ip4(rx_xlate_id2, &nat_data.src_nat_addr,
                                        &nat_data.src_nat_port);
            if (ret != 0) {
                return -1;
            }
        }
    }
    return 0;
}

int
ftlv4_export_with_entry (ipv4_flow_hash_entry_t *iv4entry,
                         ipv4_flow_hash_entry_t *rv4entry,
                         uint8_t reason, bool host_origin,
                         bool drop)
{
    operd_flow_t *flow;
    pds_session_stats_t session_stats;

    flow = pds_operd_get_flow_ip4();

    if (unlikely(flow == NULL)) {
        return -1;
    }

    flow->v4.src = iv4entry->get_key_metadata_ipv4_src();
    flow->v4.sport = iv4entry->get_key_metadata_sport();
    flow->v4.dst = rv4entry->get_key_metadata_ipv4_src();
    flow->v4.dport = rv4entry->get_key_metadata_sport();
    flow->v4.proto = iv4entry->get_key_metadata_proto();
    flow->v4.lookup_id = ftlv4_get_key_lookup_id(iv4entry);

    flow->session_id = iv4entry->get_session_index();

    if (0 != ftlv4_read_snat_info(flow->session_id, host_origin, flow->v4,
                                  flow->nat_data)) {
        pds_operd_commit_flow_ip4(flow);
        return -1;
    }

    if (FLOW_EXPORT_REASON_ADD != reason) {
        // don't read stats for add. they would be zero anyway
        if (0 != pds_session_stats_read(flow->session_id, &session_stats)) {
            pds_operd_commit_flow_ip4(flow);
            return -1;
        }
        memcpy(&flow->stats, &session_stats, sizeof(pds_session_stats_t));
    }

    switch(reason) {
    case FLOW_EXPORT_REASON_ADD:
        flow->logtype = OPERD_FLOW_LOGTYPE_ADD;
        break;
    case FLOW_EXPORT_REASON_DEL:
        flow->logtype = OPERD_FLOW_LOGTYPE_DEL;
        break;
    default:
    case FLOW_EXPORT_REASON_ACTIVE:
        flow->logtype = OPERD_FLOW_LOGTYPE_ACTIVE;
        break;
    }

    flow->type = OPERD_FLOW_TYPE_IP4;
    flow->action = drop ? OPERD_FLOW_ACTION_DENY : OPERD_FLOW_ACTION_ALLOW;

    pds_operd_commit_flow_ip4(flow);

    return 0;
}

int
ftlv4_get_flow_entry (ftlv4 *obj, uint64_t flow_handle,
                      v4_flow_entry *entry, uint16_t thread_id)
{
    return ftlv4_read_with_handle(obj, flow_handle, *entry,
                                  thread_id);
}

int
ftlv4_export_with_handle (ftlv4 *obj, uint64_t iflow_handle,
                          uint64_t rflow_handle,
                          uint8_t reason, bool host_origin,
                          bool drop, uint16_t thread_id)
{
    ipv4_flow_hash_entry_t iv4entry, rv4entry;
    sdk_ret_t ret;

    if (get_skip_ftl_program()) {
        return 0;
    }

    ret = ftlv4_read_with_handle(obj, iflow_handle, iv4entry, thread_id);
    if (SDK_RET_OK != ret) {
        return -1;
    }
    ret = ftlv4_read_with_handle(obj, rflow_handle, rv4entry, thread_id);
    if (SDK_RET_OK != ret) {
        return -1;
    }

    ftlv4_export_with_entry(&iv4entry, &rv4entry, reason, host_origin, drop);

    return 0;
}

int
ftlv4_clear (ftlv4 *obj, bool clear_global_state,
             bool clear_thread_local_state, uint16_t thread_id)
{
    sdk_table_api_params_t params = {0};

    params.thread_id = thread_id;
    if (SDK_RET_OK != obj->clear(clear_global_state,
                                 clear_thread_local_state,
                                 &params)) {
        return -1;
    }
    return 0;
}

void
ftlv4_delete (ftlv4 *obj)
{
    ftlv4::destroy(obj);
}

void
ftlv4_set_key (ipv4_flow_hash_entry_t *entry,
               uint32_t sip,
               uint32_t dip,
               uint8_t ip_proto,
               uint16_t src_port,
               uint16_t dst_port,
               uint16_t lookup_id)
{
    entry->set_key_metadata_ipv4_dst(dip);
    entry->set_key_metadata_ipv4_src(sip);
    entry->set_key_metadata_proto(ip_proto);
    entry->set_key_metadata_dport(dst_port);
    entry->set_key_metadata_sport(src_port);
    ftlv4_set_key_lookup_id(entry, lookup_id);
}

uint32_t ftlv4_entry_count;

static bool
ftlv4_dump_hw_entry_iter_cb (sdk_table_api_params_t *params)
{
    ipv4_flow_hash_entry_t *hwentry =  (ipv4_flow_hash_entry_t *) params->entry;
    FILE *fp = (FILE *)params->cbdata;
    char buf[FTL_ENTRY_STR_MAX];

    if (hwentry->entry_valid) {
        ftlv4_entry_count++;
        buf[FTL_ENTRY_STR_MAX - 1] = 0;
        hwentry->key2str(buf, FTL_ENTRY_STR_MAX - 1);
        fprintf(fp, "%s\n", buf);
    }
    return false;
}

static bool
ftlv4_dump_hw_entry_detail_iter_cb (sdk_table_api_params_t *params)
{
    ipv4_flow_hash_entry_t *hwentry =  (ipv4_flow_hash_entry_t *) params->entry;
    FILE *fp = (FILE *)params->cbdata;
    uint8_t *entry;
    uint32_t size;
    char buf[FTL_ENTRY_STR_MAX];

    if (hwentry->entry_valid) {
        ftlv4_entry_count++;
        buf[FTL_ENTRY_STR_MAX - 1] = 0;
        hwentry->tostr(buf, FTL_ENTRY_STR_MAX - 1);
        fprintf(fp, "%s\n", buf);
        session_get_addr(hwentry->get_session_index(), &entry, &size);
        fprintf(fp, " Session data: ");
        for (uint32_t i = 0; i < size; i++) {
            fprintf(fp, "%02x", entry[i]);
        }
        fprintf(fp, "\n");
        uint32_t ses = hwentry->get_session_index();
        fprintf(fp, "Timestamp %lu addr 0x%p data %lu start 0x%p ses %u\n",
                pds_session_get_timestamp(ses), entry + 36,
                (uint64_t)((uint64_t *) (entry + 36)), entry, ses);
        fprintf(fp, "\n");
    }
    return false;
}

int
ftlv4_dump_hw_entries (ftlv4 *obj, char *logfile,
                       uint8_t detail, uint16_t thread_id)
{
    sdk_ret_t ret;
    sdk_table_api_params_t params = {0};
    FILE *logfp = fopen(logfile, "a");
    int retcode = -1;
    char buf[FTL_ENTRY_STR_MAX];

    if (logfp == NULL) {
        goto end;
    }

    buf[FTL_ENTRY_STR_MAX - 1] = 0;

    params.itercb = detail ?
                    ftlv4_dump_hw_entry_detail_iter_cb :
                    ftlv4_dump_hw_entry_iter_cb;
    params.cbdata = logfp;
    params.force_hwread = false;
    params.thread_id = thread_id;
    ftlv4_entry_count = 0;

    if (!detail) {
        // ipv4_flow_hash_entry_t::keyheader2str(buf, FTL_ENTRY_STR_MAX - 1);
        // fprintf(logfp, "%s", buf);
    }

    ret = obj->iterate(&params);
    if (ret != SDK_RET_OK) {
        retcode = -1;
    } else {
        retcode = ftlv4_entry_count;
    }

    if (!detail) {
        fprintf(logfp, "\n%s", buf);
    }
    fclose(logfp);

end:
    return retcode;
}

static int
ftlv4_read_hw_entry (ftlv4 *obj, uint32_t src, uint32_t dst,
                     uint8_t ip_proto, uint16_t sport,
                     uint16_t dport, uint16_t lookup_id,
                     ipv4_flow_hash_entry_t *entry,
                     uint16_t thread_id)
{
    sdk_ret_t ret;
    sdk_table_api_params_t params = {0};
    int retcode = 0;

    entry->clear();
    ftlv4_set_key(entry, src, dst, ip_proto, sport, dport, lookup_id);
    params.entry = entry;
    params.thread_id = thread_id;

    ret = obj->get(&params);
    if (ret != SDK_RET_OK) {
        retcode = -1;
    }

    return retcode;
}

int
ftlv4_read_session_index (ftlv4 *obj, uint32_t src, uint32_t dst,
                          uint8_t ip_proto, uint16_t sport,
                          uint16_t dport, uint16_t lookup_id,
                          uint32_t *ses_id, uint16_t thread_id)
{
    int retcode = 0;
    ipv4_flow_hash_entry_t entry;

    retcode = ftlv4_read_hw_entry(obj, src, dst, ip_proto, sport, dport,
                                  lookup_id, &entry, thread_id);

    if (retcode != -1) {
        *ses_id = entry.session_index;
    }
    return retcode;
}

int
ftlv4_dump_hw_entry (ftlv4 *obj, uint32_t src, uint32_t dst,
                     uint8_t ip_proto, uint16_t sport,
                     uint16_t dport, uint16_t lookup_id,
                     char *buf, int max_len, uint16_t thread_id)
{
    int retcode = 0;
    ipv4_flow_hash_entry_t entry;

    retcode = ftlv4_read_hw_entry(obj, src, dst, ip_proto, sport, dport,
                                  lookup_id, &entry, thread_id);
    if (retcode != -1) {
        entry.tostr(buf, max_len);
    }
    return retcode;
}

void
ftlv4_init_stats_cache (void)
{
    ftl_init_stats_cache();
}

void
ftlv4_cache_stats (ftlv4 *obj, uint16_t thread_id)
{
    ftl_cache_stats(obj, thread_id);
}

void
ftlv4_dump_stats (ftlv4 *obj, char *buf, int max_len)
{
    ftl_dump_stats(obj, buf, max_len);
}

void
ftlv4_dump_stats_cache (char *buf, int max_len)
{
    ftl_dump_stats_cache(buf, max_len);
}

static bool
ftlv4_hw_entry_count_cb (sdk_table_api_params_t *params)
{
    ipv4_flow_hash_entry_t *hwentry =  (ipv4_flow_hash_entry_t *) params->entry;

    if (hwentry->entry_valid) {
        uint64_t *count = (uint64_t *)params->cbdata;
        (*count)++;
    }
    return false;
}

uint64_t
ftlv4_get_flow_count (ftlv4 *obj, uint16_t thread_id)
{
    sdk_ret_t ret;
    sdk_table_api_params_t params = {0};
    uint64_t count = 0;

    params.itercb = ftlv4_hw_entry_count_cb;
    params.cbdata = &count;
    params.force_hwread = false;
    params.thread_id = thread_id;

    ret = obj->iterate(&params);
    if (ret != SDK_RET_OK) {
        count = ~0L;
    }

    return count;
}

static void
ftlv4_set_session_index (ipv4_flow_hash_entry_t *entry, uint32_t session)
{
    entry->set_session_index(session);
}

static void
ftlv4_set_epoch (ipv4_flow_hash_entry_t *entry, uint8_t val)
{
    entry->set_epoch(val);
}

void
ftlv4_set_flow_role(ipv4_flow_hash_entry_t *entry, uint8_t flow_role)
{
    entry->set_flow_role(flow_role);
}

void
ftlv4_cache_set_hash_log (uint32_t val, uint8_t log, uint16_t tid)
{
    g_ip4_flow_cache[tid].ip4_hash[g_ip4_flow_cache[tid].count] = val;
    g_ip4_flow_cache[tid].flags[g_ip4_flow_cache[tid].count].log = log;
}

void
ftlv4_cache_set_update_flag (uint8_t update, uint16_t tid)
{
    g_ip4_flow_cache[tid].flags[g_ip4_flow_cache[tid].count].update = update;
}

void
ftlv4_cache_set_napt_flag (uint8_t napt, uint16_t tid)
{
    g_ip4_flow_cache[tid].flags[g_ip4_flow_cache[tid].count].napt = napt;
}

uint8_t
ftlv4_cache_get_napt_flag (int id, uint16_t thread_id)
{
    return g_ip4_flow_cache[thread_id].flags[id].napt;
}

void
ftlv4_cache_set_host_origin (uint8_t host_origin, uint16_t tid)
{
    g_ip4_flow_cache[tid].flags[g_ip4_flow_cache[tid].count].host_origin =
                                                               host_origin & 1;
}

void
ftlv4_cache_set_drop (uint8_t drop, uint16_t tid)
{
    g_ip4_flow_cache[tid].flags[g_ip4_flow_cache[tid].count].drop =
                                                               drop & 1;
}

void
ftlv4_cache_set_flow_miss_hit (uint8_t val, uint16_t thread_id)
{
    ftlv4_set_entry_flow_miss_hit(g_ip4_flow_cache[thread_id].ip4_flow + g_ip4_flow_cache[thread_id].count,
                                  val);
}

void
ftlv4_cache_set_flow_role(uint8_t flow_role, uint16_t thread_id)
{
    ftlv4_set_flow_role(g_ip4_flow_cache[thread_id].ip4_flow + g_ip4_flow_cache[thread_id].count,
                        flow_role);
}

void
ftlv4_cache_set_counter_index (uint8_t ctr_idx, uint16_t tid)
{
    g_ip4_flow_cache[tid].flags[g_ip4_flow_cache[tid].count].ctr_idx = ctr_idx;
}

uint8_t
ftlv4_cache_get_counter_index (int id, uint16_t thread_id)
{
    return g_ip4_flow_cache[thread_id].flags[id].ctr_idx;
}

static uint32_t
ftlv4_get_session_id (ipv4_flow_hash_entry_t *entry)
{
    return entry->get_session_index();
}

static uint8_t
ftlv4_get_proto(ipv4_flow_hash_entry_t *entry)
{
    return entry->get_key_metadata_proto();
}

void
ftlv4_cache_batch_init (uint16_t thread_id)
{
   g_ip4_flow_cache[thread_id].count = 0;
}

void
ftlv4_cache_set_key (uint32_t sip,
                     uint32_t dip,
                     uint8_t ip_proto,
                     uint16_t src_port,
                     uint16_t dst_port,
                     uint16_t lookup_id,
                     uint16_t tid)
{
   ftlv4_set_key(g_ip4_flow_cache[tid].ip4_flow + g_ip4_flow_cache[tid].count,
                 sip, dip, ip_proto, src_port, dst_port, lookup_id);
}

void
ftlv4_cache_set_nexthop (uint32_t nhid,
                         uint32_t nhtype,
                         uint8_t nh_valid,
                         uint8_t priority,
                         uint16_t tid)
{
   ftlv4_set_entry_nexthop(g_ip4_flow_cache[tid].ip4_flow + g_ip4_flow_cache[tid].count,
                           nhid, nhtype, nh_valid, priority);
}

int
ftlv4_cache_get_count (uint16_t thread_id)
{
    return g_ip4_flow_cache[thread_id].count;
}

void
ftlv4_cache_advance_count (int val, uint16_t thread_id)
{
    g_ip4_flow_cache[thread_id].count += val;
}

int
ftlv4_cache_program_index (ftlv4 *obj, uint16_t id, uint64_t *handle,
                           uint16_t thread_id)
{
    return ftlv4_insert(obj, g_ip4_flow_cache[thread_id].ip4_flow + id,
                        g_ip4_flow_cache[thread_id].ip4_hash[id],
                        handle,
                        g_ip4_flow_cache[thread_id].flags[id].update,
                        thread_id);
}

int
ftlv4_cache_delete_index (ftlv4 *obj, uint16_t id, uint16_t thread_id)
{
    return ftlv4_remove(obj, g_ip4_flow_cache[thread_id].ip4_flow + id,
                        g_ip4_flow_cache[thread_id].ip4_hash[id], thread_id);
}

void
ftlv4_cache_set_session_index (uint32_t val, uint16_t thread_id)
{
    ftlv4_set_session_index(g_ip4_flow_cache[thread_id].ip4_flow + g_ip4_flow_cache[thread_id].count, val);
}

uint32_t
ftlv4_cache_get_session_index (int id, uint16_t thread_id)
{
    return ftlv4_get_session_id(g_ip4_flow_cache[thread_id].ip4_flow + id);
}

uint8_t
ftlv4_cache_get_proto(int id, uint16_t thread_id)
{
    return ftlv4_get_proto(g_ip4_flow_cache[thread_id].ip4_flow + id);
}

int
ftlv4_cache_log_session(uint16_t iid, uint16_t rid, uint8_t reason, uint16_t tid)
{
    if ((!g_ip4_flow_cache[tid].flags[iid].log) &&
        (!g_ip4_flow_cache[tid].flags[rid].log)) {
        return 0;
    }
    return ftlv4_export_with_entry(g_ip4_flow_cache[tid].ip4_flow + iid,
                                   g_ip4_flow_cache[tid].ip4_flow + rid, reason,
                                   g_ip4_flow_cache[tid].flags[iid].host_origin,
                                   g_ip4_flow_cache[tid].flags[iid].drop);
}

void
ftlv4_cache_set_epoch (uint8_t val, uint16_t thread_id)
{
    ftlv4_set_epoch(g_ip4_flow_cache[thread_id].ip4_flow + g_ip4_flow_cache[thread_id].count, val);
}

void
ftlv4_cache_set_l2l (uint8_t val, uint16_t thread_id)
{
    ftlv4_set_entry_l2l(g_ip4_flow_cache[thread_id].ip4_flow + g_ip4_flow_cache[thread_id].count, val);
}

/*
void
ftlv4_cache_batch_flush (ftlv4 *obj, int *status)
{
    int i;
    uint32_t pindex, sindex;

    for (i = 0; i < g_ip4_flow_cache[thread_id].count; i++) {
       status[i] = ftlv4_insert(obj, g_ip4_flow_cache[thread_id].ip4_flow + i,
                                g_ip4_flow_cache[thread_id].ip4_hash[i],
                                &pindex, &sindex,
                                g_ip4_flow_cache[thread_id].flags[i].log,
                                g_ip4_flow_cache[thread_id].flags[i].update);
    }
}
*/

void
ftlv4_get_last_read_session_info (uint32_t *sip,
                                  uint32_t *dip,
                                  uint16_t *sport,
                                  uint16_t *dport,
                                  uint16_t *lkd_id,
                                  uint16_t tid)
{
    ipv4_flow_hash_entry_t *v4entry = &g_ip4_flow_cache[tid].ip4_last_read_flow;
    *sip = v4entry->get_key_metadata_ipv4_src();
    *dip = v4entry->get_key_metadata_ipv4_dst();
    *sport = v4entry->get_key_metadata_sport();
    *dport = v4entry->get_key_metadata_dport();
    *lkd_id = ftlv4_get_key_lookup_id(v4entry);
}

void
ftlv4_set_last_read_entry_epoch (uint8_t epoch, uint16_t tid)
{
    ipv4_flow_hash_entry_t *v4entry = &g_ip4_flow_cache[tid].ip4_last_read_flow;
    ftlv4_set_epoch(v4entry, epoch);
}

void
ftlv4_get_last_read_entry_epoch (uint8_t *epoch, uint16_t tid)
{
    ipv4_flow_hash_entry_t *v4entry = &g_ip4_flow_cache[tid].ip4_last_read_flow;
    *epoch = ftlv4_get_entry_epoch(v4entry);
}

void
ftlv4_get_last_read_entry_lookup_id (uint16_t *lkp_id, uint16_t tid)
{
    ipv4_flow_hash_entry_t *v4entry = &g_ip4_flow_cache[tid].ip4_last_read_flow;
    *lkp_id = ftlv4_get_key_lookup_id(v4entry);
}

void
ftlv4_set_last_read_entry_nexthop (uint32_t nhid,
                                   uint32_t nhtype,
                                   uint8_t nh_valid,
                                   uint8_t priority,
                                   uint16_t tid)
{
   ipv4_flow_hash_entry_t *v4entry = &g_ip4_flow_cache[tid].ip4_last_read_flow;
   ftlv4_set_entry_nexthop(v4entry, nhid, nhtype, nh_valid, priority);
}

void
ftlv4_set_last_read_entry_miss_hit (uint8_t flow_miss, uint16_t tid)
{
    ipv4_flow_hash_entry_t *v4entry = &g_ip4_flow_cache[tid].ip4_last_read_flow;
    ftlv4_set_entry_flow_miss_hit(v4entry, flow_miss);
}

void
ftlv4_set_last_read_entry_l2l (uint8_t l2l, uint16_t tid)
{
    ipv4_flow_hash_entry_t *v4entry = &g_ip4_flow_cache[tid].ip4_last_read_flow;
    ftlv4_set_entry_l2l(v4entry, l2l);
}

void
ftlv4_set_thread_id (ftlv4 *obj, uint32_t thread_id)
{
    //obj->set_thread_id(thread_id);
    return;
}

int
ftlv4_insert_with_new_lookup_id (ftlv4 *obj, uint64_t handle,
                                 uint64_t *ret_handle,
                                 uint16_t lookup_id,
                                 bool l2l)
{
    sdk_table_api_params_t params = {0};
    ipv4_flow_hash_entry_t v4entry;

    if (get_skip_ftl_program()) {
        return 0;
    }

    params.handle.tohandle(handle);
    params.entry = &v4entry;

    FTL_RETRY_API(obj->get_with_handle, &params, FTL_MAX_API_RETRY_COUNT, 0);

    memset(&params, 0, sizeof(params));
    ftlv4_set_key_lookup_id(&v4entry, lookup_id);
    ftlv4_set_entry_l2l(&v4entry, l2l);
    params.entry = &v4entry;
    if (SDK_RET_OK != obj->insert(&params)) {
        return -1;
    }
    *ret_handle = params.handle.tou64();

    return 0;
}

void
ftlv4_get_handle_str (char *handle_str, uint64_t handle)
{
    handle_t ftl_handle = (handle_t)handle;

    if (!ftl_handle.valid()) {
        sprintf(handle_str, "Invalid handle");
        return;
    }

    sprintf(handle_str, "%s : %d %s : %d %s : %d",
            "primary",
            ftl_handle.pindex(),
            "secondary",
            ftl_handle.svalid() ? ftl_handle.sindex() : -1,
            "epoch",
            ftl_handle.epoch());
    return;
}

}
