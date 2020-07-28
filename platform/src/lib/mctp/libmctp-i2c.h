/*
 * Copyright 2020 Pensando Systems Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef _LIBMCTP_I2C_H
#define _LIBMCTP_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libmctp.h>

struct mctp_binding_i2c;

struct mctp_binding_i2c *mctp_i2c_init(void);
void mctp_i2c_destroy(struct mctp_binding_i2c *i2c);

struct mctp_binding *mctp_binding_i2c_core(struct mctp_binding_i2c *b);

/* file-based IO */
int mctp_i2c_get_fd(struct mctp_binding_i2c *i2c);
void mctp_i2c_register_bus(struct mctp_binding_i2c *i2c, struct mctp *mctp,
                           mctp_eid_t eid);
int mctp_i2c_read(struct mctp_binding_i2c *i2c);
int mctp_i2c_open_path(struct mctp_binding_i2c *i2c, const char *path);
void mctp_i2c_open_fd(struct mctp_binding_i2c *i2c, int fd);

/* direct function call IO */
typedef int (*mctp_i2c_tx_fn)(void *data, void *buf, size_t len)
             __attribute__((warn_unused_result));
void mctp_i2c_set_tx_fn(struct mctp_binding_i2c *i2c, mctp_i2c_tx_fn fn,
                        void *data);
int mctp_i2c_rx(struct mctp_binding_i2c *i2c, const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _LIBMCTP_I2C_H */
