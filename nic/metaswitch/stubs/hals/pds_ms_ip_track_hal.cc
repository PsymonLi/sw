// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// Purpose: Stitch tracked IP to underlay reachability in HAL

#include "nic/metaswitch/stubs/hals/pds_ms_ip_track_hal.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_defs.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/apollo/api/internal/pds_tep.hpp"

namespace pds_ms {

// Call-back from MS underlay route update
sdk_ret_t
ip_track_reachability_change (const pds_obj_key_t& pds_obj_key,
                              ip_addr_t& destip,
							  ms_hw_tbl_id_t nhgroup_id,
							  obj_id_t pds_obj_id) {

    api::nh_info_t nhinfo;
    nhinfo.nh_type = PDS_NH_TYPE_UNDERLAY_ECMP;
    nhinfo.nh_group = msidx2pdsobjkey(nhgroup_id, true /* underlay UUID */);

    // HAL does not take const keys
    auto key = pds_obj_key;

    switch (pds_obj_id) {
    case OBJ_ID_TEP:
        PDS_TRACE_DEBUG("++++ IP track UUID %s TEP %s reachability change"
                        " to underlay NHgroup %d ++++",
                        pds_obj_key.str(), ipaddr2str(&destip), nhgroup_id);
        api::pds_tep_update(&key, &nhinfo);
        break;
    case OBJ_ID_MIRROR_SESSION:
        PDS_TRACE_DEBUG("++++ IP track UUID %s Mirror session %s"
                        " reachability change to underlay NHgroup %d ++++",
                        pds_obj_key.str(), ipaddr2str(&destip), nhgroup_id);
        break;
    default:
        SDK_ASSERT(0);
    }
    return SDK_RET_OK;
}

} // End namespace
