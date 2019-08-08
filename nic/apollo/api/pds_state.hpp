/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    pds_state.hpp
 *
 * @brief   This file captures all the state maintained in s/w
 */

#ifndef __PDS_STATE_HPP__
#define __PDS_STATE_HPP__

#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/sdk/lib/catalog/catalog.hpp"
#include "nic/sdk/platform/utils/mpartition.hpp"
#include "nic/sdk/platform/utils/program.hpp"
#include "nic/apollo/api/device_state.hpp"
#include "nic/apollo/api/tep_state.hpp"
#include "nic/apollo/api/vpc_state.hpp"
#include "nic/apollo/api/subnet_state.hpp"
#include "nic/apollo/api/vnic_state.hpp"
#include "nic/apollo/api/mapping_state.hpp"
#include "nic/apollo/api/route_state.hpp"
#include "nic/apollo/api/policy_state.hpp"
#include "nic/apollo/api/if_state.hpp"
#include "nic/apollo/api/mirror_state.hpp"
#include "nic/apollo/api/meter_state.hpp"
#include "nic/apollo/api/tag_state.hpp"
#include "nic/apollo/api/lif_state.hpp"
#include "nic/apollo/api/service_state.hpp"
#include "nic/apollo/api/vpc_peer_state.hpp"
#include "nic/apollo/api/nexthop_state.hpp"
#include "nic/apollo/api/nexthop_group_state.hpp"

using std::string;

namespace api {

/**
 * @defgroup PDS_STATE - Internal state
 * @{
 */

class pds_state {
public:
    pds_state();
    ~pds_state();
    static void destroy(pds_state *ps);
    void set_cfg_path(string cfg_path) { cfg_path_ = cfg_path; }
    string cfg_path(void) const { return cfg_path_; }
    void set_pipeline(string pipeline) { pipeline_ = pipeline; }
    string pipeline(void) const { return pipeline_; }
    void set_catalog(catalog *catalog) { catalog_ = catalog; }
    catalog *catalogue(void) const { return catalog_; }
    void set_mpartition(mpartition *mpartition) { mpartition_ = mpartition; }
    mpartition *mempartition(void) const { return mpartition_; }
    void set_prog_info(program_info *pginfo) { pginfo_ = pginfo; }
    program_info *prog_info(void) const { return pginfo_; }
    void set_platform_type(platform_type_t type) { platform_type_ = type; }
    platform_type_t platform_type(void) const { return platform_type_; }
    void set_scale_profile(pds_scale_profile_t profile) {
        scale_profile_ = profile;
    }
    pds_scale_profile_t scale_profile(void) const { return scale_profile_; }
    void set_control_cores_mask(uint64_t control_cores_mask) {
        control_cores_mask_ = control_cores_mask;
    }
    uint64_t control_cores_mask(void) const { return control_cores_mask_; }
    void set_num_control_cores(uint16_t num_cores) {
        num_control_cores_ = num_cores;
    }
    uint16_t num_control_cores(void) const { return num_control_cores_; }
    void set_data_cores_mask(uint64_t data_cores_mask) {
        data_cores_mask_ = data_cores_mask;
    }
    uint64_t data_cores_mask(void) const { return data_cores_mask_; }
    void set_num_data_cores(uint16_t num_cores) {
        num_data_cores_ = num_cores;
    }
    uint16_t num_data_cores(void) const { return num_data_cores_; }
    device_state *device_db(void) { return &device_db_; }
    tep_state *tep_db(void) { return &tep_db_; }
    vpc_state *vpc_db(void) { return &vpc_db_; }
    subnet_state *subnet_db(void) { return &subnet_db_; }
    vnic_state *vnic_db(void) { return &vnic_db_ ; }
    mapping_state *mapping_db(void) { return &mapping_db_; }
    route_table_state *route_table_db(void) { return &route_table_db_; }
    policy_state *policy_db(void) { return &poicy_db_; }
    if_state *if_db(void) { return &if_db_; }
    mirror_session_state *mirror_session_db(void) {
        return &mirror_session_db_;
    }
    meter_state *meter_db(void) { return &meter_db_; }
    tag_state *tag_db(void) { return &tag_db_; }
    lif_state *lif_db(void) { return &lif_db_; }
    svc_mapping_state *svc_mapping_db(void) { return &svc_mapping_db_; }
    vpc_peer_state *vpc_peer_db(void) { return &vpc_peer_db_; }
    nexthop_state *nexthop_db(void) { return &nexthop_db_; }
    nexthop_group_state *nexthop_group_db(void) { return &nexthop_group_db_; }

private:
    string                  cfg_path_;
    string                  pipeline_;
    catalog                 *catalog_;
    mpartition              *mpartition_;
    platform_type_t         platform_type_;
    pds_scale_profile_t     scale_profile_;
    program_info            *pginfo_;
    uint64_t                control_cores_mask_;
    uint16_t                num_control_cores_;
    uint64_t                data_cores_mask_;
    uint16_t                num_data_cores_;
    device_state            device_db_;
    tep_state               tep_db_;
    vpc_state               vpc_db_;
    subnet_state            subnet_db_;
    vnic_state              vnic_db_;
    mapping_state           mapping_db_;
    route_table_state       route_table_db_;
    policy_state            poicy_db_;
    if_state                if_db_;
    mirror_session_state    mirror_session_db_;
    meter_state             meter_db_;
    tag_state               tag_db_;
    lif_state               lif_db_;
    svc_mapping_state       svc_mapping_db_;
    vpc_peer_state          vpc_peer_db_;
    nexthop_state           nexthop_db_;
    nexthop_group_state     nexthop_group_db_;
};
extern pds_state g_pds_state;

/** * @} */    // end of PDS_STATE

}    // namespace api

using api::pds_state;

static inline device_state *
device_db (void)
{
    return api::g_pds_state.device_db();
}

static inline tep_state *
tep_db (void)
{
    return api::g_pds_state.tep_db();
}

static inline vpc_state *
vpc_db (void)
{
    return api::g_pds_state.vpc_db();
}

static inline subnet_state *
subnet_db (void)
{
    return api::g_pds_state.subnet_db();
}

static inline vnic_state *
vnic_db (void)
{
    return api::g_pds_state.vnic_db();
}

static inline mapping_state *
mapping_db (void)
{
    return api::g_pds_state.mapping_db();
}

static inline route_table_state *
route_table_db (void)
{
    return api::g_pds_state.route_table_db();
}

static inline policy_state *
policy_db (void)
{
    return api::g_pds_state.policy_db();
}

static inline if_state *
if_db (void)
{
    return api::g_pds_state.if_db();
}

static inline mirror_session_state *
mirror_session_db (void)
{
    return api::g_pds_state.mirror_session_db();
}

static inline meter_state *
meter_db (void)
{
    return api::g_pds_state.meter_db();
}

static inline tag_state *
tag_db (void)
{
    return api::g_pds_state.tag_db();
}

static inline lif_state *
lif_db (void)
{
    return api::g_pds_state.lif_db();
}

static inline svc_mapping_state *
svc_mapping_db (void)
{
    return api::g_pds_state.svc_mapping_db();
}

static inline vpc_peer_state *
vpc_peer_db (void)
{
    return api::g_pds_state.vpc_peer_db();
}

static inline nexthop_state *
nexthop_db (void)
{
    return api::g_pds_state.nexthop_db();
}

static inline nexthop_group_state *
nexthop_group_db (void)
{
    return api::g_pds_state.nexthop_group_db();
}

#endif    // __PDS_STATE_HPP__
