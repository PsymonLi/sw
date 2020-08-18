//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// learn global state
///
//----------------------------------------------------------------------------

#include <unistd.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/dpdk/dpdk.hpp"
#include "nic/sdk/lib/rte_indexer/rte_indexer.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/learn/learn_impl_base.hpp"
#include "nic/apollo/learn/learn_state.hpp"
#include "nic/apollo/learn/learn_uio.hpp"

namespace learn {

learn_state::learn_state() {

    // initialize to default values
    epoch_ = LEARN_API_EPOCH_START;
    vnic_objid_idxr_ = nullptr;
    learn_lif_ = nullptr;
    ep_mac_state_ = nullptr;
    ep_ip_state_ = nullptr;
    learn_thread_ready_ = false;
    pend_ntfn_state_.slab = nullptr;

    memset(&counters_, 0, sizeof(counters_));

    // default timeout/age values
    arp_probe_timeout_secs_ = LEARN_EP_ARP_PROBE_TIMEOUT_SEC;
    pkt_poll_interval_msecs_ = LEARN_PKT_POLL_INTERVAL_MSEC;
}

void
learn_state::release_resources_(pds_learn_mode_t mode)
{
    // if mode is none release all resources, else, release
    // mode specific resources only

    // auto mode resources
    if (mode == PDS_LEARN_MODE_AUTO || mode == PDS_LEARN_MODE_NONE) {
        if (vnic_objid_idxr_) {
            rte_indexer::destroy(vnic_objid_idxr_);
            vnic_objid_idxr_ = nullptr;
        }
        pending_ntfn_state_release(&pend_ntfn_state_);
    }

    // TODO: notify mode cleanup once operdb is enabled

    // release common resources
    if (mode == PDS_LEARN_MODE_NONE) {
        if (ep_mac_state_) {
            ep_mac_state_->~ep_mac_state();
            ep_mac_state_ = nullptr;
        }
        if (ep_ip_state_) {
            ep_ip_state_->~ep_ip_state();
            ep_ip_state_ = nullptr;
        }
        if (learn_lif_) {
            dpdk_device::destroy(learn_lif_);
            learn_lif_ = nullptr;
        }
    }
}

learn_state::~learn_state() {
    release_resources_();
}

sdk_ret_t
learn_state::lif_init_(void) {
    sdk_ret_t ret;
    sdk_dpdk_params_t params;
    sdk_dpdk_device_params_t args;
    const char *lif_name = impl::learn_lif_name();
    int count = 0;
    std::string eal_init_list = "-n 4 --master-lcore 1 -c 3";
    sysinit_dom_t dom = sdk::upg::init_domain();

    SDK_ASSERT(lif_name);
    // wait for uio devices to come up
    while (!uio_device_ready()) {
        if (count >= UIO_DEV_SCAN_MAX_RETRY) {
            PDS_TRACE_ERR( "Failed to initialize learn lif, UIO device not"
                           "found");
            return SDK_RET_ENTRY_NOT_FOUND;
        }
        if (0 == (count % 30)) {
            PDS_TRACE_INFO("UIO device not created yet, retry count %d", count);
        }
        sleep(UIO_DEV_SCAN_INTERVAL);
        count++;
    }
    PDS_TRACE_INFO("UIO device created, retry count %d", count);

    if (sdk::platform::sysinit_domain_b(dom)) {
        eal_init_list += " --file-prefix learn_dom_b";
    } else {
        eal_init_list += " --file-prefix learn";
    }

    params.eal_init_list = eal_init_list.c_str();
    params.log_file_name = dpdk_log_file_name();
    params.mbuf_pool_name = "learn_dpdk";
    params.mbuf_size = LEARN_LIF_PKT_BUF_SZ;
    params.num_mbuf = LEARN_LIF_MBUF_COUNT;
    params.vdev_list.push_back(string(lif_name));
    ret = dpdk_init(&params);
    if (unlikely(ret != SDK_RET_OK)) {
        // learn lif may not be present on all pipelines, so exit
        // on dpdk init failure.
        PDS_TRACE_ERR("DPDK init failed with return code %u", ret);
        return ret;
    }

    args.dev_name = lif_name;
    args.num_rx_desc = LEARN_LIF_RX_DESC_COUNT;
    args.num_rx_queue = 1;
    args.num_tx_desc = LEARN_LIF_TX_DESC_COUNT;
    args.num_tx_queue = 1;
    learn_lif_ = dpdk_device::factory(&args);
    if (unlikely(learn_lif_ == nullptr)) {
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
learn_state::init(void) {
    // instantiate MAC and IP states
    operd_region_ = nullptr;
    vnic_objid_idxr_ = nullptr;
    sdk_ret_t ret;

    ret = lif_init_();
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ep_mac_state_ = new ep_mac_state();
    ep_ip_state_ = new ep_ip_state();
    if (ep_mac_state_ == nullptr || ep_ip_state_ == nullptr) {
        release_resources_();
        return SDK_RET_OOM;
    }
    learn_thread_ready_ = true;
    return SDK_RET_OK;
}

sdk_ret_t
learn_state::init_oper_mode(pds_learn_mode_t mode) {
    sdk_ret_t ret = SDK_RET_OK;

    switch (mode) {
    case PDS_LEARN_MODE_AUTO:
        PDS_TRACE_INFO("Enabling learning in auto mode");
        vnic_objid_idxr_ = rte_indexer::factory(PDS_MAX_VNIC, false, true);
        if (vnic_objid_idxr_ == nullptr) {
            return SDK_RET_ERR;
        }
        ret = pending_ntfn_state_init(&pend_ntfn_state_);
        break;

    case PDS_LEARN_MODE_NOTIFY:
        PDS_TRACE_INFO("Enabling learning in notify mode");
        // TODO: commenting out operd init
        // operd_region_ = learn_operd_init();
        break;

    case PDS_LEARN_MODE_NONE:
        // this is hit if learning is disabled
    default:
        break;
    }
    if (ret != SDK_RET_OK) {
        release_resources_(mode);
    }
    return ret;
}

uint32_t
learn_state::ep_timeout(void) const {
    uint32_t age;
    age = device_db()->find()->learn_age_timeout();
    if (age == 0) {
        age = LEARN_EP_DEFAULT_AGE_SEC;
    }
    return age;
}

sdk_ret_t
learn_state::walk(state_walk_cb_t walk_cb, void *user_data) {
    sdk_ret_t ret;

    if (!learn_thread_ready_) {
        return SDK_RET_OK;
    }
    ret = ep_mac_state_->walk(walk_cb, user_data);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    return ep_ip_state_->walk(walk_cb, user_data);
}

sdk_ret_t
learn_state::slab_walk(state_walk_cb_t walk_cb, void *user_data) {
    sdk_ret_t ret;

    if (!learn_thread_ready_) {
        return SDK_RET_OK;
    }
    ret = ep_mac_state_->slab_walk(walk_cb, user_data);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = ep_ip_state_->slab_walk(walk_cb, user_data);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    return SDK_RET_OK;
}

}    // namepsace learn
