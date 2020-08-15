//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of mirror APIs
///
//----------------------------------------------------------------------------

#include "nic/sdk/platform/devapi/devapi_types.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/infra/core/mem.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/internal/pds_route.hpp"
#include "nic/apollo/api/impl/apulu/tep_impl.hpp"
#include "nic/apollo/api/impl/apulu/mapping_impl.hpp"
#include "nic/apollo/api/impl/apulu/mirror_impl.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_impl.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_group_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/if_impl.hpp"
#include "gen/p4gen/p4/include/ftl.h"

namespace api {
namespace impl {

/// \defgroup PDS_MIRROR_IMPL - mirror entry datapath implementation
/// \ingroup PDS_MIRROR
/// \@{

mirror_impl *
mirror_impl::factory(pds_mirror_session_spec_t *spec) {
    mirror_impl *impl;

    impl = mirror_impl_db()->alloc();
    new (impl) mirror_impl();
    return impl;
}

impl_base *
mirror_impl::clone(void) {
    mirror_impl *cloned_impl;

    cloned_impl = mirror_impl_db()->alloc();
    new (cloned_impl) mirror_impl();
    // deep copy is not needed as we don't store pointers
    *cloned_impl = *this;
    return cloned_impl;
}

void
mirror_impl::destroy(mirror_impl *impl) {
    impl->~mirror_impl();
    mirror_impl_db()->free(impl);
}

sdk_ret_t
mirror_impl::free(mirror_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                               api_obj_ctxt_t *obj_ctxt) {
    pds_mirror_session_spec_t *spec;

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        // allocate hw id for this session
        spec = &obj_ctxt->api_params->mirror_session_spec;
        return mirror_impl_db()->alloc_hw_id(&spec->key, &hw_id_);
    case API_OP_UPDATE:
        // we will use the same h/w resources as the original object
    default:
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::release_resources(api_base *api_obj) {
    if (hw_id_ != 0xFFFF) {
        return mirror_impl_db()->free_hw_id(hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::nuke_resources(api_base *api_obj) {
    if (hw_id_ != 0xFFFF) {
        return mirror_impl_db()->free_hw_id(hw_id_);
    }
    return SDK_RET_OK;
}

#define nexthop_info         action_u.nexthop_nexthop_info
#define rspan_action         action_u.mirror_rspan
#define erspan_action        action_u.mirror_erspan
#define lspan_action         action_u.mirror_lspan
sdk_ret_t
mirror_impl::program_lif_rspan_(lif_impl *lif,
                                pds_mirror_session_spec_t *spec) {
    p4pd_error_t p4pd_ret;
    nexthop_info_entry_t nh_data;
    mirror_actiondata_t mirror_data = { 0 };

    mirror_data.action_id = MIRROR_LSPAN_ID;
    mirror_data.lspan_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    switch (lif->type()) {
    case sdk::platform::LIF_TYPE_HOST:
    case sdk::platform::LIF_TYPE_MNIC_OOB_MGMT:
    case sdk::platform::LIF_TYPE_MNIC_INBAND_MGMT:
        mirror_data.lspan_action.nexthop_id = lif->nh_idx();
        mirror_data.lspan_action.truncate_len = spec->snap_len;
        break;
    default:
        // blackhole if some other type of lif is provided
        PDS_TRACE_ERR("Unsupported lif type %u in mirror session %s, "
                      "blackholing mirrored traffic",
                      lif->type(), spec->key.str());
        mirror_data.lspan_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
        mirror_data.lspan_action.nexthop_id =
            PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID;
        break;
    }
    // program the mirror table
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_MIRROR, hw_id_, NULL, NULL,
                                       &mirror_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program mirror session %s at idx %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::program_uplink_rspan_(if_entry *intf,
                                   pds_mirror_session_spec_t *spec) {
    sdk_ret_t ret;
    uint32_t oport;
    p4pd_error_t p4pd_ret;
    nexthop_info_entry_t nh_data;
    mirror_actiondata_t mirror_data = { 0 };

    intf = if_entry::eth_if(intf);
    mirror_data.action_id = MIRROR_RSPAN_ID;
    mirror_data.rspan_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    oport = if_impl::port(intf);
    SDK_ASSERT(oport != PDS_PORT_INVALID);
    // use pre-reserved per uplink nexthop
    mirror_data.rspan_action.nexthop_id = oport + 1;
    mirror_data.rspan_action.ctag = spec->rspan_spec.encap.val.vlan_tag;
    mirror_data.rspan_action.truncate_len = spec->snap_len;
    // program the nexthop entry 1st
    memset(&nh_data, 0, nh_data.entry_size());
    nh_data.set_port(oport);
    nh_data.set_vlan(spec->rspan_spec.encap.val.vlan_tag);
    ret = nh_data.write(mirror_data.rspan_action.nexthop_id);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program NEXTHOP table at idx %u, "
                      "RSPAN mirror session %s programming failed",
                      oport, spec->key.str());
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // program the mirror table
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_MIRROR, hw_id_, NULL, NULL,
                                       &mirror_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program mirror session %s at idx %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::program_rspan_(pds_epoch_t epoch,
                            pds_mirror_session_spec_t *spec) {
    lif_impl *lif;
    if_entry *intf;
    if_index_t if_index;
    pds_obj_key_t lif_key;

    intf = if_find(&spec->rspan_spec.interface);
    if (intf) {
        if ((intf->type() == IF_TYPE_UPLINK) || (intf->type() == IF_TYPE_ETH)) {
            return program_uplink_rspan_(intf, spec);
        } else if (intf->type() == IF_TYPE_HOST) {
            // local span case, where destination interface is host interface
            if_index = LIF_IFINDEX(HOST_IFINDEX_TO_IF_ID(intf->ifindex()));
            lif_key = uuid_from_objid(if_index);
            lif = (lif_impl *)lif_impl_db()->find(&lif_key);
            if (!lif) {
                 PDS_TRACE_ERR("Failed to program rspan session %s, lif %s not "
                               "found", spec->key.str(), lif_key.str());
                 return SDK_RET_ENTRY_NOT_FOUND;
            }
            return program_lif_rspan_(lif, spec);
        }
    } else {
        // local span case, get the lif and use its nexthop id
        lif = lif_impl_db()->find(&spec->rspan_spec.interface);
        return program_lif_rspan_(lif, spec);
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::program_overlay_erspan_(pds_epoch_t epoch,
                                     pds_mirror_session_spec_t *spec) {
    sdk_ret_t ret;
    subnet_entry *subnet;
    p4pd_error_t p4pd_ret;
    mapping_entry *mapping;
    pds_mapping_key_t mapping_key;
    mapping_impl *mapping_impl_obj;
    mirror_actiondata_t mirror_data = { 0 };

    SDK_ASSERT(spec->erspan_spec.dst_type == PDS_ERSPAN_DST_TYPE_IP);
    // mirror destination is either local or remote mapping
    mapping_key.type = PDS_MAPPING_TYPE_L3;
    mapping_key.vpc = spec->erspan_spec.vpc;
    mapping_key.ip_addr = spec->erspan_spec.ip_addr;
    mapping = mapping_entry::build(&mapping_key);
    if (mapping == NULL) {
        PDS_TRACE_ERR("Failed to find ERSPAN mirror session %s dst IP "
                      "mapping (%s, %s)", spec->key.str(),
                      spec->erspan_spec.vpc.str(),
                      ipaddr2str(&spec->erspan_spec.ip_addr));
        return SDK_RET_INVALID_ARG;
    }
    subnet = ((mapping_impl *)(mapping->impl()))->subnet();
    mapping_impl_obj = (mapping_impl *)mapping->impl();
    subnet = mapping_impl_obj->subnet();

    mirror_data.action_id = MIRROR_ERSPAN_ID;
    // forwarding mirror packet takes same forwarding path as any
    // other traffic to this mappping
    mirror_data.erspan_action.nexthop_type =
        mapping_impl_obj->nexthop_type();
    mirror_data.erspan_action.nexthop_id =
        mapping_impl_obj->nexthop_id();
    mirror_data.erspan_action.egress_bd_id =
        mapping_impl_obj->subnet_hw_id();
    // ERSPAN Eth header SMAC = VR IP
    sdk::lib::memrev(mirror_data.erspan_action.smac,
                     &subnet->vr_mac()[0], ETH_ADDR_LEN);
    // ERSPAN Eth header DMAC = mapping's DMAC
    sdk::lib::memrev(mirror_data.erspan_action.dmac,
                     &mapping_impl_obj->mac()[0], ETH_ADDR_LEN);
    // ERSPAN IP header SIP is IPv4 VR IP of mapping's subnet
    mirror_data.erspan_action.sip = subnet->v4_vr_ip();
    // ERSPAN IP header DIP is the collector IP
    mirror_data.erspan_action.dip =
        spec->erspan_spec.ip_addr.addr.v4_addr;
    if (mapping->is_local()) {
        // local vnic
        mirror_data.erspan_action.rewrite_flags = P4_SET_REWRITE(ENCAP, VXLAN);
    } else {
        // remote vnic
        mirror_data.erspan_action.rewrite_flags =
            P4_SET_REWRITE(ENCAP, VXLAN) | P4_SET_REWRITE(VNI, DEFAULT);
    }
    mapping_entry::soft_delete(mapping);
    mirror_data.erspan_action.erspan_type = spec->erspan_spec.type;
    mirror_data.erspan_action.gre_seq_en = TRUE;
    mirror_data.erspan_action.vlan_strip_en =
        spec->erspan_spec.vlan_strip_en;
    mirror_data.erspan_action.truncate_len = spec->snap_len;
    mirror_data.erspan_action.span_id = spec->erspan_spec.span_id;
    mirror_data.erspan_action.dscp = spec->erspan_spec.dscp;

    // program the mirror table
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_MIRROR, hw_id_, NULL, NULL,
                                       &mirror_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program mirror session %s at idx %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::program_underlay_erspan_(pds_epoch_t epoch,
                                      pds_mirror_session_spec_t *spec) {
    nexthop *nh;
    sdk_ret_t ret;
    tep_entry *tep;
    ip_addr_t mytep_ip;
    pds_obj_key_t nh_key;
    pds_nh_type_t nh_type;
    p4pd_error_t p4pd_ret;
    nexthop_group *nhgroup;
    mirror_actiondata_t mirror_data = { 0 };

    mirror_data.action_id = MIRROR_ERSPAN_ID;
    mytep_ip = device_db()->find()->ip_addr();
    mirror_data.erspan_action.sip = mytep_ip.addr.v4_addr;
    if (spec->erspan_spec.dst_type == PDS_ERSPAN_DST_TYPE_TEP) {
        tep = tep_find(&spec->erspan_spec.tep);
        mirror_data.erspan_action.nexthop_type = NEXTHOP_TYPE_TUNNEL;
        mirror_data.erspan_action.nexthop_id =
            ((tep_impl *)(tep->impl()))->hw_id();
        mirror_data.erspan_action.dip = tep->ip().addr.v4_addr;
        // DMAC and SMAC both are picked from the NH this TEP resolves to
        mirror_data.erspan_action.rewrite_flags =
            P4_SET_REWRITE(DMAC, FROM_NEXTHOP) |
            P4_SET_REWRITE(SMAC, FROM_NEXTHOP);
    } else {
        SDK_ASSERT(spec->erspan_spec.dst_type == PDS_ERSPAN_DST_TYPE_IP);
        // TODO: should we lookup for a TEP in this case first before
        //       assuming it is a non-TEP IP ?
        // destination is IP in the underlay, set the nexthop to point
        // to system drop nexthop until we hear about its reachability
        // from the routing stack
        mirror_data.erspan_action.dip = spec->erspan_spec.ip_addr.addr.v4_addr;
        // consult the underlay route db to figure out the nexthop for this
        if (pds_underlay_nexthop(spec->erspan_spec.ip_addr.addr.v4_addr,
                                 &nh_type, &nh_key) == SDK_RET_OK) {
            if (nh_type == PDS_NH_TYPE_UNDERLAY) {
                nh = nexthop_db()->find(&nh_key);
                if (unlikely(nh == NULL)) {
                    PDS_TRACE_ERR("Nexthop %s for ERSPAN collector IP %s in "
                                  "mirror session %s not found", nh_key.str(),
                                  ipaddr2str(&spec->erspan_spec.ip_addr),
                                  spec->key.str());
                    return SDK_RET_INVALID_ARG;
                }
                mirror_data.erspan_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
                mirror_data.erspan_action.nexthop_id =
                    ((nexthop_impl *)nh->impl())->hw_id();
            } else if (nh_type == PDS_NH_TYPE_UNDERLAY_ECMP) {
                nhgroup = nexthop_group_db()->find(&nh_key);
                if (unlikely(nhgroup == NULL)) {
                    PDS_TRACE_ERR("nhgroup %s for ERSPAN collector IP %s in "
                                  "mirror session %s not found",
                                  nh_key.str(),
                                  ipaddr2str(&spec->erspan_spec.ip_addr),
                                  spec->key.str());
                    return SDK_RET_INVALID_ARG;
                }
                mirror_data.erspan_action.nexthop_type = NEXTHOP_TYPE_ECMP;
                mirror_data.erspan_action.nexthop_id =
                    ((nexthop_group_impl *)nhgroup->impl())->hw_id();
            }
        } else {
            // ERSPAN collector IP reachability is unknown, use system drop
            // nexthop
            PDS_TRACE_INFO("ERSPAN collector IP %s in mirror session %s "
                           "reachability unknown, using black hole nexthop",
                           ipaddr2str(&spec->erspan_spec.ip_addr),
                           spec->key.str());
            mirror_data.erspan_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
            mirror_data.erspan_action.nexthop_id =
                PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID;
        }
    }
    mirror_data.erspan_action.erspan_type = spec->erspan_spec.type;
    mirror_data.erspan_action.gre_seq_en = TRUE;
    mirror_data.erspan_action.vlan_strip_en =
        spec->erspan_spec.vlan_strip_en;
    mirror_data.erspan_action.truncate_len = spec->snap_len;
    mirror_data.erspan_action.span_id = spec->erspan_spec.span_id;
    mirror_data.erspan_action.dscp = spec->erspan_spec.dscp;

    // program the mirror table
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_MIRROR, hw_id_, NULL, NULL,
                                       &mirror_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program mirror session %s at idx %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::activate_create_(pds_epoch_t epoch, mirror_session *ms,
                              pds_mirror_session_spec_t *spec) {
    vpc_entry *vpc;

    switch (spec->type) {
    case PDS_MIRROR_SESSION_TYPE_RSPAN:
        return program_rspan_(epoch, spec);
        break;

    case PDS_MIRROR_SESSION_TYPE_ERSPAN:
        vpc = vpc_find(&spec->erspan_spec.vpc);
        if (vpc->type() == PDS_VPC_TYPE_UNDERLAY) {
            return program_underlay_erspan_(epoch, spec);
        } else {
            return program_overlay_erspan_(epoch, spec);
        }
        break;
    default:
        break;
    }
    return SDK_RET_INVALID_ARG;
}

sdk_ret_t
mirror_impl::activate_update_(pds_epoch_t epoch, mirror_session *ms,
                              api_obj_ctxt_t *obj_ctxt) {
    // update reprograms the whole mirror session, so it is identical to create
    return activate_create_(epoch, ms,
                            &obj_ctxt->api_params->mirror_session_spec);
}

sdk_ret_t
mirror_impl::activate_delete_(pds_epoch_t epoch, mirror_session *ms) {
    p4pd_error_t p4pd_ret;
    mirror_actiondata_t mirror_data = { 0 };

    // cleanup the mirror table entry
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_MIRROR, hw_id_, NULL, NULL,
                                       &mirror_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program mirror session %s at idx %u",
                      ms->key2str().c_str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::activate_hw(api_base *api_obj, api_base *orig_obj,
                         pds_epoch_t epoch, api_op_t api_op,
                         api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_mirror_session_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->mirror_session_spec;
        ret = activate_create_(epoch, (mirror_session *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (mirror_session *)api_obj);
        break;

    case API_OP_UPDATE:
        ret = activate_update_(epoch, (mirror_session *)api_obj, obj_ctxt);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
mirror_impl::fill_spec_(pds_mirror_session_spec_t *spec,
                        mirror_actiondata_t *mirror_data) {
    switch (mirror_data->action_id) {
    case MIRROR_RSPAN_ID:
        spec->snap_len = mirror_data->rspan_action.truncate_len;
        break;
    case MIRROR_ERSPAN_ID:
        spec->snap_len = mirror_data->erspan_action.truncate_len;
        spec->erspan_spec.dscp = mirror_data->erspan_action.dscp;
        spec->erspan_spec.span_id = mirror_data->erspan_action.span_id;
        spec->erspan_spec.vlan_strip_en =
            mirror_data->erspan_action.vlan_strip_en;
        break;
    case MIRROR_LSPAN_ID:
        spec->snap_len = mirror_data->lspan_action.truncate_len;
        break;
    default:
        return sdk::SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

void
mirror_impl::fill_status_(pds_mirror_session_status_t *status) {
    status->hw_id = hw_id_;
}

sdk_ret_t
mirror_impl::fill_stats_(pds_mirror_session_stats_t *stats,
                         mirror_actiondata_t *mirror_data) {
    switch (mirror_data->action_id) {
    case MIRROR_RSPAN_ID:
    case MIRROR_LSPAN_ID:
        // we support stats only for ERSPAN
        break;
    case MIRROR_ERSPAN_ID:
        stats->packet_count = *(uint64_t *)mirror_data->erspan_action.npkts;
        stats->byte_count = *(uint64_t *)mirror_data->erspan_action.nbytes;
        break;
    default:
        return sdk::SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

sdk_ret_t
mirror_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    sdk_ret_t ret;
    p4pd_error_t p4pd_ret;
    mirror_actiondata_t mirror_data;
    pds_mirror_session_info_t *mirror_session_info =
        (pds_mirror_session_info_t *)info;

    // read the MIRROR table entry for this session
    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_MIRROR, hw_id_, NULL, NULL,
                                      &mirror_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read MIRROR table idx %u for mirror "
                      "session %s", hw_id_, api_obj->key2str().c_str());
        return sdk::SDK_RET_HW_READ_ERR;
    }

    ret = fill_spec_(&mirror_session_info->spec, &mirror_data);
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to fill config spec for mirror session %s",
                      api_obj->key2str().c_str());
        return ret;
    }
    fill_status_(&mirror_session_info->status);
    ret = fill_stats_(&mirror_session_info->stats, &mirror_data);
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to fill statistics for mirror session %s",
                      api_obj->key2str().c_str());
        return ret;
    }
    return SDK_RET_OK;

}

/// \@}

}    // namespace impl
}    // namespace api
