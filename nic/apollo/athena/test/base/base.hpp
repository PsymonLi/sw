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
