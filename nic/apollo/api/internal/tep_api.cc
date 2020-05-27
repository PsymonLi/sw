//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines tep APIs for internal module interactions
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_tep.hpp"
#include "nic/apollo/api/internal/pds.hpp"
#include "nic/apollo/api/internal/pds_tep.hpp"

namespace api {

sdk_ret_t
pds_tep_update (pds_obj_key_t *key, nh_info_t *nh_info)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_tep_info_t info = { 0 };
    pds_batch_params_t batch_params;

    // fetch the TEP entry
    ret = pds_tep_read(key, &info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to read TEP %s, err %u", key->str(), ret);
        return ret;
    }

    // update the TEP with the new nexthop information
    info.spec.nh_type = nh_info->nh_type;
    switch (nh_info->nh_type) {
    case PDS_NH_TYPE_UNDERLAY:
        info.spec.nh = nh_info->nh;;
        break;

    case PDS_NH_TYPE_UNDERLAY_ECMP:
        info.spec.nh_group = nh_info->nh_group;
        break;

    case PDS_NH_TYPE_BLACKHOLE:
        // no data associated with this type of NH
        break;

    default:
        PDS_TRACE_ERR("Unsupported nh type %u in underlay TEP %s update",
                      nh_info->nh_type, key->str());
        return SDK_RET_INVALID_ARG;
    }

    // setup batching with reserved epoch (for debugging)
    memset(&batch_params, 0, sizeof(batch_params));
    batch_params.epoch = PDS_INTERNAL_API_EPOCH_START;
    batch_params.async = true;
    // no need to pass callback as there is no action we can take there
    bctxt = pds_batch_start(&batch_params);

    // update the TEP with this new spec
    ret = pds_tep_update(&info.spec, bctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to update TEP %s, err %u", key->str(), ret);
        pds_batch_destroy(bctxt);
        return ret;
    }
    return pds_batch_commit(bctxt);
}

}    // namespace api
