//------------------------------------------------------------------------------
// Copyright (c) 2019 Pensando Systems, Inc.
//------------------------------------------------------------------------------

#ifndef __SCALE_API_HPP_
#define __SCALE_API_HPP_

#include "nic/sdk/include/sdk/table.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_tep.hpp"
#include "nic/apollo/api/include/pds_vpc.hpp"
#include "nic/apollo/api/include/pds_subnet.hpp"
#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/api/include/pds_mapping.hpp"
#include "nic/apollo/api/include/pds_mirror.hpp"
#include "nic/apollo/api/include/pds_meter.hpp"
#include "nic/apollo/api/include/pds_policy.hpp"
#include "nic/apollo/api/include/pds_service.hpp"
#include "nic/apollo/api/include/pds_tag.hpp"
#include "nic/apollo/api/include/pds_device.hpp"
#include "nic/apollo/api/include/pds_dhcp.hpp"
#include "nic/apollo/api/include/pds_nat.hpp"
#include "nic/apollo/api/internal/pds_mapping.hpp"
#include "nic/apollo/test/scale/test_common.hpp"
#include <malloc.h>

sdk_ret_t create_route_table(pds_route_table_spec_t *route_table);
sdk_ret_t create_route(pds_route_spec_t *route);
sdk_ret_t delete_route(pds_route_key_t *route);
sdk_ret_t get_heap_stats(struct mallinfo *minfo);
sdk_ret_t create_svc_mapping(pds_svc_mapping_spec_t *svc_mapping);
sdk_ret_t create_local_mapping(pds_local_mapping_spec_t *pds_local_mapping);
sdk_ret_t create_remote_mapping(pds_remote_mapping_spec_t *pds_remote_mapping);
sdk_ret_t create_vnic(pds_vnic_spec_t *pds_vnic);
sdk_ret_t delete_vnic(pds_obj_key_t *key);
sdk_ret_t create_subnet(pds_subnet_spec_t *pds_subnet);
sdk_ret_t create_vpc(pds_vpc_spec_t *pds_vpc);
sdk_ret_t create_nat_port_block(pds_nat_port_block_spec_t *pds_napt);
sdk_ret_t delete_nat_port_block(pds_obj_key_t *key);
sdk_ret_t read_vpc(pds_obj_key_t *key, pds_vpc_info_t *info);
sdk_ret_t update_vpc(pds_vpc_spec_t *pds_vpc);
sdk_ret_t delete_vpc(pds_obj_key_t *key);
sdk_ret_t create_vpc_peer(pds_vpc_peer_spec_t *pds_vpc_peer);
sdk_ret_t create_dhcp_policy(pds_dhcp_policy_spec_t *pds_dhcp);
sdk_ret_t create_tag(pds_tag_spec_t *pds_tag);
sdk_ret_t create_meter(pds_meter_spec_t *pds_meter);
sdk_ret_t create_policer(pds_policer_spec_t *pds_policer);
sdk_ret_t create_nexthop(pds_nexthop_spec_t *pds_nh);
sdk_ret_t create_nexthop_group(pds_nexthop_group_spec_t *pds_nhgroup);
sdk_ret_t create_tunnel(uint32_t id, pds_tep_spec_t *pds_tep);
sdk_ret_t create_device(pds_device_spec_t *device);
sdk_ret_t create_policy(pds_policy_spec_t *policy);
sdk_ret_t create_mirror_session(pds_mirror_session_spec_t *ms);
sdk_ret_t create_l3_intf(pds_if_spec_t *intf);
sdk_ret_t create_objects_init(test_params_t *test_params);
sdk_ret_t create_objects_end(void);
sdk_ret_t delete_objects_end(void);
sdk_ret_t iterate_objects_end(sdk::table::iterate_t table_entry_iterate);

sdk_ret_t delete_local_mapping(pds_mapping_key_t *key);
sdk_ret_t delete_remote_mapping(pds_mapping_key_t *key);

#endif
