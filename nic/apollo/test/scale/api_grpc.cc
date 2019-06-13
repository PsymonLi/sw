//------------------------------------------------------------------------------
// Copyright (c) 2019 Pensando Systems, Inc.
//------------------------------------------------------------------------------

#include <iostream>
#include <math.h>
#include "nic/apollo/agent/test/scale/app.hpp"
#include "nic/apollo/test/scale/test_common.hpp"

static int g_epoch = 1;

sdk_ret_t
create_route_table (pds_route_table_spec_t *route_table)
{
    sdk_ret_t rv;

    rv = create_route_table_grpc(route_table);
    if (rv != SDK_RET_OK) {
        return rv;
    }
    // Batching: push leftover objects
    rv = create_route_table_grpc(NULL);
    if (rv != SDK_RET_OK) {
        return rv;
    }
    rv = batch_commit_grpc();
    if (rv != SDK_RET_OK) {
        printf("%s: Batch commit failed!\n", __FUNCTION__);
        return SDK_RET_ERR;
    }
    rv = batch_start_grpc(g_epoch++);
    if (rv != SDK_RET_OK) {
        printf("%s: Batch start failed!\n", __FUNCTION__);
        return SDK_RET_ERR;
    }

    return rv;
}

sdk_ret_t
create_svc_mapping (pds_svc_mapping_spec_t *svc_mapping)
{
    return create_svc_mapping_grpc(svc_mapping);
}

sdk_ret_t
create_local_mapping (pds_local_mapping_spec_t *pds_local_mapping)
{
    return create_local_mapping_grpc(pds_local_mapping);
}

sdk_ret_t
create_remote_mapping (pds_remote_mapping_spec_t *pds_remote_mapping)
{
    return create_remote_mapping_grpc(pds_remote_mapping);
}

sdk_ret_t
create_vnic (pds_vnic_spec_t *pds_vnic)
{
    return create_vnic_grpc(pds_vnic);
}

sdk_ret_t
create_vpc (pds_vpc_spec_t *pds_vpc)
{
    return create_vpc_grpc(pds_vpc);
}

sdk_ret_t
create_vpc_peer (pds_vpc_peer_spec_t *pds_vpc_peer)
{
    return create_vpc_peer_grpc(pds_vpc_peer);
}

sdk_ret_t
create_tag (pds_tag_spec_t *pds_tag)
{
    return create_tag_grpc(pds_tag);
}

sdk_ret_t
create_meter (pds_meter_spec_t *pds_meter)
{
    return create_meter_grpc(pds_meter);
}

sdk_ret_t
create_nexthop (pds_nexthop_spec_t *pds_nh)
{
    return create_nexthop_grpc(pds_nh);
}

sdk_ret_t
create_tunnel (uint32_t id, pds_tep_spec_t *pds_tep)
{
    return create_tunnel_grpc(id, pds_tep);
}

sdk_ret_t
create_subnet (pds_subnet_spec_t *pds_subnet)
{
    return create_subnet_grpc(pds_subnet);
}

sdk_ret_t
create_device (pds_device_spec_t *device)
{
    return create_device_grpc(device);
}

sdk_ret_t
create_policy (pds_policy_spec_t *policy)
{
    sdk_ret_t rv;
    rv = create_policy_grpc(policy);
    if (rv != SDK_RET_OK) {
        return rv;
    }
    rv = create_policy_grpc(NULL);
    return rv;
}

sdk_ret_t
create_mirror_session (pds_mirror_session_spec_t *ms)
{
    sdk_ret_t rv;
    rv = create_mirror_session_grpc(ms);
    if (rv != SDK_RET_OK) {
        return rv;
    }
    // push leftover objects
    rv = create_mirror_session_grpc(NULL);
    return rv;
}

sdk_ret_t
create_objects_init (test_params_t *test_params)
{
    sdk_ret_t ret;

    /* BATCH START */
    ret = batch_start_grpc(g_epoch++);
    if (ret != SDK_RET_OK) {
        printf("%s: Batch start failed!\n", __FUNCTION__);
        return SDK_RET_ERR;
    }

    return ret;
}

sdk_ret_t
create_objects_end ()
{
    sdk_ret_t ret;

    ret = batch_start_grpc(PDS_EPOCH_INVALID);
    if (ret != SDK_RET_OK) {
        printf("%s: Batch start failed!\n", __FUNCTION__);
        return SDK_RET_ERR;
    }

    /* BATCH COMMIT */
    ret = batch_commit_grpc();
    if (ret != SDK_RET_OK) {
        printf("%s: Batch commit failed!\n", __FUNCTION__);
        return SDK_RET_ERR;
    }

    return ret;
}
