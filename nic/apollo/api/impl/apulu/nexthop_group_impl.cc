//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of nexthop group
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/nexthop_group.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/impl/apulu/tep_impl.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_impl.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_group_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/svc/nh_svc.hpp"
#include "nic/apollo/api/impl/apulu/svc/svc_utils.hpp"

namespace api {
namespace impl {

#define ecmp_info       action_u.ecmp_ecmp_info

/// \defgroup PDS_NEXTHOP_GROUP_IMPL - nexthop group datapath implementation
/// \ingroup PDS_NEXTHOP
/// \@{

nexthop_group_impl *
nexthop_group_impl::factory(pds_nexthop_group_spec_t *spec) {
    nexthop_group_impl *impl;

    impl = nexthop_group_impl_db()->alloc();
    if (unlikely(impl == NULL)) {
        return NULL;
    }
    new (impl) nexthop_group_impl();
    return impl;
}

void
nexthop_group_impl::destroy(nexthop_group_impl *impl) {
    impl->~nexthop_group_impl();
     nexthop_group_impl_db()->free(impl);
}

impl_base *
nexthop_group_impl::clone(void) {
    nexthop_group_impl *cloned_impl;

    cloned_impl = nexthop_group_impl_db()->alloc();
    new (cloned_impl) nexthop_group_impl();
    // deep copy is not needed as we don't store pointers
    *cloned_impl = *this;
    return cloned_impl;
}

sdk_ret_t
nexthop_group_impl::free(nexthop_group_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                                      api_obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    pds_nexthop_group_spec_t *spec;

    // if this object is restored from persistent storage
    // resources are reserved already
    if (api_obj->in_restore_list()) {
        return SDK_RET_OK;
    }

    spec = &obj_ctxt->api_params->nexthop_group_spec;
    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        // validate the nexthop group
        if ((spec->type == PDS_NHGROUP_TYPE_OVERLAY_ECMP) &&
            (spec->num_nexthops > PDS_IMPL_MAX_OVERLAY_NHGROUP_SIZE)) {
            PDS_TRACE_ERR("Max nexthops %u in overlay nexthop group %s "
                          "exceeded max supported %u", spec->num_nexthops,
                          spec->key.str(), PDS_IMPL_MAX_OVERLAY_NHGROUP_SIZE);
            return SDK_RET_INVALID_ARG;
        }
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();
        // reserve an entry in NEXTHOP_GROUP table
        ret = nexthop_group_impl_db()->nhgroup_idxr()->alloc(&idx);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to reserve an entry in ECMP table, "
                          "for nexthop group %s, err %u", spec->key.str(), ret);
            return ret;
        }
        hw_id_ = idx;
        if (spec->type == PDS_NHGROUP_TYPE_UNDERLAY_ECMP) {
            if (spec->num_nexthops) {
                ret = nexthop_impl_db()->nh_idxr()->alloc_block(&idx,
                                                        spec->num_nexthops);
                if (ret != SDK_RET_OK) {
                    PDS_TRACE_ERR("Failed to reserve %u entries in "
                                  "NEXTHOP table for nexthop group %s, "
                                  "err %u", spec->num_nexthops,
                                  spec->key.str(), ret);
                    goto error;
                }
                nh_base_hw_id_ = idx;
            }
        }
    case API_OP_UPDATE:
        // we will use the same h/w resources as the original object
    default:
        break;
    }
    return SDK_RET_OK;

error:

    PDS_TRACE_ERR("Failed to acquire h/w resources for nexthop group %s, "
                  "err %u", spec->key.str(), ret);
    if (hw_id_ != 0xFFFF) {
        nexthop_group_impl_db()->nhgroup_idxr()->free(hw_id_);
        hw_id_ = 0xFFFF;
    }
    return ret;
}

