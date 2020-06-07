//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements nicmgr shm manager for persistent state
///
//----------------------------------------------------------------------------

#ifndef __NICMGR_SHM_HPP__
#define __NICMGR_SHM_HPP__

#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/sdk/lib/shmmgr/shmmgr.hpp"

/// \brief nicmgr shared memory manager
/// shared memory in nicmgr is used to stored persistent states
// across reboots and upgrades
class nicmgr_shm {
public:
    /// \brief constructor
    nicmgr_shm() {
        shm_create_ = false;
        shm_mmgr_ = nullptr;
    }

    /// \brief destructor
    ~nicmgr_shm() {}

    /// \brief singleton factory method to instantiate the Nicmgr shm
    /// \param[in] shm_create create/open the shared memory instance create mode
    /// \return NULL, if failure. pointer to shm, if success
    static nicmgr_shm* factory(bool shm_create = true);

    /// \brief get the instance of nicmgr shm
    /// \return NULL if not instantiated, pointer to shm if created
    static nicmgr_shm* getInstance(void) { return instance_; };

    /// \brief allocate/find a segment in the shm memory
    /// \param[in] name name of the segment
    /// \param[in] size size of the segment
    /// \return NULL if error, pointer to segment if success
    void* alloc_find_pstate(const char *name, size_t size);

    /// \brief destroy the instance of nicmgr shm
    void destroy(nicmgr_shm *state);

private:
    /// \brief instance of singleton nicmgr shared memory manager
    static nicmgr_shm  *instance_;

    /// \brief created mode of nicmgr shared memory manager
    bool        shm_create_;

    /// \brief handle to shmmgr from the library
    shmmgr     *shm_mmgr_;

    /// \brief init method for the instance of nicmgr shm
    /// \return SDK_RET_ERR if error, SDK_RET_OK if success
    sdk_ret_t init_(bool shm_create);
};

#endif // __NICMGR_SHM_HPP__
