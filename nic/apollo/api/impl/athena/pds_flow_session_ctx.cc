//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// athena flow session context implementation
///
//----------------------------------------------------------------------------

#include <algorithm>
#include <inttypes.h>
#include "pds_flow_session_ctx.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "platform/src/lib/nicmgr/include/pd_client.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "gen/p4gen/athena/include/p4pd.h"

/*
 * Session context holds a backpointer from session_info to its corresponding
 * flow cache entry (a 1-to-1 mapping).
 *
 * There are several reasons the backpointer is stored separately from the
 * session_info itself. The primary reason is performance. The backpointer
 * creates a circular dependency between session_info and flow cache, i.e.,
 * flow cache entry points to session_info and session_info points to flow
 * cache. Flow cache and session_info creation would have necessitated
 * 3 operations: 1) create session_info, 2) create flow cache, 3) update
 * session_info. Step 3 is expensive as it is a 64-byte write with byte 
 * swizzling. In total, that would double the time for flow creation and also
 * double flow deletion time.
 * 
 * Moving the context outside of session_info results in shorter write time
 * (4 bytes) and, since SW is the only component reading/writing the context,
 * no byte swizzling is necessary. Futhermore, the context can be in process
 * memory instead of HBM, provided the following condition holds:
 * - If/when stateful process restart became a reality and the design was
 *   to have all flows emptied out upon restart, then process memory usage
 *   would continue to be the most efficient mode. Otherwise, session context
 *   would have to be stored in HBM.
 */
//#define FLOW_SESSION_CTX_HBM_HANDLE     "flow_session_ctx"

/*
 * HBM mode, if used, is only considered for HW to simplify implementation.
 */
#ifdef __aarch64__
#ifdef FLOW_SESSION_CTX_HBM_HANDLE
#define FLOW_SESSION_CTX_MODE_HBM
#endif
#endif

/*
 * Session context entry
 * Fields arrangement intended to minimize memory consumption
 */
typedef struct {
    uint64_t            handle;
} __attribute__((packed)) ctx_entry_t;

typedef struct {
    volatile uint8_t    lock;
} __attribute__((packed)) ctx_lock_t;

/*
 * Session context anchor
 */
class session_ctx_t {

public:
    session_ctx_t() :
        platform_type(platform_type_t::PLATFORM_TYPE_NONE),
        ctx_lock(nullptr),
        ctx_table(nullptr)
    {
        session_prop = {0};
        cache_prop = {0};
    }

    ~session_ctx_t()
    {
        fini();
    }

    pds_ret_t init(pds_cinit_params_t *params);
    void fini(void);

    inline ctx_entry_t *ctx_entry_get(uint32_t session_id)
    {
        return ctx_table && (session_id < session_prop.tabledepth) ?
               &ctx_table[session_id] : nullptr;
    }
    inline ctx_lock_t *ctx_lock_get(uint32_t session_id)
    {
        return ctx_lock && (session_id < session_prop.tabledepth) ?
               &ctx_lock[session_id] : nullptr;
    }

    inline ctx_entry_t *ctx_entry_lock(uint32_t session_id)
    {
        ctx_lock_t *lock = ctx_lock_get(session_id);
        if (lock) {
            while (__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE));
        }
        return ctx_entry_get(session_id);
    }

    inline void ctx_entry_unlock(uint32_t session_id)
    {
        ctx_lock_t *lock = ctx_lock_get(session_id);
        if (lock) {
            __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
        }
    }

private:
    platform_type_t             platform_type;
    p4pd_table_properties_t     session_prop;
    p4pd_table_properties_t     cache_prop;
    ctx_lock_t                  *ctx_lock;
    ctx_entry_t                 *ctx_table;
};

static session_ctx_t            session_ctx;

pds_ret_t
session_ctx_t::init(pds_cinit_params_t *params)
{
    p4pd_error_t    p4pd_error;

    platform_type = api::g_pds_state.platform_type();

    p4pd_error = p4pd_table_properties_get(P4TBL_ID_FLOW,
                                           &cache_prop);
    if ((p4pd_error != P4PD_SUCCESS) || !cache_prop.tabledepth) {
        PDS_TRACE_ERR("failed flow cache properties: count %u "
                      "error %d", cache_prop.tabledepth, p4pd_error);
        return PDS_RET_ERR;
    }

    p4pd_error = p4pd_table_properties_get(P4TBL_ID_SESSION_INFO,
                                           &session_prop);
    if ((p4pd_error != P4PD_SUCCESS) || !session_prop.tabledepth) {
        PDS_TRACE_ERR("failed session info properties: count %u "
                      "error %d", session_prop.tabledepth, p4pd_error);
        return PDS_RET_ERR;
    }
    PDS_TRACE_DEBUG("Session context init session depth %u cache depth %u",
                    session_prop.tabledepth, cache_prop.tabledepth);
    /*
     * Session contexts assumed not needed when flow aging is not enabled
     * for the current process (memory saving).
     */
#ifdef __aarch64__
    if (params->flow_age_pid == getpid()) {
#endif

#ifdef FLOW_SESSION_CTX_MODE_HBM
        uint64_t hbm_paddr = api::g_pds_state.mempartition()->start_addr(
                                              FLOW_SESSION_CTX_HBM_HANDLE);
        uint32_t hbm_bytes = api::g_pds_state.mempartition()->size(
                                              FLOW_SESSION_CTX_HBM_HANDLE);
        uint32_t table_bytes = sizeof(ctx_entry_t) * session_prop.tabledepth;
        if ((hbm_paddr == INVALID_MEM_ADDRESS) || (hbm_bytes < table_bytes)) {
            PDS_TRACE_ERR("failed to obtain enough HBM for %s",
                          FLOW_SESSION_CTX_HBM_HANDLE);
            return PDS_RET_NO_RESOURCE;
        }
        if (!platform_is_hw(platform_type)) {
            PDS_TRACE_ERR("Platform must be HW to use HBM");
            return PDS_RET_ERR;
        }
        ctx_table = (ctx_entry_t *)sdk::lib::pal_mem_map(hbm_paddr, table_bytes);
        if (!ctx_table) {
            PDS_TRACE_ERR("failed to memory map addr 0x%" PRIx64 " size %u",
                          hbm_paddr, table_bytes);
            return PDS_RET_ERR;
        }
        memset((void *)ctx_table, 0, table_bytes);
#else
        ctx_table = (ctx_entry_t *)SDK_CALLOC(SDK_MEM_ALLOC_FLOW_SESSION_CTX,
                                              session_prop.tabledepth *
                                              sizeof(ctx_entry_t));
        if (!ctx_table) {
            PDS_TRACE_ERR("fail to allocate ctx_table");
            return PDS_RET_OOM;
        }
#endif
        ctx_lock = (ctx_lock_t *)SDK_CALLOC(SDK_MEM_ALLOC_FLOW_SESSION_CTX,
                                            session_prop.tabledepth *
                                            sizeof(ctx_lock_t));
        if (!ctx_lock) {
            PDS_TRACE_ERR("fail to allocate ctx_lock");
            return PDS_RET_OOM;
        }
#ifdef __aarch64__
    }
#endif
    return PDS_RET_OK;
}

