
//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the all policer test cases for athena
///
//----------------------------------------------------------------------------
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/test/api/utils/base.hpp"
#include "nic/apollo/test/athena/api/policer/utils.hpp"
#include "nic/apollo/test/athena/api/include/scale.hpp"

#define MB (1000000)

namespace test {
namespace api {

uint32_t num_create, num_update, num_read, num_delete;
uint32_t num_entries;

enum Types {
    POLICER_BW1=0,
    POLICER_BW2,
    POLICER_PPS,
    POLICER_TYPE_MAX
};

pds_ret_t
(*pds_policer_create)(pds_policer_spec_t *);

pds_ret_t
(*pds_policer_read)(pds_policer_key_t *,
                    pds_policer_info_t *);

pds_ret_t
(*pds_policer_update)(pds_policer_spec_t *);

pds_ret_t
(*pds_policer_delete)(pds_policer_key_t *);

//----------------------------------------------------------------------------
// POLICER test class
//----------------------------------------------------------------------------

class policer_test : public pds_test_base {
protected:
    policer_test() {}
    virtual ~policer_test() {}
    void SetUp() {
        num_create = 0;
        num_update = 0;
        num_read = 0;
        num_delete = 0;
        num_entries = 0;
    }
    void TearDown() {
        display_gtest_stats();
    }
    static void SetUpTestCase() {
        pds_test_base::SetUpTestCase(g_tc_params);
    }
    static void TearDownTestCase() {
        pds_test_base::TearDownTestCase();
    }
public:
    void display_gtest_stats() {
        SDK_TRACE_INFO("GTest Table Stats: Entries:%d", num_entries);
        SDK_TRACE_INFO("Test API Stats: Create=%d Update=%d Read=%d Delete:%d",
                       num_create, num_update, num_read, num_delete);
        return;
    }
};

void
setup_func(Types t)
{
    if (t == POLICER_BW1) {
        pds_policer_create = pds_policer_bw1_create;
        pds_policer_read = pds_policer_bw1_read;
        pds_policer_update = pds_policer_bw1_update;
        pds_policer_delete = pds_policer_bw1_delete;
    } else if (t == POLICER_BW2) {
        pds_policer_create = pds_policer_bw2_create;
        pds_policer_read = pds_policer_bw2_read;
        pds_policer_update = pds_policer_bw2_update;
        pds_policer_delete = pds_policer_bw2_delete;
    } else {
        pds_policer_create = pds_vnic_policer_pps_create;
        pds_policer_read = pds_vnic_policer_pps_read;
        pds_policer_update = pds_vnic_policer_pps_update;
        pds_policer_delete = pds_vnic_policer_pps_delete;
    }
}

pds_ret_t
create_helper(uint32_t index,
              pds_ret_t expret)
{
    pds_ret_t rs;
    for (Types t=Types::POLICER_BW1; t != Types::POLICER_TYPE_MAX; t=(Types)(t+1)) {

        if (t == POLICER_PPS && index >= PDS_VNIC_POLICER_PPS_ID_MAX) {
            continue;
        }
        setup_func(t);

        pds_policer_spec_t spec;
        memset(&spec, 0, sizeof(spec));

        fill_key(&spec.key, index);
        fill_data(&spec.data, 100 * MB, 150 * MB);
        rs = pds_policer_create(&spec);
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
        if (rs == PDS_RET_OK) {
            num_create++;
            num_entries++;
        }
    }
    return rs;
}

pds_ret_t
delete_helper(uint32_t index,
              pds_ret_t expret)
{
    pds_ret_t rs;
    for (Types t=Types::POLICER_BW1; t != Types::POLICER_TYPE_MAX; t=(Types)(t+1)) {

        if (t == POLICER_PPS && index >= PDS_VNIC_POLICER_PPS_ID_MAX) {
            continue;
        }
        setup_func(t);

        pds_policer_key_t key;
        memset(&key, 0, sizeof(key));

        fill_key(&key, index);
        rs = pds_policer_delete(&key);
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
        if (rs == PDS_RET_OK) {
            num_delete++;
            num_entries--;
        }
    }
    return rs;
}

pds_ret_t
update_helper(uint32_t index,
              pds_ret_t expret)
{
    pds_ret_t rs;
    for (Types t=Types::POLICER_BW1; t != Types::POLICER_TYPE_MAX; t=(Types)(t+1)) {

        if (t == POLICER_PPS && index >= PDS_VNIC_POLICER_PPS_ID_MAX) {
            continue;
        }
        setup_func(t);

        pds_policer_spec_t spec;
        memset(&spec, 0, sizeof(spec));

        fill_key(&spec.key, index);
        update_data(&spec.data, 300 * MB, 450 * MB);
        rs = pds_policer_update(&spec);
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
        if (rs == PDS_RET_OK) {
            num_update++;
        }
    }
    return rs;
}

pds_ret_t
read_helper(uint32_t index,
            pds_ret_t expret)
{
    pds_ret_t rs;
    for (Types t=Types::POLICER_BW1; t != Types::POLICER_TYPE_MAX; t=(Types)(t+1)) {

        if (t == POLICER_PPS && index >= PDS_VNIC_POLICER_PPS_ID_MAX) {
            continue;
        }
        setup_func(t);

        pds_policer_key_t key;
        pds_policer_info_t info;
        memset(&key, 0, sizeof(key));
        memset(&info, 0, sizeof(info));

        fill_key(&key, index);
        rs = pds_policer_bw1_read(&key, &info);
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
        if (rs == PDS_RET_OK) {
            num_read++;
        }
        if ((info.spec.data.rate == 100 * MB) &&
            (info.spec.data.burst == 150 * MB))
            rs = PDS_RET_OK;
        else if ((info.spec.data.rate == 300 * MB) &&
                 (info.spec.data.burst == 450 * MB))
            rs = PDS_RET_OK;
        else
            rs = PDS_RET_ERR;
        MHTEST_CHECK_RETURN(rs == expret, PDS_RET_MAX);
    }
    return rs;
}

//----------------------------------------------------------------------------
// Policer test cases implementation
//----------------------------------------------------------------------------

/// \defgroup POLICER Tests
/// @{

/// \brief Policer tests
TEST_F(policer_test, policer_crud) {
    for (Types t=Types::POLICER_BW1; t != Types::POLICER_TYPE_MAX; t=(Types)(t+1)) {

        setup_func(t);

        pds_policer_spec_t spec;
        pds_policer_key_t key;
        pds_policer_info_t info;

        memset(&spec, 0, sizeof(spec));
        memset(&key, 0, sizeof(key));
        memset(&info, 0, sizeof(info));

        fill_key(&spec.key, 1);
        fill_data(&spec.data, 50 * MB, 100 * MB);
        SDK_ASSERT(pds_policer_create(&spec) == PDS_RET_OK);

        memset(&key, 0, sizeof(key));
        fill_key(&key, 1);
        SDK_ASSERT(pds_policer_read(&key, &info) == PDS_RET_OK);
        SDK_ASSERT(info.spec.data.rate == spec.data.rate);
        SDK_ASSERT(info.spec.data.burst == spec.data.burst);

        memset(&spec, 0, sizeof(spec)); 
        fill_key(&spec.key, 1);
        fill_data(&spec.data, 200 * MB, 300 * MB);
        SDK_ASSERT(pds_policer_update(&spec) == PDS_RET_OK);

        memset(&key, 0, sizeof(key));
        fill_key(&key, 1);
        SDK_ASSERT(pds_policer_read(&key, &info) == PDS_RET_OK);
        SDK_ASSERT(info.spec.data.rate == spec.data.rate);
        SDK_ASSERT(info.spec.data.burst == spec.data.burst);

        fill_key(&key, 1);
        SDK_ASSERT(pds_policer_delete(&key) == PDS_RET_OK);

        memset(&key, 0, sizeof(key));
        fill_key(&key, 1);
        pds_ret_t pds_ret = pds_policer_bw1_read(&key, &info);
        SDK_ASSERT(pds_policer_read(&key, &info) == PDS_RET_OK);
        SDK_ASSERT(info.spec.data.rate == 0);
        SDK_ASSERT(info.spec.data.burst == 0);
    }
}

/// \brief Policer scale tests
TEST_F(policer_test, create16)
{
    pds_ret_t rs;
    rs = create_entries(16, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

TEST_F(policer_test, delete1K)
{
    pds_ret_t rs;
    rs = create_entries(512, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = delete_entries(512, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

TEST_F(policer_test, cud_1K)
{
    pds_ret_t rs;
    rs = create_entries(1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = update_entries(1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = delete_entries(1024, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
}

TEST_F(policer_test, crud_1500)
{
    pds_ret_t rs;
    rs = create_entries(1536, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = read_entries(1536, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = update_entries(1536, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
    rs = delete_entries(1536, PDS_RET_OK);
    ASSERT_TRUE(rs == PDS_RET_OK);
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
