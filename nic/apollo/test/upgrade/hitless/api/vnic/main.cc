
//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the all upgrade vnic test cases
///
//----------------------------------------------------------------------------

#include "nic/apollo/test/api/utils/device.hpp"
#include "nic/apollo/test/api/utils/policer.hpp"
#include "nic/apollo/test/api/utils/policy.hpp"
#include "nic/apollo/test/api/utils/subnet.hpp"
#include "nic/apollo/test/api/utils/vnic.hpp"
#include "nic/apollo/test/api/utils/vpc.hpp"
#include "nic/apollo/test/api/utils/workflow.hpp"

namespace test {
namespace api {

//----------------------------------------------------------------------------
// VNIC upgrade test class
//----------------------------------------------------------------------------

class vnic_upg_test : public pds_test_base {
protected:
    vnic_upg_test() {}
    virtual ~vnic_upg_test() {}
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
vnic_upg_setup (void)
{
    pds_batch_ctxt_t bctxt = batch_start();
    sample_device_setup(bctxt);
    sample_vpc_setup(bctxt, PDS_VPC_TYPE_TENANT);
    sample_policy_setup(bctxt);
    sample_subnet_setup(bctxt);
    sample_policer_setup(bctxt);
    batch_commit(bctxt);
}

static inline void
vnic_upg_teardown (void)
{
    pds_batch_ctxt_t bctxt = batch_start();
    sample_policer_teardown(bctxt);
    sample_subnet_teardown(bctxt);
    sample_policy_teardown(bctxt);
    sample_vpc_teardown(bctxt, PDS_VPC_TYPE_TENANT);
    sample_device_teardown(bctxt);
    batch_commit(bctxt);
}

//----------------------------------------------------------------------------
// VNIC upg test cases implementation
//----------------------------------------------------------------------------

/// \defgroup VNIC Vnic upg Tests
/// @{

/// \brief VNIC WF_U_1
/// \ref WF_U_1
TEST_F(vnic_upg_test, vnic_upg_workflow_u1) {
    vnic_feeder feeder;

    // setup precursor
    vnic_upg_setup();
    // setup vnic
    feeder.init(int2pdsobjkey(1), int2pdsobjkey(1), k_max_vnic, k_feeder_mac,
                PDS_ENCAP_TYPE_DOT1Q, PDS_ENCAP_TYPE_MPLSoUDP,
                true, true, 0, 0, 5, 0, int2pdsobjkey(20010),
                int2pdsobjkey(20000), 1);
    // backup
    workflow_u1_s1<vnic_feeder>(feeder);

    // tearup precursor
    vnic_upg_teardown();
    // restore
    workflow_u1_s2<vnic_feeder>(feeder);
    // setup precursor again
    vnic_upg_setup();
    // config replay
    workflow_u1_s3<vnic_feeder>(feeder);
    // tearup precursor
    vnic_upg_teardown();
}

/// \brief VNIC WF_U_2
/// \ref WF_U_2
TEST_F(vnic_upg_test, vnic_upg_workflow_u2) {
    vnic_feeder feeder1A, feeder1B, feeder2;
    uint32_t start_key, num_obj, obj_in_set;

    start_key = 1;
    num_obj = 10;
    obj_in_set = 5;
    // setup precursor
    vnic_upg_setup();
    // setup vnic
    feeder1A.init(int2pdsobjkey(start_key), int2pdsobjkey(start_key), num_obj,
                  k_feeder_mac, PDS_ENCAP_TYPE_DOT1Q, PDS_ENCAP_TYPE_MPLSoUDP,
                  true, true, 0, 0, 5, 0, int2pdsobjkey(20010),
                  int2pdsobjkey(20000), 1);
    // backup
    workflow_u1_s1<vnic_feeder>(feeder1A);

    // tearup precursor
    vnic_upg_teardown();
    // restore
    workflow_u1_s2<vnic_feeder>(feeder1A);
    // setup precursor again
    vnic_upg_setup();

    // recalibrate feeder to replay subset of stashed objs
    feeder1A.init(int2pdsobjkey(start_key), int2pdsobjkey(start_key), obj_in_set,
                  k_feeder_mac, PDS_ENCAP_TYPE_DOT1Q, PDS_ENCAP_TYPE_MPLSoUDP,
                  true, true, 0, 0, 5, 0, int2pdsobjkey(20010),
                  int2pdsobjkey(20000));
    // setup another feeder to create additional vnic
    feeder2.init(int2pdsobjkey(start_key + num_obj), int2pdsobjkey(start_key), num_obj,
                 k_feeder_mac, PDS_ENCAP_TYPE_DOT1Q, PDS_ENCAP_TYPE_MPLSoUDP,
                 true, true, 0, 0, 5, 0, int2pdsobjkey(20010),
                 int2pdsobjkey(20000));
    // position another feeder to point to next key to be replayed
    feeder1B.init(int2pdsobjkey(start_key + obj_in_set), int2pdsobjkey(start_key),
                  (num_obj - obj_in_set), k_feeder_mac + obj_in_set,
                  PDS_ENCAP_TYPE_DOT1Q, PDS_ENCAP_TYPE_MPLSoUDP,
                  true, true, 0, 0, 5, 0, int2pdsobjkey(20010),
                  int2pdsobjkey(20000));
    workflow_u2<vnic_feeder>(feeder1A, feeder2, feeder1B);
    // tearup precursor
    vnic_upg_teardown();
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
