//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the all upgrade tep test cases
///
//----------------------------------------------------------------------------

#include "nic/apollo/test/api/utils/if.hpp"
#include "nic/apollo/test/api/utils/nexthop.hpp"
#include "nic/apollo/test/api/utils/nexthop_group.hpp"
#include "nic/apollo/test/api/utils/tep.hpp"
#include "nic/apollo/test/api/utils/workflow.hpp"

namespace test {
namespace api {

//----------------------------------------------------------------------------
// TEP upgrade test class
//----------------------------------------------------------------------------

class tep_upg_test : public ::pds_test_base {
protected:
    tep_upg_test() {}
    virtual ~tep_upg_test() {}
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

static inline void
tep_upg_setup (void)
{
    pds_batch_ctxt_t bctxt = batch_start();
    sample_if_setup(bctxt);
    sample_nexthop_setup(bctxt);
    sample_nexthop_group_setup(bctxt);
    batch_commit(bctxt);
}

static inline void
tep_upg_teardown (void)
{
    pds_batch_ctxt_t bctxt = batch_start();
    sample_nexthop_teardown(bctxt);
    sample_if_teardown(bctxt);
    sample_nexthop_group_teardown(bctxt);
    batch_commit(bctxt);
}

//----------------------------------------------------------------------------
// TEP upg test cases implementation
//----------------------------------------------------------------------------

/// \defgroup TEP TEP upg Tests
/// @{

/// \brief TEP WF_U_1
/// \ref WF_U_1
TEST_F(tep_upg_test, tep_upg_workflow_u1) {
    tep_feeder feeder;

    // setup precursor
    tep_upg_setup();
    // setup tep
    feeder.init(2, 0x0E0D0A0B0200, "50.50.1.1");
    feeder.set_stash(true);
    // backup
    workflow_u1_s1<tep_feeder>(feeder);

    // tearup precursor
    tep_upg_teardown();
    // restore
    workflow_u1_s2<tep_feeder>(feeder);
    // setup precursor again
    tep_upg_setup();
    // config replay
    workflow_u1_s3<tep_feeder>(feeder);
    // tearup precursor
    tep_upg_teardown();
}

/// \brief TEP WF_U_2
/// \ref WF_U_2
TEST_F(tep_upg_test, tep_upg_workflow_u2) {
    tep_feeder feeder1A, feeder1B, feeder2;
    uint32_t start_key, num_obj, obj_in_set;

    start_key = 2;
    num_obj = 10;
    obj_in_set = 5;

    // setup precursor
    tep_upg_setup();
    // setup tep
    feeder1A.init(start_key, 0x0E0D0A0B0200, "50.50.1.1", num_obj);
    feeder1A.set_stash(true);
    // backup
    workflow_u1_s1<tep_feeder>(feeder1A);

    // tearup precursor
    tep_upg_teardown();
    // restore
    workflow_u1_s2<tep_feeder>(feeder1A);

    // setup precursor again
    tep_upg_setup();
    // recalibrate feeder to replay subset of stashed objs
    feeder1A.init(start_key, 0x0E0D0A0B0200, "50.50.1.1", obj_in_set);
    // setup another feeder to create additional tep
    feeder2.init(start_key + num_obj, 0x0E0D0A0B0200, "50.50.1.1", num_obj);
    // position feeder to point to next key to be replayed
    feeder1B.init(start_key + obj_in_set, 0x0E0D0A0B0200, "50.50.1.1",
                  num_obj - obj_in_set);
    workflow_u2<tep_feeder>(feeder1A, feeder2, feeder1B);

    // tearup precursor
    tep_upg_teardown();
}

/// \brief TEP WF_U_1_N_1
/// \ref WF_U_1_N_1
TEST_F(tep_upg_test, tep_upg_workflow_u1_neg_1) {
    tep_feeder feeder1, feeder2;
    uint32_t start_key = 1;
    uint32_t   num_obj = 10;

    // setup precursor
    tep_upg_setup();
    // setup tep
    feeder1.init(start_key, 0x0E0D0A0B0200, "50.50.1.1", num_obj);
    feeder1.set_stash(true);
    // backup
    workflow_u1_s1<tep_feeder>(feeder1);

    // tearup precursor
    tep_upg_teardown();
    // restore
    workflow_u1_s2<tep_feeder>(feeder1);

    // setup precursor again
    tep_upg_setup();
    // setup feeder again to replay original set of objs
    feeder1.init(start_key, 0x0E0D0A0B0200, "50.50.1.1", num_obj);
    // setup another tep to delete non existing obj
    feeder2.init(start_key + num_obj, 0x0E0D0A0B0200, "50.50.1.1", 1);
    workflow_u1_neg_1<tep_feeder>(feeder1, feeder2);

    // tearup precursor
    tep_upg_teardown();
}

/// \brief TEP WF_U_3
/// \ref WF_U_3
TEST_F(tep_upg_test, tep_upg_workflow_u3) {
    tep_feeder feeder1, feeder2;
    uint32_t start_key, num_obj, obj_in_set;

    start_key = 2;
    num_obj = 10;
    obj_in_set = 5;

    // setup precursor
    tep_upg_setup();
    // setup tep
    feeder1.init(start_key, 0x0E0D0A0B0200, "50.50.1.1", num_obj);
    feeder1.set_stash(true);
    // backup
    workflow_u1_s1<tep_feeder>(feeder1);

    // tearup precursor
    tep_upg_teardown();
    // restore
    workflow_u1_s2<tep_feeder>(feeder1);

    // setup precursor again
    tep_upg_setup();
    // setup one feeder to delete few stashed nh groups
    // note: change of "obj_in_set" needs corresponding change in workflow
    feeder2.init(start_key, 0x0E0D0A0B0200, "50.50.1.1", obj_in_set);
    // recalibrate feeder to replay rest of stashed objs
    feeder1.init(start_key + obj_in_set, 0x0E0D0A0B0200, "50.50.1.1",
                 obj_in_set);
    workflow_u3<tep_feeder>(feeder1, feeder2);

    // tearup precursor
    tep_upg_teardown();
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
