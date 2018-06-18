#include "nic/hal/src/aclqos/qos.hpp"
#include "nic/gen/proto/hal/qos.pb.h"
#include "nic/hal/hal.hpp"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include "nic/p4/iris/include/defines.h"
#include "nic/hal/test/utils/hal_base_test.hpp"
#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/text_format.h>

using google::protobuf::util::MessageDifferencer;

using qos::CoppSpec;
using qos::CoppResponse;
using kh::CoppKeyHandle;
using kh::CoppType;
using qos::CoppDeleteRequest;
using qos::CoppDeleteResponse;

class copp_test : public hal_base_test {
protected:
  copp_test() {
  }

  virtual ~copp_test() {
  }

  // will be called immediately after the constructor before each test
  virtual void SetUp() {
  }

  // will be called immediately after each test before the destructor
  virtual void TearDown() {
  }

};

// Creating user-defined copps - this should fail because 
// all copps are created as part of init
TEST_F(copp_test, test1)
{
    hal_ret_t    ret;
    CoppSpec     spec;
    CoppResponse rsp;

    spec.Clear();

    spec.mutable_key_or_handle()->set_copp_type(kh::COPP_TYPE_FLOW_MISS);
    spec.mutable_policer()->mutable_bps_policer()->set_bytes_per_sec(10000);
    spec.mutable_policer()->mutable_bps_policer()->set_burst_bytes(1000);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_create(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_NE(ret, HAL_RET_OK);


    spec.Clear();
    spec.mutable_key_or_handle()->set_copp_type(kh::COPP_TYPE_ARP);
    spec.mutable_policer()->mutable_bps_policer()->set_bytes_per_sec(10000);
    spec.mutable_policer()->mutable_bps_policer()->set_burst_bytes(1000);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_create(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_NE(ret, HAL_RET_OK);
}

// Negative tests
TEST_F(copp_test, test2)
{
    hal_ret_t    ret;
    CoppSpec     spec;
    CoppResponse rsp;

    spec.Clear();

    spec.mutable_key_or_handle()->set_copp_type(kh::COPP_TYPE_DHCP);
    // rate is 0
    spec.mutable_policer()->mutable_bps_policer()->set_burst_bytes(1000);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_create(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_NE(ret, HAL_RET_OK);

    spec.Clear();
    // copp type not set
    spec.mutable_policer()->mutable_bps_policer()->set_bytes_per_sec(10000);
    spec.mutable_policer()->mutable_bps_policer()->set_burst_bytes(1000);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_create(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_INVALID_ARG);

    spec.Clear();
    // Re-create already created copp
    spec.mutable_key_or_handle()->set_copp_type(kh::COPP_TYPE_ARP);
    spec.mutable_policer()->mutable_bps_policer()->set_bytes_per_sec(10000);
    spec.mutable_policer()->mutable_bps_policer()->set_burst_bytes(1000);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_create(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_ENTRY_EXISTS);

    // Delete without copp-type set
    CoppDeleteRequest del_req;
    CoppDeleteResponse del_rsp;
    del_req.Clear();
    del_rsp.Clear();
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_delete(del_req, &del_rsp);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_INVALID_ARG);
}

// Get and update 
TEST_F(copp_test, test3)
{
    hal_ret_t    ret;
    CoppSpec     spec;
    CoppResponse rsp;
    CoppGetRequest get_req;
    CoppGetResponse get_rsp;
    CoppGetResponseMsg resp_msg;
    CoppType copp_type = kh::COPP_TYPE_FLOW_MISS;

    get_req.Clear();
    get_rsp.Clear();
    resp_msg.Clear();

    get_req.mutable_key_or_handle()->set_copp_type(copp_type);
    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    ret = hal::copp_get(get_req, &resp_msg);
    get_rsp = resp_msg.response(0);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_OK);

    spec.Clear();
    spec.mutable_key_or_handle()->set_copp_type(copp_type);
    spec.mutable_policer()->mutable_bps_policer()->set_bytes_per_sec(10000);
    spec.mutable_policer()->mutable_bps_policer()->set_burst_bytes(1000);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_update(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_OK);

    spec.Clear();
    spec.mutable_key_or_handle()->set_copp_type(copp_type);
    spec.mutable_policer()->mutable_bps_policer()->set_bytes_per_sec(10000);
    spec.mutable_policer()->mutable_bps_policer()->set_burst_bytes(1000);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_update(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_OK);

    spec.Clear();
    spec.mutable_key_or_handle()->set_copp_type(copp_type);
    // Update fail due to bps_rate is zero
    spec.mutable_policer()->mutable_bps_policer()->set_bytes_per_sec(0);
    spec.mutable_policer()->mutable_bps_policer()->set_burst_bytes(0);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_update(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_NE(ret, HAL_RET_OK);

    spec.Clear();
    spec.mutable_key_or_handle()->set_copp_type(copp_type);
    // Update to a PPS policer
    spec.mutable_policer()->mutable_pps_policer()->set_packets_per_sec(500);
    spec.mutable_policer()->mutable_pps_policer()->set_burst_packets(20);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_update(spec, &rsp);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_OK);

    get_req.Clear();
    get_rsp.Clear();
    resp_msg.Clear();

    get_req.mutable_key_or_handle()->set_copp_type(copp_type);
    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    ret = hal::copp_get(get_req, &resp_msg);
    get_rsp = resp_msg.response(0);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_OK);
    if (!MessageDifferencer::Equivalent(get_rsp.spec().policer(), spec.policer())) {
        std::string str;
        google::protobuf::TextFormat::PrintToString(get_rsp.spec().policer(), &str);
        std::cout << str << std::endl;
        google::protobuf::TextFormat::PrintToString(spec.policer(), &str);
        std::cout << str << std::endl;
        ASSERT_TRUE(MessageDifferencer::Equivalent(get_rsp.spec().policer(), spec.policer()));
    }
}

