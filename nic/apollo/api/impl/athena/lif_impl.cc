//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of lif
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/catalog/catalog.hpp"
#include "nic/sdk/asic/pd/scheduler.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/impl/athena/pds_impl_state.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"
#include "nic/apollo/api/impl/athena/athena_impl.hpp"
#include "nic/p4/common/defines.h"
#include "gen/p4gen/p4plus_txdma/include/p4plus_txdma_p4pd.h"
#include "gen/p4gen/athena/include/p4pd.h"

namespace api {
namespace impl {

lif_impl *
lif_impl::factory (pds_lif_spec_t *spec) {
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
    ifindex_ = LIF_IFINDEX(id_);
    create_done_ = false;
    nh_idx_ = 0xFFFFFFFF;
    vnic_hw_id_ = 0xFFFF;
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
    lif_sched_params.tx_sched_num_table_entries = spec->tx_sched_num_table_entries;
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
    lif_sched_params.tx_sched_num_table_entries = spec->tx_sched_num_table_entries;
    ret = sdk::asic::pd::asicpd_tx_scheduler_map_alloc(&lif_sched_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to alloc tx sched. lif %s, err %u",
                      spec->key.str(), ret);
        return ret;
    }
    // save allocation in spec
    spec->tx_sched_table_offset = lif_sched_params.tx_sched_table_offset;
    spec->tx_sched_num_table_entries = lif_sched_params.tx_sched_num_table_entries;

    // program tx scheduler and policer
    ret = sdk::asic::pd::asicpd_tx_scheduler_map_program(&lif_sched_params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program tx sched. lif %u, err %u",
                      spec->id, ret);
        return ret;
    }
    return ret;
}

#define  lif_egress_rl_params \
    action_u.tx_table_s5_t4_lif_rate_limiter_table_tx_stage5_lif_egress_rl_params
sdk_ret_t
lif_impl::program_tx_policer(sdk::qos::policer_t *policer) {
    sdk_ret_t ret;
    tx_table_s5_t4_lif_rate_limiter_table_actiondata_t rlimit_data = { 0 };
    uint64_t refresh_interval_us = SDK_DEFAULT_POLICER_REFRESH_INTERVAL;
    uint64_t rate_tokens = 0, burst_tokens = 0, rate;

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
        ret = sdk::qos::policer_to_token_rate(policer, refresh_interval_us,
                                         SDK_MAX_POLICER_TOKENS_PER_INTERVAL,
                                         &rate_tokens, &burst_tokens);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Error converting rate to token rate, err %u", ret);
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
        SDK_TRACE_ERR("LIF_TX_POLICER table write failure, lif %u, err %u",
                      id_, ret);
        return ret;
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
    pds_event_t event;

    // udpate the admin state
    admin_state_ = state;
}

#define nacl_redirect_action    action_u.nacl_nacl_redirect
sdk_ret_t
lif_impl::create_oob_mnic_(pds_lif_spec_t *spec) {
    sdk_ret_t              ret;
    nacl_swkey_t           key = { 0 };
    nacl_swkey_mask_t      mask = { 0 };
    nacl_actiondata_t      data =  { 0 };
    sdk_table_api_params_t tparams;
    uint32_t               idx;

    // ARM -> uplink
    key.capri_intrinsic_lif = id_;
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport =
        g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program NACL entry for mnic lif %u -> "
                      "uplink 0x%x, err %u", id_, pinned_if_idx_, ret);
        return ret;
    }

    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    // uplink -> ARM
    key.capri_intrinsic_lif =
        sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_);
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = id_;
    //data.nacl_redirect_action.vlan_strip = spec->vlan_strip_en;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program NACL entry for uplink %u -> mnic "
                      "lif %u, err %u", pinned_if_idx_, id_, ret);
    }
    return ret;
}

