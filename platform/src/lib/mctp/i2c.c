/* 
 * Copyright (C) 2020 Pensando Systems Inc.
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Post-condition: All bytes written or an error has occurred */
#define mctp_write_all(fn, dst, src, len)    \
({                                           \
    ssize_t wrote;                           \
    while (len) {                            \
        wrote = fn(dst, src, len);           \
        if (wrote < 0)                       \
            break;                           \
        len -= wrote;                        \
    }                                        \
    len ? -1 : 0;                            \
})

#include "libmctp.h"
#include "libmctp-alloc.h"
#include "libmctp-log.h"
#include "libmctp-i2c.h"
#include "container_of.h"

/* smbus arp in the dw driver supports fixed address */
#define I2C_SLAVE_ADDR    0x93      /* bit0 set for mctp */

struct mctp_binding_i2c {
    struct mctp_binding binding;
    int                 fd;
    unsigned long       bus_id;

    mctp_i2c_tx_fn      tx_fn;
    void                *tx_fn_data;

    /* receive buffer and state */
    uint8_t             rxbuf[1024];
    struct mctp_pktbuf  *rx_pkt;
    uint8_t             rx_exp_len;
    uint16_t            rx_pec;
    uint8_t             src_addr;
    enum {
        STATE_WAIT_MCTP_CMD,
        STATE_WAIT_LEN,
        STATE_WAIT_SRC_ADDR,
        STATE_DATA,
        STATE_WAIT_PEC,
    } rx_state;

    /* temporary transmit buffer */
    uint8_t             txbuf[256];
};

#define binding_to_i2c(b) \
    container_of(b, struct mctp_binding_i2c, binding)

struct mctp_i2c_header {
    uint8_t dest;
    uint8_t cmd;
    uint8_t len;
    uint8_t src;
};

static int
mctp_binding_i2c_tx(struct mctp_binding *b, struct mctp_pktbuf *pkt)
{
    struct mctp_binding_i2c *i2c = binding_to_i2c(b);
    struct mctp_i2c_header *hdr;
    uint8_t *buf;
    size_t len;

    /* I2C transport header */
    len = mctp_pktbuf_size(pkt);
    hdr = (void *)i2c->txbuf;
    hdr->src = I2C_SLAVE_ADDR;
    hdr->dest = i2c->src_addr & 0xfe;   // 8-bit i2c address
    hdr->cmd = 0xf;
    hdr->len = len + 1;                 // Document of fix odd math

    /* Copy MCTP header and data into buf */
    buf = (void *)(hdr + 1);            // +4 bytes
    memcpy(buf, pkt->data, len);
    len += sizeof(*hdr);

    if (!i2c->tx_fn) {
        return mctp_write_all(write, i2c->fd, i2c->txbuf, len);
    }

    return mctp_write_all(i2c->tx_fn, i2c->tx_fn_data, i2c->txbuf, len);
}

static void
mctp_i2c_finish_packet(struct mctp_binding_i2c *i2c, bool valid)
{
    struct mctp_pktbuf *pkt = i2c->rx_pkt;
    assert(pkt);

    mctp_prdebug("finish packet, valid %s", valid ? "true" : "false");

    if (valid) {
        mctp_bus_rx(&i2c->binding, pkt);
    }
    i2c->rx_pkt = NULL;
}

static void
update_libmctp_log_level(void)
{
    FILE *fp;
    int val;
    int rc;

    fp = fopen("/tmp/libmctp_log_level", "r");
    if (fp) {
        rc = fscanf(fp, "%d", &val);
        if (rc) {
            if (val >= MCTP_LOG_ERR && val <= MCTP_LOG_DEBUG) {
                if (get_log_level() != val) {
                    set_log_level(val);
                }
            }
        }
        fclose(fp);
    } else {
        if (get_log_level() != MCTP_LOG_INFO)
            set_log_level(MCTP_LOG_INFO);
    }
}

static void
mctp_i2c_start_packet(struct mctp_binding_i2c *i2c, uint8_t len)
{
    update_libmctp_log_level();

    i2c->rx_pkt = mctp_pktbuf_alloc(&i2c->binding, len);
}

