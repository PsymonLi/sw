// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __METRICS_HPP__
#define __METRICS_HPP__

#include <stdint.h>
#include <vector>

#include "include/sdk/types.hpp"
#include "lib/metrics/shm.hpp"
#include "lib/metrics/kvstore.hpp"
#include "lib/metrics/htable.hpp"

namespace sdk {
namespace metrics {

typedef unsigned __int128 key_t;

typedef enum metrics_type_ {
    SW  = 1, // Metrics are located in the process' memory
    HBM = 2, // Metrics are located in HW/HBM
} metrics_type_t;

typedef struct schema_ {
    std::string name;     // e.g. Port
    metrics_type_t type;
    std::string filename; // json schema file
} schema_t;

typedef std::pair<std::string, uint64_t> metrics_counter_pair_t;
typedef std::vector<metrics_counter_pair_t> counters_t;

// register a metrics schema with metrics module
// this API can also be used to register counter blocks i.e., variable
// number of counters per key and all counters are of same type (i.e,
// there is no need to enumerate each counter)
//
// returns a handle. NULL in case of failure
extern void *create(schema_t *schema, bool counter_block = false);

// register memory address of a metrics table for a given key
// using the handle created before. when counter_block_size is
// non-zero, it indicates the number of valid counters in the
// counter block (for exporting only relevant counters)
// NOTE: this API can be used with same handle & key multiple times
//       as the address can change during the lifetime of an associated
//       object
extern void row_address(void *handle, key_t key, void *address,
                        uint32_t counter_block_size = 0);

extern void metrics_update(void *handle, key_t key, uint64_t values[]);

// For reader
extern void *metrics_open(const char *name);
extern counters_t metrics_read(void *handle, key_t key);

}    // namespace metrics
}    // namespace sdk

#endif    // __METRICS_HPP__
