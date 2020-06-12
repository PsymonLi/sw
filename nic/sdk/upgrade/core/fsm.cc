// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Implementation for core upgrade data structures, methods, and APIs
///
//----------------------------------------------------------------------------

#include <iostream>
#include <cassert>
#include <exception>
#include <sstream>
#include <string>
#include <ev.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/container/vector.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "include/sdk/base.hpp"
#include "lib/event_thread/event_thread.hpp"
#include "fsm.hpp"
#include "ipc.hpp"
#include "stage.hpp"
#include "logger.hpp"
#include "utils.hpp"

#define UPGRADE_GRACEFUL "upgrade_graceful.json"
#define UPGRADE_HITLESS  "upgrade_hitless.json"

namespace sdk {
namespace upg {

namespace pt = boost::property_tree;

 struct ev_loop *loop;
ev_timer timeout_watcher;
fsm fsm_states;
upg_stages_map fsm_stages;
upg_svc_map fsm_services;
svc_sequence_list default_svc_names;
stage_map fsm_lookup_tbl;
event_sequence_t event_sequence;
upg_stage_t entry_stage;


static void
dispatch_event (ipc_svc_dom_id_t dom, upg_stage_t id, upg_svc svc)
{
    LOG_SEND_MSG(svc.name().c_str(), fsm_states.timeout(),
                 ipc_svc_dom_id_to_name(dom));
    SDK_ASSERT(fsm_stages.find(id) != fsm_stages.end());

    if (svc.has_valid_ipc_id()) {
        if (dom == IPC_SVC_DOM_ID_B &&
            fsm_states.init_params()->upg_event_fwd_cb) {
            fsm_states.init_params()->upg_event_fwd_cb(id, svc.name(),
                                                       svc.ipc_id());
        } else {
            fsm_states.set_pending_svc(svc.name());
            svc.dispatch_event(dom, id, fsm_states.init_params()->upg_mode);
        }
    }
}

static void
dispatch_event (ipc_svc_dom_id_t dom, upg_stage_t id)
{
    SDK_ASSERT(fsm_stages.find(id) != fsm_stages.end());

    for (auto& name : fsm_states.svc_sequence()) {
        SDK_ASSERT(fsm_services.find(name) != fsm_services.end());
        upg_svc svc = fsm_services[name];
        if (svc.has_valid_ipc_id()) {
            dispatch_event(dom, id, svc);
        } else {
            fsm_states.skip_svc();
            LOG_INVALID_OBJ("service", name.c_str());
        }
    }
}

static void
send_discovery_event (ipc_svc_dom_id_t dom, upg_stage_t id)
{
    SDK_ASSERT(fsm_stages.find(id) != fsm_stages.end());
    LOG_BROADCAST_MSG(ipc_svc_dom_id_to_name(dom));
    if (dom == IPC_SVC_DOM_ID_B &&
        fsm_states.init_params()->upg_event_fwd_cb) {
            std::string svc_name = fsm_states.next_svc();
            upg_svc svc = fsm_services[svc_name];
            fsm_states.init_params()->upg_event_fwd_cb(id, svc_name,
                                                       svc.ipc_id());
    } else {
        upg_send_broadcast_request(dom, id, fsm_states.init_params()->upg_mode,
                                   fsm_services.size(), fsm_states.timeout());
    }
}

static void
update_ipc_id (std::string name, uint32_t ipc_id)
{
    SDK_ASSERT(fsm_services.find(name) != fsm_services.end());
    LOG_UPDATE_SVC_IPC_MAP(name.c_str(), ipc_id);
    upg_svc service = fsm_services[name];
    service.set_ipc_id(ipc_id);
    fsm_services[name] = service;
}

static bool
invoke_hooks (upg_stage_t stage_id, hook_execution_t hook_type,
              svc_rsp_code_t status=SVC_RSP_MAX)
{
    upg_stage stage;
    upg_scripts hooks;
    std::string name;
    bool result = true;
    name = upg_stage2str(stage_id);
    stage = fsm_stages[stage_id];

    if (hook_type == PRE_STAGE) {
        hooks = stage.pre_hook_scripts();
    } else {
        hooks = stage.post_hook_scripts();
    }

    for (const auto& x : hooks) {
        if (!is_valid_script(fsm_states.init_params()->tools_dir, x.path())) {
            LOG_INVALID_OBJ("script", x.path().c_str());
            result = false;
            break;
        }
        if (!execute_hook(fsm_states.init_params()->tools_dir,
                          x.path(), name, fsm_states.init_params()->fw_pkgname,
                          hook_type, status)) {
            LOG_SCRIPT_STATUS(status);
            result = false;
            break;
        }
    }
    return result;
}

static bool
execute_pre_hooks (upg_stage_t stage_id)
{
    if (!fsm_states.is_pre_hooks_done()) {
        fsm_states.set_is_pre_hooks_done(true);
        return invoke_hooks(stage_id, PRE_STAGE);
    } else {
        std::string name = upg_stage2str(stage_id);
        return true;
    }
}

static bool
execute_post_hooks (upg_stage_t stage_id, svc_rsp_code_t status)
{
    SDK_ASSERT(status != SVC_RSP_MAX);
    if (!fsm_states.is_post_hooks_done()) {
        fsm_states.set_is_post_hooks_done(true);
        return invoke_hooks(stage_id, POST_STAGE, status);
    } else {
        std::string name = upg_stage2str(stage_id);
        return true;
    }
}

static void
send_ipc_to_next_service (void)
{
    ipc_svc_dom_id_t domain = fsm_states.domain();

    if (!fsm_states.has_next_svc()) {
        // Stage must have pending svc
        // if it is not over yet
        SDK_ASSERT(0);
    } else {
        upg_stage_t id = fsm_states.current_stage();
        std::string name(upg_stage2str(fsm_states.current_stage()));

        while (fsm_states.has_next_svc()) {
            std::string svc_name = fsm_states.next_svc();
            SDK_ASSERT(fsm_services.find(svc_name) != fsm_services.end());
            upg_svc svc = fsm_services[svc_name];
            if (svc.has_valid_ipc_id()) {
                fsm_states.timer_stop();
                fsm_states.timer_start();
                dispatch_event(domain, id, svc);
                break;
            } else {
                LOG_INVALID_OBJ("service", name.c_str());
                fsm_states.skip_svc();
            }
        }
    }
}

void
execute_exit_script (upg_status_t status)
{
    if (UPG_STATUS_OK != status) {
        std::string exit_script("");
        exit_script = UPGMGR_EXIT_SCRIPT;

        if (is_valid_script(fsm_states.init_params()->tools_dir, exit_script)) {
            exit_script = fsm_states.init_params()->tools_dir + "/" + exit_script;
            exit_script += " -r " + std::string(svc_rsp_code_name[status]);
            if (!execute(exit_script.c_str())) {
                UPG_TRACE_ERR("Tech support may not be generated !");
            }
        } else {
            LOG_INVALID_OBJ("script", exit_script.c_str());
        }
    }
}

static upg_status_t
get_exit_status (void)
{
    upg_status_t status;

    if (fsm_states.prev_stage_rsp() == SVC_RSP_CRIT) {
        status = UPG_STATUS_CRITICAL;
    } else if (fsm_states.prev_stage_rsp() == SVC_RSP_FAIL) {
        status = UPG_STATUS_FAIL;
    } else if (fsm_states.prev_stage_rsp() == SVC_RSP_NONE) {
        status = UPG_STATUS_FAIL;
    } else if (fsm_states.prev_stage_rsp() == SVC_RSP_OK) {
        status = UPG_STATUS_OK;
    } else {
        SDK_ASSERT(0);
    }

    if (UPG_STATUS_OK != status) {
        UPG_TRACE_ERR("Upgrade exit code %u ", status);
    } else {
        UPG_TRACE_INFO("Upgrade exit code %u ", status);
    }

    return status;
}

static void
move_to_nextstage (void)
{
    ipc_svc_dom_id_t domain = fsm_states.domain();

    upg_stage_t id = fsm_states.current_stage();
    fsm_states.set_current_stage(id);

    LOG_STAGE_START(fsm_states.get_event_sequence_type(),
                    svc_sequence_to_str(fsm_states.svc_sequence()).c_str(),
                    fsm_states.timeout(),
                    ipc_svc_dom_id_to_name(fsm_states.domain()));

    if (fsm_states.current_stage() == fsm_states.end_stage()) {
        upg_status_t status = get_exit_status();
        execute_exit_script(status);
        fsm_states.init_params()->fsm_completion_cb(status);
        SDK_ASSERT(0);
    }

    std::string name(upg_stage2str(fsm_states.current_stage()));
    if (fsm_states.is_discovery()) {
        dump(fsm_states);
        if (!execute_pre_hooks(id)) {
            fsm_states.update_stage_progress(SVC_RSP_FAIL);
            move_to_nextstage();
        }

        upg_stage_t id = fsm_states.current_stage();
        fsm_states.timer_stop();
        fsm_states.timer_start();
        send_discovery_event(domain, id);
    } else if (fsm_states.is_serial_event_sequence()) {
        dump(fsm_states);
        if (!execute_pre_hooks(id)) {
            fsm_states.update_stage_progress(SVC_RSP_FAIL);
            move_to_nextstage();
        }

        if (fsm_states.has_next_svc()) {
            send_ipc_to_next_service();
        } else {
            UPG_TRACE_ERR("Oops! no service to send request !");
            SDK_ASSERT(0);
        }
    } else if (fsm_states.is_parallel_event_sequence()) {
        dump(fsm_states);

        if (!execute_pre_hooks(id)) {
            fsm_states.update_stage_progress(SVC_RSP_FAIL);
            move_to_nextstage();
        }

        upg_stage_t id = fsm_states.current_stage();
        fsm_states.timer_stop();
        fsm_states.timer_start();
        dispatch_event(domain, id);
    } else {
       SDK_ASSERT(0);
    }
}

void
upg_event_handler (upg_event_msg_t *event)
{
    upg_stage_t id = fsm_states.current_stage();
    std::string svc_name = event->rsp_svc_name;

    LOG_RESPONSE_MSG(event->rsp_svc_name, upg_status2str(event->rsp_status));
    if (event->stage == id && fsm_states.is_valid_service(svc_name)) {
        fsm_states.clear_pending_svc(svc_name);

        if ((event->stage == fsm_states.start_stage()) ||
            fsm_states.is_discovery()) {
            update_ipc_id(svc_name, event->rsp_svc_ipc_id);
        }

        fsm_states.update_stage_progress(svc_rsp_code(event->rsp_status));
        if (fsm_states.is_current_stage_over(event->stage)) {
            upg_stage_t id = fsm_states.current_stage();
            fsm_states.set_current_stage(id);
            if (fsm_states.current_stage() == fsm_states.end_stage()) {
                upg_status_t status = get_exit_status();
                execute_exit_script(status);
                // not expeting to come back here
                fsm_states.init_params()->fsm_completion_cb(status);
                SDK_ASSERT(0);
            } else {
                if (!execute_pre_hooks(id)) {
                    fsm_states.update_stage_progress(SVC_RSP_FAIL);
                }
                move_to_nextstage();
            }
        } else if (fsm_states.is_serial_event_sequence() &&
                   (fsm_states.current_stage() != fsm_states.start_stage())) {
            send_ipc_to_next_service();
        } else { }
    }
}

static void
timeout_cb (EV_P_ ev_timer *w, int revents)
{
    fsm_states.timer_stop();
    fsm_states.update_stage_progress(SVC_RSP_NONE);

    if (fsm_states.current_stage() != fsm_states.end_stage()) {
        move_to_nextstage();
    } else {
        UPG_TRACE_WARN("Upgrade must not wait for response in last stage");
        execute_exit_script(get_exit_status());
        fsm_states.init_params()->fsm_completion_cb(UPG_STATUS_FAIL);
        SDK_ASSERT(0);
    }
}

static upg_stage_t
lookup_stage_transition (upg_stage_t cur, svc_rsp_code_t rsp)
{
    upg_stage_t next_stage = UPG_STAGE_EXIT;

    LOG_STAGE_FINISHED(upg_stage2str(cur));
    next_stage = fsm_lookup_tbl[std::to_string(cur) + std::to_string(rsp)];
    LOG_STAGE_STARTED(upg_stage2str(next_stage));
    return next_stage;
}

// Note: please do not call this in ctor
void
fsm::set_current_stage(const upg_stage_t stage_id) {
    current_stage_ = stage_id;

    SDK_ASSERT(fsm_stages.find(start_stage_) != fsm_stages.end());
    SDK_ASSERT(end_stage_ == UPG_STAGE_EXIT);

    upg_stage stage = fsm_stages[current_stage_];
    svc_sequence_ = stage.svc_sequence();
    size_ = 0;
    for (auto x : svc_sequence_) {
        size_++;
    }
    pending_response_ = size_;
    timeout_ = stage.svc_rsp_timeout();
    timeout_ = double(timeout_ * 1.0) / 1000;
    domain_ = stage.domain();

    if (domain_ == IPC_SVC_DOM_ID_B &&
        fsm_states.init_params()->upg_event_fwd_cb) {
        // this one is the master and driving the stage to domB to execute
        svc_sequence_.resize(1);
        size_ = 1;
        pending_response_ = 1;
    }

}

void
fsm::timer_stop(void) {
    ev_timer_stop(loop, &timeout_watcher);
}

void
fsm::timer_init(struct ev_loop *ev_loop) {
    loop = ev_loop;
    ev_timer_init(&timeout_watcher, timeout_cb,
                  ev_time() - ev_now(loop) + timeout_, 0.0);
}

void
fsm::timer_start(void) {
    ev_timer_set(&timeout_watcher, ev_time() - ev_now(loop) + timeout_, 0.0);
    ev_timer_start(loop, &timeout_watcher);
}

void
fsm::update_stage_progress_internal_(void) {
    if (current_stage_ != end_stage_) {
        upg_stage stage = fsm_stages[current_stage_];
        timeout_ = stage.svc_rsp_timeout();
        svc_sequence_ = stage.svc_sequence();
        size_ = 0;
        for (auto x : svc_sequence_) {
            size_++;
        }
        pending_response_ = size_;
    } else {
        timeout_ = 0;
        pending_response_ = 0;
        size_ = 0;
    }
    is_pre_hooks_done_ = false;
    is_post_hooks_done_ = false;
}

void
fsm::update_stage_progress(const svc_rsp_code_t rsp) {
    if (rsp != SVC_RSP_OK) {
        switch (rsp) {
        // prev_stage_rsp_ will carry the response
        case SVC_RSP_FAIL:
            UPG_TRACE_ERR("Got failure response in stage %s",
                          upg_stage2str(current_stage_));
            break;
        case SVC_RSP_CRIT:
            UPG_TRACE_ERR("Got critical service response in stage %s",
                          upg_stage2str(current_stage_));
            break;
        case SVC_RSP_NONE:
            if (fsm_states.is_discovery()) {
                UPG_TRACE_ERR("Timer expired, no service response so far");
            } else {
                std::string svcs = fsm_states.pending_svcs();
                UPG_TRACE_ERR("Timer expired, no service response from %s",
                              svcs.c_str());
            }
            break;
        default:
            break;
        }
        execute_post_hooks(current_stage_, rsp);
        current_stage_ = lookup_stage_transition(current_stage_, rsp);
        update_stage_progress_internal_();
        prev_stage_rsp_ = rsp;
    } else {
        pending_response_--;

        SDK_ASSERT(pending_response_ >= 0);
        if (pending_response_ == 0) {

            SDK_ASSERT(pending_response_ >= 0);

            if (!execute_post_hooks(current_stage_, rsp)) {
                prev_stage_rsp_ = SVC_RSP_FAIL;
                current_stage_ = lookup_stage_transition(current_stage_,
                                                         SVC_RSP_FAIL );
            } else {
                current_stage_ = lookup_stage_transition(current_stage_, rsp);
            }

            update_stage_progress_internal_();
            prev_stage_rsp_ = (prev_stage_rsp_ == SVC_RSP_OK) ?
                SVC_RSP_OK : prev_stage_rsp_;
        }
    }
}

bool
fsm::is_valid_service(const std::string svc) const {
    for (auto x : svc_sequence_) {
        if (svc.compare(x) == 0) {
            return true;
        }
    }
    LOG_INVALID_OBJ("service", svc.c_str());
    return false;
}

void
fsm::skip_svc(void)
{
    SDK_ASSERT(pending_response_ > 0);
    pending_response_--;
}

std::string
fsm::next_svc(void) const {
    std::string svc;
    SDK_ASSERT(pending_response_ > 0);
    svc = svc_sequence_[size_ - pending_response_];
    return svc;
}

std::string
fsm::pending_svcs(void)
{
    std::string svcs;
    for (auto const& s : pending_svcs_) {
        svcs += s + ", ";
    }
    return svcs;
}

void
fsm::clear_pending_svc(std::string svc)
{
    auto itr = std::find(pending_svcs_.begin(), pending_svcs_.end(), svc);
    if (itr != pending_svcs_.end()) {
        pending_svcs_.erase(itr);
    }
}

void
fsm::set_pending_svc(std::string svc)
{
    pending_svcs_.push_back(svc);
}
const char*
fsm::get_event_sequence_type(void) const {
    SDK_ASSERT(fsm_stages.find(current_stage_) != fsm_stages.end());
    upg_stage stage = fsm_stages[current_stage_];
    if (stage.is_discovery()) {
        return "discovery";
    } else if (stage.event_sequence() == SERIAL) {
        return "serial";
    } else if (stage.event_sequence() == PARALLEL) {
        return "parallel";
    } else {
        return "";
    }
}

bool
fsm::is_serial_event_sequence(void) const {
    SDK_ASSERT(fsm_stages.find(current_stage_) != fsm_stages.end());
    upg_stage stage = fsm_stages[current_stage_];
    return stage.event_sequence() == SERIAL;
}

bool
fsm::is_parallel_event_sequence(void) const {
    SDK_ASSERT(fsm_stages.find(current_stage_) != fsm_stages.end());
    upg_stage stage = fsm_stages[current_stage_];
    return stage.event_sequence() == PARALLEL;
}

bool
fsm::is_discovery(void) const {
    SDK_ASSERT(fsm_stages.find(current_stage_) != fsm_stages.end());
    upg_stage stage = fsm_stages[current_stage_];
    return stage.is_discovery();
}

static svc_sequence_list
str_to_svc_sequence (std::string& svc_seq)
{
    svc_sequence_list svcs_sequence;
    if (svc_seq.empty()) {
        svcs_sequence = default_svc_names;
    } else {
        std::vector<std::string> svcs;
        token_parse(svc_seq, svcs);
        for (std::string& name : svcs) {
            svcs_sequence.push_back(name);
        }
    }
    return svcs_sequence;
};

static event_sequence_t
str_to_event_sequence (std::string& evt_seq)
{
    event_sequence_t seq = PARALLEL;
    if (evt_seq.empty()) {
        seq = event_sequence;
    } else if (boost::iequals(evt_seq, "parallel")) {
        seq = PARALLEL;
    } else if (boost::iequals(evt_seq, "serial")) {
        seq = SERIAL;
    } else {
        SDK_ASSERT(0);
    }
    return seq;
};

static svc_sequence_list
get_upg_services (pt::ptree& tree)
{
    svc_sequence_list svc_list;

    BOOST_FOREACH (pt::ptree::value_type const &svc,
                   tree.get_child("upg_svc")) {
        svc_list.push_back(svc.second.get_value<std::string>());
    }
    return svc_list;
}

static sdk_ret_t
init_svc (pt::ptree& tree)
{
    sdk_ret_t ret = SDK_RET_ERR;
    try
    {
        svc_sequence_list svc_list;

        svc_list = get_upg_services(tree);
        for (auto const& name : svc_list) {
            upg_svc service = upg_svc(name);
            fsm_services[service.name()] = service;
            default_svc_names.push_back(service.name());
        }
        return SDK_RET_OK;
    }
    catch (std::exception const& ex)
    {
        UPG_TRACE_ERR("Failed to read service config from "
                      "upgrade spec %s", ex.what());
    }
    return ret;
}

static sdk_ret_t
init_stage_transitions (pt::ptree& tree)
{
    upg_stage_t curr;
    upg_stage_t next;
    svc_rsp_code_t rsp;

    std::string key;
    std::vector<std::string> stage_transition;

    sdk_ret_t ret = SDK_RET_ERR;
    try
    {
        BOOST_FOREACH (pt::ptree::value_type &row,
                       tree.get_child("upg_stage_transitions"))
        {
            stage_transition.clear();
            BOOST_FOREACH (pt::ptree::value_type& transition, row.second)
            {
                std::string str = transition.second.get_value<std::string>();
                stage_transition.push_back(str);
            }
            curr = name_to_stage_id(stage_transition[0]);
            next = name_to_stage_id(stage_transition[2]);
            rsp = svc_rsp_code_name_to_id(stage_transition[1]);
            key = std::to_string(curr) + std::to_string(rsp);

            fsm_lookup_tbl[key] = next;
        }
        return SDK_RET_OK;
    }
    catch (std::exception const& ex)
    {
        UPG_TRACE_ERR("Failed to read fsm stage transition from "
                      "upgrade spec %s", ex.what());
    }
    return ret;
}

static sdk_ret_t
init_fsm_stages (pt::ptree& tree)
{
    upg_stage_t stage_id;
    ev_tstamp svc_rsp_timeout;
    svc_sequence_list svcs_seq;
    event_sequence_t event_seq;
    upg_scripts prehook;
    upg_scripts posthook;
    bool is_discovery = false;
    ipc_svc_dom_id_t domain = IPC_SVC_DOM_ID_A;

    sdk_ret_t ret = SDK_RET_OK;
    try
    {
        BOOST_FOREACH (pt::ptree::value_type const&v,
                       tree.get_child("upg_stages")) {
            const std::string & key = v.first;
            const pt::ptree & subtree = v.second;
            std::string timeout_str;
            std::string svc_seq_str;
            std::string evt_seq_str;
            std::string pre_hook_str;
            std::string post_hook_str;
            std::string discovery_str;
            std::string domain_str;
            if(subtree.empty()) {
                UPG_TRACE_ERR("Parsing Error !");
                return SDK_RET_ERR;
            } else {
                timeout_str = subtree.get<std::string>("rsp_timeout", "60000");
                svc_seq_str = subtree.get<std::string>("svc_sequence", "");
                evt_seq_str =
                    subtree.get<std::string>("event_sequence", "parallel");
                pre_hook_str = subtree.get<std::string>("pre_hook", "");
                post_hook_str = subtree.get<std::string>("post_hook", "");
                discovery_str = subtree.get<std::string>("discovery", "no");
                domain_str = subtree.get<std::string>("domain", "A");

                stage_id = name_to_stage_id(key);
                svcs_seq = str_to_svc_sequence(svc_seq_str);
                event_seq = str_to_event_sequence(evt_seq_str);
                prehook = str_to_scripts(pre_hook_str);
                posthook  = str_to_scripts(post_hook_str);
                svc_rsp_timeout = str_to_timeout(timeout_str);
                is_discovery = (discovery_str.compare("no")== 0) ? false : true;
                domain = (domain_str.compare("A")== 0) ? IPC_SVC_DOM_ID_A :
                    IPC_SVC_DOM_ID_B;

                fsm_stages[stage_id] = upg_stage(svc_rsp_timeout, svcs_seq,
                                                 event_seq, prehook,posthook,
                                                 is_discovery, domain);
            }
        }
    }
    catch (std::exception const& ex)
    {
        UPG_TRACE_ERR("Failed to read fsm stages from "
                      "upgrade spec %s", ex.what());
    }
    dump(fsm_stages);
    return ret;
}

static sdk_ret_t
set_entry_stage (pt::ptree& tree)
{
    std::string stage;
    try
    {
        stage = tree.get<std::string>("entry_stage", "compatcheck");
        entry_stage = name_to_stage_id(stage);
    }
    catch (std::exception const& ex)
    {
        UPG_TRACE_ERR("Failed to read entry stage "
                      " from upgrade spec %s", ex.what());
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

static sdk_ret_t
set_event_sequence (pt::ptree& tree)
{
    try
    {
        std::string seq;
        seq = tree.get<std::string>("event_sequence", "parallel");
        event_sequence = str_to_event_sequence(seq);
    }
    catch (std::exception const& ex)
    {
        UPG_TRACE_ERR("Failed to read event sequence"
                      " from upgrade spec %s", ex.what());
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

static sdk_ret_t
init_fsm (fsm_init_params_t *params)
{
    fsm_states.set_start_stage(entry_stage);

    upg_stage_t start_stage = fsm_states.start_stage();

    SDK_ASSERT(fsm_stages.find(start_stage) != fsm_stages.end());
    SDK_ASSERT(params != NULL);

    if (UPG_STAGE_NONE != params->entry_stage) {
        std::string stage;
        stage = id_to_stage_name(params->entry_stage);
        fsm_states.set_current_stage(params->entry_stage);
    } else {
        std::string stage;
        stage = id_to_stage_name(start_stage);
        fsm_states.set_current_stage(start_stage);
    }
    fsm_states.timer_init(params->ev_loop);
    fsm_states.timer_start();
    upg_ipc_init(upg_event_handler);

    LOG_STAGE_START(fsm_states.get_event_sequence_type(),
                    svc_sequence_to_str(fsm_states.svc_sequence()).c_str(),
                    fsm_states.timeout(),
                    ipc_svc_dom_id_to_name(fsm_states.domain()));

    SDK_ASSERT (fsm_states.is_discovery() == true);
    fsm_states.set_init_params(params);
    LOG_STAGE_STARTED(upg_stage2str(fsm_states.current_stage()));
    if (!execute_pre_hooks(fsm_states.current_stage())) {
        fsm_states.update_stage_progress(SVC_RSP_FAIL);
        execute_exit_script(get_exit_status());
        return SDK_RET_ERR;
    }
    send_discovery_event(IPC_SVC_DOM_ID_A, fsm_states.current_stage());
    return SDK_RET_OK;
}

static sdk_ret_t
load_pipeline_json(pt::ptree& tree, sdk::platform::upg_mode_t upg_mode)
{
    sdk_ret_t ret = SDK_RET_ERR;
    try
    {   std::string upg_gen_json = std::string(std::getenv("CONFIG_PATH"));
        upg_gen_json += "/gen/";

        if (sdk::platform::upgrade_mode_graceful(upg_mode)) {
            upg_gen_json += UPGRADE_GRACEFUL;
        } else {
            upg_gen_json += UPGRADE_HITLESS;
        }

        if (access(upg_gen_json.c_str(), F_OK) != -1)
        {
            pt::read_json(upg_gen_json, tree);
            UPG_TRACE_INFO("Loaded the config file %s successfully",
                           upg_gen_json.c_str());
            return SDK_RET_OK;
        } else {
            LOG_INVALID_OBJ("upgrade json ", upg_gen_json.c_str());
            return SDK_RET_ERR;
        }
    }
    catch (std::exception const& ex)
    {
        UPG_TRACE_ERR("Failed to read upgrade spec %s", ex.what());
    }
    return ret;
}

sdk_ret_t
init (fsm_init_params_t *params)
{
    SDK_ASSERT(params != NULL);

    pt::ptree tree;
    if (SDK_RET_OK != load_pipeline_json(tree, params->upg_mode)){
        UPG_TRACE_ERR("Failed to load json !");
        return SDK_RET_ERR;
    }

    if (SDK_RET_OK != init_svc(tree)){
        UPG_TRACE_ERR("Failed to init service objects !");
        return SDK_RET_ERR;
    }

    if (SDK_RET_OK != init_stage_transitions(tree)){
        UPG_TRACE_ERR("Failed to init stage transition table !");
        return SDK_RET_ERR;
    }

    if (params->interactive_mode == true) {
        entry_stage = params->entry_stage;
    } else {
        if (SDK_RET_OK != set_entry_stage(tree)) {
            UPG_TRACE_ERR("Failed to set entry stage !");
            return SDK_RET_ERR;
        }
    }

    if (SDK_RET_OK != set_event_sequence(tree)){
        UPG_TRACE_ERR("Failed to set default event sequence !");
        return SDK_RET_ERR;
    }

    if (SDK_RET_OK != init_fsm_stages(tree)){
        UPG_TRACE_ERR("Failed to set init stages !");
        return SDK_RET_ERR;
    }

    if (params->interactive_mode == true) {
        return init_interactive(params);
    } else {
        return init_fsm(params);
    }
}

}    // namespace upg
}    // namespace sdk
