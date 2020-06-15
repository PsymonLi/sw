//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_tep.hpp"
#include "nic/apollo/test/api/utils/route.hpp"

namespace test {
namespace api {

static const pds_nh_type_t k_rt_def_nh_type = (apulu() || apollo()) ?
                                               PDS_NH_TYPE_OVERLAY
                                               : PDS_NH_TYPE_IP;
static void
set_nhtype_bit (uint16_t &bmap, uint8_t pos)
{
    bmap |= ((uint16_t)1 << pos);
}

uint8_t
learn_nhtypes_supported (uint16_t &nhtypes_bmap)
{
    uint8_t num_types = 0;

    set_nhtype_bit(nhtypes_bmap, PDS_NH_TYPE_OVERLAY);
    set_nhtype_bit(nhtypes_bmap, PDS_NH_TYPE_PEER_VPC);
    num_types += 2;

    if (artemis()) {
        set_nhtype_bit(nhtypes_bmap, PDS_NH_TYPE_IP);
        num_types++;
    } else if (apulu()) {
        set_nhtype_bit(nhtypes_bmap, PDS_NH_TYPE_OVERLAY_ECMP);
        set_nhtype_bit(nhtypes_bmap, PDS_NH_TYPE_VNIC);
        num_types += 2;
    }

    return num_types;
}

static uint16_t g_nhtypes_supported_bmap;
static const uint8_t k_num_nhtypes_supported =
                     learn_nhtypes_supported(g_nhtypes_supported_bmap);
static uint8_t k_route_class_priority = 1;
static const uint16_t MAX_NH_TYPES = 9;
//----------------------------------------------------------------------------
// Route table feeder class routines
//----------------------------------------------------------------------------

void
route_table_feeder::init(std::string base_route_pfx_str,
                         uint8_t af, uint32_t num_routes,
                         uint32_t num_route_tables,
                         uint32_t id, bool priority_en,
                         bool stash) {
    memset(&spec, 0, sizeof(pds_route_table_spec_t));
    spec.key = int2pdsobjkey(id);
    this->base_route_pfx_str = base_route_pfx_str;
    this->stash_ = stash;
    num_obj = num_route_tables;
    create_route_table_spec(base_route_pfx_str, af, num_routes,
                            num_obj, &spec, priority_en);
}

void
route_table_feeder::iter_next(int width) {
    spec.key = int2pdsobjkey(pdsobjkey2int(spec.key) + width);
    cur_iter_pos++;
}

void
route_table_feeder::key_build(pds_obj_key_t *key) const {
    memcpy(key, &this->spec.key, sizeof(pds_obj_key_t));
}

void
route_table_feeder::spec_alloc(pds_route_table_spec_t *spec) {
    spec->route_info =
        (route_info_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_ROUTE_TABLE,
                                   ROUTE_INFO_SIZE(0));
    // To-do we need to free this allocated memory during delete
}

void
route_table_feeder::spec_build(pds_route_table_spec_t *spec) const {
    memcpy(spec, &this->spec, sizeof(pds_route_table_spec_t));
    create_route_table_spec(this->base_route_pfx_str, this->spec.route_info->af,
                            this->spec.route_info->num_routes, this->num_obj,
                            (pds_route_table_spec_t *)spec,
                            this->spec.route_info->priority_en);
}

bool
route_table_feeder::key_compare(const pds_obj_key_t *key) const {
    return *key == this->spec.key;
}

inline bool
compare_routes (pds_route_t *r1, pds_route_t *r2)
{
    if (ip_prefix_is_equal((ip_prefix_t*)&r1->attrs.prefix,
                           (ip_prefix_t*)&r2->attrs.prefix) == false)
        return false;
    if (r1->attrs.class_priority != r2->attrs.class_priority)
        return false;
    if (r1->attrs.priority != r2->attrs.priority)
        return false;
    if (r1->attrs.nh_type != r2->attrs.nh_type)
        return false;
    switch (r1->attrs.nh_type) {
    case PDS_NH_TYPE_OVERLAY:
        if (r1->attrs.tep != r2->attrs.tep)
            return false;
        break;
    case PDS_NH_TYPE_OVERLAY_ECMP:
        if (r1->attrs.nh_group != r2->attrs.nh_group)
            return false;
        break;
    case PDS_NH_TYPE_PEER_VPC:
        if (r1->attrs.vpc != r2->attrs.vpc)
            return false;
        break;
    case PDS_NH_TYPE_IP:
        if (r1->attrs.nh != r2->attrs.nh)
            return false;
        break;
    case PDS_NH_TYPE_VNIC:
        if (r1->attrs.vnic != r2->attrs.vnic)
            return false;
        break;
    default:
        return false;
    }
    if (r1->attrs.nat.src_nat_type != r2->attrs.nat.src_nat_type)
        return false;
    if (ip_addr_is_equal(&r1->attrs.nat.dst_nat_ip,
                         &r2->attrs.nat.dst_nat_ip) == false)
        return false;
    if (r1->attrs.meter != r2->attrs.meter)
        return false;
    return true;
}

bool
route_table_feeder::spec_compare(const pds_route_table_spec_t *spec) const {
    if (spec->route_info == NULL)
        return false;
    if (spec->route_info->af != this->spec.route_info->af)
        return false;
    if (spec->route_info->num_routes != this->spec.route_info->num_routes) {
        return false;
    }
    if (spec->route_info->num_routes) {
        for (uint32_t i = 0; i < spec->route_info->num_routes; i++) {
            if (compare_routes(&spec->route_info->routes[i],
                               &this->spec.route_info->routes[i]) == false) {
                return false;
            }
        }
        return true;
    }
    return true;
}

bool
route_table_feeder::status_compare(
    const pds_route_table_status_t *status1,
    const pds_route_table_status_t *status2) const {
    return true;
}

// route-table CRUD helper functions
inline void
fill_route_attrs (pds_route_attrs_t *attrs, pds_nh_type_t type,
                  uint32_t route_id, ip_prefix_t route_pfx,
                  bool priority_en, uint8_t src_nat_type, bool meter)
{
    uint32_t num = 0;
    uint32_t base_id = 1;
    uint32_t base_tep_id = 2;

    attrs->nh_type = type;
    attrs->prefix = route_pfx;
    attrs->class_priority = k_route_class_priority;
    if (priority_en) {
        attrs->priority = (route_id % MAX_ROUTE_PRIORITY);
    }
    switch (type) {
    case PDS_NH_TYPE_OVERLAY:
        num = (route_id % PDS_MAX_TEP);
        attrs->tep = int2pdsobjkey(base_tep_id + num);
        break;
    case PDS_NH_TYPE_OVERLAY_ECMP:
        num = (route_id % (PDS_MAX_NEXTHOP_GROUP-1));
        attrs->nh_group = int2pdsobjkey(base_id + num);
        break;
    case PDS_NH_TYPE_PEER_VPC:
        num = (route_id % PDS_MAX_VPC);
        attrs->vpc = int2pdsobjkey(base_id + num);
        break;
    case PDS_NH_TYPE_VNIC:
        num = (route_id % PDS_MAX_VNIC);
        attrs->vnic = int2pdsobjkey(base_id + num);
        break;
    case PDS_NH_TYPE_IP:
        num = (route_id % PDS_MAX_NEXTHOP);
        attrs->nh = int2pdsobjkey(base_id + num);
        break;
    case PDS_NH_TYPE_BLACKHOLE:
    default:
        break;
    }
    attrs->nat.src_nat_type = (pds_nat_type_t)src_nat_type;
    // Not setting dst_nat_ip as upon setting dst_nat_ip,
    // reservation of the resources fail for the route-table
    // str2ipaddr(k_dst_nat_ip, &attrs->nat.dst_nat_ip);
    attrs->meter = meter;
}

void
create_route_table_spec (std::string base_route_pfx_str,
                        uint8_t af, uint32_t num_routes,
                        uint32_t num_route_tables,
                        pds_route_table_spec_t *spec,
                        bool priority_en)
{
    ip_prefix_t route_pfx;
    ip_addr_t route_addr;
    uint32_t num_routes_per_type, route_index = 0;
    uint16_t tmp_bmap;
    uint32_t index = 0;
    bool meter = false;
    uint8_t src_nat_type = PDS_NAT_TYPE_NONE;
    pds_route_t *route;

    test::extract_ip_pfx(base_route_pfx_str.c_str(), &route_pfx);
    spec->route_info =
        (route_info_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_ROUTE_TABLE,
                                   ROUTE_INFO_SIZE(num_routes));
    spec->route_info->af = af;
    spec->route_info->priority_en = priority_en;
    spec->route_info->num_routes = num_routes;
    num_routes_per_type = num_routes/k_num_nhtypes_supported;
    tmp_bmap = g_nhtypes_supported_bmap;
    while (tmp_bmap) {
        if (tmp_bmap & 0x1) {
            for (uint32_t j = 0; j < num_routes_per_type; j++) {
                route = &(spec->route_info->routes[route_index]);
                route->key = int2pdsobjkey(route_index+1);
                fill_route_attrs(&route->attrs,(pds_nh_type_e)index,
                                 route_index+1, route_pfx,
                                 priority_en, src_nat_type, meter);
                ip_prefix_ip_next(&route_pfx, &route_addr);
                route_pfx.addr = route_addr;
                route_index++;
            }
        }
        index++;
        tmp_bmap >>= 1;
    }

    while (route_index < num_routes) {
        route = &(spec->route_info->routes[route_index]);
        route->key = int2pdsobjkey(route_index+1);
        fill_route_attrs(&route->attrs, k_rt_def_nh_type, route_index+1,
                        route_pfx, priority_en, src_nat_type, meter);
        ip_prefix_ip_next(&route_pfx, &route_addr);
        route_pfx.addr = route_addr;
        route_index++;
    }
}

void
route_table_create (route_table_feeder& feeder) {
    pds_batch_ctxt_t bctxt = batch_start();

    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_create<route_table_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

void
route_table_read (route_table_feeder& feeder, sdk_ret_t exp_result) {
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_read<route_table_feeder>(feeder, exp_result)));
}

static void
route_table_attr_update (route_table_feeder& feeder,
                         pds_route_table_spec_t *spec,
                         uint64_t chg_bmap) {
    if (bit_isset(chg_bmap, ROUTE_TABLE_ATTR_AF)) {
        if(spec->route_info) {
            feeder.spec.route_info->af = spec->route_info->af;
        }
    }
    if (bit_isset(chg_bmap, ROUTE_TABLE_ATTR_ROUTES)) {
        if(spec->route_info) {
            feeder.spec.route_info->num_routes = spec->route_info->num_routes;
        }
    }
    if (bit_isset(chg_bmap, ROUTE_TABLE_ATTR_PRIORITY_EN)) {
        if(spec->route_info) {
            feeder.spec.route_info->priority_en = spec->route_info->priority_en;
        }
    }

}

void
route_table_update (route_table_feeder& feeder, pds_route_table_spec_t *spec,
                    uint64_t chg_bmap, sdk_ret_t exp_result) {
	pds_batch_ctxt_t bctxt = batch_start();
	route_table_attr_update(feeder, spec, chg_bmap);
	SDK_ASSERT_RETURN_VOID(
			(SDK_RET_OK == many_update<route_table_feeder>(bctxt, feeder)));
	//if expected result is err, batch commit should fail
	if (exp_result == SDK_RET_ERR) {
        batch_commit_fail(bctxt);
    } else {
        batch_commit(bctxt);
    }
}

void
route_table_delete (route_table_feeder& feeder) {
    pds_batch_ctxt_t bctxt = batch_start();
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_delete<route_table_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

//----------------------------------------------------------------------------
// Misc routines
//----------------------------------------------------------------------------

// do not modify these sample values as rest of system is sync with these
static route_table_feeder k_route_table_feeder;

void sample_route_table_setup(
    pds_batch_ctxt_t bctxt, const string base_route_pfx, uint8_t af,
    uint32_t num_routes, uint32_t num_route_tables, uint32_t id) {
    // setup and teardown parameters should be in sync
    k_route_table_feeder.init(base_route_pfx, af, num_routes,
                              num_route_tables, id);
    many_create(bctxt, k_route_table_feeder);
}

void sample_route_table_teardown(pds_batch_ctxt_t bctxt, uint32_t id,
                                 uint32_t num_route_tables) {
    k_route_table_feeder.init("0.0.0.0/0", IP_AF_IPV4,
                              PDS_MAX_ROUTE_PER_TABLE, num_route_tables, id);
    many_delete(bctxt, k_route_table_feeder);
}

//----------------------------------------------------------------------------
// Route feeder class routines
//----------------------------------------------------------------------------

void
route_feeder::init(std::string base_route_pfx_str,
                   uint32_t num_routes, uint32_t route_id,
                   uint32_t route_table_id, uint8_t nh_type,
                   uint8_t src_nat_type, bool meter) {
    memset(&spec, 0, sizeof(pds_route_spec_t));

    this->base_route_pfx_str = base_route_pfx_str;
    this->num_obj = num_routes;
    spec.key.route_id = int2pdsobjkey(route_id);
    spec.key.route_table_id = int2pdsobjkey(route_table_id);
    test::extract_ip_pfx(base_route_pfx_str.c_str(), &spec.attrs.prefix);
    fill_route_attrs((pds_route_attrs_t*)&spec.attrs, (pds_nh_type_e)nh_type,
                     route_id, spec.attrs.prefix, true, src_nat_type, meter);
}

inline pds_nh_type_t
get_nth_nh_type(uint16_t nh_type_bucket)
{
    uint16_t nh_type = 0;

    // iterate over the bitmap to find current nh_type
    while ((nh_type_bucket > 0) && (nh_type < MAX_NH_TYPES)) {
        if (bit_isset(g_nhtypes_supported_bmap, bit(nh_type))) {
            nh_type_bucket--;
        }
        nh_type++;
    }
    // to handle the routes which fall under the last bucket
    if (nh_type < MAX_NH_TYPES) {
        nh_type--;
        return (pds_nh_type_e)nh_type;
    }
    return k_rt_def_nh_type;
}

void
route_feeder::iter_next(int width) {
    ip_addr_t ip_addr;
    uint32_t route_id = 0;
    ip_prefix_t route_pfx;
    uint16_t nh_type_bucket = 0;
    pds_nh_type_t nh_type = k_rt_def_nh_type;

    route_id  = pdsobjkey2int(spec.key.route_id) + width;
    spec.key.route_id = int2pdsobjkey(route_id);
    cur_iter_pos++;
    ip_prefix_ip_next(&spec.attrs.prefix, &ip_addr);
    route_pfx.addr = ip_addr;
    route_pfx.len = spec.attrs.prefix.len;
    if (num_obj >= k_num_nhtypes_supported) {
        nh_type_bucket =
            (cur_iter_pos / (num_obj / k_num_nhtypes_supported)) + 1;
        nh_type = get_nth_nh_type(nh_type_bucket);
    }
    fill_route_attrs((pds_route_attrs_t*)&spec.attrs, nh_type, route_id,
                     route_pfx, true, spec.attrs.nat.src_nat_type,
                     spec.attrs.meter);
}

void
route_feeder::key_build(pds_route_key_t *key) const {
    memset(key, 0, sizeof(pds_route_key_t));
    key->route_id = spec.key.route_id;
    key->route_table_id = spec.key.route_table_id;
}

void
route_feeder::spec_build(pds_route_spec_t *spec) const {
    memcpy(spec, &this->spec, sizeof(pds_route_spec_t));
    uint32_t route_id = pdsobjkey2int(spec->key.route_id);
    fill_route_attrs((pds_route_attrs_t *)&spec->attrs,
                     this->spec.attrs.nh_type, route_id,
                     this->spec.attrs.prefix, true,
                     this->spec.attrs.nat.src_nat_type, this->spec.attrs.meter);
}

bool
route_feeder::key_compare(const pds_route_key_t *key) const {
    if (key->route_id != spec.key.route_id)
        return false;
    if (key->route_table_id != spec.key.route_table_id)
        return false;
    return true;
}

bool
route_feeder::spec_compare(const pds_route_spec_t *spec) const {
    if (ip_prefix_is_equal((ip_prefix_t*)&spec->attrs.prefix,
                           (ip_prefix_t*)&this->spec.attrs.prefix) == false)
        return false;
    if (spec->attrs.class_priority != this->spec.attrs.class_priority)
        return false;
    if (spec->attrs.priority != this->spec.attrs.priority)
        return false;
    if (spec->attrs.nh_type != this->spec.attrs.nh_type)
        return false;
    switch (spec->attrs.nh_type) {
    case PDS_NH_TYPE_OVERLAY:
        if (spec->attrs.tep != this->spec.attrs.tep)
            return false;
        break;
    case PDS_NH_TYPE_OVERLAY_ECMP:
        if (spec->attrs.nh_group != this->spec.attrs.nh_group)
            return false;
        break;
    case PDS_NH_TYPE_PEER_VPC:
        if (spec->attrs.vpc != this->spec.attrs.vpc)
            return false;
        break;
    case PDS_NH_TYPE_IP:
        if (spec->attrs.nh != this->spec.attrs.nh)
            return false;
        break;
    case PDS_NH_TYPE_VNIC:
        if (spec->attrs.vnic != this->spec.attrs.vnic)
            return false;
        break;
    default:
        return false;
    }
    if (spec->attrs.nat.src_nat_type != this->spec.attrs.nat.src_nat_type)
        return false;
    if (ip_addr_is_equal(&spec->attrs.nat.dst_nat_ip,
                         &this->spec.attrs.nat.dst_nat_ip) == false)
        return false;
    if (spec->attrs.meter != this->spec.attrs.meter)
        return false;
    return true;
}

bool
route_feeder::status_compare(
    const pds_route_status_t *status1,
    const pds_route_status_t *status2) const {
    return true;
}

}    // namespace api
}    // namespace test
