//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines APIs for store 
///

#ifndef __PDS_STORE_H__
#define __PDS_STORE_H__

#include "nic/sdk/lib/shmstore/shmstore.hpp"

#define PDS_STORE_NAME_SIZE    (256)

/// \brief     create a store for state save/restore
/// \param[in] mod_name module name, eg ftl, impl
/// \param[in] tbl_name table name, eg flow, session_age_ctx
/// \param[in] version_id version of table
/// \param[in] size size of store
/// \param[in] create flag to select create or open the store
/// \return    shmstore pointer on success, null on failure
/// \remark    Valid pointers for mod_name and tbl_name should be passed
sdk::lib::shmstore* pds_store_init(const char *mod_name, const char *tbl_name,
                                   uint8_t version_id, size_t size,
                                   bool create = true);

#endif // __PDS_STORE_H__
