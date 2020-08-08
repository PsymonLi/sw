//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines the global and per thread initialisation APIs
///
//----------------------------------------------------------------------------


#ifndef __PDS_INIT_H__
#define __PDS_INIT_H__

#include "pds_base.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum pds_init_mode_ce {
    PDS_CINIT_MODE_NONE,            ///< invalid mode
    PDS_CINIT_MODE_COLD_START,      ///< initialize from scratch
    PDS_CINIT_MODE_POST_UPGRADE     ///< read from any state if preserved previously
} pds_cinit_mode_t;


#define PDS_FLAG_INIT_TYPE_HARD 1
#define PDS_FLAG_INIT_TYPE_SOFT 2

/// \brief Initialization parameters
typedef struct pds_cinit_params_s {
    pds_cinit_mode_t           init_mode;       ///< mode of initialization
    void                       *trace_cb;         ///< callback for trace msgs
    uint32_t                   flags;
    pid_t                      flow_age_pid;   ///< designated process to do flow aging
} pds_cinit_params_t;

/// \brief     Global init
//  \param[in] params pds init parameters
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    This needs to be called precisely once by application
///            from the control core
pds_ret_t pds_global_init(pds_cinit_params_t *params);

/// \brief     Per thread init
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    A valid core id should be passed
///            This needs to be called on every data core of the application
pds_ret_t pds_thread_init(uint32_t core_id);

/// \brief     Global teardown
/// \remark    This needs to be called precisely once by application
///            from the control core after threads are stopped
void pds_global_teardown(void);

/// \brief Mfg mode parameters 
#define PDS_MFG_TEST_NUM_VLANS 2

typedef struct pds_mfg_mode_params_s {
    uint16_t vlans[PDS_MFG_TEST_NUM_VLANS];
} pds_mfg_mode_params_t;

/// \brief      Setup operations for mfg mode 
/// \param[in]  params parameters for setup of mfg mode
/// \return     #PDS_RET_OK on success, failure status code on error 
/// \remark     This currently programs the NACLs needed to setup forwarding 
///             for mfg mode testing
pds_ret_t pds_mfg_mode_setup(pds_mfg_mode_params_t *params);

/// \brief      Teardown operations for mfg mode 
/// \param[in]  params parameters for teardown of mfg mode
/// \return     #PDS_RET_OK on success, failure status code on error 
/// \remark     params passed to this call should be identical to the setup call
pds_ret_t pds_mfg_mode_teardown(pds_mfg_mode_params_t *params);

#ifdef __cplusplus
}
#endif

#endif // __PDS_INIT_H__
