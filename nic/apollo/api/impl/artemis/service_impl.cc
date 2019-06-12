//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of service mapping
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/service.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/impl/artemis/pds_impl_state.hpp"
#include "nic/apollo/api/impl/artemis/artemis_impl.hpp"
#include "nic/apollo/api/impl/artemis/service_impl.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/table/memhash/mem_hash.hpp"
#include "nic/sdk/include/sdk/table.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "gen/p4gen/artemis/include/p4pd.h"

using sdk::table::sdk_table_api_params_t;

namespace api {
namespace impl {

/// \defgroup PDS_SVC_MAPPING_IMPL - service mapping datapath implementation
/// \ingroup PDS_SERVICE
/// @{

#define PDS_IMPL_FILL_SVC_MAPPING_SWKEY(key, vpc_hw_id, vip_or_dip, svc_port,  \
                                        provider_ip)                           \
{                                                                              \
    memset((key), 0, sizeof(*(key)));                                          \
    (key)->key_metadata_mapping_port = svc_port;                               \
    if ((vip_or_dip)->af == IP_AF_IPV6) {                                      \
        sdk::lib::memrev((key)->key_metadata_mapping_ip,                       \
                         (vip_or_dip)->addr.v6_addr.addr8, IP6_ADDR8_LEN);     \
        if (provider_ip) {                                                     \
            sdk::lib::memrev((key)->key_metadata_mapping_ip2,                  \
                             (provider_ip)->addr.v6_addr.addr8, IP6_ADDR8_LEN);\
        }                                                                      \
    } else {                                                                   \
        memcpy((key)->key_metadata_mapping_ip,                                 \
               &(vip_or_dip)->addr.v4_addr, IP4_ADDR8_LEN);                    \
        if (provider_ip) {                                                     \
            memcpy((key)->key_metadata_mapping_ip2,                            \
                   &(provider_ip)->addr.v4_addr, IP4_ADDR8_LEN);               \
        }                                                                      \
    }                                                                          \
    (key)->vnic_metadata_vpc_id = vpc_hw_id;                                   \
}

#define svc_mapping_action action_u.service_mapping_service_mapping_info
#define PDS_IMPL_FILL_SVC_MAPPING_DATA(data, xlate_idx, xlate_port)            \
{                                                                              \
    memset((data), 0, sizeof(*(data)));                                        \
    (data)->action_id = SERVICE_MAPPING_SERVICE_MAPPING_INFO_ID;               \
    (data)->svc_mapping_action.service_xlate_idx = xlate_idx;                  \
    (data)->svc_mapping_action.service_xlate_port = xlate_port;                \
}

svc_mapping_impl *
svc_mapping_impl::factory(pds_svc_mapping_spec_t *mapping) {
    svc_mapping_impl    *impl;

    impl = svc_mapping_impl_db()->alloc();
    if (unlikely(impl == NULL)) {
        return NULL;
    }
    new (impl) svc_mapping_impl();
    return impl;
}

void
svc_mapping_impl::soft_delete(svc_mapping_impl *impl) {
    impl->~svc_mapping_impl();
    svc_mapping_impl_db()->free(impl);
}

void
svc_mapping_impl::destroy(svc_mapping_impl *impl) {
    svc_mapping_impl::soft_delete(impl);
}

svc_mapping_impl *
svc_mapping_impl::build(pds_svc_mapping_key_t *key) {
    return NULL;
}

sdk_ret_t
svc_mapping_impl::reserve_resources(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    pds_svc_mapping_spec_t *spec;
    sdk_table_api_params_t api_params;
    service_mapping_swkey_t svc_mapping_key = { 0 };

    spec = &obj_ctxt->api_params->svc_mapping_spec;
    vpc = vpc_db()->find(&spec->key.vpc);

    // reserve an entry in SERVICE_MAPPING with (VIP, provider IP, port) as key
    PDS_IMPL_FILL_SVC_MAPPING_SWKEY(&svc_mapping_key,
                                    vpc->hw_id(), &spec->key.vip,
                                    spec->key.svc_port,
                                    &spec->backend_provider_ip);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&api_params, &svc_mapping_key,
                                   NULL, 0, sdk::table::handle_t::null());
    ret = svc_mapping_impl_db()->svc_mapping_tbl()->reserve(&api_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve (VIP, provider-ip, port) entry in "
                      "SERVICE_MAPPING table for mapping %s, err %u",
                      orig_obj->key2str().c_str(), ret);
        return ret;
    }
    vip_to_dip_handle_ = api_params.handle;

    // reserve an entry in the NAT table for (VIP, provider-ip, VIP-port) -> DIP
    // xlation rewrite
    ret = artemis_impl_db()->nat_tbl()->reserve(&to_dip_nat_hdl_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve (VIP, provider-ip, VIP-port) -> "
                      "DIP xlation entry in NAT table, err %u", ret);
        goto error;
    }

    // reserve an entry in SERVICE_MAPPING with (DIP/overlay_ip, port) as key
    vpc = vpc_db()->find(&spec->vpc);
    memset(&svc_mapping_key, 0, sizeof(svc_mapping_key));
    PDS_IMPL_FILL_SVC_MAPPING_SWKEY(&svc_mapping_key,
                                    vpc->hw_id(), &spec->backend_ip,
                                    spec->svc_port, (ip_addr_t *)NULL);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&api_params, &svc_mapping_key,
                                   NULL, 0, sdk::table::handle_t::null());
    ret = svc_mapping_impl_db()->svc_mapping_tbl()->reserve(&api_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve (backend-ip, port) entry in "
                      "SERVICE_MAPPING table for mapping %s, err %u",
                      orig_obj->key2str().c_str(), ret);
        goto error;
    }
    dip_to_vip_handle_ = api_params.handle;

    // reserve an entry in the NAT table for (DIP, DIP-port) -> VIP
    // xlation rewrite
    ret = artemis_impl_db()->nat_tbl()->reserve(&to_vip_nat_hdl_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve (DIP, DIP-port) -> VIP xlation entry "
                      "in NAT table, err %u", ret);
        goto error;
    }
    return SDK_RET_OK;

