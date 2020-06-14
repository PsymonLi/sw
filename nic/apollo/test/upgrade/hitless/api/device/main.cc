
//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the all upgrade device test cases
///
//----------------------------------------------------------------------------

#include "nic/apollo/test/api/utils/workflow.hpp"
#include "nic/apollo/test/api/utils/device.hpp"

namespace test {
namespace api {

//----------------------------------------------------------------------------
// DEVICE upgrade test class
//----------------------------------------------------------------------------

class device_upg_test : public pds_test_base {
protected:
    device_upg_test() {}
    virtual ~device_upg_test() {}
    virtual void SetUp() {}
    virtual void TearDown() {}
    static void SetUpTestCase() {
        if (!agent_mode())
            pds_test_base::SetUpTestCase(g_tc_params);
    }
    static void TearDownTestCase() {
        if (!agent_mode())
            pds_test_base::TearDownTestCase();
    }
};

//----------------------------------------------------------------------------
// DEVICE upgrade test cases implementation
//----------------------------------------------------------------------------

/// \defgroup DEVICE_TEST DEVICE upgrade Tests
/// @{

/// \brief DEVICE WF_U_1
/// \ref WF_U_1
TEST_F(device_upg_test, device_upg_workflow_u1) {
    device_feeder feeder;

    // setup device
    feeder.init("2001:1::1", "00:02:01:00:00:01", "2001:1::2", false, false,
                0, false, PDS_DEVICE_PROFILE_DEFAULT,
                PDS_MEMORY_PROFILE_DEFAULT, PDS_DEV_OPER_MODE_HOST, 1, true);
    // backup
    workflow_u1_s1<device_feeder>(feeder);

    // restore
    workflow_u1_s2<device_feeder>(feeder);
    // config replay
    workflow_u1_s3<device_feeder>(feeder);
}

/// @}

}    // namespace api
}    // namespace test

//----------------------------------------------------------------------------
// Entry point
//----------------------------------------------------------------------------

/// @private
int
main (int argc, char **argv)
{
    return api_test_program_run(argc, argv);
}
