/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#define _GNU_SOURCE

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "libmctp-log.h"
#include "libmctp-i2c.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct mctp_binding_i2c_pipe {
	int ingress;
	int egress;

	struct mctp_binding_i2c *i2c;
};

static int mctp_binding_i2c_pipe_tx(void *data, void *buf, size_t len)
{
	struct mctp_binding_i2c_pipe *ctx = data;
	ssize_t rc;

	printf("tx: len %ld\n", len);

	rc = write(ctx->egress, buf, len);
	assert(rc == len);

	return rc;
}

uint8_t mctp_msg_src[2 * MCTP_BTU];

static bool seen;

static void rx_message(uint8_t eid, void *data, void *msg, size_t len)
{
	uint8_t type;

	type = *(uint8_t *)msg;

	mctp_prdebug("MCTP message received: len %zd, type %d", len, type);

	assert(sizeof(mctp_msg_src) == len);
	assert(!memcmp(mctp_msg_src, msg, len));

	seen = true;
}

struct i2c_test {
	struct mctp_binding_i2c_pipe binding;
	struct mctp *mctp;
};

int main(void)
{
	struct i2c_test scenario[2];

	struct mctp_binding_i2c_pipe *a;
	struct mctp_binding_i2c_pipe *b;
	int p[2][2];
	int rc;

	printf("test_i2c\n");

	mctp_set_log_stdio(MCTP_LOG_DEBUG);

	memset(&mctp_msg_src[0], 0x5a, MCTP_BTU);
	memset(&mctp_msg_src[MCTP_BTU], 0xa5, MCTP_BTU);

	rc = pipe(p[0]);
	assert(!rc);

	rc = pipe(p[1]);
	assert(!rc);

	/* Instantiate the A side of the i2c pipe */
	scenario[0].mctp = mctp_init();
	assert(scenario[0].mctp);
	scenario[0].binding.i2c = mctp_i2c_init();
	assert(scenario[0].binding.i2c);
	a = &scenario[0].binding;
	a->ingress = p[0][0];
	a->egress = p[1][1];
	mctp_i2c_open_fd(a->i2c, a->ingress);
	mctp_i2c_set_tx_fn(a->i2c, mctp_binding_i2c_pipe_tx, a);
	mctp_register_bus(scenario[0].mctp, mctp_binding_i2c_core(a->i2c), 8);

	/* Instantiate the B side of the i2c pipe */
	scenario[1].mctp = mctp_init();
	assert(scenario[1].mctp);
	mctp_set_rx_all(scenario[1].mctp, rx_message, NULL);
	scenario[1].binding.i2c = mctp_i2c_init();
	assert(scenario[1].binding.i2c);
	b = &scenario[1].binding;
	b->ingress = p[1][0];
	b->egress = p[0][1];
	mctp_i2c_open_fd(b->i2c, b->ingress);
	mctp_i2c_set_tx_fn(b->i2c, mctp_binding_i2c_pipe_tx, a);
	mctp_register_bus(scenario[1].mctp, mctp_binding_i2c_core(b->i2c), 9);

	/* Transmit a message from A to B */
	printf("-----------\n");
	printf("i2c: A send\n");
	printf("-----------\n");
	rc = mctp_message_tx(scenario[0].mctp, 9, mctp_msg_src, sizeof(mctp_msg_src));
	assert(rc == 0);

	/* Read the message at B from A */
	printf("-----------\n");
	printf("i2c: B read\n");
	printf("-----------\n");
	seen = false;
	mctp_i2c_read(b->i2c);
	assert(seen);

	mctp_i2c_destroy(scenario[1].binding.i2c);
	mctp_destroy(scenario[1].mctp);
	mctp_i2c_destroy(scenario[0].binding.i2c);
	mctp_destroy(scenario[0].mctp);

	return 0;
}
