//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __INTERFACE_LLDP_HPP__
#define __INTERFACE_LLDP_HPP__

#include "gen/proto/interface.pb.h"

using intf::UplinkResponseInfo;
using intf::LldpIfStats;

namespace hal {

hal_ret_t hal_lldp_config_init(void);

sdk_ret_t interface_lldp_status_get(uint16_t uplink_idx, 
                                    UplinkResponseInfo *rsp);
sdk_ret_t interface_lldp_stats_get(uint16_t uplink_idx, 
                                   LldpIfStats *lldp_stats);
} // namespace hal
#endif // __INTERFACE_LLDP_HPP__
