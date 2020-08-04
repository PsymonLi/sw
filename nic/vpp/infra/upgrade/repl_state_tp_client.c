//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// Code t establish a session by VPP on domain 'B' to vpp on domain 'A'
// during hitless upgrade

#include <vppinfra/clib.h>
#include <vlib/vlib.h>
#include <vppinfra/socket.h>
#include <vppinfra/file.h>
#include <vlib/unix/unix.h>
#include <vlib/threads.h>
#include "nic/vpp/infra/ipc/pdsa_vpp_hdlr.h"
#include "repl_state_tp_pvt.h"

static clib_socket_t repl_state_tp_client_sock;
static uword repl_state_tp_client_file_index = ~0;

static void
close_sock (void)
{
    if (repl_state_tp_client_file_index != ~0 ) {
        clib_file_del_by_index(&file_main, repl_state_tp_client_file_index);
        // closing of the socket happens in clib_file_del_by_index above
        clib_socket_free(&repl_state_tp_client_sock);
        repl_state_tp_client_file_index = ~0;
    }
}

static clib_error_t *
repl_state_tp_client_read (clib_file_t * uf)
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
        close_sock();
        return clib_error_return_unix(0, "read-close");
    }

    /* TODO: Need to process the input */
    vec_free(input_buf);
    return 0;
}

static clib_error_t *
repl_state_tp_client_write (clib_file_t * uf)
{
    return 0;
}

static clib_error_t *
repl_state_tp_client_error (clib_file_t * uf)
{
    close_sock();
    return 0;
}

// Initialize a Unix Domain Socket and register the FD with VPP event loop
int
repl_state_tp_client_init()
{
    clib_socket_t *s = &repl_state_tp_client_sock;
    clib_error_t *error = 0;
    clib_file_t clib_file = { 0 };
    vlib_thread_main_t *tm = &vlib_thread_main;

    memset(s, '\0', sizeof(*s));
    s->config = VPP_UIPC_REPL_SOCKET_FILE;
    // We don't use non-blocking connect here as by the time we are here
    // server must be up and running
    s->flags = CLIB_SOCKET_F_IS_CLIENT |
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

    clib_file.read_function = repl_state_tp_client_read;
    clib_file.write_function = repl_state_tp_client_write;
    clib_file.error_function = repl_state_tp_client_error;
    clib_file.file_descriptor = s->fd;
    clib_file.private_data = 0;
    clib_file.description = format(0, "VPP Inter domain IPC client");
    repl_state_tp_client_file_index = clib_file_add(&file_main, &clib_file);

    /*
     * We were able to connect to VPP on domain 'A'. We must send protobuf msg
     * to negotiate the queue name to use and advertise our capabilities
     * like number of threads etc. We will do it in phase-2
     */

    // Make worker threads wait. pds_vpp_worker_thread_barrier() has already
    // been called from vpp_upg_ev_hdlr().
    pds_vpp_set_suspend_resume_worker_threads(1);
    pds_vpp_worker_thread_barrier_release();

    vlib_set_main_thread_affinity(PDS_VPP_RESTORE_CORE);

    repl_state_tp_restore(REPL_STATE_OBJ_ID_SESS, sqname);
    repl_state_tp_restore(REPL_STATE_OBJ_ID_NAT, sqname);

    vlib_set_main_thread_affinity(tm->main_lcore);

    // Wakeup worker threads
    pds_vpp_worker_thread_barrier();
    pds_vpp_set_suspend_resume_worker_threads(0);

    // Close the client socket and cleanup resources
    close_sock();

    return 0;
}

