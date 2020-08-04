//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines upgrade state for PDS
///
//----------------------------------------------------------------------------

#ifndef __API_UPGRADE_STATE_HPP__
#define __API_UPGRADE_STATE_HPP__

#include <list>
#include <unordered_map>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/shmstore/shmstore.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/apollo/api/include/pds_upgrade.hpp"
#include "nic/apollo/api/include/pds_init.hpp"
#include "nic/apollo/api/internal/upgrade_ev.hpp"
#include "nic/apollo/api/internal/upgrade.hpp"
#include "nic/apollo/api/internal/upgrade_pstate.hpp"

namespace api {

/// \brief qstate info
typedef struct qstate_cfg_s {
    uint64_t addr;
    uint32_t size;
    uint32_t pgm_off;
} qstate_cfg_t;

/// \brief event relay and processing states
typedef struct ev_in_progress_state_s {
    /// pending responses from threads
    uint8_t rsps_pending;
    /// event status in terms of return code
    sdk_ret_t status;
    /// more events are required for a particular stage to complete
    bool more;
    /// in progress event message id
    upg_ev_msg_id_t id;
    /// in progress params
    sdk::upg::upg_ev_params_t params;
} ev_in_progress_state_t;

/// \defgroup PDS_UPGRADE PDS Upgrade
/// \ingroup  UPGRADE_STATE
/// @{

/// \brief state maintained for upgrade
// singleton class
class upg_state {
public:
    /// \brief constructor
    upg_state() {}

    /// \brief destructor
    ~upg_state() {}

    /// \brief singleton factory method to instantiate the upgrade state
    /// \return NULL if there is a failure, pointer to state if success
    static upg_state *factory(pds_init_params_t *params);

    /// \brief destroy the upgrade state
    /// \param[in] state destroy the upgrade state
    static void destroy(upg_state *state);

