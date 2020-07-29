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

// segment walk callback function
typedef shmmgr_seg_walk_cb_t shmstore_seg_walk_cb_t;

class shmstore {
public:
    /// \brief factory method to create a store instance
    /// \return store instance
    static shmstore *factory(void);

    /// \brief destroy the upgrade shm including the backend
    /// \param[in] created store instance
    static void destroy(shmstore *);

    /// \brief create the store
    /// \param[in] name name of the store
    /// \param[in] size size of the store
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t create(const char *name, size_t size);

    /// \brief open a store for read
    /// \param[in] name name of the store
    /// \param[in] mode open mode
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t open(const char *name, enum shm_mode_e mode = SHM_OPEN_READ_ONLY);

    /// \brief get the mode of the store
    /// \return shm mode type
    shm_mode_e mode(void) { return mode_; }

    /// \brief get the size of the store
    /// \return store size
    size_t size(void);

    /// \brief create a segment in the store which has been created.
    ///        cannot create segments on opened store.
    /// \param[in] name segment name
    /// \param[in] size segment size
    /// \param[in] segment label, identifies the data content
    /// \param[in] alignment segment alignment
    /// \return valid segment pointer, null on failure
    void *create_segment(const char *name, size_t size, uint16_t label = 0,
                         size_t alignment = 0);

    /// \brief open a segment in the given in store
    /// \param[in] name segment name
    /// \return valid segment pointer, null on failure
    void *open_segment(const char *name);

    /// \brief create or open a segment in the store which has been created.
    ///        cannot create segments on opened store.
    /// \param[in] name segment name
    /// \param[in] size segment size, valid for create case only
    /// \param[in] segment label, identifies the data content
    /// \param[in] alignment segment alignment, valid for create case only
    /// \return valid segment pointer, null on failure
    void *create_or_open_segment(const char *name, size_t size, uint16_t label = 0,
                                 size_t alignment = 0);

    /// \brief get segment size
    /// \param[in] name segment name
    /// \return size of segment
    size_t segment_size(const char *name);

    /// \brief walk through all named segments in the store
    /// \param[in] callback context
    /// \param[in] callback function
    /// \return None
    void segment_walk(void *ctxt, shmstore_seg_walk_cb_t cb);

private:
    /// store mode
    sdk::lib::shm_mode_e mode_;
    /// shared memory manager
    sdk::lib::shmmgr *shmmgr_;
private:
    sdk_ret_t file_init_(const char *name, size_t size, enum shm_mode_e mode);
    void *segment_init_(const char *name, size_t size, bool create,
                        uint16_t label, size_t alignment = 0);
};

}    // namespace lib
}    // namespace sdk

#endif    // __SDK_LIB_SHMSTORE_HPP__
