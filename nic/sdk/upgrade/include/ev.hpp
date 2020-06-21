//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines event handlers during process upgrade
///
//----------------------------------------------------------------------------

/// \defgroup UPGRADE Upgrade event handlers
/// \ingroup  UPG_EVENT_HDLR
/// @{
///

#ifndef __UPGRADE_EV_HPP__
#define __UPGRADE_EV_HPP__

#include "string.h"
#include "stdio.h"
#include "include/sdk/globals.hpp"
#include "upgrade/include/upgrade.hpp"
#include "lib/ipc/ipc.hpp"

namespace sdk {
namespace upg {

// event-ids offset from sdk upgmgr reserved ids
#define EV_ID_UPGMGR(off) (SDK_IPC_EVENT_ID_UPGMGR_MIN + off)
/// \brief upgrade event ids
/// ...WARNING...over the releases, should not change the order of it
/// as it breaks the backward compatibility. it should be extended
/// in the end
#define UPG_EV_ENTRIES(E)                                                   \
    E(UPG_EV_NONE,            EV_ID_UPGMGR(UPG_STAGE_NONE),            "")  \
    E(UPG_EV_COMPAT_CHECK,    EV_ID_UPGMGR(UPG_STAGE_COMPAT_CHECK),    "")  \
    E(UPG_EV_START,           EV_ID_UPGMGR(UPG_STAGE_START),           "")  \
    E(UPG_EV_BACKUP,          EV_ID_UPGMGR(UPG_STAGE_BACKUP),          "")  \
    E(UPG_EV_PREPARE,         EV_ID_UPGMGR(UPG_STAGE_PREPARE),         "")  \
    E(UPG_EV_SYNC,            EV_ID_UPGMGR(UPG_STAGE_SYNC),            "")  \
    E(UPG_EV_CONFIG_REPLAY,   EV_ID_UPGMGR(UPG_STAGE_CONFIG_REPLAY),   "")  \
    E(UPG_EV_PRE_SWITCHOVER,  EV_ID_UPGMGR(UPG_STAGE_PRE_SWITCHOVER), "")   \
    E(UPG_EV_SWITCHOVER,      EV_ID_UPGMGR(UPG_STAGE_SWITCHOVER),      "")  \
    E(UPG_EV_READY,           EV_ID_UPGMGR(UPG_STAGE_READY),           "")  \
    E(UPG_EV_PRE_RESPAWN,     EV_ID_UPGMGR(UPG_STAGE_PRE_RESPAWN),     "")  \
    E(UPG_EV_RESPAWN,         EV_ID_UPGMGR(UPG_STAGE_RESPAWN),         "")  \
    E(UPG_EV_ROLLBACK,        EV_ID_UPGMGR(UPG_STAGE_ROLLBACK),        "")  \
    E(UPG_EV_REPEAL,          EV_ID_UPGMGR(UPG_STAGE_REPEAL),          "")  \
    E(UPG_EV_FINISH,          EV_ID_UPGMGR(UPG_STAGE_FINISH),          "")  \
    E(UPG_EV_EXIT,            (SDK_IPC_EVENT_ID_UPGMGR_MAX - 1),       "")  \
    E(UPG_EV_MAX,             (SDK_IPC_EVENT_ID_UPGMGR_MAX),           "")

SDK_DEFINE_ENUM(upg_ev_id_t, UPG_EV_ENTRIES)
SDK_DEFINE_ENUM_TO_STR(upg_ev_id_t, UPG_EV_ENTRIES)
#undef UPG_EV_ENTRIES

// used in hitless upgrade
#define UPGRADE_DOMAIN(ENTRY)                                     \
    ENTRY(UPGRADE_DOMAIN_NONE,     0, "UPGRADE_DOMAIN_NONE")    \
    ENTRY(UPGRADE_DOMAIN_A,        1, "UPGRADE_DOMAIN_A")       \
    ENTRY(UPGRADE_DOMAIN_B,        2, "UPGRADE_DOMAIN_B")       \

SDK_DEFINE_ENUM(upg_dom_t,        UPGRADE_DOMAIN)
SDK_DEFINE_ENUM_TO_STR(upg_dom_t, UPGRADE_DOMAIN)
SDK_DEFINE_MAP_EXTERN(upg_dom_t,  UPGRADE_DOMAIN)
// #undef UPGRADE_DOMAIN

static inline bool
upg_domain_none (upg_dom_t dom)
{
    return dom == UPGRADE_DOMAIN_NONE;
}

static inline bool
upg_domain_a (upg_dom_t dom)
{
    return dom == UPGRADE_DOMAIN_A;
}

static inline bool
upg_domain_b (upg_dom_t dom)
{
    return dom == UPGRADE_DOMAIN_B;
}

/// \brief asynchronous response callback by the event handler
/// cookie is passed to the event handler and it should not be modified
typedef void (*upg_async_ev_response_cb_t)(sdk_ret_t status, const void *cookie);

static inline const char *
upg_event2str (upg_ev_id_t id)
{
    return UPG_EV_ENTRIES_str(id);
}

static inline upg_ev_id_t
upg_stage2event (upg_stage_t stage)
{
    if (stage >= UPG_STAGE_MAX) {
        SDK_ASSERT(0);
    }
    return (upg_ev_id_t)EV_ID_UPGMGR(stage);
}

// file setup by the system init module during bringup
// if it not set, regular boot is assumed
#define UPGRADE_INIT_MODE_FILE "/.upgrade_init_mode"
#define UPGRADE_INIT_DOM_FILE "/.upgrade_init_domain"

static inline upg_mode_t
upg_init_mode (const char *file = NULL)
{
    FILE *fp;
    upg_mode_t mode;
    char buf[32], *m = NULL;

    fp = file ? fopen(file, "r") : fopen(UPGRADE_INIT_MODE_FILE, "r");
    if (fp) {
        m = fgets(buf, sizeof(buf), fp);
        fclose(fp);
    }
    if (!m) {
        return upg_mode_t::UPGRADE_MODE_NONE;
    }
    if (strncmp(m, "graceful", strlen("graceful")) == 0) {
        mode = upg_mode_t::UPGRADE_MODE_GRACEFUL;
    } else if (strncmp(m, "hitless", strlen("hitless")) == 0) {
        mode = upg_mode_t::UPGRADE_MODE_HITLESS;
    } else if (strncmp(m, "none", strlen("none")) == 0) {
        mode = upg_mode_t::UPGRADE_MODE_NONE;
    } else {
        SDK_ASSERT(0);
    }
    return mode;
}

static inline upg_dom_t
upg_init_domain (void)
{
    FILE *fp;
    char buf[32], *m = NULL;
    upg_dom_t dom_id;
    std::string file = UPGRADE_INIT_DOM_FILE;

    if (upg_init_mode() != upg_mode_t::UPGRADE_MODE_HITLESS) {
        return upg_dom_t::UPGRADE_DOMAIN_NONE;
    }
    fp = fopen(file.c_str(), "r");
    SDK_ASSERT(fp != NULL);
    m = fgets(buf, sizeof(buf), fp);
    SDK_ASSERT(m != NULL);
    fclose(fp);

    if (strncmp(m, "dom_a", strlen("dom_a")) == 0) {
        dom_id = upg_dom_t::UPGRADE_DOMAIN_A;
    } else if (strncmp(m, "dom_b", strlen("dom_b")) == 0) {
        dom_id = upg_dom_t::UPGRADE_DOMAIN_B;
    } else {
        SDK_ASSERT(0);
    }
    return dom_id;
}

/// \brief upgrade event msg
typedef struct upg_ev_params_s {
    upg_ev_id_t id;                         ///< upgrade event id
    sdk::platform::upg_mode_t mode;         ///< upgrade mode
    upg_async_ev_response_cb_t response_cb; ///< response callback
    void *response_cookie;                  ///< response cookie
    void *svc_ctx;                          ///< service context, given during
                                            ///< registration
    // TODO other infos
} __PACK__ upg_ev_params_t;

/// \brief upgrade event handler
typedef sdk_ret_t (*upg_ev_hdlr_t)(upg_ev_params_t *params);

/// \brief upgrade event handlers for upgrade
typedef struct upg_ev_s {

    /// service name. used for debug trace only
    char svc_name[SDK_MAX_NAME_LEN];

    /// service ipc id
    uint32_t svc_ipc_id;

    /// service context
    void *svc_ctx;

    /// compat checks should be done here (on A)
    upg_ev_hdlr_t compat_check_hdlr;

    /// start of a new upgrade, mount check the existance of B should be
    /// done here (on A).
    upg_ev_hdlr_t start_hdlr;

    /// software states should be saved here (on A).
    upg_ev_hdlr_t backup_hdlr;

    /// new process spawning should be done here
    /// processs should be paused here to a safe point for switchover (on A)
    upg_ev_hdlr_t prepare_hdlr;

    /// hardware quiescing should be done here (on A)
    upg_ev_hdlr_t pre_switchover_hdlr;

    /// switching to B (on B)
    upg_ev_hdlr_t switchover_hdlr;

    /// rollback if there is failure (on B)
    upg_ev_hdlr_t rollback_hdlr;

    /// config replay (on B)
    upg_ev_hdlr_t config_replay_hdlr;

    /// operational table syncing (on B)
    upg_ev_hdlr_t sync_hdlr;

    /// respawn processes (on A)
    /// used in graceful to restart the processes with previously
    /// saved states, used for separating sysmgr and rest of teh services
    // respawn
    upg_ev_hdlr_t pre_respawn_hdlr;

    /// respawn processes (on A)
    /// used in graceful to restart the processes with previously
    /// saved states
    upg_ev_hdlr_t respawn_hdlr;

    /// making sure B bringup is successful (on B)
    upg_ev_hdlr_t ready_hdlr;

    /// repeal an upgrade (on A / B)
    /// undo what it has been done and go to safe state
    upg_ev_hdlr_t repeal_hdlr;

    /// finish the upgrade (on A / B)
    /// processs can do final cleanup
    upg_ev_hdlr_t finish_hdlr;

} upg_ev_t;

// registers both discovery(broadcast) and other events
void upg_ev_hdlr_register(upg_ev_t &ev);
void upg_ev_hdlr_unregister(void);

// application can register for specific event and invoke the
// specific handlers for non default processing
void upg_invoke_ev_hdlr(sdk::ipc::ipc_msg_ptr msg, const void *ctxt,
                        upg_ev_hdlr_t hdlr);

#undef EV_ID_UPGMGR

}   // namespace upg
}   // namespace sdk

