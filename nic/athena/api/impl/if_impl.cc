//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of interface
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/if.hpp"
#include "nic/athena/api/impl/if_impl.hpp"
#include "nic/athena/api/impl/pds_impl_state.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "gen/p4gen/athena/include/p4pd.h"

namespace api {
namespace impl {

/// \defgroup IF_IMPL_IMPL - interface datapath implementation
/// \ingroup IF_IMPL
/// \@{

if_impl *
if_impl::factory(pds_if_spec_t *spec) {
    if_impl *impl;

    impl = if_impl_db()->alloc();
    if (unlikely(impl == NULL)) {
        return NULL;
    }
    new (impl) if_impl();
    return impl;
}

void
if_impl::destroy(if_impl *impl) {
    impl->~if_impl();
    if_impl_db()->free(impl);
}

impl_base *
if_impl::clone(void) {
    if_impl *cloned_impl;

    cloned_impl = if_impl_db()->alloc();
    new (cloned_impl) if_impl();
    // deep copy is not needed as we don't store pointers
    *cloned_impl = *this;
    return cloned_impl;
}

sdk_ret_t
if_impl::free(if_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
if_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                           api_obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    pds_if_spec_t *spec = &obj_ctxt->api_params->if_spec;

    if (spec->type != IF_TYPE_UPLINK) {
        // nothing to reserve
        return SDK_RET_OK;
    }

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();
        ret = if_impl_db()->lif_idxr()->alloc(&idx);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to alloc lif hw id for uplink %s, err %u",
                          spec->key.id, ret);
            return ret;
        }
        hw_id_ = idx;
        break;

    case API_OP_UPDATE:
        // we will use the same h/w resources as the original object
    default:
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_impl::release_resources(api_base *api_obj) {
    if_entry *intf = (if_entry *)api_obj;

    if (intf->type() != IF_TYPE_UPLINK) {
        return SDK_RET_OK;
    }
    if (hw_id_ != 0xFFFF) {
        if_impl_db()->lif_idxr()->free(hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_impl::nuke_resources(api_base *api_obj) {
    return this->release_resources(api_obj);
}

sdk_ret_t
if_impl::activate_create_(pds_epoch_t epoch, if_entry *intf,
                          pds_if_spec_t *spec) {
    sdk_ret_t ret;
    uint32_t tm_port;
    p4pd_error_t p4pd_ret;

    PDS_TRACE_DEBUG("Activating if %s, type %u, admin state %u",
                    spec->key.id, spec->type, spec->admin_state);

    if (spec->type == IF_TYPE_UPLINK) {
        PDS_TRACE_DEBUG("if_index_t: %x, logical_port:%u\n",
                intf->ifindex(), sdk::lib::catalog::ifindex_to_logical_port(intf->ifindex()));
        // program the lif id in the TM
        tm_port =
            g_pds_state.catalogue()->ifindex_to_tm_port(intf->ifindex());
        PDS_TRACE_DEBUG("Creating uplink if %s, port %s, hw_id_ %u, "
                        "tm_port %u", spec->key.id, spec->uplink_info.port.str(),
                        hw_id_, tm_port);
        ret = sdk::asic::pd::asicpd_tm_uplink_lif_set(tm_port,
                                                      sdk::lib::catalog::ifindex_to_logical_port(intf->ifindex()));
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program uplink %s's lif %u in TM "
                          "register", spec->key.id, hw_id_);
        }
    }

    return SDK_RET_OK;
}

sdk_ret_t
if_impl::activate_delete_(pds_epoch_t epoch, if_entry *intf) {
    p4pd_error_t p4pd_ret;

    return SDK_RET_OK;
}

sdk_ret_t
if_impl::activate_update_(pds_epoch_t epoch, if_entry *intf,
                          api_obj_ctxt_t *obj_ctxt) {
    pds_if_spec_t *spec = &obj_ctxt->api_params->if_spec;

    switch(spec->type) {
    case IF_TYPE_UPLINK:
        if (obj_ctxt->upd_bmap & PDS_IF_UPD_ADMIN_STATE) {
            // TODO: @akoradha, we need to bring port down here !!
            return SDK_RET_INVALID_OP;
        }
        return SDK_RET_OK;

    case IF_TYPE_CONTROL:
        return SDK_RET_OK;

    case IF_TYPE_L3:
        return SDK_RET_OK;

    case IF_TYPE_ETH:
        return SDK_RET_OK;

    default:
        break;
    }
    return SDK_RET_INVALID_ARG;
}

sdk_ret_t
if_impl::activate_hw(api_base *api_obj, api_base *orig_obj, pds_epoch_t epoch,
                     api_op_t api_op, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_if_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->if_spec;
        ret = activate_create_(epoch, (if_entry *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (if_entry *)api_obj);
        break;

    case API_OP_UPDATE:
        ret = activate_update_(epoch, (if_entry *)api_obj, obj_ctxt);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
if_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    if_entry *intf;
    if_index_t if_index;
    pds_if_spec_t *spec;
    uint8_t num_lifs = 0;
    p4pd_error_t p4pd_ret;
    pds_obj_key_t lif_key;
    pds_if_info_t *if_info = (pds_if_info_t *)info;

    intf = if_db()->find((pds_obj_key_t *)key);
    spec = &if_info->spec;
    uint32_t port_num;

    if_info->status.ifindex = intf->ifindex();
    if (spec->type == IF_TYPE_UPLINK) {
        if_info->status.uplink_status.lif_id = hw_id_;
    } else if (spec->type == IF_TYPE_HOST) {
        if_index =
            LIF_IFINDEX(HOST_IFINDEX_TO_IF_ID(objid_from_uuid(spec->key)));
        lif_key = uuid_from_objid(if_index);
        lif_impl *lif = (lif_impl *)lif_impl_db()->find(&lif_key);
        if_info->status.host_if_status.lifs[num_lifs++] = lif->key();
        if_info->status.host_if_status.num_lifs = num_lifs;
        strncpy(if_info->status.host_if_status.name,
                intf->name().c_str(), SDK_MAX_NAME_LEN);
        MAC_ADDR_COPY(if_info->status.host_if_status.mac_addr,
                      intf->host_if_mac());
    }
    return SDK_RET_OK;
}

/// \@}    // end of IF_IMPL_IMPL

}    // namespace impl
}    // namespace api
