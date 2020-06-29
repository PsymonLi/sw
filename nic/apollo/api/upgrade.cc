//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include <sys/stat.h>
#include "nic/sdk/upgrade/include/ev.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/api/include/pds_event.hpp"
#include "nic/apollo/include/upgrade_shmstore.hpp"
#include "nic/apollo/api/include/pds_upgrade.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/nicmgr/nicmgr.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"

namespace api {

static inline void
read_module_version (uint32_t thread_id)
{
    module_version_t version = { 0 };

    // TODO: fill by reading json.
    // need to map the thread-id to json module version-key
    version.major = 1;
    g_upg_state->insert_module_version(thread_id, version);
    g_upg_state->insert_module_prev_version(thread_id, version);
}

static std::string
upg_shmstore_name (uint32_t thread_id, const char *name, sysinit_mode_t mode,
                   bool curr_mod_version, bool vstore)
{
    std::string fname = std::string(name);
    module_version_t curr_version;
    module_version_t prev_version;
    struct stat st = { 0 };

    // versioned stores are used only for operational state objects and it is
    // present in volatile store
    if (vstore) {
        curr_version = g_upg_state->module_version(thread_id);
        prev_version = g_upg_state->module_prev_version(thread_id);
        if (curr_mod_version) {
            fname = fname + "." + std::to_string(curr_version.major) + "." +
                std::to_string(curr_version.minor);
        } else {
            fname = fname + "." + std::to_string(prev_version.major) + "." +
                std::to_string(prev_version.minor);
        }
        if (stat(PDS_UPGRADE_SHMSTORE_VPATH_HITLESS, &st) == 0) {
            return std::string(PDS_UPGRADE_SHMSTORE_VPATH_HITLESS) + "/" + fname;
        } else {
            return std::string(PDS_UPGRADE_SHMSTORE_VPATH_GRACEFUL) + "/" + fname;
        }
    } else {
        return std::string(PDS_UPGRADE_SHMSTORE_PPATH) + "/" + fname;
    }
}

static inline sdk_ret_t
upg_shmstore_create_ (uint32_t thread_id, const char *name, size_t size,
                      sysinit_mode_t mode, bool vstore)
{
    sdk::lib::shmstore *store;
    std::string fname;
    sdk_ret_t ret;

    fname = upg_shmstore_name(thread_id, name, mode, true, vstore);
    store = sdk::lib::shmstore::factory();
    ret = store->create(fname.c_str(), size);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade store create failed for thread %u, ret %u",
                      thread_id, ret);
        return ret;
    }
    api::g_upg_state->insert_backup_shmstore(thread_id, vstore, store);
    return ret;
}

static inline sdk_ret_t
upg_shmstore_open_ (uint32_t thread_id, const char *name, sysinit_mode_t mode,
                    bool vstore, bool rw_mode = false)
{
    sdk::lib::shmstore *store;
    std::string fname;
    sdk_ret_t ret;

    // agent store
    fname = upg_shmstore_name(thread_id, name, mode, false, vstore);
    store = sdk::lib::shmstore::factory();
    ret = store->open(fname.c_str(), rw_mode ? sdk::lib::SHM_OPEN_ONLY :
                                               sdk::lib::SHM_OPEN_READ_ONLY);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade store open failed for thread %u, ret %u",
                      thread_id, ret);
        return ret;
    }
    api::g_upg_state->insert_restore_shmstore(thread_id, vstore, store);
    return ret;
}

