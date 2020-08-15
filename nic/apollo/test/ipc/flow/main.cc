//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains test cases for configuring VPP flow plugin thru IPC
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/infra/core/msg.h"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/test/api/utils/api_base.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_policy.hpp"

namespace test {
namespace ipc {

//----------------------------------------------------------------------------
// flow plugin IPC config test class
//----------------------------------------------------------------------------

class ipc_flow_test : public ::pds_test_base {
protected:
    ipc_flow_test() {}
    virtual ~ipc_flow_test() {}
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

/// \brief callback function used to validate return value
static void
response_handler_cb(sdk::ipc::ipc_msg_ptr msg, const void *request_cookie) {
    ASSERT_TRUE(msg->length() == sizeof(sdk::sdk_ret_t));
    sdk::sdk_ret_t *ret_val = (sdk::sdk_ret_t *)request_cookie;

    // store the value in a global, so the test can validate
    *ret_val = *(sdk::sdk_ret_t *)msg->data();
}

typedef struct reply_data_s {
    bool is_msg;
    union {
        pds_msg_t msg;
        sdk::sdk_ret_t code;
    };
} reply_data_t;

static void
read_handler_cb(sdk::ipc::ipc_msg_ptr msg, const void *request_cookie) {
    reply_data_t *reply = (reply_data_t *)request_cookie;

    if (msg->length() == sizeof(sdk::sdk_ret_t)) {
        reply->is_msg = false;
        reply->code = *(sdk::sdk_ret_t *)msg->data();
        return;
    }

    if (msg->length() == sizeof(pds_msg_t)) {
        reply->is_msg = true;
        memcpy(&reply->msg, msg->data(), sizeof(pds_msg_t));
        return;
    }

    // unknown response size
    ASSERT_TRUE(0);
}

/// \brief callback function for counting configured instances used with
///        get-all
void
sec_profile_count_cb (const pds_security_profile_info_t *info, void *ctxt)
{
    int *count_ptr = (int *)ctxt;

    (*count_ptr)++;
}

/// \brief read flow plugin config and verify that default timeouts are setup
//         correctly
TEST_F(ipc_flow_test, ipc_flow_cfg_read) {
    if (!apulu()) return;
    int count = 0;
    sdk::sdk_ret_t ret;

    ret = pds_security_profile_read(&sec_profile_count_cb, (void *)&count);

    EXPECT_EQ(ret, sdk::SDK_RET_OK); // read should succeed
    EXPECT_EQ(count, 0); // no instances found
}

/// \brief callback function for retrieving configured instances used with
///        get-all
void
sec_profile_get_cb (const pds_security_profile_info_t *info, void *ctxt)
{
    pds_security_profile_info_t *info_ptr = (pds_security_profile_info_t *)ctxt;

    memcpy(info_ptr, info, sizeof(pds_security_profile_info_t));
}

/// \brief configure flow plugin using a security profile and a CREATE op.
TEST_F(ipc_flow_test, ipc_flow_cfg_create) {
    if (!apulu()) return;

    sdk::sdk_ret_t ret_val;
    pds_security_profile_spec_t spec;

    spec.key = int2pdsobjkey(1234);
    spec.tcp_idle_timeout = 100;
    spec.udp_idle_timeout = 200;
    spec.icmp_idle_timeout = 300;
    spec.other_idle_timeout = 400;

    ret_val = pds_security_profile_create(&spec);
    EXPECT_EQ(ret_val, sdk::SDK_RET_OK);

    // Read data and verify that it is configured value
    pds_security_profile_info_t info;

    ret_val = pds_security_profile_read(&sec_profile_get_cb, (void *)&info);

    EXPECT_EQ(ret_val, sdk::SDK_RET_OK); // read should succeed
    EXPECT_EQ(info.spec.key, int2pdsobjkey(1234));
    EXPECT_EQ(info.spec.tcp_idle_timeout, 100);
    EXPECT_EQ(info.spec.udp_idle_timeout, 200);
    EXPECT_EQ(info.spec.icmp_idle_timeout, 300);
    EXPECT_EQ(info.spec.other_idle_timeout, 400);
}

/// \brief configure flow plugin using a security profile and an UPDATE op.
///        This test uses a singleton configuration message
TEST_F(ipc_flow_test, ipc_flow_update) {
    if (!apulu()) return;
    sdk::sdk_ret_t ret_val;
    pds_security_profile_spec_t spec;

    spec.key = int2pdsobjkey(1234);
    spec.tcp_idle_timeout = 1000;
    spec.udp_idle_timeout = 2000;
    spec.icmp_idle_timeout = 3000;
    spec.other_idle_timeout = 4000;

    ret_val = pds_security_profile_update(&spec);
    EXPECT_EQ(ret_val, sdk::SDK_RET_OK);

    // Read data and verify that it is configured value
    pds_security_profile_info_t info;

    ret_val = pds_security_profile_read(&sec_profile_get_cb, (void *)&info);

    EXPECT_EQ(ret_val, sdk::SDK_RET_OK); // read should succeed
    EXPECT_EQ(info.spec.key, int2pdsobjkey(1234));
    EXPECT_EQ(info.spec.tcp_idle_timeout, 1000);
    EXPECT_EQ(info.spec.udp_idle_timeout, 2000);
    EXPECT_EQ(info.spec.icmp_idle_timeout, 3000);
    EXPECT_EQ(info.spec.other_idle_timeout, 4000);
}

#define TEST_BATCH_SIZE 3

/// \brief Batch configuration for flow plugin. Send multiple CREATE ops, and
///        verify processing
TEST_F(ipc_flow_test, ipc_flow_batch_test) {
    if (!apulu()) return;
    sdk::sdk_ret_t ret_val;

    pds_batch_params_t batch_params = {1234, false, NULL, NULL};
    pds_batch_ctxt_t ctxt = pds_batch_start(&batch_params);

    for (int i = 0; i < TEST_BATCH_SIZE; i++) {
        pds_security_profile_spec_t spec;

        spec.key = int2pdsobjkey(i + 1);
        spec.tcp_idle_timeout = i * 100;
        spec.udp_idle_timeout = i * 200;
        spec.icmp_idle_timeout = i * 300;
        spec.other_idle_timeout = i * 400;
        pds_security_profile_create(&spec, ctxt);
    }
    ret_val = pds_batch_commit(ctxt);
    EXPECT_FALSE(ret_val == sdk::SDK_RET_OK);

    batch_params.epoch = 2345;
    ctxt = pds_batch_start(&batch_params);

    for (int i = 0; i < TEST_BATCH_SIZE; i++) {
        pds_security_profile_spec_t spec;

        spec.key = int2pdsobjkey(1234);
        spec.tcp_idle_timeout = i * 100;
        spec.udp_idle_timeout = i * 200;
        spec.icmp_idle_timeout = i * 300;
        spec.other_idle_timeout = i * 400;
        pds_security_profile_update(&spec, ctxt);
    }
    ret_val = pds_batch_commit(ctxt);
    EXPECT_TRUE(ret_val == sdk::SDK_RET_OK);


    // Read data and verify that it is last configured value
    pds_security_profile_info_t info;

    ret_val = pds_security_profile_read(&sec_profile_get_cb, (void *)&info);

    EXPECT_EQ(ret_val, sdk::SDK_RET_OK); // read should succeed
    EXPECT_EQ(info.spec.key, int2pdsobjkey(1234));
    EXPECT_EQ(info.spec.tcp_idle_timeout, (TEST_BATCH_SIZE - 1) * 100);
    EXPECT_EQ(info.spec.udp_idle_timeout, (TEST_BATCH_SIZE - 1) * 200);
    EXPECT_EQ(info.spec.icmp_idle_timeout, (TEST_BATCH_SIZE - 1) * 300);
    EXPECT_EQ(info.spec.other_idle_timeout, (TEST_BATCH_SIZE - 1) * 400);

}

/// \brief configure flow plugin using a security profile and a DELETE op.
///        This test uses a singleton configuration message. The expectation is
///        the configuration values are restored to default, and the spec
///        contents are ignored
TEST_F(ipc_flow_test, ipc_flow_delete) {
    if (!apulu()) return;
    sdk::sdk_ret_t ret_val;

    pds_obj_key_t key = int2pdsobjkey(1234);

    ret_val = pds_security_profile_delete(&key);
    EXPECT_TRUE(ret_val == sdk::SDK_RET_OK);

    // read back the values and they should be default again
    int count  = 0;

    ret_val = pds_security_profile_read(&sec_profile_count_cb, (void *)&count);

    EXPECT_EQ(ret_val, sdk::SDK_RET_OK); // read should succeed
    EXPECT_EQ(count, 0); // no instances found
}


}    // namespace ipc
}    // namespace test

//----------------------------------------------------------------------------
// Entry point
//----------------------------------------------------------------------------

int
main (int argc, char **argv)
{
    return api_test_program_run(argc, argv);
}
