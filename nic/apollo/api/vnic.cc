//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vnic entry handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/msg.h"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/api/vnic.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/internal/ipc.hpp"

namespace api {

/// \defgroup PDS_VNIC_ENTRY - vnic functionality
/// \ingroup PDS_VNIC
/// @{

vnic_entry::vnic_entry() {
    switch_vnic_ = false;
    binding_checks_en_ = false;
    host_if_ = k_pds_obj_key_invalid;
    meter_en_ = false;
    primary_ = false;
    ht_ctxt_.reset();
    impl_ = NULL;
}

vnic_entry *
vnic_entry::factory(pds_vnic_spec_t *spec) {
    vnic_entry *vnic;

    // create vnic entry with defaults, if any
    vnic = vnic_db()->alloc();
    if (vnic) {
        new (vnic) vnic_entry();
        vnic->impl_ = impl_base::factory(impl::IMPL_OBJ_ID_VNIC, spec);
        if (vnic->impl_ == NULL) {
            vnic_entry::destroy(vnic);
            return NULL;
        }
    }
    return vnic;
}

vnic_entry::~vnic_entry() {
}

void
vnic_entry::destroy(vnic_entry *vnic) {
    vnic->nuke_resources_();
    if (vnic->impl_) {
        impl_base::destroy(impl::IMPL_OBJ_ID_VNIC, vnic->impl_);
    }
    vnic->~vnic_entry();
    vnic_db()->free(vnic);
}

api_base *
vnic_entry::clone(api_ctxt_t *api_ctxt) {
    vnic_entry *cloned_vnic;

    cloned_vnic = vnic_db()->alloc();
    if (cloned_vnic) {
        new (cloned_vnic) vnic_entry();
        if (cloned_vnic->init_config(api_ctxt) != SDK_RET_OK) {
            goto error;
        }
        cloned_vnic->impl_ = impl_->clone();
        if (unlikely(cloned_vnic->impl_ == NULL)) {
            PDS_TRACE_ERR("Failed to clone vnic %s impl", key_.str());
            goto error;
        }
    }
    return cloned_vnic;

error:

    cloned_vnic->~vnic_entry();
    vnic_db()->free(cloned_vnic);
    return NULL;
}

sdk_ret_t
vnic_entry::free(vnic_entry *vnic) {
    if (vnic->impl_) {
        impl_base::free(impl::IMPL_OBJ_ID_VNIC, vnic->impl_);
    }
    vnic->~vnic_entry();
    vnic_db()->free(vnic);
    return SDK_RET_OK;
}

sdk_ret_t
vnic_entry::reserve_resources(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    return impl_->reserve_resources(this, orig_obj, obj_ctxt);
}

sdk_ret_t
vnic_entry::release_resources(void) {
    return impl_->release_resources(this);
}

sdk_ret_t
vnic_entry::nuke_resources_(void) {
    return impl_->nuke_resources(this);
}

sdk_ret_t
vnic_entry::init_config(api_ctxt_t *api_ctxt) {
    pds_vnic_spec_t *spec = &api_ctxt->api_params->vnic_spec;

    memcpy(&key_, &spec->key, sizeof(pds_obj_key_t));
    memcpy(&hostname_, &spec->hostname, sizeof(hostname_));
    subnet_ = spec->subnet;
    v4_meter_ = spec->v4_meter;
    v6_meter_ = spec->v6_meter;
    rx_policer_ = spec->rx_policer;
    tx_policer_ = spec->tx_policer;
    vnic_encap_ = spec->vnic_encap;
    if (unlikely((vnic_encap_.type != PDS_ENCAP_TYPE_NONE) &&
                 (vnic_encap_.type != PDS_ENCAP_TYPE_DOT1Q))) {
        PDS_TRACE_ERR("Invalid encap type %u on vnic %s",
                      vnic_encap_.type, key_.str());
        return SDK_RET_INVALID_ARG;

    }
    fabric_encap_ = spec->fabric_encap;
    switch_vnic_ = spec->switch_vnic;
    binding_checks_en_ = spec->binding_checks_en;
    if (switch_vnic_) {
        // switch vnics can send/receive traffic multiple SIPs/SMACs
        if (unlikely(spec->binding_checks_en)) {
            PDS_TRACE_ERR("switch vnics can't have IP/MAC binding checks "
                          "enabled, vnic %s api op %u failed", key_.str(),
                          api_ctxt->api_op);
            return SDK_RET_INVALID_ARG;
        }
        // switch vnics can send/receive traffic on multiple vlans
        if (unlikely(vnic_encap_.type != PDS_ENCAP_TYPE_NONE)) {
            PDS_TRACE_ERR("switch vnic can'thave VLAN encap, vnic %s api op %u "
                          "failed", key_.str(), api_ctxt->api_op);
            return SDK_RET_INVALID_ARG;
        }
    }
    num_ing_v4_policy_ = spec->num_ing_v4_policy;
    for (uint8_t i = 0; i < num_ing_v4_policy_; i++) {
        ing_v4_policy_[i] = spec->ing_v4_policy[i];
    }
    num_ing_v6_policy_ = spec->num_ing_v6_policy;
    for (uint8_t i = 0; i < num_ing_v6_policy_; i++) {
        ing_v6_policy_[i] = spec->ing_v6_policy[i];
    }
    num_egr_v4_policy_ = spec->num_egr_v4_policy;
    for (uint8_t i = 0; i < num_egr_v4_policy_; i++) {
        egr_v4_policy_[i] = spec->egr_v4_policy[i];
    }
    num_egr_v6_policy_ = spec->num_egr_v6_policy;
    for (uint8_t i = 0; i < num_egr_v6_policy_; i++) {
        egr_v6_policy_[i] = spec->egr_v6_policy[i];
    }
    num_tx_mirror_session_ = spec->num_tx_mirror_session;
    for (uint8_t i = 0; i < num_tx_mirror_session_; i++) {
        tx_mirror_session_[i] = spec->tx_mirror_session[i];
    }
    num_rx_mirror_session_ = spec->num_rx_mirror_session;
    for (uint8_t i = 0; i < num_rx_mirror_session_; i++) {
        rx_mirror_session_[i] = spec->rx_mirror_session[i];
    }
    if (is_mac_set(spec->mac_addr)) {
        memcpy(mac_, spec->mac_addr, ETH_ADDR_LEN);
    }
    host_if_ = spec->host_if;
    if (host_if_ != k_pds_obj_key_invalid) {
        if (unlikely(lif_db()->find(&host_if_) == NULL)) {
            PDS_TRACE_ERR("host if %s not found, vnic %s init failed",
                          spec->host_if.str(), spec->key.str());
            return SDK_RET_INVALID_ARG;
        }
    }
    primary_ = spec->primary;
    meter_en_ = spec->meter_en;
    return SDK_RET_OK;
}

sdk_ret_t
vnic_entry::populate_msg(pds_msg_t *msg, api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.op = obj_ctxt->api_op;
    msg->cfg_msg.obj_id = OBJ_ID_VNIC;
    if (obj_ctxt->api_op == API_OP_DELETE) {
        msg->cfg_msg.vnic.key = obj_ctxt->api_params->key;
    } else {
        msg->cfg_msg.vnic.spec = obj_ctxt->api_params->vnic_spec;
        impl_->populate_msg(msg, this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_entry::program_create(api_obj_ctxt_t *obj_ctxt) {
    pds_vnic_spec_t *spec = &obj_ctxt->api_params->vnic_spec;

    PDS_TRACE_DEBUG("Programming vnic %s, subnet %s, mac %s\n"
                    "vnic encap %s, fabric encap %s, binding checks %s, "
                    "host if %s", key_.str(), spec->subnet.str(),
                    macaddr2str(spec->mac_addr),
                    pds_encap2str(&spec->vnic_encap),
                    pds_encap2str(&spec->fabric_encap),
                    spec->binding_checks_en ? "true" : "false",
                    spec->host_if.str());
    return impl_->program_hw(this, obj_ctxt);
}

sdk_ret_t
vnic_entry::cleanup_config(api_obj_ctxt_t *obj_ctxt) {
    return impl_->cleanup_hw(this, obj_ctxt);
}

sdk_ret_t
vnic_entry::compute_update(api_obj_ctxt_t *obj_ctxt) {
    pds_vnic_spec_t *spec = &obj_ctxt->api_params->vnic_spec;

    if (subnet_ != spec->subnet) {
        PDS_TRACE_ERR("Attempt to modify immutable attr \"subnet\" "
                      "from %s to %s on vnic %s", subnet_.str(),
                      spec->subnet.str(), key_.str());
        return SDK_RET_INVALID_ARG;
    }
    if (memcmp(mac_, spec->mac_addr, ETH_ADDR_LEN)) {
        PDS_TRACE_ERR("Attempt to modify immutable attr \"mac address\" "
                      "from %s to %s on vnic %s", macaddr2str(mac_),
                      macaddr2str(spec->mac_addr), key_.str());
        return SDK_RET_INVALID_ARG;
    }
    if (primary_ != spec->primary) {
        PDS_TRACE_ERR("Attempt to modify immutable attr \"primary\" "
                      "from %u to %u on vnic %s", primary_, spec->primary,
                      key_.str());
        return SDK_RET_INVALID_ARG;
    }
    if ((vnic_encap_.type != spec->vnic_encap.type) ||
        (vnic_encap_.val.value != spec->vnic_encap.val.value)) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_VNIC_ENCAP;
        PDS_TRACE_ERR("Attempt to modify immutable attr \"encap\" "
                      "from %s to %s on vnic %s", pds_encap2str(&vnic_encap_),
                      pds_encap2str(&spec->vnic_encap), key_.str());
        return SDK_RET_INVALID_ARG;
    }
    if (switch_vnic_ != spec->switch_vnic) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_SWITCH_VNIC;
    }
    if (binding_checks_en_ != spec->binding_checks_en) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_BINDING_CHECKS;
    }
    if ((num_ing_v4_policy_ != spec->num_ing_v4_policy)          ||
        (num_ing_v6_policy_ != spec->num_ing_v6_policy)          ||
        (num_egr_v4_policy_ != spec->num_egr_v4_policy)          ||
        (num_egr_v6_policy_ != spec->num_egr_v6_policy)          ||
        (memcmp(ing_v4_policy_, spec->ing_v4_policy,
                num_ing_v4_policy_ * sizeof(ing_v4_policy_[0]))) ||
        (memcmp(ing_v6_policy_, spec->ing_v6_policy,
                num_ing_v6_policy_ * sizeof(ing_v6_policy_[0]))) ||
        (memcmp(egr_v4_policy_, spec->egr_v4_policy,
                num_egr_v4_policy_ * sizeof(egr_v4_policy_[0]))) ||
        (memcmp(egr_v6_policy_, spec->egr_v6_policy,
                num_egr_v6_policy_ * sizeof(egr_v6_policy_[0])))) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_POLICY;
    }
    if ((num_tx_mirror_session_ != spec->num_tx_mirror_session) ||
        memcmp(tx_mirror_session_, spec->tx_mirror_session,
               num_tx_mirror_session_ * sizeof(tx_mirror_session_[0]))) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_TX_MIRROR_SESSION;
    }
    if ((num_rx_mirror_session_ != spec->num_rx_mirror_session) ||
        memcmp(rx_mirror_session_, spec->rx_mirror_session,
               num_rx_mirror_session_ * sizeof(rx_mirror_session_[0]))) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_RX_MIRROR_SESSION;
    }
    if (host_if_ != spec->host_if) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_HOST_IFINDEX;
    }
    if (tx_policer_ != spec->tx_policer) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_TX_POLICER;
    }
    if (rx_policer_ != spec->rx_policer) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_RX_POLICER;
    }
    if (meter_en_ != spec->meter_en) {
        obj_ctxt->upd_bmap |= PDS_VNIC_UPD_METER_EN;
    }
    PDS_TRACE_DEBUG("vnic %s upd bmap 0x%lx", key_.str(), obj_ctxt->upd_bmap);
    return SDK_RET_OK;
}