sdk_ret_t
nexthop_group_impl::release_resources(api_base *api_obj) {
    nexthop_group *nhgroup = (nexthop_group *)api_obj;

    if (hw_id_ != 0xFFFF) {
        nexthop_group_impl_db()->nhgroup_idxr()->free(hw_id_);
    }
    if (nh_base_hw_id_ != 0xFFFF) {
        nexthop_impl_db()->nh_idxr()->free(nh_base_hw_id_,
                                           nhgroup->num_nexthops());
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::nuke_resources(api_base *api_obj) {
    // for indexers, release_resources() does the job
    return this->release_resources(api_obj);
}

sdk_ret_t
nexthop_group_impl::populate_ecmp_tep_info_(pds_nexthop_group_spec_t *spec,
                                            ecmp_actiondata_t *ecmp_data)
{
    tep_entry *entry;
    tep_impl *tep;

    for (uint8_t i = 0; i < spec->num_nexthops; i++) {
        entry = tep_db()->find(&spec->nexthops[i].tep);
        if (!entry) {
            PDS_TRACE_ERR("Tep entry %s not found in nexthop group %s",
                          spec->nexthops[i].tep.str(), spec->key.str());
            return SDK_RET_ERR;
        }
        tep = (tep_impl *)entry->impl();
        switch (i) {
        case 0:
            ecmp_data->ecmp_info.tunnel_id1 = tep->hw_id();
            break;
        case 1:
            ecmp_data->ecmp_info.tunnel_id2 = tep->hw_id();
            break;
        case 2:
            ecmp_data->ecmp_info.tunnel_id3 = tep->hw_id();
            break;
        case 3:
            ecmp_data->ecmp_info.tunnel_id4 = tep->hw_id();
            break;
        default:
            PDS_TRACE_ERR("Unexpected nexthop count %u in nexthop group %s",
                          spec->num_nexthops, spec->key.str());
            return SDK_RET_ERR;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::activate_create_update_(pds_epoch_t epoch,
                                            nexthop_group *nh_group,
                                            pds_nexthop_group_spec_t *spec) {
    sdk_ret_t ret;
    p4pd_error_t p4pd_ret;
    nexthop_info_entry_t nh_data;
    ecmp_actiondata_t ecmp_data = { 0 };

    ecmp_data.action_id = ECMP_ECMP_INFO_ID;
    ecmp_data.ecmp_info.num_nexthops = spec->num_nexthops;
    if (spec->type == PDS_NHGROUP_TYPE_OVERLAY_ECMP) {
        // populate ECMP table entry data
        ecmp_data.ecmp_info.nexthop_type = NEXTHOP_TYPE_TUNNEL;
        ret = populate_ecmp_tep_info_(spec, &ecmp_data);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    } else if (spec->type == PDS_NHGROUP_TYPE_UNDERLAY_ECMP) {
        // program the nexthops first in NEXTHOP table
        for (uint8_t i = 0; i < spec->num_nexthops; i++) {
            ret = populate_underlay_nh_info_(&spec->nexthops[i], &nh_data);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Programming of nexthop %u in NEXTHOP table for "
                              "nexthop group %s failed, err %u",
                              i, spec->key.str(), ret);
                return ret;
            }
            ret = nh_data.write(nh_base_hw_id_ + i);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to program nexthop %u of nexthop "
                              "group %s at idx %u", i, spec->key.str(),
                              nh_base_hw_id_ + i);
                return sdk::SDK_RET_HW_PROGRAM_ERR;
            }
        }
        // populate ECMP table entry data
        ecmp_data.ecmp_info.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
        ecmp_data.ecmp_info.nexthop_base = nh_base_hw_id_;
    }

    // program the ECMP table now
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_ECMP, hw_id_,
                                       NULL, NULL, &ecmp_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program nexthop group %s at idx %u in "
                      "ECMP table", spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::activate_delete_(pds_epoch_t epoch,
                                     nexthop_group *nh_group) {
    uint8_t num_nexthops;
    sdk_ret_t ret;
    pds_obj_key_t key;
    pds_nexthop_group_type_t type;
    p4pd_error_t p4pd_ret;
    nexthop_info_entry_t nh_data = { 0 };
    ecmp_actiondata_t ecmp_data = { 0 };

    memset(&nh_data, 0, nh_data.entry_size());
    key = nh_group->key();
    type = nh_group->type();
    num_nexthops = nh_group->num_nexthops();
    if (type == PDS_NHGROUP_TYPE_UNDERLAY_ECMP) {
        for (uint8_t i = 0; i < num_nexthops; i++) {
            ret = nh_data.write(nh_base_hw_id_ + i);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to program nexthop %u of nexthop "
                              "group %s at idx %u", i, key.str(),
                              nh_base_hw_id_ + i);
                return sdk::SDK_RET_HW_PROGRAM_ERR;
            }
        }
    }
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_ECMP, hw_id_,
                                       NULL, NULL, &ecmp_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program nexthop group %s at idx %u",
                      key.str(), hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::activate_hw(api_base *api_obj, api_base *orig_obj,
                                pds_epoch_t epoch, api_op_t api_op,
                                api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_nexthop_group_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
    case API_OP_UPDATE:
        spec = &obj_ctxt->api_params->nexthop_group_spec;
        ret = activate_create_update_(epoch, (nexthop_group *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (nexthop_group *)api_obj);
        break;

    default:
        return SDK_RET_INVALID_OP;
    }
    return ret;
}

sdk_ret_t
nexthop_group_impl::fill_status_(pds_nexthop_group_status_t *status,
                                 ecmp_actiondata_t *ecmp_data,
                                 pds_nexthop_group_spec_t *spec) {
    uint16_t nh_base_hw_id = ecmp_data->ecmp_info.nexthop_base;
    uint16_t tunnel_id;
    sdk_ret_t ret;

    status->hw_id = hw_id_;
    if (ecmp_data->ecmp_info.nexthop_type == NEXTHOP_TYPE_NEXTHOP) {
        status->nh_base_idx = nh_base_hw_id_;
        for (uint8_t i = 0; i < ecmp_data->ecmp_info.num_nexthops; i++) {
            ret = fill_nh_status_(&status->nexthops[i], nh_base_hw_id + i);
        }
    } else if (ecmp_data->ecmp_info.nexthop_type == NEXTHOP_TYPE_TUNNEL) {
        for (uint8_t i = 0; i < ecmp_data->ecmp_info.num_nexthops; i++) {
            switch (i) {
            case 0:
                tunnel_id = ecmp_data->ecmp_info.tunnel_id1;
                break;
            case 1:
                tunnel_id = ecmp_data->ecmp_info.tunnel_id2;
                break;
            case 2:
                tunnel_id = ecmp_data->ecmp_info.tunnel_id3;
                break;
            case 3:
                tunnel_id = ecmp_data->ecmp_info.tunnel_id4;
                break;
            default:
                PDS_TRACE_ERR("Unexpected nexthop count %u in nexthop group %s",
                              spec->num_nexthops, spec->key.str());
                return SDK_RET_ERR;
            }
            ret = fill_nh_status_overlay_tep_(&status->nexthops[i], tunnel_id);
        }
    }

    return ret;
}

sdk_ret_t
nexthop_group_impl::fill_spec_(pds_nexthop_group_spec_t *spec,
                               ecmp_actiondata_t *ecmp_data) {
    uint16_t nh_base_hw_id;

    spec->num_nexthops = ecmp_data->ecmp_info.num_nexthops;
    if (ecmp_data->ecmp_info.nexthop_type == NEXTHOP_TYPE_TUNNEL) {
        spec->type = PDS_NHGROUP_TYPE_OVERLAY_ECMP;
        for (uint8_t i = 0; i < spec->num_nexthops; i++) {
            spec->nexthops[i].type = PDS_NH_TYPE_OVERLAY;
        }
    } else if (ecmp_data->ecmp_info.nexthop_type == NEXTHOP_TYPE_NEXTHOP) {
        spec->type = PDS_NHGROUP_TYPE_UNDERLAY_ECMP;
        nh_base_hw_id = ecmp_data->ecmp_info.nexthop_base;
        for (uint8_t i = 0; i < spec->num_nexthops; i++) {
            fill_nh_spec_(&spec->nexthops[i], nh_base_hw_id + i);
        }
    }

    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::fill_info_(pds_nexthop_group_info_t *info) {
    p4pd_error_t p4pd_ret;
    ecmp_actiondata_t ecmp_data;

    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_ECMP, hw_id_,
                                      NULL, NULL, &ecmp_data);
    if (unlikely(p4pd_ret != P4PD_SUCCESS)) {
        PDS_TRACE_ERR("Failed to read nexthop group table at index %u ret %u",
                      hw_id_, p4pd_ret);
        return sdk::SDK_RET_HW_READ_ERR;
    }

    fill_spec_(&info->spec, &ecmp_data);
    fill_status_(&info->status, &ecmp_data, &info->spec);

    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::read_hw(api_base *api_obj, obj_key_t *key,
                            obj_info_t *info) {
    sdk_ret_t ret;
    pds_nexthop_group_info_t *dinfo = (pds_nexthop_group_info_t *)info;

    ret = fill_info_(dinfo);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::backup(obj_info_t *info, upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    pds::NhGroupGetResponse proto_msg;
    pds_nexthop_group_info_t *nh_group_info;
    upg_obj_tlv_t *tlv;

    tlv = (upg_obj_tlv_t *)upg_info->mem;
    nh_group_info = (pds_nexthop_group_info_t *)info;

    ret = fill_info_(nh_group_info);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    // convert api info to proto
    pds_nh_group_api_info_to_proto(nh_group_info, (void *)&proto_msg);
    ret = pds_svc_serialize_proto_msg(upg_info, tlv, &proto_msg);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to serialize nh group %s err %u",
                      nh_group_info->spec.key.str(), ret);
    }
    return ret;
}

sdk_ret_t
nexthop_group_impl::restore_resources(obj_info_t *info) {
    sdk_ret_t ret;
    pds_nexthop_group_info_t *nh_group_info;
    pds_nexthop_group_spec_t *spec;
    pds_nexthop_group_status_t *status;

    nh_group_info = (pds_nexthop_group_info_t *)info;
    spec = &nh_group_info->spec;
    status = &nh_group_info->status;

    ret = nexthop_group_impl_db()->nhgroup_idxr()->alloc(status->hw_id);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore an entry in ECMP table, "
                      "for nexthop group %s, err %u hw id %u",
                      spec->key.str(), ret, status->hw_id);
        return ret;
    }
    hw_id_ = status->hw_id;
    if (spec->type == PDS_NHGROUP_TYPE_UNDERLAY_ECMP) {
        if (spec->num_nexthops) {
            ret = nexthop_impl_db()->nh_idxr()->alloc_block(status->nh_base_idx,
                                                            spec->num_nexthops);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to restore %u entries in "
                              "NEXTHOP table for nexthop group %s, "
                              "err %u base hw id %u", spec->num_nexthops,
                              spec->key.str(), ret, status->nh_base_idx);
                goto error;
            }
            nh_base_hw_id_ = status->nh_base_idx;
        }
    }
    return SDK_RET_OK;

