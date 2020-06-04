/*
 *  {C} Copyright 2019 Pensando Systems Inc. All rights reserved.
 */

#include <nic/sdk/include/sdk/table.hpp>
#include <lib/table/ftl/ftl_base.hpp>
#include <nic/apollo/p4/include/defines.h>
#include <ftl_wrapper.h>

using namespace sdk;
using namespace sdk::table;
using namespace sdk::platform;

extern "C" {

typedef struct ftlv6_cache_s {
    flow_hash_entry_t flow[MAX_FLOW_ENTRIES_PER_BATCH]; 
    flow_hash_entry_t last_read_flow;
    uint32_t hash[MAX_FLOW_ENTRIES_PER_BATCH];
    flow_flags_t flags[MAX_FLOW_ENTRIES_PER_BATCH];
    uint16_t count;
} ftlv6_cache_t;

thread_local ftlv6_cache_t g_ip6_flow_cache;

void
ftlv6_set_thread_id (ftlv6 *obj, uint32_t thread_id)
{
    ftl_set_thread_id(obj, thread_id);
}

int
ftlv6_get_with_handle(ftl *obj, uint64_t handle)
{
    sdk_table_api_params_t params = {0};
    flow_hash_entry_t entry;
    uint8_t sip[16], dip[16];
    if (get_skip_ftl_program()) {
        return 0;
    }

    params.handle.tohandle(handle);
    params.entry = &entry;

    if (SDK_RET_OK != obj->get_with_handle(&params)) {
        return -1;
    }

    entry.get_key_metadata_src(sip);
    entry.get_key_metadata_dst(dip);
    ftlv6_set_key(&g_ip6_flow_cache.last_read_flow,
                  sip, dip,
                  entry.get_key_metadata_proto(),
                  entry.get_key_metadata_sport(),
                  entry.get_key_metadata_dport(),
                  ftl_get_key_lookup_id(&entry));

    return 0;
}

int 
ftlv6_remove_cached_entry(ftl *obj)
{
    return ftl_remove(obj, &g_ip6_flow_cache.last_read_flow, 0);
}

int
ftlv6_remove (ftlv6 *obj, flow_hash_entry_t *entry, uint32_t hash)
{
    return ftl_remove(obj, entry, hash);
}

int
ftlv6_clear (ftlv6 *obj, bool clear_global_state,
             bool clear_thread_local_state)
{
    return ftl_clear(obj, clear_global_state, clear_thread_local_state);
}

void
ftlv6_delete (ftlv6 *obj)
{
    ftl_delete(obj);
}

void 
ftlv6_set_key (flow_hash_entry_t *entry,
               uint8_t *sip,
               uint8_t *dip,
               uint8_t ip_proto,
               uint16_t src_port,
               uint16_t dst_port,
               uint16_t lookup_id)
{
    entry->set_key_metadata_dst(dip);
    entry->set_key_metadata_src(sip);
    entry->set_key_metadata_proto(ip_proto);
    entry->set_key_metadata_dport(dst_port);
    entry->set_key_metadata_sport(src_port);
    ftl_set_key_lookup_id(entry, lookup_id);
    entry->set_key_metadata_ktype(KEY_TYPE_IPV6);
}

int
ftlv6_dump_hw_entries (ftlv6 *obj, char *logfile, uint8_t detail)
{
    return ftl_dump_hw_entries(obj, logfile, detail, true);
}

static int
ftlv6_read_hw_entry (ftlv6 *obj, uint8_t *src, uint8_t *dst,
                     uint8_t ip_proto, uint16_t sport,
                     uint16_t dport, uint16_t lookup_id,
                     flow_hash_entry_t *entry)
{
    sdk_ret_t ret;
    sdk_table_api_params_t params = {0};
    int retcode = 0;

    entry->clear();
    ftlv6_set_key(entry, src, dst, ip_proto, sport, dport, lookup_id);
    params.entry = entry;

    ret = obj->get(&params);
    if (ret != SDK_RET_OK) {
        retcode = -1;
    }

    return retcode;
}

int
ftlv6_read_session_index (ftlv6 *obj, uint8_t *src, uint8_t *dst,
                          uint8_t ip_proto, uint16_t sport,
                          uint16_t dport, uint16_t lookup_id,
                          uint32_t *ses_id)
{
    int retcode = 0;
    flow_hash_entry_t entry;

    retcode = ftlv6_read_hw_entry(obj, src, dst, ip_proto, sport, dport,
                                  lookup_id, &entry);

    if (retcode != -1) {
        *ses_id = entry.session_index;
    }

    return retcode;
}

int
ftlv6_dump_hw_entry (ftlv6 *obj, uint8_t *src, uint8_t *dst,
                     uint8_t ip_proto, uint16_t sport,
                     uint16_t dport, uint16_t lookup_id,
                     char *buf, int max_len)
{
    int retcode = 0;
    flow_hash_entry_t entry;

    retcode = ftlv6_read_hw_entry(obj, src, dst, ip_proto, sport, dport,
                                  lookup_id, &entry);

    if (retcode != -1) {
        entry.tostr(buf, max_len);
    }
    
    return retcode;
}

void
ftlv6_init_stats_cache (void)
{
    ftl_init_stats_cache();
}

void
ftlv6_cache_stats (ftlv6 *obj, uint16_t thread_id)
{
    ftl_cache_stats(obj, thread_id);
}

void
ftlv6_dump_stats (ftlv6 *obj, char *buf, int max_len)
{
    ftl_dump_stats(obj, buf, max_len);
}

void
ftlv6_dump_stats_cache (char *buf, int max_len)
{
    ftl_dump_stats_cache(buf, max_len);
}

uint64_t
ftlv6_get_flow_count (ftlv6 *obj)
{
    return ftl_get_flow_count(obj, true);
}

void
ftlv6_cache_batch_init (void)
{
   g_ip6_flow_cache.count = 0;
}

void
ftlv6_cache_set_key (uint8_t *sip,
                     uint8_t *dip,
                     uint8_t ip_proto,
                     uint16_t src_port,
                     uint16_t dst_port,
                     uint16_t lookup_id)
{
   ftlv6_set_key(g_ip6_flow_cache.flow + g_ip6_flow_cache.count,
                 sip, dip, ip_proto, src_port, dst_port, lookup_id);
}

void
ftlv6_cache_set_nexthop (uint32_t nhid,
                         uint32_t nhtype,
                         uint8_t nh_valid,
                         uint8_t priority)
{
   ftl_set_entry_nexthop(g_ip6_flow_cache.flow + g_ip6_flow_cache.count, nhid, 
                         nhtype, nh_valid, priority);
}

int
ftlv6_cache_get_count (void)
{
    return g_ip6_flow_cache.count;
}

void
ftlv6_cache_advance_count (int val)
{
    g_ip6_flow_cache.count += val;
}

int
ftlv6_cache_program_index (ftlv6 *obj, uint16_t id, uint64_t *handle)
{
    return ftl_insert(obj, g_ip6_flow_cache.flow + id,
                      g_ip6_flow_cache.hash[id], handle,
                      g_ip6_flow_cache.flags[id].update);
}

int
ftlv6_cache_log_session(uint16_t iid, uint16_t rid, uint8_t reason)
{
    if (!g_ip6_flow_cache.flags[iid].log) {
        return 0;
    }
    // rid not logged, since it's iid reversed
    return ftl_export_with_entry(g_ip6_flow_cache.flow + iid, reason,
                                 g_ip6_flow_cache.flags[iid].drop);
}

int
ftlv6_cache_delete_index (ftlv6 *obj, uint16_t id)
{
    return ftlv6_remove(obj, g_ip6_flow_cache.flow + id,
                        g_ip6_flow_cache.hash[id]);
}

void
ftlv6_cache_set_session_index (uint32_t val)
{
    ftl_set_session_index(g_ip6_flow_cache.flow + g_ip6_flow_cache.count, val);
}

void
ftlv6_cache_set_flow_role(uint8_t flow_role)
{
    ftl_set_flow_role(g_ip6_flow_cache.flow + g_ip6_flow_cache.count, flow_role);
}

uint32_t
ftlv6_cache_get_session_index (int id)
{
    return ftl_get_session_id(g_ip6_flow_cache.flow + id);
}

uint8_t
ftlv6_cache_get_proto(int id)
{
    return ftl_get_proto(g_ip6_flow_cache.flow + id);
}

void
ftlv6_cache_set_epoch (uint8_t val)
{
    ftl_set_epoch(g_ip6_flow_cache.flow + g_ip6_flow_cache.count, val);
}

void
ftlv6_cache_set_l2l (uint8_t val)
{
    ftl_set_entry_l2l(g_ip6_flow_cache.flow + g_ip6_flow_cache.count, val);
}

void
ftlv6_cache_set_hash_log (uint32_t val, uint8_t log)
{
    g_ip6_flow_cache.hash[g_ip6_flow_cache.count] = val;
    g_ip6_flow_cache.flags[g_ip6_flow_cache.count].log = log;
}

void
ftlv6_cache_set_update_flag (uint8_t update)
{
    g_ip6_flow_cache.flags[g_ip6_flow_cache.count].update = update;
}

void
ftlv6_cache_set_flow_miss_hit (uint8_t val)
{
    ftl_set_entry_flow_miss_hit(g_ip6_flow_cache.flow + g_ip6_flow_cache.count, val);
}

void
ftlv6_cache_set_counter_index (uint8_t ctr_idx)
{
    g_ip6_flow_cache.flags[g_ip6_flow_cache.count].ctr_idx = ctr_idx;
}

uint8_t
ftlv6_cache_get_counter_index (int id)
{
    return g_ip6_flow_cache.flags[id].ctr_idx;
}

/*
void
ftlv6_get_last_read_session_info (uint8_t *sip, uint8_t *dip, uint16_t *sport,
                                  uint16_t *dport, uint16_t *lkp_id)
{
    flow_hash_entry_t entry = g_ip6_flow_cache.last_read_flow;
    entry.get_key_metadata_src(sip);
    entry.get_key_metadata_dst(dip);
    *sport = entry.get_key_metadata_sport();
    *dport = entry.get_key_metadata_dport();
    *lkp_id = ftl_get_key_lookup_id(&entry);
}

void
ftlv6_cache_batch_flush (ftlv6 *obj, int *status)
{
    int i;
    uint32_t pindex, sindex;

    for (i = 0; i < g_ip6_flow_cache.count; i++) {
       status[i] = ftl_insert(obj, g_ip6_flow_cache.flow + i,
                              g_ip6_flow_cache.hash[i],
                              &pindex, &sindex,
                              g_ip6_flow_cache.flags[i].log,
                              g_ip6_flow_cache.flags[i].update);
    }
}
*/

}
