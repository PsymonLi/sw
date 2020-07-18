//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// pipeline global state implementation
///
//----------------------------------------------------------------------------

#ifndef __APULU_IMPL_STATE_HPP__
#define __APULU_IMPL_STATEHPP__

#include "nic/sdk/include/sdk/table.hpp"
#include "nic/sdk/lib/table/sltcam/sltcam.hpp"
#include "nic/sdk/lib/table/slhash/slhash.hpp"
#include "nic/sdk/lib/rte_indexer/rte_indexer.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/impl/clock/clock_sync.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/p4/include/apulu_defines.h"

using sdk::table::handle_t;
using sdk::table::slhash;

namespace api {
namespace impl {

/// \defgroup PDS_APULU_IMPL_STATE - pipeline global state functionality
/// \ingroup PDS_APULU
/// \@{

// forward declaration
class apulu_impl;

/// \brief pipeline global state
class apulu_impl_state : public state_base {
public:
    /// \brief constructor
    apulu_impl_state(pds_state *state);

    /// \brief destructor
    ~apulu_impl_state();

    /// \brief     API to get table stats
    /// \param[in] cb   callback to be called on stats
    /// \param[in] ctxt opaque ctxt passed to the callback
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t table_stats(debug::table_stats_get_cb_t cb, void *ctxt);

    /// \brief  API to initiate transaction over all the table manamgement
    ///         library instances
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t table_transaction_begin(void);

    /// \brief  API to end transaction over all the table manamgement
    ///         library instances
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t table_transaction_end(void);

    /// \brief accessors
    rte_indexer *nacl_idxr(void) { return nacl_idxr_; }
    rte_indexer *high_prio_nacl_idxr(void) { return high_prio_nacl_idxr_; }
    rte_indexer *copp_idxr(void) { return copp_idxr_; }
    rte_indexer *nat_idxr(void) { return nat_idxr_; }
    rte_indexer *dnat_idxr(void) { return dnat_idxr_; }
    slhash *lif_vlan_tbl(void) { return lif_vlan_tbl_; }
    sdk_ret_t nacl_dump(int fd);
    friend class apulu_impl;            ///< friend of apulu_impl_state

private:
    rte_indexer *nacl_idxr_;            ///< indexer for NACL table
    rte_indexer *high_prio_nacl_idxr_;  ///< indexer for DHCP relay NACLs
    rte_indexer *copp_idxr_;            ///< indexer for CoPP table
    rte_indexer *nat_idxr_;             ///< indexer for NAT table
    rte_indexer *dnat_idxr_;            ///< indexer for DNAT table
    slhash *lif_vlan_tbl_;              ///< (if, vlan) table
    pds_clock_sync clock_sync_;         ///< h/w and s/w clock sync state
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __APULU_IMPL_STATE_HPP__
