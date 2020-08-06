//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains all policy rule test cases
///
//----------------------------------------------------------------------------

#include "nic/apollo/test/api/utils/policy.hpp"
#include "nic/apollo/test/api/utils/vpc.hpp"
#include "nic/apollo/test/api/utils/workflow.hpp"

namespace test {
namespace api {

// globals
static constexpr uint16_t g_num_stateful_rules = 64;
static const std::string k_base_v4_pfx = "30.0.0.1/16";
static const std::string k_base_v4_pfx_2 = "30.50.0.1/16";
static const std::string k_base_v4_pfx_3 = "30.100.0.1/16";
static const std::string k_base_v4_pfx_4 = "30.150.0.1/16";

//----------------------------------------------------------------------------
// Policy test class
//----------------------------------------------------------------------------

class policy_rule_test : public ::pds_test_base {
protected:
    policy_rule_test() {}
    virtual ~policy_rule_test() {}
    virtual void SetUp() {}
    virtual void TearDown() {}
    static void SetUpTestCase() {
        if (!agent_mode()) {
            pds_test_base::SetUpTestCase(g_tc_params);
        }
        g_trace_level = sdk::types::trace_verbose;
        pds_batch_ctxt_t bctxt = batch_start();
        sample_vpc_setup(bctxt, PDS_VPC_TYPE_TENANT);
        batch_commit(bctxt);
    }
    static void TearDownTestCase() {
        g_trace_level = sdk::types::trace_debug;
        pds_batch_ctxt_t bctxt = batch_start();
        sample_vpc_teardown(bctxt, PDS_VPC_TYPE_TENANT);
        batch_commit(bctxt);
        if (!agent_mode()) {
            pds_test_base::TearDownTestCase();
        }
    }
};

static uint32_t k_num_init_rules = 10;
static uint32_t k_num_rule_add = 1;
static uint32_t k_num_rule_del = 1;

//----------------------------------------------------------------------------
// Policy rule test cases implementation
//----------------------------------------------------------------------------
static void
policy_setup (pds_batch_ctxt_t bctxt) {
    policy_feeder pol_feeder;
    pds_obj_key_t pol_key = int2pdsobjkey(TEST_POLICY_ID_BASE + 1);

    // setup and teardown parameters should be in sync
    pol_feeder.init(pol_key, 512, IP_AF_IPV4, "10.0.0.1/16",
                    1, k_num_init_rules);
    many_create(bctxt, pol_feeder);
}

static void
policy_teardown (pds_batch_ctxt_t bctxt) {
    policy_feeder pol_feeder;
    pds_obj_key_t pol_key = int2pdsobjkey(TEST_POLICY_ID_BASE + 1);

    // this feeder base values doesn't matter in case of deletes
    pol_feeder.init(pol_key, 512, IP_AF_IPV4, "10.0.0.1/16",
                    1, k_num_init_rules);
    many_delete(bctxt, pol_feeder);
}

/// \defgroup POLICY_RULE_TESTS Policy rule tests
/// \brief Policy rule WF_B1
/// \ref WF_B1
TEST_F(policy_rule_test, policy_rule_workflow_b1) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    // setup a policy
    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    // create multiple policy_rules
    feeder.init(100, policy_id,  k_base_v4_pfx, 50);
    workflow_b1<policy_rule_feeder>(feeder);
    // tear down the policy
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_b2
///// \ref WF_b2
TEST_F(policy_rule_test, policy_rule_workflow_b2) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder_1;
    policy_rule_feeder feeder_2;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);
    // create multiple rules and update them
    feeder_1.init(100, policy_id, k_base_v4_pfx, 50);
    feeder_2.init(100, policy_id, k_base_v4_pfx_2, 50);
    workflow_b2<policy_rule_feeder>(feeder_1, feeder_2);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_1
///// \ref WF_1
TEST_F(policy_rule_test, policy_rule_workflow_1) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder.init(100, policy_id,  k_base_v4_pfx, 50);
    workflow_1<policy_rule_feeder>(feeder);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_2
/// \ref WF_2
TEST_F(policy_rule_test, policy_rule_workflow_2) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder.init(100, policy_id,  k_base_v4_pfx, 50);
    workflow_2<policy_rule_feeder>(feeder);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_3
/// \ref WF_3
TEST_F(policy_rule_test, policy_rule_workflow_3) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder2, feeder3;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder2.init(200, policy_id, k_base_v4_pfx_2, 100);
    feeder3.init(300, policy_id, k_base_v4_pfx_3, 100);
    workflow_3<policy_rule_feeder>(feeder1, feeder2, feeder3);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_4
/// \ref WF_4
TEST_F(policy_rule_test, policy_rule_workflow_4) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder.init(100, policy_id, k_base_v4_pfx, 50);
    workflow_4<policy_rule_feeder>(feeder);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_5
/// \ref WF_5
TEST_F(policy_rule_test, policy_rule_workflow_5) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder2, feeder3;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder2.init(200, policy_id, k_base_v4_pfx_2, 100);
    feeder3.init(300, policy_id, k_base_v4_pfx_3, 100);
    workflow_5<policy_rule_feeder>(feeder1, feeder2, feeder3);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_6
/// \ref WF_6
TEST_F(policy_rule_test, policy_rule_workflow_6) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder1A, feeder1B;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder1A.init(100, policy_id, k_base_v4_pfx_2, 100);
    feeder1B.init(100, policy_id, k_base_v4_pfx_3, 100);
    workflow_6<policy_rule_feeder>(feeder1, feeder1A, feeder1B);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_7
/// \ref WF_7
TEST_F(policy_rule_test, policy_rule_workflow_7) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder1A, feeder1B;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder1A.init(100, policy_id, k_base_v4_pfx_2, 100);
    feeder1B.init(100, policy_id, k_base_v4_pfx_3, 100);
    workflow_7<policy_rule_feeder>(feeder1, feeder1A, feeder1B);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_8
/// \ref WF_8
TEST_F(policy_rule_test, policy_rule_workflow_8) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder1A, feeder1B;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder1A.init(100, policy_id, k_base_v4_pfx_2, 100);
    feeder1B.init(100, policy_id, k_base_v4_pfx_3, 100);
    workflow_8<policy_rule_feeder>(feeder1, feeder1A, feeder1B);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_9
/// \ref WF_9
TEST_F(policy_rule_test, policy_rule_workflow_9) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder1A;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder1A.init(100, policy_id, k_base_v4_pfx_2, 100);
    workflow_9<policy_rule_feeder>(feeder1, feeder1A);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy_rule WF_10
///// \ref WF_10
TEST_F(policy_rule_test, policy_rule_workflow_10) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder2, feeder2A, feeder3, feeder3A, feeder4;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder2.init(200, policy_id, k_base_v4_pfx_2, 100);
    feeder2A.init(200, policy_id, k_base_v4_pfx, 100);
    feeder3.init(300, policy_id, k_base_v4_pfx_3, 100);
    feeder3A.init(300, policy_id, k_base_v4_pfx_4, 100);
    feeder4.init(400, policy_id , k_base_v4_pfx, 100);
    workflow_10<policy_rule_feeder>(
        feeder1, feeder2, feeder2A, feeder3, feeder3A, feeder4);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

// @sarat/param commenting neg-test cases as they are pointing to vpc_impl
// release_class_id core
#if 0
/// \brief policy-rule WF_N_1
/// \ref WF_N_1
/// Currently this case is failing because even though we create
/// a policy-rule which is already present int the policy instead
/// of rejecting the create we are updating the rule in the policy
TEST_F(policy_rule_test, DISABLED_policy_rule_workflow_neg_1) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder.init(200, policy_id, k_base_v4_pfx_2, 50);
    workflow_neg_1<policy_rule_feeder>(feeder);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy-rule WF_N_2
/// \ref WF_N_2
TEST_F(policy_rule_test, DISABLED_policy_rule_workflow_neg_2) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder.init(200, policy_id, k_base_v4_pfx_2,
                PDS_MAX_RULES_PER_IPV4_SECURITY_POLICY+2);
    workflow_neg_2<policy_rule_feeder>(feeder);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy-rule WF_N_3
/// \ref WF_N_3
TEST_F(policy_rule_test, DISABLED_policy_rule_workflow_neg_3) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder.init(100, policy_id, k_base_v4_pfx_2, 50);
    workflow_neg_3<policy_rule_feeder>(feeder);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}