sdk_ret_t
lif_impl::create_datapath_mnic_(pds_lif_spec_t *spec) {
    sdk_ret_t              ret;
    nacl_swkey_t           key = { 0 };
    nacl_swkey_mask_t      mask = { 0 };
    nacl_actiondata_t      data =  { 0 };
    sdk_table_api_params_t tparams = {0};

    PDS_TRACE_ERR("create_datapath_mnic_\n");

    // ARM -> uplink
    key.capri_intrinsic_lif = id_;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport =
        g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    tparams.highest = true;
    ret = athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program CPU-redirect NACL entry for mnic lif %u -> "
                      "uplink 0x%x, err %u\n", id_, pinned_if_idx_, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("NACL ARM->Uplink: LIF: 0x%x -> Uplink: %d\n",
                id_, data.nacl_redirect_action.oport);
    }

    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    // uplink -> ARM on flow miss
    key.control_metadata_flow_miss = 1;
    mask.control_metadata_flow_miss_mask = 1;
    key.capri_intrinsic_lif =
        sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_);
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = id_;
    //data.nacl_redirect_action.vlan_strip = spec->vlan_strip_en;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    tparams.highest = true;
    ret = athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program CPU-redirect NACL entry for uplink %u (key %u) -> "
                      "mnic lif %u, err %u\n", pinned_if_idx_, key.capri_intrinsic_lif,
                      id_, ret);
    }
    else {
        PDS_TRACE_DEBUG("NACL Uplink->ARM: Uplink LIF: 0x%x -> LIF: 0x%x\n",
                key.capri_intrinsic_lif, id_);
    }

    return ret;
}

typedef struct lif_internal_mgmt_ctx_s {
    lif_impl **lif;
    lif_type_t type;
} __PACK__ lif_internal_mgmt_ctx_t;

static bool
lif_internal_mgmt_cb_(void *api_obj, void *ctxt) {
    lif_impl *lif = (lif_impl *)api_obj;
    lif_internal_mgmt_ctx_t *cb_ctx = (lif_internal_mgmt_ctx_t *)ctxt;

    if (lif->type() == cb_ctx->type) {
        *cb_ctx->lif = lif;
        return true;
    }
    return false;
}

sdk_ret_t
lif_impl::create_internal_mgmt_mnic_(pds_lif_spec_t *spec) {
    uint32_t idx;
    sdk_ret_t ret;
    nacl_swkey_t key = { 0 };
    nacl_swkey_mask_t mask = { 0 };
    nacl_actiondata_t data =  { 0 };
    lif_impl *host_mgmt_lif = NULL, *mnic_int_mgmt_lif = NULL;
    lif_internal_mgmt_ctx_t cb_ctx = {0};
    sdk_table_api_params_t  tparams;

    if (spec->type == sdk::platform::LIF_TYPE_HOST_MGMT) {
        host_mgmt_lif = this;
        cb_ctx.type = sdk::platform::LIF_TYPE_MNIC_INTERNAL_MGMT;
        cb_ctx.lif = &mnic_int_mgmt_lif;
    } else if (spec->type == sdk::platform::LIF_TYPE_MNIC_INTERNAL_MGMT) {
        mnic_int_mgmt_lif = this;
        cb_ctx.type = sdk::platform::LIF_TYPE_HOST_MGMT;
        cb_ctx.lif = &host_mgmt_lif;
    }
    lif_impl_db()->walk(lif_internal_mgmt_cb_, &cb_ctx);

    if (!(host_mgmt_lif && mnic_int_mgmt_lif &&
          host_mgmt_lif->create_done_ && mnic_int_mgmt_lif->create_done_)) {
        // we will program when both lifs are available and initialized properly
        // to avoid inserting same entry twice
        return SDK_RET_OK;
    }

    PDS_TRACE_DEBUG("programming nacls for internal management:");
    // program host_mgmt -> mnic_int_mgmt
    key.capri_intrinsic_lif = host_mgmt_lif->id();
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = mnic_int_mgmt_lif->id();
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed NACL entry for host_mgmt -> mnic_int_mgmt "
                      "err %u host-lif:%u int-mgmt-lif:%u ", ret, key.capri_intrinsic_lif, mnic_int_mgmt_lif->id());
    }

    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    memset(&tparams, 0, sizeof(tparams));

    // program mnic_int_mgmt -> host_mgmt
    key.capri_intrinsic_lif = mnic_int_mgmt_lif->id();
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = host_mgmt_lif->id();
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed NACL entry for mnic_int_mgmt -> host_mgmt"
                      "err %u int-mgmt-lif:%u host-lif:%u ", ret, key.capri_intrinsic_lif, mnic_int_mgmt_lif->id());
    }
    return ret;
}

