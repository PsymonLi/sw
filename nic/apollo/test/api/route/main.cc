//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the all route test cases
///
//----------------------------------------------------------------------------

#include "nic/apollo/test/api/utils/api_base.hpp"
#include "nic/apollo/test/api/utils/batch.hpp"
#include "nic/apollo/test/api/utils/nexthop.hpp"
#include "nic/apollo/test/api/utils/route.hpp"
#include "nic/apollo/test/api/utils/device.hpp"
#include "nic/apollo/test/api/utils/tep.hpp"
#include "nic/apollo/test/api/utils/vpc.hpp"
#include "nic/apollo/test/api/utils/subnet.hpp"
#include "nic/apollo/test/api/utils/if.hpp"
#include "nic/apollo/test/api/utils/nexthop.hpp"
#include "nic/apollo/test/api/utils/nexthop_group.hpp"
#include "nic/apollo/test/api/utils/workflow.hpp"

namespace test {
namespace api {

// globals
static const std::string k_base_v4_pfx    = "100.50.100.1/16";
static const std::string k_base_v4_pfx_2  = "100.100.100.1/16";
static const std::string k_base_v4_pfx_3  = "100.150.100.1/16";
static const std::string k_base_v4_pfx_4  = "100.200.100.1/16";
static const std::string k_base_v4_pfx_5  = "100.225.100.1/16";
static const std::string k_base_v4_pfx_6  = "100.201.100.1/16";
static int k_max_route_per_tbl = PDS_MAX_ROUTE_PER_TABLE;

//----------------------------------------------------------------------------
// Route test class
//----------------------------------------------------------------------------

class route_test : public pds_test_base {
protected:
    route_test() {}
    virtual ~route_test() {}
    virtual void SetUp() {}
    virtual void TearDown() {}
    static void SetUpTestCase() {
        if (!agent_mode()) {
            pds_test_base::SetUpTestCase(g_tc_params);
        }
        pds_batch_ctxt_t bctxt = batch_start();
        sample1_vpc_setup(bctxt, PDS_VPC_TYPE_TENANT);
        sample_device_setup(bctxt);
        sample_if_setup(bctxt);
        sample_nexthop_setup(bctxt);
        sample_nexthop_group_setup(bctxt);
        sample_tep_setup(bctxt);
        sample_subnet_setup(bctxt);
        batch_commit(bctxt);
    }
    static void TearDownTestCase() {
        pds_batch_ctxt_t bctxt = batch_start();
        sample_subnet_teardown(bctxt);
        sample_tep_teardown(bctxt);
        sample_nexthop_group_teardown(bctxt);
        sample_nexthop_teardown(bctxt);
        sample_if_teardown(bctxt);
        sample_device_teardown(bctxt);
        sample1_vpc_teardown(bctxt, PDS_VPC_TYPE_TENANT);
        batch_commit(bctxt);
        if (!agent_mode()) {
            pds_test_base::TearDownTestCase();
        }
    }
};

static uint32_t k_num_init_routes = 10;
static uint32_t k_num_route_add = 5;
static uint32_t k_num_route_del = 2;
static uint32_t k_num_route_tables = 1;
static uint32_t k_route_table_id = 1;

//----------------------------------------------------------------------------
// Route test cases implementation
//----------------------------------------------------------------------------
/// \defgroup ROUTE_TESTS Route tests
/// @{
/// \brief Route table WF_B1
/// \ref WF_B1
TEST_F(route_test, route_workflow_b1) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder;

    // setup a route-table
    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    // create one route
    feeder.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    workflow_b1<route_feeder>(feeder);

    // create multiple routes
    feeder.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    workflow_b1<route_feeder>(feeder);

    // teadown the route-table
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route table memory leak test
/// \ref WF_B2
TEST_F(route_test, route_memory_leak) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder_1;
    FILE *fp;
    int fd;

    fp = fopen("memory_leak_dump.txt", "w");
    if (fp) {
        fd = fileno(fp);
        bctxt = batch_start();
        sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                                 k_num_init_routes, k_num_route_tables,
                                 k_route_table_id);
        batch_commit(bctxt);

