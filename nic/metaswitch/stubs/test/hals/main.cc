//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains all VXLAN tunnel test cases
///
//----------------------------------------------------------------------------

#include <nbase.h>
#include <ntl_remote_object.hpp>
extern "C"
{
#include <lipi.h>
}
#include "nic/metaswitch/stubs/common/pdsa_util.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_li.hpp"
#include "nic/metaswitch/stubs/test/hals/test_params.hpp"
#include "nic/metaswitch/stubs/test/hals/vxlan_test_params.hpp"
#include "nic/metaswitch/stubs/test/hals/phy_port_test_params.hpp"
#include "nic/metaswitch/stubs/test/hals/underlay_ecmp_test_params.hpp"
#include "nic/metaswitch/stubs/test/hals/bd_test_params.hpp"
#include "nic/metaswitch/stubs/test/hals/vrf_test_params.hpp"
#include "nic/metaswitch/stubs/common/pdsa_state_init.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"
#include "nic/metaswitch/stubs/pdsa_stubs_init.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/apollo/api/include/pds_init.hpp"
#include <iostream>
#include <fstream>

/*------- Test class hierarchy and dependencies -----
 *
 * xxx - stands for an area that is being tested
 *       e.g. vxlan - tests vxlan tunnel create,update,delete
 *
 *                 test_params 
 *                 /         \
 *                /           \
 *       test_input_base   test_output_base 
 *                |             \
 *                |            pds_api_mock
 *          xxx_test_params        \
 *           /           \          \
 *          /             \          \
 *         /               \          \
 *    xxx_ips_feeder        xxx_pds_mock
 *        (or)                   (or)
 *    xxx_proto_feeder      xxx_hal_pds_verif
 *
 *-------------------------------------------------*/
  
namespace pdsa_test {
test_params_t* test_params() {    
    static test_params_t  g_test_params;
    return &g_test_params;
}
} // End namespace pdsa_test

class pdsa_hals_test: public ::testing::Test {
protected:
    void TearDown(void) {
        pdsa_test::test_params()->test_input->cleanup();
        pdsa_test::test_params()->test_output->cleanup();
        pdsa_test::test_params()->test_input = nullptr;
        pdsa_test::test_params()->test_output = nullptr;
    }
};

TEST_F(pdsa_hals_test, phy_port_test) {
    pdsa_test::load_phy_port_test();
    auto test_input = pdsa_test::test_params()->test_input;
    auto test_output = pdsa_test::test_params()->test_output;

    // Initialize
    auto phy_port_input = dynamic_cast<pdsa_test::phy_port_input_params_t*>
                              (test_input);
    phy_port_input->init ();

    // Create
    std::cout << "=== IF Create test ===" << std::endl;
    test_output->expect_create();
    test_input->trigger_create();
    test_output->validate();

    // Update
    std::cout << "=== IF Update test ===" << std::endl;
    test_input->modify(); // Change any field
    test_output->expect_update();
    test_input->trigger_update();
    test_output->validate();

    // Delete
    std::cout << "=== IF Delete test ===" << std::endl;
    test_output->expect_delete();
    test_input->trigger_delete();
    test_output->validate();

    // Mock batch update failure
    std::cout << "=== IF Spec Create failure test ===" << std::endl;
    test_input->next();
    test_output->expect_pds_spec_op_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock batch commit failure
    std::cout << "=== IF Spec Batch commit failure test ===" << std::endl;
    test_input->next();
    test_output->expect_pds_batch_commit_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock async batch failure return
    std::cout << "=== IF Spec Batch commit async failure test ===" << std::endl;
    test_input->next();
    test_output->expect_create_pds_async_fail();
    test_input->trigger_create();
    test_output->validate();
}

TEST_F(pdsa_hals_test, vxlan_test) {
    pdsa_test::load_vxlan_test();
    auto test_input = pdsa_test::test_params()->test_input;
    auto test_output = pdsa_test::test_params()->test_output;

    // Initialize
    auto vxlan_input = dynamic_cast<pdsa_test::vxlan_input_params_t*>(test_input);
    vxlan_input->init ("10.0.0.1",  // Source IP
                       "20.0.0.1"); // Dest IP
    // Create
    std::cout << "=== TEP Create test ===" << std::endl;
    test_output->expect_create();
    test_input->trigger_create();
    test_output->validate();

    // Update
    std::cout << "=== TEP Update test ===" << std::endl;
    test_input->modify(); // Change any field
    test_output->expect_update();
    test_input->trigger_update();
    test_output->validate();

    // Delete
    std::cout << "=== TEP Delete test ===" << std::endl;
    test_output->expect_delete();
    test_input->trigger_delete();
    test_output->validate();

    // Mock batch update failure
    std::cout << "=== TEP Spec Create failure test ===" << std::endl;
    test_input->next();
    test_output->expect_pds_spec_op_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock batch commit failure
    std::cout << "=== TEP Spec Batch commit failure test ===" << std::endl;
    test_input->next();
    test_output->expect_pds_batch_commit_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock async batch failure return
    std::cout << "=== TEP Spec Batch commit async failure test ===" << std::endl;
    test_input->next();
    test_output->expect_create_pds_async_fail();
    test_input->trigger_create();
    test_output->validate();
}

