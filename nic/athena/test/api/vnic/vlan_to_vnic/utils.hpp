//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cinttypes>
#include "nic/athena/api/include/pds_vnic.h"

#define UPDATE_DELTA 12

void fill_vlan_to_vnic_key(pds_vlan_to_vnic_map_key_t *key, uint32_t index);
void fill_vlan_to_vnic_data(pds_vlan_to_vnic_map_data_t *data, uint32_t index);
void update_vlan_to_vnic_data(pds_vlan_to_vnic_map_data_t *data,
                              uint32_t index);

#endif // __UTILS_HPP__