sdk_ret_t
vnic_entry::program_update(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Updating vnic %s", key_.str());
    return impl_->update_hw(orig_obj, this, obj_ctxt);
}

sdk_ret_t
vnic_entry::activate_config(pds_epoch_t epoch, api_op_t api_op,
                            api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Activating vnic %s config", key_.str());
    return impl_->activate_hw(this, orig_obj, epoch, api_op, obj_ctxt);
}

sdk_ret_t
vnic_entry::reprogram_config(api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Reprogramming vnic %s, subnet %s, fabric encap %s, "
                    "upd bmap 0x%lx", key_.str(), subnet_.str(),
                    pds_encap2str(&fabric_encap_), obj_ctxt->upd_bmap);
    return impl_->reprogram_hw(this, obj_ctxt);
}

sdk_ret_t
vnic_entry::reactivate_config(pds_epoch_t epoch, api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Reactivating vnic %s, subnet %s, fabric encap %s",
                    key_.str(), subnet_.str(), pds_encap2str(&fabric_encap_));
    return impl_->reactivate_hw(this, epoch, obj_ctxt);
}

sdk_ret_t
vnic_entry::fill_spec_(pds_vnic_spec_t *spec) {
    subnet_entry *subnet;
    uint8_t i;

    subnet = subnet_db()->find(&subnet_);
    if (subnet == NULL) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    memcpy(&spec->key, &key_, sizeof(pds_obj_key_t));
    memcpy(&spec->hostname, &hostname_, sizeof(hostname_));
    spec->subnet = subnet_;
    spec->primary = primary_;
    spec->fabric_encap = fabric_encap_;
    spec->vnic_encap = vnic_encap_;
    spec->v4_meter = v4_meter_;
    spec->v6_meter = v6_meter_;
    spec->switch_vnic = switch_vnic_;
    spec->binding_checks_en = binding_checks_en_;
    spec->host_if = host_if_;
    spec->num_ing_v4_policy = num_ing_v4_policy_;
    for (i = 0; i < num_ing_v4_policy_; i++) {
        spec->ing_v4_policy[i] = ing_v4_policy_[i];
    }
    spec->num_ing_v6_policy = num_ing_v6_policy_;
    for (i = 0; i < num_ing_v6_policy_; i++) {
        spec->ing_v6_policy[i] = ing_v6_policy_[i];
    }
    spec->num_egr_v4_policy = num_egr_v4_policy_;
    for (i = 0; i < num_egr_v4_policy_; i++) {
        spec->egr_v4_policy[i] = egr_v4_policy_[i];
    }
    spec->num_egr_v6_policy = num_egr_v6_policy_;
    for (i = 0; i < num_egr_v6_policy_; i++) {
        spec->egr_v6_policy[i] = egr_v6_policy_[i];
    }
    spec->rx_policer = rx_policer_;
    spec->tx_policer = tx_policer_;
    spec->meter_en = meter_en_;
    return SDK_RET_OK;
}