static void
mctp_rx_consume_one(struct mctp_binding_i2c *i2c, uint8_t c)
{
    struct mctp_pktbuf *pkt = i2c->rx_pkt;
    struct mctp_hdr *hdr;

    switch (i2c->rx_state) {
    case STATE_WAIT_MCTP_CMD:
        mctp_prdebug("STATE_WAIT_MCTP_CMD: c 0x%02x", c);
        if (c != 0x0f) {
            mctp_prinfo("%s lost sync, dropping packet 0x%02x", __func__, c);
            if (pkt)
                mctp_i2c_finish_packet(i2c, false);
        } else {
            i2c->rx_state = STATE_WAIT_LEN;
        }
        break;

    case STATE_WAIT_LEN:
        mctp_prdebug("STATE_WAIT_LEN: len %d", c);
        if (c > i2c->binding.pkt_size || c < sizeof(struct mctp_hdr)) {
            mctp_prinfo("%s: invalid size %d", __func__, c);
            i2c->rx_state = STATE_WAIT_MCTP_CMD;
        } else {
            i2c->rx_exp_len = c;
            i2c->rx_state = STATE_WAIT_SRC_ADDR;
        }
        break;

    case STATE_WAIT_SRC_ADDR:
        mctp_prdebug("STATE_WAIT_SRC_ADDR: addr 0x%02x", c);
        i2c->src_addr = c;
        mctp_i2c_start_packet(i2c, 0);
        pkt = i2c->rx_pkt;
        i2c->rx_state = STATE_DATA;
        break;

    case STATE_DATA:
        mctp_prdebug("STATE_DATA: data 0x%02x", c);
        mctp_pktbuf_push(pkt, &c, 1);
        // TODO fix strange math
        if (pkt->end - pkt->mctp_hdr_off + 1 == i2c->rx_exp_len)
            i2c->rx_state = STATE_WAIT_PEC;
        break;

    case STATE_WAIT_PEC:
        mctp_prdebug("STATE_WAIT_PEC: pec 0x%02x", c);
        i2c->rx_pec = c;
        // Check PEC here, if fails drop and back to wait mctp cmd.
        hdr = mctp_pktbuf_hdr(pkt);
        mctp_prdebug("mctp hdr ver %d dest 0x%02x src 0x%02x flags 0x%02x",
                     hdr->ver, hdr->dest, hdr->src, hdr->flags_seq_tag);
        mctp_i2c_finish_packet(i2c, true);
        i2c->rx_state = STATE_WAIT_MCTP_CMD;
        break;
    }
}

static void
mctp_rx_consume(struct mctp_binding_i2c *i2c, const void *buf, size_t len)
{
    size_t i;

    mctp_prdebug("consume %ld bytes", len);

    for (i = 0; i < len; i++)
        mctp_rx_consume_one(i2c, *(uint8_t *)(buf + i));
}

int
mctp_i2c_read(struct mctp_binding_i2c *i2c)
{
    ssize_t len;
    char log[256];
    char *logp = log;
    int i;

    len = read(i2c->fd, i2c->rxbuf, sizeof(i2c->rxbuf));
    if (len == 0) {
        return 0;
    }

    if (len < 0) {
        fprintf(stdout, "%s can't read from i2c device", __func__);
        return -1;
    }

    logp += sprintf(logp, "RX len %lu pkt ", len);
    for (i=0; i<len; i++)
        logp += sprintf(logp, "%02x ", i2c->rxbuf[i]);
    mctp_prdebug("%s", log);

    mctp_rx_consume(i2c, i2c->rxbuf, len);

    return 0;
}

int
mctp_i2c_get_fd(struct mctp_binding_i2c *i2c)
{
    return i2c->fd;
}

int
mctp_i2c_open_path(struct mctp_binding_i2c *i2c, const char *device)
{
    i2c->fd = open(device, O_RDWR);
    if (i2c->fd < 0) {
        mctp_prerr("can't open device %s", device);
        return -1;
    }
    return 0;
}

void
mctp_i2c_open_fd(struct mctp_binding_i2c *i2c, int fd)
{
    i2c->fd = fd;
}

void
mctp_i2c_register_bus(struct mctp_binding_i2c *i2c,
                      struct mctp *mctp, mctp_eid_t eid)
{
    mctp_prdebug("%s eid 0x%x i2c->fd %d", __func__, eid, i2c->fd);

    assert(i2c->fd >= 0);
    mctp_register_bus(mctp, &i2c->binding, eid);
    mctp_binding_set_tx_enabled(&i2c->binding, true);
}

int
mctp_i2c_rx(struct mctp_binding_i2c *i2c, const void *buf, size_t len)
{
    mctp_rx_consume(i2c, buf, len);
    return 0;
}

static int
mctp_i2c_core_start(struct mctp_binding *binding)
{
    mctp_binding_set_tx_enabled(binding, true);
    return 0;
}

struct mctp_binding*
mctp_binding_i2c_core(struct mctp_binding_i2c *b)
{
    return &b->binding;
}

struct mctp_binding_i2c*
mctp_i2c_init(void)
{
    struct mctp_binding_i2c *i2c;

    mctp_prdebug("mctp_i2c_init");

    i2c = __mctp_alloc(sizeof(*i2c));
    memset(i2c, 0, sizeof(*i2c));
    i2c->fd = -1;
    i2c->rx_state = STATE_WAIT_MCTP_CMD;
    i2c->rx_pkt = NULL;
    i2c->binding.name = "i2c";
    i2c->binding.version = 1;
    i2c->binding.pkt_size = MCTP_PACKET_SIZE(MCTP_BTU);
    i2c->binding.pkt_pad = 0;

    i2c->binding.start = mctp_i2c_core_start;
    i2c->binding.tx = mctp_binding_i2c_tx;

    return i2c;
}

void
mctp_i2c_destroy(struct mctp_binding_i2c *i2c)
{
    __mctp_free(i2c);
}
