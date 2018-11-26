#include <gtest/gtest.h>
#include "nic/hal/pd/iris/nw/tnnl_rw_pd.hpp"
#include "nic/hal/iris/include/hal_state.hpp"
#include "nic/include/hal_pd.hpp"
#include "nic/hal/pd/pd_api.hpp"
#include "nic/sdk/include/sdk/eth.hpp"

using hal::pd::pd_tnnl_rw_entry_key_t;
using hal::pd::pd_tnnl_rw_entry_t;

class tnnl_rw_test : public ::testing::Test {
protected:
    tnnl_rw_test() {
  }

  virtual ~tnnl_rw_test() {
  }

  // will be called immediately after the constructor before each test
  virtual void SetUp() {
  }

  // will be called immediately after each test before the destructor
  virtual void TearDown() {
  }

  static void SetUpTestCase() {
      hal::g_delay_delete = false;
  }

};

/* -----------------------------------------------------------------------------
 *
 * Test Case 1:
 *      - Test Case to verify the insert
 * - Create DM table
 * - Insert DM Entry
 *
 * ---------------------------------------------------------------------------*/
TEST_F(tnnl_rw_test, test1) {
    hal_ret_t           ret;
    pd_tnnl_rw_entry_key_t   key;
    pd_tnnl_rw_entry_t       *tnnl_rwe;
    uint64_t            mac_sa = 0x000000000001;
    uint64_t            mac_da = 0x000000000002;
    uint64_t            mac_da1 = 0x000000000003;
    uint32_t            tnnl_rw_idx = 0, tnnl_rw_idx1 = 0;

    MAC_UINT64_TO_ADDR(key.mac_sa, mac_sa);
    MAC_UINT64_TO_ADDR(key.mac_da, mac_da);
    key.tnnl_rw_act = (hal::tunnel_rewrite_actions_en)TUNNEL_REWRITE_ENCAP_VXLAN_ID;

    ret = tnnl_rw_entry_find(&key, &tnnl_rwe);
    ASSERT_TRUE(ret == HAL_RET_ENTRY_NOT_FOUND);

    ret = tnnl_rw_entry_alloc(&key, NULL, &tnnl_rw_idx);
    ASSERT_TRUE(ret == HAL_RET_OK);

    ret = tnnl_rw_entry_find_or_alloc(&key, &tnnl_rw_idx1);
    ASSERT_TRUE(ret == HAL_RET_OK);

    ASSERT_TRUE(tnnl_rw_idx == tnnl_rw_idx1);

    MAC_UINT64_TO_ADDR(key.mac_da, mac_da1);
    ret = tnnl_rw_entry_alloc(&key, NULL, &tnnl_rw_idx);
    ASSERT_TRUE(ret == HAL_RET_OK);

    MAC_UINT64_TO_ADDR(key.mac_da, mac_da);
    ret = tnnl_rw_entry_delete(&key);
    ASSERT_TRUE(ret == HAL_RET_OK);

    ret = tnnl_rw_entry_delete(&key);
    ASSERT_TRUE(ret == HAL_RET_OK);

    ret = tnnl_rw_entry_delete(&key);
    ASSERT_TRUE(ret == HAL_RET_ENTRY_NOT_FOUND);
}

int main(int argc, char **argv) {
    hal::pd::pd_mem_init_args_t    args;
    hal::pd::pd_func_args_t pd_func_args = {0};

    ::testing::InitGoogleTest(&argc, argv);
    args.cfg_path = std::getenv("HAL_CONFIG_PATH");
    pd_func_args.pd_mem_init = &args;
    hal::pd::pd_mem_init(&pd_func_args);
    return RUN_ALL_TESTS();
}