sdk_ret_t
vnic_entry::read(pds_vnic_info_t *info) {
    sdk_ret_t ret;

    ret = pds_ipc_cfg_get(PDS_IPC_ID_VPP, OBJ_ID_VNIC, &key_, info);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = fill_spec_(&info->spec);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    return impl_->read_hw(this, (impl::obj_key_t *)(&info->spec.key),
                          (impl::obj_info_t *)info);
}

sdk_ret_t
vnic_entry::reset_stats(void) {
    return impl_->reset_stats();
}

sdk_ret_t
vnic_entry::add_to_db(void) {
    PDS_TRACE_VERBOSE("Adding vnic %s to db", key_.str());
    return vnic_db()->insert(this);
}

sdk_ret_t
vnic_entry::del_from_db(void) {
    if (vnic_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
vnic_entry::update_db(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    if (vnic_db()->remove((vnic_entry *)orig_obj)) {
        return vnic_db()->insert(this);
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
vnic_entry::delay_delete(void) {
    return delay_delete_to_slab(PDS_SLAB_ID_VNIC, this);
}

sdk_ret_t
vnic_entry::backup(upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    pds_vnic_info_t info;

    memset(&info, 0, sizeof(pds_vnic_info_t));
    fill_spec_(&info.spec);
    ret = impl_->backup((impl::obj_info_t *)&info, upg_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to backup vnic %s, err %u", key_.str(), ret);
    }
    return ret;
}

sdk_ret_t
vnic_entry::restore(upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    api_ctxt_t *api_ctxt;
    pds_vnic_info_t info;

    memset(&info, 0, sizeof(pds_vnic_info_t));
    // fetch info from proto buf
    ret = impl_->restore((impl::obj_info_t *)&info, upg_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore vnic err %u", ret);
        return ret;
    }
    // restore PI fields
    api_ctxt = api::api_ctxt_alloc((obj_id_t)upg_info->obj_id, API_OP_NONE);
    if (api_ctxt == NULL) {
        return SDK_RET_OOM;
    }
    api_ctxt->api_params->vnic_spec = info.spec;
    ret = init_config(api_ctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore vnic %s, err %u",
                      info.spec.key.str(), ret);
        return ret;
    }
    api_ctxt_free(api_ctxt);
    return ret;
}

/// @}     // end of PDS_VNIC_ENTRY

}    // namespace api
