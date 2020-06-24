//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines mirror session APIs for internal module interactions
///
//----------------------------------------------------------------------------

#include <unordered_map>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/lock.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_mirror.hpp"
#include "nic/apollo/api/internal/pds.hpp"
#include "nic/apollo/api/internal/pds_mirror.hpp"

using std::unordered_map;

namespace api {

// mirror session to nh/nh-group mapping
typedef unordered_map<pds_obj_key_t, nh_info_t, pds_obj_key_hash> mirror_session_nh_map_t;

// mirror session forwarding database
typedef struct mirror_session_fdb_s {
public:
    mirror_session_fdb_s() {
        SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_PRIVATE);
    }
    ~mirror_session_fdb_s() {
        SDK_SPINLOCK_DESTROY(&slock_);
    }
    sdk_ret_t add(pds_obj_key_t& mirror_session_key, nh_info_t& nh_info) {
        SDK_SPINLOCK_LOCK(&slock_);
        nh_map_[mirror_session_key] = nh_info;
        SDK_SPINLOCK_UNLOCK(&slock_);
        return SDK_RET_OK;
    }
    sdk_ret_t remove(pds_obj_key_t& mirror_session_key) {
        SDK_SPINLOCK_LOCK(&slock_);
        nh_map_.erase(mirror_session_key);
        SDK_SPINLOCK_UNLOCK(&slock_);
        return SDK_RET_OK;
    }

    sdk_ret_t find(pds_obj_key_t& key, nh_info_t *nh_info) {
        sdk_ret_t ret;

        SDK_SPINLOCK_LOCK(&slock_);
        auto it = nh_map_.find(key);
        if (it != nh_map_.end()) {
            *nh_info = it->second;
            ret = SDK_RET_OK;
        } else {
            ret = SDK_RET_ENTRY_NOT_FOUND;
        }
        SDK_SPINLOCK_UNLOCK(&slock_);
        return ret;
    }

private:
    sdk_spinlock_t slock_;
    mirror_session_nh_map_t nh_map_;
} mirror_session_fdb_t;

// mirror session forwarding database instance
static mirror_session_fdb_t g_mirror_session_fdb;

/// \brief this API is invoked by metaswitch stub to inform
///        forwarding/reachability information for a given erspan collector
///        in a mirror session
/// \param[in] key        mirror session key
/// \param[in] nh_info    nexthop or nexthop group information
/// \return    SDK_RET_OK on sucess or error code in case of failure
sdk_ret_t
pds_mirror_session_update (pds_obj_key_t *key, nh_info_t *nh_info)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_mirror_session_info_t info;
    pds_batch_params_t batch_params;

    // add this entry to mirror session forwarding db
    g_mirror_session_fdb.add(*key, *nh_info);

    // now read the mirror session object
    memset(&info, 0, sizeof(info));
    ret = pds_mirror_session_read(key, &info);
    if (unlikely(ret != SDK_RET_OK)) {
        // may be session is deleted by now
        PDS_TRACE_ERR("Failed to read mirror session %s to update "
                      "reachability, err %u", key->str(), ret);
        return ret;
    }

    // setup batching with reserved epoch (for debugging)
    memset(&batch_params, 0, sizeof(batch_params));
    batch_params.epoch = PDS_INTERNAL_API_EPOCH_START;
    batch_params.async = true;
    // no need to pass callback as there is no action we can take there
    bctxt = pds_batch_start(&batch_params);

    // and post mirror session update
    ret = pds_mirror_session_update(&info.spec, bctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to update mirror session %s, err %u",
                      info.spec.key.str(), ret);
        pds_batch_destroy(bctxt);
        return ret;
    }
    return pds_batch_commit(bctxt);
}

/// \brief this API is invoked by metaswitch stubs to cleanup the forwarding
///        information about a ERSPAN mirror session thats been deleted
/// \param[in] key        mirror session key
/// \return    SDK_RET_OK on sucess or error code in case of failure
sdk_ret_t
pds_mirror_session_delete (pds_obj_key_t *key)
{
    return g_mirror_session_fdb.remove(*key);
}

/// \brief this API is invoked to get the reachability information for a erspan
///        collector while programming a mirror session in p4
/// \param[in] key         mirror session key
/// \param[out] nh_info    nexthop/nexthop-group key corresponding to the mirror
/// \return    SDK_RET_OK on sucess or error code in case of failure
sdk_ret_t
pds_mirror_session_nh (pds_obj_key_t *key, nh_info_t *nh_info)
{
    return g_mirror_session_fdb.find(*key, nh_info);
}

}    // namespace api
