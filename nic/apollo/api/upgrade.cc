//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include <sys/stat.h>
#include "nic/sdk/upgrade/include/ev.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/api/include/pds_event.hpp"
#include "nic/apollo/api/include/pds_upgrade.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/nicmgr/nicmgr.hpp"
#include "nic/apollo/upgrade/api/include/shmstore.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"

namespace api {

static inline std::string
module_name (uint32_t thread_id)
{
    std::string module;

    switch (thread_id) {
    case core::PDS_THREAD_ID_NICMGR:  return "nicmgr";
    case SDK_THREAD_ID_LINKMGR_CTRL:  return "linkmgr";
    case core::PDS_THREAD_ID_API:     return "pdsagent";
    default :
        PDS_TRACE_ERR("Invalid module version request, thread %u", thread_id);
        break;
    }
    SDK_ASSERT(0);
    return "";
}

// this function read the module versions from the files
static inline void
read_modules_version (sysinit_mode_t init_mode)
{
    std::string full_path;
    module_version_pair_t version_pair;
    module_version_t version = { 0 };
    uint32_t thr_ids[] = { core::PDS_THREAD_ID_NICMGR, core::PDS_THREAD_ID_API,
                           SDK_THREAD_ID_LINKMGR_CTRL };

    // decode current graceful version
    full_path = api::g_pds_state.cfg_path() + "/" + api::g_pds_state.pipeline() +
                    "/upgrade_cc_graceful_version.json";
    // TODO graceful_store_db = store_cfg_parse(full_path);
    // decode current hitless version
    full_path = api::g_pds_state.cfg_path() + "/" + api::g_pds_state.pipeline() +
                    "/upgrade_cc_hitless_version.json";
    // TODO hitless_store_db = store_cfg_parse(full_path);

    // if it is graceful/hitless boot, decode the previous version
    if (!sdk::platform::sysinit_mode_graceful(init_mode)) {
        full_path = upg_shmstore_persistent_path() +
                        "/upgrade_cc_graceful_version.json";
        // TODO graceful_store_prev_db = store_cfg_parse(full_path);
    } else if (!sdk::platform::sysinit_mode_hitless(init_mode)) {
        full_path = upg_shmstore_volatile_path_hitless() +
                        "/upgrade_cc_hitless_version.json";
        // TODO hitless_store_prev_db = store_cfg_parse(full_path);
    }
    // TODO : extract the above and insert the version.
    // right now insert the default version
    version.major = 1;
    version_pair = module_version_pair_t(version, version);
    for (uint32_t i = 0; i < sizeof(thr_ids)/sizeof(uint32_t); i++) {
        g_upg_state->insert_module_version(thr_ids[i], MODULE_VERSION_GRACEFUL,
                                           version_pair);
        g_upg_state->insert_module_version(thr_ids[i], MODULE_VERSION_HITLESS,
                                           version_pair);
    }
}

static void
stale_shmstores_remove (std::string path)
{
    DIR *dir;
    struct dirent *entry;
    uint32_t id;
    sdk::lib::shmstore *store;
    std::string name;

    PDS_TRACE_DEBUG("shmstore stale check, path %s", path.c_str());
    dir = opendir(path.c_str());
    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir))) {
        PDS_TRACE_DEBUG("shmstore stale check, name %s", entry->d_name);
        if (!strstr(entry->d_name, "upgdata")) {
            continue;
        }
        name = path + "/" + entry->d_name;
        for (id = PDS_SHMSTORE_ID_MIN + 1; id < PDS_SHMSTORE_ID_MAX; id++) {
            store = g_upg_state->backup_shmstore((pds_shmstore_id_t)id);
            if ((store != NULL) && (!strncmp(store->name(), name.c_str(),
                                             SHMSEG_NAME_MAX_LEN))) {
                break;
            }
            store = g_upg_state->restore_shmstore((pds_shmstore_id_t)id);
            if ((store != NULL) && (!strncmp(store->name(), name.c_str(),
                                             SHMSEG_NAME_MAX_LEN))) {
                break;
            }
        }
        if (id >= PDS_SHMSTORE_ID_MAX) { // not found
            PDS_TRACE_INFO("Removing unused shmstore file %s", name.c_str());
            remove(name.c_str());
        }
    }
    closedir(dir);
}

// remove all the un-used stale stores from the previous upgrade
// should be invoked only during a new upgrade request
void
upg_stale_shmstores_remove (void)
{
    stale_shmstores_remove(upg_shmstore_volatile_path_hitless());
    stale_shmstores_remove(upg_shmstore_volatile_path_graceful());
    stale_shmstores_remove(upg_shmstore_persistent_path());
}

