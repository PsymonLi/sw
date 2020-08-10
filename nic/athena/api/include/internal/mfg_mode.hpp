//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// mfg mode related API 
///
//----------------------------------------------------------------------------

#ifndef __MFG_MODE_HPP__
#define __MFG_MODE_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/athena/api/include/pds_init.h"

sdk_ret_t
athena_program_mfg_mode_nacls(pds_mfg_mode_params_t *params);
sdk_ret_t
athena_unprogram_mfg_mode_nacls(pds_mfg_mode_params_t *params);
    
#endif    //__MFG_MODE_HPP__