        // dump memory statistics
        dprintf(fd, "Memory dump before route creation:\n");
        memory_dump(fd);
        for (uint32_t i = 0; i < 10; i ++) {
            // create 1000 routes
            feeder_1.init(k_base_v4_pfx_2, 1000, 100, k_route_table_id,
                          PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                          false);
            workflow_b1<route_feeder>(feeder_1);
            // dump memory statistics
            dprintf(fd, "Memory dump after iteration %u:\n", i + 1);
            memory_dump(fd);
        }

        // malloc trim
        malloc_trim(0);
        dprintf(fd, "Memory dump after malloc trim:\n");
        memory_dump(fd);

        bctxt = batch_start();
        sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
        batch_commit(bctxt);

        fclose(fp);
    }
}

/// \brief Route table WF_B2
/// \ref WF_B2
TEST_F(route_test, route_workflow_b2) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder_1;
    route_feeder feeder_2;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    // create one route
    feeder_1.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    // update the route
    feeder_2.init(k_base_v4_pfx_3, 1, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    workflow_b2<route_feeder>(feeder_1, feeder_2);

    // create multiple routes
    feeder_1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    feeder_2.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                                    PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                                                      false);
    workflow_b2<route_feeder>(feeder_1, feeder_2);

    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_1
/// \ref WF_1
TEST_F(route_test, route_workflow_1) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    // create one route
    feeder.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    workflow_1<route_feeder>(feeder);

    // create multiple routes
    feeder.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    workflow_1<route_feeder>(feeder);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_2
/// \ref WF_2
TEST_F(route_test, route_workflow_2) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    workflow_2<route_feeder>(feeder);

    feeder.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    workflow_2<route_feeder>(feeder);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_3
