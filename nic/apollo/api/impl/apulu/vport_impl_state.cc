//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vport datapath database handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_vport.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/vport_impl.hpp"
#include "nic/apollo/api/impl/apulu/vport_impl_state.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_VPORT_IMPL_STATE - vport database functionality
/// \ingroup PDS_VPORT
/// @{

vport_impl_state::vport_impl_state(pds_state *state) {
    p4pd_table_properties_t tinfo;
    sdk_table_factory_params_t table_params;

    // create a slab for vport impl entries
    vport_impl_slab_ =
        slab::factory("vport-impl", PDS_SLAB_ID_VPORT_IMPL,
                      sizeof(vport_impl), 16, true, true, true, NULL);
    SDK_ASSERT(vport_impl_slab_ != NULL);
}

vport_impl_state::~vport_impl_state() {
    slab::destroy(vport_impl_slab_);
}

vport_impl *
vport_impl_state::alloc(void) {
    return (vport_impl *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_VPORT_IMPL,
                                   sizeof(vport_impl));
}

void
vport_impl_state::free(vport_impl *impl) {
    SDK_FREE(SDK_MEM_ALLOC_PDS_VPORT_IMPL, impl);
}

sdk_ret_t
vport_impl_state::table_transaction_begin(void) {
    return SDK_RET_OK;
}

sdk_ret_t
vport_impl_state::table_transaction_end(void) {
    return SDK_RET_OK;
}

sdk_ret_t
vport_impl_state::slab_walk(state_walk_cb_t walk_cb, void *ctxt) {
    walk_cb(vport_impl_slab_, ctxt);
    return SDK_RET_OK;
}

/// \@}

}    // namespace impl
}    // namespace api
