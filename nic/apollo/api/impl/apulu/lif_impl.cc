//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of lif
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/lib/catalog/catalog.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/include/sdk/qos.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/asic/pd/scheduler.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/internal/lif.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"
#include "nic/apollo/api/impl/apulu/if_impl.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/nacl_data.h"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/p4/common/defines.h"
#include "gen/p4gen/apulu/include/p4pd.h"
#include "gen/p4gen/p4plus_txdma/include/p4plus_txdma_p4pd.h"
#include "gen/p4gen/p4/include/ftl.h"
#include "gen/platform/mem_regions.hpp"

#define COPP_FLOW_MISS_ARP_REQ_FROM_HOST_PPS    250
#define COPP_LEARN_MISS_ARP_REQ_FROM_HOST_PPS   250
#define COPP_FLOW_MISS_DHCP_REQ_FROM_HOST_PPS   250
#define COPP_LEARN_MISS_DHCP_REQ_FROM_HOST_PPS  250
#define COPP_LEARN_MISS_IP4_FROM_HOST_PPS       250
#define COPP_ARP_FROM_ARM_PPS                   250
#define COPP_BURST(pps)                         ((uint64_t)((pps)/2))
#define COPP_FLOW_MISS_TO_DATAPATH_LIF_PPS      600000
#define COPP_DEFUNCT_FLOW_TO_DATAPATH_LIF_PPS   100000

// 4 bits of copp class can be used to account for copp drops
#define P4_COPP_DROP_CLASS_OTHER                0
#define P4_COPP_DROP_CLASS_INBAND               1
#define P4_COPP_DROP_CLASS_ARP_HOST             2
#define P4_COPP_DROP_CLASS_DHCP_HOST            3
#define P4_COPP_DROP_CLASS_FLOW_EPOCH           4
#define P4_COPP_DROP_CLASS_FLOW_MISS            5
#define P4_COPP_DROP_CLASS_DATAPATH_LEARN       6

namespace api {
namespace impl {

/// \defgroup PDS_LIF_IMPL - lif entry datapath implementation
/// \ingroup PDS_LIF
/// \@{

lif_impl *
lif_impl::factory(pds_lif_spec_t *spec) {
    lif_impl *impl;
    impl = lif_impl_db()->alloc();
    if (unlikely(impl == NULL)) {
        return NULL;
    }
    new (impl) lif_impl(spec);
    return impl;
}

void
lif_impl::destroy(lif_impl *impl) {
    impl->~lif_impl();
    lif_impl_db()->free(impl);
}

lif_impl::lif_impl(pds_lif_spec_t *spec) {
    memcpy(&key_, &spec->key, sizeof(key_));
    id_ = spec->id;
    pinned_if_idx_ = spec->pinned_ifidx;
    type_ = spec->type;
    memcpy(mac_, spec->mac, ETH_ADDR_LEN);
    memset(name_, 0, sizeof(name_));
    ifindex_ = LIF_IFINDEX(id_);
    nh_idx_ = 0xFFFFFFFF;
    vnic_hw_id_ = 0xFFFF;
    tx_sched_offset_ = INVALID_INDEXER_INDEX;
    state_ = sdk::types::LIF_STATE_NONE;
    admin_state_ = sdk::types::LIF_STATE_NONE;
    create_done_ = false;
    ht_ctxt_.reset();
    id_ht_ctxt_.reset();
}

sdk_ret_t
lif_impl::reserve_tx_scheduler(pds_lif_spec_t *spec) {
    sdk_ret_t ret;
    asicpd_scheduler_lif_params_t lif_sched_params;

    // reserve scheduler units
    lif_sched_params.lif_id = spec->id;
    lif_sched_params.hw_lif_id = spec->id;
    lif_sched_params.tx_sched_table_offset = spec->tx_sched_table_offset;
    lif_sched_params.tx_sched_num_table_entries =
        spec->tx_sched_num_table_entries;
    ret = sdk::asic::pd::asicpd_tx_scheduler_map_reserve(&lif_sched_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve tx sched. lif %s, err %u",
                      spec->key.str(), ret);
    }
    return ret;
}

sdk_ret_t
lif_impl::program_tx_scheduler(pds_lif_spec_t *spec) {
    sdk_ret_t ret;
    asicpd_scheduler_lif_params_t lif_sched_params;

    lif_sched_params.lif_id = spec->id;
    lif_sched_params.hw_lif_id = spec->id;
    lif_sched_params.total_qcount = spec->total_qcount;
    lif_sched_params.cos_bmp = spec->cos_bmp;

    // allocate tx-scheduler resource, or use existing allocation
    lif_sched_params.tx_sched_table_offset = spec->tx_sched_table_offset;
    lif_sched_params.tx_sched_num_table_entries =
        spec->tx_sched_num_table_entries;
    ret = sdk::asic::pd::asicpd_tx_scheduler_map_alloc(&lif_sched_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to alloc tx sched. lif %s, err %u",
                      spec->key.str(), ret);
        return ret;
    }
    // save allocation in spec
    spec->tx_sched_table_offset = lif_sched_params.tx_sched_table_offset;
    spec->tx_sched_num_table_entries =
        lif_sched_params.tx_sched_num_table_entries;

    // program tx scheduler and policer
    ret = sdk::asic::pd::asicpd_tx_scheduler_map_program(&lif_sched_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program tx sched. lif %u, err %u",
                      spec->id, ret);
        return ret;
    }
    return ret;
}

#define lif_egress_rl_params       action_u.tx_table_s5_t4_lif_rate_limiter_table_tx_stage5_lif_egress_rl_params
sdk_ret_t
lif_impl::program_tx_policer(sdk::qos::policer_t *policer) {
    sdk_ret_t ret;
    uint64_t rate_tokens = 0, burst_tokens = 0, rate;
    asicpd_scheduler_lif_params_t lif_scheduler_params;
    uint64_t refresh_interval_us = SDK_DEFAULT_POLICER_REFRESH_INTERVAL;
    tx_table_s5_t4_lif_rate_limiter_table_actiondata_t rlimit_data = { 0 };

    if (policer) {
        rlimit_data.action_id =
            TX_TABLE_S5_T4_LIF_RATE_LIMITER_TABLE_TX_STAGE5_LIF_EGRESS_RL_PARAMS_ID;
        if (policer->rate == 0) {
            rlimit_data.lif_egress_rl_params.entry_valid = 0;
        } else {
            rlimit_data.lif_egress_rl_params.entry_valid = 1;
            rlimit_data.lif_egress_rl_params.pkt_rate =
                (policer->type ==
                          sdk::qos::policer_type_t::POLICER_TYPE_PPS) ? 1 : 0;
            rlimit_data.lif_egress_rl_params.rlimit_en = 1;
            ret = sdk::qos::policer_to_token_rate(
                                             policer, refresh_interval_us,
                                             SDK_MAX_POLICER_TOKENS_PER_INTERVAL,
                                             &rate_tokens, &burst_tokens);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Error converting rate to token rate, err %u",
                              ret);
                return ret;
            }
            memcpy(rlimit_data.lif_egress_rl_params.burst, &burst_tokens,
                   sizeof(rlimit_data.lif_egress_rl_params.burst));
            memcpy(rlimit_data.lif_egress_rl_params.rate, &rate_tokens,
                   sizeof(rlimit_data.lif_egress_rl_params.rate));
        }
        ret = lif_impl_db()->tx_rate_limiter_tbl()->insert_withid(&rlimit_data,
                                                                  id_, NULL);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("LIF_TX_POLICER table write failure, lif %s, err %u",
                          key_.str(), ret);
            return ret;
        }
    } else {
        // remove tx policer
        ret = lif_impl_db()->tx_rate_limiter_tbl()->remove(id_);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("LIF_TX_POLICER table write failure, lif %s, err %u",
                          key_.str(), ret);
            return ret;
        }
    }

    lif_scheduler_params.lif_id = id_;
    lif_scheduler_params.hw_lif_id = id_;
    lif_scheduler_params.tx_sched_table_offset = tx_sched_offset_;
    lif_scheduler_params.tx_sched_num_table_entries = tx_sched_num_entries_;
    lif_scheduler_params.total_qcount = total_qcount_;
    lif_scheduler_params.cos_bmp = cos_bmp_;
    if (policer) {
        // program mapping from rate-limiter-table to scheduler-table
        // rate-limiter-group (RLG) for pausing
        ret = sdk::asic::pd::asicpd_tx_policer_program(&lif_scheduler_params);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("lif %s unable to program hw for tx policer",
                          key_.str());
            return ret;
        }
    } else {
        // remove tx policer
        ret = sdk::asic::pd::asicpd_tx_policer_cleanup(&lif_scheduler_params);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("lif %s unable to cleanup hw for tx policer",
                          key_.str());
            return ret;
        }
    }

    return SDK_RET_OK;
}

