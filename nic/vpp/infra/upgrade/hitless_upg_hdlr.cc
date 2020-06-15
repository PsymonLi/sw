//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include "infra/api/intf.h"
#include "nic/vpp/infra/ipc/pdsa_vpp_hdlr.h"
#include "log.hpp"
#include "hitless_upg_hdlr.hpp"

extern "C" {
    extern int repl_state_tp_server_init(void);
    extern void repl_state_tp_server_stop(void);
    extern int repl_state_tp_client_init(void);
}

using namespace sdk::upg;

sdk_ret_t
vpp_hitless_upg_ev_hdlr (upg_ev_params_t *params)
{
    static bool hitless_upgrade_failure = false;
    sdk_ret_t ret = SDK_RET_OK;
    int i;

    switch(params->id) {
    case UPG_EV_START:
        // Domain 'A'
        // Initialize the sync server 
        upg_log_notice("%s: Initializing the inter domain UIPC server\n",
                       __FUNCTION__);
        i = repl_state_tp_server_init();
        if (i != 0) {
            ret = SDK_RET_ERR;
        }
        break;

    case UPG_EV_PREPARE:
        // Domain 'A'
        // Bring down all interfaces and suspend workers
        pds_infra_set_all_intfs_status(false);
        pds_vpp_set_suspend_resume_worker_threads(1);
        break;

    case UPG_EV_SYNC:
        // Domain 'B'.
        // Initialize the sync clinet 
        upg_log_notice("%s: Initializing the inter domain UIPC client\n",
                       __FUNCTION__);
        i = repl_state_tp_client_init();
        if (i != 0) {
            ret = SDK_RET_ERR;
        }
        upg_log_notice("%s: Inter domain UIPC client state sync: %d\n",
                       __FUNCTION__, ret);
        break;

    case UPG_EV_REPEAL:
        // Domain 'A'
        // Resume workers and bring up all interfaces
        upg_log_notice("%s: Upgrade attempt seem to have failed\n", __FUNCTION__);
        upg_log_notice("%s: Resuming worker threads, bringing up interfaces "
                       "and cleaning up\n", __FUNCTION__);
        pds_vpp_set_suspend_resume_worker_threads(0);
        pds_infra_set_all_intfs_status(true);
        // Stop the inter-domain IPC server
        repl_state_tp_server_stop();
        // mark hitless_upgrade_failure
        hitless_upgrade_failure = true;
        break;

    case UPG_EV_FINISH:
        // Domain 'A'
        // If the switchover from vpp's perspective was a success
        // detach from all interfaces
        if (!hitless_upgrade_failure) {
            // Remove all interfaces
            upg_log_notice("%s: Detaching from all interfaces\n", __FUNCTION__);
            pds_infra_remove_all_intfs();
            hitless_upgrade_failure = false;
        }
        break;

    case UPG_EV_COMPAT_CHECK:
    case UPG_EV_BACKUP:
    case UPG_EV_CONFIG_REPLAY:
    case UPG_EV_PRE_SWITCHOVER:
    case UPG_EV_SWITCHOVER:
    case UPG_EV_READY:
    case UPG_EV_PRE_RESPAWN:
    case UPG_EV_RESPAWN:
    case UPG_EV_ROLLBACK:
        break;

    default:
        //  Should provide default handler. Otherwise we may miss
        //  adding handlers for new events.
        upg_log_error("Upgrade handler for event %s not implemented",
                      sdk::upg::upg_event2str(params->id));
        SDK_ASSERT(0);
    }
    return ret;
}