typedef struct lif_p2p_ctx_s {
    lif_impl        **lif;
    lif_type_t      type;
    pds_lif_id_t    id;
} __PACK__ lif_p2p_ctx_t;

static bool
lif_p2p_cb_(void *api_obj, void *ctxt) {
    lif_impl *lif = (lif_impl *)api_obj;
    lif_p2p_ctx_t *cb_ctx = (lif_p2p_ctx_t *)ctxt;

    if ((lif->type() == cb_ctx->type) &&
            (lif->id() == cb_ctx->id)) {
        *cb_ctx->lif = lif;
        return true;
    }
    return false;
}

sdk_ret_t
lif_impl::create_p2p_mnic_(pds_lif_spec_t *spec) {
    uint32_t idx;
    sdk_ret_t ret;
    nacl_swkey_t key = { 0 };
    nacl_swkey_mask_t mask = { 0 };
    nacl_actiondata_t data =  { 0 };
    lif_impl *this_p2p_lif = NULL, *peer_p2p_lif = NULL;
    lif_p2p_ctx_t   cb_ctx = {0};
    sdk_table_api_params_t  tparams;

    this_p2p_lif = this;
    cb_ctx.type = sdk::platform::LIF_TYPE_P2P;
    cb_ctx.id = spec->peer_lif_id;
    cb_ctx.lif = &peer_p2p_lif;
    lif_impl_db()->walk(lif_p2p_cb_, &cb_ctx);

    if (!(this_p2p_lif && peer_p2p_lif &&
          this_p2p_lif->create_done_ && peer_p2p_lif->create_done_)) {
        // we will program when both lifs are available and initialized properly
        // to avoid inserting same entry twice
        return SDK_RET_OK;
    }

    PDS_TRACE_DEBUG("programming nacls for P2P Pair:");
    key.capri_intrinsic_lif = this_p2p_lif->id();
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = peer_p2p_lif->id();
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed NACL entry for P2P intf"
                      "err %u lif-1:%u lif-2:%u ", ret, key.capri_intrinsic_lif, peer_p2p_lif->id());
    }

    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    memset(&tparams, 0, sizeof(tparams));

    key.capri_intrinsic_lif = peer_p2p_lif->id();
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = this_p2p_lif->id();
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed NACL entry for P2P intf"
                      "err %u lif-1:%u lif-2:%u ", ret, key.capri_intrinsic_lif, this_p2p_lif->id());
    }
    return ret;
}

sdk_ret_t
lif_impl::create(pds_lif_spec_t *spec) {
    sdk_ret_t ret = SDK_RET_OK;

    switch (spec->type) {
    case sdk::platform::LIF_TYPE_MNIC_OOB_MGMT:
        ret = create_oob_mnic_(spec);
        break;
    case sdk::platform::LIF_TYPE_MNIC_INBAND_MGMT:
        /* The support for inband mgmt interface uses the same mechanism as
         * datapath MNICs. Hence the device.json should never contain
         * both the inband mgmt MNICs and datapath MNICs at the same time.
         */
        ret = create_datapath_mnic_(spec);
        break;
    case sdk::platform::LIF_TYPE_MNIC_CPU:
        ret = create_datapath_mnic_(spec);
        break;
    case sdk::platform::LIF_TYPE_HOST_MGMT:
    case sdk::platform::LIF_TYPE_MNIC_INTERNAL_MGMT:
        create_done_ = true;
        ret = create_internal_mgmt_mnic_(spec);
        if (ret != SDK_RET_OK) {
            create_done_ = false;
        }
        break;
    case sdk::platform::LIF_TYPE_SERVICE:
        break;
    case sdk::platform::LIF_TYPE_P2P:
        create_done_ = true;
        ret = create_p2p_mnic_(spec);
        if (ret != SDK_RET_OK) {
            create_done_ = false;
        }
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    return ret;
}

sdk_ret_t
lif_impl::reset_stats(void) {
    return SDK_RET_ERR;
}

typedef struct lif_datapath_mnic_ctx_s {
    lif_impl **lif_0;
    lif_impl **lif_1;
    lif_type_t type;
} __PACK__ lif_datapath_mnic_ctx_t;

static bool
lif_datapath_mnic_cb_(void *api_obj, void *ctxt) {
    lif_impl *lif = (lif_impl *)api_obj;
    lif_datapath_mnic_ctx_t *cb_ctx = (lif_datapath_mnic_ctx_t *)ctxt;

    PDS_TRACE_DEBUG("lif cb called for lif type %u id %u ifidx %u", lif->type(),
                    lif->id(), lif->pinned_ifindex());
    if (lif->type() == cb_ctx->type) {
        if (*cb_ctx->lif_0 == NULL) {
            *cb_ctx->lif_0 = lif;
        } else {
            *cb_ctx->lif_1 = lif;
            return true;
        }
    }
    return false;
}

}    // namespace impl
}    // namespace api

