// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __SDK_IPC_MS_HPP__
#define __SDK_IPC_MS_HPP__

#include <stdint.h>

#include "ipc.hpp"

namespace sdk {
namespace ipc {

typedef void (*handler_ms_cb)(int fd, int, void *ctx);

typedef void (*fd_watch_ms_cb)(int fd, handler_ms_cb cb, void *cb_ctx);

extern void ipc_init_metaswitch(uint32_t client_id,
                                fd_watch_ms_cb fw_watch_ms_cb);

}
}

#endif
