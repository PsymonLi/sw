//------------------------------------------------------------------------------
// Copyright (c) 2019 Pensando Systems, Inc.
//------------------------------------------------------------------------------

#include <iostream>
#include <malloc.h>
#include <math.h>
#include "nic/apollo/agent/test/client/app.hpp"
#include "nic/apollo/test/scale/test_common.hpp"

static int g_epoch = 1;
static pds_batch_ctxt_t g_bctxt = PDS_BATCH_CTXT_INVALID;

static inline bool
pds_batching_enabled (void)
{
    if (getenv("BATCHING_DISABLED")) {
        return FALSE;
    }
    return TRUE;
}

sdk_ret_t
create_route_table (pds_route_table_spec_t *route_table)
{
    sdk_ret_t ret;

    ret = create_route_table_impl(route_table);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "create route table failed!, ret %u", ret);
    // Batching: push leftover objects
    ret = create_route_table_impl(NULL);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "create route table failed!, ret %u", ret);
    if (pds_batching_enabled()) {
        ret = batch_commit_impl(g_bctxt);
        SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                                "Batch commit failed!, ret %u", ret);
        g_bctxt = batch_start_impl(g_epoch++);
        SDK_ASSERT_TRACE_RETURN((g_bctxt != PDS_BATCH_CTXT_INVALID),
                                SDK_RET_ERR,
                                "Batch start failed!");
    }
    return ret;
}

sdk_ret_t
create_route (pds_route_spec_t *route)
{
    return create_route_impl(route);
}

sdk_ret_t
delete_route (pds_route_key_t *route)
{
    return delete_route_impl(route);
}

sdk_ret_t
get_heap_stats (struct mallinfo *minfo)
{
    return get_heap_stats_impl(minfo);
}

sdk_ret_t
create_svc_mapping (pds_svc_mapping_spec_t *svc_mapping)
{
    return create_svc_mapping_impl(svc_mapping);
}

sdk_ret_t
create_local_mapping (pds_local_mapping_spec_t *pds_local_mapping)
{
    return create_local_mapping_impl(pds_local_mapping);
}

sdk_ret_t
create_remote_mapping (pds_remote_mapping_spec_t *pds_remote_mapping)
{
    return create_remote_mapping_impl(pds_remote_mapping);
}

sdk_ret_t
delete_local_mapping (pds_mapping_key_t *key)
{
    return SDK_RET_OK;
}

sdk_ret_t
delete_remote_mapping (pds_mapping_key_t *key)
{
    return SDK_RET_OK;
}

sdk_ret_t
create_vnic (pds_vnic_spec_t *pds_vnic)
{
    return create_vnic_impl(pds_vnic);
}

sdk_ret_t
delete_vnic (pds_obj_key_t *key)
{
    return delete_vnic_impl(key);
}

sdk_ret_t
delete_nat_port_block (pds_obj_key_t *key)
{
    return delete_nat_port_block_impl(key);
}

sdk_ret_t
create_vpc (pds_vpc_spec_t *pds_vpc)
{
    return create_vpc_impl(pds_vpc);
}

sdk_ret_t
create_dhcp_policy (pds_dhcp_policy_spec_t *pds_dhcp)
{
    return create_dhcp_policy_impl(pds_dhcp);
}

sdk_ret_t
create_nat_port_block (pds_nat_port_block_spec_t *pds_napt)
{
    return create_nat_port_block_impl(pds_napt);
}

sdk_ret_t
read_vpc (pds_obj_key_t *key, pds_vpc_info_t *info)
{
    return read_vpc_impl(key, info);
}

sdk_ret_t
update_vpc (pds_vpc_spec_t *pds_vpc)
{
    return update_vpc_impl(pds_vpc);
}

sdk_ret_t
delete_vpc (pds_obj_key_t *key)
{
    return delete_vpc_impl(key);
}

sdk_ret_t
create_vpc_peer (pds_vpc_peer_spec_t *pds_vpc_peer)
{
    return create_vpc_peer_impl(pds_vpc_peer);
}

sdk_ret_t
create_l3_intf (pds_if_spec_t *pds_if)
{
    return create_l3_intf_impl(pds_if);
}

sdk_ret_t
create_tag (pds_tag_spec_t *pds_tag)
{
    return create_tag_impl(pds_tag);
}

sdk_ret_t
create_meter (pds_meter_spec_t *pds_meter)
{
    return create_meter_impl(pds_meter);
}

sdk_ret_t
create_policer (pds_policer_spec_t *pds_policer)
{
    return create_policer_impl(pds_policer);
}

sdk_ret_t
create_nexthop (pds_nexthop_spec_t *pds_nh)
{
    return create_nexthop_impl(pds_nh);
}

sdk_ret_t
create_nexthop_group (pds_nexthop_group_spec_t *pds_nhgroup)
{
    return create_nexthop_group_impl(pds_nhgroup);
}

sdk_ret_t
create_tunnel (uint32_t id, pds_tep_spec_t *pds_tep)
{
    return create_tunnel_impl(id, pds_tep);
}

sdk_ret_t
create_subnet (pds_subnet_spec_t *pds_subnet)
{
    return create_subnet_impl(pds_subnet);
}

sdk_ret_t
create_device (pds_device_spec_t *device)
{
    return create_device_impl(device);
}

sdk_ret_t
create_policy (pds_policy_spec_t *policy)
{
    sdk_ret_t ret;
    ret = create_policy_impl(policy);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "create policy failed!, ret %u", ret);
    ret = create_policy_impl(NULL);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "create policy failed!, ret %u", ret);
    return ret;
}

sdk_ret_t
create_mirror_session (pds_mirror_session_spec_t *ms)
{
    sdk_ret_t ret;
    ret = create_mirror_session_impl(ms);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "create mirror failed!, ret %u", ret);
    // push leftover objects
    ret = create_mirror_session_impl(NULL);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "create mirror failed!, ret %u", ret);
    return ret;
}

sdk_ret_t
create_objects_init (test_params_t *test_params)
{
    if (pds_batching_enabled()) {
        g_bctxt = batch_start_impl(g_epoch++);
        SDK_ASSERT_TRACE_RETURN((g_bctxt != PDS_BATCH_CTXT_INVALID),
                                SDK_RET_ERR,
                                "Batch start failed!");
    }
    return SDK_RET_OK;
}

sdk_ret_t
create_objects_end (void)
{
    sdk_ret_t ret;

    if (pds_batching_enabled()) {
        // TODO: hack until hooks is cleaned up
        (void)batch_start_impl(PDS_EPOCH_INVALID);
        ret = batch_commit_impl(g_bctxt);
        SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                                "Batch commit failed!, ret %u", ret);
    }
    return SDK_RET_OK;
}

sdk_ret_t
delete_objects_end (void)
{
    SDK_TRACE_DEBUG("flow delete not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
iterate_objects_end (sdk::table::iterate_t table_entry_iterate)
{
    SDK_TRACE_DEBUG("flow iteration not implemented");
    return SDK_RET_OK;
}
