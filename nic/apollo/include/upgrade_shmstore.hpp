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

// every segment consumes an additional 512 bytes in shmmgr
#define SHMMGR_SEGMENT_META_SIZE                    512

// ----------------------------------------------------------------------------
// upgrade persistent store directory
// ----------------------------------------------------------------------------
#define PDS_UPGRADE_SHMSTORE_PPATH                "/update"

// ----------------------------------------------------------------------------
// upgrade volatile store directory.
// ----------------------------------------------------------------------------
#define PDS_UPGRADE_SHMSTORE_VPATH_GRACEFUL      "/dev/shm"
#define PDS_UPGRADE_SHMSTORE_VPATH_HITLESS       "/share"

// ----------------------------------------------------------------------------
// pds agent upgrade store for config objects
// ----------------------------------------------------------------------------
#define PDS_AGENT_UPGRADE_CFG_SHMSTORE_NAME         "pds_agent_upgdata"
#define PDS_AGENT_UPGRADE_CFG_SHMSTORE_SIZE         (1 * 1024 * 1024)  // 1MB

// ----------------------------------------------------------------------------
// pds nicmgr upgrade store for config objects
// ----------------------------------------------------------------------------
#define PDS_NICMGR_UPGRADE_CFG_SHMSTORE_NAME       "pds_nicmgr_upgdata"
#define PDS_NICMGR_UPGRADE_CFG_SHMSTORE_SIZE       (32 * 1024)  // 32K

// ----------------------------------------------------------------------------
// pds agent upgrade store for service lif operational states
// ----------------------------------------------------------------------------
#define PDS_AGENT_UPGRADE_OPER_SHMSTORE_NAME      "pds_agent_oper_upgdata"
#define PDS_AGENT_UPGRADE_OPER_SHMSTORE_SIZE      (32 * 1024)  // 32K

// ----------------------------------------------------------------------------
// pds nicmgr upgrade store for device and lif operational states
// ----------------------------------------------------------------------------
#define PDS_NICMGR_UPGRADE_OPER_SHMSTORE_NAME      "pds_nicmgr_oper_upgdata"
#define PDS_NICMGR_UPGRADE_OPER_SHMSTORE_SIZE      (128 * 1024)  // 128K

// ----------------------------------------------------------------------------
// pds linkmgr upgrade store for port operational states
// ----------------------------------------------------------------------------
#define PDS_LINKMGR_UPGRADE_OPER_SHMSTORE_NAME     "pds_linkmgr_oper_upgdata"
#define PDS_LINKMGR_UPGRADE_OPER_SHMSTORE_SIZE     (32 * 1024)  // 32K

#endif    // __UPGRADE_SHMSTORE_HPP__
