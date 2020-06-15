//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vport implementation state
///
//----------------------------------------------------------------------------

#ifndef __VPORT_IMPL_STATE_HPP__
#define __VPORT_IMPL_STATE_HPP__

#include "nic/sdk/lib/rte_indexer/rte_indexer.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/pds_state.hpp"

using sdk::table::sdk_table_factory_params_t;

namespace api {
namespace impl {

/// \defgroup PDS_VPORT_IMPL_STATE - VPORT state functionality
/// \ingroup PDS_VPORT
/// @{

// forward declaration
class vport_impl;

/// \brief state maintained for vports
class vport_impl_state : public state_base {
public:
    /// \brief constructor
    vport_impl_state(pds_state *state);

    /// \brief destructor
    ~vport_impl_state();

    /// \brief  allocate memory required for a vport impl instance
    /// \return pointer to the allocated instance, NULL if no memory
    vport_impl *alloc(void);

    /// \brief     free vport impl instance back
    /// \param[in] impl pointer to the allocated impl instance
    void free(vport_impl *impl);

    /// \brief  API to initiate transaction over all the table manamgement
    ///         library instances
    /// \return SDK_RET_OK on success, failure status code on error
    sdk_ret_t table_transaction_begin(void);

    /// \brief  API to end transaction over all the table manamgement
    ///         library instances
    /// \return SDK_RET_OK on success, failure status code on error
    sdk_ret_t table_transaction_end(void);

    /// \brief     API to get table stats
    /// \param[in]  cb      callback to be called on stats
    ///             ctxt    opaque ctxt passed to the callback
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t table_stats(debug::table_stats_get_cb_t cb, void *ctxt);

    /// \brief API to walk all the slabs
    /// \param[in] walk_cb    callback to be invoked for every slab
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t slab_walk(state_walk_cb_t walk_cb, void *ctxt) override;

private:
    /// h/w table indexer for vports
    rte_indexer *vport_idxr(void) { return vport_idxr_; }
    /// slab instance to allocate/free vports
    slab *vport_impl_slab(void) { return vport_impl_slab_; }
    /// vport_impl class is friend of vport_impl_state
    friend class vport_impl;

private:
    /// slab for vport impl instances
    slab *vport_impl_slab_;
    /// indexer to allocate h/w vport id
    rte_indexer  *vport_idxr_;
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __VPORT_IMPL_STATE_HPP__
