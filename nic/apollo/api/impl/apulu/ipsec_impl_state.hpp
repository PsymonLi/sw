//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec sa implementation state
///
//----------------------------------------------------------------------------

#ifndef __IPSEC_IMPL_STATE_HPP__
#define __IPSEC_IMPL_STATE_HPP__

#include "nic/sdk/lib/rte_indexer/rte_indexer.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "gen/p4gen/p4/include/ftl_table.hpp"

using sdk::table::sdk_table_factory_params_t;

namespace api {
namespace impl {

/// \defgroup PDS_IPSEC_IMPL_STATE - ipsec state functionality
/// \ingroup PDS_IPSEC
/// @{

// forward declaration
class ipsec_sa_impl;

/// \brief state maintained for ipsec sa
class ipsec_sa_impl_state : public state_base {
public:
    /// \brief constructor
    ipsec_sa_impl_state(pds_state *state);

    /// \brief destructor
    ~ipsec_sa_impl_state();

    /// \brief  allocate memory required for a ipsec sa impl instance
    /// \return pointer to the allocated instance, NULL if no memory
    ipsec_sa_impl *alloc(void);

    /// \brief     free ipsec sa impl instance back
    /// \param[in] impl pointer to the allocated impl instance
    void free(ipsec_sa_impl *impl);

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

    /// \brief      API to find ipsec sa impl obj using hw id
    /// \return     ipsec sa impl object
    ipsec_sa_impl *find(uint16_t hw_id);

    /// \brief      API to insert ipsec sa impl into hash table
    /// \param[in]  key     ipsec sa key
    /// \param[in]  impl    ipsec sa impl object
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t insert(uint16_t hw_id, ipsec_sa_impl *impl);

    /// \brief      API to update ipsec sa impl in the hash table
    /// \param[in]  key     ipsec sa key
    /// \param[in]  impl    ipsec sa impl object to be updated with
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t update(uint16_t hw_id, ipsec_sa_impl *impl);

    /// \brief      API to remove hw id and ipsec sa key from the hash table
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t remove(uint16_t hw_id);

    sdk::table::ftl_base *flow_tbl(void) { return flow_tbl_; }

private:
    /// h/w table indexer for ipsec sa
    rte_indexer *ipsec_sa_encrypt_idxr(void) { return ipsec_sa_encrypt_idxr_; }
    rte_indexer *ipsec_sa_decrypt_idxr(void) { return ipsec_sa_decrypt_idxr_; }
    /// slab instance to allocate/free ipsec sa
    slab *ipsec_sa_impl_slab(void) { return ipsec_sa_impl_slab_; }
    ht *impl_ht(void) const { return impl_ht_; }
    /// ipsec_sa_impl class is friend of ipsec_sa_impl_state
    friend class ipsec_sa_impl;

private:
    /// slab for ipsec sa impl instances
    slab *ipsec_sa_impl_slab_;
    /// indexer to allocate h/w ipsec sa id
    rte_indexer  *ipsec_sa_encrypt_idxr_;
    rte_indexer  *ipsec_sa_decrypt_idxr_;
    sdk::table::ftl_base *flow_tbl_;
    ///< hash table for hw_id to ipsec sa key
    ht *impl_ht_;
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __IPSEC_IMPL_STATE_HPP__
