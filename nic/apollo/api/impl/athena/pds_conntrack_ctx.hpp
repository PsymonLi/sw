//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines connection tracking context API
///
//----------------------------------------------------------------------------

#ifndef __PDS_CONNTRACK_CTX_HPP__
#define __PDS_CONNTRACK_CTX_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/api/include/athena/pds_base.h"
#include "nic/apollo/api/include/athena/pds_init.h"
#include "nic/sdk/include/sdk/table.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define PDS_CONNTRACK_CTX_CACHE_HANDLE_MASK  (~((uint64_t)0xffffffff))

pds_ret_t pds_conntrack_ctx_init(pds_cinit_params_t *params);
void pds_conntrack_ctx_fini(void);
void pds_conntrack_ctx_lock(uint32_t conntrack_id);
void pds_conntrack_ctx_unlock(uint32_t conntrack_id);

pds_ret_t pds_conntrack_ctx_set(uint32_t conntrack_id,
                                uint64_t handle);
void pds_conntrack_ctx_clr(uint32_t conntrack_id);
pds_ret_t pds_conntrack_ctx_get(uint32_t conntrack_id,
                                uint64_t *ret_handle);
pds_ret_t pds_conntrack_ctx_get_clr(uint32_t conntrack_id,
                                    uint64_t *ret_handle);
void pds_conntrack_ctx_move(uint32_t conntrack_id,
                            uint64_t handle, 
                            bool move_complete);

static inline bool
pds_conntrack_ctx_session_handle_valid(uint64_t handle)
{
    return handle && ((handle & PDS_CONNTRACK_CTX_CACHE_HANDLE_MASK) == 0);
}

static inline bool
pds_conntrack_ctx_cache_handle_valid(uint64_t handle)
{
    return (handle & PDS_CONNTRACK_CTX_CACHE_HANDLE_MASK) != 0;
}

#ifdef __cplusplus
}
#endif

#endif // __PDS_CONNTRACK_CTX_HPP__

