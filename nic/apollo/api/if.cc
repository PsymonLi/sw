//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// interface entry handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/linkmgr/port.hpp"
#include "nic/sdk/platform/drivers/xcvr.hpp"
#include "nic/infra/core/mem.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/if.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/api/include/pds_if.hpp"
#include "nic/apollo/api/include/pds_lif.hpp"
#include "nic/apollo/api/internal/metrics.hpp"
#include "nic/apollo/api/utils.hpp"

using sdk::lib::catalog;

namespace api {

if_entry::if_entry() {
    ht_ctxt_.reset();
    memset(&if_info_, 0, sizeof(if_info_));
    ifindex_ = IFINDEX_INVALID;
    num_tx_mirror_session_ = 0;
    num_rx_mirror_session_ = 0;
    ifindex_ht_ctxt_.reset();
}

// TODO: this method should go away !!!
if_entry *
if_entry::factory(pds_obj_key_t& key, if_index_t ifindex) {
    if_entry *intf;

    // create interface entry with defaults, if any
    intf = if_db()->alloc();
    if (intf) {
        new (intf) if_entry();
    }
    memcpy(&intf->key_, &key, sizeof(key_));
    intf->type_ = ifindex_to_if_type(ifindex);
    intf->ifindex_ = ifindex;
    return intf;
}

if_entry *
if_entry::factory(pds_if_spec_t *spec) {
    if_entry *intf;

    // create interface entry with defaults, if any
    intf = if_db()->alloc();
    if (intf) {
        new (intf) if_entry();
        intf->impl_ = (if_impl_base *)impl_base::factory(impl::IMPL_OBJ_ID_IF,
                                                         spec);
    }
    return intf;
}

if_entry::~if_entry() {
}

void
if_entry::destroy(if_entry *intf) {
    intf->nuke_resources_();
    if (intf->impl_) {
        impl_base::destroy(impl::IMPL_OBJ_ID_IF, (impl_base *)intf->impl_);
    }
    intf->~if_entry();
    if_db()->free(intf);
}

api_base *
if_entry::clone(api_ctxt_t *api_ctxt) {
    if_entry *cloned_if;

    cloned_if = if_db()->alloc();
    if (cloned_if) {
        new (cloned_if) if_entry();
        if (cloned_if->init_config(api_ctxt) != SDK_RET_OK) {
            goto error;
        }
        if (cloned_if->type() == IF_TYPE_HOST) {
            // host interface name and mac address cannot
            // be modified by external apis so copy them
            cloned_if->set_host_if_name(if_info_.host_.name_);
            cloned_if->set_host_if_mac(if_info_.host_.mac_);
        }
        cloned_if->impl_ = (if_impl_base *)impl_->clone();
        if (unlikely(cloned_if->impl_ == NULL)) {
            PDS_TRACE_ERR("Failed to clone intf %s impl", key_.str());
            goto error;
        }
        if (type() == IF_TYPE_ETH) {
            cloned_if->set_port_info(port_info());
        }
    }
    return cloned_if;

error:

    cloned_if->~if_entry();
    if_db()->free(cloned_if);
    return NULL;
}

sdk_ret_t
if_entry::free(if_entry *intf) {
    if (intf->impl_) {
        impl_base::free(impl::IMPL_OBJ_ID_IF, (impl_base *)intf->impl_);
    }
    intf->~if_entry();
    if_db()->free(intf);
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::reserve_resources(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        return impl_->reserve_resources(this, orig_obj, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::release_resources(void) {
    if (impl_) {
        impl_->release_resources(this);
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::nuke_resources_(void) {
    if (impl_) {
        return impl_->nuke_resources(this);
    }
    return SDK_RET_OK;
}

void
if_entry::dump_stats(uint32_t fd) {
    if (impl_ && (type_ == IF_TYPE_UPLINK)) {
        impl_->dump_stats(this, fd);
    }
}

sdk_ret_t
if_entry::track_pps(uint32_t interval) {
    if (type_ != IF_TYPE_UPLINK) {
        return SDK_RET_INVALID_ARG;
    }
    if (!impl_) {
        return SDK_RET_ERR;
    }
    return impl_->track_pps(this, interval);
}

sdk_ret_t
if_entry::init_config(api_ctxt_t *api_ctxt) {
    static uint32_t l3_if_idxr_ = 0;
    pds_if_spec_t *spec = &api_ctxt->api_params->if_spec;

    memcpy(&key_, &spec->key, sizeof(key_));
    type_ = spec->type;
    admin_state_ = spec->admin_state;
    switch (type_) {
    case IF_TYPE_UPLINK:
        {
            if_entry *phy_intf;
            phy_intf = if_db()->find(&spec->uplink_info.port);
            ifindex_ = phy_intf->ifindex();
            ifindex_ = ETH_IFINDEX_TO_UPLINK_IFINDEX(ifindex_);
            PDS_TRACE_DEBUG("Initializing uplink interface %s, ifindex 0x%x, "
                            "port %s", spec->key.str(), ifindex_,
                            spec->uplink_info.port.str());
            if_info_.uplink_.port_ = spec->uplink_info.port;
        }
        break;

    case IF_TYPE_L3:
        if (ifindex_ == IFINDEX_INVALID) {
            ifindex_ = L3_IFINDEX(l3_if_idxr_++);
        }
        PDS_TRACE_DEBUG("Initializing L3 interface %s, ifindex 0x%x, "
                        "port %s", spec->key.str(), ifindex_,
                        spec->l3_if_info.port.str());
        if_info_.l3_.vpc_ = spec->l3_if_info.vpc;
        if_info_.l3_.ip_pfx_ = spec->l3_if_info.ip_prefix;
        if_info_.l3_.port_ = spec->l3_if_info.port;
        if_info_.l3_.encap_ = spec->l3_if_info.encap;
        memcpy(if_info_.l3_.mac_, spec->l3_if_info.mac_addr,
               ETH_ADDR_LEN);
        break;

    case IF_TYPE_CONTROL:
        ifindex_ = CONTROL_IFINDEX(0);
        PDS_TRACE_DEBUG("Initializing inband control interface %s, "
                        "ifindex 0x%x", spec->key.str(), ifindex_);
        if_info_.control_.ip_pfx_ = spec->control_if_info.ip_prefix;
        memcpy(if_info_.control_.mac_, spec->control_if_info.mac_addr,
               ETH_ADDR_LEN);
        if_info_.control_.gateway_ = spec->control_if_info.gateway;
        break;

    case IF_TYPE_HOST:
        ifindex_ =
            HOST_IFINDEX(IFINDEX_TO_IFID(api::objid_from_uuid(spec->key)));
        if_info_.host_.tx_policer_ = spec->host_if_info.tx_policer;
        num_tx_mirror_session_ = spec->num_tx_mirror_session;
        for (uint8_t i = 0; i < num_tx_mirror_session_; i++) {
            tx_mirror_session_[i] = spec->tx_mirror_session[i];
        }
        num_rx_mirror_session_ = spec->num_rx_mirror_session;
        for (uint8_t i = 0; i < num_rx_mirror_session_; i++) {
            rx_mirror_session_[i] = spec->rx_mirror_session[i];
        }
        if_info_.host_.conn_track_en_ = spec->host_if_info.conn_track_en;
        break;

    case IF_TYPE_ETH:
        ifindex_ = api::objid_from_uuid(key_);
        break;

    case IF_TYPE_LOOPBACK:
        if_info_.loopback_.ip_pfx_ = spec->loopback_if_info.ip_prefix;
        break;

    case IF_TYPE_NONE:
        break;

    default:
        return sdk::SDK_RET_INVALID_ARG;
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::port_create_(pds_if_spec_t *spec) {
    port_args_t port_args;
    void *port_info;

    memset(&port_args, 0, sizeof(port_args_t));
    port_api_spec_to_args_(&port_args, spec);
    // sdk port_num is logical port
    port_args.port_num =
        sdk::lib::catalog::ifindex_to_logical_port(ifindex());
    // update port_args based on the xcvr state
    sdk::linkmgr::port_args_set_by_xcvr_state(&port_args);
    port_info = sdk::linkmgr::port_create(&port_args);
    if (port_info == NULL) {
        PDS_TRACE_ERR("port %u create failed", port_args.port_num);
        return SDK_RET_ERR;
    }
    set_port_info(port_info);
    // register the stats region with metrics submodule
    if (port_args.port_type == port_type_t::PORT_TYPE_ETH) {
        sdk::metrics::row_address(
            g_pds_state.port_metrics_handle(),
            *(sdk::metrics::key_t *)key().id,
            (void *)sdk::linkmgr::port_stats_addr(ifindex()));
    } else if (port_args.port_type == port_type_t::PORT_TYPE_MGMT) {
        sdk::metrics::row_address(
            g_pds_state.mgmt_port_metrics_handle(),
            *(sdk::metrics::key_t *)key().id,
            (void *)sdk::linkmgr::port_stats_addr(ifindex()));
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::program_create(api_obj_ctxt_t *obj_ctxt) {
    pds_if_spec_t *spec = &obj_ctxt->api_params->if_spec;

    if (type() == IF_TYPE_ETH) {
        port_create_(spec);
    }
    if (impl_) {
        return impl_->program_hw(this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::cleanup_config(api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        return impl_->cleanup_hw(this, obj_ctxt);
    }
    return SDK_RET_OK;
}

bool
if_entry::circulate(api_obj_ctxt_t *obj_ctxt) {
    if ((type_ == IF_TYPE_L3) || (type_ == IF_TYPE_LOOPBACK)) {
        return true;
    }
    return false;
}

sdk_ret_t
if_entry::populate_msg(pds_msg_t *msg, api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.op = obj_ctxt->api_op;
    msg->cfg_msg.obj_id = OBJ_ID_IF;
    if (obj_ctxt->api_op == API_OP_DELETE) {
        msg->cfg_msg.intf.key = obj_ctxt->api_params->key;
    } else {
        msg->cfg_msg.intf.spec = obj_ctxt->api_params->if_spec;
        impl_->populate_msg(msg, this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::compute_update(api_obj_ctxt_t *obj_ctxt) {
    pds_if_spec_t *spec = &obj_ctxt->api_params->if_spec;

    obj_ctxt->upd_bmap = 0;
    if (type_ != spec->type) {
        PDS_TRACE_ERR("Attempt to modify immutable attr \"type\" "
                      "from type %u to %u on interface %s",
                      type_, spec->type, key_.str());
        return SDK_RET_INVALID_ARG;
    }

    if (admin_state_ != spec->admin_state) {
        obj_ctxt->upd_bmap |= PDS_IF_UPD_ADMIN_STATE;
    }

    if (type_ == IF_TYPE_UPLINK) {
        // no other changes are relevant to uplink
        return SDK_RET_OK;
    } else if (type_ == IF_TYPE_HOST) {
        if (if_info_.host_.tx_policer_ != spec->host_if_info.tx_policer) {
            obj_ctxt->upd_bmap |= PDS_IF_UPD_TX_POLICER;
        }
        if (if_info_.host_.conn_track_en_ != spec->host_if_info.conn_track_en) {
            obj_ctxt->upd_bmap |= PDS_IF_UPD_CONN_TRACK_EN;
        }
    }
    if ((num_tx_mirror_session_ != spec->num_tx_mirror_session) ||
        memcmp(tx_mirror_session_, spec->tx_mirror_session,
               num_tx_mirror_session_ * sizeof(tx_mirror_session_[0]))) {
        obj_ctxt->upd_bmap |= PDS_IF_UPD_TX_MIRROR_SESSION;
    }
    if ((num_rx_mirror_session_ != spec->num_rx_mirror_session) ||
        memcmp(rx_mirror_session_, spec->rx_mirror_session,
               num_rx_mirror_session_ * sizeof(rx_mirror_session_[0]))) {
        obj_ctxt->upd_bmap |= PDS_IF_UPD_RX_MIRROR_SESSION;
    }
    PDS_TRACE_DEBUG("if %s update bmap 0x%lx", key_.str(), obj_ctxt->upd_bmap);
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::program_update(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    port_args_t port_args;
    pds_if_spec_t *spec;

    if (type() == IF_TYPE_ETH) {
        memset(&port_args, 0, sizeof(port_args_t));
        spec = &obj_ctxt->api_params->if_spec;
        port_api_spec_to_args_(&port_args, spec);
        // sdk port_num is logical port
        port_args.port_num =
            sdk::lib::catalog::ifindex_to_logical_port(ifindex());
        // update port_args based on the xcvr state
        sdk::linkmgr::port_args_set_by_xcvr_state(&port_args);
        return sdk::linkmgr::port_update(port_info(), &port_args);
    }
    if (impl_) {
        return impl_->update_hw(orig_obj, this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::activate_config(pds_epoch_t epoch, api_op_t api_op,
                          api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        return impl_->activate_hw(this, orig_obj, epoch, api_op, obj_ctxt);
    }
    return SDK_RET_OK;
}

void
if_entry::fill_port_if_spec_(pds_if_spec_t *spec, port_args_t *port_args) {
    spec->port_info.admin_state = port_args->user_admin_state;
    spec->port_info.type = port_args->port_type;
    spec->port_info.speed = port_args->port_speed;
    spec->port_info.fec_type = port_args->user_fec_type;
    spec->port_info.autoneg_en = port_args->auto_neg_cfg;
    spec->port_info.debounce_timeout = port_args->debounce_time;
    spec->port_info.mtu = port_args->mtu;
    spec->port_info.pause_type = port_args->pause;
    spec->port_info.tx_pause_en = port_args->tx_pause_enable;
    spec->port_info.rx_pause_en = port_args->rx_pause_enable;
    spec->port_info.loopback_mode = port_args->loopback_mode;
    spec->port_info.num_lanes = port_args->num_lanes_cfg;
}

void
if_entry::fill_port_if_status_(pds_if_status_t *status,
                               port_args_t *port_args) {
    status->port_status.link_status.oper_state = port_args->oper_status;
    status->port_status.link_status.speed = port_args->port_speed;
    status->port_status.link_status.autoneg_en = port_args->auto_neg_enable;
    status->port_status.link_status.num_lanes = port_args->num_lanes;
    status->port_status.link_status.fec_type = port_args->fec_type;

    status->port_status.xcvr_status.port = port_args->xcvr_event_info.phy_port;
    status->port_status.xcvr_status.state = port_args->xcvr_event_info.state;
    status->port_status.xcvr_status.pid = port_args->xcvr_event_info.pid;
    status->port_status.xcvr_status.media_type =
        port_args->xcvr_event_info.cable_type;
    memcpy(status->port_status.xcvr_status.xcvr_sprom,
           port_args->xcvr_event_info.sprom,
           sizeof(uint8_t) * XCVR_SPROM_SIZE);

    status->port_status.fsm_state = port_args->link_sm;
    status->port_status.mac_id = port_args->mac_id;
    status->port_status.mac_ch = port_args->mac_ch;
    status->port_status.sm_logger = port_args->sm_logger;
}

void
if_entry::fill_port_if_stats_(pds_if_stats_t *stats, port_args_t *port_args) {
    memcpy(stats->port_stats.stats, port_args->stats_data,
           sizeof(uint64_t) * MAX_MAC_STATS);
    stats->port_stats.num_linkdown = port_args->num_link_down;
    strncpy(stats->port_stats.last_down_timestamp, port_args->last_down_timestamp, TIME_STR_SIZE);
    stats->port_stats.bringup_duration.tv_sec = port_args->bringup_duration.tv_sec;
    stats->port_stats.bringup_duration.tv_nsec = port_args->bringup_duration.tv_nsec;
}

void
if_entry::port_api_spec_to_args_(port_args_t *port_args,
                                 pds_if_spec_t *spec) {
    port_args->port_num = sdk::lib::catalog::ifindex_to_logical_port(ifindex());
    port_args->admin_state = spec->port_info.admin_state;
    port_args->port_type = spec->port_info.type;
    port_args->port_speed = spec->port_info.speed;
    port_args->fec_type = spec->port_info.fec_type;
    port_args->auto_neg_enable = spec->port_info.autoneg_en;
    port_args->debounce_time = spec->port_info.debounce_timeout;
    port_args->mtu = spec->port_info.mtu;
    port_args->pause = spec->port_info.pause_type;
    port_args->tx_pause_enable = spec->port_info.tx_pause_en;
    port_args->rx_pause_enable = spec->port_info.rx_pause_en;
    port_args->loopback_mode = spec->port_info.loopback_mode;
    port_args->num_lanes = spec->port_info.num_lanes;

    // invoke after populating port_args from spec
    sdk::linkmgr::port_store_user_config(port_args);
}

sdk_ret_t
if_entry::port_get(port_args_t *port_args) {
    sdk_ret_t ret;
    int phy_port;

    ret = sdk::linkmgr::port_get(port_info(), port_args);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to get port 0x%s info, err %u", key().str(), ret);
        return ret;
    }
    if (port_args->port_type != port_type_t::PORT_TYPE_MGMT) {
        phy_port = sdk::lib::catalog::ifindex_to_phy_port(ifindex());
        ret = sdk::platform::xcvr_get(phy_port - 1,
                                      &port_args->xcvr_event_info);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to get xcvr for phy port %u, err %u",
                          phy_port, ret);
            return ret;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::fill_spec_(pds_if_spec_t *spec, port_args_t *port_args) {
    memcpy(&spec->key, &key_, sizeof(key_));
    spec->type = type_;
    spec->admin_state = admin_state_;

    switch (spec->type) {
    case IF_TYPE_UPLINK:
        spec->uplink_info.port = if_info_.uplink_.port_;
        break;
    case IF_TYPE_L3:
        spec->l3_if_info.port = if_info_.l3_.port_;
        spec->l3_if_info.vpc = if_info_.l3_.vpc_;
        spec->l3_if_info.ip_prefix = if_info_.l3_.ip_pfx_;
        spec->l3_if_info.encap = if_info_.l3_.encap_;
        memcpy(spec->l3_if_info.mac_addr, if_info_.l3_.mac_,
               ETH_ADDR_LEN);
        break;
    case IF_TYPE_LOOPBACK:
        spec->loopback_if_info.ip_prefix = if_info_.loopback_.ip_pfx_;
        break;
    case IF_TYPE_CONTROL:
        spec->control_if_info.ip_prefix = if_info_.control_.ip_pfx_;
        memcpy(spec->control_if_info.mac_addr, if_info_.control_.mac_,
               ETH_ADDR_LEN);
        spec->control_if_info.gateway = if_info_.control_.gateway_;
        break;
    case IF_TYPE_HOST:
        spec->host_if_info.tx_policer = if_info_.host_.tx_policer_;
        break;
    case IF_TYPE_ETH:
        fill_port_if_spec_(spec, port_args);
        break;
    default:
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

pds_if_state_t
if_entry::state(void) {
    sdk_ret_t ret;
    if_entry *eth_if;
    port_args_t port_args = { 0 };

    // get the eth interface corresponding to this interface entry
    eth_if = if_entry::eth_if((if_entry *)this);
    if (eth_if == NULL) {
        return PDS_IF_STATE_NONE;
    }
    // and get the oper state of the eth interface
    ret = sdk::linkmgr::port_get(eth_if->port_info(), &port_args);
    if (likely(ret == SDK_RET_OK)) {
        if (port_args.oper_status == port_oper_status_t::PORT_OPER_STATUS_UP) {
            return PDS_IF_STATE_UP;
        }
        return PDS_IF_STATE_DOWN;
    }
    PDS_TRACE_ERR("Failed to get port 0x%x info, err %u", ifindex_, ret);
    return PDS_IF_STATE_NONE;
}

port_type_t
if_entry::port_type(void) {
    sdk_ret_t ret;
    if_entry *eth_if;
    port_args_t port_args = { 0 };

    // get the eth interface corresponding to this interface entry
    eth_if = if_entry::eth_if((if_entry *)this);
    if (eth_if == NULL) {
        return port_type_t::PORT_TYPE_NONE;
    }
    // and get the port type of the eth interface
    ret = sdk::linkmgr::port_get(eth_if->port_info(), &port_args);
    if (likely(ret == SDK_RET_OK)) {
        return port_args.port_type;
    }
    PDS_TRACE_ERR("Failed to get port 0x%x info, err %u", ifindex_, ret);
    return port_type_t::PORT_TYPE_NONE;
}

sdk_ret_t
if_entry::fill_status_(pds_if_status_t *status, port_args_t *port_args) {
    status->state = this->state();
    status->ifindex = ifindex();

    switch (type()) {
    case IF_TYPE_ETH:
        fill_port_if_status_(status, port_args);
        break;
    case IF_TYPE_HOST:
        {
            if_index_t ifindex;
            uint8_t num_lifs = 0;
            pds_obj_key_t lif_key;

            ifindex = LIF_IFINDEX(HOST_IFINDEX_TO_IF_ID(objid_from_uuid(key_)));
            lif_key = uuid_from_objid(ifindex);
            status->host_if_status.lifs[num_lifs++] = lif_key;
            status->host_if_status.num_lifs = num_lifs;
            strncpy(status->host_if_status.name,
                    this->name().c_str(), SDK_MAX_NAME_LEN);
            MAC_ADDR_COPY(status->host_if_status.mac_addr,
                          this->host_if_mac());
        }
        break;
    default:
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::fill_stats_(pds_if_stats_t *stats, port_args_t *port_args) {
    switch (type()) {
    case IF_TYPE_ETH:
        fill_port_if_stats_(stats, port_args);
        break;
    default:
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::read(pds_if_info_t *info) {
    sdk_ret_t ret;
    port_args_t port_args = { 0 };
    uint64_t stats[MAX_MAC_STATS];

    if (type() == IF_TYPE_ETH) {
        port_args.stats_data = stats;
        ret = port_get(&port_args);
        if(ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to get port info for %s, err %u",
                          eth_ifindex_to_str(ifindex()).c_str(), ret);
            return ret;
        }
    }
    ret = fill_spec_(&info->spec, &port_args);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = fill_status_(&info->status, &port_args);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = fill_stats_(&info->stats, &port_args);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    if (impl_) {
        return impl_->read_hw(this, (impl::obj_key_t *)(&info->spec.key),
                              (impl::obj_info_t *)info);
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_entry::add_to_db(void) {
    return if_db()->insert(this);
}

sdk_ret_t
if_entry::del_from_db(void) {
    if (if_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
if_entry::update_db(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    if (if_db()->remove((if_entry *)orig_obj)) {
        return if_db()->insert(this);
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
if_entry::delay_delete(void) {
    return delay_delete_to_slab(PDS_SLAB_ID_IF, this);
}

if_entry *
if_entry::eth_if(if_entry *intf) {
    switch (intf->type()) {
    case IF_TYPE_UPLINK:
        return if_db()->find(&intf->if_info_.uplink_.port_);

    case IF_TYPE_L3:
        return if_db()->find(&intf->if_info_.l3_.port_);

    case IF_TYPE_ETH:
        return intf;

    case IF_TYPE_CONTROL:
    case IF_TYPE_HOST:
        return NULL;

    default:
        PDS_TRACE_ERR("Unknown interface type %u", intf->type());
    }
    return NULL;
}

std::string
if_entry::name(void) {
    std::string system_mac;
    std::string separator = "-";
    std::string intf_type, intf_id;

    // NOTE: keep in sync with GetIfName() of dscagent
    system_mac = macaddr2str(api::g_pds_state.system_mac(), true);
    if (this->port_type() == port_type_t::PORT_TYPE_MGMT) {
        intf_type = "mgmt";
    } else {
        intf_type = if_type_to_str(type_);
    }

    switch (type_) {
    case IF_TYPE_ETH:
        intf_id = eth_ifindex_to_ifid_str(ifindex_, separator);
        break;
    case IF_TYPE_L3:
    case IF_TYPE_LOOPBACK:
        intf_id = std::to_string(IFINDEX_TO_IFID(ifindex_));
        break;
    case IF_TYPE_HOST:
        return std::string(if_info_.host_.name_);
    default:
        intf_id = "none";
        break;
    }

    return system_mac + separator + intf_type + separator + intf_id;
}

}    // namespace api
