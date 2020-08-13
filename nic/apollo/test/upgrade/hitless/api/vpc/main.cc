
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

/// \brief VPC WF_U_2
/// \ref WF_U_2
TEST_F(vpc_upg_test, vpc_upg_workflow_u2) {
    vpc_feeder feeder1A, feeder1B, feeder2;
    uint32_t start_key, num_obj, obj_in_set;

    start_key = 1;
    num_obj = 10;
    obj_in_set = 5;

    // setup vpc
    feeder1A.init(int2pdsobjkey(start_key), PDS_VPC_TYPE_UNDERLAY, "10.0.0.0/16",
                  num_obj);
    feeder1A.set_stash(true);
    // backup
    workflow_u1_s1<vpc_feeder>(feeder1A);

    // restore
    workflow_u1_s2<vpc_feeder>(feeder1A);

    // recalibrate feeder to replay subset of stashed objs
    feeder1A.init(int2pdsobjkey(start_key), PDS_VPC_TYPE_UNDERLAY, "10.0.0.0/16",
                  obj_in_set);
    // setup another feeder to create additional vpcs
    feeder2.init(int2pdsobjkey(start_key + num_obj), PDS_VPC_TYPE_UNDERLAY,
                 "10.0.0.0/16", num_obj);
    // position another feeder to point to next key to be replayed
    feeder1B.init(int2pdsobjkey(start_key + obj_in_set), PDS_VPC_TYPE_UNDERLAY,
                  "10.0.0.0/16", num_obj - obj_in_set);
    workflow_u2<vpc_feeder>(feeder1A, feeder2, feeder1B);
}

/// \brief VPC WF_U_1_N_1
/// \ref WF_U_1_N_1
TEST_F(vpc_upg_test, vpc_upg_workflow_u1_neg_1) {
    vpc_feeder feeder1, feeder2;
    uint32_t start_key = 1;
    uint32_t   num_obj = 10;

    // setup vpc
    feeder1.init(int2pdsobjkey(start_key), PDS_VPC_TYPE_UNDERLAY, "10.0.0.0/16",
                num_obj);
    feeder1.set_stash(true);
    // backup
    workflow_u1_s1<vpc_feeder>(feeder1);

    // restore
    workflow_u1_s2<vpc_feeder>(feeder1);
    // setup feeder again to configure original set of objs
    feeder1.init(int2pdsobjkey(start_key), PDS_VPC_TYPE_UNDERLAY, "10.0.0.0/16",
                 num_obj);
    // setup another feeder to delete one obj
    feeder2.init(int2pdsobjkey(start_key + num_obj), PDS_VPC_TYPE_UNDERLAY,
                 "10.0.0.0/16", 1);
    workflow_u1_neg_1<vpc_feeder>(feeder1, feeder2);
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
