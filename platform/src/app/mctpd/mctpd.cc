/*
 * Copyright (C) 2020 Pensando Systems Inc.
 *
 * Pensando MCTP Control Daemon (mctpd)
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
#include "lib/catalog/catalog.hpp"

// Logging via operd, configured in nic/sysmgr/iris/operd-regions.json
extern sdk::operd::logger_ptr g_log;
sdk::operd::logger_ptr g_log = sdk::operd::logger::create("mctpd");

// mctp over i2c uses smbus block read/write with maximum size 255
// mctp control commands always fit in a single 64 byte packet
struct mctp_msg_body {
    uint8_t msg_type;
    uint8_t msg_hdr;     // Rq,D,Instance ID
    uint8_t cmd_code;
    uint8_t data[64];
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

static struct mctp_settings {
    uint8_t  assigned_eid;
    uint16_t pcie_vendorid;
    uint16_t pcie_subvendorid;
    uint16_t pcie_subdeviceid;
} mctp_settings;

static int mctpd_log_level = MCTP_LOG_INFO;
//static int mctpd_log_level = MCTP_LOG_DEBUG;

void
set_mctpd_log_level(int log_level)
{
    mctpd_log_level = log_level;
}

int
get_mctpd_log_level(void)
{
    return mctpd_log_level;
}

void
log_ctrl_cmd(uint8_t cmd)
{
    switch(cmd) {
    case MCTP_CTRL_RSVD:
        g_log->info("## MCTP_CTRL_RSVD ##");
        break;
    case MCTP_SET_EID:
        g_log->info("## MCTP_SET_EID ##");
        break;
    case MCTP_GET_EID:
        g_log->info("## MCTP_GET_EID ##");
        break;
    case MCTP_GET_UUID:
        g_log->info("## MCTP_GET_UUID ##");
        break;
    case MCTP_GET_VERSION_SUPPORT:
        g_log->info("## MCTP_GET_VERSION_SUPPORT ##");
        break;
    case MCTP_GET_MSG_SUPPORT:
        g_log->info("## MCTP_GET_MSG_SUPPORT ##");
        break;
    case MCTP_GET_VENDOR_MSG_SUPPORT:
        g_log->info("## MCTP_GET_VENDOR_MSG_SUPPORT ##");
        break;
    case MCTP_RESOLVE_EID:
        g_log->info("## MCTP_RESOLVE_EID ##");
        break;
    case MCTP_ALLOCATE_EIDS:
        g_log->info("## MCTP_ALLOCATE_EIDS ##");
        break;
    case MCTP_ROUTING_INFO_UPDATE:
        g_log->info("## MCTP_ROUTING_INFO_UPDATE ##");
        break;
    case MCTP_GET_ROUTING_TABLE:
        g_log->info("## MCTP_GET_ROUTING_TABLE ##");
        break;
    case MCTP_PREPARE_ENDPOINT_DISCOVERY:
        g_log->info("## MCTP_PREPARE_ENDPOINT_DISCOVERY ##");
        break;
    case MCTP_ENDPOINT_DISCOVERY:
        g_log->info("## MCTP_ENDPOINT_DISCOVERY ##");
        break;
    case MCTP_DISCOVERY_NOTIFY:
        g_log->info("## MCTP_DISCOVERY_NOTIFY ##");
        break;
    case MCTP_GET_NETWORK_ID:
        g_log->info("## MCTP_GET_NETWORK_ID ##");
        break;
    case MCTP_QUERY_HOP:
        g_log->info("## MCTP_QUERY_HOP ##");
        break;
    case MCTP_RESOLVE_UUID:
        g_log->info("## MCTP_RESOLVE_UUID ##");
        break;
    case MCTP_QUERY_RATE_LIMIT:
        g_log->info("## MCTP_QUERY_RATE_LIMIT ##");
        break;
    case MCTP_RQST_TX_RATE_LIMIT:
        g_log->info("## MCTP_RQST_TX_RATE_LIMIT ##");
        break;
    case MCTP_UPDATE_RATE_LIMIT:
        g_log->info("## MCTP_UPDATE_RATE_LIMIT ##");
        break;
    case MCTP_QUERY_SUPPORTED_INTERFACES:
        g_log->info("## MCTP_QUERY_SUPPORTED_INTERFACES ##");
        break;
    default:
        g_log->info("## MCTP CMD UNKNOWN ##");
    }
}

static void
log_rx_pkt(uint8_t src_eid, uint8_t msg[], size_t len, uint8_t tag)
{
    uint8_t type = msg[0] & 0x7f;
    uint8_t id = msg[1] & 0x1f;
    uint8_t cmd = msg[2];
    char log[100];
    char *logp = log;
    int i, cnt;

    log_ctrl_cmd(cmd);

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
    uint8_t cmd = msg[2];
    char log[100];
    char *logp = log;
    int i, cnt;

    // Log the first 16 data bytes makes each entry 50 + 48 = 98 bytes
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
    struct mctp_msg_body resp;
    uint8_t resp_len;
    int rc;

    g_log->info("MCTP command %d not supported", msg[2]);

    // Request message header returned with response
    resp.msg_type = msg[0];           // MsgType
    resp.msg_hdr  = msg[1] & 0x7f;    // Clear Rq
    resp.cmd_code = msg[2];           // CmdCode

    // Response data
    resp.data[0] = MCTP_CTRL_UNSUPPORTED_CMD;
    resp_len = 4;

    log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    return rc;
}

// Command 1: MCTP_SET_EID
static int
mctp_set_eid(struct mctp *mctp, uint8_t src_eid, uint8_t tag, uint8_t msg[])
{
    struct mctp_msg_body resp;
    uint8_t resp_len;
    uint8_t assigned_eid;
    bool valid_eid = false;
    int fd;
    int rc;

    // Request message header returned with response
    resp.msg_type = msg[0];           // MsgType
    resp.msg_hdr  = msg[1] & 0x7f;    // Clear Rq
    resp.cmd_code = msg[2];           // CmdCode

    // Request data
    assigned_eid  = msg[4];

    // Response data
    if ((assigned_eid) == 0 || (assigned_eid == 0xff)) {
        resp.data[0] = MCTP_CTRL_INVALID_DATA;
        resp.data[1] = EID_REJECTED;
        resp.data[2] = mctp_settings.assigned_eid;
    } else {
        resp.data[0] = MCTP_CTRL_SUCCESS;
        resp.data[1] = EID_ACCEPTED;
        resp.data[2] = assigned_eid;
        valid_eid = true;
    }
    resp.data[3] = NO_DYNAMIC_EID_POOL;
    resp_len = 7;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == MCTP_CTRL_SUCCESS && valid_eid) {
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

        mctp_settings.assigned_eid = assigned_eid;
        g_log->info("mctp eid 0x%02x assigned", assigned_eid);

        // Update bus eid setting in core library
        mctp_set_bus_eid(mctp, assigned_eid);

        // Store the current setting in the driver
        fd = open("/dev/mctp", O_RDWR);
        if (fd < 0) {
            g_log->err("error opening /dev/mctp");
            return -1;
        }
        rc = ioctl(fd, SET_EID, &assigned_eid);
        if (rc)
            g_log->err("mctp set eid ioctl error - %d", rc);
        else
            g_log->info("mctp set eid 0x%02x in driver", assigned_eid);

        close(fd);
    } else {
        g_log->info("invalid mctp eid 0x%02x assigned",  assigned_eid);
    }

    return rc;
}
 
// Command 2: MCTP_GET_EID
static int
mctp_get_eid(struct mctp *mctp, uint8_t src_eid, uint8_t tag, uint8_t msg[])
{
    struct mctp_msg_body resp;
    size_t resp_len;
    int rc;

    // Request message header returned with response
    resp.msg_type = msg[0];           // MsgType
    resp.msg_hdr  = msg[1] & 0x7f;    // Clear Rq
    resp.cmd_code = msg[2];           // CmdCode

    // Response data
    resp.data[0] = MCTP_CTRL_SUCCESS;
    resp.data[1] = mctp_settings.assigned_eid;
    resp.data[2] = 0;                 // simple endpoint and dynamic eid
    resp.data[3] = 0;                 // medium specific
    resp_len = 7;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == MCTP_CTRL_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

// Command 3: MCTP_GET_UUID
static int
mctp_get_uuid(struct mctp *mctp, uint8_t src_eid, uint8_t tag, uint8_t msg[])
{
    struct mctp_msg_body resp;
    size_t resp_len;
    int rc;

    // Request message header returned with response
    resp.msg_type = msg[0];           // MsgType
    resp.msg_hdr  = msg[1] & 0x7f;    // Clear Rq
    resp.cmd_code = msg[2];           // CmdCode

    // Version 4:  https://www.uuidgenerator.net/version4
    // A Version 4 UUID is a universally unique identifier that
    // is generated using random numbers. The Version 4 UUIDs 
    // produced by this site were generated using a secure random 
    // number generator.
    //
    // d3d22581-1016-42f1-91e1-ac1bbc7d36ff

    // Response data
    resp.data[0] = MCTP_CTRL_SUCCESS;
    resp.data[1] = 0xd3;
    resp.data[2] = 0xd2;
    resp.data[3] = 0x25;
    resp.data[4] = 0x81;
    resp.data[5] = 0x10;
    resp.data[6] = 0x16;
    resp.data[7] = 0x42;
    resp.data[8] = 0xf1;
    resp.data[9] = 0x91;
    resp.data[10] = 0xe1;
    resp.data[11] = 0xac;
    resp.data[12] = 0x1b;
    resp.data[13] = 0xbc;
    resp.data[14] = 0x7d;
    resp.data[15] = 0x36;
    resp.data[16] = 0xff;
    resp_len = 20;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == MCTP_CTRL_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

// Command 4: MCTP_GET_VERSION_SUPPORT
static int
mctp_get_version_support(struct mctp *mctp, uint8_t src_eid, uint8_t tag,
                         uint8_t msg[])
{
    struct mctp_msg_body resp;
    size_t resp_len;
    uint8_t msg_type_num;
    int rc;

    // Request message header returned with response
    resp.msg_type = msg[0];           // MsgType
    resp.msg_hdr  = msg[1] & 0x7f;    // Clear Rq
    resp.cmd_code = msg[2];           // CmdCode

    // Response data
    resp.data[0] = MCTP_CTRL_SUCCESS;
    resp.data[1] = 1;                 // Version number entry count
    resp.data[2] = 0xf1;              // Major version number
    resp.data[3] = 0xf0;              // Minor version number

    msg_type_num = msg[3];
    if (msg_type_num == 1)
        resp.data[4] = 0xf0;          // Update version number    
    else if (msg_type_num == 2)
        resp.data[4] = 0xf0;
    else
        resp.data[4] = 0xff;
    resp.data[5] = 0x00;              // Alpha byte
    resp_len = 9;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == MCTP_CTRL_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

// Command 5: MCTP_GET_MSG_SUPPORT
static int
mctp_get_msg_type_support(struct mctp *mctp, uint8_t src_eid, uint8_t tag,
                          uint8_t msg[])
{
    struct mctp_msg_body resp;
    size_t resp_len;
    int rc;

    // Request message header returned with response
    resp.msg_type = msg[0];           // MsgType
    resp.msg_hdr  = msg[1] & 0x7f;    // Clear Rq
    resp.cmd_code = msg[2];           // CmdCode

    // Response data
    resp.data[0] = MCTP_CTRL_SUCCESS;
    resp.data[1] = 3;                 // Count of supported MCTP msg types
    resp.data[2] = 0;                 // MCTP Control
    resp.data[3] = 2;                 // NC-SI over MCTP
    resp.data[4] = 1;                 // PLDM over MCTP
    resp_len = 8;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == MCTP_CTRL_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

// Command 6: MCTP_GET_VENDOR_MSG_SUPPORT
static int
mctp_get_vendor_msg_support(struct mctp *mctp, uint8_t src_eid, uint8_t tag,
                            uint8_t msg[])
{
    struct mctp_msg_body resp;
    size_t resp_len;
    int rc;

    // Request message header returned with response
    resp.msg_type = msg[0];           // MsgType
    resp.msg_hdr  = msg[1] & 0x7f;    // Clear Rq
    resp.cmd_code = msg[2];           // CmdCode

    // TODO get from catalog
    // Response data
    resp.data[0] = MCTP_CTRL_SUCCESS;
    resp.data[1] = 0xff;              // No more capability sets
    resp.data[2] = 0;                 // PCI Vendor ID
    resp.data[3] = 0x1d;              // subvendor id
    resp.data[4] = 0xd8;              // subvendor id
    resp.data[5] = 0xbb;
    resp.data[6] = 0x7a;
    resp_len = 10;

    rc = mctp_message_tx(mctp, src_eid, &resp, resp_len, tag);

    if (rc == MCTP_CTRL_SUCCESS)
        log_tx_pkt(src_eid, msg, resp.data, resp_len - 3, tag);

    return rc;
}

static void
update_mctpd_log_level(void)
{
    FILE *fp;
    int val;
    int rc;

    fp = fopen("/tmp/mctpd_log_level", "r");
    if (fp) {
        rc = fscanf(fp, "%d", &val);
        if (rc) {
            if (val >= MCTP_LOG_ERR && val <= MCTP_LOG_DEBUG) {
                if (get_mctpd_log_level() != val) {
                    set_mctpd_log_level(val);
                }
            }
        } 
        fclose(fp);
    } else {
        if (get_mctpd_log_level() != MCTP_LOG_INFO)
            set_mctpd_log_level(MCTP_LOG_INFO);
    }
}

static void
rx_message(uint8_t src_eid, void *data, void *msg, size_t len, uint8_t tag)
{
    struct ctx *ctx = (struct ctx *)data;
    struct mctp *mctp = ctx->mctp;
    uint8_t cmd = *((uint8_t *)msg + 2);

    if (len < 2)
        return;

    update_mctpd_log_level();   

    log_rx_pkt(src_eid, (uint8_t *)msg, len, tag);

    switch(cmd) {
    case MCTP_SET_EID:
        mctp_set_eid(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case MCTP_GET_EID:
        mctp_get_eid(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case MCTP_GET_UUID:
        mctp_get_uuid(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case MCTP_GET_VERSION_SUPPORT:
        mctp_get_version_support(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case MCTP_GET_MSG_SUPPORT:
        mctp_get_msg_type_support(mctp, src_eid, tag, (uint8_t *)msg);
        break;

    case MCTP_GET_VENDOR_MSG_SUPPORT:
        mctp_get_vendor_msg_support(mctp, src_eid, tag, (uint8_t *)msg);
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
    if (rc == ENOENT) {
        g_log->err("kernel support missing, %s not found, pausing app", path);
        while (1) {
            pause();
        }
    }
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
        g_log->err("no such binding %s", name);
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

void
catalog_to_sysfs_update(void) 
{
    sdk::lib::catalog *catalog = sdk::lib::catalog::factory();
    FILE *fp;
    int rc;

    mctp_settings.pcie_vendorid = catalog->pcie_vendorid();
    mctp_settings.pcie_subvendorid = catalog->pcie_subvendorid();
    mctp_settings.pcie_subdeviceid = catalog->pcie_subdeviceid();

    fp = fopen("/sys/bus/i2c/devices/3-1061/pcie_vendorid", "w");
    if (fp) {
        rc = fwrite(&mctp_settings.pcie_vendorid, 2, 1, fp);
        if (rc) {
            g_log->info("Wrote 0x%x to sysfs 3-1061/pcie_vendorid",
                        catalog->pcie_vendorid());
        } 
        fclose(fp);
    }

    fp = fopen("/sys/bus/i2c/devices/3-1061/pcie_subvendorid", "w");
    if (fp) {
        rc = fwrite(&mctp_settings.pcie_subvendorid, 2, 1, fp);
        if (rc) {
            g_log->info("Wrote 0x%x to sysfs 3-1061/pcie_subvendorid",
                        catalog->pcie_subvendorid());
        } 
        fclose(fp);
    }

    fp = fopen("/sys/bus/i2c/devices/3-1061/pcie_subdeviceid", "w");
    if (fp) {
        rc = fwrite(&mctp_settings.pcie_subdeviceid, 2, 1, fp);
        if (rc) {
            g_log->info("Wrote 0x%x to sysfs 3-1061/pcie_subdeviceid",
                        catalog->pcie_subdeviceid());
        } 
        fclose(fp);
    }
}

static int
run_daemon(struct ctx *ctx)
{
    int rc;

    g_log->info("STARTING");

    // update default settings in mctp driver for pcie_vendorid,
    // pcie_subvendorid, and pcie_subdeviceid
    if (0) catalog_to_sysfs_update();

    ctx->pollfds = (pollfd *)malloc(FD_NR * sizeof(struct pollfd));

    if (ctx->binding->get_fd) {
        ctx->pollfds[FD_BINDING].fd = ctx->binding->get_fd(ctx->binding);
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
            if (ctx->binding->process)
                rc = ctx->binding->process(ctx->binding);
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

    memset(&mctp_settings, 0, sizeof(mctp_settings));

    while(1) {
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

    // setup initial buffer
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
