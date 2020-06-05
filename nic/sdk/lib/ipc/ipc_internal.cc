// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <string>
#include <stdlib.h>

#include "ipc_internal.hpp"

#define IPC_SUFFIX_ENV "IPC_SUFFIX"

const std::string
ipc_env_suffix (std::string original)
{
    const char *suffix;
    
    suffix = getenv(IPC_SUFFIX_ENV);
    if (suffix == NULL) {
        return std::string(original);
    }

    return std::string(original) + std::string(suffix);
}

