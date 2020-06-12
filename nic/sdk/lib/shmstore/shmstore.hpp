//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines a data store based on shared memory
///
//----------------------------------------------------------------------------

#ifndef __SDK_LIB_SHMSTORE_HPP__
#define __SDK_LIB_SHMSTORE_HPP__

#include <map>
#include "include/sdk/base.hpp"
#include "include/sdk/platform.hpp"
#include "lib/shmmgr/shmmgr.hpp"

namespace sdk {
namespace lib {

class shmstore {
public:
    /// \brief factory method to create a store instance
    static shmstore *factory(void);

    /// \brief destroy the upgrade shm including the backend
    /// \param[in] created store instance
    static void destroy(shmstore *);

    /// \brief create the store
    /// \param[in] name name of the store
    /// \param[in] size size of the store
    sdk_ret_t create(const char *name, size_t size);

    /// \brief open a store for read
    /// \param[in] name name of the store
    /// \param[in] mode open mode
    sdk_ret_t open(const char *name, enum shm_mode_e mode = SHM_OPEN_READ_ONLY);

    /// \brief get the size of the store
    size_t size(void);

    /// \brief create a segment in the store which has been created.
    ///        cannot create segments on opened store.
    /// \param[in] name segment name
    /// \param[in] size segment size
    void *create_segment(const char *name, size_t size);

    /// \brief open a segment in the given in store
    /// \param[in] name segment name
    void *open_segment(const char *name);

    /// \brief get segment size
    /// \param[in] name segment name
    size_t segment_size(const char *name);

private:
    /// shared memory manager
    sdk::lib::shmmgr *shmmgr_;
private:
    sdk_ret_t file_init_(const char *name, size_t size, enum shm_mode_e mode);
    void *segment_init_(const char *name, size_t size, bool create);
};

}    // namespace lib
}    // namespace sdk

#endif    // __SDK_LIB_SHMSTORE_HPP__
