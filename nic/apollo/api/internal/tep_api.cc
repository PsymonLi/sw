//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines tep APIs for internal module interactions
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_tep.hpp"
#include "nic/apollo/api/tep.hpp"
#include "nic/apollo/api/internal/pds.hpp"
#include "nic/apollo/api/internal/pds_tep.hpp"

namespace api {

// copy TEP attributes and form TEP spec
// NOTE: skip copying nexthop information since that is what we are going to
//       update
static inline void
pds_tep_spec_from_tep (pds_tep_spec_t *spec, tep_entry *tep)
{
    spec->key = tep->key();
    spec->remote_ip = tep->ip();
    memcpy(spec->mac, tep->mac(), ETH_ADDR_LEN);
    spec->type = tep->type();
    spec->encap = tep->encap();
    spec->remote_svc = tep->remote_svc();
    spec->tos = tep->tos();
    spec->encrypt_sa = tep->ipsec_encrypt_sa();
    spec->decrypt_sa = tep->ipsec_decrypt_sa();
}

sdk_ret_t
pds_tep_update (pds_obj_key_t *key, nh_info_t *nh_info)
{
    sdk_ret_t ret;
    tep_entry *tep;
    pds_batch_ctxt_t bctxt;
    pds_tep_spec_t spec = { 0 };
    pds_batch_params_t batch_params;

    PDS_TRACE_VERBOSE("Rcvd TEP %s nh update, nh type %u, nh %s",
                      key->str(), nh_info->nh_type, nh_info->nh.str());

    tep = tep_db()->find(key);
    if (unlikely(tep == NULL)) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    // form the spec
    pds_tep_spec_from_tep(&spec, tep);

    // update the TEP with the new nexthop information
    spec.nh_type = nh_info->nh_type;
    switch (nh_info->nh_type) {
    case PDS_NH_TYPE_UNDERLAY:
        spec.nh = nh_info->nh;
        break;

    case PDS_NH_TYPE_UNDERLAY_ECMP:
        spec.nh_group = nh_info->nh_group;
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
    ret = pds_tep_update(&spec, bctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to update TEP %s, err %u", key->str(), ret);
        pds_batch_destroy(bctxt);
        return ret;
    }
    return pds_batch_commit(bctxt);
}

}    // namespace api
