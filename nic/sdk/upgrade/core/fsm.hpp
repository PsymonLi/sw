// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Implementation for core upgrade data structures, methods, and APIs
///
//----------------------------------------------------------------------------

#ifndef __UPGRADE_CORE_FSM_HPP__
#define __UPGRADE_CORE_FSM_HPP__

#include <iostream>
#include <ev.h>
#include <boost/unordered_map.hpp>
#include <boost/container/vector.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "include/sdk/base.hpp"
#include "lib/ipc/ipc.hpp"
#include "upgrade/include/upgrade.hpp"
#include "stage.hpp"
#include "service.hpp"
#include "logger.hpp"
#define UPGMGR_EXIT_SCRIPT "upgmgr_exit.sh"

namespace sdk {
namespace upg {

// async callback function for upgrade completion
typedef void (*fsm_completion_cb_t)(upg_status_t status);
typedef void (*upg_event_fwd_cb_t)(upg_stage_t stage, std::string svc_name,
                                   uint32_t svc_id);

// fsm initialization parameters
typedef struct fsm_init_params_s {
    sdk::platform::sysinit_mode_t upg_mode; // upgrade mode
    struct ev_loop *ev_loop;                // event loop
    fsm_completion_cb_t fsm_completion_cb;  // fsm exit or fsm stage exit
    // fsm event forward handler for hitless upgrade
    upg_event_fwd_cb_t upg_event_fwd_cb;
    upg_stage_t entry_stage;                // start a specific discovery stage
    std::string tools_dir;                  // tools directory
    std::string fw_pkgname;                 // firmware package name with full path
    bool interactive_mode;                  // start fsm or interactive mode
} fsm_init_params_t;


class fsm {
public:
    fsm(upg_stage_t start = UPG_STAGE_COMPAT_CHECK,
        upg_stage_t end = UPG_STAGE_EXIT) {
        current_stage_ = start;
        start_stage_ = start;
        end_stage_ = end;
        pending_response_ = 0;
        size_ = 0;
        timeout_ = DEFAULT_SVC_RSP_TIMEOUT;
        prev_stage_rsp_ = SVC_RSP_OK;
        stage_response_ = SVC_RSP_OK;
        is_pre_hooks_done_ = false;
        is_post_hooks_done_ = false;
        domain_ = IPC_SVC_DOM_ID_A;
    }
    ~fsm(void){};
    upg_stage_t current_stage(void) const { return current_stage_; }
    upg_stage_t start_stage(void) const { return start_stage_; }
    void set_start_stage(const upg_stage_t entry_stage) {
        start_stage_ = entry_stage;
        is_pre_hooks_done_ = false;
        is_post_hooks_done_ = false;
    }
    upg_stage_t end_stage(void) const { return end_stage_; }
    uint32_t pending_response(void) const { return pending_response_; }
    void set_pending_response(const uint32_t count) {
        pending_response_ = count;
    }
    bool is_current_stage_over(upg_stage_t id) const {
        return current_stage_ != id;
    };
    void set_timeout(const ev_tstamp timeout) { timeout_ = timeout; }
    ev_tstamp timeout(void) const { return timeout_; }
    svc_sequence_list svc_sequence(void) const { return svc_sequence_; }
    bool has_next_svc(void) const { return pending_response_ > 0; }
    svc_rsp_code_t prev_stage_rsp(void) const { return prev_stage_rsp_; }
    void set_prev_stage_rsp(svc_rsp_code_t rsp);
    void set_stage_response(svc_rsp_code_t rsp);
    svc_rsp_code_t stage_response(void) const { return stage_response_; };
    void reset_stage_response(svc_rsp_code_t rsp = SVC_RSP_OK) {
        stage_response_ = rsp;
    };
    void set_current_stage(const upg_stage_t id);
    void update_stage_progress(const svc_rsp_code_t rsp,
                               bool check_pending_rsp=true);
    void update_stage_progress_interactive(const svc_rsp_code_t rsp);
    bool is_serial_event_sequence(void) const;
    bool is_parallel_event_sequence(void) const;
    const char* get_event_sequence_type(void) const;
    bool is_discovery(void) const;
    bool is_pre_hooks_done(void) const { return is_pre_hooks_done_; };
    bool is_post_hooks_done(void) const { return is_post_hooks_done_; };
    void set_is_pre_hooks_done(bool done) { is_pre_hooks_done_ = done; };
    void set_is_post_hooks_done(bool done) { is_post_hooks_done_ = done; };
    bool is_valid_service(const std::string svc) const;
    std::string next_svc(void) const;
    void timer_init(struct ev_loop *loop);
    void timer_start(void);
    void timer_stop(void);
    void timer_set(void);
    void skip_svc(void);
    fsm_init_params_t *init_params(void) { return &init_params_; }
    void set_init_params(fsm_init_params_t *params) { init_params_ = *params; }
    ipc_svc_dom_id_t domain(void) const { return domain_; };
    void set_domain(ipc_svc_dom_id_t dom ) { domain_ = dom; };
    std::string pending_svcs(void) ;
    void set_pending_svcs(void);
    bool find_pending_svc(std::string svc) const;
    void clear_pending_svc(std::string svc);
    void set_pending_svc(std::string svc) { pending_svcs_.push_back(svc); }
    bool is_empty_pending_svcs(void) const { return pending_svcs_.empty(); }
    void clear_pending_svcs(void) { pending_svcs_.clear(); }
private:
    void update_stage_progress_internal_(void);
    upg_stage_t current_stage_;
    upg_stage_t start_stage_;
    upg_stage_t end_stage_;
    uint32_t pending_response_;
    svc_sequence_list pending_svcs_;
    uint32_t size_;
    svc_sequence_list svc_sequence_;
    ev_tstamp timeout_;
    fsm_init_params_t init_params_;
    svc_rsp_code_t prev_stage_rsp_;
    svc_rsp_code_t stage_response_;
    bool is_pre_hooks_done_;
    bool is_post_hooks_done_;
    ipc_svc_dom_id_t domain_;
};

sdk_ret_t init(fsm_init_params_t *params);
sdk_ret_t init_interactive(fsm_init_params_t *params);
void upg_event_handler(upg_event_msg_t *event);
void upg_event_interactive_handler(upg_event_msg_t *event);
sdk_ret_t upg_interactive_stage_exec(upg_stage_t stage);
sdk_ret_t execute_exit_script (const char *option, upg_status_t status);
upg_status_t get_exit_status(void);
ev_tstamp stage_timeout(void);
sdk_ret_t fsm_exit(upg_status_t status);
sdk_ret_t interactive_fsm_exit(upg_status_t status);

extern struct ev_loop *loop;
extern ev_timer timeout_watcher;
extern fsm fsm_states;
extern upg_stages_map fsm_stages;
extern upg_svc_map fsm_services;
extern svc_sequence_list default_svc_names;
extern stage_map fsm_lookup_tbl;
extern event_sequence_t event_sequence;
extern upg_stage_t entry_stage;

}   // namespace upg
}   // namespace sdk

#endif    //  __UPGRADE_CORE_FSM_HPP__
