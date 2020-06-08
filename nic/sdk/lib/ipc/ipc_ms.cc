// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include "ipc.hpp"
#include "ipc_ms.hpp"

namespace sdk {
namespace ipc {

struct fd_watcher_t {
    handler_cb cb;
    const void *ctx;
};

static void
fd_cb_wrap (int fd, int, void *ctx)
{
    fd_watcher_t *watcher = (fd_watcher_t *)ctx;

    watcher->cb(fd, watcher->ctx);
}

static void *
fd_watch (int fd, handler_cb cb, const void *ipc_ctx, const void *infra_ctx)
{
    fd_watch_ms_cb ms_cb = (fd_watch_ms_cb) infra_ctx;
    fd_watcher_t *watcher = new fd_watcher_t();

    watcher->cb = cb;
    watcher->ctx = ipc_ctx;

    ms_cb(fd, fd_cb_wrap, watcher);
    
    return watcher;
}

static void
fd_unwatch (int fd, void *watcher, const void *infra_ctx)
{
    fd_watcher_t *w = (fd_watcher_t *)watcher;

    // We must call the MS unwatch function here

    delete w;
        
}

void
ipc_init_metaswitch (uint32_t client_id, fd_watch_ms_cb fd_watch_ms_cb)
{

    ipc_init_async(client_id, std::unique_ptr<infra_t>(new infra_t{
                    .fd_watch = fd_watch,
                    .fd_watch_ctx = (void *)fd_watch_ms_cb,
                    .fd_unwatch = fd_unwatch,
                    .fd_unwatch_ctx = NULL,
                    .timer_add = NULL,
                    .timer_add_ctx = NULL,
                    .timer_del = NULL,
                    .timer_del_ctx =  NULL,
                    }), true);
}
        
} // namespace ipc
} // namespace sdk
