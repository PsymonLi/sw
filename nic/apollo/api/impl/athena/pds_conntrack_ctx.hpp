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
#include "nic/sdk/include/sdk/table.hpp"

#ifdef __cplusplus
extern "C" {
#endif

pds_ret_t pds_conntrack_ctx_init(void);
void pds_conntrack_ctx_fini(void);
void pds_conntrack_ctx_lock(uint32_t conntrack_id);
void pds_conntrack_ctx_unlock(uint32_t conntrack_id);

pds_ret_t pds_conntrack_ctx_set(uint32_t conntrack_id,
                                uint32_t handle);
void pds_conntrack_ctx_clr(uint32_t conntrack_id);
pds_ret_t pds_conntrack_ctx_get(uint32_t conntrack_id,
                                uint32_t *ret_handle);
pds_ret_t pds_conntrack_ctx_get_clr(uint32_t conntrack_id,
                                    uint32_t *ret_handle);
#ifdef __cplusplus
}
#endif

#endif // __PDS_CONNTRACK_CTX_HPP__

