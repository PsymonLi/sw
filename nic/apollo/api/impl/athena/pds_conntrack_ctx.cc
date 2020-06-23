//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// athena flow connection tracking context implementation
///
//----------------------------------------------------------------------------

#include <algorithm>
#include <inttypes.h>
#include "pds_conntrack_ctx.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/include/athena/pds_conntrack.h"
#include "platform/src/lib/nicmgr/include/pd_client.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "gen/p4gen/athena/include/p4pd.h"

/*
 * Connection tracking context holds a backpointer from conntrack entry to its
 * corresponding conntrack info entry (a 1-to-1 mapping).
 *
 * Moving the context outside of conntrack results in shorter write time
 * and, since SW is the only component reading/writing the context,
 * no byte swizzling is necessary. Futhermore, the context can be in process
 * memory instead of HBM, provided the following condition holds:
 * - If/when stateful process restart became a reality and the design was
 *   to have all flows emptied out upon restart, then process memory usage
 *   would continue to be the most efficient mode. Otherwise, conntrack
 *   context would have to be stored in HBM.
 */
//#define CONNTRACK_CTX_HBM_HANDLE     "conntrack_ctx"

/*
 * HBM mode, if used, is only considered for HW to simplify implementation.
 */
#ifdef __aarch64__
#ifdef CONNTRACK_CTX_HBM_HANDLE
#define CONNTRACK_CTX_MODE_HBM
#endif
#endif

/*
 * Conntrack context entry
 * Fields arrangement intended to minimize memory consumption
 */
typedef struct {
    uint32_t            handle;
} __attribute__((packed)) ctx_entry_t;

typedef struct {
    volatile uint8_t    lock;
} __attribute__((packed)) ctx_lock_t;

/*
 * Conntrack context anchor
 */
class conntrack_ctx_t {

public:
    conntrack_ctx_t() :
        platform_type(platform_type_t::PLATFORM_TYPE_NONE),
        ctx_lock(nullptr),
        ctx_table(nullptr)
    {
        conntrack_prop = {0};
    }

    ~conntrack_ctx_t()
    {
        fini();
    }

    pds_ret_t init(void);
    void fini(void);

    inline ctx_entry_t *ctx_entry_get(uint32_t conntrack_id)
    {
        return conntrack_id < conntrack_prop.tabledepth ?
               &ctx_table[conntrack_id] : nullptr;
    }
    inline ctx_lock_t *ctx_lock_get(uint32_t conntrack_id)
    {
        return conntrack_id < conntrack_prop.tabledepth ?
               &ctx_lock[conntrack_id] : nullptr;
    }

    inline ctx_entry_t *ctx_entry_lock(uint32_t conntrack_id)
    {
        ctx_lock_t *lock = ctx_lock_get(conntrack_id);
        if (lock) {
            while (__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE));
        }
        return ctx_entry_get(conntrack_id);
    }

    inline void ctx_entry_unlock(uint32_t conntrack_id)
    {
        ctx_lock_t *lock = ctx_lock_get(conntrack_id);
        if (lock) {
            __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
        }
    }

private:
    platform_type_t             platform_type;
    p4pd_table_properties_t     conntrack_prop;
    ctx_lock_t                  *ctx_lock;
    ctx_entry_t                 *ctx_table;
};

static conntrack_ctx_t            conntrack_ctx;

