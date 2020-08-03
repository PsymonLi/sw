//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines upgrade shared memory store API
///
//----------------------------------------------------------------------------

#ifndef __UPGRADE_API_INCLUDE_SHMSTORE_HPP__
#define __UPGRADE_API_INCLUDE_SHMSTORE_HPP__

#include <string>

namespace api {

std::string upg_shmstore_persistent_path(void);
std::string upg_shmstore_volatile_path_graceful(void);
std::string upg_shmstore_volatile_path_hitless(void);

int upg_shmstore_segment_meta_size();

std::string upg_cfg_shmstore_name(std::string module_name);
int upg_cfg_shmstore_size(std::string module_name);

std::string upg_oper_shmstore_name(std::string module_name);
int upg_oper_shmstore_size(std::string module_name);

}    // namespace api

#endif    // __UPGRADE_API_INCLUDE_SHMSTORE_HPP__