error:
    // TODO: cleanup
    return ret;
}

sdk_ret_t
svc_mapping_impl::nuke_resources(api_base *api_obj) {
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
svc_mapping_impl::release_resources(api_base *api_obj) {
    return SDK_RET_OK;
}

sdk_ret_t
svc_mapping_impl::program_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    vpc_entry *vip_vpc, *dip_vpc;
    pds_svc_mapping_spec_t *spec;
    sdk_table_api_params_t api_params;
    nat_actiondata_t nat_data;
    service_mapping_actiondata_t svc_mapping_data;
    service_mapping_swkey_t svc_mapping_key;

    spec = &obj_ctxt->api_params->svc_mapping_spec;
    vip_vpc = vpc_db()->find(&spec->key.vpc);
    dip_vpc = vpc_db()->find(&spec->vpc);
    PDS_TRACE_DEBUG("Programming svc mapping (vpc %u, vip %s, port %u, "
                    "provider IP %s) -> (vpc %u, dip %s, port %u)",
                    spec->key.vpc.id, ipaddr2str(&spec->key.vip),
                    spec->key.svc_port, ipaddr2str(&spec->backend_provider_ip),
                    spec->vpc.id, ipaddr2str(&spec->backend_ip),
                    spec->svc_port);

    // add NAT entry with DIP in the data
    PDS_IMPL_FILL_NAT_DATA(&nat_data, &spec->backend_ip);
    ret = artemis_impl_db()->nat_tbl()->insert_atid(&nat_data, to_dip_nat_hdl_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to add DIP NAT entry for mapping (vpc %u, vip %s,"
                      " port %u, provider IP %s) -> (vpc %u, dip %s, port %u), "
                      "err %u", spec->key.vpc.id, ipaddr2str(&spec->key.vip),
                      spec->key.svc_port,
                      ipaddr2str(&spec->backend_provider_ip),
                      spec->vpc.id, ipaddr2str(&spec->backend_ip),
                      spec->svc_port, ret);
        return ret;
    }

    // add an entry in SERVICE_MAPPING with (VIP, provider-ip, port) as key
    PDS_IMPL_FILL_SVC_MAPPING_SWKEY(&svc_mapping_key,
                                    vip_vpc->hw_id(), &spec->key.vip,
                                    spec->key.svc_port,
                                    &spec->backend_provider_ip);
    PDS_IMPL_FILL_SVC_MAPPING_DATA(&svc_mapping_data,
                                   to_dip_nat_hdl_,
                                   spec->svc_port);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&api_params, &svc_mapping_key,
                                   &svc_mapping_data,
                                   SERVICE_MAPPING_SERVICE_MAPPING_INFO_ID,
                                   vip_to_dip_handle_);

    ret = svc_mapping_impl_db()->svc_mapping_tbl()->insert(&api_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to add svc mapping (vpc %u, vip %s, port %u, "
                      "provider IP %s) -> (vpc %u, dip %s, port %u), err %u",
                      spec->key.vpc.id, ipaddr2str(&spec->key.vip),
                      spec->key.svc_port,
                      ipaddr2str(&spec->backend_provider_ip),
                      spec->vpc.id, ipaddr2str(&spec->backend_ip),
                      spec->svc_port, ret);
        return ret;
    }

    // add NAT entry with VIP in the data
    PDS_IMPL_FILL_NAT_DATA(&nat_data, &spec->key.vip);
    ret = artemis_impl_db()->nat_tbl()->insert_atid(&nat_data, to_vip_nat_hdl_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to add VIP NAT entry for mapping (vpc %u, vip %s,"
                      " port %u, provider IP %s) -> (vpc %u, dip %s, port %u), "
                      "err %u", spec->key.vpc.id, ipaddr2str(&spec->key.vip),
                      spec->key.svc_port,
                      ipaddr2str(&spec->backend_provider_ip),
                      spec->vpc.id, ipaddr2str(&spec->backend_ip),
                      spec->svc_port, ret);
        return ret;
    }
    // add an entry in SERVICE_MAPPING with (DIP/overlay_ip, port) as key
    PDS_IMPL_FILL_SVC_MAPPING_SWKEY(&svc_mapping_key,
                                    dip_vpc->hw_id(), &spec->backend_ip,
                                    spec->svc_port, (ip_addr_t *)NULL);
    PDS_IMPL_FILL_SVC_MAPPING_DATA(&svc_mapping_data,
                                   to_vip_nat_hdl_,
                                   spec->key.svc_port);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&api_params, &svc_mapping_key,
                                   &svc_mapping_data,
                                   SERVICE_MAPPING_SERVICE_MAPPING_INFO_ID,
                                   dip_to_vip_handle_);
    ret = svc_mapping_impl_db()->svc_mapping_tbl()->insert(&api_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to add svc mapping (vpc %u, backend-ip %s, "
                      "port %u) --> (vpc %u, vip %s, port %u), err %u",
                      spec->vpc.id, ipaddr2str(&spec->backend_ip),
                      spec->svc_port, spec->key.vpc.id,
                      ipaddr2str(&spec->key.vip), spec->key.svc_port, ret);
        return ret;
    }
    return SDK_RET_OK;
}

sdk_ret_t
svc_mapping_impl::cleanup_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
svc_mapping_impl::update_hw(api_base *curr_obj, api_base *prev_obj,
                            obj_ctxt_t *obj_ctxt) {
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
svc_mapping_impl::activate_hw(api_base *api_obj, pds_epoch_t epoch,
                              api_op_t api_op, obj_ctxt_t *obj_ctxt) {
    return SDK_RET_OK;
}

sdk_ret_t
svc_mapping_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    return SDK_RET_INVALID_OP;
}

/// \@}    // end of PDS_SVC_MAPPING_IMPL

}    // namespace impl
}    // namespace api