    // table engine configuration. will be extracted during pre-upgrade and will be
    // applied during the final stage of the upgrade.
    uint32_t tbl_eng_cfg(p4pd_pipeline_t pipe, p4_tbl_eng_cfg_t **cfg, uint32_t *max_cfgs);
    void incr_tbl_eng_cfg_count(p4pd_pipeline_t pipe, uint32_t ncfgs);
    // clear the saved info if a new backup request come.
    // this can come if there is a upgrade failure after backup and retrying
    void clear_tbl_eng_cfg(void) {
        memset(tbl_eng_cfgs_count_, 0, sizeof(tbl_eng_cfgs_count_));
        memset(tbl_eng_cfgs_, 0, sizeof(tbl_eng_cfgs_));
    }
    // qstate config info. only pc_offset will be modified during upgrade
    // pc_offset will be exctracted during pre_upgrade and will be applied during
    // the final stage of the upgrade
    void set_qstate_cfg(uint64_t addr, uint32_t size, uint32_t pgm_off);
    std::list<qstate_cfg_t> &qstate_cfg(void) { return qstate_cfgs_; }
    // table engine rss configuration
    void tbl_eng_rss_cfg(p4_tbl_eng_cfg_t **cfg) { *cfg = &tbl_eng_cfg_rss_; }
    // register event threads
    void register_ev_thread_hdlr(upg_ev_hitless_t &ev) {
        ev_threads_hdlr_hl_.push_back(ev);
    }
    std::list<upg_ev_hitless_t> &ev_threads_hdlr_hitless(void) {
        return ev_threads_hdlr_hl_;
    }
    void register_ev_thread_hdlr(upg_ev_graceful_t &ev) {
        ev_threads_hdlr_gf_.push_back(ev);
    }
    std::list<upg_ev_graceful_t> &ev_threads_hdlr_graceful(void) {
        return ev_threads_hdlr_gf_;
    }
    sdk_ret_t ev_status(void) { return ev_state_.status; }
    void set_ev_status(sdk_ret_t status) {
        if (status == SDK_RET_OK || status == SDK_RET_IN_PROGRESS) {
            return;
        }
        if (ev_state_.status != sdk_ret_t::SDK_RET_UPG_CRITICAL) {
            ev_state_.status = status;
        }
    }
    void clear_ev_status(void) { ev_state_.status = SDK_RET_OK; }
    bool ev_in_progress(void) { return ev_state_.rsps_pending > 0; }
    void ev_incr_in_progress(void) { ev_state_.rsps_pending++; }
    void ev_decr_in_progress(void) { ev_state_.rsps_pending--; }
    void ev_clear_in_progress(void) { ev_state_.rsps_pending = 0; }
    void set_ev_in_progress_id(upg_ev_msg_id_t id) { ev_state_.id = id; }
    upg_ev_msg_id_t ev_in_progress_id(void) { return ev_state_.id; }
    void set_ev_more(bool more) { ev_state_.more = more; }
    bool ev_more(void) { return ev_state_.more; }
    sdk::upg::upg_ev_params_t *ev_params(void) { return &ev_state_.params; }
    void set_ev_params(sdk::upg::upg_ev_params_t *params) {
        ev_state_.params = *params;
    }
    void set_init_mode(sysinit_mode_t mode) { init_mode_ = mode; }
    sysinit_mode_t init_mode(void) { return init_mode_; }
    void set_init_domain(sysinit_dom_t dom) { init_dom_ = dom; }
    sysinit_dom_t init_domain(void) { return init_dom_; }
    void set_upg_req_mode(sysinit_mode_t mode) { upg_req_mode_ = mode; }
    sysinit_mode_t upg_req_mode(void) { return upg_req_mode_; }
    /// \brief set backup/restore  status
    void set_backup_status(bool status) { backup_status_ = status; }
    /// \brief get backup/restore status
    bool backup_status(void) { return backup_status_; }
    /// \brief upgrade store instance
    void insert_backup_shmstore(pds_shmstore_id_t id,
                                sdk::lib::shmstore *store) {
        SDK_ASSERT(!backup_shmstore_[id]);
        backup_shmstore_[id] = store;
    }
    sdk::lib::shmstore *backup_shmstore(pds_shmstore_id_t id) {
        return backup_shmstore_[id];
    }
    void insert_restore_shmstore(pds_shmstore_id_t id,
                                 sdk::lib::shmstore *store) {
        SDK_ASSERT(!restore_shmstore_[id]);
        restore_shmstore_[id] = store;
    }
    sdk::lib::shmstore *restore_shmstore(pds_shmstore_id_t id) {
        return restore_shmstore_[id];
    }
    /// \brief module versions, indexed using module id
    void insert_module_version(uint32_t id, module_version_conf_t conf,
                               module_version_pair_t &version) {
        module_version_map_[conf].insert(std::make_pair(id, version));
    }
    module_version_pair_t module_version(uint32_t id,
                                         module_version_conf_t conf) const {
        return module_version_map_[conf].at(id);
    }
    upgrade_pstate_t *pstate(void) { return pstate_; }
    void set_pstate(upgrade_pstate_t *pstate) { pstate_ = pstate; }

private:
    /// lif qstate mpu program offset map
    std::list<qstate_cfg_t> qstate_cfgs_;
    /// table engine configs. saved during upgrade init and applied during switch
    p4_tbl_eng_cfg_t tbl_eng_cfgs_[P4_PIPELINE_MAX][P4TBL_ID_MAX];
    /// number of valid entries in table engine config
    uint32_t tbl_eng_cfgs_count_[P4_PIPELINE_MAX];
    /// rss table engine config requires special handling in capri programming
    p4_tbl_eng_cfg_t tbl_eng_cfg_rss_;
    /// upgrade event callbacks registered by pds threads
    std::list<upg_ev_graceful_t> ev_threads_hdlr_gf_;
    std::list<upg_ev_hitless_t> ev_threads_hdlr_hl_;
    /// event relay and processing states
    ev_in_progress_state_t ev_state_;
    ///  upgrade mode in new request
    sysinit_mode_t upg_req_mode_;
    ///  initialization mode during process bringup
    sysinit_mode_t init_mode_;
    ///  upgrade init domain
    sysinit_dom_t init_dom_;
    /// backup status
    bool backup_status_;
    /// upgrade object store. indexed using a unique id and store type
    sdk::lib::shmstore *backup_shmstore_[PDS_SHMSTORE_ID_MAX];
    sdk::lib::shmstore *restore_shmstore_[PDS_SHMSTORE_ID_MAX];
    /// module versions indexed using a unique id
    std::unordered_map<uint32_t, module_version_pair_t>
        module_version_map_[MODULE_VERSION_CONF_MAX];
    /// states shared b/w A and B during hitless upgrade
    upgrade_pstate_t *pstate_;
private:
    void init_(pds_init_params_t *params);
};

extern upg_state *g_upg_state;

/// @}

}    // namespace api
#endif    // __API_UPGRADE_STATE_HPP__