pds_ret_t
conntrack_ctx_t::init(void)
{
    p4pd_error_t    p4pd_error;

    platform_type = api::g_pds_state.platform_type();
    p4pd_error = p4pd_table_properties_get(P4TBL_ID_CONNTRACK,
                                           &conntrack_prop);
    if ((p4pd_error != P4PD_SUCCESS) || !conntrack_prop.tabledepth) {
        PDS_TRACE_ERR("failed conntrack info properties: count %u "
                      "error %d", conntrack_prop.tabledepth, p4pd_error);
        return PDS_RET_ERR;
    }
    PDS_TRACE_DEBUG("Conntrack context init conntrack depth %u",
                    conntrack_prop.tabledepth);
#ifdef CONNTRACK_CTX_MODE_HBM
    uint64_t hbm_paddr = api::g_pds_state.mempartition()->start_addr(
                                          CONNTRACK_CTX_HBM_HANDLE);
    uint32_t hbm_bytes = api::g_pds_state.mempartition()->size(
                                          CONNTRACK_CTX_HBM_HANDLE);
    uint32_t table_bytes = sizeof(ctx_entry_t) * conntrack_prop.tabledepth;
    if ((hbm_paddr == INVALID_MEM_ADDRESS) || (hbm_bytes < table_bytes)) {
        PDS_TRACE_ERR("failed to obtain enough HBM for %s",
                      CONNTRACK_CTX_HBM_HANDLE);
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
    ctx_table = (ctx_entry_t *)SDK_CALLOC(SDK_MEM_ALLOC_CONNTRACK_CTX,
                                          conntrack_prop.tabledepth *
                                          sizeof(ctx_entry_t));
    if (!ctx_table) {
        PDS_TRACE_ERR("fail to allocate ctx_table");
        return PDS_RET_OOM;
    }
#endif
    ctx_lock = (ctx_lock_t *)SDK_CALLOC(SDK_MEM_ALLOC_CONNTRACK_CTX,
                                        conntrack_prop.tabledepth *
                                        sizeof(ctx_lock_t));
    if (!ctx_lock) {
        PDS_TRACE_ERR("fail to allocate ctx_lock");
        return PDS_RET_OOM;
    }
    return PDS_RET_OK;
}

void
conntrack_ctx_t::fini(void)
{
#ifdef CONNTRACK_CTX_MODE_HBM
    if (ctx_table) {
        sdk::lib::pal_mem_unmap((void *)ctx_table);
        ctx_vaddr = nullptr;
    }
#else
    if (ctx_table) {
        SDK_FREE(SDK_MEM_ALLOC_CONNTRACK_CTX, ctx_table);
        ctx_table = nullptr;
    }
#endif
    if (ctx_lock) {
        SDK_FREE(SDK_MEM_ALLOC_CONNTRACK_CTX, ctx_lock);
        ctx_lock = nullptr;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

pds_ret_t
pds_conntrack_ctx_init(void)
{
    return conntrack_ctx.init();
}

void
pds_conntrack_ctx_fini(void)
{
    conntrack_ctx.fini();
}

pds_ret_t
pds_conntrack_ctx_set(uint32_t conntrack_id,
                      uint32_t handle)
{
    ctx_entry_t *ctx = conntrack_ctx.ctx_entry_get(conntrack_id);
    if (ctx) {
        uint32_t ctx_handle = 0;
        SDK_ATOMIC_LOAD_UINT32(&ctx->handle, &ctx_handle);
        if (ctx_handle && (ctx_handle != handle)) {

            /*
             * Return a unique error code to allow caller to
             * differentiate from case of "entry exists".
             */
            return PDS_RET_MAPPING_CONFLICT;
        }
        SDK_ATOMIC_STORE_UINT32(&ctx->handle, &handle);
        return PDS_RET_OK;
    }
    PDS_TRACE_ERR("failed conntrack_id %u handle %u",
                  conntrack_id, handle);
    return PDS_RET_INVALID_ARG;
}

void
pds_conntrack_ctx_clr(uint32_t conntrack_id)
{
    ctx_entry_t *ctx = conntrack_ctx.ctx_entry_get(conntrack_id);
    if (ctx) {
        uint32_t ctx_handle = 0;
        SDK_ATOMIC_STORE_UINT32(&ctx->handle, &ctx_handle);
    }
}

pds_ret_t
pds_conntrack_ctx_get(uint32_t conntrack_id,
                      uint32_t *ret_handle)
{
    ctx_entry_t *ctx = conntrack_ctx.ctx_entry_get(conntrack_id);
    if (ctx) {
        SDK_ATOMIC_LOAD_UINT32(&ctx->handle, ret_handle);
        if (*ret_handle) {
            return PDS_RET_OK;
        }
    }
    return PDS_RET_ENTRY_NOT_FOUND;
}

pds_ret_t
pds_conntrack_ctx_get_clr(uint32_t conntrack_id,
                          uint32_t *ret_handle)
{
    ctx_entry_t *ctx = conntrack_ctx.ctx_entry_get(conntrack_id);
    if (ctx) {
        uint32_t ctx_handle = 0;
        SDK_ATOMIC_EXCHANGE_UINT32(&ctx->handle, &ctx_handle, ret_handle);
        if (*ret_handle) {
            return PDS_RET_OK;
        }
    }
    return PDS_RET_ENTRY_NOT_FOUND;
}

/*
 * These 2 API should only be used if the user wants to have full control
 * of the lock/unlock operations.
 */
void
pds_conntrack_ctx_lock(uint32_t conntrack_id)
{
    conntrack_ctx.ctx_entry_lock(conntrack_id);
}

void
pds_conntrack_ctx_unlock(uint32_t conntrack_id)
{
    conntrack_ctx.ctx_entry_unlock(conntrack_id);
}

#ifdef __cplusplus
}
#endif

