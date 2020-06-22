//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines mirror session APIs for internal module interactions
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_mirror.hpp"
#include "nic/apollo/api/internal/pds.hpp"
#include "nic/apollo/api/internal/pds_mirror.hpp"

namespace api {

sdk_ret_t
pds_mirror_session_update (pds_obj_key_t *key, nh_info_t *nh_info)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_mirror_session_info_t info;
    pds_batch_params_t batch_params;

    // read the mirror session object
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

    // post mirror session update
    ret = pds_mirror_session_update(&info.spec, bctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to update mirror session %s, err %u",
                      info.spec.key.str(), ret);
        pds_batch_destroy(bctxt);
        return ret;
    }
    return pds_batch_commit(bctxt);
}

}    // namespace api
