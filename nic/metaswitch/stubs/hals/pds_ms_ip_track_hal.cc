// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// Purpose: Stitch tracked IP to underlay reachability in HAL

#include "nic/metaswitch/stubs/hals/pds_ms_ip_track_hal.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_defs.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/apollo/api/internal/pds_tep.hpp"
#include "nic/apollo/api/internal/pds_mirror.hpp"

namespace pds_ms {

// Call-back from MS underlay route update
sdk_ret_t
ip_track_reachability_change (const pds_obj_key_t& pds_obj_key,
                              const ip_addr_t& destip,
							  ms_hw_tbl_id_t nhgroup_id,
							  obj_id_t pds_obj_id) {

    bool op_delete = false;
    api::nh_info_t nhinfo;
    if (nhgroup_id != PDS_MS_ECMP_INVALID_INDEX) {
        nhinfo.nh_type = PDS_NH_TYPE_UNDERLAY_ECMP;
        nhinfo.nh_group = msidx2pdsobjkey(nhgroup_id, true /* underlay UUID */);
    } else {
        // Reachability lost - set blackhole nexthop
        nhinfo.nh_type = PDS_NH_TYPE_BLACKHOLE;
        op_delete = true;
    }

    // HAL does not take const keys
    auto key = pds_obj_key;

    switch (pds_obj_id) {
    case OBJ_ID_TEP:
        PDS_TRACE_DEBUG("++++ IP track UUID %s TEP %s reachability %s"
                        " to underlay NHgroup %d ++++",
                        pds_obj_key.str(), ipaddr2str(&destip),
                        (op_delete) ? "lost" : "change",
                        nhgroup_id);
        api::pds_tep_update(&key, &nhinfo);
        break;
    case OBJ_ID_MIRROR_SESSION:
        PDS_TRACE_DEBUG("++++ IP track UUID %s Mirror session %s"
                        " reachability %s to underlay NHgroup %d ++++",
                        pds_obj_key.str(), ipaddr2str(&destip),
                        (op_delete) ? "lost" : "change",
                        nhgroup_id);
        api::pds_mirror_session_update(&key, &nhinfo);
        break;
    default:
        SDK_ASSERT(0);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ip_track_reachability_delete (const pds_obj_key_t& pds_obj_key,
                              obj_id_t pds_obj_id) {

    // HAL does not take const keys
    auto key = pds_obj_key;

    // IP track object deleted
    if (pds_obj_id == OBJ_ID_MIRROR_SESSION) {
        api::pds_mirror_session_delete(&key);
    }
    return SDK_RET_OK;
}

} // End namespace