void
lif_impl::set_name(const char *name) {
    if ((type_ == sdk::platform::LIF_TYPE_HOST_MGMT) ||
        (type_ == sdk::platform::LIF_TYPE_HOST)) {
        memcpy(name_, name, SDK_MAX_NAME_LEN);
    }
}

void
lif_impl::set_admin_state(lif_state_t state) {
    // update the admin state
    admin_state_ = state;
#if 0
    // and notify lif update
    if ((type_ == sdk::platform::LIF_TYPE_HOST_MGMT) ||
        (type_ == sdk::platform::LIF_TYPE_HOST)) {
        event.event_id = PDS_EVENT_ID_LIF_UPDATE;
        pds_lif_to_lif_spec(&event.lif_info.spec, this);
        pds_lif_to_lif_status(&event.lif_info.status, this);
        g_pds_state.event_notify(&event);
    }
#endif
}

#define nacl_redirect_action    action_u.nacl_nacl_redirect
#define nexthop_info            action_u.nexthop_nexthop_info
sdk_ret_t
lif_impl::install_oob_mnic_nacls_(uint32_t uplink_nh_idx) {
    sdk_ret_t ret;
    if_impl *intf;
    nacl_swkey_t key;
    if_index_t if_index;
    p4pd_error_t p4pd_ret;
    nacl_swkey_mask_t mask;
    nacl_actiondata_t data;
    uint32_t idx, nacl_idx;
    sdk::qos::policer_t policer;

    // cap ARP packets from oob lif(s) to 256 pps
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = {
        sdk::qos::POLICER_TYPE_PPS,
        COPP_ARP_FROM_ARM_PPS,
        COPP_BURST(COPP_ARP_FROM_ARM_PPS)
    };
    program_copp_entry_(&policer, idx, false);
    // install NACL entry for ARM to uplink ARP traffic (all vlans)
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.capri_intrinsic_lif = id_;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_MAC;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = ETH_TYPE_ARP;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.capri_intrinsic_lif_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_action.nexthop_id = uplink_nh_idx;
    data.nacl_redirect_action.copp_policer_id = idx;
    data.nacl_redirect_action.copp_class = P4_COPP_DROP_CLASS_INBAND;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // install NACL for ARM to uplink traffic (all vlans)
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.capri_intrinsic_lif = id_;
    mask.key_metadata_entry_valid_mask= ~0;
    mask.capri_intrinsic_lif_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_action.nexthop_id = uplink_nh_idx;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // install for NACL for uplink to ARM (untagged) traffic
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    if_index = ETH_IFINDEX_TO_UPLINK_IFINDEX(pinned_if_idx_);
    intf = (if_impl *)if_db()->find(&if_index)->impl();
    key.key_metadata_entry_valid = 1;
    key.capri_intrinsic_lif = intf->hw_id();
    mask.key_metadata_entry_valid_mask = ~0;
    mask.capri_intrinsic_lif_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_action.nexthop_id = nh_idx_;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    return SDK_RET_OK;
}

sdk_ret_t
lif_impl::create_oob_mnic_(pds_lif_spec_t *spec) {
    sdk_ret_t ret;
    uint32_t idx, uplink_nh_idx;
    nexthop_info_entry_t nexthop_info_entry;

    strncpy(name_, spec->name, sizeof(name_));
    PDS_TRACE_DEBUG("Creating oob lif %s", name_);

    // program the nexthop for ARM to uplink traffic
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.port =
            g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_);
    // per uplink nh idx is reserved during init time, so use it
    uplink_nh_idx = nexthop_info_entry.port + 1;
    ret = nexthop_info_entry.write(nexthop_info_entry.port + 1);
    SDK_ASSERT(ret == SDK_RET_OK);

    // allocate required nexthop for ARM out of band lif
    ret = nexthop_impl_db()->nh_idxr()->alloc(&nh_idx_);
    SDK_ASSERT(ret == SDK_RET_OK);

    // program the nexthop for uplink to ARM traffic
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.lif = id_;
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    ret = nexthop_info_entry.write(nh_idx_);
    SDK_ASSERT(ret == SDK_RET_OK);

    // install all oob NACLs
    ret = install_oob_mnic_nacls_(uplink_nh_idx);
    SDK_ASSERT(ret == SDK_RET_OK);

    // allocate vnic h/w id for this lif
    if ((ret = vnic_impl_db()->vnic_idxr()->alloc(&idx)) != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to allocate vnic h/w id for lif %s, err %u",
                      key_.str(), ret);
        return ret;
    }
    // program the LIF table
    ret = program_lif_table(id_, P4_LIF_TYPE_ARM_MGMT, PDS_IMPL_RSVD_VPC_HW_ID,
                            PDS_IMPL_RSVD_BD_HW_ID, idx, g_zero_mac,
                            false, true, P4_LIF_DIR_HOST);
    if (ret != SDK_RET_OK) {
        goto error;
    }
    return SDK_RET_OK;

error:

    SDK_ASSERT(FALSE);
    return ret;
}

