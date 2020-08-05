//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ftl internal defines
///
//----------------------------------------------------------------------------

#ifndef __FTL_DEFINES_HPP__
#define __FTL_DEFINES_HPP__

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cinttypes>

#ifdef SIM
#define FTL_TRACES_ENABLE
#endif

#ifdef FTL_TRACES_ENABLE
#define FTL_TRACE_VERBOSE(_msg, _args...) SDK_TRACE_VERBOSE(_msg, ##_args)
#define FTL_TRACE_DEBUG(_msg, _args...) SDK_TRACE_DEBUG(_msg, ##_args)
#else
#define FTL_TRACE_VERBOSE(_msg, _args...)
#define FTL_TRACE_DEBUG(_msg, _args...)
#endif

#define FTL_TRACE_INFO(_msg, _args...) SDK_TRACE_INFO(_msg, ##_args)
#define FTL_TRACE_ERR(_msg, _args...) SDK_TRACE_ERR(_msg, ##_args)
#define FTL_SNPRINTF(_buf, _len, _fmt, _args...) snprintf(_buf, _len, _fmt, ##_args)

#define FTL_RET_CHECK_AND_GOTO(_status, _label, _msg, _args...) {   \
    if (unlikely((_status) != sdk::SDK_RET_OK)) {                   \
        if (((_status) == SDK_RET_ENTRY_EXISTS) ||                  \
            ((_status) == SDK_RET_RETRY)) {                         \
            SDK_TRACE_VERBOSE(_msg, ##_args);                       \
        } else {                                                    \
            SDK_TRACE_ERR(_msg, ##_args);                           \
        }                                                           \
        goto _label;                                                \
    }                                                               \
}

#define PDS_FLOW_HINT_POOL_COUNT_MAX    256
#define PDS_FLOW_HINT_POOLS_MAX         8

#endif    // __FTL_DEFINES_HPP__