/// \brief policy-rule WF_N_4
/// \ref WF_N_4
TEST_F(policy_rule_test, DISABLED_policy_rule_workflow_neg_4) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder2;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder2.init(200, policy_id, k_base_v4_pfx_2, 100);
    workflow_neg_4<policy_rule_feeder>(feeder1, feeder2);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy-rule WF_N_5
/// \ref WF_N_5
TEST_F(policy_rule_test, policy_rule_workflow_neg_5) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder1A;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 100);
    feeder1A.init(100, policy_id, k_base_v4_pfx_2, 100);
    workflow_neg_5<policy_rule_feeder>(feeder1, feeder1A);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy-rule WF_N_6
/// \ref WF_N_6
TEST_F(policy_rule_test, policy_rule_workflow_neg_6) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder1A;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 1);
    feeder1A.init(100, policy_id, k_base_v4_pfx_2, 10);
    workflow_neg_6<policy_rule_feeder>(feeder1, feeder1A);

    feeder1.init(200, policy_id, k_base_v4_pfx, 100);
    feeder1A.init(200, policy_id, k_base_v4_pfx_2, 200);
    workflow_neg_6<policy_rule_feeder>(feeder1, feeder1A);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy-rule WF_N_7
/// \ref WF_N_7
TEST_F(policy_rule_test, policy_rule_workflow_neg_7) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1, feeder1A, feeder2;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(200, policy_id, k_base_v4_pfx, 100);
    feeder1A.init(200, policy_id, k_base_v4_pfx_2, 100);
    feeder2.init(300, policy_id, k_base_v4_pfx_3, 200);
    workflow_neg_7<policy_rule_feeder>(feeder1, feeder1A, feeder2);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}

