//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec sa datapath database handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_ipsec.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/ipsec_impl.hpp"
#include "nic/apollo/api/impl/apulu/ipsec_impl_state.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_ipsec_sa_impl_state - ipsec database functionality
/// \ingroup PDS_IPSEC
/// @{

ipsec_sa_impl_state::ipsec_sa_impl_state(pds_state *state) {
    // allocate indexer for ipsec hw id allocation and reserve 0th entry
    ipsec_sa_encrypt_idxr_ = rte_indexer::factory(PDS_MAX_IPSEC_SA, true, true);
    ipsec_sa_decrypt_idxr_ = rte_indexer::factory(PDS_MAX_IPSEC_SA, true, true);
    SDK_ASSERT(ipsec_sa_encrypt_idxr_ != NULL);
    SDK_ASSERT(ipsec_sa_decrypt_idxr_ != NULL);

    // create a slab for ipsec sa impl entries
    ipsec_sa_impl_slab_ =
        slab::factory("ipsec-impl", PDS_SLAB_ID_IPSEC_IMPL,
                      sizeof(ipsec_sa_impl), 16, true, true, true, NULL);
    SDK_ASSERT(ipsec_sa_impl_slab_ != NULL);

    // create ht for ipsec sa id to key mapping
    impl_ht_ = ht::factory(PDS_MAX_IPSEC_SA >> 2,
                           ipsec_sa_impl::key_get,
                           sizeof(uint16_t));
    SDK_ASSERT(impl_ht_ != NULL);

    // IPSEC flow table
    sdk_table_factory_params_t factory_params = { 0 };
    flow_tbl_ = flow_hash::factory(&factory_params);
}

ipsec_sa_impl_state::~ipsec_sa_impl_state() {
    rte_indexer::destroy(ipsec_sa_encrypt_idxr_);
    rte_indexer::destroy(ipsec_sa_decrypt_idxr_);
    slab::destroy(ipsec_sa_impl_slab_);
    ht::destroy(impl_ht_);
}

ipsec_sa_impl *
ipsec_sa_impl_state::alloc(void) {
    return (ipsec_sa_impl *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_IPSEC_SA_IMPL,
                                       sizeof(ipsec_sa_impl));
}

void
ipsec_sa_impl_state::free(ipsec_sa_impl *impl) {
    SDK_FREE(SDK_MEM_ALLOC_PDS_IPSEC_SA_IMPL, impl);
}

sdk_ret_t
ipsec_sa_impl_state::table_transaction_begin(void) {
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_impl_state::table_transaction_end(void) {
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_impl_state::slab_walk(state_walk_cb_t walk_cb, void *ctxt) {
    walk_cb(ipsec_sa_impl_slab_, ctxt);
    return SDK_RET_OK;
}

ipsec_sa_impl *
ipsec_sa_impl_state::find(uint16_t hw_id) {
    return (ipsec_sa_impl *)impl_ht_->lookup(&hw_id);
}

sdk_ret_t
ipsec_sa_impl_state::insert(uint16_t hw_id, ipsec_sa_impl *impl) {
    impl_ht_->insert_with_key(&hw_id, impl, impl->ht_ctxt());
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_impl_state::update(uint16_t hw_id, ipsec_sa_impl *impl) {
    impl_ht_->remove(&hw_id);
    impl_ht_->insert_with_key(&hw_id, impl, impl->ht_ctxt());
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_impl_state::remove(uint16_t hw_id) {
    ipsec_sa_impl *ipsec_sa;

    ipsec_sa = (ipsec_sa_impl *)impl_ht_->remove(&hw_id);
    if (!ipsec_sa) {
        PDS_TRACE_ERR("Failed to find ipsec_sa impl for hw id %u", hw_id);
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    return SDK_RET_OK;
}

/// \@}

}    // namespace impl
}    // namespace api
