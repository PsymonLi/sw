//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the all nexthop group hitless upgrade test cases
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
// NH Group upgrade test class
//----------------------------------------------------------------------------

class nh_group_upg_test : public ::pds_test_base {
protected:
    nh_group_upg_test() {}
    virtual ~nh_group_upg_test() {}
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
nh_group_upg_setup (void)
{
    pds_batch_ctxt_t bctxt = batch_start();
    sample_if_setup(bctxt);
    sample_tep_setup(bctxt);
    sample_underlay_nexthop_setup(bctxt);
    batch_commit(bctxt);
}

static inline void
nh_group_upg_teardown (void)
{
    pds_batch_ctxt_t bctxt = batch_start();
    sample_underlay_nexthop_teardown(bctxt);
    sample_tep_teardown(bctxt);
    sample_if_teardown(bctxt);
    batch_commit(bctxt);
}

//----------------------------------------------------------------------------
// NH group upg test cases implementation
//----------------------------------------------------------------------------

/// \defgroup NH_GRP Nexthop group upg tests
/// @{

/// \brief Nexthop group WF_U_1
/// \ref WF_U_1
TEST_F(nh_group_upg_test, nh_group_upg_workflow_u1) {
    nexthop_group_feeder feeder1;

    pds_batch_ctxt_t bctxt = batch_start();
    // setup precursor
    nh_group_upg_setup();
    // setup nh group
    feeder1.init(PDS_NHGROUP_TYPE_OVERLAY_ECMP, PDS_MAX_OVERLAY_ECMP_TEP,
                 int2pdsobjkey(1), 10, 1);
    // backup
    workflow_u1_s1<nexthop_group_feeder>(feeder1);

    // tearup precursor
    nh_group_upg_teardown();
    // restore
    workflow_u1_s2<nexthop_group_feeder>(feeder1);

    // setup precursor again
    nh_group_upg_setup();
    // config replay
    workflow_u1_s3<nexthop_group_feeder>(feeder1);
    // tearup precursor
    nh_group_upg_teardown();
}

/// \brief Nexthop group WF_U_2
/// \ref WF_U_2
TEST_F(nh_group_upg_test, nh_group_upg_workflow_u2) {
    nexthop_group_feeder feeder1A, feeder1B, feeder2;
    uint32_t start_key, num_obj, obj_in_set;

    start_key = 1;
    num_obj = 10;
    obj_in_set = 5;

    // setup precursor
    nh_group_upg_setup();
    // setup nh group
    feeder1A.init(PDS_NHGROUP_TYPE_OVERLAY_ECMP, PDS_MAX_OVERLAY_ECMP_TEP,
                  int2pdsobjkey(start_key), num_obj, 1);
    // backup
    workflow_u1_s1<nexthop_group_feeder>(feeder1A);

    // tearup precursor
    nh_group_upg_teardown();
    // restore
    workflow_u1_s2<nexthop_group_feeder>(feeder1A);

    // setup precursor again
    nh_group_upg_setup();
    // recalibrate feeder to replay subset of stashed objs
    feeder1A.init(PDS_NHGROUP_TYPE_OVERLAY_ECMP, PDS_MAX_OVERLAY_ECMP_TEP,
                  int2pdsobjkey(start_key), obj_in_set);
    // setup another feeder to create additional nh groups
    feeder2.init(PDS_NHGROUP_TYPE_OVERLAY_ECMP, PDS_MAX_OVERLAY_ECMP_TEP,
                 int2pdsobjkey(start_key + num_obj), num_obj);
    // position another feeder to point to next key to be replayed
    feeder1B.init(PDS_NHGROUP_TYPE_OVERLAY_ECMP, PDS_MAX_OVERLAY_ECMP_TEP,
                  int2pdsobjkey(start_key + obj_in_set), num_obj - obj_in_set);
    workflow_u2<nexthop_group_feeder>(feeder1A, feeder2, feeder1B);

    // tearup precursor
    nh_group_upg_teardown();
}

/// \brief Nexthop group WF_U_1_N_1
/// \ref WF_U_1_N_1
TEST_F(nh_group_upg_test, nh_group_upg_workflow_u1_neg_1) {
    nexthop_group_feeder feeder1, feeder2;
    uint32_t start_key = 1;
    uint32_t   num_obj = 10;

    // setup precursor
    nh_group_upg_setup();
    // setup nh group
    feeder1.init(PDS_NHGROUP_TYPE_OVERLAY_ECMP, PDS_MAX_OVERLAY_ECMP_TEP,
                int2pdsobjkey(start_key), num_obj, 1);
    // backup
    workflow_u1_s1<nexthop_group_feeder>(feeder1);

    // tearup precursor
    nh_group_upg_teardown();
    // restore
    workflow_u1_s2<nexthop_group_feeder>(feeder1);

    // setup precursor again
    nh_group_upg_setup();
    // recalibrate feeder to configure max+1 objs
    feeder1.init(PDS_NHGROUP_TYPE_OVERLAY_ECMP, PDS_MAX_OVERLAY_ECMP_TEP,
                 int2pdsobjkey(start_key), num_obj);
    // setup another feeder to delete one obj
    feeder2.init(PDS_NHGROUP_TYPE_OVERLAY_ECMP, PDS_MAX_OVERLAY_ECMP_TEP,
                 int2pdsobjkey(start_key + num_obj), 1);
    workflow_u1_neg_1<nexthop_group_feeder>(feeder1, feeder2);

    // tearup precursor
    nh_group_upg_teardown();
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
    api_test_program_run(argc, argv);
}