sdk_ret_t
lif_impl::install_inb_mnic_nacls_(uint32_t uplink_nh_idx) {
    if_impl *intf;
    sdk_ret_t ret;
    nacl_swkey_t key;
    if_index_t if_index;
    p4pd_error_t p4pd_ret;
    uint32_t idx, nacl_idx;
    nacl_swkey_mask_t mask;
    nacl_actiondata_t data;
    sdk::qos::policer_t policer;

    // cap ARP packets from inband lif(s) to 256 pps
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = {
        sdk::qos::POLICER_TYPE_PPS,
        COPP_ARP_FROM_ARM_PPS,
        COPP_BURST(COPP_ARP_FROM_ARM_PPS)
    };
    program_copp_entry_(&policer, idx, false);
    // install NACL entry for ARM to uplink ARP traffic (all vlans)
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.capri_intrinsic_lif = id_;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_MAC;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = ETH_TYPE_ARP;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.capri_intrinsic_lif_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_action.nexthop_id = uplink_nh_idx;
    data.nacl_redirect_action.copp_policer_id = idx;
    data.nacl_redirect_action.copp_class = P4_COPP_DROP_CLASS_INBAND;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // install NACL for ARM to uplink traffic (all vlans)
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.capri_intrinsic_lif = id_;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.capri_intrinsic_lif_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_action.nexthop_id = uplink_nh_idx;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // install for NACL for uplink to ARM (untagged) traffic
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    if_index = ETH_IFINDEX_TO_UPLINK_IFINDEX(pinned_if_idx_);
    intf = (if_impl *)if_db()->find(&if_index)->impl();
    key.key_metadata_entry_valid = 1;
    key.capri_intrinsic_lif = intf->hw_id();
    key.control_metadata_lif_type = P4_LIF_TYPE_UPLINK;
    key.control_metadata_tunneled_packet = 0;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.capri_intrinsic_lif_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    key.control_metadata_flow_miss = 1;
    mask.control_metadata_flow_miss_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_action.nexthop_id = nh_idx_;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);
    return SDK_RET_OK;
}

#define p4i_device_info    action_u.p4i_device_info_p4i_device_info
sdk_ret_t
lif_impl::create_inb_mnic_(pds_lif_spec_t *spec) {
    sdk_ret_t ret;
    uint32_t idx,tm_port;
    p4pd_error_t p4pd_ret;
    lif_actiondata_t lif_data = { 0 };
    nexthop_info_entry_t nexthop_info_entry;
    p4i_device_info_actiondata_t p4i_device_info_data;

    strncpy(name_, spec->name, sizeof(name_));
    PDS_TRACE_DEBUG("Creating inband lif %s", name_);

    // program the nexthop for ARM to uplink traffic
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    tm_port = nexthop_info_entry.port =
        g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_);
    // per uplink nh idx is reserved during init time, so use it
    ret = nexthop_info_entry.write(tm_port + 1);
    SDK_ASSERT(ret == SDK_RET_OK);

    // allocate required nexthop for ARM inband lif
    ret = nexthop_impl_db()->nh_idxr()->alloc(&nh_idx_);
    SDK_ASSERT(ret == SDK_RET_OK);

    // program the nexthop for uplink to ARM traffic
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.lif = id_;
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    ret = nexthop_info_entry.write(nh_idx_);
    SDK_ASSERT(ret == SDK_RET_OK);

    // install all the NACLs needed for inband traffic
    ret = install_inb_mnic_nacls_(tm_port + 1);
    SDK_ASSERT(ret == SDK_RET_OK);

    // program the device info table with the MAC of this lif by
    // doing read-modify-write
    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_P4I_DEVICE_INFO, 0,
                                      NULL, NULL, &p4i_device_info_data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    if (tm_port == TM_PORT_UPLINK_0) {
        sdk::lib::memrev(p4i_device_info_data.p4i_device_info.device_mac_addr1,
                         spec->mac, ETH_ADDR_LEN);
    } else if (tm_port == TM_PORT_UPLINK_1) {
        sdk::lib::memrev(p4i_device_info_data.p4i_device_info.device_mac_addr2,
                         spec->mac, ETH_ADDR_LEN);
    }
    // program the P4I_DEVICE_INFO table
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4I_DEVICE_INFO, 0,
                                       NULL, NULL, &p4i_device_info_data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // allocate vnic h/w id for this lif
    SDK_ASSERT(vnic_impl_db()->vnic_idxr()->alloc(&idx) == SDK_RET_OK);
    // program the LIF table
    ret = program_lif_table(id_, P4_LIF_TYPE_ARM_MGMT, PDS_IMPL_RSVD_VPC_HW_ID,
                            PDS_IMPL_RSVD_BD_HW_ID, idx, g_zero_mac,
                            false, true, P4_LIF_DIR_HOST);
    SDK_ASSERT(ret == SDK_RET_OK);
    return SDK_RET_OK;
}

