//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines upgrade shared memory store for PDS
///
//----------------------------------------------------------------------------

#ifndef __UPGRADE_SHMSTORE_HPP__
#define __UPGRADE_SHMSTORE_HPP__

// two types of stores, persistent and volatile
#define UPGRADE_SHMSTORE_TYPE_MAX   2

// ----------------------------------------------------------------------------
// upgrade persistent store directory
// ----------------------------------------------------------------------------
#define PDS_UPGRADE_SHMSTORE_PPATH                "/update"

// ----------------------------------------------------------------------------
// upgrade volatile store directory.
// ----------------------------------------------------------------------------
#define PDS_UPGRADE_SHMSTORE_VPATH_GRACEFUL      "/dev/shm"
#define PDS_UPGRADE_SHMSTORE_VPATH_HITLESS      "/share"

// ----------------------------------------------------------------------------
// pds agent upgrade store for config objects
// ----------------------------------------------------------------------------
#define PDS_AGENT_UPGRADE_CFG_SHMSTORE_NAME         "pds_agent_upgdata"
#define PDS_AGENT_UPGRADE_CFG_SHMSTORE_SIZE         (2 * 1024 * 1024)  // 2MB

// pds agent config objects. these will be allocated from pds agent store
#define PDS_AGENT_UPGRADE_SHMSTORE_OBJ_SEG_NAME    "pds_agent_upgobjs"
#define PDS_AGENT_UPGRADE_SHMSTORE_OBJ_SEG_SIZE    (1 * 1024 * 1024)

// ----------------------------------------------------------------------------
// pds nicmgr upgrade store for config objects
// ----------------------------------------------------------------------------
#define PDS_NICMGR_UPGRADE_CFG_SHMSTORE_NAME       "pds_nicmgr_upgdata"
#define PDS_NICMGR_UPGRADE_CFG_SHMSTORE_SIZE       (30 * 1024)  // 30K

// pds nicmgr config objects. these will be allocated from pds nicmgr store
#define PDS_NICMGR_UPGRADE_SHMSTORE_OBJ_SEG_NAME   "pds_nicmgr_upgobjs"
#define PDS_NICMGR_UPGRADE_SHMSTORE_OBJ_SEG_SIZE   (20 * 1024)

// ----------------------------------------------------------------------------
// pds nicmgr upgrade store for device and lif operational states
// ----------------------------------------------------------------------------
#define PDS_NICMGR_UPGRADE_OPER_SHMSTORE_NAME      "pds_nicmgr_oper_upgdata"
#define PDS_NICMGR_UPGRADE_OPER_SHMSTORE_SIZE      (100 * 1024)  // 100K

// ----------------------------------------------------------------------------
// pds linkmgr upgrade store for port operational states
// ----------------------------------------------------------------------------
#define PDS_LINKMGR_UPGRADE_OPER_SHMSTORE_NAME     "pds_linkmgr_oper_upgdata"
#define PDS_LINKMGR_UPGRADE_OPER_SHMSTORE_SIZE     (30 * 1024)  // 30K

#endif    // __UPGRADE_SHMSTORE_HPP__