static std::string
upg_shmstore_name (const char *name, module_version_t version, bool vstore)
{
    std::string fname = std::string(name);
    struct stat st = { 0 };

    if (version.major != 0) {
        fname = fname + "." + std::to_string(version.major) + "." +
            std::to_string(version.minor);
    }
    if (vstore) {
        if (stat(upg_shmstore_volatile_path_hitless().c_str(), &st) == 0) {
            return upg_shmstore_volatile_path_hitless() + "/" + fname;
        } else {
            return upg_shmstore_volatile_path_graceful() + "/" + fname;
        }
    } else {
        return upg_shmstore_persistent_path() + "/" + fname;
    }
}

static inline sdk_ret_t
upg_shmstore_create_ (pds_shmstore_id_t id, const char *name, size_t size,
                      module_version_t version, bool vstore)
{
    sdk::lib::shmstore *store;
    std::string fname;
    sdk_ret_t ret;

    fname = upg_shmstore_name(name, version, vstore);
    store = sdk::lib::shmstore::factory();
    ret = store->create(fname.c_str(), size);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade store create failed for id %u, ret %u", id, ret);
        return ret;
    }
    api::g_upg_state->insert_backup_shmstore(id, store);
    return ret;
}

static inline sdk_ret_t
upg_shmstore_open_ (pds_shmstore_id_t id, const char *name,
                    module_version_t version, bool vstore,
                    bool rw_mode = false)
{
    sdk::lib::shmstore *store;
    std::string fname;
    sdk_ret_t ret;

    fname = upg_shmstore_name(name, version, vstore);
    store = sdk::lib::shmstore::factory();
    ret = store->open(fname.c_str(), rw_mode ? sdk::lib::SHM_OPEN_ONLY :
                                               sdk::lib::SHM_OPEN_READ_ONLY);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade store open failed for id %u, ret %u", id, ret);
        return ret;
    }
    api::g_upg_state->insert_restore_shmstore(id, store);
    return ret;
}