sdk_ret_t
athena_program_mfr_nacls(uint16_t vlan_a, uint16_t vlan_b) {

    sdk_ret_t ret = SDK_RET_OK;
    api::impl::lif_datapath_mnic_ctx_t cb_ctx = {0};
    api::impl::lif_impl *dp_mnic0 = NULL, *dp_mnic1 = NULL;
    pds_lif_id_t mnic0_id, mnic1_id;
    if_index_t pinned_if_idx_mnic0, pinned_if_idx_mnic1;
    nacl_swkey_t           key = { 0 };
    nacl_swkey_mask_t      mask = { 0 };
    nacl_actiondata_t      data =  { 0 };
    sdk_table_api_params_t tparams;
    std::array<uint16_t, 2> vlans = {vlan_a, vlan_b};
    uint32_t hw_lif_a, hw_lif_b, hw_oport_a, hw_oport_b;

    cb_ctx.type = sdk::platform::LIF_TYPE_MNIC_CPU;
    cb_ctx.lif_0 = &dp_mnic0;
    cb_ctx.lif_1 = &dp_mnic1;

    api::impl::lif_impl_db()->walk(api::impl::lif_datapath_mnic_cb_, &cb_ctx);

    if(dp_mnic0 == NULL || dp_mnic1 == NULL) {
        PDS_TRACE_ERR("Failed to fetch dp mnic info: dp_mnic0 0x%p, dp_mnic1 0x%p",
                        dp_mnic0, dp_mnic1);
        return SDK_RET_OK;
    }

    mnic0_id = dp_mnic0->id();
    pinned_if_idx_mnic0 = dp_mnic0->pinned_ifindex();
    mnic1_id = dp_mnic1->id();
    pinned_if_idx_mnic1 = dp_mnic1->pinned_ifindex();

    PDS_TRACE_DEBUG("data path mnic lif info: dp_mnic0 lif %u uplink 0x%x, "
                    "dp_mnic1 lif %u uplink 0x%x", mnic0_id,
                    pinned_if_idx_mnic0, mnic1_id,
                    pinned_if_idx_mnic1);

    //NACLs to redirect traffic with specific vlans to ARM
    hw_lif_a = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic0);
    hw_lif_b = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic1);

    key.capri_intrinsic_lif = hw_lif_a;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = mnic0_id;

    for (int idx = 0; idx != vlans.size(); ++idx) {
        key.vlan = vlans[idx];
        mask.vlan_mask = 0xFFFF;

        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                       NACL_NACL_REDIRECT_ID,
                                       sdk::table::handle_t::null());
        ret = api::impl::athena_impl_db()->nacl_tbl()->insert(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program vlan redirect NACL for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_a,
                    mnic0_id, vlans[idx], ret);
            return ret;

        } else {
            PDS_TRACE_DEBUG("Vlan redirect nacl programmed for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_a,
                    mnic0_id, vlans[idx], ret);
        }
    }

    key.capri_intrinsic_lif = hw_lif_b;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = mnic1_id;

    for (int idx = 0; idx != vlans.size(); ++idx) {
        key.vlan = vlans[idx];
        mask.vlan_mask = 0xFFFF;

        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                       NACL_NACL_REDIRECT_ID,
                                       sdk::table::handle_t::null());
        ret = api::impl::athena_impl_db()->nacl_tbl()->insert(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program vlan redirect NACL for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_b,
                    mnic1_id, vlans[idx], ret);
            return ret;

        } else {
            PDS_TRACE_DEBUG("Vlan redirect nacl programmed for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_b,
                    mnic1_id, vlans[idx], ret);
        }
    }

    //NACLs to redirect traffic between uplinks
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    hw_oport_a =
        api::g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_mnic0);
    hw_oport_b =
        api::g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_mnic1);

    key.capri_intrinsic_lif = hw_lif_a;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = hw_oport_b;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = api::impl::athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program redirect nacl from uplink lif %u "
                      "to uplink lif %u, err 0x%x\n", hw_lif_a, hw_lif_b, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("Nacl programmed to redirect traffic from uplink "
                "lif %u to uplink lif %u\n", hw_lif_a, hw_lif_b);
    }

    key.capri_intrinsic_lif = hw_lif_b;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = hw_oport_a;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = api::impl::athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program redirect nacl from uplink lif %u "
                      "to uplink lif %u, err 0x%x\n", hw_lif_b, hw_lif_a, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("Nacl programmed to redirect traffic from uplink "
                "lif %u to uplink lif %u\n", hw_lif_b, hw_lif_a);
    }

    return SDK_RET_OK;
}

