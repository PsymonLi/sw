//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the declarations of global routines
///
//----------------------------------------------------------------------------

#ifndef __ATHENA_TEST_BASE_HPP__
#define __ATHENA_TEST_BASE_HPP__

#include <gtest/gtest.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/logger/logger.hpp"

// globals
static const std::string k_sim_arch = "x86";
static const std::string k_hw_arch = "arm";

// Returns target arch string
static inline std::string
aarch_get (void)
{
#ifdef __x86_64__
    return k_sim_arch;
#elif __aarch64__
    return k_hw_arch;
#else
    SDK_ASSERT(0);
    return NULL;
#endif
}

// Returns true on hw
static inline bool
hw (void)
{
    return (aarch_get() == k_hw_arch);
}

// test case parameters
typedef struct test_case_params_t_ {
    const char      *cfg_file;        ///< config file
    std::string     profile;          ///< config profile
    std::string     oper_mode;        ///< operational mode of the device
} test_case_params_t;

// base class for all gtests.
// implements init and teardown routines common to all test cases
class pds_test_base : public ::testing::Test {
protected:
    // constructor
    pds_test_base() {}

    // destructor
    virtual ~pds_test_base() {}

    // called immediately after the constructor before each test
    virtual void SetUp(void) {}

    // called immediately after each test before the destructor
    virtual void TearDown(void) {}

    // called at the beginning of all test cases in this class
    static void SetUpTestCase();

    // called at the end of all test cases in this class
    static void TearDownTestCase(void);
};

// externs
extern trace_level_e g_trace_level;  // trace level, default is DEBUG

#endif  // __ATHENA_TEST_BASE_HPP__
