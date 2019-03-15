//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
// SDK types header file
//------------------------------------------------------------------------------
#ifndef __SDK_TABLE_HPP__
#define __SDK_TABLE_HPP__

#include <stdint.h>

#define SDK_TABLE_BITS_TO_BYTES(_b) (((_b) >> 3) + (((_b) & 0x7) ? 1 : 0))
#define SDK_TABLE_ALIGN_TO_64B(_s) \
        (((_s)*8) <= 512) ? (_s) : (((_s)%64) ? ((_s)+(64-((_s)%64))) : (_s))

#define SDK_TABLE_HANDLE_INVALID    0

namespace sdk {
namespace table {

typedef enum health_s {
    HEALTH_GREEN,
    HEALTH_YELLOW,
    HEALTH_RED
} health_t;

#define SDK_TABLE_HANDLE_INVALID 0

#define SDK_TABLE_MAX_SW_KEY_LEN 128
#define SDK_TABLE_MAX_SW_DATA_LEN 128
#define SDK_TABLE_MAX_HW_KEY_LEN 64
#define SDK_TABLE_MAX_HW_DATA_LEN 64

#define SIZE_BITS_TO_BYTES(_sizebits) \
        (((_sizebits) >> 3) + ((_sizebits) & 0x7) ? 1 : 0)

// Forward declaration
typedef struct sdk_table_api_params_ sdk_table_api_params_t;

typedef char* (*bytes2str_t)(void *bytes);

typedef uint64_t sdk_table_handle_t;
typedef char* (*handle2str)(sdk_table_handle_t handle);
typedef void (*iterate_t)(sdk_table_api_params_t *params);

typedef enum sdk_table_api_op_ {
    SDK_TABLE_API_NONE,
    SDK_TABLE_API_INSERT,
    SDK_TABLE_API_REMOVE,
    SDK_TABLE_API_UPDATE,
    SDK_TABLE_API_GET,
    SDK_TABLE_API_RESERVE,
    SDK_TABLE_API_RELEASE,
    SDK_TABLE_API_ITERATE,
} sdk_table_api_op_t;

#define SDK_TABLE_API_OP_IS_CRUD(_op) \
        (((_op) == SDK_TABLE_API_INSERT) || ((_op) == SDK_TABLE_API_REMOVE) || \
         ((_op) == SDK_TABLE_API_UPDATE) || ((_op) == SDK_TABLE_API_GET) || \
         ((_op) == SDK_TABLE_API_RESERVE) || ((_op) == SDK_TABLE_API_RELEASE))

typedef enum sdk_table_health_state_s {
    SDK_TABLE_HEALTH_GREEN,
    SDK_TABLE_HEALTH_YELLOW,
    SDK_TABLE_HEALTH_RED
} sdk_table_health_state_t;

// Callback on every INSERT and DELETE of a table lib.
// Callback has to be implemented to set new state based on capacity & usage.
//typedef void (*table_health_monitor_func_t)(uint32_t table_id,
//                     char *name, sdk_table_health_state_t curr_state,
//                     uint32_t capacity, uint32_t usage,
//                     sdk_table_health_state_t *new_state);

typedef struct sdk_table_factory_params_ {
    // TableID of this table given by P4
    uint32_t table_id;
    // If this table uses hints to resolve collisions,
    // specific the number of hints used by this table.
    uint32_t num_hints;
    // If collision in this table is resolved using 
    // recircs, then specify the maximum number of 
    // recircs allowed.
    uint32_t max_recircs;
    // Convert key to string
    bytes2str_t key2str;
    // Convert mask to string
    bytes2str_t mask2str;
    // Convert data to string
    bytes2str_t appdata2str;
    // Enable entry tracing
    bool entry_trace_en;
    // Health state
    //table_health_state_t health_state_; // health state
    // Health monitoring callback
    //table_health_monitor_func_t health_monitor_func;
} sdk_table_factory_params_t;

typedef struct sdk_table_api_params_ {
    // [Input] Key of the entry
    void *key;
    // [Input] Key mask of the entry
    void *mask;
    // [Input] Data of the entry
    void *appdata;
    // [Input] ActionID of the entry
    uint8_t action_id;
    // [Input/Output] (Input is Optional)
    // valid only for HASH tables
    bool hash_valid;
    // [Input/Output] (Input is Optional)
    // Upto 32bits of hash value, valid only for HASH tables
    uint32_t hash_32b;
    // [Input/Output]
    // Handle of the entry, encoding differs for each table.
    sdk_table_handle_t handle;
    // [Input]
    // Iterator callback function
    iterate_t itercb;
    // [Input]
    // Callback data for table iteration
    void *cbdata;
} sdk_table_api_params_t;

typedef struct sdk_table_api_stats_ {
    uint32_t insert;
    uint32_t insert_duplicate;
    uint32_t insert_fail;
    uint32_t remove;
    uint32_t remove_not_found;
    uint32_t remove_fail;
    uint32_t update;
    uint32_t update_fail;
    uint32_t get;
    uint32_t get_fail;
    uint32_t reserve;
    uint32_t reserve_fail;
    uint32_t release;
    uint32_t release_fail;
} sdk_table_api_stats_t;

typedef struct sdk_table_stats_ {
    uint32_t entries;
    uint32_t collisions;
} sdk_table_stats_t;

} // namespace table
} // namespace sdk
#endif // __SDK_TABLE_HPP__
