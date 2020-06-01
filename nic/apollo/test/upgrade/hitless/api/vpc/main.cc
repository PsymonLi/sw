
//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the all upgrade vpc test cases
///
//----------------------------------------------------------------------------

#include "nic/apollo/test/api/utils/workflow.hpp"
#include "nic/apollo/test/api/utils/utils.hpp"
#include "nic/apollo/test/api/utils/vpc.hpp"

namespace test {
namespace api {

//----------------------------------------------------------------------------
// VPC upgrade test class
//----------------------------------------------------------------------------

class vpc_upg_test : public pds_test_base {
protected:
    vpc_upg_test() {}
    virtual ~vpc_upg_test() {}
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
// VPC upgrade test cases implementation
//----------------------------------------------------------------------------

/// \defgroup VPC_TEST VPC upgrade Tests
/// @{

/// \brief VPC WF_U_1
/// \ref WF_U_1
TEST_F(vpc_upg_test, vpc_upg_workflow_u1) {
    vpc_feeder feeder;

    // setup vpc
    feeder.init(int2pdsobjkey(1), PDS_VPC_TYPE_UNDERLAY, "10.0.0.0/16",
                PDS_MAX_VPC + 1);
    feeder.set_stash(true);
    // backup
    workflow_u1_s1<vpc_feeder>(feeder);

    // restore
    workflow_u1_s2<vpc_feeder>(feeder);
    // config replay
    workflow_u1_s3<vpc_feeder>(feeder);
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
