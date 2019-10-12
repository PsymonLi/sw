//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// device api implementation for this pipeline
///
//----------------------------------------------------------------------------

#include <cmath>
#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/sdk/lib/utils/port_utils.hpp"
#include "nic/sdk/lib/catalog/catalog.hpp"
#include "nic/apollo/api/impl/devapi_impl.hpp"
#include "nic/sdk/platform/devapi/devapi_types.hpp"
#include "nic/sdk/asic/pd/scheduler.hpp"
// TODO: replace this with asic pd
#include "nic/sdk/platform/capri/capri_tm_rw.hpp"
#include "nic/sdk/linkmgr/port_mac.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"
#include "nic/apollo/api/impl/apollo/pds_impl_state.hpp"

namespace api {
namespace impl {

#define MAX_FILTERS_CLASSIC 32

devapi *
devapi_impl::factory(void) {
    devapi_impl    *impl;

    impl = (devapi_impl *)SDK_CALLOC(SDK_MEM_ALLOC_DEVAPI_IMPL,
                                     sizeof(devapi_impl));
    if (!impl) {
        return NULL;
    }
    new (impl) devapi_impl();
    return impl;
}

void
devapi_impl::destroy(devapi *impl) {
    (dynamic_cast<devapi_impl *>(impl))->~devapi_impl();
    SDK_FREE(SDK_MEM_ALLOC_DEVAPI_IMPL, impl);
}

sdk_ret_t
devapi_impl::lif_create(lif_info_t *info) {
    sdk_ret_t ret;
    lif_impl *lif;
    pds_lif_spec_t spec = { 0 };

    // program tx scheduler
    lif_program_tx_scheduler_(info);

    spec.key = info->lif_id;
    spec.pinned_ifidx = info->pinned_uplink_port_num;
    spec.type = info->type;
    spec.vlan_strip_en = info->vlan_strip_en;
    lif = lif_impl::factory(&spec);
    if (unlikely(lif == NULL)) {
        return sdk::SDK_RET_OOM;
    }
    ret = lif->create(&spec);
    if (ret == SDK_RET_OK) {
        PDS_TRACE_DEBUG("Programmed lif %u %s %u",
                        info->lif_id, info->name, info->type);
        lif_impl_db()->insert(lif);
    }
    return ret;
}

sdk_ret_t
devapi_impl::lif_destroy(uint32_t lif_id) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_reset(uint32_t lif_id) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_add_mac(uint32_t lif_id, mac_t mac) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_del_mac(uint32_t lif_id, mac_t mac) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_add_vlan(uint32_t lif_id, vlan_t vlan) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_del_vlan(uint32_t lif_id, vlan_t vlan) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_add_macvlan(uint32_t lif_id, mac_t mac, vlan_t vlan) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_del_macvlan(uint32_t lif_id, mac_t mac, vlan_t vlan) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_upd_vlan_offload(uint32_t lif_id, bool vlan_strip,
                                  bool vlan_insert) {
    // TODO: handle vlan strip
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_upd_rx_mode(uint32_t lif_id, bool broadcast,
                             bool all_multicast, bool promiscuous) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_upd_name(uint32_t lif_id, string name) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_get_max_filters(uint32_t *ucast_filters,
                                 uint32_t *mcast_filters) {
    *ucast_filters = *mcast_filters = MAX_FILTERS_CLASSIC;
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::qos_class_get(uint8_t group, qos_class_info_t *info) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::qos_class_create(qos_class_info_t *info) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::qos_class_delete(uint8_t group) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::qos_get_txtc_cos(const string &group, uint32_t uplink_port,
                              uint8_t *cos) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::uplink_create(__UNUSED__ uint32_t uplink_ifidx,
                           pds_ifindex_t ifidx, bool is_oob) {
#if 0
    sdk_ret_t ret;
    if_entry *intf;
    uint32_t logical_port, tm_port;

    PDS_TRACE_DEBUG("Uplink Create ifidx %u, pds_ifidx 0x%x, is_oob %d",
                    uplink_ifidx, ifidx, is_oob);

    intf = if_db()->find(&ifidx);
    if (intf == NULL) {
        PDS_TRACE_ERR("Unable to get if for ifidx 0x%x", ifidx);
        return SDK_RET_INVALID_ARG;
    }
    logical_port = sdk::lib::catalog::ifindex_to_logical_port(ifidx);
    tm_port = g_pds_state.catalogue()->ifindex_to_tm_port(ifidx);
    PDS_TRACE_DEBUG("TM Register log_port %u, tm_port %u",
                    logical_port, tm_port);
    // we use logical port as uplink hw lif identifier in the system
    ret = sdk::platform::capri::capri_tm_uplink_lif_set(tm_port, logical_port);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program uplink ifidx 0x%x's lif id "
                      "in TM, err %u", ret);
    }
    return ret;
#endif
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::port_get_config(pds_ifindex_t ifidx, port_config_t *config) {
    sdk_ret_t ret = SDK_RET_OK;
    if_entry *intf;
    port_args_t port_args = { 0 };

    intf = if_db()->find(&ifidx);
    if (intf == NULL) {
        PDS_TRACE_ERR("Unable to if for ifidx 0x%x", ifidx);
        return SDK_RET_INVALID_ARG;
    }

    ret = sdk::linkmgr::port_get(intf->port_info(), &port_args);

    config->speed = sdk::lib::port_speed_enum_to_mbps(port_args.port_speed);
    config->mtu = port_args.mtu;
    config->state =
        sdk::lib::port_admin_state_enum_to_uint(port_args.admin_state);
    config->an_enable = port_args.auto_neg_enable;
    config->fec_type = (uint8_t)port_args.fec_type;
    config->pause_type = (uint8_t)port_args.pause;
    config->loopback_mode = (uint8_t)port_args.loopback_mode;

    PDS_TRACE_DEBUG("if 0x%x, speed %u, mtu %u, state %u, an_enable %u, "
                    "fec_type %u, pause_type %u, loopback_mode %u",
                    ifidx, config->speed, config->mtu, config->state,
                    config->an_enable, config->fec_type, config->pause_type,
                    config->loopback_mode);
    return ret;
}

sdk_ret_t
devapi_impl::port_get_status(pds_ifindex_t ifidx, port_status_t *status) {
    sdk_ret_t ret = SDK_RET_OK;
    if_entry *intf;
    port_args_t port_args = { 0 };

    intf = if_db()->find(&ifidx);
    if (intf == NULL) {
        PDS_TRACE_ERR("Unable to if for ifidx 0x%x", ifidx);
        return SDK_RET_INVALID_ARG;
    }

    ret = sdk::linkmgr::port_get(intf->port_info(), &port_args);

    // status->speed =
    // status->id =
    status->status =
        sdk::lib::port_oper_state_enum_to_uint(port_args.oper_status);
    status->xcvr.state = port_args.xcvr_event_info.state;
    // status->xcvr.phy =
    status->xcvr.pid = port_args.xcvr_event_info.pid;
    memcpy(status->xcvr.sprom, port_args.xcvr_event_info.xcvr_sprom,
           sizeof(status->xcvr.sprom));

    PDS_TRACE_DEBUG("if 0x%x, status %u, xcvr state %u, pid %u",
                    ifidx, status->status, status->xcvr.state,
                    status->xcvr.pid);

    return ret;
}


sdk_ret_t
devapi_impl::port_set_config(uint32_t port_num, port_config_t *config) {
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::swm_update(bool enable,
                        uint32_t port_num,
                        uint32_t vlan,
                        mac_t mac)
{
    PDS_TRACE_WARN("Not implemented");
    return SDK_RET_OK;
}

sdk_ret_t
devapi_impl::lif_program_tx_scheduler_(lif_info_t *info) {
    asicpd_scheduler_lif_params_t   apd_lif;
    sdk_ret_t                       ret = SDK_RET_OK;

    apd_lif.lif_id = info->lif_id;
    apd_lif.hw_lif_id = info->lif_id;
    apd_lif.tx_sched_table_offset = INVALID_INDEXER_INDEX;
    apd_lif.tx_sched_num_table_entries = 0;
    apd_lif.total_qcount = lif_get_qcount_(info);
    apd_lif.cos_bmp = lif_get_cos_bmp_(info);

    // allocate tx-scheduler resource
    ret = sdk::asic::pd::asicpd_tx_scheduler_map_alloc(&apd_lif);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to alloc tx sched. lif %lu. err %d",
                      info->lif_id, ret);
        return ret;
    }

    // program tx scheduler and policer
    ret = sdk::asic::pd::asicpd_tx_scheduler_map_program(&apd_lif);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program tx sched. lif %lu. err %d",
                      info->lif_id, ret);
        return ret;
    }
    return ret;
}

uint32_t
devapi_impl::lif_get_qcount_(lif_info_t *info) {
    uint32_t qcount = 0;

     for (uint32_t i = 0; i < NUM_QUEUE_TYPES; i++) {
         auto & qinfo = info->queue_info[i];
         if (qinfo.size < 1) continue;

         PDS_TRACE_DEBUG("Queue type_num %lu, entries %lu, purpose %lu",
                         qinfo.type_num,
                         qinfo.entries, qinfo.purpose);
         qcount += pow(2, qinfo.entries);
     }
     PDS_TRACE_DEBUG("Lifid %u, qcount %lu", info->lif_id, qcount);
     return qcount;
}

uint16_t
devapi_impl::lif_get_cos_bmp_(lif_info_t *info) {
    uint16_t cos_bmp = 0;
    uint16_t cosA = DEVAPI_IMPL_ADMIN_COS;
    uint16_t cosB = 0;
    uint16_t cos_control = 2;

    cos_bmp =  ((1 << cosA) | (1 << cosB) | (1 << cos_control));
    return cos_bmp;
}



}    // namespace impl
}    // namespace api
