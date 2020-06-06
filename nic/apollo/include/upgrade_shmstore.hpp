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

// ----------------------------------------------------------------------------
// upgrade store directory
// ----------------------------------------------------------------------------
#define PDS_UPGRADE_SHMSTORE_DIR_NAME              "/update"

// ----------------------------------------------------------------------------
// pds agent upgrade store
// ----------------------------------------------------------------------------
#define PDS_AGENT_UPGRADE_SHMSTORE                 "pds_agent_upgdata"
#define PDS_AGENT_UPGRADE_SHMSTORE_SIZE            (2 * 1024 * 1024)  // 2MB

// pds agent config objects. these will be allocated from pds agent store
#define PDS_AGENT_UPGRADE_SHMSTORE_OBJ_SEG         "pds_agent_upgobjs"
#define PDS_AGENT_UPGRADE_SHMSTORE_OBJ_SEG_SIZE    (1 * 1024 * 1024)

// ----------------------------------------------------------------------------
// pds nicmgr upgrade store
// ----------------------------------------------------------------------------
#define PDS_NICMGR_UPGRADE_SHMSTORE                "pds_nicmgr_upgdata"
#define PDS_NICMGR_UPGRADE_SHMSTORE_SIZE           (30 * 1024)  // 30K

// pds nicmgr config objects. these will be allocated from pds nicmgr store
#define PDS_NICMGR_UPGRADE_SHMSTORE_OBJ_SEG        "pds_nicmgr_upgobjs"
#define PDS_NICMGR_UPGRADE_SHMSTORE_OBJ_SEG_SIZE   (20 * 1024)

// ----------------------------------------------------------------------------
// extend other modules here
// ----------------------------------------------------------------------------


#endif    // __UPGRADE_SHMSTORE_HPP__
