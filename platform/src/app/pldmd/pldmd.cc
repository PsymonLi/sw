/*
 * Copyright (C) 2020 Pensando Systems Inc.
 *
 * Pensando PLDM Control Daemon (pldmd)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "libmctp.h"
#include "libmctp-i2c.h"
#include "mctp_ioctl.h"
#include "lib/operd/logger.hpp"
#include "pldm_base.h"

// Logging via operd, configured in nic/sysmgr/iris/operd-regions.json
extern sdk::operd::logger_ptr g_log;
sdk::operd::logger_ptr g_log = sdk::operd::logger::create("pldmd");

// mctp over i2c uses smbus block read/write with maximum size 255
// mctp control commands always fit in a single 64 byte packet
struct pldm_msg_body {
    uint8_t msg_type;
    uint8_t msg_hdr;     // Rq,D,Instance ID
    uint8_t pldm_type;
    uint8_t cmd_code;
    uint8_t data[256];
};

static const mctp_eid_t local_eid_default = 0;

struct binding {
    const char *name;
    int        (*init)(struct mctp *mctp,
                       struct binding *binding,
                       mctp_eid_t eid,
                       int n_params,
                       char * const *params);
    int        (*get_fd)(struct binding *binding);
    int        (*process)(struct binding *binding);
    void       *data;
};

struct ctx {
    struct mctp *mctp;
    struct binding *binding;
    bool   verbose;
    int    local_eid;
    void   *buf;
    size_t buf_size;
    int    sock;
    struct pollfd *pollfds;
};

static struct pldm_settings {
    uint8_t assigned_tid;
} pldm_settings;

static int pldmd_log_level = MCTP_LOG_INFO;
//static int pldmd_log_level = MCTP_LOG_DEBUG;

void
set_pldmd_log_level(int log_level)
{
    pldmd_log_level = log_level;
}

int
get_pldmd_log_level(void)
{
    return pldmd_log_level;
}

void
log_pldm_cmd(uint8_t cmd)
{
    switch(cmd) {
    case PLDM_GET_TID:
        g_log->info("## PLDM_GET_TID ##");
        break;
    case PLDM_GET_PLDM_VERSION:
        g_log->info("## GET_PLDM_VERSION ##");
        break;
    case PLDM_GET_PLDM_TYPES:
        g_log->info("## GET_PLDM_TYPES ##");
        break;
    case PLDM_GET_PLDM_COMMANDS:
        g_log->info("## GET_PLDM_COMMANDS ##");
        break;
    default:
        g_log->info("## PLDM CMD UNKNOWN ##");
    }
}

static void
log_rx_pkt(uint8_t src_eid, uint8_t msg[], size_t len, uint8_t tag)
{
    uint8_t type = msg[0] & 0x7f;
    uint8_t id = msg[1] & 0x1f;
    uint8_t cmd = msg[3];
    char log[100];
    char *logp = log;
    int i, cnt;

    log_pldm_cmd(cmd);

    // Log the first 16 data bytes makes each entry 50 + 48 = 98 bytes
    logp += sprintf(logp, "RX ID %02d type %s cmd %d src %02x tag %d len %02lu msg ",
                    id, get_type_name(type), cmd, src_eid, tag, len);

    if (len > 16)
        cnt = 16;
    else
        cnt = len;

    for (i=0; i<cnt; i++)
        logp += sprintf(logp, "%02x ", *((uint8_t *)msg + i));

    g_log->info("%s", log);
}

static void
log_tx_pkt(uint8_t src_eid, uint8_t msg[], void *data, size_t len, uint8_t tag)
{
    uint8_t type = msg[0] & 0x7f;
    uint8_t id = msg[1] & 0x1f;
    uint8_t cmd = msg[3];
    char log[100];
    char *logp = log;
    int i, cnt;

    logp += sprintf(logp, "TX ID %02d type %s cmd %d src %02x tag %d len %02lu msg ",
                    id, get_type_name(type), cmd, src_eid, tag, len);

    if (len > 16)
        cnt = 16;
    else
        cnt = len;

    for (i=0; i<cnt; i++)
        logp += sprintf(logp, "%02x ", *((uint8_t *)data + i));

    g_log->info("%s", log);
}

static int
not_supported(struct mctp *mctp, uint8_t src_eid, uint8_t tag, uint8_t msg[])
{
    struct pldm_msg_body resp;
    uint8_t resp_len;
    int rc;

    g_log->info("PLDM command %d not supported", msg[3]);

    // Request message header returned with response
    resp.msg_type  = msg[0];           // MsgType
    resp.msg_hdr   = msg[1] & 0x7f;    // Clear Rq
    resp.pldm_type = msg[2];           // CmdCode
    resp.cmd_code  = msg[3];

    // Response data
    resp.data[0] = MCTP_CTRL_UNSUPPORTED_CMD;
    resp_len = 5;

    log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    return rc;
}

// Command 1: PLDM_SET_TID
static int
pldm_set_tid(struct mctp *mctp, uint8_t src_eid, uint8_t tag, uint8_t *msg)
{
    struct pldm_msg_body resp;
    uint8_t resp_len;
    uint8_t assigned_tid;
    bool valid_tid = false;
    int rc;

    // Request message header returned with response
    resp.msg_type  = msg[0];           // MsgType
    resp.msg_hdr   = msg[1] & 0x7f;    // Clear Rq
    resp.pldm_type = msg[2];           // CmdCode
    resp.cmd_code  = msg[3];

    // Request data
    assigned_tid = msg[4];

    // Response data
    if ((assigned_tid) == 0 || (assigned_tid == 0xff)) {
        resp.data[0] = PLDM_ERROR_INVALID_DATA;
    } else {
        resp.data[0] = PLDM_SUCCESS;
        valid_tid = true;
    }
    resp_len = 5;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == PLDM_SUCCESS) {
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);
        if (valid_tid) {
            pldm_settings.assigned_tid = assigned_tid;
            g_log->info("pldm tid 0x%02x assigned", assigned_tid);
        } else {
            g_log->info("invalid pldm tid 0x%02x assigned",  assigned_tid);
        }
    }

    return rc;
}

// Command 2: PLDM_GET_TID
static int
pldm_get_tid(struct mctp *mctp, uint8_t src_eid, uint8_t tag, uint8_t *msg)
{
    struct pldm_msg_body resp;
    uint8_t resp_len;
    int rc;

    // Request message header returned with response
    resp.msg_type  = msg[0];           // MsgType
    resp.msg_hdr   = msg[1] & 0x7f;    // Clear Rq
    resp.pldm_type = msg[2];           // CmdCode
    resp.cmd_code  = msg[3];

    // Response data
    resp.data[0] = PLDM_SUCCESS;
    resp.data[1] = pldm_settings.assigned_tid;
    resp_len = 6;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == PLDM_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

// Command 3: PLDM_GET_PLDM_VERSION
static int
pldm_get_pldm_version(struct mctp *mctp, uint8_t src_eid, uint8_t tag,
                      uint8_t *msg)
{
    struct pldm_msg_body resp;
    size_t resp_len;
    int rc;

    // Request message header returned with response
    resp.msg_type  = msg[0];           // MsgType
    resp.msg_hdr   = msg[1] & 0x7f;    // Clear Rq
    resp.pldm_type = msg[2];           // CmdCode
    resp.cmd_code  = msg[3];

    // Response data
    resp.data[0] = PLDM_SUCCESS;
    resp.data[1] = 0x08;
    resp.data[2] = 0x00;
    resp.data[3] = 0x00;
    resp.data[4] = 0x00;
    resp.data[5] = 0x05;
    resp.data[6] = 0x00;
    resp.data[7] = 0xf0;
    resp.data[8] = 0xf0;
    resp.data[9] = 0xf1;
    resp.data[10] = 0xfb;
    resp.data[11] = 0x8f;
    resp.data[12] = 0x86;
    resp.data[13] = 0x4a;
    resp_len = 18;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == PLDM_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

// Command 4: PLDM_GET_PLDM_TYPES
static int
pldm_get_pldm_types(struct mctp *mctp, uint8_t src_eid, uint8_t tag,
                    uint8_t *msg)
{
    struct pldm_msg_body resp;
    size_t resp_len;
    int rc;

    // Request message header returned with response
    resp.msg_type  = msg[0];           // MsgType
    resp.msg_hdr   = msg[1] & 0x7f;    // Clear Rq
    resp.pldm_type = msg[2];           // CmdCode
    resp.cmd_code  = msg[3];

    // Response data
    resp.data[0] = PLDM_SUCCESS;
    resp.data[1] = 5;    // bitfield 7
    resp.data[2] = 0;    // bitfield 6
    resp.data[3] = 0;    // bitfield 5
    resp.data[4] = 0;    // bitfield 4
    resp.data[5] = 0;    // bitfield 3
    resp.data[6] = 0;    // bitfield 2
    resp.data[7] = 0;    // bitfield 1
    resp.data[8] = 0;    // bitfield 0
    resp_len = 13;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == PLDM_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

// Command 5: PLDM_GET_PLDM_COMMANDS
static int
pldm_get_pldm_commands(struct mctp *mctp, uint8_t src_eid, uint8_t tag,
                       uint8_t *msg)
{
    struct pldm_msg_body resp;
    size_t resp_len;
    int rc;

    // Request message header returned with response
    resp.msg_type  = msg[0];           // MsgType
    resp.msg_hdr   = msg[1] & 0x7f;    // Clear Rq
    resp.pldm_type = msg[2];           // CmdCode
    resp.cmd_code  = msg[3];

    // Response data
    resp.data[0] = PLDM_SUCCESS;
    resp.data[1] = 0x3e;
    resp.data[2] = 0x00;
    resp.data[3] = 0x00;
    resp.data[4] = 0x00;

    resp.data[5] = 0x00;
    resp.data[6] = 0x00;
    resp.data[7] = 0x00;
    resp.data[8] = 0x00;
    resp.data[9] = 0x00;
    resp.data[10] = 0x00;
    resp.data[11] = 0x00;
    resp.data[12] = 0x00;
    resp.data[13] = 0x00;
    resp.data[14] = 0x00;
    resp.data[15] = 0x00;
    resp.data[16] = 0x00;
    resp.data[17] = 0x00;
    resp.data[18] = 0x00;
    resp.data[19] = 0x00;
    resp.data[20] = 0x00;

    resp.data[21] = 0x00;
    resp.data[22] = 0x00;
    resp.data[23] = 0x00;
    resp.data[24] = 0x00;
    resp.data[25] = 0x00;
    resp.data[26] = 0x00;
    resp.data[27] = 0x00;
    resp.data[28] = 0x00;
    resp.data[29] = 0x00;
    resp.data[30] = 0x00;
    resp.data[31] = 0x00;
    resp.data[32] = 0x00;
    resp_len = 37;
    resp_len = 20; // ok
    //resp_len = 30;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == PLDM_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

static void
update_pldmd_log_level(void)
{
    FILE *fp;
    int val;
    int rc;

    fp = fopen("/tmp/pldmd_log_level", "r");
    if (fp) {
        rc = fscanf(fp, "%d", &val);
        if (rc) {
            if (val >= MCTP_LOG_ERR && val <= MCTP_LOG_DEBUG) {
                if (get_pldmd_log_level() != val) {
                    set_pldmd_log_level(val);
                }
            }
        }
        fclose(fp);
    } else {
        if (get_pldmd_log_level() != MCTP_LOG_INFO)
            set_pldmd_log_level(MCTP_LOG_INFO);
    }
}

static void
rx_message(uint8_t src_eid, void *data, void *msg, size_t len, uint8_t tag)
{
    struct ctx *ctx = (struct ctx *)data;
    struct mctp *mctp = ctx->mctp;
    uint8_t cmd = *((uint8_t *)msg + 3);

    if (len < 2)
        return;

    update_pldmd_log_level();

    log_rx_pkt(src_eid, (uint8_t *)msg, len, tag);

    switch(cmd) {
    case PLDM_SET_TID:
        pldm_set_tid(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case PLDM_GET_TID:
        pldm_get_tid(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case PLDM_GET_PLDM_VERSION:
        pldm_get_pldm_version(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case PLDM_GET_PLDM_TYPES:
        pldm_get_pldm_types(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case PLDM_GET_PLDM_COMMANDS:
        pldm_get_pldm_commands(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    default:
        not_supported(mctp, src_eid, tag, (uint8_t *)msg);
    }

    return;
}

static int
binding_null_init(struct mctp *mctp __unused, struct binding *binding __unused,
                  mctp_eid_t eid __unused, int n_params,
                  char * const *params __unused)
{
    if (n_params != 0) {
        g_log->warn("null binding doesn't accept parameters");
        return -1;
    }
    return 0;
}

static int
binding_i2c_init(struct mctp *mctp, struct binding *binding, mctp_eid_t eid,
                 int n_params, char * const *params)
{
    struct mctp_binding_i2c *i2c;
    const char *path;
    int rc;

    if (n_params != 1) {
        fprintf(stderr, "i2c binding requires device param\n");
        return -1;
    }

    path = params[0];
    g_log->debug("binding_i2c_init: %s", path);

    i2c = mctp_i2c_init();
    assert(i2c);

    rc = mctp_i2c_open_path(i2c, path);
    if (rc)
        return -1;

    mctp_register_bus(mctp, mctp_binding_i2c_core(i2c), eid);

    binding->data = i2c;

    return 0;
}

static int
binding_i2c_get_fd(struct binding *binding)
{
    return mctp_i2c_get_fd((mctp_binding_i2c *)binding->data);
}

static int
binding_i2c_process(struct binding *binding)
{
    return mctp_i2c_read((mctp_binding_i2c *)binding->data);
}

struct binding bindings[] = {
    {
        .name = "null",
        .init = binding_null_init,
    },
    {
        .name = "i2c",
        .init = binding_i2c_init,
        .get_fd = binding_i2c_get_fd,
        .process = binding_i2c_process,
    }
};

struct binding *binding_lookup(const char *name)
{
    struct binding *binding;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(bindings); i++) {
        binding = &bindings[i];

        if (!strcmp(binding->name, name))
            return binding;
    }

    return NULL;
}

static int
binding_init(struct ctx *ctx, const char *name, int argc, char * const *argv)
{
    int rc;

    g_log->debug("binding_init: %s", name);

    ctx->binding = binding_lookup(name);
    if (!ctx->binding) {
        fprintf(stdout, "no such binding '%s'\n", name);
        return -1;
    }

    rc = ctx->binding->init(ctx->mctp, ctx->binding, ctx->local_eid,
                            argc, argv);

    return rc;
}

enum {
    FD_BINDING = 0,
    FD_SOCKET,
    FD_NR,
};

static int
run_daemon(struct ctx *ctx)
{
    int rc;

    g_log->info("STARTING");

    ctx->pollfds = (pollfd *)malloc(FD_NR * sizeof(struct pollfd));

    if (ctx->binding->get_fd) {
        ctx->pollfds[FD_BINDING].fd =
            ctx->binding->get_fd(ctx->binding);
        ctx->pollfds[FD_BINDING].events = POLLIN;
    } else {
        ctx->pollfds[FD_BINDING].fd = -1;
        ctx->pollfds[FD_BINDING].events = 0;
    }

    mctp_set_rx_all(ctx->mctp, rx_message, ctx);

    for (;;) {
        rc = poll(ctx->pollfds, FD_NR, -1);
        if (rc < 0) {
            g_log->warn("poll failed");
            break;
        }

        if (ctx->pollfds[FD_BINDING].revents) {
            rc = 0;
            if (ctx->binding->process) {
                rc = ctx->binding->process(ctx->binding);
            }
            if (rc)
                break;
        }
    }

    free(ctx->pollfds);

    return rc;
}

static const struct option options[] = {
    { "verbose", no_argument, 0, 'v' },
    { 0 },
};

static void
usage(const char *progname)
{
    unsigned int i;

    fprintf(stdout, "usage: %s <binding> [params]\n", progname);
    fprintf(stdout, "Available bindings:\n");
    for (i = 0; i < ARRAY_SIZE(bindings); i++)
        fprintf(stdout, "  %s\n", bindings[i].name);
}

int
main(int argc, char * const *argv)
{
    struct ctx *ctx, _ctx;
    int rc;

    ctx = &_ctx;
    ctx->local_eid = local_eid_default;
    ctx->verbose = false;

    memset(&pldm_settings, 0, sizeof(pldm_settings));

    for (;;) {
        rc = getopt_long(argc, argv, "v", options, NULL);
        if (rc == -1)
            break;
        switch (rc) {
        case 'v':
            ctx->verbose = true;
            break;
        default:
            fprintf(stderr, "Invalid argument\n");
            return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "missing binding argument\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* setup initial buffer */
    ctx->buf_size = 4096;
    ctx->buf = malloc(ctx->buf_size);

    ctx->mctp = mctp_init();
    assert(ctx->mctp);

    rc = binding_init(ctx, argv[optind], argc - optind - 1, argv + optind + 1);
    if (rc)
        return EXIT_FAILURE;

    rc = run_daemon(ctx);

    return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
