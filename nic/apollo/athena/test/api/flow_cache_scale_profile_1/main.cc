//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cinttypes>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/utils/time_profile.hpp"
#include "nic/apollo/api/include/athena/pds_init.h"
#include "nic/apollo/api/include/athena/pds_flow_cache.h"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/athena/test/api/utils/base.hpp"
#include "nic/apollo/athena/test/api/include/ftl_scale.hpp"
#include "ftl_p4pd_mock.hpp"
#include "ftltest_utils.hpp"

extern "C" {
// Function prototypes
pds_ret_t pds_flow_cache_create(pds_cinit_params_t *params);
void pds_flow_cache_delete(void);
void pds_flow_cache_set_core_id(uint32_t core_id);
}

sdk_logger::trace_cb_t g_trace_cb;

namespace test {
namespace api {

//----------------------------------------------------------------------------
// FLOW CACHE SCALE PROFILE test class
//----------------------------------------------------------------------------

class flow_cache_scale_profile_1: public ftl_scale_base {
    trace_level_e trace_level;

protected:
    sdk::utils::time_profile::time_profile_info t_info;

    flow_cache_scale_profile_1() {
        trace_level = g_trace_level;
        g_trace_level = sdk::types::trace_err;
        sdk::utils::time_profile::time_profile_enable = true;
    }

    ~flow_cache_scale_profile_1() {
        g_trace_level = trace_level;
        sdk::utils::time_profile::time_profile_enable = false;
    }

    void SetUp() {
        pds_cinit_params_t init_params;
        ftl_scale_base::SetUp();
        flow_mock_init();
        memset(&init_params, 0, sizeof(init_params));
        init_params.init_mode = PDS_CINIT_MODE_COLD_START;
        pds_flow_cache_create(&init_params);
        pds_flow_cache_set_core_id(2);
        t_info.start();
    }

    void TearDown() {
        t_info.stop();
        SDK_TRACE_INFO("Time to %s.%s : %s",
                       ::testing::UnitTest::GetInstance()->
                       current_test_info()->test_case_name(),
                       ::testing::UnitTest::GetInstance()->
                       current_test_info()->name(),
                       t_info.print_diff().c_str());
        validate_stats();
        pds_flow_cache_delete();
        flow_mock_cleanup();
        ftl_scale_base::TearDown();
    }
public:
    void display_gtest_stats() {
        SDK_TRACE_INFO("GTest Table Stats: Entries:%d", GET_ATOMIC(table_count));
        SDK_TRACE_INFO("Test  API Stats: Insert=%d Update=%d Get=%d Remove:%d Reserve:%d Release:%d",
                       GET_ATOMIC(num_insert), GET_ATOMIC(num_update), GET_ATOMIC(num_get),
                       GET_ATOMIC(num_remove), GET_ATOMIC(num_reserve), GET_ATOMIC(num_release));
        return;
    }

    pds_ret_t validate_stats() {
        pds_flow_stats_t stats = { 0 };

        SDK_ASSERT(pds_flow_cache_stats_get(2, &stats) == PDS_RET_OK);
        display_gtest_stats();
        dump_stats(&stats);

        EXPECT_TRUE(stats.api_insert >= stats.api_remove);
        EXPECT_EQ(table_count, stats.table_entries);
        EXPECT_EQ(num_insert, stats.api_insert + stats.api_insert_duplicate + stats.api_insert_fail);
        EXPECT_EQ(num_remove, stats.api_remove + stats.api_remove_not_found + stats.api_remove_fail);
        EXPECT_EQ(num_update, stats.api_update + stats.api_update_fail);
        // FIXME: For read we use iterate in ftl layer, so this wont work for now
        //EXPECT_EQ(num_get, (stats.api_get + stats.api_get_fail));
        EXPECT_EQ(num_reserve, stats.api_reserve + stats.api_reserve_fail);
        EXPECT_EQ(num_release, stats.api_release + stats.api_release_fail);
        return PDS_RET_OK;
    }

    pds_ret_t create_helper(uint32_t index, pds_ret_t expret) {
        pds_flow_spec_t spec = { 0 };

        fill_key(index, &spec.key);
        fill_data(index, PDS_FLOW_SPEC_INDEX_SESSION, &spec.data);
        auto rs = (pds_ret_t)pds_flow_cache_entry_create(&spec);
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
        if (rs == PDS_RET_OK) {
            create_();
            table_count++;
        }
        return rs;
    }

    pds_ret_t delete_helper(uint32_t index, pds_ret_t expret) {
        pds_flow_key_t key = { 0 };

        fill_key(index, &key);
        auto rs = (pds_ret_t)pds_flow_cache_entry_delete(&key);
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
        if (rs == PDS_RET_OK) {
            delete_();
            table_count--;
        }
        return rs;
    }

    pds_ret_t update_helper(uint32_t index, pds_ret_t expret) {
        pds_flow_spec_t spec = { 0 };

        fill_key(index, &spec.key);
        fill_data(index + 10, PDS_FLOW_SPEC_INDEX_CONNTRACK, &spec.data);
        auto rs = (pds_ret_t)pds_flow_cache_entry_update(&spec);
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
        if (rs == PDS_RET_OK) {
            update_();
        }
        return rs;
    }

    pds_ret_t read_helper(uint32_t index, pds_ret_t expret) {
        pds_flow_key_t key = { 0 };
        pds_flow_info_t info = { 0 };

        fill_key(index, &key);
        auto rs = (pds_ret_t)pds_flow_cache_entry_read(&key, &info);;
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
        if (rs == PDS_RET_OK) {
            read_();
        }
        if ((info.spec.data.index_type == PDS_FLOW_SPEC_INDEX_CONNTRACK) &&
            (info.spec.data.index == index + 10))
            return PDS_RET_OK;
        else if ((info.spec.data.index_type == PDS_FLOW_SPEC_INDEX_SESSION) &&
                 (info.spec.data.index == index))
            return PDS_RET_OK;
        else
            return PDS_RET_ERR;
    }
};

//----------------------------------------------------------------------------
// Flow cache scale profile test cases implementation
//----------------------------------------------------------------------------

/// \defgroup FLOW CACHE SCALE PROFILE Flow cache scale tests
/// @{

TEST_F(flow_cache_scale_profile_1, DISABLED_create1M) {
    pds_ret_t rs;
    rs = create_entries(1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

TEST_F(flow_cache_scale_profile_1, DISABLED_delete1M) {
    pds_ret_t rs;
    rs = create_entries(1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = delete_entries(1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

TEST_F(flow_cache_scale_profile_1, create2M) {
    pds_ret_t rs;
    rs = create_entries(2*1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

TEST_F(flow_cache_scale_profile_1, DISABLED_delete2M) {
    pds_ret_t rs;
    rs = create_entries(2*1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = delete_entries(2*1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

TEST_F(flow_cache_scale_profile_1, DISABLED_create4M) {
    pds_ret_t rs;
    rs = create_entries(4*1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

TEST_F(flow_cache_scale_profile_1, DISABLED_delete4M) {
    pds_ret_t rs;
    rs = create_entries(4*1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = delete_entries(4*1024*1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

/// @}

}    // namespace api
}    // namespace test

/// @private
int
main (int argc, char **argv)
{
    register_trace_cb(sdk_test_logger);
    return api_test_program_run(argc, argv);
}
