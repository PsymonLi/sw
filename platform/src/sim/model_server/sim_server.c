/*
 * Copyright (c) 2017, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

#include "pciehsvc.h"
#include "bdf.h"
#include "simserver.h"
#include "simdevices.h"
#include "zmq_wait.h"

typedef struct simctx_s {
    int serverfd;
    int clientfd;
    char user[32];
} simctx_t;

static simctx_t simctx;
static int verbose_flag;

void
verbose(const char *fmt, ...)
{
    va_list arg;

    if (verbose_flag) {
        va_start(arg, fmt);
        vprintf(fmt, arg);
        va_end(arg);
    }
}

simctx_t *
simctx_get(void)
{
    return &simctx;
}

/*
 * ================================================================
 * model_server api
 * ----------------------------------------------------------------
 */

extern int model_server_step_doorbell(u_int64_t addr, u_int64_t data);
extern int model_server_read_reg(u_int64_t addr, u_int32_t *data);
extern int model_server_write_reg(u_int64_t addr, u_int32_t data);
extern int model_server_read_mem(u_int64_t addr, u_int8_t *buf, size_t size);
extern int model_server_write_mem(u_int64_t addr, u_int8_t *buf, size_t size);

/*
 * ================================================================
 * sim_server_api
 * ----------------------------------------------------------------
 */

void 
sim_server_set_user(const char *user)
{
    simctx_t *sc = simctx_get();

    verbose("================\n");
    verbose("init %s\n", user);
    verbose("----------------\n");
    strncpy(sc->user, user, sizeof(sc->user) - 1);
}

static void
sim_server_log(const char *fmt, va_list ap)
{
    if (verbose_flag) {
        vprintf(fmt, ap);
    }
}

static void
sim_server_error(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
}

static int
sim_server_doorbell(u_int64_t addr, u_int64_t data)
{
    verbose("doorbell addr 0x%"PRIx64" data 0x%"PRIx64"\n", addr, data);
    return model_server_step_doorbell(addr, data);
}

static int
sim_server_read_reg(u_int64_t addr, u_int32_t *data)
{
    verbose("read_reg addr 0x%"PRIx64"\n", addr);
    return model_server_read_reg(addr, data);
}

static int
sim_server_write_reg(u_int64_t addr, u_int32_t data)
{
    verbose("write_reg addr 0x%"PRIx64" data 0x%x\n", addr, data);
    return model_server_write_reg(addr, data);
}

static int
sim_server_read_mem(u_int64_t addr, void *buf, size_t size)
{
    verbose("read_mem addr 0x%"PRIx64" size 0x%lx\n", addr, size);
    return model_server_read_mem(addr, buf, size);
}

static int
sim_server_write_mem(u_int64_t addr, void *buf, size_t size)
{
    verbose("write_mem addr 0x%"PRIx64" size 0x%lx\n", addr, size);
    return model_server_write_mem(addr, buf, size);
}

static simdev_api_t sim_server_api = {
    .set_user  = sim_server_set_user,
    .log       = sim_server_log,
    .error     = sim_server_error,
    .doorbell  = sim_server_doorbell,
    .read_reg  = sim_server_read_reg,
    .write_reg = sim_server_write_reg,
    .read_mem  = sim_server_read_mem,
    .write_mem = sim_server_write_mem,
};

/*
 * ================================================================
 * initialization
 * ----------------------------------------------------------------
 */

static void
zmq_sim_recv(int clientfd, void *arg)
{
    if (sims_client_recv_and_handle(simctx.clientfd) == 0) {
        verbose("lost connection to client\n");
        zmq_wait_remove_fd(simctx.clientfd);
        sims_close_client(simctx.clientfd);
        simctx.clientfd = -1;
    }
}

static void
zmq_sim_new_client(int serverfd, void *arg)
{
    simctx.clientfd = sims_open_client(serverfd);
    if (simctx.clientfd < 0) {
        perror("sims_open_client");
    } else {
        verbose("simclient conected\n");
        zmq_wait_add_fd(simctx.clientfd, zmq_sim_recv, NULL);
    }
}

void
sim_server_init(int argc, char *argv[])
{
    int opt;

    if (simdev_open(&sim_server_api) < 0) {
        fprintf(stderr, "simdev_open failed\n");
        exit(1);
    }
    optind = 0;
    while ((opt = getopt(argc, argv, "d:v")) != -1) {
        switch (opt) {
        case 'd':
            if (simdev_add_dev(optarg) < 0) {
                fprintf(stderr, "simdev_add_dev %s failed\n", optarg);
                exit(1);
            }
            break;
        case 'v':
            verbose_flag = 1;
            break;
        case '?':
        default:
            break;
        }
    }
    optind = 0;
    pciehsvc_open(NULL);
    simctx.serverfd = sims_open(NULL, simdev_msg_handler);
    zmq_wait_add_fd(simctx.serverfd, zmq_sim_new_client, NULL);
}

void
sim_server_shutdown(void)
{
    pciehsvc_close();
    zmq_wait_remove_fd(simctx.clientfd);
    sims_close_client(simctx.clientfd);
    sims_close(simctx.serverfd);
}

int 
sim_server_read_clientmem(const u_int64_t addr, 
                          void *buf,
                          const size_t len)
{
    int s = simctx.clientfd;
    u_int16_t bdf = 0x0300; /* XXX */

    return sims_memrd(s, bdf, addr, len, buf);
}

int 
sim_server_write_clientmem(const u_int64_t addr,
                           const void *buf, 
                           const size_t len)
{
    int s = simctx.clientfd;
    u_int16_t bdf = 0x0300; /* XXX */

    return sims_memwr(s, bdf, addr, len, buf);
}