#define nacl_redirect_to_arm_action     action_u.nacl_nacl_redirect_to_arm
#define nacl_drop_action                action_u.nacl_nacl_drop
sdk_ret_t
lif_impl::create_datapath_mnic_(pds_lif_spec_t *spec) {
    sdk_ret_t ret;
    nacl_swkey_t key;
    p4pd_error_t p4pd_ret;
    uint32_t idx, nacl_idx;
    nacl_swkey_mask_t mask;
    nacl_actiondata_t data;
    static uint32_t dplif = 0;
    sdk::qos::policer_t policer;
    nexthop_info_entry_t nexthop_info_entry;

    strncpy(name_, spec->name, sizeof(name_));
    // during hitless upgrade, domain-b uses cpu_mnic2
    if (sdk::platform::sysinit_domain_b(api::g_upg_state->init_domain())) {
        if (!strncmp(name_, "cpu_mnic0", strlen("cpu_mnic0"))) {
            return SDK_RET_OK;
        }
    } else {
        if (!strncmp(name_, "cpu_mnic2", strlen("cpu_mnic2"))) {
            return SDK_RET_OK;
        }
    }
    PDS_TRACE_DEBUG("Creating s/w datapath lif %s, id %u", name_, id_);
    // allocate required nexthop to point to ARM datapath lif
    ret = nexthop_impl_db()->nh_idxr()->alloc(&nh_idx_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to allocate nexthop entry for flow miss, "
                      "lif %s, id %s, err %u", name_, key_.str(), ret);
        return ret;
    }

    // program the nexthop to point to vpp lif
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.lif = id_;
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    ret = nexthop_info_entry.write(nh_idx_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program NEXTHOP table for datapath lif %u "
                      "at idx %u", id_, nh_idx_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // cap ARP requests from host lifs to 256/sec
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = {
        sdk::qos::POLICER_TYPE_PPS,
        COPP_FLOW_MISS_ARP_REQ_FROM_HOST_PPS,
        COPP_BURST(COPP_FLOW_MISS_ARP_REQ_FROM_HOST_PPS)
    };
    program_copp_entry_(&policer, idx, false);
    // install NACL entry for ARP requests from host
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_MAC;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.control_metadata_flow_miss = 1;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = ETH_TYPE_ARP;
    key.key_metadata_sport = 1;    // ARP request
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_flow_miss_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    mask.key_metadata_sport_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_ARP_HOST;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_FLOW_MISS_ARP;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL redirect entry for "
                      "(flow miss, ARP requests from host) -> lif %s", name_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // cap DHCP requests from host lifs to 256/sec
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = {
        sdk::qos::POLICER_TYPE_PPS,
        COPP_FLOW_MISS_DHCP_REQ_FROM_HOST_PPS,
        COPP_BURST(COPP_FLOW_MISS_DHCP_REQ_FROM_HOST_PPS)
    };
    program_copp_entry_(&policer, idx, false);
    // install NACL entry for DHCP requests going to vpp
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_IPV4;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.control_metadata_flow_miss = 1;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = 67;
    key.key_metadata_sport = 68;
    key.key_metadata_proto = 17;    // UDP
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_flow_miss_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    mask.key_metadata_sport_mask = ~0;
    mask.key_metadata_proto_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_DHCP_HOST;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_FLOW_MISS_DHCP_HOST;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry for (host lif, DHCP req) "
                      "-> lif %s", name_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // allocate and program copp table entry for flow miss
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = {
        sdk::qos::POLICER_TYPE_PPS,
        COPP_DEFUNCT_FLOW_TO_DATAPATH_LIF_PPS,
        COPP_BURST(COPP_DEFUNCT_FLOW_TO_DATAPATH_LIF_PPS),
    };
    program_copp_entry_(&policer, idx, false);

    // flow miss packet coming back from txdma to going to s/w datapath lif
    // (i.e., dpdk/vpp lif) and hitting epoch mismatch condition
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_flow_miss = 1;
    key.ingress_recirc_defunct_flow = 1;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_flow_miss_mask = ~0;
    mask.ingress_recirc_defunct_flow_mask = ~0;
    if ((g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_HOST) ||
        (g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_BITW_SMART_SWITCH)) {
        // in all other modes, mappings are not installed in datapath
        key.control_metadata_local_mapping_miss = 0;
        mask.control_metadata_local_mapping_miss_mask = ~0;
    }
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_FLOW_EPOCH;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_FLOW_MISS_IP4_IP6;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry for defunct flow redirect "
                      "to arm lif %u", id_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // allocate and program copp table entry for flow miss
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = {
        sdk::qos::POLICER_TYPE_PPS,
        COPP_FLOW_MISS_TO_DATAPATH_LIF_PPS,
        COPP_BURST(COPP_FLOW_MISS_TO_DATAPATH_LIF_PPS)
    };
    program_copp_entry_(&policer, idx, false);

    // redirect flow miss encapped TCP traffic from uplinks to s/w datapath lif
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 1;
    key.control_metadata_lif_type = P4_LIF_TYPE_UPLINK;
    key.control_metadata_flow_miss = 1;
    key.control_metadata_tunneled_packet = 1;
    key.key_metadata_proto = IP_PROTO_TCP;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_flow_miss_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_proto_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_FLOW_MISS;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_FLOW_MISS_IP4_IP6;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry to redirect encapped TCP");
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // redirect flow miss encapped UDP traffic from uplinks to s/w datapath lif
    key.key_metadata_proto = IP_PROTO_UDP;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry to redirect encapped UDP");
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // redirect flow miss encapped ICMP traffic from uplinks to s/w datapath lif
    key.key_metadata_proto = IP_PROTO_ICMP;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry to redirect encapped ICMP");
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // drop all flow miss non-TCP/UDP/ICMP traffic from PFs and uplinks
    // TODO: dols need to be fixed for this to be relaxed
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_flow_miss = 1;
    key.control_metadata_tunneled_packet = 1;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_flow_miss_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    data.action_id = NACL_NACL_DROP_ID;
    data.nacl_drop_action.drop_reason_valid = 1;
    data.nacl_drop_action.drop_reason = P4I_DROP_PROTOCOL_NOT_SUPPORTED;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program drop entry for nonTCP/UDP/ICMP "
                      "encapped traffic");
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // flow miss packet coming back from txdma to s/w datapath
    // lif (i.e., dpdk/vpp lif)
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_flow_miss = 1;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_flow_miss_mask = ~0;
    if ((g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_HOST) ||
        (g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_BITW_SMART_SWITCH)) {
        // in all other modes, mappings are not installed in datapath
        key.control_metadata_local_mapping_miss = 0;
        mask.control_metadata_local_mapping_miss_mask = ~0;
    }
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_FLOW_MISS;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_FLOW_MISS_IP4_IP6;
    SDK_ASSERT(apulu_impl_db()->nacl_idxr()->alloc(&nacl_idx) == SDK_RET_OK);
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry for redirect to "
                      "arm lif %u", id_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // allocate vnic h/w id for this lif
    if ((ret = vnic_impl_db()->vnic_idxr()->alloc(&idx)) != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to allocate vnic h/w id for lif %u, err %u",
                      id_, ret);
        return ret;
    }
    // program the LIF table
    ret = program_lif_table(id_, P4_LIF_TYPE_ARM_DPDK, PDS_IMPL_RSVD_VPC_HW_ID,
                            PDS_IMPL_RSVD_BD_HW_ID, idx, g_zero_mac,
                            false, true, P4_LIF_DIR_HOST);
    if (ret != SDK_RET_OK) {
        goto error;
    }
    return SDK_RET_OK;

error:

    nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    nh_idx_ = 0xFFFFFFFF;
    return ret;
}

sdk_ret_t
lif_impl::install_internal_mgmt_mnic_nacls_(lif_impl *host_mgmt_lif,
                                            uint32_t host_mgmt_lif_nh_idx,
                                            lif_impl *int_mgmt_lif,
                                            uint32_t int_mgmt_lif_nh_idx) {
    p4pd_error_t p4pd_ret;
    nacl_swkey_t key = { 0 };
    nacl_swkey_mask_t mask = { 0 };
    nacl_actiondata_t data =  { 0 };

    // program NACL for host mgmt lif to internal mgmt lif traffic
    key.key_metadata_entry_valid = 1;
    key.capri_intrinsic_lif = host_mgmt_lif->id();
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST_MGMT;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.capri_intrinsic_lif_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_action.nexthop_id = int_mgmt_lif_nh_idx;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL,
                                  PDS_IMPL_NACL_BLOCK_INTERNAL_MGMT_MIN,
                                  &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // program NACL for internal mgmt lif to host mgmt lif traffic
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.capri_intrinsic_lif = int_mgmt_lif->id();
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST_MGMT;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.capri_intrinsic_lif_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_action.nexthop_id = host_mgmt_lif_nh_idx;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL,
                                  PDS_IMPL_NACL_BLOCK_INTERNAL_MGMT_MIN + 1,
                                  &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);
    return SDK_RET_OK;
}

