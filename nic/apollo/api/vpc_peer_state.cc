//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vpc peer state handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/vpc_peer_state.hpp"

namespace api {

/// \defgroup PDS_VPC_PEER_STATE - vpc peering database functionality
/// \ingroup PDS_VPC
/// \@{

vpc_peer_state::vpc_peer_state() {
    vpc_peer_slab_ = slab::factory("vpc-peer", PDS_SLAB_ID_VPC_PEER,
                                   sizeof(vpc_peer_entry), 16,
                                   true, true, NULL);
    SDK_ASSERT(vpc_peer_slab() != NULL);
}

vpc_peer_state::~vpc_peer_state() {
    slab::destroy(vpc_peer_slab());
}

vpc_peer_entry *
vpc_peer_state::alloc(void) {
    return ((vpc_peer_entry *)vpc_peer_slab()->alloc());
}

void
vpc_peer_state::free(vpc_peer_entry *vpc_peer) {
    vpc_peer_slab()->free(vpc_peer);
}

/// \@}    // end of PDS_VPC_PEER_STATE

}    // namespace api