error:

    if (hw_id_ != 0xFFFF) {
        nexthop_group_impl_db()->nhgroup_idxr()->free(hw_id_);
        hw_id_ = 0xFFFF;
    }
    return ret;
}

sdk_ret_t
nexthop_group_impl::restore(obj_info_t *info, upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    pds::NhGroupGetResponse proto_msg;
    pds_nexthop_group_info_t *nh_group_info;
    upg_obj_tlv_t *tlv;
    uint32_t obj_size, meta_size;

    tlv = (upg_obj_tlv_t *)upg_info->mem;
    nh_group_info = (pds_nexthop_group_info_t *)info;
    obj_size = tlv->len;
    meta_size = sizeof(upg_obj_tlv_t);
    // fill up the size, even if it fails later. to try and restore next obj
    upg_info->size = obj_size + meta_size;
    // de-serialize proto msg
    if (proto_msg.ParseFromArray(tlv->obj, tlv->len) == false) {
        PDS_TRACE_ERR("Failed to de-serialize nexthop group");
        return SDK_RET_OOM;
    }
    // convert proto msg to nh group info
    ret = pds_nh_group_proto_to_api_info(nh_group_info, &proto_msg);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to convert nh group proto msg to info, err %u",
                      ret);
        return ret;
    }
    // now restore hw resources
    ret = restore_resources((obj_info_t *)nh_group_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore hw resources for nh group %s",
                      nh_group_info->spec.key.str());
        return ret;
    }
    return ret;
}

/// \@}    // end of PDS_NEXTHOP_GROUP_IMPL

}    // namespace impl
}    // namespace api
