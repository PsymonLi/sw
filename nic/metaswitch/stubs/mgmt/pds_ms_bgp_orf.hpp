//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS MS BGP ORF propagation at RR
//---------------------------------------------------------------

#ifndef __PDS_MS_BGP_ORF__
#define __PDS_MS_BGP_ORF__

#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_bgp_store.hpp"
#include <unordered_map>
#include <unordered_set>

namespace pds_ms {

struct bgp_orf_rr_peer_state_t {
    // pending list
    std::unordered_set<ms_rt_t, ms_rt_hash> pend_add;
    std::unordered_set<ms_rt_t, ms_rt_hash> pend_del;
    // committed list
    std::unordered_set<ms_rt_t, ms_rt_hash> committed;
};

struct bgp_orf_rr_state_t {
    // bgp peer -> rt lists
    std::unordered_map<bgp_peer_t, bgp_orf_rr_peer_state_t,
        bgp_peer_hash> peers;

    // rt -> list of bgp peers
    std::unordered_map<ms_rt_t,
                       std::unordered_set<bgp_peer_t, bgp_peer_hash>,
                       ms_rt_hash> orfs;
};

} // end namespace

#endif
