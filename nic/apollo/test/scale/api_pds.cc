//------------------------------------------------------------------------------
// Copyright (c) 2019 Pensando Systems, Inc.
//------------------------------------------------------------------------------

#include <iostream>
#include <math.h>
#include "nic/apollo/test/flow_test/flow_test.hpp"
#include "nic/apollo/test/scale/test_common.hpp"

flow_test *g_flow_test_obj;

sdk_ret_t
create_route_table (pds_route_table_spec_t *route_table)
{
    if (route_table) {
        return pds_route_table_create(route_table);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_svc_mapping (pds_svc_mapping_spec_t *svc_mapping)
{
    if (svc_mapping) {
        return pds_svc_mapping_create(svc_mapping);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_local_mapping (pds_local_mapping_spec_t *pds_local_mapping)
{
    sdk_ret_t rv;

    if (pds_local_mapping) {
        rv = pds_local_mapping_create(pds_local_mapping);
        if (rv != SDK_RET_OK) {
            return rv;
        }
        g_flow_test_obj->add_local_ep(pds_local_mapping);
        return rv;
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_remote_mapping (pds_remote_mapping_spec_t *pds_remote_mapping)
{
    sdk_ret_t rv;

    if (pds_remote_mapping) {
        rv = pds_remote_mapping_create(pds_remote_mapping);
        if (rv != SDK_RET_OK) {
            return rv;
        }
        g_flow_test_obj->add_remote_ep(pds_remote_mapping);
        return rv;
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_vnic (pds_vnic_spec_t *pds_vnic)
{
    if (pds_vnic) {
        return pds_vnic_create(pds_vnic);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_subnet (pds_subnet_spec_t *pds_subnet)
{
    if (pds_subnet) {
        return pds_subnet_create(pds_subnet);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_vpc (pds_vpc_spec_t *pds_vpc)
{
    if (pds_vpc) {
        return pds_vpc_create(pds_vpc);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_vpc_peer (pds_vpc_peer_spec_t *pds_vpc_peer)
{
    if (pds_vpc_peer) {
        return pds_vpc_peer_create(pds_vpc_peer);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_tag (pds_tag_spec_t *pds_tag)
{
    if (pds_tag) {
        return pds_tag_create(pds_tag);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_meter (pds_meter_spec_t *pds_meter)
{
    if (pds_meter) {
        return pds_meter_create(pds_meter);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_nexthop (pds_nexthop_spec_t *pds_nh)
{
    if (pds_nh) {
        return pds_nexthop_create(pds_nh);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_tunnel (uint32_t id, pds_tep_spec_t *pds_tep)
{
    if (pds_tep) {
        return pds_tep_create(pds_tep);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_device (pds_device_spec_t *device)
{
    if (device) {
        return pds_device_create(device);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_policy (pds_policy_spec_t *policy)
{
    if (policy) {
        return pds_policy_create(policy);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_mirror_session (pds_mirror_session_spec_t *ms)
{
    if (ms) {
        return pds_mirror_session_create(ms);
    } else {
        return SDK_RET_OK;
    }
}

sdk_ret_t
create_objects_init (test_params_t *test_params)
{
    g_flow_test_obj = new flow_test(test_params);
    g_flow_test_obj->set_cfg_params(test_params->dual_stack,
            test_params->num_tcp,
            test_params->num_udp,
            test_params->num_icmp,
            test_params->sport_lo,
            test_params->sport_hi,
            test_params->dport_lo,
            test_params->dport_hi);

#if defined(ARTEMIS)
    g_flow_test_obj->set_session_info_cfg_params(
            test_params->num_vpcs,
            test_params->num_ip_per_vnic,
            test_params->num_remote_mappings,
            test_params->meter_scale,
            TESTAPP_METER_NUM_PREFIXES,
            test_params->num_nh);
#endif

    return SDK_RET_OK;
}

sdk_ret_t
create_objects_end ()
{
    sdk_ret_t ret;

    ret = g_flow_test_obj->create_flows();
    if (ret != SDK_RET_OK) {
        return ret;
    }

    return SDK_RET_OK;
}
