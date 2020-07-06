// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __PDS_MS_IP_TRACK_HAL_HPP__
#define __PDS_MS_IP_TRACK_HAL_HPP__

#include "nic/metaswitch/stubs/common/pds_ms_defs.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_ip_track_store.hpp"
#include "nic/apollo/framework/api.h"
#include "nic/sdk/include/sdk/ip.hpp"

namespace pds_ms {

// Call-back from MS underlay route update
sdk_ret_t
ip_track_reachability_change (const pds_obj_key_t& pds_obj_key,
                              const ip_addr_t& destip,
							  ms_hw_tbl_id_t nhgroup_id,
							  obj_id_t pds_obj_id);
sdk_ret_t
ip_track_reachability_delete (const pds_obj_key_t& pds_obj_key,
                              obj_id_t pds_obj_id);

bool
ip_track_are_all_reachable (void);

void
ip_track_set_reachable (ip_track_obj_t* ip_track_obj, bool reachable);

} // End namespace

#endif

