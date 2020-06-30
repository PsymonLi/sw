//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vport state handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/vport_state.hpp"

namespace api {

/// \defgroup PDS_VPORT_STATE - vport database functionality
/// \ingroup PDS_VPORT
/// \@{

vport_state::vport_state() {
    vport_ht_ = ht::factory(PDS_MAX_VPORT >> 1,
                            vport_entry::vport_key_func_get,
                            sizeof(pds_obj_key_t));
    SDK_ASSERT(vport_ht_ != NULL);
    vport_slab_ = slab::factory("vport", PDS_SLAB_ID_VPORT, sizeof(vport_entry),
                                16, true, true, true, NULL);
    SDK_ASSERT(vport_slab_ != NULL);
}

vport_state::~vport_state() {
    ht::destroy(vport_ht_);
    slab::destroy(vport_slab_);
}

vport_entry *
vport_state::alloc(void) {
    return ((vport_entry *)vport_slab_->alloc());
}

sdk_ret_t
vport_state::insert(vport_entry *vport) {
    return vport_ht_->insert_with_key(&vport->key_, vport, &vport->ht_ctxt_);
}

vport_entry *
vport_state::remove(vport_entry *vport) {
    return (vport_entry *)(vport_ht_->remove(&vport->key_));
}

void
vport_state::free(vport_entry *vport) {
    vport_slab_->free(vport);
}

vport_entry *
vport_state::find(pds_obj_key_t *key) const {
    return (vport_entry *)(vport_ht_->lookup(key));
}

sdk_ret_t
vport_state::walk(state_walk_cb_t walk_cb, void *ctxt) {
    return vport_ht_->walk_safe(walk_cb, ctxt);
}

sdk_ret_t
vport_state::slab_walk(state_walk_cb_t walk_cb, void *ctxt) {
    walk_cb(vport_slab_, ctxt);
    return SDK_RET_OK;
}

/// \@}    // end of PDS_VPORT_STATE

}    // namespace api
