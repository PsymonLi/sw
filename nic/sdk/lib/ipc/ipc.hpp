//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#ifndef __IPC_H__
#define __IPC_H__

#include <memory>
#include <mutex>
#include <vector>

#include <stddef.h>
#include <stdint.h>

#define IPC_MAX_ID 63
#define IPC_MAX_CLIENT 63

namespace sdk {
namespace ipc {

typedef enum ipc_msg_type {
    DIRECT    = 0,
    BROADCAST = 1,
} ipc_msg_type_t;

class ipc_msg {
public:
    virtual uint32_t code(void) = 0;
    /// \brief get a pointer to the data of the message
    virtual void *data(void) = 0;
    /// \brief get the size of the data payload
    virtual size_t length(void) = 0;
    /// \brief get the type of the message (DIRECT or BROADCAST)
    virtual ipc_msg_type_t type(void) = 0;
    /// \brief get the sender of the message
    virtual uint32_t sender(void) = 0;
    /// \brief get a debug string for the message
    virtual std::string debug(void) = 0;
};
typedef std::shared_ptr<struct ipc_msg> ipc_msg_ptr;


///
/// Callbacks
///

typedef void (*request_cb)(ipc_msg_ptr msg, const void *ctx);

typedef void (*response_cb)(ipc_msg_ptr msg, const void *request_cookie,
                            const void *ctx);

typedef void (*response_oneshot_cb)(ipc_msg_ptr msg,
                                    const void *request_cookie);

typedef void (*subscription_cb)(ipc_msg_ptr msg, const void *ctx);


///
/// Init
///

typedef void (*handler_cb)(int fd, const void *ctx);
typedef void *(*fd_watch_fn)(int fd, handler_cb cb, const void *ipc_ctx,
                             const void *infra_ctx);
typedef void (*fd_unwatch_fn)(int fd, void *watcher, const void *infra_ctx);
typedef void (*timer_callback)(void *timer, const void *ipc_ctx);
typedef void *(*timer_add_fn)(timer_callback callback, const void *ipc_ctx,
                              double timeout, const void *infra_ctx);
typedef void (*timer_del_fn)(void *timer, const void *infra_ctx);

typedef struct infra_t_ {
    fd_watch_fn fd_watch;
    const void *fd_watch_ctx;
    fd_unwatch_fn fd_unwatch;
    const void *fd_unwatch_ctx;
    timer_add_fn timer_add;
    const void *timer_add_ctx;
    timer_del_fn timer_del;
    const void *timer_del_ctx;
} infra_t;
typedef std::unique_ptr<infra_t> infra_ptr;

extern void ipc_init_async(uint32_t client_id, infra_ptr infra,
                           bool associate_thread_name = false);

extern void ipc_init_sync(uint32_t client_id);

extern void ipc_init_sync(uint32_t client_id, infra_ptr infra);

///
/// Sending
///

extern void request(uint32_t recipient, uint32_t msg_code, const void *data,
                    size_t length, const void *cookie, double timeout = 0.0);

extern void request(uint32_t recipient, uint32_t msg_code, const void *data,
                    size_t length, response_oneshot_cb response_cb,
                    const void *cookie, double timeout = 0.0);

extern void broadcast(uint32_t msg_code, const void *data, size_t data_length);

extern void respond(ipc_msg_ptr msg, const void *data, size_t data_length);

///
/// Receiving
///

extern void reg_request_handler(uint32_t msg_code, request_cb callback,
                                const void *ctx);


extern void reg_response_handler(uint32_t msg_code, response_cb callback,
                                 const void *ctx);

extern void subscribe(uint32_t msg_code, subscription_cb callback,
                      const void *ctx);

// This is to be used in sync cases when we want to receive requests or
// subscription messages
extern void receive(void);


///
/// Configuration
///

extern void set_drip_feeding(bool enabled);

} // namespace ipc
} // namespace sdk

#endif // __IPC_H__
