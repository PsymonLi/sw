#include "nic/hal/src/interface.hpp"
#include "nic/gen/proto/hal/interface.pb.h"
#include "nic/hal/hal.hpp"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include "nic/hal/test/utils/hal_test_utils.hpp"
#include "nic/hal/test/utils/hal_base_test.hpp"

using intf::LifSpec;
using intf::LifResponse;
using intf::LifKeyHandle;
using hal::lif_hal_info_t;

class lif_test : public hal_base_test {
protected:
  lif_test() {
  }

  virtual ~lif_test() {
  }

  // will be called immediately after the constructor before each test
  virtual void SetUp() {
  }

  // will be called immediately after each test before the destructor
  virtual void TearDown() {
  }

  // Will be called at the beginning of all test cases in this class
  static void SetUpTestCase() {
    hal_base_test::SetUpTestCase();
    hal_test_utils_slab_disable_delete();
  }

};

// ----------------------------------------------------------------------------
// Creating a lif
// ----------------------------------------------------------------------------
TEST_F(lif_test, test1) 
{
    hal_ret_t            ret;
    LifSpec spec;
    LifResponse rsp;

    spec.set_port_num(10);
    spec.set_vlan_strip_en(1);
    //spec.set_allmulti(1);
    spec.mutable_key_or_handle()->set_lif_id(1);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::lif_create(spec, &rsp, NULL);
    hal::hal_cfg_db_close();
    printf("ret: %d\n", ret);
    ASSERT_TRUE(ret == HAL_RET_OK);

    spec.set_vlan_strip_en(0);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::lif_update(spec, &rsp);
    hal::hal_cfg_db_close();
    printf("ret: %d\n", ret);
    ASSERT_TRUE(ret == HAL_RET_OK);

    spec.set_vlan_strip_en(0);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::lif_update(spec, &rsp);
    hal::hal_cfg_db_close();
    printf("ret: %d\n", ret);
    ASSERT_TRUE(ret == HAL_RET_OK);
}

// ----------------------------------------------------------------------------
// Creating muliple lifs with hwlifid
// ----------------------------------------------------------------------------
TEST_F(lif_test, test2) 
{
    hal_ret_t            ret;
    LifSpec             spec;
    LifResponse         rsp;
    lif_hal_info_t      lif_info = {0};

    uint32_t            hw_lif_id = 100;
    for (int i = 0; i < 10; i++) {
        spec.set_port_num(i);
        spec.set_vlan_strip_en(i & 1);
        //spec.set_allmulti(i & 1);
        spec.mutable_key_or_handle()->set_lif_id(200 + i);

        lif_info.with_hw_lif_id = true;
        lif_info.hw_lif_id = hw_lif_id;
        hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
        ret = hal::lif_create(spec, &rsp, &lif_info);
        hal::hal_cfg_db_close();
        ASSERT_TRUE(ret == HAL_RET_INVALID_ARG);
        hw_lif_id++;
    }

}
// ----------------------------------------------------------------------------
// Creating muliple lifs
// ----------------------------------------------------------------------------
TEST_F(lif_test, test3) 
{
    hal_ret_t            ret;
    LifSpec spec;
    LifResponse rsp;

    for (int i = 0; i < 10; i++) {
        spec.set_port_num(i);
        spec.set_vlan_strip_en(i & 1);
        //spec.set_allmulti(i & 1);
        spec.mutable_key_or_handle()->set_lif_id(300 + i);

        hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
        ret = hal::lif_create(spec, &rsp, NULL);
        hal::hal_cfg_db_close();
        ASSERT_TRUE(ret == HAL_RET_OK);
    }

}

// ----------------------------------------------------------------------------
// Creating & deleting of a lif
// ----------------------------------------------------------------------------
TEST_F(lif_test, test4) 
{
    hal_ret_t                       ret;
    LifSpec                         spec;
    LifResponse                     rsp;
    LifDeleteRequest                del_req;
    LifDeleteResponseMsg            del_rsp;
    slab_stats_t                    *pre = NULL, *post = NULL;
    bool                            is_leak = false;

    pre = hal_test_utils_collect_slab_stats();

    // Create lif
    spec.set_port_num(10);
    spec.set_vlan_strip_en(1);
    //spec.set_allmulti(1);
    spec.mutable_key_or_handle()->set_lif_id(400);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::lif_create(spec, &rsp, NULL);
    hal::hal_cfg_db_close();
    ASSERT_TRUE(ret == HAL_RET_OK);

    // Delete lif
    del_req.mutable_key_or_handle()->set_lif_id(400);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::lif_delete(del_req, &del_rsp);
    hal::hal_cfg_db_close();
    ASSERT_TRUE(ret == HAL_RET_OK);

    post = hal_test_utils_collect_slab_stats();

    hal_test_utils_check_slab_leak(pre, post, &is_leak);
    ASSERT_TRUE(is_leak == false);
}
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