sdk_ret_t
lif_impl::create_internal_mgmt_mnic_(pds_lif_spec_t *spec) {
    uint32_t idx;
    sdk_ret_t ret;
    lif_impl *host_mgmt_lif = NULL, *int_mgmt_lif = NULL;
    nexthop_info_entry_t nexthop_info_entry;

    if (spec->type == sdk::platform::LIF_TYPE_HOST_MGMT) {
        strncpy(name_, spec->name, sizeof(name_));
        PDS_TRACE_DEBUG("Creating host lif %s for naples mgmt", name_);
        host_mgmt_lif = this;
        int_mgmt_lif =
            lif_impl_db()->find(sdk::platform::LIF_TYPE_MNIC_INTERNAL_MGMT);
        if ((int_mgmt_lif == NULL) || (int_mgmt_lif->create_done_ == false)) {
            // other lif is not ready yet
            return SDK_RET_OK;
        }
    } else if (spec->type == sdk::platform::LIF_TYPE_MNIC_INTERNAL_MGMT) {
        strncpy(name_, spec->name, sizeof(name_));
        PDS_TRACE_DEBUG("Creating internal mgmt. lif %s", name_);
        int_mgmt_lif = this;
        host_mgmt_lif = lif_impl_db()->find(sdk::platform::LIF_TYPE_HOST_MGMT);
        if ((host_mgmt_lif == NULL) || (host_mgmt_lif->create_done_ == false)) {
            // other lif is not ready yet
            return SDK_RET_OK;
        }
    }
    PDS_TRACE_DEBUG("Programming NACLs for internal management");
    ret = nexthop_impl_db()->nh_idxr()->alloc_block(&nh_idx_, 2);
    SDK_ASSERT(ret == SDK_RET_OK);

    // program the nexthop for host mgmt. lif to internal mgmt. lif traffic
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.lif = int_mgmt_lif->id();
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    ret = nexthop_info_entry.write(nh_idx_);
    SDK_ASSERT(ret == SDK_RET_OK);

    // program the nexthop for internal mgmt. lif to host mgmt. lif traffic
    nexthop_info_entry.lif = host_mgmt_lif->id();
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    ret = nexthop_info_entry.write(nh_idx_ + 1);
    SDK_ASSERT(ret == SDK_RET_OK);

    ret = install_internal_mgmt_mnic_nacls_(host_mgmt_lif, nh_idx_ + 1,
                                            int_mgmt_lif, nh_idx_);
    SDK_ASSERT(ret == SDK_RET_OK);

    // allocate vnic h/w ids for these lifs
    if ((ret = vnic_impl_db()->vnic_idxr()->alloc_block(&idx, 2)) !=
            SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to allocate vnic h/w id for lif %u, err %u",
                      id_, ret);
        return ret;
    }
    // program the LIF table entry for host mgmt lif
    ret = program_lif_table(host_mgmt_lif->id(), P4_LIF_TYPE_HOST_MGMT,
                            PDS_IMPL_RSVD_VPC_HW_ID, PDS_IMPL_RSVD_BD_HW_ID,
                            idx, g_zero_mac, false, true, P4_LIF_DIR_HOST);
    if (ret != SDK_RET_OK) {
        goto error;
    }

    // program the LIF table entry for host mgmt lif
    ret = program_lif_table(int_mgmt_lif->id(), P4_LIF_TYPE_HOST_MGMT,
                            PDS_IMPL_RSVD_VPC_HW_ID, PDS_IMPL_RSVD_BD_HW_ID,
                            idx + 1, g_zero_mac, false, true, P4_LIF_DIR_HOST);
    if (ret != SDK_RET_OK) {
        goto error;
    }
    return SDK_RET_OK;

error:

    nexthop_impl_db()->nh_idxr()->free(nh_idx_, 2);
    vnic_impl_db()->vnic_idxr()->free(idx, 2);
    nh_idx_ = 0xFFFFFFFF;
    return ret;
}

sdk_ret_t
lif_impl::create_host_lif_(pds_lif_spec_t *spec) {
    uint32_t idx;
    sdk_ret_t ret;
    lif_actiondata_t lif_data = { 0 };

    PDS_TRACE_DEBUG("Creating host lif %u", id_);

    // allocate vnic h/w id for this lif
    if ((ret = vnic_impl_db()->vnic_idxr()->alloc(&idx)) != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to allocate vnic h/w id for lif %u, err %u",
                      id_, ret);
        return ret;
    }
    vnic_hw_id_ = idx;
    // program the LIF table
    ret = program_lif_table(id_, P4_LIF_TYPE_HOST, PDS_IMPL_RSVD_VPC_HW_ID,
                            PDS_IMPL_RSVD_BD_HW_ID, vnic_hw_id_,
                            g_zero_mac, false, true, P4_LIF_DIR_HOST);
    if (unlikely(ret != SDK_RET_OK)) {
        goto error;
    }
    return SDK_RET_OK;

error:

    vnic_impl_db()->vnic_idxr()->free(vnic_hw_id_);
    vnic_hw_id_ = 0xFFFF;
    return ret;
}