using sdk::upg::upg_ev_id_t::UPG_EV_NONE;
using sdk::upg::upg_ev_id_t::UPG_EV_COMPAT_CHECK;
using sdk::upg::upg_ev_id_t::UPG_EV_START;
using sdk::upg::upg_ev_id_t::UPG_EV_BACKUP;
using sdk::upg::upg_ev_id_t::UPG_EV_PREPARE;
using sdk::upg::upg_ev_id_t::UPG_EV_PRE_SWITCHOVER;
using sdk::upg::upg_ev_id_t::UPG_EV_SWITCHOVER;
using sdk::upg::upg_ev_id_t::UPG_EV_READY;
using sdk::upg::upg_ev_id_t::UPG_EV_CONFIG_REPLAY;
using sdk::upg::upg_ev_id_t::UPG_EV_SYNC;
using sdk::upg::upg_ev_id_t::UPG_EV_ROLLBACK;
using sdk::upg::upg_ev_id_t::UPG_EV_REPEAL;
using sdk::upg::upg_ev_id_t::UPG_EV_PRE_RESPAWN;
using sdk::upg::upg_ev_id_t::UPG_EV_RESPAWN;
using sdk::upg::upg_ev_id_t::UPG_EV_FINISH;
using sdk::upg::upg_ev_id_t::UPG_EV_EXIT;
using sdk::upg::upg_ev_id_t::UPG_EV_MAX;

/// @}

#endif   // __UPGRADE_EV_HPP__
