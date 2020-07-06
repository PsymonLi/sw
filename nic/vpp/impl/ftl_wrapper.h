//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
// Hardware Flow Programming interface

#ifndef __VPP_IMPL_FTL_WRAPPER_H__
#define __VPP_IMPL_FTL_WRAPPER_H__

typedef struct ftl_base ftl;
typedef struct ftl_base ftlv4;
typedef struct ftl_base ftlv6;
typedef struct ftl_base ftll2;
typedef struct ipv4_flow_hash_entry_t v4_flow_entry;
typedef struct flow_hash_entry_t flow_entry;

typedef char* (*key2str_t)(void *key);
typedef char* (*appdata2str_t)(void *data);
typedef void (*session_update_cb) (uint32_t ses_id,
                                   uint64_t new_handle64,
                                   bool iflow,
                                   bool lock);
typedef int (*nat_flow_dealloc_cb) (uint32_t vpc_id, uint32_t dip, uint16_t dport,
             uint8_t protocol, uint32_t sip, uint16_t sport);

#ifdef __cplusplus

#include "gen/p4gen/p4/include/ftl.h"
#include <ftl_error.h>

extern "C" {
#endif

#define FTL_ENTRY_STR_MAX   2048
#define FTL_MAX_API_RETRY_COUNT 3

#define FTL_RETRY_API(_func, _args, _retry_count, _continue) \
do {                                                         \
    int loop = _retry_count;                                 \
    sdk::sdk_ret_t retval;                                   \
    retval = _func(_args);                                   \
    if (retval != SDK_RET_OK) {                              \
        while ((retval == SDK_RET_RETRY) && loop) {          \
            retval = _func(_args);                           \
            loop--;                                          \
        }                                                    \
        if (retval != SDK_RET_OK) {                          \
            if (!_continue) {                                \
                if (retval == SDK_RET_RETRY)                 \
                    return FTL_RETRY;                        \
                return -1;                                   \
            }                                                \
        }                                                    \
    }                                                        \
}while(0)

#define FTL_RETRY_API_WITH_RET(_func, _args, _retry_count, _retval) \
do {                                                                \
    int loop = _retry_count;                                        \
    _retval = _func(_args);                                         \
    if (_retval != SDK_RET_OK) {                                    \
        while ((_retval == SDK_RET_RETRY) && loop) {                \
            _retval = _func(_args);                                 \
            loop--;                                                 \
        }                                                           \
    }                                                               \
}while(0)

// max 256 packets are processed by VPP, so 512 flows
#define MAX_FLOW_ENTRIES_PER_BATCH 512

// max sessions in a batch during upgrade.
#define MAX_SESSION_ENTRIES_PER_BATCH (MAX_FLOW_ENTRIES_PER_BATCH/2)

typedef struct flow_flags_s {
    uint8_t log : 1;
    uint8_t update : 1;
    uint8_t napt : 1;
    uint8_t host_origin : 1;
    uint8_t ctr_idx : 4;
    uint8_t drop : 1;
    uint8_t reserve : 7;
} flow_flags_t;

typedef struct v4_flow_info_s {
    uint32_t key_metadata_ipv4_src;
    uint32_t key_metadata_ipv4_dst;
    uint16_t key_metadata_flow_lkp_id;
    uint16_t key_metadata_dport;
    uint16_t key_metadata_sport;
    uint8_t key_metadata_proto;
    uint8_t epoch;
    uint32_t session_index;
    uint8_t nexthop_valid;
    uint8_t nexthop_type;
    uint16_t nexthop_id;
    uint8_t nexthop_priority;
} v4_flow_info_t;

// Prototypes

void ftl_reg_session_update_cb(session_update_cb cb);

void ftl_reg_nat_dealloc_cb(nat_flow_dealloc_cb cb);

void set_skip_ftl_program(int val);

int get_skip_ftl_program(void);

void ftl_init_stats_cache(void);

void ftl_cache_stats(ftl *obj, uint16_t thread_id);

void ftl_dump_stats_cache(char *buf, int max_len);

void ftl_dump_stats(ftl *obj, char *buf, int max_len);

ftl * ftl_create(void *key2str, void *appdata2str);

int ftl_insert(ftl *obj, flow_entry *entry, uint32_t hash, uint64_t *handle,
               uint8_t update);

int ftl_remove(ftl *obj, flow_entry *entry, uint32_t hash);

int ftl_remove_with_handle(ftl *obj, uint64_t handle);

int ftl_export_with_handle(ftl *obj, uint64_t handle,
                           uint8_t reason, bool drop);

int ftl_clear(ftl *obj, bool clear_global_state, bool clear_thread_local_state);

void ftl_delete(ftl *obj);

int ftl_dump_hw_entries(ftl *obj, char *logfile, uint8_t detail, bool v6);

uint64_t ftl_get_flow_count(ftl *obj, bool v6);

void ftl_set_session_index(flow_entry *entry, uint32_t session);

void ftl_set_flow_role(flow_entry *entry, uint8_t flow_role);

void ftl_set_epoch(flow_entry *entry, uint8_t val);

uint32_t ftl_get_session_id(flow_entry *entry);

uint8_t ftl_get_proto(flow_entry *entry);

uint16_t ftl_get_key_lookup_id (flow_entry *entry);

void ftl_set_thread_id(ftl *obj, uint32_t thread_id);

void ftl_set_key_lookup_id(flow_entry *entry, uint16_t lookup_id);

void ftl_set_entry_flow_miss_hit(flow_entry *entry, uint8_t val);

void ftl_set_entry_l2l(flow_entry *entry, uint8_t val);

void ftl_set_entry_nexthop(flow_entry *entry, uint32_t nhid, uint32_t nhtype,
                           uint8_t nhvalid, uint8_t priority);

void ftlv4_set_entry_nexthop(v4_flow_entry *entry, uint32_t nhid,
                             uint32_t nhtype, uint8_t nhvalid,
                             uint8_t priority);

void ftlv4_set_key_lookup_id(v4_flow_entry *entry, uint16_t lookup_id);

uint16_t ftlv4_get_key_lookup_id(v4_flow_entry *entry);

uint8_t ftlv4_get_entry_epoch(v4_flow_entry *entry);

uint8_t ftlv4_get_entry_nexthop_valid(v4_flow_entry *entry);

uint8_t ftlv4_get_entry_nexthop_type(v4_flow_entry *entry);

uint16_t ftlv4_get_entry_nexthop_id(v4_flow_entry *entry);

uint8_t ftlv4_get_entry_nexthop_priority(v4_flow_entry *entry);

void ftlv4_set_entry_flow_miss_hit(v4_flow_entry *entry, uint8_t val);

void ftlv4_set_entry_l2l(v4_flow_entry *entry, uint8_t val);

ftlv4 * ftlv4_create(void *key2str,
                     void *appdata2str);

void ftlv4_delete(ftlv4 *obj);

int ftlv4_dump_hw_entries(ftlv4 *obj, char *logfile,
                          uint8_t detail, uint16_t thread_id);

int ftlv4_dump_hw_entry(ftlv4 *obj, uint32_t src, uint32_t dst,
                        uint8_t ip_proto, uint16_t sport,
                        uint16_t dport, uint16_t lookup_id,
                        char *buf, int max_len, uint16_t thread_id);

int ftlv4_dump_entry_with_handle(ftlv4 *obj, uint64_t handle,
                                 v4_flow_info_t *flow_info, uint16_t thread_id);

int ftlv4_read_session_index(ftlv4 *obj, uint32_t src, uint32_t dst,
                             uint8_t ip_proto, uint16_t sport,
                             uint16_t dport, uint16_t lookup_id,
                             uint32_t *ses_id, uint16_t thread_id);

void ftlv4_init_stats_cache(void);

void ftlv4_cache_stats(ftlv4 *obj, uint16_t thread_id);

void ftlv4_dump_stats(ftlv4 *obj, char *buf, int max_len);

void ftlv4_dump_stats_cache(char *buf, int max_len);

int ftlv4_clear(ftlv4 *obj,
                bool clear_global_state,
                bool clear_thread_local_state,
                uint16_t thread_id);

uint64_t ftlv4_get_flow_count(ftlv4 *obj, uint16_t thread_id);

void ftlv4_cache_batch_init(uint16_t thread_id);

void ftlv4_cache_set_key(
             uint32_t sip,
             uint32_t dip,
             uint8_t ip_proto,
             uint16_t src_port,
             uint16_t dst_port,
             uint16_t lookup_id,
             uint16_t thread_id);

void ftlv4_cache_set_nexthop(
             uint32_t nhid,
             uint32_t nhtype,
             uint8_t nh_valid,
             uint8_t priority,
             uint16_t thread_id);

int ftlv4_cache_get_count(uint16_t thread_id);

void ftlv4_cache_advance_count(int val, uint16_t thread_id);

int ftlv4_cache_program_index(ftlv4 *obj, uint16_t id,
                              uint64_t *handle,
                              uint16_t thread_id);

int ftlv4_cache_delete_index(ftlv4 *obj, uint16_t id, uint16_t thread_id);

void ftlv4_cache_set_session_index(uint32_t val, uint16_t thread_id);

void ftlv4_cache_set_flow_role(uint8_t flow_role, uint16_t thread_id);

uint32_t ftlv4_cache_get_session_index(int id, uint16_t thread_id);

uint8_t ftlv4_cache_get_proto(int id, uint16_t thread_id);

void ftlv4_cache_set_epoch(uint8_t val, uint16_t thread_id);

void ftlv4_cache_set_l2l(uint8_t val, uint16_t thread_id);

void ftlv4_cache_set_hash_log(uint32_t val, uint8_t log, uint16_t thread_id);

void ftlv4_cache_set_flow_miss_hit(uint8_t val, uint16_t thread_id);

void ftlv4_cache_set_update_flag(uint8_t update, uint16_t thread_id);

void ftlv4_cache_set_napt_flag(uint8_t napt, uint16_t thread_id);

uint8_t ftlv4_cache_get_napt_flag (int id, uint16_t thread_id);

void ftlv4_cache_set_counter_index(uint8_t ctr_idx, uint16_t thread_id);

uint8_t ftlv4_cache_get_counter_index(int id, uint16_t thread_id);

void ftlv4_cache_batch_flush(ftlv4 *obj, int *status);

void ftlv4_set_thread_id(ftlv4 *obj, uint32_t thread_id);

void ftlv6_delete(ftlv6 *obj);

int ftlv6_dump_hw_entries(ftlv6 *obj, char *logfile, uint8_t detail);

int ftlv6_dump_hw_entry(ftlv6 *obj, uint8_t *src, uint8_t *dst,
                        uint8_t ip_proto, uint16_t sport,
                        uint16_t dport, uint16_t lookup_id,
                        char *buf, int max_len);

int ftlv6_read_session_index(ftlv6 *obj, uint8_t *src, uint8_t *dst,
                             uint8_t ip_proto, uint16_t sport,
                             uint16_t dport, uint16_t lookup_id,
                             uint32_t *ses_id);

void ftlv6_init_stats_cache(void);

void ftlv6_cache_stats(ftlv6 *obj, uint16_t thread_id);

void ftlv6_dump_stats(ftlv6 *obj, char *buf, int max_len);

void ftlv6_dump_stats_cache(char *buf, int max_len);

int ftlv6_clear(ftlv6 *obj, bool clear_global_state,
                bool clear_thread_local_state);

uint64_t ftlv6_get_flow_count(ftlv6 *obj);

void ftlv6_cache_batch_init(void);

void ftlv6_cache_set_key(uint8_t *sip,
                         uint8_t *dip,
                         uint8_t ip_proto,
                         uint16_t src_port,
                         uint16_t dst_port,
                         uint16_t lookup_id);

void ftll2_cache_set_key(uint8_t *src,
                         uint8_t *dst,
                         uint16_t ether_type,
                         uint16_t lookup_id);

void ftll2_cache_set_counter_index(uint8_t ctr_idx);

void ftlv6_cache_set_nexthop(uint32_t nhid,
                             uint32_t nhtype,
                             uint8_t nh_valid,
                             uint8_t priority);

int ftlv6_cache_get_count(void);

void ftlv6_cache_advance_count(int val);

int ftlv6_cache_program_index(ftlv6 *obj, uint16_t id,
                              uint64_t *handle);

int ftlv6_cache_delete_index(ftlv6 *obj, uint16_t id);

void ftlv6_cache_set_session_index(uint32_t val);

void ftlv6_cache_set_flow_role(uint8_t flow_role);

uint32_t ftlv6_cache_get_session_index(int id);

uint8_t ftlv6_cache_get_proto(int id);

void ftlv6_cache_set_epoch(uint8_t val);

void ftlv6_cache_set_l2l(uint8_t val);

void ftlv6_cache_set_flow_miss_hit(uint8_t val);

void ftlv6_cache_set_update_flag(uint8_t update);

void ftlv6_cache_set_hash_log(uint32_t val, uint8_t log);

void ftlv6_cache_set_counter_index(uint8_t ctr_idx);

uint8_t ftlv6_cache_get_counter_index(int id);

void ftlv6_cache_batch_flush(ftlv6 *obj, int *status);

void ftlv4_set_key(v4_flow_entry *entry,
                   uint32_t sip,
                   uint32_t dip,
                   uint8_t ip_proto,
                   uint16_t src_port,
                   uint16_t dst_port,
                   uint16_t lookup_id);

void ftlv6_set_key(flow_entry *entry,
                   uint8_t *sip,
                   uint8_t *dip,
                   uint8_t ip_proto,
                   uint16_t src_port,
                   uint16_t dst_port,
                   uint16_t lookup_id);

void ftll2_set_key(flow_entry *entry,
                   uint8_t *src,
                   uint8_t *dst,
                   uint16_t ether_type,
                   uint16_t lookup_id);

int ftlv4_remove(ftlv4 *obj, v4_flow_entry *entry,
                 uint32_t hash, uint16_t thread_id);

int ftlv4_remove_cached_entry(ftlv4 *obj, uint16_t thread_id);

void ftlv4_update_iflow_nat_session (ftlv4 *obj, uint16_t thread_id);
void ftlv4_update_rflow_nat_session (ftlv4 *obj, uint16_t thread_id);
int ftlv4_remove_nat_session(uint32_t vpc_id, ftlv4 *obj, uint16_t thread_id);

int ftlv4_update_cached_entry(ftlv4 *obj, uint16_t thread_id);

int ftlv4_get_with_handle(ftlv4 *obj, uint64_t handle,
                          uint16_t thread_id);

void ftlv4_get_last_read_session_info(uint32_t *sip,
                                      uint32_t *dip,
                                      uint16_t *sport,
                                      uint16_t *dport,
                                      uint16_t *lkp_id,
                                      uint16_t thread_id);

void ftlv4_set_last_read_entry_epoch(uint8_t epoch, uint16_t thread_id);

void ftlv4_get_last_read_entry_epoch(uint8_t *epoch, uint16_t thread_id);

void ftlv4_get_last_read_entry_lookup_id(uint16_t *lkp_id, uint16_t thread_id);

void ftlv4_set_last_read_entry_nexthop(uint32_t nhid,
                                       uint32_t nhtype,
                                       uint8_t nh_valid,
                                       uint8_t priority,
                                       uint16_t thread_id);

void ftlv4_set_last_read_entry_miss_hit(uint8_t flow_miss, uint16_t thread_id);

void ftlv4_set_last_read_entry_l2l(uint8_t l2l, uint16_t thread_id);

enum flow_export_reason_e {
    FLOW_EXPORT_REASON_ADD,
    FLOW_EXPORT_REASON_DEL,
    FLOW_EXPORT_REASON_ACTIVE,
};

int ftlv4_export_with_handle(ftlv4 *obj, uint64_t iflow_handle,
                             uint64_t rflow_handle,
                             uint8_t reason,
                             bool host_origin, bool drop,
                             uint16_t thread_id);

int ftlv4_export_with_entry(v4_flow_entry *iv4entry,
                            v4_flow_entry *rv4entry,
                            uint8_t reason, bool host_origin,
                            bool drop);

int ftlv4_get_flow_entry(ftlv4 *obj, uint64_t flow_handle,
                         v4_flow_entry *entry, uint16_t thread_id);
int ftlv4_cache_log_session(uint16_t iid, uint16_t rid,
                            uint8_t reason, uint16_t thread_id);

void ftlv4_cache_set_host_origin(uint8_t host_origin, uint16_t thread_id);

void ftlv4_cache_set_drop(uint8_t drop, uint16_t tid);

int ftlv6_cache_log_session(uint16_t iid, uint16_t rid, uint8_t reason);

int ftl_export_with_entry(flow_entry *entry, uint8_t reason, bool drop);

int ftlv6_remove(ftlv6 *obj, flow_entry *entry, uint32_t hash);

int ftlv4_insert_with_new_lookup_id (ftlv4 *obj, uint64_t handle,
                                     uint64_t *ret_handle,
                                     uint16_t lookup_id, bool l2l);

void ftlv4_get_handle_str (char *handle_str, uint64_t handle);

int ftlv6_remove_cached_entry(ftlv6 *obj);

int ftlv6_get_with_handle(ftlv6 *obj, uint64_t handle);

void ftlv6_get_last_read_session_info (uint8_t *sip,
                                       uint8_t *dip,
                                       uint16_t *sport,
                                       uint16_t *dport,
                                       uint16_t *lkp_id);

void ftlv6_set_thread_id(ftlv6 *obj, uint32_t thread_id);

#ifdef __cplusplus
}
#endif

#endif    // __VPP_IMPL_FTL_WRAPPER_H__