sdk_ret_t
lif_impl::create_learn_lif_(pds_lif_spec_t *spec) {
    sdk_ret_t ret;
    nacl_swkey_t key;
    p4pd_error_t p4pd_ret;
    sdk::qos::policer_t policer;
    nacl_swkey_mask_t mask;
    nacl_actiondata_t data;
    uint32_t idx, nacl_idx = PDS_IMPL_NACL_BLOCK_LEARN_MIN;
    nexthop_info_entry_t nexthop_info_entry;

    strncpy(name_, spec->name, sizeof(name_));
    // during hitless upgrade, domain-b uses cpu_mnic3
    if (sdk::platform::sysinit_domain_b(api::g_upg_state->init_domain())) {
        if (!strncmp(name_, "cpu_mnic1", strlen("cpu_mnic1"))) {
            return SDK_RET_OK;
        }
    } else {
        if (!strncmp(name_, "cpu_mnic3", strlen("cpu_mnic3"))) {
            return SDK_RET_OK;
        }
    }
    PDS_TRACE_DEBUG("Creating learn lif %s", name_);
    // allocate required nexthop to point to ARM datapath lif
    ret = nexthop_impl_db()->nh_idxr()->alloc(&nh_idx_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to allocate nexthop entry for learn lif %s, "
                      "id %u, err %u", name_, id_, ret);
        return ret;
    }

    // program the nexthop
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.lif = id_;
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    ret = nexthop_info_entry.write(nh_idx_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program NEXTHOP table for learn lif %u "
                      "at idx %u", id_, nh_idx_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // cap ARP requests from host lifs to 256/sec towards learn lif
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = {
        sdk::qos::POLICER_TYPE_PPS,
        COPP_LEARN_MISS_ARP_REQ_FROM_HOST_PPS,
        COPP_BURST(COPP_LEARN_MISS_ARP_REQ_FROM_HOST_PPS)
    };
    program_copp_entry_(&policer, idx, false);
    // install NACL entry
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_MAC;
    key.control_metadata_learn_enabled = 1;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.control_metadata_local_mapping_miss = 1;
    key.arm_to_p4i_learning_done = 0;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = ETH_TYPE_ARP;
    key.key_metadata_sport = 1;    // ARP request
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_learn_enabled_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_local_mapping_miss_mask = ~0;
    mask.arm_to_p4i_learning_done_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    mask.key_metadata_sport_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_ARP_HOST;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_L2_MISS_ARP;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx++,
                                  &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry for (learn miss, ARP "
                      "requests from host) -> lif %s", name_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // install NACL entry to direct ARP replies to learn lif
    // reusing the policer index allocated for ARP requests
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_MAC;
    key.control_metadata_learn_enabled = 1;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.arm_to_p4i_learning_done = 0;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = ETH_TYPE_ARP;
    key.key_metadata_sport = 2;    // ARP response
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_learn_enabled_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.arm_to_p4i_learning_done_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    mask.key_metadata_sport_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_ARP_HOST;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_ARP_REPLY;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx++,
                                  &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry for (learn probe, ARP "
                      "replies from host) -> lif %s", name_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }
    // install NACL entry to direct RARP packets to learn lif
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_MAC;
    key.control_metadata_learn_enabled = 1;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.arm_to_p4i_learning_done = 0;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = ETH_TYPE_RARP;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_learn_enabled_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.arm_to_p4i_learning_done_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_ARP_HOST;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_L2_MISS_RARP;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx++,
                                  &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL entry for (learn probe, RARP "
                      "packets from host) -> lif %s", name_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // cap DHCP requests from host lifs to 256/sec
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = {
        sdk::qos::POLICER_TYPE_PPS,
        COPP_LEARN_MISS_DHCP_REQ_FROM_HOST_PPS,
        COPP_BURST(COPP_LEARN_MISS_DHCP_REQ_FROM_HOST_PPS)
    };
    program_copp_entry_(&policer, idx, false);
    // install NACL entry for DHCP requests
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_IPV4;
    key.control_metadata_learn_enabled = 1;
    key.arm_to_p4i_nexthop_valid = FALSE;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.arm_to_p4i_learning_done = 0;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = 67;
    key.key_metadata_sport = 68;
    key.key_metadata_proto = 17;    // UDP
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_learn_enabled_mask = ~0;
    mask.arm_to_p4i_nexthop_valid_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.arm_to_p4i_learning_done_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    mask.key_metadata_sport_mask = ~0;
    mask.key_metadata_proto_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class = P4_COPP_DROP_CLASS_DHCP_HOST;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_L2_MISS_DHCP;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx++,
                                  &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL redirect entry for "
                      "(host lif, DHCP req) -> lif %s", name_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    // allocate and program copp table entry for mapping miss
    ret = apulu_impl_db()->copp_idxr()->alloc(&idx);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);
    policer = { sdk::qos::POLICER_TYPE_PPS,
                COPP_LEARN_MISS_IP4_FROM_HOST_PPS,
                COPP_LEARN_MISS_IP4_FROM_HOST_PPS
    };
    program_copp_entry_(&policer, idx, false);

    // redirect mapping miss IPv4 packets from host PFs for datapath learning
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.control_metadata_learn_enabled = 1;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.control_metadata_local_mapping_miss = 1;
    key.arm_to_p4i_learning_done = 0;
    key.control_metadata_tunneled_packet = 0;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.control_metadata_learn_enabled_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_local_mapping_miss_mask = ~0;
    mask.arm_to_p4i_learning_done_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_TO_ARM_ID;
    data.nacl_redirect_to_arm_action.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    data.nacl_redirect_to_arm_action.nexthop_id = nh_idx_;
    data.nacl_redirect_to_arm_action.copp_policer_id = idx;
    data.nacl_redirect_to_arm_action.copp_class =
        P4_COPP_DROP_CLASS_DATAPATH_LEARN;
    data.nacl_redirect_to_arm_action.data = NACL_DATA_ID_L3_MISS_IP4_IP6;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, nacl_idx++,
                                  &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program NACL redirect entry for "
                      "(host lif, TCP) -> lif %s", name_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    SDK_ASSERT(nacl_idx <= PDS_IMPL_NACL_BLOCK_GENERIC_MIN);

    // program the LIF table
    ret = program_lif_table(id_, P4_LIF_TYPE_ARM_LEARN,
                            PDS_IMPL_RSVD_VPC_HW_ID, PDS_IMPL_RSVD_BD_HW_ID,
                            PDS_IMPL_RSVD_VNIC_HW_ID, g_zero_mac, false, true,
                            P4_LIF_DIR_HOST);
    if (ret != SDK_RET_OK) {
        goto error;
    }
    return SDK_RET_OK;

error:

    nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    nh_idx_ = 0xFFFFFFFF;
    return ret;
}

sdk_ret_t
lif_impl::create_control_lif_(pds_lif_spec_t *spec) {
    uint32_t idx;
    sdk_ret_t ret;
    nexthop_info_entry_t nexthop_info_entry;

    strncpy(name_, spec->name, sizeof(name_));
    PDS_TRACE_DEBUG("Creating control lif %s", name_);

    // allocate vnic h/w id for this lif
    if ((ret = vnic_impl_db()->vnic_idxr()->alloc(&idx)) != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to allocate vnic h/w id for lif %u, err %u",
                      id_, ret);
        return ret;
    }
    vnic_hw_id_ = idx;

    // allocate required nexthop to point to lif
    ret = nexthop_impl_db()->nh_idxr()->alloc(&nh_idx_);
    if (ret != SDK_RET_OK) {
        vnic_impl_db()->vnic_idxr()->free(vnic_hw_id_);
        vnic_hw_id_ = 0xFFFF;
        PDS_TRACE_ERR("Failed to allocate nexthop entry for control lif %s, "
                      "id %u, err %u", name_, id_, ret);
        return ret;
    }

    // program the nexthop
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.lif = id_;
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    ret = nexthop_info_entry.write(nh_idx_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program NEXTHOP table for control lif %u "
                      "at idx %u", id_, nh_idx_);
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }

    return SDK_RET_OK;

error:

    vnic_impl_db()->vnic_idxr()->free(vnic_hw_id_);
    vnic_hw_id_ = 0xFFFF;
    nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    nh_idx_ = 0xFFFFFFFF;
    return ret;
}

