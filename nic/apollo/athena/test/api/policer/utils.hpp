//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cinttypes>
#include "nic/apollo/athena/api/include/pds_policer.h"
#include "nic/apollo/athena/api/impl/pds_policer_impl.hpp"

#define MHTEST_CHECK_RETURN(_exp, _ret) if (!(_exp)) return (_ret)

void fill_key(pds_policer_key_t *key,
              uint16_t index);

void fill_data(pds_policer_data_t *data,
               uint64_t rate,
               uint64_t burst);

void update_data(pds_policer_data_t *data,
                 uint64_t rate,
                 uint64_t burst);

#endif // __UTILS_HPP__
