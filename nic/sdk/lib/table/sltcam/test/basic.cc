//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
#include "base.hpp"
#define BASIC_TEST_COUNT TABLE_SIZE

class basic: public SltcamGtestBase {
};

TEST_F(basic, insert)
{
    sdk_ret_t rs;
    rs = Insert(BASIC_TEST_COUNT, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}

TEST_F(basic, insert_get)
{
    sdk_ret_t rs;
    rs = Insert(BASIC_TEST_COUNT, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = GetAllCached(sdk::SDK_RET_OK, false);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}

TEST_F(basic, update)
{
    sdk_ret_t rs;
    rs = Insert(BASIC_TEST_COUNT, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = UpdateAllCached(sdk::SDK_RET_OK, false);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}

TEST_F(basic, repeated_update)
{
    sdk_ret_t rs;
    rs = Insert(BASIC_TEST_COUNT, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    for (uint32_t j = 0; j < 5; j++) {
        rs = UpdateAllCached(sdk::SDK_RET_OK, false);
        ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    }
}

TEST_F(basic, insert_duplicate) 
{
    sdk_ret_t rs;
    rs = Insert(BASIC_TEST_COUNT, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = InsertAllCached(sdk::SDK_RET_ENTRY_EXISTS, false);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}

TEST_F(basic, insert_remove) 
{
    sdk_ret_t rs;
    rs = Insert(BASIC_TEST_COUNT, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = RemoveAllCached(sdk::SDK_RET_OK, false);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}

TEST_F(basic, insert_update_remove) 
{
    sdk_ret_t rs;
    rs = Insert(BASIC_TEST_COUNT, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = UpdateAllCached(sdk::SDK_RET_OK, false);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = RemoveAllCached(sdk::SDK_RET_OK, false);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}

TEST_F(basic, duplicate_remove) 
{
    sdk_ret_t rs;
    rs = Insert(BASIC_TEST_COUNT, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = RemoveAllCached(sdk::SDK_RET_OK, false);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = RemoveAllCached(sdk::SDK_RET_ENTRY_NOT_FOUND, false);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}

TEST_F(basic, remove_not_found) 
{
    sdk_ret_t rs;
    rs = Remove(BASIC_TEST_COUNT, sdk::SDK_RET_ENTRY_NOT_FOUND);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}

TEST_F(basic, iterate)
{
    sdk_ret_t rs;
    rs = Insert(64, sdk::SDK_RET_OK);
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
    rs = Iterate();
    ASSERT_TRUE(rs == sdk::SDK_RET_OK);
}
