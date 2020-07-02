//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_FTL_ERROR_H__
#define __VPP_IMPL_FTL_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

enum ftl_sdk_reason_e {
    FTL_OK = 0,
    FTL_OOM,
    FTL_INVALID_ARG,
    FTL_INVALID_OP,
    FTL_ENTRY_NOT_FOUND,
    FTL_ENTRY_EXISTS,
    FTL_NO_RESOURCE,
    FTL_TABLE_FULL,
    FTL_OOB,
    FTL_HW_PROGRAM_ERR,
    FTL_RETRY,
    FTL_NOOP,
    FTL_DUPLICATE_FREE,
    FTL_COLLISION,
    FTL_MAX_RECIRC_EXCEED,
    FTL_HW_READ_ERR,
    FTL_TXN_NOT_FOUND,
    FTL_TXN_EXISTS,
    FTL_TXN_INCOMPLETE,
    FTL_COMM_FAIL,
    FTL_HW_SW_OO_SYNC,
    FTL_OBJ_CLONE_ERR,
    FTL_IN_PROGRESS,
    FTL_UPG_CRITICAL,
    FTL_MAPPING_CONFLICT,
    FTL_TIMEOUT,
    FTL_MAX,
};


#ifdef __cplusplus
}
#endif

#endif  // __VPP_IMPL_FTL_ERROR_H__
