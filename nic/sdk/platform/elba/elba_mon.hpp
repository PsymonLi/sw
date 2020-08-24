//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// elbamon headers
///
//----------------------------------------------------------------------------

#ifndef __ELBA_MON_HPP__
#define __ELBA_MON_HPP__

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include "include/sdk/base.hpp"
#include "asic/rw/asicrw.hpp"
#include "third-party/asic/elba/verif/apis/elb_freq_sw_api.h"

namespace sdk {
namespace platform {
namespace elba {

typedef struct hbmerrcause_s {
    uint32_t offset;
    std::string message;
} hbmerrcause_t;

#define MAX_CHANNEL   8
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)       (sizeof (a) / sizeof ((a)[0]))
#endif

pen_adjust_perf_status_t elba_adjust_perf(int chip_id, int inst_id,
                                          pen_adjust_index_t &idx,
                                          pen_adjust_perf_type_t perf_type);
void elba_set_half_clock(int chip_id, int inst_id);
sdk_ret_t elba_unravel_hbm_intrs(bool *iscattrip, bool *iseccerr,
                                  bool logging);

}   // namespace elba
}   // namespace platform
}   // namespace sdk

#endif
