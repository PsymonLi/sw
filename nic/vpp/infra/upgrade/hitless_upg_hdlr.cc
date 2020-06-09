//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include "infra/api/intf.h"
#include "log.hpp"
#include "hitless_upg_hdlr.hpp"

extern "C" {
    extern int repl_state_tp_server_init(void);
    extern int repl_state_tp_client_init(void);
}

using namespace sdk::upg;

sdk_ret_t
vpp_hitless_upg_ev_hdlr (upg_ev_params_t *params)
{
    sdk_ret_t ret = SDK_RET_OK;
    int i;

    switch(params->id) {
    case UPG_EV_START:
        // Domain 'A'
        // Initialize the sync server 
        upg_log_notice("%s: Initializing the inter domain UIPC server\n",
                       __FUNCTION__);
        i = repl_state_tp_server_init();
        if (i == 0) {
            ret = SDK_RET_OK;
        } else {
            ret = SDK_RET_ERR;
        }
        break;

    case UPG_EV_SYNC:
        // Domain 'B'.
        // Initialize the sync clinet 
        upg_log_notice("%s: Initializing the inter domain UIPC client\n",
                       __FUNCTION__);
        i = repl_state_tp_client_init();
        if (i == 0) {
            ret = SDK_RET_OK;
        } else {
            ret = SDK_RET_ERR;
        }
        break;

    case UPG_EV_COMPAT_CHECK:
    case UPG_EV_BACKUP:
    case UPG_EV_PREPARE:
    case UPG_EV_CONFIG_REPLAY:
    case UPG_EV_PRE_SWITCHOVER:
    case UPG_EV_SWITCHOVER:
    case UPG_EV_READY:
    case UPG_EV_PRE_RESPAWN:
    case UPG_EV_RESPAWN:
    case UPG_EV_ROLLBACK:
    case UPG_EV_REPEAL:
    case UPG_EV_FINISH:
        break;

    default:
        //  Should provide default handler. Otherwise we may miss
        //  adding handlers for new events.
        upg_log_error("Upgrade handler for event %s not implemented",
                      sdk::upg::upg_event2str(params->id));
        SDK_ASSERT(0);
    }
    ret = SDK_RET_OK; // TODO: Returning ok now - need to test other services
    return ret;
}