// below are for config objects. so need to create only when a backup request
// comes from upgrade manager
sdk_ret_t
upg_shmstore_create (sysinit_mode_t mode, bool vstore)
{
    static bool done = false;
    sdk_ret_t ret;

    if (done) {
        return SDK_RET_OK;
    }

    // create all the required backup files in store
    if (sdk::platform::sysinit_mode_hitless(mode)) {
        // agent config store
        ret = upg_shmstore_create_(core::PDS_THREAD_ID_API,
                                   PDS_AGENT_UPGRADE_CFG_SHMSTORE_NAME,
                                   PDS_AGENT_UPGRADE_CFG_SHMSTORE_SIZE,
                                   mode, vstore);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // nicmgr config store
    ret = upg_shmstore_create_(core::PDS_THREAD_ID_NICMGR,
                               PDS_NICMGR_UPGRADE_CFG_SHMSTORE_NAME,
                               PDS_NICMGR_UPGRADE_CFG_SHMSTORE_SIZE,
                               mode, vstore);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    done = true;
    return ret;
}

sdk_ret_t
upg_shmstore_open (sysinit_mode_t mode, bool vstore)
{
    static bool done = false;
    sdk_ret_t ret;

    if (done) {
        return SDK_RET_OK;
    }

    // create all the required backup files in store
    if (sdk::platform::sysinit_mode_hitless(mode)) {
        // agent config store
        ret = upg_shmstore_open_(core::PDS_THREAD_ID_API,
                                 PDS_AGENT_UPGRADE_CFG_SHMSTORE_NAME,
                                 mode, vstore);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // nicmgr config store
    ret = upg_shmstore_open_(core::PDS_THREAD_ID_NICMGR,
                             PDS_NICMGR_UPGRADE_CFG_SHMSTORE_NAME,
                             mode, vstore);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    done = true;
    return ret;
}

static sdk_ret_t
upg_oper_shmstore_create (uint32_t thread_id, const char *name, size_t size,
                          sysinit_mode_t mode)
{
    sdk_ret_t ret;
    bool rw_mode = false;
    bool vstore = true;
    module_version_t curr_version, prev_version;

    read_module_version(thread_id);
    // linkmgr follow inline shared memory state model. so need two
    // stores during bring up itself
    // on hitless bringup,
    //  B shares the store with A if the module version are same
    //  B opens A store and do a copy and reconcile model if versions not match
    // on graceful bringup, opens A store and uses it
    PDS_TRACE_DEBUG("Upgrade, module version for thread %u, current %u.%u, "
                    "prev %u.%u", thread_id,
                    g_upg_state->module_version(thread_id).major,
                    g_upg_state->module_version(thread_id).minor,
                    g_upg_state->module_prev_version(thread_id).major,
                    g_upg_state->module_prev_version(thread_id).minor);
    if (sdk::platform::sysinit_mode_hitless(mode)) {
        curr_version = g_upg_state->module_version(thread_id);
        prev_version = g_upg_state->module_prev_version(thread_id);
        if (curr_version.version == prev_version.version) {
            rw_mode = true;
        }
        ret = upg_shmstore_open_(thread_id, name, mode, vstore, rw_mode);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // not sharing the store
    if (rw_mode == false) {
        ret = upg_shmstore_create_(thread_id, name, size, mode, vstore);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    return ret;
}

sdk_ret_t
linkmgr_shmstore_create (sysinit_mode_t mode)
{
    return upg_oper_shmstore_create(SDK_IPC_ID_LINKMGR_CTRL,
                                    PDS_LINKMGR_UPGRADE_OPER_SHMSTORE_NAME,
                                    PDS_LINKMGR_UPGRADE_OPER_SHMSTORE_SIZE, mode);
}

sdk_ret_t
nicmgr_shmstore_create (sysinit_mode_t mode)
{
    return upg_oper_shmstore_create(core::PDS_THREAD_ID_NICMGR,
                                    PDS_NICMGR_UPGRADE_OPER_SHMSTORE_NAME,
                                    PDS_NICMGR_UPGRADE_OPER_SHMSTORE_SIZE, mode);
}

sdk_ret_t
upg_init (pds_init_params_t *params)
{
    sdk_ret_t ret;
    sysinit_mode_t mode;
    sysinit_dom_t dom;

    mode = sdk::upg::init_mode();
    dom = sdk::upg::init_domain();

    PDS_TRACE_DEBUG("Setting bootup upgrade mode to %s",
                    mode == sysinit_mode_t::SYSINIT_MODE_DEFAULT ? "Default" :
                    mode == sysinit_mode_t::SYSINIT_MODE_GRACEFUL ? "Graceful" :
                    "Hitless");

    PDS_TRACE_DEBUG("Setting bootup domain to %s",
                    dom == sysinit_dom_t::SYSINIT_DOMAIN_B ? "Dom_B" :
                    "Dom_A");

    // initialize upgrade state and call the upgade compatibitly checks
    if ((g_upg_state = upg_state::factory(params)) == NULL) {
        PDS_TRACE_ERR("Upgrade state creation failed");
        return SDK_RET_OOM;
    }

    // save upgrade init mode and domain
    g_upg_state->set_init_mode(mode);
    g_upg_state->set_init_domain(dom);

    // offset the memory regions based on the regions in use
    if (sdk::platform::sysinit_mode_hitless(mode)) {
        ret = g_pds_state.mempartition()->upgrade_hitless_offset_regions(
            api::g_pds_state.cfg_path().c_str(), false);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Upgrade hitless memory offset failed");
            return ret;
        }
    }

    // pds hal registers for upgrade events
    ret = upg_graceful_init(params);
    if (ret == SDK_RET_OK) {
        ret = upg_hitless_init(params);
    }
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade graceful/hitless init failed");
        return ret;
    }

    // nicmgr registers for upgrade events
    ret = nicmgr_upg_graceful_init();
    if (ret == SDK_RET_OK) {
        ret = nicmgr_upg_hitless_init();
    }
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade nicmgr graceful/hitless init failed");
        return ret;
    }

    // ms stub registers for upgrade events.
    ret = pds_ms::pds_ms_upg_hitless_init();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade pds ms graceful/hitless init failed");
        return ret;
    }

    ret = linkmgr_shmstore_create(mode);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade pds linkmgr shmstore create failed");
        return ret;
    }

    ret = nicmgr_shmstore_create(mode);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade pds nicmgr shmstore create failed");
        return ret;
    }

    // open the store of config objects
    if (!sdk::platform::sysinit_mode_default(mode)) {
        ret = upg_shmstore_open(mode);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
upg_restore_api_objs (void)
{
    // TODO @sunny fix the hang
    return SDK_RET_OK;

    if (sdk::platform::sysinit_mode_hitless(g_upg_state->init_mode())) {
        return upg_hitless_restore_api_objs();
    }
}

static sdk_ret_t
pds_upgrade_start_hdlr (sdk::upg::upg_ev_params_t *params)
{
    sdk_ret_t ret;
    pds_event_t pds_event;

    // notify the agent that firmware upgrade is starting so that it can
    // shutdown its external gRPC channel and stop accepting any new incoming
    // config
    if (sdk::platform::sysinit_mode_hitless(params->mode)) {
        pds_event.event_id = PDS_EVENT_ID_UPGRADE_START;
        ret = g_pds_state.event_notify(&pds_event);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    return pds_upgrade(params);
}

static sdk_ret_t
pds_upgrade_repeal_hdlr (sdk::upg::upg_ev_params_t *params)
{
    sdk_ret_t ret;
    pds_event_t pds_event;

    // notify the agent that firmware upgrade failed and it needs to re-open
    // its external gRPC to start accepting config
    if (sdk::platform::sysinit_mode_hitless(params->mode)) {
        pds_event.event_id = PDS_EVENT_ID_UPGRADE_ABORT;
        ret = g_pds_state.event_notify(&pds_event);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    return pds_upgrade(params);
}

static void
upg_ev_fill (sdk::upg::upg_ev_t *ev)
{
    ev->svc_ipc_id = core::PDS_THREAD_ID_UPGRADE;
    strncpy(ev->svc_name, "pdsagent", sizeof(ev->svc_name));
    ev->compat_check_hdlr = pds_upgrade;
    ev->start_hdlr = pds_upgrade_start_hdlr;
    ev->backup_hdlr = pds_upgrade;
    ev->prepare_hdlr = pds_upgrade;
    ev->pre_switchover_hdlr = pds_upgrade;
    ev->switchover_hdlr = pds_upgrade;
    ev->rollback_hdlr = pds_upgrade;
    ev->ready_hdlr = pds_upgrade;
    ev->config_replay_hdlr = pds_upgrade;
    ev->sync_hdlr = pds_upgrade;
    ev->repeal_hdlr = pds_upgrade_repeal_hdlr;
    ev->respawn_hdlr = pds_upgrade;
    ev->pre_respawn_hdlr = pds_upgrade;
    ev->finish_hdlr = pds_upgrade;
}

void
upgrade_thread_init_fn (void *ctxt)
{
    sdk::upg::upg_ev_t ev = { 0 };
    sdk_ret_t ret;
    sdk::upg::upg_ev_params_t *params = api::g_upg_state->ev_params();

    // register for all upgrade events
    upg_ev_fill(&ev);
    sdk::upg::upg_ev_hdlr_register(ev);

    ret = pds_upgrade(params);
    if (ret != SDK_RET_IN_PROGRESS) {
        params->response_cb(ret, params->response_cookie);
    }
}

void
upgrade_thread_exit_fn (void *ctxt)
{
    sdk::upg::upg_ev_hdlr_unregister();
}

void
upgrade_thread_event_cb (void *msg, void *ctxt)
{
}

}   // namespace api
