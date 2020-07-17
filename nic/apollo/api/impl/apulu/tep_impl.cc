//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// TEP datapath implementation
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/tep.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/internal/pds_route.hpp"
#include "nic/apollo/api/impl/ipsec/ipseccb.hpp"
#include "nic/apollo/api/impl/apulu/tep_impl.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_impl.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_group_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/svc/tunnel_svc.hpp"
#include "nic/apollo/api/impl/apulu/svc/svc_utils.hpp"
#include "nic/apollo/api/impl/apulu/ipsec_impl.hpp"

#define PDS_NUM_NH_NO_ECMP                 1
#define tunnel_action                      action_u.tunnel_tunnel_info
#define tunnel2_action                     action_u.tunnel2_tunnel2_info

#define PDS_IMPL_FILL_TEP_DATA_FROM_NH(tep_data, nh_hw_id)                     \
{                                                                              \
    (tep_data)->tunnel_action.nexthop_base = (nh_hw_id);                       \
    (tep_data)->tunnel_action.num_nexthops = PDS_NUM_NH_NO_ECMP;               \
}

#define PDS_IMPL_FILL_TEP_DATA_FROM_NH_GROUP(tep_data, base_nh_hw_id, num_nh)  \
{                                                                              \
    (tep_data)->tunnel_action.nexthop_base = (base_nh_hw_id);                  \
    (tep_data)->tunnel_action.num_nexthops = (num_nh);                         \
}

