/*
 * Copyright (c) 2017, Pensando Systems Inc.
 */

/*
 * capri_config.h: Load the Capri configuration
 */

#ifndef _CAPRI_CONFIG_H_
#define _CAPRI_CONFIG_H_

#include "nic/include/base.hpp"

hal_ret_t capri_load_config(char *pathname);

hal_ret_t capri_verify_config(char *config_dir);

#endif   // _CAPRI_CONFIG_H_