// Get all 
TEST_F(copp_test, test4)
{
    hal_ret_t    ret;
    CoppGetRequest get_req;
    CoppGetResponse get_rsp;
    CoppGetResponseMsg resp_msg;

    get_req.Clear();
    get_rsp.Clear();
    resp_msg.Clear();

    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    ret = hal::copp_get(get_req, &resp_msg);
    get_rsp = resp_msg.response(0);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_OK);
    // Make sure that all the available COPP types are returned
    bool get_success[kh::CoppType_ARRAYSIZE] = {0};

    for(int i = 0; i < resp_msg.response_size(); i++) {
        get_rsp = resp_msg.response(i);
        kh::CoppType copp_type = get_rsp.spec().key_or_handle().copp_type();
        get_success[copp_type] = true;
    }

    for (int i = 0; i < kh::CoppType_ARRAYSIZE; i++) {
        ASSERT_EQ(get_success[i], true);
    }
}

// Delete test
TEST_F(copp_test, test5)
{
    hal_ret_t    ret;
    CoppGetRequest get_req;
    CoppGetResponseMsg resp_msg;
    CoppGetResponse get_rsp;
    CoppDeleteRequest del_req;
    CoppDeleteResponse del_rsp;
    CoppType copp_type = kh::COPP_TYPE_FLOW_MISS;

    get_req.Clear();
    get_rsp.Clear();
    resp_msg.Clear();

    get_req.mutable_key_or_handle()->set_copp_type(copp_type);
    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    ret = hal::copp_get(get_req, &resp_msg);
    get_rsp = resp_msg.response(0);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(get_rsp.api_status(), types::API_STATUS_OK);

    // Delete and Get
    del_req.Clear();
    del_rsp.Clear();

    del_req.mutable_key_or_handle()->set_copp_type(copp_type);
    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);
    ret = hal::copp_delete(del_req, &del_rsp);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_OK);

    get_req.Clear();
    get_rsp.Clear();
    resp_msg.Clear();

    get_req.mutable_key_or_handle()->set_copp_type(copp_type);
    hal::hal_cfg_db_open(hal::CFG_OP_READ);
    ret = hal::copp_get(get_req, &resp_msg);
    get_rsp = resp_msg.response(0);
    hal::hal_cfg_db_close();
    ASSERT_EQ(ret, HAL_RET_COPP_NOT_FOUND);
    ASSERT_EQ(get_rsp.api_status(), types::API_STATUS_NOT_FOUND);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