namespace api {
namespace impl {

/// \defgroup PDS_TEP_IMPL - tep datapath implementation
/// \ingroup PDS_TEP
/// @{

tep_impl *
tep_impl::factory(pds_tep_spec_t *pds_tep) {
    tep_impl *impl;

    impl = tep_impl_db()->alloc();
    new (impl) tep_impl();
    return impl;
}

void
tep_impl::destroy(tep_impl *impl) {
    impl->~tep_impl();
    tep_impl_db()->free(impl);
}

impl_base *
tep_impl::clone(void) {
    tep_impl *cloned_impl;

    cloned_impl = tep_impl_db()->alloc();
    new (cloned_impl) tep_impl();
    // deep copy is not needed as we don't store pointers
    *cloned_impl = *this;
    return cloned_impl;
}

sdk_ret_t
tep_impl::free(tep_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                            api_obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    tep_entry *tep;
    pds_tep_spec_t *spec = &obj_ctxt->api_params->tep_spec;

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();
        if (spec->nh_type == PDS_NH_TYPE_OVERLAY) {
            tep = tep_db()->find(&spec->tep);
            if (tep == NULL) {
                PDS_TRACE_ERR("Failed to find nexthop TEP %s used by TEP %s",
                              spec->tep.str(), spec->key.str());
                return SDK_RET_INVALID_ARG;
            }
        }
        // if this object is restored from persistent storage
        // resources are reserved already
        if (api_obj->in_restore_list()) {
            return SDK_RET_OK;
        }

        // allocate a resource in TUNNEL and TUNNEL2 tables
        ret = tep_impl_db()->tunnel_idxr()->alloc(&idx);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to reserve entry in TUNNEL table for %s, "
                          "err %u", api_obj->key2str().c_str(), ret);
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
tep_impl::release_resources(api_base *api_obj) {
    tep_entry *tep;

    tep = (tep_entry *)api_obj;
    if (hw_id_ != 0xFFFF) {
        tep_impl_db()->tunnel_idxr()->free(hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::nuke_resources(api_base *api_obj) {
    tep_entry *tep;

    tep = (tep_entry *)api_obj;
    if (hw_id_ != 0xFFFF) {
        tep_impl_db()->tunnel_idxr()->free(hw_id_);
    }
    return SDK_RET_OK;
}

static inline sdk_ret_t
fill_p4_tep_data_from_nh_ (tep_entry *tep, pds_obj_key_t *nh_key,
                           tunnel_actiondata_t *tep_data)
{
    nexthop *nh;
    sdk_ret_t ret;
    nexthop_impl *nh_impl;

    nh = nexthop_db()->find(nh_key);
    if (unlikely(nh == NULL)) {
        PDS_TRACE_ERR("nh %s in TEP %s not found",
                      nh_key->str(), tep->key2str().c_str());
        return SDK_RET_INVALID_ARG;
    }
    nh_impl = (nexthop_impl *)nh->impl();
    PDS_IMPL_FILL_TEP_DATA_FROM_NH(tep_data, nh_impl->hw_id());
    return SDK_RET_OK;
}

static inline sdk_ret_t
fill_p4_tep_data_from_nhgroup_ (tep_entry *tep, pds_obj_key_t *nhgroup_key,
                                tunnel_actiondata_t *tep_data)
{
    sdk_ret_t ret;
    nexthop_group *nhgroup;
    nexthop_group_impl *nhgroup_impl;

    nhgroup = nexthop_group_db()->find(nhgroup_key);
    if (unlikely(nhgroup == NULL)) {
        PDS_TRACE_ERR("nhgroup %s in TEP %s not found",
                      nhgroup_key->str(), tep->key2str().c_str());
        return SDK_RET_INVALID_ARG;
    }
    nhgroup_impl = (nexthop_group_impl *)nhgroup->impl();
    PDS_IMPL_FILL_TEP_DATA_FROM_NH_GROUP(tep_data,
                                         nhgroup_impl->nh_base_hw_id(),
                                         nhgroup->num_nexthops());
    return SDK_RET_OK;
}

static inline sdk_ret_t
fill_p4_tep_data_from_ipsec_sa_ (tep_entry *tep, pds_obj_key_t *encrypt_sa_key,
                                 tunnel_actiondata_t *tep_data,
                                 pds_obj_key_t *nhgroup_key, pds_obj_key_t *nh_key)
{
    sdk_ret_t ret;
    ipsec_sa_entry *ipsec_sa;
    ipsec_sa_impl *ipsec_impl;
    nexthop *nh;
    nexthop_impl *nh_impl;
    nexthop_group *nhgroup;
    nexthop_group_impl *nhgroup_impl;
    uint32_t nh_idx;

    ipsec_sa = ipsec_sa_encrypt_find(encrypt_sa_key);
    if (ipsec_sa == NULL) {
        PDS_TRACE_ERR("ipsec encrypt sa %s in TEP %s not found",
                      encrypt_sa_key->str(), tep->key2str().c_str());
        return SDK_RET_INVALID_ARG;
    }
    ipsec_impl = (ipsec_sa_impl *)ipsec_sa->impl();
    nh_idx = ipsec_impl->nh_idx();
    PDS_TRACE_DEBUG("Programming tep with ipsec nh %d", nh_idx);
    PDS_IMPL_FILL_TEP_DATA_FROM_NH(tep_data, nh_idx);

    if (nh_key) {
        nh = nexthop_db()->find(nh_key);
        if (unlikely(nh == NULL)) {
            PDS_TRACE_ERR("nh %s in TEP %s not found",
                          nh_key->str(), tep->key2str().c_str());
            return SDK_RET_INVALID_ARG;
        }
        nh_impl = (nexthop_impl *)nh->impl();
        PDS_TRACE_DEBUG("Programming ipsec cb hw_id %u with nh %u",
                        ipsec_impl->hw_id(), nh_impl->hw_id());
        ipseccb_encrypt_update_nexthop_id(ipsec_impl->hw_id(), ipsec_impl->base_pa(),
                                          nh_impl->hw_id(), NEXTHOP_TYPE_NEXTHOP);
    } else if (nhgroup_key) {
        nhgroup = nexthop_group_db()->find(nhgroup_key);
        if (unlikely(nhgroup == NULL)) {
            PDS_TRACE_ERR("nhgroup %s in TEP %s not found",
                          nhgroup_key->str(), tep->key2str().c_str());
            return SDK_RET_INVALID_ARG;
        }
        nhgroup_impl = (nexthop_group_impl *)nhgroup->impl();
        PDS_TRACE_DEBUG("Programming ipsec cb hw_id %u with ecmp nh %u",
                        ipsec_impl->hw_id(), nhgroup_impl->hw_id());
        ipseccb_encrypt_update_nexthop_id(ipsec_impl->hw_id(), ipsec_impl->base_pa(),
                                          nhgroup_impl->hw_id(),
                                          NEXTHOP_TYPE_ECMP);
    }

    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::fill_tep_nh_info_(api_op_t api_op, tep_entry *tep,
                            pds_tep_spec_t *spec,
                            tunnel_actiondata_t *tep_data,
                            api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    tep_entry *tep2;
    pds_obj_key_t nh_key;
    pds_obj_key_t nhgroup_key;

    if ((api_op == API_OP_UPDATE)                       &&
        !(obj_ctxt->upd_bmap & PDS_TEP_UPD_NH_TYPE)     &&
        !(obj_ctxt->upd_bmap & PDS_TEP_UPD_UNDERLAY_NH) &&
        !(obj_ctxt->upd_bmap & PDS_TEP_UPD_OVERLAY_NH)) {
        // update operation, but forwarding information has not changed
        return SDK_RET_OK;
    }

    switch (spec->nh_type) {
    case PDS_NH_TYPE_UNDERLAY_ECMP:
        if (!tep->ipsec_encrypt_sa().valid()) {
            ret = fill_p4_tep_data_from_nhgroup_(tep, &spec->nh_group, tep_data);
        } else {
            pds_obj_key_t ipsec_sa_id = tep->ipsec_encrypt_sa();
            ret = fill_p4_tep_data_from_ipsec_sa_(tep, &ipsec_sa_id, tep_data,
                                                  &spec->nh_group, NULL);
        }
        if (ret != SDK_RET_OK) {
            return ret;
        }
        break;

    case PDS_NH_TYPE_UNDERLAY:
        if (!tep->ipsec_encrypt_sa().valid()) {
            ret = fill_p4_tep_data_from_nh_(tep, &spec->nh, tep_data);
        } else {
            pds_obj_key_t ipsec_sa_id = tep->ipsec_encrypt_sa();
            ret = fill_p4_tep_data_from_ipsec_sa_(tep, &ipsec_sa_id, tep_data,
                                                  NULL, &spec->nh);
        }
        if (ret != SDK_RET_OK) {
            return ret;
        }
        break;

    case PDS_NH_TYPE_OVERLAY:
        // tunnel pointing to another tunnel case, do recursive resolution
        tep2 = tep_db()->find(&spec->tep);
        if (unlikely(tep2 == NULL)) {
            PDS_TRACE_ERR("tep %s in overlay TEP %s not found",
                          spec->tep.str(), spec->key.str());
            return SDK_RET_INVALID_ARG;
        }
        if (tep2->nh_type() == PDS_NH_TYPE_UNDERLAY) {
            nh_key = tep2->nh();
            ret = fill_p4_tep_data_from_nh_(tep, &nh_key, tep_data);
            if (ret != SDK_RET_OK) {
                return ret;
            }
        } else if (tep2->nh_type() == PDS_NH_TYPE_UNDERLAY_ECMP) {
            nhgroup_key = tep2->nh_group();
            ret = fill_p4_tep_data_from_nhgroup_(tep, &nhgroup_key, tep_data);
            if (ret != SDK_RET_OK) {
                return ret;
            }
        } else if (tep2->nh_type() == PDS_NH_TYPE_BLACKHOLE) {
            PDS_IMPL_FILL_TEP_DATA_FROM_NH(tep_data,
                                           PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID);
        } else {
            SDK_ASSERT(tep2->nh_type() == PDS_NH_TYPE_NONE);
            // reachability is unknown at this time
            PDS_IMPL_FILL_TEP_DATA_FROM_NH(tep_data,
                                           PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID);
        }
        break;

    case PDS_NH_TYPE_BLACKHOLE:
        PDS_IMPL_FILL_TEP_DATA_FROM_NH(tep_data,
                                       PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID);
        break;

    case PDS_NH_TYPE_NONE:
        if (api_op == API_OP_CREATE) {
            // point to blackhole NH and wait for the update from routing stack
            PDS_IMPL_FILL_TEP_DATA_FROM_NH(tep_data,
                                           PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID);
        } else {
            // TEP update case
            if (obj_ctxt->upd_bmap & PDS_TEP_UPD_NH_TYPE) {
                // update to PDS_NH_TYPE_NONE is same as no update (i.e., user
                // wants to retain the current forwarding information), this can
                // happen when
                // 1. user doesn't provide any nexthop information (i.e.,
                //    PDS_NH_TYPE_NONE) and later
                // 2. routing stack updates the TEP with reachability
                //    information (thus changing the nh_type_ from
                //    PDS_NH_TYPE_NONE to something else) and then
                // 3. user updates some other attribute of TEP but still
                //    providing no nexthop information
                // in this case, nh_type_ change is detected but in reality user
                // intention is not to update it
            }
        }
        break;

    default:
        PDS_TRACE_ERR("Unsupported nh type %u in TEP %s spec",
                      spec->nh_type, spec->key.str());
        SDK_ASSERT_RETURN(false, SDK_RET_INVALID_ARG);
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::program_tunnel_(api_op_t api_op, tep_entry *tep, pds_tep_spec_t *spec,
                          tunnel_actiondata_t *tep_data,
                          api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    p4pd_error_t p4pd_ret;
    tunnel2_actiondata_t tep2_data = { 0 };

    if (api_op == API_OP_CREATE) {
        memset(tep_data, 0, sizeof(*tep_data));
    } else {
        // do read-modify-write
        p4pd_ret = p4pd_global_entry_read(P4TBL_ID_TUNNEL, hw_id_,
                                          NULL, NULL, tep_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to read TEP %s at TUNNEL table idx %u",
                          spec->key.str(), hw_id_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
    }

    // fill the nexthop information for this TEP
    ret = fill_tep_nh_info_(api_op, tep, spec, tep_data, obj_ctxt);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }

    if (spec->encap.type != PDS_ENCAP_TYPE_NONE) {
        tep_data->tunnel_action.vni = spec->encap.val.value;
    }
    if (spec->remote_ip.af == IP_AF_IPV4) {
        tep_data->tunnel_action.ip_type = IPTYPE_IPV4;
        memcpy(tep_data->tunnel_action.dipo, &spec->remote_ip.addr.v4_addr,
               IP4_ADDR8_LEN);
        // fill TUNNEL2 table data as well
        tep2_data.tunnel2_action.ip_type = IPTYPE_IPV4;
        memcpy(tep2_data.tunnel2_action.dipo, &spec->remote_ip.addr.v4_addr,
               IP4_ADDR8_LEN);
    } else if (spec->remote_ip.af == IP_AF_IPV6) {
        tep_data->tunnel_action.ip_type = IPTYPE_IPV6;
        sdk::lib::memrev(tep_data->tunnel_action.dipo,
                         spec->remote_ip.addr.v6_addr.addr8,
                         IP6_ADDR8_LEN);
        // fill TUNNEL2 table data as well
        tep2_data.tunnel2_action.ip_type = IPTYPE_IPV6;
        sdk::lib::memrev(tep2_data.tunnel2_action.dipo,
                         spec->remote_ip.addr.v6_addr.addr8,
                         IP6_ADDR8_LEN);
    }
    sdk::lib::memrev(tep_data->tunnel_action.dmaci, spec->mac, ETH_ADDR_LEN);
    if (spec->tos) {
        tep_data->tunnel_action.tos_override = 1;
        tep_data->tunnel_action.tos = spec->tos;
    }
    tep_data->action_id = TUNNEL_TUNNEL_INFO_ID;
    // program TUNNEL table
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_TUNNEL, hw_id_,
                                       NULL, NULL, tep_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program TEP %s at idx %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    // program TUNNEL2 table
    tep2_data.action_id = TUNNEL2_TUNNEL2_INFO_ID;
    tep2_data.tunnel2_action.encap_type = P4_REWRITE_ENCAP_VXLAN;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_TUNNEL2, hw_id_,
                                       NULL, NULL, &tep2_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program TEP %s in TUNNEL2 table at idx %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::program_inter_dc_tunnel_(api_op_t api_op, tep_entry *tep,
                                   tep_entry *orig_tep, pds_tep_spec_t *spec,
                                   api_obj_ctxt_t *obj_ctxt) {
    nexthop *nh;
    sdk_ret_t ret;
    tep_entry *orig_obj;
    pds_obj_key_t nh_key;
    nexthop_impl *nh_impl;
    p4pd_error_t p4pd_ret;
    nexthop_group *nhgroup;
    pds_obj_key_t nh_group_key;
    nexthop_info_entry_t nh_data;
    nexthop_group_impl *nhgroup_impl;
    tunnel2_actiondata_t tep2_data = { 0 };

    // program the TUNNEL2 table entry first
    if (spec->remote_ip.af == IP_AF_IPV4) {
        tep2_data.tunnel2_action.ip_type = IPTYPE_IPV4;
        memcpy(tep2_data.tunnel2_action.dipo, &spec->remote_ip.addr.v4_addr,
               IP4_ADDR8_LEN);
    } else if (spec->remote_ip.af == IP_AF_IPV6) {
        tep2_data.tunnel2_action.ip_type = IPTYPE_IPV6;
        sdk::lib::memrev(tep2_data.tunnel2_action.dipo,
                         spec->remote_ip.addr.v6_addr.addr8,
                         IP6_ADDR8_LEN);
    }
    if (spec->encap.type == PDS_ENCAP_TYPE_MPLSoUDP) {
        tep2_data.tunnel2_action.encap_type = P4_REWRITE_ENCAP_MPLSoUDP;
    } else if (spec->encap.type == PDS_ENCAP_TYPE_VXLAN) {
        tep2_data.tunnel2_action.encap_type = P4_REWRITE_ENCAP_VXLAN;
    }
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_TUNNEL2, hw_id_,
                                       NULL, NULL, &tep2_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program TEP %s in TUNNEL2 table at idx %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    memset(&nh_data, 0, nh_data.entry_size());
    // now we need to update/fix the nexthop(s)
    switch (spec->nh_type) {
    case PDS_NH_TYPE_UNDERLAY_ECMP:
        nhgroup = nexthop_group_db()->find(&spec->nh_group);
        if (unlikely(nhgroup == NULL)) {
            PDS_TRACE_ERR("nhgroup %s in TEP %s not found",
                          spec->nh_group.str(), spec->key.str());
            SDK_ASSERT_RETURN(false, SDK_RET_INVALID_ARG);
        }
        nhgroup_impl = (nexthop_group_impl *)nhgroup->impl();
        for (uint32_t i = 0, nh_idx = nhgroup_impl->nh_base_hw_id();
             i < nhgroup->num_nexthops(); nh_idx++, i++) {
            ret = nh_data.read(nh_idx);
            if (unlikely(ret != SDK_RET_OK)) {
                PDS_TRACE_ERR("Failed to read NEXTHOP table at %u", nh_idx);
                return sdk::SDK_RET_HW_READ_ERR;
            }
            nh_data.set_tunnel2_id(hw_id_);
            nh_data.set_vlan(spec->encap.val.value);
            ret = nh_data.write(nh_idx);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to update NEXTHOP table at %u", nh_idx);
                return sdk::SDK_RET_HW_PROGRAM_ERR;
            }
        }
        break;
    case PDS_NH_TYPE_UNDERLAY:
        nh = (nexthop *)nexthop_db()->find(&spec->nh);
        if (unlikely(nh == NULL)) {
            PDS_TRACE_ERR("nh %s in TEP %s not found",
                          spec->nh.str(), spec->key.str());
            SDK_ASSERT_RETURN(false, SDK_RET_INVALID_ARG);
        }
        nh_impl = (nexthop_impl *)nh->impl();
        ret = nh_data.read(nh_impl->hw_id());
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_TRACE_ERR("Failed to read NEXTHOP table at %u",
                          nh_impl->hw_id());
            return sdk::SDK_RET_HW_READ_ERR;
        }
        nh_data.set_tunnel2_id(hw_id_);
        nh_data.set_vlan(spec->encap.val.value);
        ret = nh_data.write(nh_impl->hw_id());
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to update NEXTHOP table at %u",
                          nh_impl->hw_id());
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
        break;
    case PDS_NH_TYPE_BLACKHOLE:
        // no-np
        break;
    case PDS_NH_TYPE_NONE:
        if (api_op == API_OP_CREATE) {
            // no nexthops to program
        } else {
            // TEP update case
            if (obj_ctxt->upd_bmap & PDS_TEP_UPD_NH_TYPE) {
                // we need to fix up NHs that were used earlier
                if (orig_tep->nh_type() == PDS_NH_TYPE_UNDERLAY_ECMP) {
                    nh_group_key = orig_tep->nh_group();
                    nhgroup = nexthop_group_db()->find(&nh_group_key);
                    nhgroup_impl = (nexthop_group_impl *)nhgroup->impl();
                    for (uint32_t i = 0, nh_idx = nhgroup_impl->nh_base_hw_id();
                         i < nhgroup->num_nexthops(); nh_idx++, i++) {
                        ret = nh_data.read(nh_idx);
                        if (unlikely(ret != SDK_RET_OK)) {
                            PDS_TRACE_ERR("Failed to read NEXTHOP table at %u",
                                          nh_idx);
                            return sdk::SDK_RET_HW_READ_ERR;
                        }
                        nh_data.set_tunnel2_id(0);
                        nh_data.set_vlan(0);
                        ret = nh_data.write(nh_idx);
                        if (ret != SDK_RET_OK) {
                            PDS_TRACE_ERR("Failed to update NEXTHOP table "
                                          "at %u", nh_idx);
                            return sdk::SDK_RET_HW_PROGRAM_ERR;
                        }
                    }
                } else if (orig_tep->nh_type() == PDS_NH_TYPE_UNDERLAY) {
                    nh_key = orig_tep->nh();
                    nh_impl =
                        (nexthop_impl *)nexthop_db()->find(&nh_key)->impl();
                    ret = nh_data.read(nh_impl->hw_id());
                    if (unlikely(ret != SDK_RET_OK)) {
                        PDS_TRACE_ERR("Failed to read NEXTHOP table at %u",
                                      nh_impl->hw_id());
                        return sdk::SDK_RET_HW_READ_ERR;
                    }
                    nh_data.set_tunnel2_id(0);
                    nh_data.set_vlan(0);
                    ret = nh_data.write(nh_impl->hw_id());
                    if (ret != SDK_RET_OK) {
                        PDS_TRACE_ERR("Failed to update NEXTHOP table at %u",
                                      nh_impl->hw_id());
                        return sdk::SDK_RET_HW_PROGRAM_ERR;
                    }
                }
            } else {
                // if nexthop information has not changed, don't override as
                // routing stack would have programmed the right reachability
                // information by now and those nexthops would have been
                // programmed correctly
            }
        }
        break;
    default:
        PDS_TRACE_ERR("Unsupported nh type %u in TEP %s spec",
                      spec->nh_type, spec->key.str());
        SDK_ASSERT_RETURN(false, SDK_RET_INVALID_ARG);
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::activate_create_(pds_epoch_t epoch, tep_entry *tep,
                           pds_tep_spec_t *spec, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    tunnel_actiondata_t tep_data;

    PDS_TRACE_DEBUG("Activating TEP %s create, h/w id %u",
                    spec->key.str(), hw_id_);
    if (tep->type() == PDS_TEP_TYPE_INTER_DC) {
        // program outer tunnel in double encap case
        return program_inter_dc_tunnel_(API_OP_CREATE, tep, NULL,
                                        spec, obj_ctxt);
    }
    // non-INTER_DC tunnel
    memset(&tep_data, 0, sizeof(tep_data));
    return program_tunnel_(API_OP_CREATE, tep, spec, &tep_data, obj_ctxt);
}

sdk_ret_t
tep_impl::activate_delete_tunnel_table_(pds_epoch_t epoch, tep_entry *tep) {
    p4pd_error_t p4pd_ret;
    tunnel_actiondata_t tep_data;

    // 1st read the TEP entry from h/w
    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_TUNNEL, hw_id_,
                                      NULL, NULL, &tep_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read TEP %s at TUNNEL table idx %u",
                      tep->key().str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    // point the tunnel to the blackhole nexthop
    tep_data.tunnel_action.nexthop_base = PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID;
    tep_data.tunnel_action.num_nexthops = 0;
    // update the entry in the TUNNEL table
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_TUNNEL, hw_id_,
                                       NULL, NULL, &tep_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to update TEP %s at TUNNEL table idx %u",
                      tep->key().str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // TUNNEL2 table entry doesn't need to be cleaned up because mirrored
    // traffic will be blackholed because of the cleanup to TUNNEL table entry
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::activate_delete_tunnel2_(pds_epoch_t epoch, tep_entry *tep) {
    sdk_ret_t ret;
    pds_obj_key_t nh_key;
    p4pd_error_t p4pd_ret;
    nexthop_impl *nh_impl;
    nexthop_group *nhgroup;
    pds_obj_key_t nh_group_key;
    nexthop_info_entry_t nh_data;
    nexthop_group_impl *nhgroup_impl;

    memset(&nh_data, 0, nh_data.entry_size());
    // update/fix the nexthop(s) to not point to this outer tunnel anymore
    if (tep->nh_type() == PDS_NH_TYPE_UNDERLAY_ECMP) {
        nh_group_key = tep->nh_group();
        nhgroup = nexthop_group_db()->find(&nh_group_key);
        nhgroup_impl = (nexthop_group_impl *)nhgroup->impl();
        for (uint32_t i = 0, nh_idx = nhgroup_impl->nh_base_hw_id();
             i < nhgroup->num_nexthops(); nh_idx++, i++) {
            ret = nh_data.read(nh_idx);
            if (unlikely(ret != SDK_RET_OK)) {
                PDS_TRACE_ERR("Failed to read NEXTHOP table at %u", nh_idx);
                return sdk::SDK_RET_HW_READ_ERR;
            }
            nh_data.set_tunnel2_id(0);
            nh_data.set_vlan(0);
            ret = nh_data.write(nh_idx);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to update NEXTHOP table at %u", nh_idx);
                return sdk::SDK_RET_HW_PROGRAM_ERR;
            }
        }
    } else if (tep->nh_type() == PDS_NH_TYPE_UNDERLAY) {
        nh_key = tep->nh();
        nh_impl = (nexthop_impl *)nexthop_db()->find(&nh_key)->impl();
        ret = nh_data.read(nh_impl->hw_id());
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_TRACE_ERR("Failed to read NEXTHOP table at %u",
                          nh_impl->hw_id());
            return sdk::SDK_RET_HW_READ_ERR;
        }
        nh_data.set_tunnel2_id(0);
        nh_data.set_vlan(0);
        ret = nh_data.write(nh_impl->hw_id());
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to update NEXTHOP table at %u",
                          nh_impl->hw_id());
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
    }
    // we don't need to touch the TUNNEL2 table entry as no entries are pointing
    // to it now !!
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::activate_delete_(pds_epoch_t epoch, tep_entry *tep) {
    sdk_ret_t ret;

    PDS_TRACE_DEBUG("Activating TEP %s delete, h/w id %u",
                    tep->key().str(), hw_id_);
    if (tep->type() == PDS_TEP_TYPE_INTER_DC) {
        // cleanup outer tunnel in double encap case
        ret = activate_delete_tunnel2_(epoch, tep);
    } else {
        ret = activate_delete_tunnel_table_(epoch, tep);
    }
    return ret;
}

sdk_ret_t
tep_impl::activate_update_(pds_epoch_t epoch, tep_entry *tep,
                           tep_entry *orig_tep, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_tep_spec_t *spec;
    tunnel_actiondata_t tep_data;

    spec = &obj_ctxt->api_params->tep_spec;
    PDS_TRACE_DEBUG("Activating TEP %s update, h/w id1 %u",
                    tep->key().str(), hw_id_);
    if (tep->type() == PDS_TEP_TYPE_INTER_DC) {
        // update outer tunnel in double encap case
        return program_inter_dc_tunnel_(API_OP_UPDATE, tep, orig_tep,
                                        spec, obj_ctxt);
    }
    // non-INTER_DC tunnel
    return program_tunnel_(API_OP_UPDATE, tep, spec, &tep_data, obj_ctxt);
}

sdk_ret_t
tep_impl::activate_hw(api_base *api_obj, api_base *orig_obj, pds_epoch_t epoch,
                      api_op_t api_op, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_tep_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->tep_spec;
        ret = activate_create_(epoch, (tep_entry *)api_obj, spec, obj_ctxt);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (tep_entry *)api_obj);
        break;

    case API_OP_UPDATE:
        ret = activate_update_(epoch, (tep_entry *)api_obj,
                               (tep_entry *)orig_obj, obj_ctxt);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

void
tep_impl::fill_status_(pds_tep_status_t *status) {
    status->hw_id = hw_id_;
}

sdk_ret_t
tep_impl::fill_spec_(pds_tep_spec_t *spec) {
    p4pd_error_t p4pdret;
    tunnel_actiondata_t tep_data;

    p4pdret = p4pd_global_entry_read(P4TBL_ID_TUNNEL, hw_id_,
                                     NULL, NULL, &tep_data);
    if (unlikely(p4pdret != P4PD_SUCCESS)) {
        PDS_TRACE_ERR("Failed to read TUNNEL table at idx %u", hw_id_);
        return sdk::SDK_RET_HW_READ_ERR;
    }
    switch (tep_data.tunnel_action.ip_type) {
    case IPTYPE_IPV4:
        spec->remote_ip.af = IP_AF_IPV4;
        memcpy(&spec->remote_ip.addr.v4_addr, tep_data.tunnel_action.dipo,
               IP4_ADDR8_LEN);
        break;
    case IPTYPE_IPV6:
        spec->remote_ip.af = IP_AF_IPV6;
        sdk::lib::memrev(spec->remote_ip.addr.v6_addr.addr8,
                         tep_data.tunnel_action.dipo,
                         IP6_ADDR8_LEN);
        break;
    default:
        break;
    }
    sdk::lib::memrev(spec->mac, tep_data.tunnel_action.dmaci, ETH_ADDR_LEN);
    if (tep_data.tunnel_action.num_nexthops > PDS_NUM_NH_NO_ECMP) {
        spec->nh_type = PDS_NH_TYPE_UNDERLAY_ECMP;
    } else if (tep_data.tunnel_action.num_nexthops == PDS_NUM_NH_NO_ECMP) {
        spec->nh_type = PDS_NH_TYPE_UNDERLAY;
    }
    if (tep_data.tunnel_action.vni) {
        spec->encap.type = PDS_ENCAP_TYPE_VXLAN;
        spec->encap.val.value = tep_data.tunnel_action.vni;
    }
    if (tep_data.tunnel_action.tos_override) {
        spec->tos = tep_data.tunnel_action.tos;
    }
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    sdk_ret_t rv;
    pds_tep_info_t *tep_info = (pds_tep_info_t *)info;

    rv = fill_spec_(&tep_info->spec);
    if (unlikely(rv != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to read from h/w for TEP %s",
                      api_obj->key2str().c_str());
        return rv;
    }

    fill_status_(&tep_info->status);
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::backup(obj_info_t *info, upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    pds_tep_info_t *tep_info;
    upg_obj_tlv_t *tlv;
    pds::TunnelGetResponse proto_msg;

    tlv = (upg_obj_tlv_t *)upg_info->mem;
    tep_info = (pds_tep_info_t *)info;

    ret = fill_spec_(&tep_info->spec);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    fill_status_(&tep_info->status);
    // convert api info to proto
    pds_tep_api_info_to_proto(tep_info, (void *)&proto_msg);
    ret = pds_svc_serialize_proto_msg(upg_info, tlv, &proto_msg);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to serialize tep %s err %u",
                      tep_info->spec.key.str(), ret);
    }
    return ret;
}

sdk_ret_t
tep_impl::restore_resources(obj_info_t *info) {
    sdk_ret_t ret;
    pds_tep_info_t *tep_info;
    pds_tep_spec_t *spec;
    pds_tep_status_t *status;

    tep_info = (pds_tep_info_t *)info;
    spec = &tep_info->spec;
    status = &tep_info->status;

    // restore the TUNNEL & TUNNEL2 table resources
    ret = tep_impl_db()->tunnel_idxr()->alloc(status->hw_id);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore entry %u in TUNNEL table, err %u",
                      status->hw_id, ret);
        return ret;
    }
    hw_id_ = status->hw_id;
    return SDK_RET_OK;
}

sdk_ret_t
tep_impl::restore(obj_info_t *info, upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    pds::TunnelGetResponse proto_msg;
    pds_tep_info_t *tep_info;
    upg_obj_tlv_t *tlv;
    uint32_t obj_size, meta_size;

    tlv = (upg_obj_tlv_t *)upg_info->mem;
    tep_info = (pds_tep_info_t *)info;
    obj_size = tlv->len;
    meta_size = sizeof(upg_obj_tlv_t);
    // fill up the size, even if it fails later. to try and restore next obj
    upg_info->size = obj_size + meta_size;
    // de-serialize proto msg
    if (proto_msg.ParseFromArray(tlv->obj, tlv->len) == false) {
        PDS_TRACE_ERR("Failed to de-serialize tep");
        return SDK_RET_OOM;
    }
    // convert proto msg to tep info
    ret = pds_tep_proto_to_api_info(tep_info, &proto_msg);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to convert tep proto msg to info, err %u", ret);
        return ret;
    }
    // now restore hw resources
    ret = restore_resources((obj_info_t *)tep_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore hw resources for tep %s err %u",
                      tep_info->spec.key.str(), ret);
    }
    return ret;
}

/// \@}

}    // namespace impl
}    // namespace api
