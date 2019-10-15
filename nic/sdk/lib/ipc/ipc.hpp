//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#ifndef __IPC_H__
#define __IPC_H__

#include <memory>
#include <vector>

#include <stddef.h>
#include <stdint.h>

namespace sdk {
namespace lib {
namespace ipc {

#define IPC_MAX_ID 63

class ipc_msg {
public:
    /// \brief get a pointer to the data of the message
    virtual void *data(void) = 0;
    /// \brief get the size of the data payload
    virtual size_t length(void) = 0;
};
typedef std::shared_ptr<struct ipc_msg> ipc_msg_ptr;

/// \brief send a message to an ipc_server and wait for the reply
/// \param[in] recipient is the id of the ipc_server to send the message to
/// \param[in] data is a pointer to the data of the message to be send
/// \param[in] data_length is the length of the data to send
extern ipc_msg_ptr send_recv(uint32_t recipient, const void *data,
                             size_t data_length);

class ipc_server {
public:
    /// \brief create a new ipc server
    /// \param[in] id is the id that will be used by clients to send this server
    ///               requests
    static ipc_server *factory(uint32_t id);
    static void destroy(ipc_server *server);
    virtual int fd(void) = 0;
    virtual ipc_msg_ptr recv(void) = 0;
    virtual void reply(ipc_msg_ptr msg, const void *data,
                       size_t data_length) = 0;
};

class ipc_client {
public:
    static ipc_client *factory(uint32_t recipient);
    static void destroy(ipc_client *client);
    virtual int fd(void) = 0;
    virtual void send(const void *data, size_t data_length,
                      const void *cookie) = 0;
    virtual ipc_msg_ptr recv(const void** cokie) = 0;
};

} // namespace ipc
} // namespace lib
} // namespace sdk

#endif // __IPC_H__
