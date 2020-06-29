//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the helper functions for backup/restore
///
//----------------------------------------------------------------------------

#ifndef __TEST_HITLESS_HELPER_HPP__
#define __TEST_HITLESS_HELPER_HPP__

namespace test {
namespace api {

sdk_ret_t upg_obj_backup(sysinit_mode_t mode);
sdk_ret_t upg_obj_restore(sysinit_mode_t mode);

}   // namespace api
}   // namespace test

#endif    // __TEST_HITLESS_HELPER_HPP__