/// \brief policy-rule WF_N_8
/// \ref WF_N_8
TEST_F(policy_rule_test, policy_rule_workflow_neg_8) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder1,feeder2;
    uint32_t policy_id = TEST_POLICY_ID_BASE + 1;

    bctxt = batch_start();
    policy_setup(bctxt);
    batch_commit(bctxt);

    feeder1.init(100, policy_id, k_base_v4_pfx, 1);
    feeder2.init(200, policy_id, k_base_v4_pfx_2, 10);
    workflow_neg_8<policy_rule_feeder>(feeder1, feeder2);

    feeder1.init(200, policy_id, k_base_v4_pfx, 100);
    feeder2.init(300, policy_id, k_base_v4_pfx_2, 200);
    workflow_neg_8<policy_rule_feeder>(feeder1, feeder2);
    bctxt = batch_start();
    policy_teardown(bctxt);
    batch_commit(bctxt);
}
#endif
//---------------------------------------------------------------------
//// Non templatized test cases
////---------------------------------------------------------------------
/// \brief change policy  and policy-rule in the same batch
TEST_F(policy_rule_test, DISABLED_policy_rule_update_rule_policy) {
    pds_batch_ctxt_t bctxt;
    policy_rule_feeder feeder;
    policy_feeder pol_feeder;
    sdk_ret_t ret;
    uint32_t policy_id;
    pds_obj_key_t pol_key;

    policy_id = TEST_POLICY_ID_BASE + 1;
    pol_key = int2pdsobjkey(policy_id);

    // create policy and add policy_rules in same batch
    pol_feeder.init(pol_key, 512, IP_AF_IPV4, k_base_v4_pfx,
                    1, 100);
    feeder.init(101, policy_id, "30.0.0.140/16", 100);
    bctxt = batch_start();
    ret = many_create<policy_feeder>(bctxt, pol_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    ret = many_create<policy_rule_feeder>(bctxt, feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);
    // verify
    ret = many_read<policy_rule_feeder>(feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    ret = many_read<policy_feeder>(pol_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    // tear-down policy
    bctxt = batch_start();
    ret = many_delete(bctxt, pol_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);

    // create policy and delete policy_rules in same batch
    pol_feeder.init(pol_key, 512, IP_AF_IPV4, k_base_v4_pfx,
                    1, 100);
    feeder.init(20, policy_id, "30.0.0.140/16", 80);
    bctxt = batch_start();
    ret = many_create<policy_feeder>(bctxt, pol_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    ret = many_delete<policy_rule_feeder>(bctxt, feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);
    // verify
    ret = many_read<policy_rule_feeder>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);
    ASSERT_TRUE(ret == SDK_RET_OK);
    pol_feeder.init(pol_key, 512, IP_AF_IPV4, k_base_v4_pfx,
                    1, 20);
    ret = many_read<policy_feeder>(pol_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    // tear-down policy
    bctxt = batch_start();
    ret = many_delete(bctxt, pol_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);

    // create policy and update policy_rules in same batch
    pol_feeder.init(pol_key, 512, IP_AF_IPV4, k_base_v4_pfx,
                    1, 100);
    feeder.init(20, policy_id, "30.0.0.140/16", 80);
    bctxt = batch_start();
    ret = many_create<policy_feeder>(bctxt, pol_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    ret = many_update<policy_rule_feeder>(bctxt, feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);
    // verify
    ret = many_read<policy_rule_feeder>(feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    // tear-down policy
    bctxt = batch_start();
    ret = many_delete(bctxt, pol_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);
}
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