// below are for config objects. so need to create only when a backup request
// comes from upgrade manager
sdk_ret_t
upg_shmstore_create (sysinit_mode_t mode, bool vstore)
{
    static bool done = false;
    sdk_ret_t ret;
    module_version_t version = { 0 }; // invalid version

    if (done) {
        return SDK_RET_OK;
    }

    // create all the required backup files in store
    if (sdk::platform::sysinit_mode_hitless(mode)) {
        // agent config store
        ret = upg_shmstore_create_(PDS_AGENT_CFG_SHMSTORE_ID,
                                   upg_cfg_shmstore_name("pdsagent").c_str(),
                                   upg_cfg_shmstore_size("pdsagent"),
                                   version, vstore);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // nicmgr config store
    ret = upg_shmstore_create_(PDS_NICMGR_CFG_SHMSTORE_ID,
                               upg_cfg_shmstore_name("nicmgr").c_str(),
                               upg_cfg_shmstore_size("nicmgr"),
                               version, vstore);
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
    module_version_t version = { 0 }; // invalid version

    if (done) {
        return SDK_RET_OK;
    }

    // create all the required backup files in store
    if (sdk::platform::sysinit_mode_hitless(mode)) {
        // agent config store
        ret = upg_shmstore_open_(PDS_AGENT_CFG_SHMSTORE_ID,
                                 upg_cfg_shmstore_name("pdsagent").c_str(),
                                 version, vstore);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // nicmgr config store
    ret = upg_shmstore_open_(PDS_NICMGR_CFG_SHMSTORE_ID,
                             upg_cfg_shmstore_name("nicmgr").c_str(),
                             version, vstore);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    done = true;
    return ret;
}

static sdk_ret_t
upg_oper_shmstore_create (uint32_t thread_id, pds_shmstore_id_t id,
                          const char *name, size_t size, sysinit_mode_t mode)
{
    sdk_ret_t ret;
    bool rw_mode = false;
    bool vstore = true;
    module_version_t curr_version, prev_version;
    (void)mode;

    // modules which are following inline shared memory state model, requires
    // two stores during bring up itself
    // on hitless bringup,
    //  B shares the store with A if the module version are same
    //  B opens A store and do a copy and reconcile model if versions not match
    // on graceful bringup, opens A store and uses it

    // oper states always uses hitless version irrespective of the bootup and
    // upgrade mode
    std::tie(curr_version, prev_version) = g_upg_state->module_version(
                                              thread_id, MODULE_VERSION_HITLESS);
    PDS_TRACE_DEBUG("Upgrade, module version for thread %u, current %u.%u, "
                    "prev %u.%u", thread_id,
                    curr_version.major, curr_version.minor,
                    prev_version.major, prev_version.minor);
    if (sdk::platform::sysinit_mode_hitless(mode) ||
        sdk::asic::asic_is_soft_init()) {
        if (curr_version.version == prev_version.version) {
            rw_mode = true;
        }
        ret = upg_shmstore_open_(id, name, prev_version, vstore, rw_mode);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // not sharing the store
    if (rw_mode == false) {
        ret = upg_shmstore_create_(id, name, size, curr_version, vstore);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    return ret;
}

sdk_ret_t
linkmgr_shmstore_create (sysinit_mode_t mode)
{
    return upg_oper_shmstore_create(SDK_THREAD_ID_LINKMGR_CTRL,
                                    PDS_LINKMGR_OPER_SHMSTORE_ID,
                                    upg_oper_shmstore_name("linkmgr").c_str(),
                                    upg_oper_shmstore_size("linkmgr"), mode);
}

sdk_ret_t
nicmgr_shmstore_create (sysinit_mode_t mode)
{
    return upg_oper_shmstore_create(core::PDS_THREAD_ID_NICMGR,
                                    PDS_NICMGR_OPER_SHMSTORE_ID,
                                    upg_oper_shmstore_name("nicmgr").c_str(),
                                    upg_oper_shmstore_size("nicmgr"), mode);
}

sdk_ret_t
api_shmstore_create (sysinit_mode_t mode)
{
    return upg_oper_shmstore_create(core::PDS_THREAD_ID_API,
                                    PDS_AGENT_OPER_SHMSTORE_ID,
                                    upg_oper_shmstore_name("pdsagent").c_str(),
                                    upg_oper_shmstore_size("pdsagent"), mode);
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
            upg_shmstore_volatile_path_hitless().c_str(), false);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Upgrade hitless memory offset failed");
            return ret;
        }
    }

    // extract the version
    read_modules_version(mode);

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
        PDS_TRACE_ERR("Upgrade MS graceful/hitless init failed");
        return ret;
    }

    ret = linkmgr_shmstore_create(mode);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade linkmgr shmstore create failed");
        return ret;
    }

    ret = nicmgr_shmstore_create(mode);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade nicmgr shmstore create failed");
        return ret;
    }

    ret = api_shmstore_create(mode);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade api shmstore create failed");
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
upg_soft_init (pds_init_params_t *params)
{
    sysinit_mode_t mode;
    sdk_ret_t ret;

    // initialize upgrade state and call the upgade compatibitly checks
    if ((g_upg_state = upg_state::factory(params)) == NULL) {
        PDS_TRACE_ERR("Upgrade state creation failed");
        return SDK_RET_OOM;
    }

    mode = sdk::upg::init_mode();
    // soft inited applications such as vpp, needs the upgraded
    // mem regions for its libraries, ex ftl
    if (sdk::platform::sysinit_mode_hitless(mode)) {
        // offset the memory regions based on the regions in use
        ret = g_pds_state.mempartition()->upgrade_hitless_offset_regions(
            upg_shmstore_volatile_path_hitless().c_str(), false);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Upgrade hitless memory offset failed");
            return ret;
        }
    }
    // extract the module versions
    read_modules_version(mode);

    ret = nicmgr_shmstore_create(mode);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade pds nicmgr shmstore create failed");
        return ret;
    }

    return SDK_RET_OK;
}

sdk_ret_t
upg_restore_api_objs (void)
{
    if (sdk::platform::sysinit_mode_hitless(g_upg_state->init_mode())) {
        return upg_hitless_restore_api_objs();
    }
    return SDK_RET_OK;
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
    ev->compat_check_hdlr = UPGRADE_EV_HDLR_INVALID;
    ev->start_hdlr = pds_upgrade_start_hdlr;
    ev->backup_hdlr = pds_upgrade;
    ev->prepare_hdlr = pds_upgrade;
    ev->pre_switchover_hdlr = pds_upgrade;
    ev->switchover_hdlr = pds_upgrade;
    ev->rollback_hdlr = pds_upgrade;
    ev->ready_hdlr = UPGRADE_EV_HDLR_INVALID;
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
