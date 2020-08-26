//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// upgrade thread implementation
///
//----------------------------------------------------------------------------

#ifndef __INTERNAL_UPGRADE_HPP__
#define __INTERNAL_UPGRADE_HPP__

#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/include/sdk/platform.hpp"

namespace event = sdk::event_thread;

namespace api {

// callback function invoked during upgrade thread initialization
void upgrade_thread_init_fn(void *ctxt);

// callback function invoked during upgrade thread exit
void upgrade_thread_exit_fn(void *ctxt);

// callback function invoked to process events received
void upgrade_thread_event_cb(void *msg, void *ctxt);

// module version pair having current and previous version
typedef std::pair<module_version_t, module_version_t> module_version_pair_t;

// store id - this can be used by threads/modules to get the store pointer
typedef enum pds_shmstore_id_e {
    PDS_SHMSTORE_ID_MIN        = 0,
    PDS_NICMGR_CFG_SHMSTORE_ID,
    PDS_NICMGR_OPER_SHMSTORE_ID,
    PDS_AGENT_CFG_SHMSTORE_ID,
    PDS_AGENT_OPER_SHMSTORE_ID,
    PDS_LINKMGR_OPER_SHMSTORE_ID,
    PDS_SHMSTORE_ID_MAX,
} pds_shmstore_id_t;

// pstate header info. common for pds pstates
typedef struct pstate_meta_info_s {
    module_version_t version;
    uint64_t rsvd[4];
} __PACK__ pstate_meta_info_t;

// module names for using shmstore and versions
#define PDS_NICMGR_MODULE_NAME   "nicmgr"
#define PDS_AGENT_MODULE_NAME    "pdsagent"
#define PDS_LINKMGR_MODULE_NAME  "linkmgr"

}    // namespace api

#endif    // __INTERNAL_UPGRADE_HPP__