sdk_ret_t
athena_unprogram_mfr_vlan_rdt_nacls(uint16_t vlan_a, uint16_t vlan_b) {

    sdk_ret_t ret = SDK_RET_OK;
    api::impl::lif_datapath_mnic_ctx_t cb_ctx = {0};
    api::impl::lif_impl *dp_mnic0 = NULL, *dp_mnic1 = NULL;
    pds_lif_id_t mnic0_id, mnic1_id;
    if_index_t pinned_if_idx_mnic0, pinned_if_idx_mnic1;
    nacl_swkey_t           key = { 0 };
    nacl_swkey_mask_t      mask = { 0 };
    nacl_actiondata_t      data =  { 0 };
    sdk_table_api_params_t tparams;
    std::array<uint16_t, 2> vlans = {vlan_a, vlan_b};
    uint32_t hw_lif_a, hw_lif_b;

    cb_ctx.type = sdk::platform::LIF_TYPE_MNIC_CPU;
    cb_ctx.lif_0 = &dp_mnic0;
    cb_ctx.lif_1 = &dp_mnic1;

    api::impl::lif_impl_db()->walk(api::impl::lif_datapath_mnic_cb_, &cb_ctx);

    if(dp_mnic0 == NULL || dp_mnic1 == NULL) {
        PDS_TRACE_ERR("Failed to fetch dp mnic info: dp_mnic0 0x%p, dp_mnic1 0x%p",
                        dp_mnic0, dp_mnic1);
        return SDK_RET_OK;
    }

    mnic0_id = dp_mnic0->id();
    pinned_if_idx_mnic0 = dp_mnic0->pinned_ifindex();
    mnic1_id = dp_mnic1->id();
    pinned_if_idx_mnic1 = dp_mnic1->pinned_ifindex();

    PDS_TRACE_DEBUG("data path mnic lif info: dp_mnic0 lif %u uplink 0x%x, "
                    "dp_mnic1 lif %u uplink 0x%x", mnic0_id,
                    pinned_if_idx_mnic0, mnic1_id,
                    pinned_if_idx_mnic1);

    //Remove NACLs that redirect traffic with specific vlans to ARM
    hw_lif_a = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic0);
    hw_lif_b = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic1);

    key.capri_intrinsic_lif = hw_lif_a;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = mnic0_id;

    for (int idx = 0; idx != vlans.size(); ++idx) {
        key.vlan = vlans[idx];
        mask.vlan_mask = 0xFFFF;

        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                       NACL_NACL_REDIRECT_ID,
                                       sdk::table::handle_t::null());
        ret = api::impl::athena_impl_db()->nacl_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remove vlan redirect NACL for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_a,
                    mnic0_id, vlans[idx], ret);
            return ret;

        } else {
            PDS_TRACE_DEBUG("Vlan redirect nacl removed for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_a,
                    mnic0_id, vlans[idx], ret);
        }
    }

    key.capri_intrinsic_lif = hw_lif_b;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = TM_PORT_DMA;
    data.nacl_redirect_action.lif = mnic1_id;

    for (int idx = 0; idx != vlans.size(); ++idx) {
        key.vlan = vlans[idx];
        mask.vlan_mask = 0xFFFF;

        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                       NACL_NACL_REDIRECT_ID,
                                       sdk::table::handle_t::null());
        ret = api::impl::athena_impl_db()->nacl_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remove vlan redirect NACL for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_b,
                    mnic1_id, vlans[idx], ret);
            return ret;

        } else {
            PDS_TRACE_DEBUG("Vlan redirect nacl removed for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_b,
                    mnic1_id, vlans[idx], ret);
        }
    }

    return SDK_RET_OK;
}

