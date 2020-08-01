/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <gtest/gtest.h>

#include "ionic_spp.h"
#include "ionic_spp_util.h"

int
main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

/*
 * Test for invalid log file.
 */
TEST (IonicSppTest, GetDebugInfoInvalid)
{
	int ret;

	ret = dsc_get_debug_info((spp_char_t *)"/non/existing/path");
	EXPECT_EQ(ret, HPE_SPP_LIBRARY_DEP_FAILED);
}

TEST (IonicSppTest, End2End1Thread)
{
	std::string fw_path = "./fw_pacakge";
	std::string dis_file = ".gtest_discovery.xml";
	std::string log_file = std::tmpnam(nullptr);;
	int ret;

	ret = ionic_test_discovery(log_file.c_str(), fw_path.c_str(), dis_file.c_str());
	EXPECT_EQ(ret, HPE_SPP_HW_ACCESS);
	ret = ionic_test_update(log_file.c_str(), fw_path.c_str(), dis_file.c_str(), false);
	EXPECT_EQ(ret, HPE_SPP_DIS_MISSING);
}

