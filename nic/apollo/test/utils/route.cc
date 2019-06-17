//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the route table test utility routines implementation
///
//----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_tep.hpp"
#include "nic/apollo/test/utils/route.hpp"
#include "nic/apollo/test/utils/utils.hpp"

namespace api_test {

pds_nh_type_t g_rt_def_nh_type = PDS_NH_TYPE_TEP;

route_util::route_util() {
    this->nh_type = PDS_NH_TYPE_BLACKHOLE;
    this->peer_vpc_id = PDS_VPC_ID_INVALID;
}

route_util::~route_util() {}

route_table_util::route_table_util() {
    this->id = PDS_ROUTE_TABLE_ID_INVALID;
    this->af = IP_AF_IPV4;
    this->num_routes = 0;
    memset(routes, 0, sizeof(route_util) * (PDS_MAX_ROUTE_PER_TABLE + 1));
}

route_table_util::route_table_util(pds_route_table_id_t id,
                                   ip_prefix_t base_route_pfx,
                                   ip_addr_t base_nh_ip, uint8_t af,
                                   uint32_t num_routes, pds_nh_type_t nh_type,
                                   pds_vpc_id_t peer_vpc_id,
                                   pds_nexthop_id_t base_nh_id) {
    uint32_t nh_offset = 0, nh_offset_max = PDS_MAX_TEP - 1;
    uint32_t nh_id_offset = 0, nh_id_offset_max = PDS_MAX_NEXTHOP - 1;
    ip_addr_t route_addr;

    memset(routes, 0, sizeof(route_util) * (PDS_MAX_ROUTE_PER_TABLE + 1));
    this->id = id;
    this->af = af;
    this->base_route_pfx = base_route_pfx;
    this->base_nh_ip = base_nh_ip;
    this->num_routes = num_routes;
    for (uint32_t i = 0; i < this->num_routes; ++i) {
        this->routes[i].ip_pfx = base_route_pfx;
        this->routes[i].nh_type = nh_type;
        switch (nh_type) {
        case PDS_NH_TYPE_IP:
            this->routes[i].nh = base_nh_id + nh_id_offset;
            nh_id_offset += 1;
            if (nh_id_offset > nh_id_offset_max) {
                nh_id_offset %= nh_id_offset_max;
            }
            break;
        case PDS_NH_TYPE_TEP:
            this->routes[i].nh_ip.addr.v4_addr =
                base_nh_ip.addr.v4_addr + nh_offset;
            nh_offset += 1;
            if (nh_offset > nh_offset_max) {
                nh_offset %= nh_offset_max;
            }
            break;
        case PDS_NH_TYPE_PEER_VPC:
            this->routes[i].peer_vpc_id = peer_vpc_id;
            break;
        default:
            break;
        }
        ip_prefix_ip_next(&base_route_pfx, &route_addr);
        base_route_pfx.addr = route_addr;
    }
}

route_table_util::~route_table_util() {}

sdk::sdk_ret_t
route_table_util::create(void) const {
    pds_route_table_spec_t spec;

    memset(&spec, 0, sizeof(pds_route_table_spec_t));
    spec.key.id = this->id;
    spec.af = this->af;
    spec.num_routes = this->num_routes;
    spec.routes = (pds_route_t *)calloc(1,
        (this->num_routes * sizeof(pds_route_t)));
    for (uint32_t i = 0; i < this->num_routes; ++i) {
        spec.routes[i].prefix = this->routes[i].ip_pfx;
        spec.routes[i].nh_type = this->routes[i].nh_type;
        switch (this->routes[i].nh_type) {
        case PDS_NH_TYPE_IP:
            spec.routes[i].nh = this->routes[i].nh;
            break;
        case PDS_NH_TYPE_TEP:
            spec.routes[i].nh_ip = this->routes[i].nh_ip;
            break;
        case PDS_NH_TYPE_PEER_VPC:
            spec.routes[i].vpc.id = this->routes[i].peer_vpc_id;
            break;
        default:
            break;
        }
    }
    return (pds_route_table_create(&spec));
}

sdk::sdk_ret_t
route_table_util::read(pds_route_table_info_t *info) const {
    // route table being stateless, do not support read
    return sdk::SDK_RET_INVALID_OP;
}

sdk::sdk_ret_t
route_table_util::update(void) const {
    pds_route_table_spec_t spec;

    memset(&spec, 0, sizeof(pds_route_table_spec_t));
    spec.key.id = this->id;
    spec.af = this->af;
    spec.num_routes = this->num_routes;
    spec.routes = (pds_route_t *)calloc(1,
        (this->num_routes * sizeof(pds_route_t)));
    for (uint32_t i = 0; i < this->num_routes; ++i) {
        spec.routes[i].prefix = this->routes[i].ip_pfx;
        spec.routes[i].nh_type = this->routes[i].nh_type;
        switch (this->routes[i].nh_type) {
        case PDS_NH_TYPE_IP:
            spec.routes[i].nh = this->routes[i].nh;
            break;
        case PDS_NH_TYPE_TEP:
            spec.routes[i].nh_ip = this->routes[i].nh_ip;
            break;
        case PDS_NH_TYPE_PEER_VPC:
            spec.routes[i].vpc.id = this->routes[i].peer_vpc_id;
            break;
        default:
            break;
        }
    }
    return (pds_route_table_update(&spec));
}

sdk::sdk_ret_t
route_table_util::del(void) const {
    pds_route_table_key_t key;

    memset(&key, 0, sizeof(pds_route_table_key_t));
    key.id = this->id;
    return (pds_route_table_delete(&key));
}

static inline sdk::sdk_ret_t
route_table_util_stepper (route_table_seed_t *seed,
                          utils_op_t op,
                          sdk_ret_t expected_result = sdk::SDK_RET_OK)
{
    sdk::sdk_ret_t rv;
    pds_route_table_info_t info;
    ip_prefix_t route_pfx = seed->base_route_pfx;
    ip_addr_t route_addr;

    for (uint32_t idx = 0; idx < seed->num_route_tables; ++idx) {
        route_table_util rt_obj(idx + seed->base_route_table_id, route_pfx,
                                seed->base_nh_ip, seed->af, seed->num_routes,
                                seed->nh_type, seed->peer_vpc_id,
                                seed->base_nh_id);
        switch (op) {
        case OP_MANY_CREATE:
            rv = rt_obj.create();
            break;
        case OP_MANY_READ:
            rv = rt_obj.read(&info);
            break;
        case OP_MANY_UPDATE:
            rv = rt_obj.update();
            break;
        case OP_MANY_DELETE:
            rv = rt_obj.del();
            break;
        default:
            return sdk::SDK_RET_INVALID_OP;
        }
        if (rv != expected_result) {
            return sdk::SDK_RET_ERR;
        }
        for (uint32_t i = 0; i < seed->num_routes; ++i) {
            // increment route_pfx by num_routes
            ip_prefix_ip_next(&route_pfx, &route_addr);
            route_pfx.addr = route_addr;
        }

    }
    return sdk::SDK_RET_OK;
}

static inline sdk::sdk_ret_t
route_table_util_object_stepper (route_table_stepper_seed_t *seed,
                                 utils_op_t op,
                                 sdk_ret_t expected_result = sdk::SDK_RET_OK)
{
    sdk::sdk_ret_t rv;

    rv = route_table_util_stepper(&seed->v4_rt_seed, op, expected_result);
    if (rv != sdk::SDK_RET_OK) {
        return rv;
    }
    rv = route_table_util_stepper(&seed->v6_rt_seed, op, expected_result);
    if (rv != sdk::SDK_RET_OK) {
        return rv;
    }
    return sdk::SDK_RET_OK;
}

sdk::sdk_ret_t
route_table_util::many_create(route_table_stepper_seed_t *seed) {
    return (route_table_util_object_stepper(seed, OP_MANY_CREATE));
}

sdk::sdk_ret_t
route_table_util::many_read(route_table_stepper_seed_t *seed,
                            sdk::sdk_ret_t exp_result) {
    return (route_table_util_object_stepper(seed, OP_MANY_READ, exp_result));
}


sdk::sdk_ret_t
route_table_util::many_update(route_table_stepper_seed_t *seed) {
    return (route_table_util_object_stepper(seed, OP_MANY_UPDATE));
}

sdk::sdk_ret_t
route_table_util::many_delete(route_table_stepper_seed_t *seed) {
    return (route_table_util_object_stepper(seed, OP_MANY_DELETE));
}

void
route_table_util::route_table_stepper_seed_init(
                                            route_table_stepper_seed_t *seed,
                                            uint32_t num_route_tables,
                                            uint32_t base_route_table_id,
                                            std::string base_route_pfx_str,
                                            std::string base_nh_ip_str,
                                            uint8_t af, uint32_t num_routes,
                                            pds_nh_type_t nh_type,
                                            pds_vpc_id_t peer_vpc_id,
                                            pds_nexthop_id_t base_nh_id) {
    route_table_seed_t *rt_seed;
    if (af == IP_AF_IPV4) {
        rt_seed = &seed->v4_rt_seed;
    } else {
        rt_seed = &seed->v6_rt_seed;
    }
    rt_seed->num_route_tables = num_route_tables;
    rt_seed->base_route_table_id = base_route_table_id;
    extract_ip_pfx(base_route_pfx_str.c_str(), &rt_seed->base_route_pfx);
    extract_ip_addr(base_nh_ip_str.c_str(), &rt_seed->base_nh_ip);
    rt_seed->af = af;
    rt_seed->num_routes = num_routes;
    rt_seed->nh_type = nh_type;
    rt_seed->peer_vpc_id = peer_vpc_id;
    rt_seed->base_nh_id = base_nh_id;
}

}    // namespace api_test
