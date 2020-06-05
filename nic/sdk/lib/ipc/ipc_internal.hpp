// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#ifndef __IPC_INTERNAL_HPP__
#define __IPC_INTERNAL_HPP__

#include <string>

#include "ipc.hpp"

#define IPC_MAX_ID 63
#define IPC_MAX_CLIENT 63

const std::string ipc_env_suffix(std::string original);

#endif // __IPC_INTERNAL_H__
