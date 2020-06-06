//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/include/upgrade_shmstore.hpp"
#include "nic/apollo/api/include/pds_upgrade.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/nicmgr/nicmgr.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"

namespace api {


sdk_ret_t
upg_init (pds_init_params_t *params)
{
    sdk_ret_t ret;
    upg_mode_t mode;
    sdk::upg::upg_dom_t dom;

    mode = sdk::upg::upg_init_mode();
    dom = sdk::upg::upg_init_domain();

    PDS_TRACE_DEBUG("Setting bootup upgrade mode to %s",
                    mode == upg_mode_t::UPGRADE_MODE_NONE ? "NONE" :
                    mode == upg_mode_t::UPGRADE_MODE_GRACEFUL ? "Graceful" :
                    "Hitless");

    PDS_TRACE_DEBUG("Setting bootup domain to %s",
                    dom == sdk::upg::upg_dom_t::UPGRADE_DOMAIN_NONE ? "NONE" :
                    dom == sdk::upg::upg_dom_t::UPGRADE_DOMAIN_A ? "Dom_A" :
                    "Dom_B");

    // initialize upgrade state and call the upgade compatibitly checks
    if ((g_upg_state = upg_state::factory(params)) == NULL) {
        PDS_TRACE_ERR("Upgrade state creation failed");
        return SDK_RET_OOM;
    }

    // save upgrade init mode and domain
    g_upg_state->set_upg_init_mode(mode);
    g_upg_state->set_upg_init_domain(dom);

    // offset the memory regions based on the regions in use
    if (sdk::platform::upgrade_mode_hitless(mode)) {
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

    return SDK_RET_OK;
}

static std::string
upg_shmstore_name (const char *name, upg_mode_t mode, bool create)
{
    std::string fname = std::string(name);

    // for hitless, open if shm_create is false (shared from A to B). create if
    // shm_create is true (new states on B). cannot use the same name as A, as A
    // need to reuse this if there is an upgrade failure
    if (sdk::platform::upgrade_mode_hitless(mode)) {
        sdk::upg::upg_dom_t dom = sdk::upg::upg_init_domain();

        if ((create && sdk::upg::upg_domain_b(dom)) ||
            (!create && sdk::upg::upg_domain_a(dom))) {
            fname = fname + "_dom_b";
        }
    }
    return std::string(PDS_UPGRADE_SHMSTORE_DIR_NAME) + "/" + fname;
}


static inline sdk_ret_t
upg_shmstore_create_ (uint32_t thread_id, const char *name, size_t size,
                      upg_mode_t mode)
{
    sdk::lib::shmstore *store;
    std::string fname;
    sdk_ret_t ret;

    // agent store
    fname = upg_shmstore_name(name, mode, true);
    store = sdk::lib::shmstore::factory();
    ret = store->create(fname.c_str(), size);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade store create failed for thread %u, ret %u",
                      thread_id, ret);
        return ret;
    }
    api::g_upg_state->insert_upg_shmstore(thread_id,
                                          UPGRADE_SVC_SHMSTORE_TYPE_BACKUP,
                                          store);
    return ret;
}

static inline sdk_ret_t
upg_shmstore_open_ (uint32_t thread_id, const char *name, upg_mode_t mode)
{
    sdk::lib::shmstore *store;
    std::string fname;
    sdk_ret_t ret;

    // agent store
    fname = upg_shmstore_name(name, mode, false);
    store = sdk::lib::shmstore::factory();
    ret = store->open(fname.c_str());
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade store open failed for thread %u, ret %u",
                      thread_id, ret);
        return ret;
    }
    api::g_upg_state->insert_upg_shmstore(thread_id,
                                          UPGRADE_SVC_SHMSTORE_TYPE_RESTORE,
                                          store);
    return ret;
}

sdk_ret_t
upg_shmstore_create (upg_mode_t mode)
{
    static bool done = false;
    sdk_ret_t ret;

    if (done) {
        return SDK_RET_OK;
    }

    // create all the required backup files in store
    if (upgrade_mode_hitless(mode)) {
        // agent store
        ret = upg_shmstore_create_(core::PDS_THREAD_ID_API,
                                   PDS_AGENT_UPGRADE_SHMSTORE,
                                   PDS_AGENT_UPGRADE_SHMSTORE_SIZE, mode);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // nicmgr store
    ret = upg_shmstore_create_(core::PDS_THREAD_ID_NICMGR,
                               PDS_NICMGR_UPGRADE_SHMSTORE,
                               PDS_NICMGR_UPGRADE_SHMSTORE_SIZE, mode);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    done = true;
    return ret;
}

sdk_ret_t
upg_shmstore_open (upg_mode_t mode)
{
    static bool done = false;
    sdk_ret_t ret;

    if (done) {
        return SDK_RET_OK;
    }

    // create all the required backup files in store
    if (upgrade_mode_hitless(mode)) {
        // agent store
        ret = upg_shmstore_open_(core::PDS_THREAD_ID_API,
                                 PDS_AGENT_UPGRADE_SHMSTORE, mode);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // nicmgr store
    ret = upg_shmstore_open_(core::PDS_THREAD_ID_NICMGR,
                             PDS_NICMGR_UPGRADE_SHMSTORE, mode);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    done = true;
    return ret;
}

}   // namespace api
