//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include "sess_restore.h"
#include "ftl_wrapper.h"

#define VPP_SESS_MAX_THREADS 4

typedef struct sess_info_cache_s {
    sess_info_t sessions[MAX_SESSION_ENTRIES_PER_BATCH];
    uint16_t count;
} sess_info_cache_t;

static sess_info_cache_t g_sess_info_cache[VPP_SESS_MAX_THREADS];

void
sess_info_cache_batch_init (uint16_t thread_id)
{
   g_sess_info_cache[thread_id].count = 0;
}

sess_info_t *
sess_info_cache_batch_get_entry (uint16_t thread_id)
{
    uint16_t count = g_sess_info_cache[thread_id].count;
    return &g_sess_info_cache[thread_id].sessions[count];
}

sess_info_t *
sess_info_cache_batch_get_entry_index (int index, uint16_t thread_id)
{
    return &g_sess_info_cache[thread_id].sessions[index];
}

int
sess_info_cache_batch_get_count (uint16_t thread_id)
{
    return g_sess_info_cache[thread_id].count;
}

void
sess_info_cache_advance_count (int val, uint16_t thread_id)
{
    g_sess_info_cache[thread_id].count += val;
}

bool
sess_info_cache_full (uint16_t thread_id)
{
    return (g_sess_info_cache[thread_id].count ==
            MAX_SESSION_ENTRIES_PER_BATCH);
}
