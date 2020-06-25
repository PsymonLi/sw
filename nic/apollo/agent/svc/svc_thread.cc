//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include <cerrno>
#include <sys/un.h>
#include "nic/sdk/include/sdk/uds.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/apollo/agent/svc/svc_thread.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/init.hpp"
#include "nic/apollo/api/include/pds_debug.hpp"
#include "nic/apollo/api/include/pds_upgrade.hpp"
#include "nic/apollo/core/mem.hpp"
#include "gen/proto/types.pb.h"

#define SVC_SERVER_SOCKET_PATH          "/var/run/pds_svc_server_sock"
#define CMD_IOVEC_DATA_LEN              (1024 * 1024)

namespace core {

static int g_uds_sock_fd;
static thread_local sdk::event_thread::io_t cmd_accept_io;

static void
svc_server_read_cb (sdk::event_thread::io_t *io, int fd, int events)
{
    char *iov_data;
    types::ServiceRequestMessage proto_req;
    int cmd_fd, bytes_read;

    // allocate memory to read from socket
    iov_data = (char *)SDK_CALLOC(PDS_MEM_ALLOC_CMD_READ_IO, CMD_IOVEC_DATA_LEN);
    // read from existing connection
    if ((bytes_read = uds_recv(fd, &cmd_fd, iov_data, CMD_IOVEC_DATA_LEN)) < 0) {
        PDS_TRACE_ERR("Read from unix domain socket failed");
        return;
    }
    // execute command
    if (bytes_read > 0) {
        // convert to proto msg
        if (proto_req.ParseFromArray(iov_data, bytes_read)) {
            // handle cmd
            handle_svc_req(fd, &proto_req, cmd_fd);
        } else {
            PDS_TRACE_ERR("Parse service request message from socket failed, "
                          "bytes_read {}", bytes_read);
        }
        // close fd
        if (cmd_fd >= 0) {
            close(cmd_fd);
        }
    }
    // free iov data
    SDK_FREE(PDS_MEM_ALLOC_CMD_READ_IO, iov_data);
    // close connection
    close(fd);
    // stop the watcher
    sdk::event_thread::io_stop(io);
    // free the watcher
    SDK_FREE(PDS_MEM_ALLOC_CMD_READ_IO, io);
}

static void
svc_server_accept_cb (sdk::event_thread::io_t *io, int fd, int events)
{
    sdk::event_thread::io_t *cmd_read_io;
    int fd2;

    // accept incoming connection
    if ((fd2 = accept(fd, NULL, NULL)) == -1) {
        PDS_TRACE_ERR("Accept failed");
        return;
    }

    // allocate memory for cmd_read_io
    cmd_read_io = (sdk::event_thread::io_t *)
                    SDK_MALLOC(PDS_MEM_ALLOC_CMD_READ_IO,
                               sizeof(sdk::event_thread::io_t));
    if (cmd_read_io == NULL) {
        PDS_TRACE_ERR("Memory allocation for cmd_read_io failed");
        return;
    }

    // Initialize and start watcher to read client requests
    sdk::event_thread::io_init(cmd_read_io, svc_server_read_cb, fd2,
                               EVENT_READ);
    sdk::event_thread::io_start(cmd_read_io);
}

static inline sdk_ret_t
svc_server_thread_uds_init (void)
{
    struct sockaddr_un sock_addr;

    // initialize unix socket
    if ((g_uds_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        PDS_TRACE_ERR("Failed to open UDS for cmd server thread, err {} {}",
                      errno, strerror(errno));
        return SDK_RET_ERR;
    }

    memset(&sock_addr, 0, sizeof (sock_addr));
    sock_addr.sun_family = AF_UNIX;
    strncpy(sock_addr.sun_path, SVC_SERVER_SOCKET_PATH,
            sizeof(SVC_SERVER_SOCKET_PATH));
    unlink(sock_addr.sun_path);
    if (bind(g_uds_sock_fd, (struct sockaddr *)&sock_addr,
             sizeof(struct sockaddr_un)) == -1) {
        PDS_TRACE_ERR ("Failed to bind UDS for service thread, err {} {}",
                       errno, strerror(errno));
        return SDK_RET_ERR;
    }

    if (listen(g_uds_sock_fd, 1) == -1) {
        PDS_TRACE_ERR ("Failed to listen on UDS fd, err {} {}",
                       errno, strerror(errno));
        return SDK_RET_ERR;
    }
    PDS_TRACE_INFO("Listening to UDS {}", SVC_SERVER_SOCKET_PATH);
    sdk::event_thread::io_init(&cmd_accept_io, svc_server_accept_cb,
                               g_uds_sock_fd, EVENT_READ);
    sdk::event_thread::io_start(&cmd_accept_io);
    return SDK_RET_OK;
}

void
svc_server_thread_init (void *ctxt)
{
    // register for initial upgrade discovery events
    pds_upgrade_init();

    // bind to UDS socket
    svc_server_thread_uds_init();

}

void
svc_server_thread_exit (void *ctxt)
{
    sdk::event_thread::io_stop(&cmd_accept_io);
    if (g_uds_sock_fd >= 0) {
        close(g_uds_sock_fd);
    }
}

sdk_ret_t
svc_server_thread_suspend_cb (void *ctxt)
{
    sdk::event_thread::event_thread *thr = (sdk::event_thread::event_thread *)ctxt;
    sdk_ret_t ret;

    svc_server_thread_exit(NULL);
    ret = sdk::event_thread::event_thread::suspend(thr);
    return ret;
}

sdk_ret_t
svc_server_thread_resume_cb (void *ctxt)
{
    sdk::event_thread::event_thread *thr = (sdk::event_thread::event_thread *)ctxt;;
    sdk_ret_t ret;

    ret = svc_server_thread_uds_init();
    if (ret == SDK_RET_OK) {
        ret = sdk::event_thread::event_thread::resume(thr);
    }
    return ret;
}

}    // namespace core
