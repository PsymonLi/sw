//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// lif implementation state
///
//----------------------------------------------------------------------------

#ifndef __LIF_IMPL_STATE_HPP__
#define __LIF_IMPL_STATE_HPP__

#include "nic/sdk/lib/table/directmap/directmap.hpp"
#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_LIF_IMPL_STATE - lif state functionality
/// \ingroup PDS_LIF
/// \@{

// forward declarion
class lif_impl;

///< \brief    state maintained for lifs
class lif_impl_state : public state_base {
public:
    ///< \brief    constructor
    lif_impl_state(pds_state *state);

    ///< \@brief destructor
    ~lif_impl_state();

    /// \brief    allocate memory required for a lif instance
    /// \return pointer to the allocated lif, NULL if no memory
    lif_impl *alloc(void) {
        return (lif_impl *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_LIF_IMPL,
                                      sizeof(lif_impl));
    }

    /// \brief    insert given lif instance into the lif db
    /// \param[in] impl    lif to be added to the db
    /// \return   SDK_RET_OK on success, failure status code on error
    sdk_ret_t insert(lif_impl *impl) {
        return lif_ht_->insert_with_key(&impl->key_, impl, &impl->ht_ctxt_);
    }

    /// \brief     remove the given instance of lif object from db
    /// \param[in] impl    lif entry to be deleted from the db
    /// \return    pointer to the removed lif instance or NULL,
    ///            if not found
    lif_impl *remove(lif_impl *impl) {
        return (lif_impl *)(lif_ht_->remove(&impl->key_));
    }

    /// \brief      free lif impl instance
    /// \param[in]  impl   pointer to the allocated lif impl instance
    void free(lif_impl *impl) {
        SDK_FREE(SDK_MEM_ALLOC_PDS_LIF_IMPL, impl);
    }

    /// \brief     lookup a lif in database given the key
    /// \param[in] key    lif key
    /// \return pointer to the lif impl instance or NULL if not found
    lif_impl *find(pds_lif_key_t *key) const {
        return (lif_impl *)(lif_ht_->lookup(key));
    }

    /// \brief     lookup a lif in database given its type, if multiple lifs
    ///            exist for given type the 1st one encountered will be returned
    /// \param[in] key    lif key
    /// \return pointer to the lif impl instance or NULL if not found
    lif_impl *find(lif_type_t type) {
        lif_find_cb_ctxt_t lif_cb_ctxt;

        lif_cb_ctxt.lif = NULL;
        lif_cb_ctxt.type = type;
        lif_ht_->walk(lif_type_compare_cb_, &lif_cb_ctxt);
        return lif_cb_ctxt.lif;
    }

    /// \brief API to walk all the db elements
    /// \param[in] walk_cb    callback to be invoked for every node
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t walk(state_walk_cb_t walk_cb, void *ctxt) {
        return lif_ht_->walk(walk_cb, ctxt);
    }

private:
    directmap *tx_rate_limiter_tbl(void) { return tx_rate_limiter_tbl_; }

    typedef struct lif_find_cb_ctxt_s {
        lif_impl *lif;
        lif_type_t type;
    } lif_find_cb_ctxt_t;

    static bool lif_type_compare_cb_(void *entry, void *ctxt) {
        lif_impl *lif = (lif_impl *)entry;
        lif_find_cb_ctxt_t *cb_ctxt = (lif_find_cb_ctxt_t *)ctxt;

        if (lif->type() == cb_ctxt->type) {
            // break the walk
            cb_ctxt->lif = lif;
            return true;
        }
        return false;
    }

    friend class lif_impl;    // lif_impl class is friend of lif_impl_state

private:
    ht           *lif_ht_;
    directmap    *tx_rate_limiter_tbl_;
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __LIF_IMPL_STATE_HPP__
