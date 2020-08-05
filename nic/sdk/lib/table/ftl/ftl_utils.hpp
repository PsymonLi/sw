//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __FTL_UTILS_HPP__
#define __FTL_UTILS_HPP__

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cinttypes>
#include "lib/utils/crc_fast.hpp"
#include "include/sdk/base_table_entry.hpp"
#include "lib/table/ftl/ftl_defines.hpp"
#include "lib/table/ftl/ftl_bucket.hpp"

static inline void *
get_sw_entry_pointer (base_table_entry_t *p)
{
    return (uint8_t *)p + sizeof(base_table_entry_t);
}

static inline char *
ftlu_rawstr (void *data, uint32_t len)
{
    static thread_local char str[512];
    uint32_t i = 0;
    uint32_t slen = 0;
    for (i = 0; i < len; i++) {
        slen += sprintf(str+slen, "%02x", ((uint8_t*)data)[i]);
    }
    return str;
}

#endif    // __FTL_UTILS_HPP__
