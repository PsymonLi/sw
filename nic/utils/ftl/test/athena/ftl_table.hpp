//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ftl table definition
///
//----------------------------------------------------------------------------

#include <cstring>
#include "gen/p4gen/p4/include/ftl.h"
#include "lib/table/ftl/ftl_base.hpp"

#ifndef __FTL_ATHENA_TABLE_HPP__
#define __FTL_ATHENA_TABLE_HPP__

namespace sdk {
namespace table {

class flow_hash_shm: public ftl_base {
public:
    static ftl_base *factory(sdk_table_factory_params_t *params);
    static void destroy(ftl_base *f);

    flow_hash_shm() {}
    ~flow_hash_shm(){}

    virtual base_table_entry_t *get_entry(uint16_t thread_id, int index) override;

protected:
    virtual sdk_ret_t genhash_(sdk_table_api_params_t *params) override;
    virtual void *ftl_calloc(uint32_t mem_id, size_t size);
    virtual void ftl_free(uint32_t mem_id, void *ptr);
    virtual bool restore_state(void);

private:
    sdk_ret_t init_(sdk_table_factory_params_t *params);
    static flow_hash_entry_t entry_[FTL_MAX_THREADS][FTL_MAX_RECIRCS];
};

}   // namespace table
}   // namespace sdk

using sdk::table::flow_hash_shm;

#endif   // __FTL_ATHENA_TABLE_HPP__
