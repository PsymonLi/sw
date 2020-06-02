//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <include/sdk/base.hpp>
#include <lib/pal/pal.hpp>
#include <lib/p4/p4_utils.hpp>
#include <lib/p4/p4_api.hpp>
#include <platform/capri/capri_tbl_rw.hpp>
#include <pd_utils.h>
#include "session.h"
#include "gen/p4gen/p4/include/ftl.h"
#include "ftl_wrapper.h"

#define VPP_SESS_MAX_THREADS 4

using namespace sdk::lib;
using namespace sdk::platform;

p4pd_table_properties_t g_session_tbl_ctx;

static int skip_session_program = 0;

int
get_skip_session_program(void)
{
    return skip_session_program;
}

void
set_skip_session_program(int val)
{
    skip_session_program = val;
}

int
initialize_session(void)
{
    p4pd_error_t p4pd_ret;

    p4pd_ret = p4pd_table_properties_get(P4TBL_ID_SESSION, &g_session_tbl_ctx);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    return 0;
}

void
session_insert(uint32_t ses_id, void *ses_info)
{
    uint64_t entry_addr = (ses_id * g_session_tbl_ctx.hbm_layout.entry_width);
    uint64_t *src_addr = (uint64_t *)ses_info;

    if (get_skip_session_program()) {
        return;
    }

    if (likely(g_session_tbl_ctx.base_mem_va)) {
        uint64_t *dst_addr = (uint64_t *)
                              (g_session_tbl_ctx.base_mem_va + entry_addr);
        for (uint16_t i = 0;
             i < (g_session_tbl_ctx.hbm_layout.entry_width / sizeof(uint64_t)); i++) {
            dst_addr[i] = src_addr[i];
        }
    } else {
        pal_mem_write(g_session_tbl_ctx.base_mem_pa, (uint8_t *)src_addr,
                      g_session_tbl_ctx.hbm_layout.entry_width);
    }
    capri::capri_hbm_table_entry_cache_invalidate(g_session_tbl_ctx.cache,
                                                  entry_addr,
                                                  g_session_tbl_ctx.hbm_layout.entry_width,
                                                  g_session_tbl_ctx.base_mem_pa);
}

void
session_get_addr(uint32_t ses_id, uint8_t **ses_addr, uint32_t *entry_size)
{
    static thread_local uint8_t *ret_addr =
        (uint8_t *) calloc(g_session_tbl_ctx.hbm_layout.entry_width, 1);
    *entry_size = g_session_tbl_ctx.hbm_layout.entry_width;
    if (likely(g_session_tbl_ctx.base_mem_va)) {
        *ses_addr = (uint8_t *) (g_session_tbl_ctx.base_mem_va +
                    (ses_id * (*entry_size)));
    } else {
       pal_mem_read((g_session_tbl_ctx.base_mem_pa + (ses_id * (*entry_size))),
                    ret_addr, *entry_size);
       *ses_addr = ret_addr;
    }
    return;
}

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
sess_info_cache_batch_get_entry(uint16_t thread_id)
{
    return &g_sess_info_cache[thread_id].sessions[g_sess_info_cache[thread_id].count];
}

sess_info_t *
sess_info_cache_batch_get_entry_index(int index, uint16_t thread_id)
{
    return &g_sess_info_cache[thread_id].sessions[index];
}

int
sess_info_cache_batch_get_count(uint16_t thread_id)
{
    return g_sess_info_cache[thread_id].count;
}

void
sess_info_cache_advance_count(int val, uint16_t thread_id)
{
    g_sess_info_cache[thread_id].count += val;
}

bool
sess_info_cache_full(uint16_t thread_id)
{
    return (g_sess_info_cache[thread_id].count == MAX_SESSION_ENTRIES_PER_BATCH);
}
