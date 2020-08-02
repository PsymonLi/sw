//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/upgrade/shmstore/api.hpp"

namespace api {

std::string
upg_shmstore_persistent_path (void)
{
    return "/update";
}

std::string
upg_shmstore_volatile_path_graceful (void)
{
    return "/dev/shm";
}

std::string
upg_shmstore_volatile_path_hitless (void)
{
    return "/share";
}

int
upg_shmstore_segment_meta_size (void)
{
    // every segment consumes an additional 512 bytes in shmmgr
    return 512;
}

std::string
upg_cfg_shmstore_name (std::string module_name)
{
    if (module_name == std::string("pdsagent"))
        return "pds_agent_upgdata";

    if (module_name == std::string("nicmgr"))
        return "pds_nicmgr_upgdata";

    return NULL;
}

int
upg_cfg_shmstore_size (std::string module_name)
{
    if (module_name == std::string("pdsagent"))
        return (1 * 1024 * 1024);  // 1MB

    if (module_name == std::string("nicmgr"))
        return (32 * 1024);        // 32K

    return -1;
}

std::string
upg_oper_shmstore_name (std::string module_name)
{
    if (module_name == std::string("pdsagent"))
        return "pds_agent_oper_upgdata";

    if (module_name == std::string("nicmgr"))
        return "nicmgr_oper_upgdata";

    if (module_name == std::string("linkmgr"))
        return "linkmgr_oper_upgdata";

    return NULL;
}

int
upg_oper_shmstore_size (std::string module_name)
{
    if (module_name == std::string("pdsagent"))
        return (32 * 1024);     // 32K

    if (module_name == std::string("nicmgr"))
        return (128 * 1024);    // 128K

    if (module_name == std::string("linkmgr"))
        return (32 * 1024);     // 32K

    return -1;
}

}    // namespace api
