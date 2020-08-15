//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements APIs for store
///

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/shmstore/shmstore.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/athena/api/include/pds_base.h"
#include "nic/athena/api/include/internal/pds_store.h"

using namespace sdk::lib;

static inline pds_ret_t
pds_store_name (const char *mod_name, const char *tbl_name,
                uint8_t version_id, char *store_name)
{
    std::string name;

    if (mod_name == NULL || tbl_name == NULL ||
        store_name == NULL) {
        PDS_TRACE_ERR("Name null");
        return PDS_RET_ERR;
    }

    name = std::string(mod_name) +
               std::string("_") +
               std::string(tbl_name) +
               std::string("_v") +
               std::to_string(version_id);
    strncpy(store_name, name.c_str(), name.size());
    store_name[name.size()] = '\0';
    return PDS_RET_OK;
}

sdk::lib::shmstore*
pds_store_init (const char *mod_name, const char *tbl_name,
                uint8_t version_id, size_t size, bool create)
{
    pds_ret_t ret;
    shmstore *store;
    char store_name[PDS_STORE_NAME_SIZE];

    store = shmstore::factory();
    if (store == NULL) {
        PDS_TRACE_ERR("store factory for table %s, module %s failed",
                      tbl_name, mod_name);
        return NULL;
    }
    ret = pds_store_name(mod_name, tbl_name, version_id, store_name);
    if (ret != PDS_RET_OK)
        return NULL;

    if (create)
        ret = (pds_ret_t)store->create(store_name, size);
    else
        ret = (pds_ret_t)store->open(store_name, SHM_OPEN_ONLY);

    if (ret != PDS_RET_OK) {
        PDS_TRACE_ERR("%s store %s failed ret %d",
                      store_name, create ? "create" : "open", ret);
        return NULL;
    }
    return store;
}