sdk_ret_t
lif_impl::create(pds_lif_spec_t *spec) {
    sdk_ret_t ret;

    PDS_TRACE_DEBUG("Creating lif %u, key %s, type %u, pinned if 0x%x",
                    spec->id, spec->key.str(), spec->type, spec->pinned_ifidx);
    switch (spec->type) {
    case sdk::platform::LIF_TYPE_MNIC_OOB_MGMT:
        ret = create_oob_mnic_(spec);
        break;
    case sdk::platform::LIF_TYPE_MNIC_INBAND_MGMT:
        ret = create_inb_mnic_(spec);
        break;
    case sdk::platform::LIF_TYPE_MNIC_CPU:
        ret = create_datapath_mnic_(spec);
        break;
    case sdk::platform::LIF_TYPE_HOST_MGMT:
    case sdk::platform::LIF_TYPE_MNIC_INTERNAL_MGMT:
        ret = create_internal_mgmt_mnic_(spec);
        break;
    case sdk::platform::LIF_TYPE_HOST:
        ret = create_host_lif_(spec);
        break;
    case sdk::platform::LIF_TYPE_LEARN:
        ret = create_learn_lif_(spec);
        break;
    case sdk::platform::LIF_TYPE_CONTROL:
        ret = create_control_lif_(spec);
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    if (ret == SDK_RET_OK) {
        create_done_ = true;
    }
    return ret;
}

sdk_ret_t
lif_impl::reset_stats(void) {
    mem_addr_t base_addr, lif_addr;

    base_addr = g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    lif_addr = base_addr + (id_ << LIF_STATS_SIZE_SHIFT);

    sdk::lib::pal_mem_set(lif_addr, 0, 1 << LIF_STATS_SIZE_SHIFT);
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(lif_addr, 1 << LIF_STATS_SIZE_SHIFT,
                                                  P4PLUS_CACHE_INVALIDATE_BOTH);
    sdk::asic::pd::asicpd_p4_invalidate_cache(lif_addr, 1 << LIF_STATS_SIZE_SHIFT,
                                              P4_TBL_CACHE_INGRESS);
    sdk::asic::pd::asicpd_p4_invalidate_cache(lif_addr, 1 << LIF_STATS_SIZE_SHIFT,
                                              P4_TBL_CACHE_EGRESS);
    return SDK_RET_OK;
}

sdk_ret_t
lif_impl::track_pps(uint32_t interval) {
    sdk_ret_t ret;
    lif_metrics_t lif_metrics = { 0 };
    uint64_t curr_tx_pkts, curr_tx_bytes;
    uint64_t curr_rx_pkts, curr_rx_bytes;
    uint64_t tx_pkts, tx_bytes, rx_pkts, rx_bytes;
    mem_addr_t base_addr, lif_addr, write_mem_addr;

    base_addr = g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    lif_addr = base_addr + (id_ << LIF_STATS_SIZE_SHIFT);

    ret = sdk::asic::asic_mem_read(lif_addr, (uint8_t *)&lif_metrics,
                                   sizeof(lif_metrics_t));
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Error reading stats for lif %s hw id %u, err %u",
                      key_.str(), id_, ret);
        return ret;
    }

    tx_pkts  = lif_metrics.tx_pkts;
    tx_bytes = lif_metrics.tx_bytes;
    rx_pkts  = lif_metrics.rx_pkts;
    rx_bytes = lif_metrics.rx_bytes;

    curr_tx_pkts = lif_metrics.tx_ucast_packets +
                       lif_metrics.tx_mcast_packets +
                       lif_metrics.tx_bcast_packets +
                       lif_metrics.tx_ucast_drop_packets +
                       lif_metrics.tx_mcast_drop_packets +
                       lif_metrics.tx_bcast_drop_packets +
                       lif_metrics.tx_rdma_ucast_packets +
                       lif_metrics.tx_rdma_mcast_packets +
                       lif_metrics.tx_rdma_cnp_packets;

    curr_tx_bytes = lif_metrics.tx_ucast_bytes +
                        lif_metrics.tx_mcast_bytes +
                        lif_metrics.tx_bcast_bytes +
                        lif_metrics.tx_ucast_drop_bytes +
                        lif_metrics.tx_mcast_drop_bytes +
                        lif_metrics.tx_bcast_drop_bytes +
                        lif_metrics.tx_rdma_ucast_bytes +
                        lif_metrics.tx_rdma_mcast_bytes;

    curr_rx_pkts = lif_metrics.rx_ucast_packets +
                       lif_metrics.rx_mcast_packets +
                       lif_metrics.rx_bcast_packets +
                       lif_metrics.rx_ucast_drop_packets +
                       lif_metrics.rx_mcast_drop_packets +
                       lif_metrics.rx_bcast_drop_packets +
                       lif_metrics.rx_rdma_ucast_packets +
                       lif_metrics.rx_rdma_mcast_packets +
                       lif_metrics.rx_rdma_cnp_packets +
                       lif_metrics.rx_rdma_ecn_packets;

    curr_rx_bytes = lif_metrics.rx_ucast_bytes +
                        lif_metrics.rx_mcast_bytes +
                        lif_metrics.rx_bcast_bytes +
                        lif_metrics.rx_ucast_drop_bytes +
                        lif_metrics.rx_mcast_drop_bytes +
                        lif_metrics.rx_bcast_drop_bytes +
                        lif_metrics.rx_rdma_ucast_bytes +
                        lif_metrics.rx_rdma_mcast_bytes;

    lif_metrics.tx_pps = RATE_OF_X(tx_pkts, curr_tx_pkts, interval);
    lif_metrics.tx_bytesps = RATE_OF_X(tx_bytes, curr_tx_bytes, interval);
    lif_metrics.rx_pps = RATE_OF_X(rx_pkts, curr_rx_pkts, interval);
    lif_metrics.rx_bytesps = RATE_OF_X(rx_bytes, curr_rx_bytes, interval);
    lif_metrics.tx_pkts = curr_tx_pkts;
    lif_metrics.tx_bytes = curr_tx_bytes;
    lif_metrics.rx_pkts = curr_rx_pkts;
    lif_metrics.rx_bytes = curr_rx_bytes;

    write_mem_addr = lif_addr + LIF_STATS_TX_PKTS_OFFSET;
    ret = sdk::asic::asic_mem_write(write_mem_addr,
                                    (uint8_t *)&lif_metrics.tx_pkts,
                                    PDS_LIF_IMPL_NUM_PPS_STATS *
                                        PDS_LIF_IMPL_COUNTER_SIZE); // 8 fields
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Writing stats for lif %s hw id %u failed, err %u",
                      key_.str(), id_, ret);
    }
    return ret;
}

