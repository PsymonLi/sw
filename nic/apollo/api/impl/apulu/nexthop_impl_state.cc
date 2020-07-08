//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// nexthop datapath database handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/apollo/api/include/pds_nexthop.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_impl.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_impl_state.hpp"
#include "gen/p4gen/apulu/include/p4pd.h"

namespace api {
namespace impl {

/// \defgroup PDS_NEXTHOP_IMPL_STATE - nexthop database functionality
/// \ingroup PDS_NEXTHOP
/// \@{

nexthop_impl_state::nexthop_impl_state(pds_state *state) {
    p4pd_table_properties_t    tinfo;

    p4pd_global_table_properties_get(P4TBL_ID_NEXTHOP, &tinfo);
    // create indexer and reserve system default blackhole/drop nexthop entry
    nh_idxr_ = rte_indexer::factory(tinfo.tabledepth, true, true);
    SDK_ASSERT(nh_idxr_ != NULL);

    // reserve one nexthop for each uplink
    SDK_ASSERT(nh_idxr_->alloc_block(PDS_IMPL_UPLINK_NH_HW_ID_START,
                                     TM_UPLINK_PORT_END + 1,
                                     false) == SDK_RET_OK);
}

nexthop_impl_state::~nexthop_impl_state() {
    rte_indexer::destroy(nh_idxr_);
}

nexthop_impl *
nexthop_impl_state::alloc(void) {
    return (nexthop_impl *)
               SDK_CALLOC(SDK_MEM_ALLOC_PDS_NEXTHOP_IMPL,
                          sizeof(nexthop_impl));
}

void
nexthop_impl_state::free(nexthop_impl *impl) {
    SDK_FREE(SDK_MEM_ALLOC_PDS_NEXTHOP_IMPL, impl);
}

/// \@}    // end of PDS_NEXTHOP_IMPL_STATE

}    // namespace impl
}    // namespace api
