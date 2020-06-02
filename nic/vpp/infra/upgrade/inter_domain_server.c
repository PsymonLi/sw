//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// This file initializes a Unix Domain Socket which is used to communicate
// between VPP on Domain 'A' and VPP on Domain 'B' during a hitless upgrade

#include <vppinfra/clib.h>
#include <vlib/vlib.h>
#include <vppinfra/socket.h>
#include <vppinfra/file.h>
#include <vlib/unix/unix.h>
#include "inter_domain_pvt.h"

clib_socket_t inter_domain_server_sock;

static clib_error_t *
inter_domain_server_read (clib_file_t * uf)
{
    u8 *input_buf = 0;
    int n;

    /*
     * FIXME: This is a SEQPACKET socket and here we are assuming that
     * payload length will never exceed BUF_SIZE. Ideally we need to do
     * a msg peek, vec_resize for that size and then call recv_msg.
     */
    vec_resize(input_buf, BUF_SIZE);
    if ((n = recv(uf->file_descriptor, input_buf, BUF_SIZE, 0)) < 0) {
        return clib_error_return_unix(0, "read");
    } else if (n == 0) {
        clib_file_del(&file_main, uf);
        return clib_error_return_unix(0, "read-close");
    }

    /* TODO: Need to process the input */
    vec_free(input_buf);
    return 0;
}

static clib_error_t *
inter_domain_server_write (clib_file_t * uf)
{
    return 0;
}

static clib_error_t *
inter_domain_server_error (clib_file_t * uf)
{
    clib_file_del(&file_main, uf);
    return 0;
}

// Socket has a new connection.
static clib_error_t *
inter_domain_server_accept (clib_file_t * uf)
{
    clib_socket_t *s = &inter_domain_server_sock;
    clib_error_t *error;
    clib_socket_t client;
    clib_file_t clib_file = { 0 };

    error = clib_socket_accept(s, &client);
    if (error) {
        return error;
    }

    clib_file.read_function = inter_domain_server_read;
    clib_file.write_function = inter_domain_server_write;
    clib_file.error_function = inter_domain_server_error;
    clib_file.file_descriptor = client.fd;
    clib_file.private_data = 0;
    clib_file.description = format(0, "VPP Inter domain IPC client");
    clib_file_add(&file_main, &clib_file);

    /*
     * VPP from doman 'B' has connected. Ideally we need a protobuf message
     * to negotiate the queue name to use and know the client's capabilities
     * like number of threads etc. We will do it in phase-2
     */
    idipc_sync(IDIPC_OBJ_ID_SESS, sqname);
    return 0;
}

// Initialize a Unix Domain Socket and register the FD with VPP event loop
int
inter_domain_server_init()
{
    clib_socket_t *s = &inter_domain_server_sock;
    clib_error_t *error = 0;

    s->config = SOCKET_FILE;
    s->flags = CLIB_SOCKET_F_IS_SERVER |
               CLIB_SOCKET_F_ALLOW_GROUP_WRITE |    /* PF_LOCAL socket only */
               CLIB_SOCKET_F_SEQPACKET;    /* no stream boundaries headache */

    // makedir of file socket
    u8 *tmp = format(0, "%s", "/run/vpp");
    vlib_unix_recursive_mkdir((char *) tmp);
    vec_free (tmp);

    error = clib_socket_init(s);
    if (error) {
        clib_error_report(error);
        return 1;
    }

    clib_file_t clib_file = { 0 };
    clib_file.read_function = inter_domain_server_accept;
    clib_file.file_descriptor = s->fd;
    clib_file.description = format(0, "VPP Inter domain IPC listener %s", s->config);
    clib_file_add(&file_main, &clib_file);

    return 0;
}
