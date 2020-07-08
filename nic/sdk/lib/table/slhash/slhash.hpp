//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// stateless tcam table management library
//------------------------------------------------------------------------------
#ifndef __SDK_SLHASH_HPP__
#define __SDK_SLHASH_HPP__

#include "include/sdk/table.hpp"
#include "include/sdk/mem.hpp"
#include "include/sdk/base.hpp"
#include "lib/table/sltcam/sltcam.hpp"

#include "slhash_properties.hpp"
#include "slhash_api_context.hpp"
#include "slhash_stats.hpp"
#include "slhash_table.hpp"
#include "slhash_txn.hpp"

using sdk::table::slhash_internal::slhctx;
using sdk::table::sdk_table_api_params_t;

namespace sdk {
namespace table {

class slhash {
public:
    static slhash* factory(sdk_table_factory_params_t *params);
    static void destroy(slhash *slhash);

    // all public APIs
    sdk_ret_t insert(sdk_table_api_params_t *params);
    sdk_ret_t remove(sdk_table_api_params_t *params);
    sdk_ret_t get(sdk_table_api_params_t *params);
    sdk_ret_t update(sdk_table_api_params_t *params);
    sdk_ret_t iterate(sdk_table_api_params_t *params);
    sdk_ret_t reserve(sdk_table_api_params_t *params);
    sdk_ret_t release(sdk_table_api_params_t *params);
    sdk_ret_t stats_get(sdk_table_api_stats_t *api_stats,
                        sdk_table_stats_t *table_stats);
    sdk_ret_t txn_start(void);
    sdk_ret_t txn_end(void);
    
    // DEV USAGE ONLY:
    // api to check sanity of the internal state
    sdk_ret_t sanitize(void);

private:
    slhash(void) {
        SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_PRIVATE);
    }
    ~slhash(void) {
        SDK_SPINLOCK_DESTROY(&slock_);
    }
    sdk_ret_t init_(sdk_table_factory_params_t *params);
    sdk_ret_t find_(void);
    sdk_ret_t insert_(void);
    sdk_ret_t remove_(void);
    sdk_ret_t update_(void);
    sdk_ret_t reserve_(void);
    sdk_ret_t release_(void);

private:
    sdk::table::slhash_internal::properties props_;
    sdk::table::slhash_internal::stats stats_;
    sdk::table::slhash_internal::txn txn_;
    sdk_spinlock_t slock_;
    slhctx ctx_;
    
    // hash table
    sdk::table::slhash_internal::table table_;
    // overflow TCAM
    sdk::table::sltcam *tcam_;
};

}    // namespace table
}    // namespace sdk

#endif    // __SDK_TCAM_HPP__
