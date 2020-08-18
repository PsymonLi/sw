//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// pipeline global state implementation
///
//----------------------------------------------------------------------------

#ifndef __ATHENA_IMPL_STATE_HPP__
#define __ATHENA_IMPL_STATEHPP__

#include "nic/sdk/include/sdk/table.hpp"
#include "nic/sdk/lib/table/sltcam/sltcam.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/athena/p4/include/defines.h"

namespace api {
namespace impl {

/// \defgroup PDS_ATHENA_IMPL_STATE - pipeline global state functionality
/// \ingroup PDS_ATHENA
/// \@{

// forward declaration
class athena_impl;

#define MAX_KEY_NATIVE_TBL_ENTRIES   3
#define MAX_KEY_TUNNELED_TBL_ENTRIES 3
#define MAX_EGRESS_KEY_NATIVE_TBL_ENTRIES   6
#define MAX_EGRESS_KEY_TUNNELED_TBL_ENTRIES 6

/// \brief pipeline global state
class athena_impl_state : public state_base {
public:
    /// \brief constructor
    athena_impl_state(pds_state *state);

    /// \brief destructor
    ~athena_impl_state();

    /// \brief accessors
    sltcam *key_native_tbl(void) { return key_native_tbl_; };
    sltcam *key_tunneled_tbl(void) { return key_tunneled_tbl_; };
    sltcam *egress_key_native_tbl(void) { return egress_key_native_tbl_; };
    sltcam *egress_key_tunneled_tbl(void) { return egress_key_tunneled_tbl_; };
    sltcam *ingress_drop_stats_tbl(void) { return ingress_drop_stats_tbl_; }
    sltcam *egress_drop_stats_tbl(void) { return egress_drop_stats_tbl_; }
    sltcam *nacl_tbl(void) { return nacl_tbl_; }
    sltcam *checksum_tbl(void) { return checksum_tbl_; }

    friend class athena_impl;         ///< friend of athena_impl_state

private:
    sltcam *key_native_tbl_;            ///< key table for native packets
    sltcam *key_tunneled_tbl_;          ///< key table for tunneled packets
    sltcam *egress_key_native_tbl_;            ///< egress_key table for native packets
    sltcam *egress_key_tunneled_tbl_;          ///< egress_key table for tunneled packets
    sltcam *ingress_drop_stats_tbl_;    ///< ingress drop stats table
    sltcam *egress_drop_stats_tbl_;     ///< egress drop stats table
    sltcam *nacl_tbl_;                  ///< NACL tcam table
    sltcam *checksum_tbl_;              ///< Checksum tcam table
    sdk::table::handle_t key_native_tbl_hdls_[MAX_KEY_NATIVE_TBL_ENTRIES];
    sdk::table::handle_t key_tunneled_tbl_hdls_[MAX_KEY_TUNNELED_TBL_ENTRIES];
    sdk::table::handle_t egress_key_native_tbl_hdls_[MAX_EGRESS_KEY_NATIVE_TBL_ENTRIES];
    sdk::table::handle_t egress_key_tunneled_tbl_hdls_[MAX_EGRESS_KEY_TUNNELED_TBL_ENTRIES];
    sdk::table::handle_t ingress_drop_stats_tbl_hdls_[P4I_DROP_REASON_MAX + 1];
    sdk::table::handle_t egress_drop_stats_tbl_hdls_[P4E_DROP_REASON_MAX + 1];
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __ATHENA_IMPL_STATE_HPP__
