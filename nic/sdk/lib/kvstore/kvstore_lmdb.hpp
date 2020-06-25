//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// lmdb based implementation of key-value store APIs
///
//----------------------------------------------------------------------------

#ifndef __KVSTORE_LMDB_HPP__
#define __KVSTORE_LMDB_HPP__

#include <sys/stat.h>
#include <lmdb.h>
#include "lib/kvstore/kvstore.hpp"

namespace sdk {
namespace lib {

typedef char kvstore_key;

class kvstore_lmdb : public kvstore {
public:
    static kvstore *factory(std::string dbpath, size_t size,
                            kvstore_mode_t mode);
    static void destroy(kvstore *kvs);
    virtual sdk_ret_t txn_start(txn_type_t txn_type) override;
    virtual sdk_ret_t txn_commit(void) override;
    virtual sdk_ret_t txn_abort(void) override;
    virtual sdk_ret_t find(_In_ const void *key, _In_ size_t key_sz,
                           _Out_ void *data, _Inout_ size_t *data_sz,
                           std::string key_prefix) override;
    virtual sdk_ret_t insert(const void *key, size_t key_sz,
                             const void *data, size_t data_sz,
                             std::string key_prefix) override;
    virtual sdk_ret_t remove(const void *key, size_t key_sz,
                             std::string key_prefix) override;
    virtual sdk_ret_t iterate(kvstore_iterate_cb_t cb, void *ctxt,
                              std::string key_prefix) override;

private:
    kvstore_lmdb() {
        env_ = NULL;
        t_txn_hdl_ = NULL;
    }
    ~kvstore_lmdb() {}
    sdk_ret_t init(std::string dbpath, size_t size, kvstore_mode_t mode);
    static void *key_prefix_match_(void *key, size_t key_sz,
                                   std::string key_prefix);

private:
    MDB_env *env_;
    static thread_local MDB_txn *t_txn_hdl_;
    static thread_local MDB_dbi db_dbi_;
};

}    // namespace lib
}    // namespace sdk

#endif    // __KVSTORE_LMDB_HPP__