TEST(pdsa_loopback_test, test) {
   
    ip_addr_t in_ip;
    inet_aton ("39.0.0.1", (in_addr*) &(in_ip.addr.v4_addr));
    in_ip.af = IP_AF_IPV4;
    ATG_LIPI_L3_IP_ADDR ip_addr = {0};
    pdsa_stub::convert_ipaddr_pdsa_to_ms (in_ip, &ip_addr.inet_addr);

    std::cout << "=== Loopback IP Add test ===" << std::endl;
    pds_ms::li_is()->softwif_addr_set("lo1", &ip_addr,
                                         nullptr);
    sleep(1);
    auto fp = popen ("ip addr show dev lo to 39.0.0.1 | grep 39.0.0.1", "r");
    if (!fp) {
        throw std::runtime_error ("ERROR Fetching linux loopback IP");
    }
    char buf[100];
    buf[0] = 0;
    while (std::fgets(buf, sizeof buf, fp) != NULL) {
        std::cout << '"' << buf << '"' << '\n';
    }
    pclose (fp);
    ASSERT_TRUE (strcmp (buf,"") != 0) ;

    std::cout << "=== Loopback IP Delete test ===" << std::endl;
    pds_ms::li_is()->softwif_addr_del("lo1", &ip_addr,
                                         nullptr);
    sleep(1);
    buf[0] = 0;
    fp = popen ("ip addr show dev lo to 39.0.0.1", "r");
    if (!fp) {
        throw std::runtime_error ("ERROR Fetching linux loopback IP");
    }
    while (std::fgets(buf, sizeof buf, fp) != NULL) {
        std::cout << '"' << buf << '"' << '\n';
    }
    pclose (fp);
    ASSERT_TRUE (strcmp (buf,"") == 0) ;
}

TEST_F(pdsa_hals_test, underlay_ecmp_test) {
    pdsa_test::load_underlay_ecmp_test();
    auto test_input = pdsa_test::test_params()->test_input;
    auto test_output = pdsa_test::test_params()->test_output;

    // Initialize
    std::vector<pdsa_test::nhinfo_t>   nexthops;
    auto underlay_ecmp_input = dynamic_cast<pdsa_test::underlay_ecmp_input_params_t*>(test_input);
    // Nexthop MS L3 Intf Index, Dest MAC
    underlay_ecmp_input->init ({{0x10001, "00:05:56:54:57:58"},
                                {0x20001, "00:06:66:64:67:68"}});
    // Create
    std::cout << "=== NH Create test ===" << std::endl;
    test_output->expect_create();
    test_input->trigger_create();
    test_output->validate();

    // Update
    std::cout << "=== NH Update test ===" << std::endl;
    test_input->modify(); // Delete 1 NH in group
    test_output->expect_update();
    test_input->trigger_update();
    test_output->validate();

    // Delete
    std::cout << "=== NH Delete test ===" << std::endl;
    test_output->expect_delete();
    test_input->trigger_delete();
    test_output->validate();
}

TEST_F(pdsa_hals_test, bd_test) {
    pdsa_test::load_bd_test();
    auto test_input = pdsa_test::test_params()->test_input;
    auto test_output = pdsa_test::test_params()->test_output;

    // Initialize
    auto bd_input = dynamic_cast<pdsa_test::bd_input_params_t*>
                              (test_input);
    bd_input->init ();

    // Create
    std::cout << "=== BD Create test ===" << std::endl;
    test_output->expect_create();
    test_input->trigger_create();
    test_output->validate();

    // Update
    std::cout << "=== BD Update test ===" << std::endl;
    // Modify both fastpath and slowpath fields
    // Expect update of only fastpath fields first
    ((pdsa_test::bd_input_params_t*) test_input)->modify_fast_fields();
    test_output->expect_update();
    ((pdsa_test::bd_input_params_t*) test_input)->modify_slow_fields();
    // Trigger direct update
    ((pdsa_test::bd_input_params_t*) test_input)->send_direct_update();
    test_output->validate();
    sleep(1);
    // Next Expect update of slowpath fields
    test_output->expect_update();
    // Trigger BD stub call for slowpath fields
    test_input->trigger_update();
    test_output->validate();

    std::cout << "=== BD If bind test ===" << std::endl;
    ((pdsa_test::bd_input_params_t*) test_input)->add_if_bind();
    test_output->expect_update();
    test_input->trigger_update();
    test_output->validate();

    std::cout << "=== BD If Unbind test ===" << std::endl;
    ((pdsa_test::bd_input_params_t*) test_input)->del_if_bind();
    test_output->expect_update();
    test_input->trigger_update();
    test_output->validate();

    // Delete
    std::cout << "=== BD Delete test ===" << std::endl;
    test_output->expect_delete();
    test_input->trigger_delete();
    test_output->validate();

    // Mock batch spec create failure
    std::cout << "=== BD Spec Create failure test ===" << std::endl;
    test_input->next();
    test_output->expect_pds_spec_op_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock batch commit failure
    std::cout << "=== BD Spec Batch commit failure test ===" << std::endl;
    test_input->next();
    test_output->expect_pds_batch_commit_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock async batch failure return
    std::cout << "=== BD Spec Batch commit async failure test ===" << std::endl;
    test_input->next();
    test_output->expect_create_pds_async_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock Direct update received before L2F Stub create call
    std::cout << "=== BD Direct update out of sequence test ===" << std::endl;
    ((pdsa_test::bd_input_params_t*) test_input)->init_direct_update();
    test_output->expect_create();
    ((pdsa_test::bd_input_params_t*) test_input)->send_direct_update();
    test_input->trigger_create();
    test_output->validate();

    // Mock Direct update received after L2F Stub create call
    std::cout << "=== BD Direct update in sequence test ===" << std::endl;
    ((pdsa_test::bd_input_params_t*) test_input)->modify_direct_update();
    test_output->expect_update();
    ((pdsa_test::bd_input_params_t*) test_input)->send_direct_update();
    test_output->validate();
}

