//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// transceiver utils
///
//----------------------------------------------------------------------------

#ifndef __XCVR_UTILS_HPP__
#define __XCVR_UTILS_HPP__

#define MAX_XCVR_ACCESS_RETRIES     5
#define INVALID_TEMP                0
#define XCVR_TEMP_VALID_MIN         1
#define XCVR_TEMP_VALID_MAX         127
#define XCVR_TEMP_VALID(x) ((x > XCVR_TEMP_VALID_MIN) && (x < XCVR_TEMP_VALID_MAX))

#endif
