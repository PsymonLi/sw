//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/upgrade/api/include/shmstore.hpp"

namespace api {

std::string
upg_shmstore_persistent_path (void)
{
    return NULL;
}

std::string
upg_shmstore_volatile_path_graceful (void)
{
    return NULL;
}

std::string
upg_shmstore_volatile_path_hitless (void)
{
    return NULL;
}

int
upg_shmstore_segment_meta_size (void)
{
    return -1;
}

std::string
upg_cfg_shmstore_name (std::string module_name)
{
    return NULL;
}

int
upg_cfg_shmstore_size (std::string module_name)
{
    return -1;
}

std::string
upg_oper_shmstore_name (std::string module_name)
{
    return NULL;
}

int
upg_oper_shmstore_size (std::string module_name)
{
    return -1;
}

}    // namespace api