sdk_ret_t
athena_unprogram_mfr_up2up_nacls()
{
    sdk_ret_t ret = SDK_RET_OK;
    api::impl::lif_datapath_mnic_ctx_t cb_ctx = {0};
    api::impl::lif_impl *dp_mnic0 = NULL, *dp_mnic1 = NULL;
    pds_lif_id_t mnic0_id, mnic1_id;
    if_index_t pinned_if_idx_mnic0, pinned_if_idx_mnic1;
    nacl_swkey_t           key = { 0 };
    nacl_swkey_mask_t      mask = { 0 };
    nacl_actiondata_t      data =  { 0 };
    sdk_table_api_params_t tparams;
    uint32_t hw_lif_a, hw_lif_b, hw_oport_a, hw_oport_b;

    cb_ctx.type = sdk::platform::LIF_TYPE_MNIC_CPU;
    cb_ctx.lif_0 = &dp_mnic0;
    cb_ctx.lif_1 = &dp_mnic1;

    api::impl::lif_impl_db()->walk(api::impl::lif_datapath_mnic_cb_, &cb_ctx);

    if(dp_mnic0 == NULL || dp_mnic1 == NULL) {
        PDS_TRACE_ERR("Failed to fetch dp mnic info: dp_mnic0 0x%p, dp_mnic1 0x%p",
                        dp_mnic0, dp_mnic1);
        return SDK_RET_OK;
    }

    mnic0_id = dp_mnic0->id();
    pinned_if_idx_mnic0 = dp_mnic0->pinned_ifindex();
    mnic1_id = dp_mnic1->id();
    pinned_if_idx_mnic1 = dp_mnic1->pinned_ifindex();

    PDS_TRACE_DEBUG("data path mnic lif info: dp_mnic0 lif %u uplink 0x%x, "
                    "dp_mnic1 lif %u uplink 0x%x", mnic0_id,
                    pinned_if_idx_mnic0, mnic1_id,
                    pinned_if_idx_mnic1);

    //Remove NACLs that redirect traffic between uplinks
    hw_lif_a = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic0);
    hw_lif_b = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic1);

    hw_oport_a =
        api::g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_mnic0);
    hw_oport_b =
        api::g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_mnic1);

    key.capri_intrinsic_lif = hw_lif_a;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = hw_oport_b;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = api::impl::athena_impl_db()->nacl_tbl()->remove(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to remove redirect nacl from uplink lif %u "
                      "to uplink lif %u, err 0x%x\n", hw_lif_a, hw_lif_b, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("Nacl to redirect traffic from uplink "
                "lif %u to uplink lif %u removed\n", hw_lif_a, hw_lif_b);
    }

    key.capri_intrinsic_lif = hw_lif_b;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.nacl_redirect_action.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.nacl_redirect_action.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.nacl_redirect_action.oport = hw_oport_a;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = api::impl::athena_impl_db()->nacl_tbl()->remove(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to remove redirect nacl from uplink lif %u "
                      "to uplink lif %u, err 0x%x\n", hw_lif_b, hw_lif_a, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("Nacl to redirect traffic from uplink "
                "lif %u to uplink lif %u removed\n", hw_lif_b, hw_lif_a);
    }

    return SDK_RET_OK;
}
