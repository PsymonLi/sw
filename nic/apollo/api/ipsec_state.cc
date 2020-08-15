//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec state handling
///
//----------------------------------------------------------------------------

#include "nic/infra/core/mem.hpp"
#include "nic/apollo/api/ipsec_state.hpp"

using namespace sdk;

namespace api {

ipsec_sa_state::ipsec_sa_state() {
    ipsec_sa_ht_ =
        ht::factory(PDS_MAX_IPSEC_SA >> 1,
                    ipsec_sa_entry::ipsec_sa_key_func_get,
                    sizeof(pds_obj_key_t));
    SDK_ASSERT(ipsec_sa_ht_ != NULL);
    ipsec_sa_slab_ = slab::factory("ipsec_sa", PDS_SLAB_ID_IPSEC,
                                   sizeof(ipsec_sa_entry), 16, true,
                                   true, true, NULL);
    SDK_ASSERT(ipsec_sa_slab_ != NULL);
}

ipsec_sa_state::~ipsec_sa_state() {
    ht::destroy(ipsec_sa_ht_);
    slab::destroy(ipsec_sa_slab_);
}

ipsec_sa_entry *
ipsec_sa_state::alloc(void) {
    return ((ipsec_sa_entry *)ipsec_sa_slab_->alloc());
}

sdk_ret_t
ipsec_sa_state::insert(ipsec_sa_entry *ipsec_sa) {
    return ipsec_sa_ht_->insert_with_key(&ipsec_sa->key_, ipsec_sa,
                                         &ipsec_sa->ht_ctxt_);
}

ipsec_sa_entry *
ipsec_sa_state::remove(ipsec_sa_entry *ipsec_sa) {
    return (ipsec_sa_entry *)(ipsec_sa_ht_->remove(&ipsec_sa->key_));
}

void
ipsec_sa_state::free(ipsec_sa_entry *ipsec_sa) {
    ipsec_sa_slab_->free(ipsec_sa);
}

ipsec_sa_entry *
ipsec_sa_state::find(pds_obj_key_t *ipsec_sa_key) const {
    return (ipsec_sa_entry *)(ipsec_sa_ht_->lookup(ipsec_sa_key));
}

sdk_ret_t
ipsec_sa_state::walk(state_walk_cb_t walk_cb, void *ctxt) {
    return ipsec_sa_ht_->walk_safe(walk_cb, ctxt);
}

sdk_ret_t
ipsec_sa_state::slab_walk(state_walk_cb_t walk_cb, void *ctxt) {
    walk_cb(ipsec_sa_slab_, ctxt);
    return SDK_RET_OK;
}

}    // namespace api