TEST_F(pdsa_hals_test, vrf_test) {
    pdsa_test::load_vrf_test();
    auto test_input = pdsa_test::test_params()->test_input;
    auto test_output = pdsa_test::test_params()->test_output;

    // Initialize
    auto vrf_input = dynamic_cast<pdsa_test::vrf_input_params_t*>
                              (test_input);
    vrf_input->init ();

    // Create
    std::cout << "=== VRF Create test ===" << std::endl;
    test_output->expect_create();
    test_input->trigger_create();
    test_output->validate();

    // Update
    std::cout << "=== VRF Update test ===" << std::endl;
    test_input->modify(); 
    test_output->expect_update();
    test_input->trigger_update();
    test_output->validate();

    // Delete
    std::cout << "=== VRF Delete test ===" << std::endl;
    test_output->expect_delete();
    test_input->trigger_delete();
    test_output->validate();

    // Mock batch spec create failure
    std::cout << "=== VRF Spec Create failure test ===" << std::endl;
    test_input->next();
    test_output->expect_pds_spec_op_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock batch commit failure
    std::cout << "=== VRF Spec Batch commit failure test ===" << std::endl;
    test_input->next();
    test_output->expect_pds_batch_commit_fail();
    test_input->trigger_create();
    test_output->validate();

    // Mock async batch failure return
    std::cout << "=== VRF Spec Batch commit async failure test ===" << std::endl;
    test_input->next();
    test_output->expect_create_pds_async_fail();
    test_input->trigger_create();
    test_output->validate();

    // LI Direct update received before LI Stub create call
    std::cout << "=== VRF Direct update out of sequence test ===" << std::endl;
    ((pdsa_test::vrf_input_params_t*) test_input)->init_direct_update();
    test_output->expect_create();
    ((pdsa_test::vrf_input_params_t*) test_input)->send_direct_update();
    test_input->trigger_create();
    test_output->validate();

    // LI Direct update received after LI Stub create call
    std::cout << "=== VRF Direct update in sequence test ===" << std::endl;
    ((pdsa_test::vrf_input_params_t*) test_input)->modify_direct_update();
    test_output->expect_update();
    ((pdsa_test::vrf_input_params_t*) test_input)->send_direct_update();
    test_output->validate();
}


//----------------------------------------------------------------------------
// Entry point
//----------------------------------------------------------------------------
static sdk::lib::thread *g_routing_thread;
int
main (int argc, char **argv)
{
    // Call the mock pds init
    pds_init(nullptr);
    // This will start nbase
    g_routing_thread =
        sdk::lib::thread::factory(
            "routing", 0, sdk::lib::THREAD_ROLE_CONTROL,
            0x0, &pdsa_stub::pdsa_thread_init,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            false);
    SDK_ASSERT_TRACE_RETURN((g_routing_thread != NULL), SDK_RET_ERR,
                            "Routing thread create failure");
    g_routing_thread->start(g_routing_thread);
    std::cout << "Config thread is waiting for Nbase....\n";
    while (!g_routing_thread->ready()) {
         pthread_yield();
    }
    std::cout << "Nbase is ready!\n";
    sleep(1);

    while (pdsa_stub::state_t::thread_context().state()->get_slab_in_use (pdsa_stub::PDSA_COOKIE_SLAB_ID)
           != 0) {
        sleep(1);
    }
    std::cout << "Nbase is really ready!\n";
#ifdef PDS_MOCKAPI
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#else
    return api_test_program_run(argc, argv);
#endif
}
