//
// Copyright (c) 2019 Pensando Systems, Inc.
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of mapping
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/mapping.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/impl/artemis/vnic_impl.hpp"
#include "nic/apollo/api/impl/artemis/mapping_impl.hpp"
#include "nic/apollo/api/impl/artemis/pds_impl_state.hpp"
#include "nic/artemis/p4/include/defines.h"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/table/memhash/mem_hash.hpp"
#include "nic/sdk/include/sdk/table.hpp"
#include "nic/sdk/lib/utils/utils.hpp"

using sdk::table::sdk_table_api_params_t;

namespace api {
namespace impl {

/// \defgroup PDS_MAPPING_IMPL - mapping entry datapath implementation
/// \ingroup PDS_MAPPING
/// @{

// TODO: IP address type (i.e., v4 or v6 bit) is not part of the key
#define PDS_IMPL_FILL_LOCAL_IP_MAPPING_SWKEY(key, vnic_hw_id, ip, rev)       \
{                                                                            \
    (key)->vnic_metadata_local_vnic_tag = vnic_hw_id;                        \
    if ((ip)->af == IP_AF_IPV6) {                                            \
        if (rev) {                                                           \
            sdk::lib::memrev((key)->control_metadata_mapping_lkp_addr,       \
                             (ip)->addr.v6_addr.addr8, IP6_ADDR8_LEN);       \
        } else {                                                             \
            memcpy((key)->control_metadata_mapping_lkp_addr,                 \
                   (ip)->addr.v6_addr.addr8, IP6_ADDR8_LEN);                 \
        }                                                                    \
    } else {                                                                 \
        /* key is initialized to zero by the caller */                       \
        memcpy((key)->control_metadata_mapping_lkp_addr,                     \
               &(ip)->addr.v4_addr, IP4_ADDR8_LEN);                          \
    }                                                                        \
}

#define PDS_IMPL_FILL_LOCAL_IP_MAPPING_APPDATA(data, vpc_hw_id, xlate_idx,   \
                                               iptype)                       \
{                                                                            \
    (data)->vpc_id = (vpc_hw_id);                                            \
    (data)->vpc_id_valid = true;                                             \
    (data)->xlate_index = (uint32_t)xlate_idx;                               \
    (data)->ip_type = (iptype);                                              \
}

#define PDS_IMPL_FILL_NAT_DATA(data, ip)                                     \
{                                                                            \
    (data)->action_id = NAT_NAT_ID;                                          \
    if ((ip)->af == IP_AF_IPV6) {                                            \
        sdk::lib::memrev((data)->nat_action.nat_ip,                          \
                         (ip)->addr.v6_addr.addr8, IP6_ADDR8_LEN);           \
    } else {                                                                 \
        /* key is initialized to zero by the caller */                       \
        memcpy((data)->nat_action.nat_ip, &(ip)->addr.v4_addr,               \
               IP4_ADDR8_LEN);                                               \
    }                                                                        \
}

#define PDS_IMPL_FILL_TABLE_API_PARAMS(api_params, key_, data, action, hdl)  \
{                                                                            \
    memset((api_params), 0, sizeof(*(api_params)));                          \
    (api_params)->key = (key_);                                              \
    (api_params)->appdata = (data);                                          \
    (api_params)->action_id = (action);                                      \
    (api_params)->handle = (hdl);                                            \
}

#define nat_action                action_u.nat_nat
mapping_impl *
mapping_impl::factory(pds_mapping_spec_t *pds_mapping) {
    mapping_impl    *impl;

    impl = mapping_impl_db()->alloc();
    if (unlikely(impl == NULL)) {
        return NULL;
    }
    new (impl) mapping_impl();
    // TODO: set is_local_ here !!
    return impl;
}

void
mapping_impl::soft_delete(mapping_impl *impl) {
    impl->~mapping_impl();
    mapping_impl_db()->free(impl);
}

void
mapping_impl::destroy(mapping_impl *impl) {
    mapping_impl::soft_delete(impl);
}

void
mapping_impl::set_is_local(bool val) {
    is_local_ = val;
}

mapping_impl *
mapping_impl::build(pds_mapping_key_t *key) {
    sdk_ret_t                             ret;
    device_entry                          *device;
    vpc_entry                             *vpc;
    mapping_impl                          *impl;
    uint16_t                              vnic_hw_id;
    bool                                  local_mapping = false;
    sdk_table_api_params_t                api_params;
    remote_vnic_mapping_tx_swkey_t        remote_vnic_mapping_tx_key;
    remote_vnic_mapping_tx_appdata_t      remote_vnic_mapping_tx_data;
    remote_vnic_mapping_rx_swkey_t        remote_vnic_mapping_rx_key;
    remote_vnic_mapping_rx_appdata_t      remote_vnic_mapping_rx_data;
    local_ip_mapping_swkey_t              local_ip_mapping_key;
    local_ip_mapping_appdata_t            local_ip_mapping_data;
    local_vnic_by_slot_rx_swkey_t         local_vnic_by_slot_rx_key;
    local_vnic_by_slot_rx_actiondata_t    local_vnic_by_slot_rx_data;
    nexthop_actiondata_t                  nh_data;
    tep_actiondata_t                      tep_data;
    nat_actiondata_t                      nat_data;
    handle_t                              remote_vnic_tx_hdl;

    vpc = vpc_db()->find(&key->vpc);
    if (unlikely(vpc == NULL)) {
        return NULL;
    }

    impl = mapping_impl_db()->alloc();
    if (unlikely(impl == NULL)) {
        return NULL;
    }
    new (impl) mapping_impl();
    return NULL;
}

sdk_ret_t
mapping_impl::reserve_local_ip_mapping_resources_(api_base *api_obj,
                                                  vpc_entry *vpc,
                                                  pds_mapping_spec_t *spec) {
    return SDK_RET_ERR;
}

sdk_ret_t
mapping_impl::reserve_remote_ip_mapping_resources_(api_base *api_obj,
                                                   vpc_entry *vpc,
                                                   pds_mapping_spec_t *spec) {
    return SDK_RET_ERR;
}

sdk_ret_t
mapping_impl::reserve_resources(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    pds_mapping_spec_t    *spec;
    vpc_entry             *vpc;

    spec = &obj_ctxt->api_params->mapping_spec;
    vpc = vpc_db()->find(&spec->key.vpc);

    PDS_TRACE_DEBUG("Reserving resources for mapping (vpc %u, ip %s), "
                    "local %u, subnet %u, tep %s, vnic %u, "
                    "pub_ip_valid %u pub_ip %s",
                    spec->key.vpc.id, ipaddr2str(&spec->key.ip_addr), is_local_,
                    spec->subnet.id, ipv4addr2str(spec->tep.ip_addr),
                    spec->vnic.id, spec->public_ip_valid,
                    ipaddr2str(&spec->public_ip));

    if (is_local_) {
        return reserve_local_ip_mapping_resources_(orig_obj, vpc, spec);
    }
    return reserve_remote_ip_mapping_resources_(orig_obj, vpc, spec);
}

sdk_ret_t
mapping_impl::nuke_resources(api_base *api_obj) {
    sdk_table_api_params_t    api_params = { 0 };

    if (is_local_) {
        if (overlay_ip_hdl_.valid()) {
            api_params.handle = overlay_ip_hdl_;
            mapping_impl_db()->local_ip_mapping_tbl()->remove(&api_params);
        }
        if (public_ip_hdl_.valid()) {
            api_params.handle = public_ip_hdl_;
            mapping_impl_db()->local_ip_mapping_tbl()->remove(&api_params);
        }
        if (provider_ip_hdl_.valid()) {
            api_params.handle = provider_ip_hdl_;
            mapping_impl_db()->local_ip_mapping_tbl()->remove(&api_params);
        }

        // TODO: change the api calls here once DM APIs are standardized
        if (overlay_ip_to_public_ip_nat_hdl_) {
            //api_params.handle = overlay_ip_to_public_ip_nat_hdl_;
            //mapping_impl_db()->nat_tbl()->release(&api_params);
            mapping_impl_db()->nat_tbl()->remove(overlay_ip_to_public_ip_nat_hdl_);
        }
        if (public_ip_to_overlay_ip_nat_hdl_) {
            //api_params.handle = public_ip_to_overlay_ip_nat_hdl_;
            //mapping_impl_db()->nat_tbl()->release(&api_params);
            mapping_impl_db()->nat_tbl()->remove(public_ip_to_overlay_ip_nat_hdl_);
        }
        if (overlay_ip_to_provider_ip_nat_hdl_) {
            //api_params.handle = overlay_ip_to_provider_ip_nat_hdl_;
            //mapping_impl_db()->nat_tbl()->release(&api_params);
            mapping_impl_db()->nat_tbl()->remove(overlay_ip_to_provider_ip_nat_hdl_);
        }
        if (provider_ip_to_overlay_ip_nat_hdl_) {
            //api_params.handle = provider_ip_to_overlay_ip_nat_hdl_;
            //mapping_impl_db()->nat_tbl()->release(&api_params);
            mapping_impl_db()->nat_tbl()->remove(provider_ip_to_overlay_ip_nat_hdl_);
        }
    }
    if (mapping_hdl_.valid()) {
        api_params.handle = mapping_hdl_;
        mapping_impl_db()->mappings_tbl()->remove(&api_params);
    }
    return SDK_RET_OK;
}

sdk_ret_t
mapping_impl::release_local_ip_mapping_resources_(api_base *api_obj) {
    sdk_table_api_params_t    api_params = { 0 };

    if (overlay_ip_hdl_.valid()) {
        api_params.handle = overlay_ip_hdl_;
        mapping_impl_db()->local_ip_mapping_tbl()->release(&api_params);
    }
    if (public_ip_hdl_.valid()) {
        api_params.handle = public_ip_hdl_;
        mapping_impl_db()->local_ip_mapping_tbl()->release(&api_params);
    }
    if (provider_ip_hdl_.valid()) {
        api_params.handle = provider_ip_hdl_;
        mapping_impl_db()->local_ip_mapping_tbl()->release(&api_params);
    }
    if (mapping_hdl_.valid()) {
        api_params.handle = mapping_hdl_;
        mapping_impl_db()->mappings_tbl()->release(&api_params);
    }

    // TODO: change the api calls here once DM APIs are standardized
    if (overlay_ip_to_public_ip_nat_hdl_) {
        //api_params.handle = overlay_ip_to_public_ip_nat_hdl_;
        //mapping_impl_db()->nat_tbl()->release(&api_params);
        mapping_impl_db()->nat_tbl()->release(overlay_ip_to_public_ip_nat_hdl_);
    }
    if (public_ip_to_overlay_ip_nat_hdl_) {
        //api_params.handle = public_ip_to_overlay_ip_nat_hdl_;
        //mapping_impl_db()->nat_tbl()->release(&api_params);
        mapping_impl_db()->nat_tbl()->release(public_ip_to_overlay_ip_nat_hdl_);
    }
    if (overlay_ip_to_provider_ip_nat_hdl_) {
        //api_params.handle = overlay_ip_to_provider_ip_nat_hdl_;
        //mapping_impl_db()->nat_tbl()->release(&api_params);
        mapping_impl_db()->nat_tbl()->release(overlay_ip_to_provider_ip_nat_hdl_);
    }
    if (provider_ip_to_overlay_ip_nat_hdl_) {
        //api_params.handle = provider_ip_to_overlay_ip_nat_hdl_;
        //mapping_impl_db()->nat_tbl()->release(&api_params);
        mapping_impl_db()->nat_tbl()->release(provider_ip_to_overlay_ip_nat_hdl_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
mapping_impl::release_remote_ip_mapping_resources_(api_base *api_obj) {
    sdk_table_api_params_t    api_params = { 0 };

    if (mapping_hdl_.valid()) {
        api_params.handle = mapping_hdl_;
        mapping_impl_db()->mappings_tbl()->release(&api_params);
    }
    return SDK_RET_OK;
}

sdk_ret_t
mapping_impl::release_resources(api_base *api_obj) {
    if (is_local_) {
        return release_local_ip_mapping_resources_(api_obj);
    }
    return release_remote_ip_mapping_resources_(api_obj);
}

sdk_ret_t
mapping_impl::add_nat_entries_(pds_mapping_spec_t *spec) {
#if 0
    sdk_ret_t           ret;
    nat_actiondata_t    nat_data = { 0 };

    // allocate NAT table entries
    if (spec->public_ip_valid) {
        // add private to public IP xlation NAT entry
        PDS_IMPL_FILL_NAT_DATA(&nat_data, &spec->public_ip);
        ret =
            mapping_impl_db()->nat_tbl()->insert_atid(&nat_data,
                                                      handle_.local_.overlay_ip_to_public_ip_nat_hdl_);
        if (ret != SDK_RET_OK) {
            return ret;
        }

        // add public to private IP xlation NAT entry
        PDS_IMPL_FILL_NAT_DATA(&nat_data, &spec->key.ip_addr);
        ret =
            mapping_impl_db()->nat_tbl()->insert_atid(&nat_data,
                                                      handle_.local_.public_ip_to_overlay_ip_nat_hdl_);
        if (ret != SDK_RET_OK) {
            goto error;
        }
    }
    return SDK_RET_OK;

error:
    // TODO: handle cleanup in case of failure
    return ret;
#endif
    return SDK_RET_OK;
}

sdk_ret_t
mapping_impl::add_local_ip_mapping_entries_(vpc_entry *vpc,
                                            pds_mapping_spec_t *spec) {
#if 0
    sdk_ret_t                           ret;
    vnic_impl                           *vnic_impl_obj;
    local_ip_mapping_swkey_t            local_ip_mapping_key = { 0 };
    local_ip_mapping_appdata_t          local_ip_mapping_data = { 0 };
    remote_vnic_mapping_tx_swkey_t      remote_vnic_mapping_tx_key = { 0 };
    remote_vnic_mapping_tx_appdata_t    remote_vnic_mapping_tx_data = { 0 };
    sdk_table_api_params_t              api_params = { 0 };

    // add entry to LOCAL_IP_MAPPING table for overlay IP
    vnic_impl_obj =
        (vnic_impl *)vnic_db()->vnic_find(&spec->vnic)->impl();
    PDS_IMPL_FILL_LOCAL_IP_MAPPING_SWKEY(&local_ip_mapping_key,
                                         vnic_impl_obj->hw_id(),
                                         &spec->key.ip_addr, true);
    PDS_IMPL_FILL_LOCAL_IP_MAPPING_APPDATA(&local_ip_mapping_data, vpc->hw_id(),
                                           handle_.local_.overlay_ip_to_public_ip_nat_hdl_,
                                           IP_TYPE_OVERLAY);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&api_params, &local_ip_mapping_key,
                                   &local_ip_mapping_data,
                                   LOCAL_IP_MAPPING_LOCAL_IP_MAPPING_INFO_ID,
                                   handle_.local_.overlay_ip_hdl_);
    ret = mapping_impl_db()->local_ip_mapping_tbl()->insert(&api_params);
    if (ret != SDK_RET_OK) {
        goto error;
    }

    // add entry to LOCAL_IP_MAPPING table for public IP
    if (spec->public_ip_valid) {
        PDS_IMPL_FILL_LOCAL_IP_MAPPING_SWKEY(&local_ip_mapping_key,
                                             vnic_impl_obj->hw_id(),
                                             &spec->public_ip, true);
        PDS_IMPL_FILL_LOCAL_IP_MAPPING_APPDATA(&local_ip_mapping_data,
                                               vpc->hw_id(),
                                               handle_.local_.public_ip_to_overlay_ip_nat_hdl_,
                                               IP_TYPE_PUBLIC);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&api_params,
                                       &local_ip_mapping_key,
                                       &local_ip_mapping_data,
                                       LOCAL_IP_MAPPING_LOCAL_IP_MAPPING_INFO_ID,
                                       handle_.local_.public_ip_hdl_);
        ret = mapping_impl_db()->local_ip_mapping_tbl()->insert(&api_params);
        if (ret != SDK_RET_OK) {
            goto error;
        }
    }
    // add entry to REMOTE_VNIC_MAPPING_TX table for overlay & public IPs
    add_remote_vnic_mapping_tx_entries_(vpc, spec);

    return SDK_RET_OK;

error:

    // TODO: handle cleanup in case of failure
    return ret;
#endif
    return SDK_RET_OK;
}

sdk_ret_t
mapping_impl::program_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t             ret;
    pds_mapping_spec_t    *spec;
    vpc_entry             *vpc;
    subnet_entry          *subnet;

    spec = &obj_ctxt->api_params->mapping_spec;
    vpc = vpc_db()->find(&spec->key.vpc);
    subnet = subnet_db()->find(&spec->subnet);
    PDS_TRACE_DEBUG("Programming mapping (vpc %u, ip %s), subnet %u, tep %s, "
                    "overlay mac %s, fabric encap type %u "
                    "fabric encap value %u, vnic %u",
                    spec->key.vpc.id, ipaddr2str(&spec->key.ip_addr),
                    spec->subnet.id, ipv4addr2str(spec->tep.ip_addr),
                    macaddr2str(spec->overlay_mac), spec->fabric_encap.type,
                    spec->fabric_encap.val.value, spec->vnic.id);
    if (is_local_) {
        // allocate NAT table entries
        ret = add_nat_entries_(spec);
        if (ret != SDK_RET_OK) {
            goto error;
        }

        ret = add_local_ip_mapping_entries_(vpc, spec);
        if (ret != SDK_RET_OK) {
            goto error;
        }
    } else {
        ret = add_remote_ip_mapping_entries_(vpc, spec);
        if (ret != SDK_RET_OK) {
            goto error;
        }
    }
    return SDK_RET_OK;

error:

    return ret;
}

sdk_ret_t
mapping_impl::cleanup_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

sdk_ret_t
mapping_impl::update_hw(api_base *curr_obj, api_base *prev_obj,
                        obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

sdk_ret_t
mapping_impl::activate_hw(api_base *api_obj, pds_epoch_t epoch,
                          api_op_t api_op, obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

void
mapping_impl::fill_mapping_spec_(
}

sdk_ret_t
mapping_impl::read_local_mapping_(vpc_entry *vpc, pds_mapping_spec_t *spec) {
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
mapping_impl::read_remote_mapping_(vpc_entry *vpc, pds_mapping_spec_t *spec) {
    sdk_ret_t ret;
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
mapping_impl::read_hw(pds_mapping_key_t *key, pds_mapping_info_t *info) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    nat_actiondata_t nat_data = { 0 };

    vpc = vpc_db()->find(&key->vpc);
    if (is_local_) {
        ret = read_local_mapping_(vpc, &info->spec);
    } else {
        ret = read_remote_mapping_(vpc, &info->spec);
    }
    if (ret != SDK_RET_OK) {
        return ret;
    }
    return ret;
}

/// \@}    // end of PDS_MAPPING_IMPL

}    // namespace impl
}    // namespace api