void
lif_impl::dump_stats(uint32_t fd) {
    sdk_ret_t ret;
    lif_metrics_t stats = { 0 };
    mem_addr_t base_addr, lif_addr;

    base_addr = g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    lif_addr = base_addr + (id_ << LIF_STATS_SIZE_SHIFT);

    ret = sdk::asic::asic_mem_read(lif_addr, (uint8_t *)&stats,
                                   sizeof(lif_metrics_t));
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Error reading stats for lif %s hw id %u, err %u",
                      key_.str(), id_, ret);
        return;
    }

    dprintf(fd, "\nLIF ID: %s HW ID: %u\n", key_.str(), id_);
    dprintf(fd, "%s\n", std::string(54, '-').c_str());
    dprintf(fd, "rx_ucast_bytes                  : %lu\n", stats.rx_ucast_bytes);
    dprintf(fd, "rx_ucast_packets                : %lu\n", stats.rx_ucast_packets);
    dprintf(fd, "rx_mcast_bytes                  : %lu\n", stats.rx_mcast_bytes);
    dprintf(fd, "rx_mcast_packets                : %lu\n", stats.rx_mcast_packets);
    dprintf(fd, "rx_bcast_bytes                  : %lu\n", stats.rx_bcast_bytes);
    dprintf(fd, "rx_bcast_packets                : %lu\n", stats.rx_bcast_packets);
    dprintf(fd, "rx_ucast_drop_bytes             : %lu\n", stats.rx_ucast_drop_bytes);
    dprintf(fd, "rx_ucast_drop_packets           : %lu\n", stats.rx_ucast_drop_packets);
    dprintf(fd, "rx_mcast_drop_bytes             : %lu\n", stats.rx_mcast_drop_bytes);
    dprintf(fd, "rx_mcast_drop_packets           : %lu\n", stats.rx_mcast_drop_packets);
    dprintf(fd, "rx_bcast_drop_bytes             : %lu\n", stats.rx_bcast_drop_bytes);
    dprintf(fd, "rx_bcast_drop_packets           : %lu\n", stats.rx_bcast_drop_packets);
    dprintf(fd, "rx_dma_error                    : %lu\n", stats.rx_dma_error);
    dprintf(fd, "tx_ucast_bytes                  : %lu\n", stats.tx_ucast_bytes);
    dprintf(fd, "tx_ucast_packets                : %lu\n", stats.tx_ucast_packets);
    dprintf(fd, "tx_mcast_bytes                  : %lu\n", stats.tx_mcast_bytes);
    dprintf(fd, "tx_mcast_packets                : %lu\n", stats.tx_mcast_packets);
    dprintf(fd, "tx_bcast_bytes                  : %lu\n", stats.tx_bcast_bytes);
    dprintf(fd, "tx_bcast_packets                : %lu\n", stats.tx_bcast_packets);
    dprintf(fd, "tx_ucast_drop_bytes             : %lu\n", stats.tx_ucast_drop_bytes);
    dprintf(fd, "tx_ucast_drop_packets           : %lu\n", stats.tx_ucast_drop_packets);
    dprintf(fd, "tx_mcast_drop_bytes             : %lu\n", stats.tx_mcast_drop_bytes);
    dprintf(fd, "tx_mcast_drop_packets           : %lu\n", stats.tx_mcast_drop_packets);
    dprintf(fd, "tx_bcast_drop_bytes             : %lu\n", stats.tx_bcast_drop_bytes);
    dprintf(fd, "tx_bcast_drop_packets           : %lu\n", stats.tx_bcast_drop_packets);
    dprintf(fd, "tx_dma_error                    : %lu\n", stats.tx_dma_error);
    dprintf(fd, "rx_queue_disabled               : %lu\n", stats.rx_queue_disabled);
    dprintf(fd, "rx_queue_empty                  : %lu\n", stats.rx_queue_empty);
    dprintf(fd, "rx_queue_error                  : %lu\n", stats.rx_queue_error);
    dprintf(fd, "rx_desc_fetch_error             : %lu\n", stats.rx_desc_fetch_error);
    dprintf(fd, "rx_desc_data_error              : %lu\n", stats.rx_desc_data_error);
    dprintf(fd, "tx_queue_disabled               : %lu\n", stats.tx_queue_disabled);
    dprintf(fd, "tx_queue_error                  : %lu\n", stats.tx_queue_error);
    dprintf(fd, "tx_desc_fetch_error             : %lu\n", stats.tx_desc_fetch_error);
    dprintf(fd, "tx_desc_data_error              : %lu\n", stats.tx_desc_data_error);
    dprintf(fd, "tx_queue_empty                  : %lu\n", stats.tx_queue_empty);
    dprintf(fd, "tx_ucast_drop_bytes             : %lu\n", stats.tx_ucast_drop_bytes);
    dprintf(fd, "tx_ucast_drop_packets           : %lu\n", stats.tx_ucast_drop_packets);
    dprintf(fd, "tx_mcast_drop_bytes             : %lu\n", stats.tx_mcast_drop_bytes);
    dprintf(fd, "tx_mcast_drop_packets           : %lu\n", stats.tx_mcast_drop_packets);
    dprintf(fd, "tx_bcast_drop_bytes             : %lu\n", stats.tx_bcast_drop_bytes);
    dprintf(fd, "tx_bcast_drop_packets           : %lu\n", stats.tx_bcast_drop_packets);
    dprintf(fd, "tx_dma_error                    : %lu\n", stats.tx_dma_error);
    dprintf(fd, "tx_rdma_ucast_bytes             : %lu\n", stats.tx_rdma_ucast_bytes);
    dprintf(fd, "tx_rdma_ucast_packets           : %lu\n", stats.tx_rdma_ucast_packets);
    dprintf(fd, "tx_rdma_mcast_bytes             : %lu\n", stats.tx_rdma_mcast_bytes);
    dprintf(fd, "tx_rdma_mcast_packets           : %lu\n", stats.tx_rdma_mcast_packets);
    dprintf(fd, "tx_rdma_cnp_packets             : %lu\n", stats.tx_rdma_cnp_packets);
    dprintf(fd, "rx_rdma_ucast_bytes             : %lu\n", stats.rx_rdma_ucast_bytes);
    dprintf(fd, "rx_rdma_ucast_packets           : %lu\n", stats.rx_rdma_ucast_packets);
    dprintf(fd, "rx_rdma_mcast_bytes             : %lu\n", stats.rx_rdma_mcast_bytes);
    dprintf(fd, "rx_rdma_mcast_packets           : %lu\n", stats.rx_rdma_mcast_packets);
    dprintf(fd, "rx_rdma_cnp_packets             : %lu\n", stats.rx_rdma_cnp_packets);
    dprintf(fd, "rx_rdma_ecn_packets             : %lu\n", stats.rx_rdma_ecn_packets);
    dprintf(fd, "tx_pkts                         : %lu\n", stats.tx_pkts);
    dprintf(fd, "tx_bytes                        : %lu\n", stats.tx_bytes);
    dprintf(fd, "rx_pkts                         : %lu\n", stats.rx_pkts);
    dprintf(fd, "rx_bytes                        : %lu\n", stats.rx_bytes);
    dprintf(fd, "tx_pps                          : %lu\n", stats.tx_pps);
    dprintf(fd, "tx_bytesps                      : %lu\n", stats.tx_bytesps);
    dprintf(fd, "rx_pps                          : %lu\n", stats.rx_pps);
    dprintf(fd, "rx_bytesps                      : %lu\n", stats.rx_bytesps);
    dprintf(fd, "rdma_packet_seq_err             : %lu\n", stats.rdma_req_rx_pkt_seq_err);
    dprintf(fd, "rdma_req_rnr_retry_err          : %lu\n", stats.rdma_req_rx_rnr_retry_err);
    dprintf(fd, "rdma_req_remote_access_err      : %lu\n", stats.rdma_req_rx_remote_access_err);
    dprintf(fd, "rdma_req_remote_inv_req_err     : %lu\n", stats.rdma_req_rx_remote_inv_req_err);
    dprintf(fd, "rdma_req_remote_oper_err        : %lu\n", stats.rdma_req_rx_remote_oper_err);
    dprintf(fd, "rdma_implied_nak_seq_err        : %lu\n", stats.rdma_req_rx_implied_nak_seq_err);
    dprintf(fd, "rdma_req_cqe_err                : %lu\n", stats.rdma_req_rx_cqe_err);
    dprintf(fd, "rdma_req_cqe_flush_err          : %lu\n", stats.rdma_req_rx_cqe_flush_err);
    dprintf(fd, "rdma_duplicate_responses        : %lu\n", stats.rdma_req_rx_dup_responses);
    dprintf(fd, "rdma_req_invalid_packets        : %lu\n", stats.rdma_req_rx_invalid_packets);
    dprintf(fd, "rdma_req_local_access_err       : %lu\n", stats.rdma_req_tx_local_access_err);
    dprintf(fd, "rdma_req_local_oper_err         : %lu\n", stats.rdma_req_tx_local_oper_err);
    dprintf(fd, "rdma_req_memory_mgmt_err        : %lu\n", stats.rdma_req_tx_memory_mgmt_err);
    dprintf(fd, "rdma_duplicate_request          : %lu\n", stats.rdma_resp_rx_dup_requests);
    dprintf(fd, "rdma_out_of_buffer              : %lu\n", stats.rdma_resp_rx_out_of_buffer);
    dprintf(fd, "rdma_out_of_sequence            : %lu\n", stats.rdma_resp_rx_out_of_seq_pkts);
    dprintf(fd, "rdma_resp_cqe_err               : %lu\n", stats.rdma_resp_rx_cqe_err);
    dprintf(fd, "rdma_resp_cqe_flush_err         : %lu\n", stats.rdma_resp_rx_cqe_flush_err);
    dprintf(fd, "rdma_resp_local_len_err         : %lu\n", stats.rdma_resp_rx_local_len_err);
    dprintf(fd, "rdma_resp_inv_request_err       : %lu\n", stats.rdma_resp_rx_inv_request_err);
    dprintf(fd, "rdma_resp_local_qp_oper_err     : %lu\n", stats.rdma_resp_rx_local_qp_oper_err);
    dprintf(fd, "rdma_out_of_atomic_resource     : %lu\n", stats.rdma_resp_rx_out_of_atomic_resource);
    dprintf(fd, "rdma_resp_pkt_seq_err           : %lu\n", stats.rdma_resp_tx_pkt_seq_err);
    dprintf(fd, "rdma_resp_remote_inv_req_err    : %lu\n", stats.rdma_resp_tx_remote_inv_req_err);
    dprintf(fd, "rdma_resp_remote_access_err     : %lu\n", stats.rdma_resp_tx_remote_access_err);
    dprintf(fd, "rdma_resp_remote_oper_err       : %lu\n", stats.rdma_resp_tx_remote_oper_err);
    dprintf(fd, "rdma_resp_rnr_retry_err         : %lu\n", stats.rdma_resp_tx_rnr_retry_err);
}

/// \@}

}    // namespace impl
}    // namespace api
