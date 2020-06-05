//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_SESS_RESTORE__H__
#define __VPP_IMPL_SESS_RESTORE__H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct sess_info_ {
    uint32_t id;
    uint8_t proto;
    uint8_t thread_id;
    uint16_t ingress_bd;
    uint32_t v4 : 1;
    uint32_t flow_state : 3;
    uint32_t packet_type : 5;
    uint32_t iflow_rx : 1;
    uint32_t nat : 1;
    uint32_t drop : 1;
    uint32_t vnic_id : 7;
    uint64_t iflow_handle;
    uint64_t rflow_handle;
    void *flow_table;
} sess_info_t;

bool pds_decode_one_v4_session(const uint8_t *data, const uint8_t len,
                               sess_info_t *sess,
                               const uint16_t thread_index);
void pds_encode_one_v4_session(uint8_t *data, uint8_t *len,
                               sess_info_t *sess,
                               const uint16_t thread_index);

void sess_info_cache_batch_init (uint16_t thread_id);

sess_info_t *sess_info_cache_batch_get_entry(uint16_t thread_id);

sess_info_t *sess_info_cache_batch_get_entry_index(int index,
                                                   uint16_t thread_id);

int sess_info_cache_batch_get_count(uint16_t thread_id);

void sess_info_cache_advance_count(int val, uint16_t thread_id);

bool sess_info_cache_full(uint16_t thread_id);

#ifdef __cplusplus
}
#endif

#endif  // __VPP_IMPL_SESS_RESTORE__H__
