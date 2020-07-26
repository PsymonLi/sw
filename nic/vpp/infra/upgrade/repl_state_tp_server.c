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
#include <vlib/threads.h>
#include "infra/api/intf.h"
#include "nic/vpp/infra/ipc/pdsa_vpp_hdlr.h"
#include "repl_state_tp_pvt.h"

static clib_socket_t repl_state_tp_server_sock;
static clib_socket_t repl_state_tp_client_sock;
static uword repl_state_tp_server_file_index = ~0;
static uword repl_state_tp_client_file_index = ~0;

static void
close_client_sock (void)
{
    if (repl_state_tp_client_file_index != ~0 ) {
        clib_file_del_by_index(&file_main, repl_state_tp_client_file_index);
        // closing of the socket happens in clib_file_del_by_index above
        clib_socket_free(&repl_state_tp_client_sock);
        repl_state_tp_client_file_index = ~0;
    }
}

static void
close_server_sock (void)
{
    if (repl_state_tp_server_file_index != ~0 ) {
        clib_file_del_by_index(&file_main, repl_state_tp_server_file_index);
        // closing of the socket happens in clib_file_del_by_index above
        clib_socket_free(&repl_state_tp_server_sock);
        repl_state_tp_server_file_index = ~0;
    }
}

static clib_error_t *
repl_state_tp_server_read (clib_file_t * uf)
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
        close_client_sock();
        return clib_error_return_unix(0, "read-close");
    }

    /* TODO: Need to process the input */
    vec_free(input_buf);
    return 0;
}

static clib_error_t *
repl_state_tp_server_write (clib_file_t * uf)
{
    return 0;
}

static clib_error_t *
repl_state_tp_server_error (clib_file_t * uf)
{
    close_client_sock();
    return 0;
}

// Socket has a new connection.
static clib_error_t *
repl_state_tp_server_accept (clib_file_t * uf)
{
    clib_socket_t *s = &repl_state_tp_server_sock;
    clib_error_t *error;
    clib_socket_t *client = &repl_state_tp_client_sock;
    clib_file_t clib_file = { 0 };
    vlib_thread_main_t *tm = &vlib_thread_main;


    memset(client, '\0', sizeof(*client));
    error = clib_socket_accept(s, client);
    if (error) {
        return error;
    }

    clib_file.read_function = repl_state_tp_server_read;
    clib_file.write_function = repl_state_tp_server_write;
    clib_file.error_function = repl_state_tp_server_error;
    clib_file.file_descriptor = client->fd;
    clib_file.private_data = 0;
    clib_file.description = format(0, "VPP Inter domain IPC client");
    repl_state_tp_client_file_index = clib_file_add(&file_main, &clib_file);

    /*
     * VPP from doman 'B' has connected. Ideally we need a protobuf message
     * to negotiate the queue name to use and know the client's capabilities
     * like number of threads etc. We will do it in phase-2.
     */
    // Bring down all interfaces and suspend workers
    // TODO: Once we have UPG_EV_PRESYNC stage, move the
    // commented code to that stage.
    pds_vpp_worker_thread_barrier();
    pds_infra_set_all_intfs_status(0);
    // Make worker threads wait
    pds_vpp_set_suspend_resume_worker_threads(1);
    pds_vpp_worker_thread_barrier_release();

    vlib_set_main_thread_affinity(PDS_VPP_SYNC_CORE);

    repl_state_tp_sync(REPL_STATE_OBJ_ID_SESS, sqname);

    vlib_set_main_thread_affinity(tm->main_lcore);

    return 0;
}

// Initialize a Unix Domain Socket and register the FD with VPP event loop
int
repl_state_tp_server_init (void)
{
    clib_socket_t *s = &repl_state_tp_server_sock;
    clib_error_t *error = 0;

    memset(s, '\0', sizeof(*s));
    s->config = VPP_UIPC_REPL_SOCKET_FILE;
    s->flags = CLIB_SOCKET_F_IS_SERVER |
               CLIB_SOCKET_F_ALLOW_GROUP_WRITE |    /* PF_LOCAL socket only */
               CLIB_SOCKET_F_SEQPACKET;    /* no stream boundaries headache */

    // makedir of file socket
    u8 *tmp = format(0, "%s", VPP_UIPC_REPL_SOCKET_DIR);
    vlib_unix_recursive_mkdir((char *) tmp);
    vec_free (tmp);

    error = clib_socket_init(s);
    if (error) {
        clib_error_report(error);
        return error->code;
    }

    clib_file_t clib_file = { 0 };
    clib_file.read_function = repl_state_tp_server_accept;
    clib_file.file_descriptor = s->fd;
    clib_file.description = format(0, "VPP Inter domain IPC listener %s", s->config);
    repl_state_tp_server_file_index = clib_file_add(&file_main, &clib_file);

    return 0;
}

// Close the server socket and cleanup resources
void
repl_state_tp_server_stop (void)
{
    close_client_sock();
    close_server_sock();
}
