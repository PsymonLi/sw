//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// VNIC datapath database handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/api/impl/apollo/vnic_impl_state.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_VNIC_IMPL_STATE - VNIC database functionality
/// \ingroup PDS_VNIC
/// @{

vnic_impl_state::vnic_impl_state(pds_state *state) {
    sdk_table_factory_params_t    table_params;

    // allocate indexer for vnic hw id allocation
    vnic_idxr_ = indexer::factory(PDS_MAX_VNIC);
    SDK_ASSERT(vnic_idxr_ != NULL);

    // create a slab for vnic impl entries
    vnic_impl_slab_ =
        slab::factory("vnic-impl", PDS_SLAB_ID_VNIC_IMPL,
                      sizeof(vnic_impl), 16, true, true);
    SDK_ASSERT(vnic_impl_slab_ != NULL);
}

vnic_impl_state::~vnic_impl_state() {
    indexer::destroy(vnic_idxr_);
    slab::destroy(vnic_impl_slab_);
}

sdk_ret_t
vnic_impl_state::table_transaction_begin(void) {
    //vnic_idxr_->txn_start();
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl_state::table_transaction_end(void) {
    //vnic_idxr_->txn_end();
    return SDK_RET_OK;
}

/// \@}

}    // namespace impl
}    // namespace api
