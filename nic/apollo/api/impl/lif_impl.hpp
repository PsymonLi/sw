//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// lif datapath implementation
///
//----------------------------------------------------------------------------

#ifndef __LIF_IMPL_HPP__
#define __LIF_IMPL_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/sdk/include/sdk/qos.hpp"
#include "nic/sdk/platform/devapi/devapi_types.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_lif.hpp"

namespace api {
namespace impl {

// forward declaration
class lif_impl_state;

/// \defgroup PDS_LIF_IMPL - lif entry datapath implementation
/// \ingroup PDS_LIF
/// \@{
/// \brief LIF implementation

class lif_impl : public impl_base {
public:
    /// \brief  factory method to allocate and initialize a lif entry
    /// \param[in] spec    lif configuration parameters
    /// \return    new instance of lif or NULL, in case of error
    static lif_impl *factory(pds_lif_spec_t *spec);

    /// \brief  release all the s/w state associate with the given lif,
    ///         if any, and free the memory
    /// \param[in] impl    lif impl instance to be freed
    ///                  NOTE: h/w entries should have been cleaned up (by
    ///                  calling impl->cleanup_hw() before calling this
    static void destroy(lif_impl *impl);

    /// \brief     helper function to get key given lif entry
    /// \param[in] entry    pointer to lif instance
    /// \return    pointer to the lif instance's key
    static void *lif_key_func_get(void *entry) {
        lif_impl *lif = (lif_impl *)entry;
        return (void *)&(lif->key_);
    }

    /// \brief   helper function to get size of key
    /// \return  size of key
    static uint32_t key_size(void) {
        return sizeof(pds_lif_key_t);
    }

    ///< \brief    program lif tx policer for given lif
    ///< param[in] lif_id     h/w lif id
    ///< param[in] policer    policer parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    static sdk_ret_t program_tx_policer(uint32_t lif_id,
                                        sdk::policer_t *policer);

    /// \brief     ifindex this lif is pinned to
    /// \return    pinned interface index
    pds_ifindex_t pinned_ifindex(void) const {
        return pinned_if_idx_;
    }

    /// \brief     get function for lif type
    /// \return    lif type
    lif_type_t type(void);

    /// \brief     get function for lif key
    /// \return    key
    pds_lif_key_t key(void);

    /// \brief    handle all programming during lif creation
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create(pds_lif_spec_t *spec);

private:
    ///< constructor
    ///< \param[in] spec    lif configuration parameters
    lif_impl(pds_lif_spec_t *spec);

    ///< destructor
    ~lif_impl() {}

    ///< \brief    program necessary h/w entries for oob mnic lif
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_oob_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary h/w entries for inband mnic lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_inb_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary h/w entries for flow miss lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_flow_miss_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for internal mgmt. mnic lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_internal_mgmt_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for host (data) lifs
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_host_lif_(pds_lif_spec_t *spec);

private:
    pds_lif_key_t    key_;            ///< (s/w & h/w) lif id
    pds_ifindex_t    pinned_if_idx_;  ///< pinnned if index, if any
    lif_type_t       type_;           ///< type of the lif
    ht_ctxt_t        ht_ctxt_;        ///< hash table context
    // TODO: we can have state per pipeline in this class
    //       ideally, we should have the concrete class inside pipeline specific
    //       dir and this should be a base class !!
    uint32_t         nh_idx_;         ///< nexthop idx of this lif
    uint16_t         vnic_hw_id_;     ///< vnic hw id
    friend class lif_impl_state;      ///< lif_impl_state is friend of lif_impl
} __PACK__;

/// \@}

}    // namespace impl
}    // namespace api

#endif    /** __LIF_IMPL_HPP__ */