/// \ref WF_3
TEST_F(route_test, route_workflow_3) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1,feeder2,feeder3;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder2.init(k_base_v4_pfx_3, 1, 200, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder3.init(k_base_v4_pfx_4, 1, 300, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY_ECMP, PDS_NAT_TYPE_NONE,
                 false);
    workflow_3<route_feeder>(feeder1, feeder2, feeder3);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder2.init(k_base_v4_pfx_3, 50, 200, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder3.init(k_base_v4_pfx_4, 50, 300, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    workflow_3<route_feeder>(feeder1, feeder2, feeder3);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route table WF_4
/// \ref WF_4
TEST_F(route_test, route_workflow_4) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    workflow_4<route_feeder>(feeder);

    feeder.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    workflow_4<route_feeder>(feeder);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route table WF_5
/// \ref WF_5
TEST_F(route_test, route_workflow_5) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1,feeder2,feeder3;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder2.init(k_base_v4_pfx_3, 1, 200, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder3.init(k_base_v4_pfx_4, 1, 300, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    workflow_5<route_feeder>(feeder1, feeder2, feeder3);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder2.init(k_base_v4_pfx_3, 50, 200, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder3.init(k_base_v4_pfx_4, 50, 300, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    workflow_5<route_feeder>(feeder1, feeder2, feeder3);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route table WF_6
/// \ref WF_6
TEST_F(route_test, route_workflow_6) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1, feeder1A, feeder1B;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder1A.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_IP, PDS_NAT_TYPE_NONE,
                 true);
    feeder1B.init(k_base_v4_pfx_4, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_VNIC, PDS_NAT_TYPE_NONE,
                 false);
    workflow_6<route_feeder>(feeder1, feeder1A, feeder1B);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_7
/// \ref WF_7
TEST_F(route_test, route_workflow_7) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1, feeder1A, feeder1B;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder1A.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_IP, PDS_NAT_TYPE_NONE,
                 true);
    feeder1B.init(k_base_v4_pfx_4, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_VNIC, PDS_NAT_TYPE_NONE,
                 false);
    workflow_7<route_feeder>(feeder1, feeder1A, feeder1B);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_8
/// \ref WF_8
TEST_F(route_test, route_workflow_8) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1, feeder1A, feeder1B;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder1A.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY_ECMP, PDS_NAT_TYPE_NONE,
                 true);
    feeder1B.init(k_base_v4_pfx_4, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_PEER_VPC, PDS_NAT_TYPE_NONE,
                 false);
    workflow_8<route_feeder>(feeder1, feeder1A, feeder1B);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_9
/// \ref WF_9
TEST_F(route_test, route_workflow_9) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1, feeder1A;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  false);
    feeder1A.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_VNIC, PDS_NAT_TYPE_NONE,
                  true);
    workflow_9<route_feeder>(feeder1, feeder1A);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route table WF_10
/// \ref WF_10
TEST_F(route_test, route_workflow_10) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1, feeder2, feeder2A, feeder3, feeder3A, feeder4;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder2.init(k_base_v4_pfx_3, 25, 200, k_route_table_id,
                 PDS_NH_TYPE_IP, PDS_NAT_TYPE_NONE,
                 false);
    feeder2A.init(k_base_v4_pfx_2, 25, 200, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY_ECMP, PDS_NAT_TYPE_NONE,
                  true);
    feeder3.init(k_base_v4_pfx_4, 25, 300, k_route_table_id,
                 PDS_NH_TYPE_PEER_VPC, PDS_NAT_TYPE_NONE,
                 false);
    feeder3A.init(k_base_v4_pfx_5, 25, 300, k_route_table_id,
                  PDS_NH_TYPE_VNIC, PDS_NAT_TYPE_NONE,
                  true);
    feeder4.init(k_base_v4_pfx_2, 25, 400, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    workflow_10<route_feeder>(
        feeder1, feeder2, feeder2A, feeder3, feeder3A, feeder4);

    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_N_1
/// \ref WF_N_1
/// This case is failng for route as we reject (with assert statement)
/// the config if we find the two same routes in a route-table while
/// programming the hardware
TEST_F(route_test, DISABLED_route_workflow_neg_1) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    workflow_neg_1<route_feeder>(feeder);

    feeder.init(k_base_v4_pfx_3, 50, 200, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    workflow_neg_1<route_feeder>(feeder);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_N_2
/// \ref WF_N_2
/// This case is failing because even though we have exceeded the limit for
/// maximum number of routes per route-table we are still processing the
/// batch successfully
TEST_F(route_test, DISABLED_route_workflow_neg_2) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder.init(k_base_v4_pfx_2, PDS_MAX_ROUTE_PER_TABLE+2, 100,
                k_route_table_id, PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    workflow_neg_2<route_feeder>(feeder);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_N_3
/// \ref WF_N_3
TEST_F(route_test, route_workflow_neg_3) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    workflow_neg_3<route_feeder>(feeder);

    feeder.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    workflow_neg_3<route_feeder>(feeder);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_N_4
/// \ref WF_N_4
TEST_F(route_test, route_workflow_neg_4) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1, feeder2;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 1, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder2.init(k_base_v4_pfx_3, 1, 200, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    workflow_neg_4<route_feeder>(feeder1, feeder2);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder2.init(k_base_v4_pfx_3, 50, 200, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    workflow_neg_4<route_feeder>(feeder1, feeder2);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_N_5
/// \ref WF_N_5
TEST_F(route_test, route_workflow_neg_5) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1,feeder1A;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder1A.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  true);
    workflow_neg_5<route_feeder>(feeder1, feeder1A);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_N_6
/// \ref WF_N_6
TEST_F(route_test, route_workflow_neg_6) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1,feeder1A;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_5, 25, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder1A.init(k_base_v4_pfx_5, 50, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  true);
    workflow_neg_6<route_feeder>(feeder1, feeder1A);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_N_7
/// \ref WF_N_7
TEST_F(route_test, route_workflow_neg_7) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1, feeder1A, feeder2;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 25, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder1A.init(k_base_v4_pfx_3, 25, 100, k_route_table_id,
                  PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                  true);
    feeder2.init(k_base_v4_pfx_4, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 true);
    workflow_neg_7<route_feeder>(feeder1, feeder1A, feeder2);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}

/// \brief Route WF_N_8
/// \ref WF_N_8
TEST_F(route_test, route_workflow_neg_8) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder1,feeder2;

    bctxt = batch_start();
    sample_route_table_setup(bctxt, k_base_v4_pfx, IP_AF_IPV4,
                             k_num_init_routes, k_num_route_tables,
                             k_route_table_id);
    batch_commit(bctxt);

    feeder1.init(k_base_v4_pfx_2, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 false);
    feeder2.init(k_base_v4_pfx_3, 50, 100, k_route_table_id,
                 PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                 true);
    workflow_neg_8<route_feeder>(feeder1, feeder2);
    bctxt = batch_start();
    sample_route_table_teardown(bctxt, k_route_table_id, k_num_route_tables);
    batch_commit(bctxt);
}
/// @}
//---------------------------------------------------------------------
// Non templatized test cases
//---------------------------------------------------------------------
/// \brief change route-table and routes in the same batch
TEST_F(route_test, route_update_rt_tbl_rt) {
    pds_batch_ctxt_t bctxt;
    route_feeder feeder;
    route_table_feeder rt_feeder;
    sdk_ret_t ret;

    // create route_table and add routes in the same batch
    bctxt = batch_start();
    rt_feeder.init(k_base_v4_pfx_2, IP_AF_IPV4, 100,
                   k_num_route_tables, 1);
    feeder.init(k_base_v4_pfx_6, 25, 101, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    ret = many_create<route_table_feeder>(bctxt, rt_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    ret = many_create<route_feeder>(bctxt, feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);

    ret = many_read<route_feeder>(feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    feeder.init(k_base_v4_pfx_2, 100, 1, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    ret = many_read<route_feeder>(feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    bctxt = batch_start();
    // teardown the route-table
    rt_feeder.init("0.0.0.0/0", IP_AF_IPV4, PDS_MAX_ROUTE_PER_TABLE,
                   k_num_route_tables, k_route_table_id);
    ret = many_delete(bctxt, rt_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);

    // create route-table and delete routes in same batch
    bctxt = batch_start();
    rt_feeder.init(k_base_v4_pfx_2, IP_AF_IPV4, 125,
                   k_num_route_tables, 1);
    feeder.init(k_base_v4_pfx_6, 25, 101, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    ret = many_create<route_table_feeder>(bctxt, rt_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    ret = many_delete<route_feeder>(bctxt, feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);

    ret = many_read<route_feeder>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);
    ASSERT_TRUE(ret == SDK_RET_OK);
    // tear-down route-table
    bctxt = batch_start();
    rt_feeder.init("0.0.0.0/0", IP_AF_IPV4, PDS_MAX_ROUTE_PER_TABLE,
                   k_num_route_tables, k_route_table_id);
    ret = many_delete(bctxt, rt_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);

    // create route-table and update routes in same batch
    bctxt = batch_start();
    rt_feeder.init(k_base_v4_pfx_2, IP_AF_IPV4, 125,
                   k_num_route_tables, 1);
    feeder.init(k_base_v4_pfx_6, 25, 101, k_route_table_id,
                PDS_NH_TYPE_OVERLAY, PDS_NAT_TYPE_NONE,
                false);
    ret = many_create<route_table_feeder>(bctxt, rt_feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    ret = many_update<route_feeder>(bctxt, feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    batch_commit(bctxt);

    ret = many_read<route_feeder>(feeder);
    ASSERT_TRUE(ret == SDK_RET_OK);
    bctxt = batch_start();
    rt_feeder.init("0.0.0.0/0", IP_AF_IPV4, PDS_MAX_ROUTE_PER_TABLE,
                   k_num_route_tables, k_route_table_id);
    ret = many_delete(bctxt, rt_feeder);
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
