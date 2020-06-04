//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines flow session context API
///
//----------------------------------------------------------------------------

#ifndef __PDS_FLOW_SESSION_CTX_HPP__
#define __PDS_FLOW_SESSION_CTX_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/api/include/athena/pds_base.h"
#include "nic/sdk/include/sdk/table.hpp"

#ifdef __cplusplus
extern "C" {
#endif

pds_ret_t pds_flow_session_ctx_init(void);
void pds_flow_session_ctx_fini(void);
void pds_flow_session_ctx_lock(uint32_t session_id);
void pds_flow_session_ctx_unlock(uint32_t session_id);

pds_ret_t pds_flow_session_ctx_set(uint32_t session_id,
                                   uint64_t handle);
void pds_flow_session_ctx_clr(uint32_t session_id);
pds_ret_t pds_flow_session_ctx_get(uint32_t session_id,
                                   uint64_t *ret_handle);
pds_ret_t pds_flow_session_ctx_get_clr(uint32_t session_id,
                                       uint64_t *ret_handle);
void pds_flow_session_ctx_move(uint32_t session_id,
                               uint64_t handle, 
                               bool move_complete);
#ifdef __cplusplus
}
#endif

#endif // __PDS_FLOW_SESSION_CTX_HPP__