void
session_ctx_t::fini(void)
{
#ifdef FLOW_SESSION_CTX_MODE_HBM
    if (ctx_table) {
        sdk::lib::pal_mem_unmap((void *)ctx_table);
        ctx_vaddr = nullptr;
    }
#else
    if (ctx_table) {
        SDK_FREE(SDK_MEM_ALLOC_FLOW_SESSION_CTX, ctx_table);
        ctx_table = nullptr;
    }
#endif
    if (ctx_lock) {
        SDK_FREE(SDK_MEM_ALLOC_FLOW_SESSION_CTX, ctx_lock);
        ctx_lock = nullptr;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

pds_ret_t
pds_flow_session_ctx_init(pds_cinit_params_t *params)
{
    return session_ctx.init(params);
}

void
pds_flow_session_ctx_fini(void)
{
    session_ctx.fini();
}

pds_ret_t
pds_flow_session_ctx_set(uint32_t session_id,
                         uint64_t handle)
{
    ctx_entry_t *ctx = session_ctx.ctx_entry_get(session_id);
    if (ctx) {
        uint64_t ctx_handle = 0;
        SDK_ATOMIC_LOAD_UINT64(&ctx->handle, &ctx_handle);
        if (ctx_handle && (ctx_handle != handle)) {

            /*
             * Return a unique error code to allow caller to
             * differentiate from flow cache case of "entry exists".
             */
            return PDS_RET_MAPPING_CONFLICT;
        }
        SDK_ATOMIC_STORE_UINT64(&ctx->handle, &handle);
        return PDS_RET_OK;
    }
    PDS_TRACE_ERR("failed session_id %u", session_id);
    return PDS_RET_INVALID_ARG;
}

void
pds_flow_session_ctx_clr(uint32_t session_id)
{
    ctx_entry_t *ctx = session_ctx.ctx_entry_get(session_id);
    if (ctx) {
        uint64_t ctx_handle = 0;
        SDK_ATOMIC_STORE_UINT64(&ctx->handle, &ctx_handle);
    }
}

pds_ret_t
pds_flow_session_ctx_get(uint32_t session_id,
                         uint64_t *ret_handle)
{
    ctx_entry_t *ctx = session_ctx.ctx_entry_get(session_id);
    if (ctx) {
        SDK_ATOMIC_LOAD_UINT64(&ctx->handle, ret_handle);
        if (*ret_handle) {
            return PDS_RET_OK;
        }
    }
    return PDS_RET_ENTRY_NOT_FOUND;
}

pds_ret_t
pds_flow_session_ctx_get_clr(uint32_t session_id,
                             uint64_t *ret_handle)
{
    ctx_entry_t *ctx = session_ctx.ctx_entry_get(session_id);
    if (ctx) {
        uint64_t ctx_handle = 0;
        SDK_ATOMIC_EXCHANGE_UINT64(&ctx->handle, &ctx_handle, ret_handle);
        if (*ret_handle) {
            return PDS_RET_OK;
        }
    }
    return PDS_RET_ENTRY_NOT_FOUND;
}

void
pds_flow_session_ctx_move(uint32_t session_id,
                          uint64_t handle) 
{
    ctx_entry_t *ctx = session_ctx.ctx_entry_get(session_id);
    if (ctx) {
        uint64_t old_handle;
        SDK_ATOMIC_EXCHANGE_UINT64(&ctx->handle, &handle, &old_handle);
        if (!old_handle) {
            SDK_ATOMIC_STORE_UINT64(&ctx->handle, &old_handle);
        }
    }
}

/*
 * These 2 API should only be used if the user wants to have full control
 * of the lock/unlock operations.
 */
void
pds_flow_session_ctx_lock(uint32_t session_id)
{
    session_ctx.ctx_entry_lock(session_id);
}

void
pds_flow_session_ctx_unlock(uint32_t session_id)
{
    session_ctx.ctx_entry_unlock(session_id);
}

#ifdef __cplusplus
}
#endif

