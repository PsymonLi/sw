/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <gtest/gtest.h>

#include "ionic_os.h"
#include "ionic_spp.h"
#include "ionic_spp_util.h"

/* Test functions for ionic_spp_utils */
TEST (IonicSppUtilTest, FwActionUpgrade1)
{

	/* New Fw 1.3.2 and cur 1.3.1 */
	ASSERT_STREQ("upgrade", ionic_fw_decision2(1, 3, 2, 1, 3, 1));
}

TEST (IonicSppUtilTest, FwActionUpgrade2)
{

	/* New Fw 2.2.0 and cur 1.3.1 */
	ASSERT_STREQ("upgrade", ionic_fw_decision2(2, 2, 0, 1, 3, 1));
}

TEST (IonicSppUtilTest, FwVerInvalid)
{
	uint32_t maj = 1, min, rel;
	int ret;

	ret = ionic_encode_fw_version(NULL, &maj, &min, &rel);
	EXPECT_EQ(ret, EINVAL);
	EXPECT_EQ(maj, 0);
	EXPECT_EQ(min, 0);
	EXPECT_EQ(rel, 0);
}

TEST (IonicSppUtilTest, FwVerDev)
{
	uint32_t maj = 1, min, rel;
	int ret;
	char ver[] = "decode";

	ret = ionic_encode_fw_version(ver, &maj, &min, &rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(maj, 0);
	EXPECT_EQ(min, 0);
	EXPECT_EQ(rel, 0);
}

TEST (IonicSppUtilTest, FwVerInValidMaj)
{
	uint32_t maj, min, rel;
	int ret;
	char ver[] = "2";

	ret = ionic_encode_fw_version(ver, &maj, &min, &rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(maj, 2);
	EXPECT_EQ(min, 0);
	EXPECT_EQ(ret, 0);
}

TEST (IonicSppUtilTest, FwVerValidMajMin)
{
	uint32_t maj, min, rel;
	int ret;
	char ver[] = "2.12";

	ret = ionic_encode_fw_version(ver, &maj, &min, &rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(maj, 2);
	EXPECT_EQ(min, 12);
	EXPECT_EQ(rel, 0);
}

TEST (IonicSppUtilTest, FwVerValidMajMinRel)
{
	uint32_t maj, min, rel;
	int ret;
	char buffer[32];
	char ver[] = "2.12.8";

	ret = ionic_encode_fw_version(ver, &maj, &min, &rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(maj, 2);
	EXPECT_EQ(min, 12);
	EXPECT_EQ(rel, 0x2000000);
}

TEST (IonicSppUtilTest, FwVerValidMajMinRelPartial)
{
	uint32_t maj, min, rel;
	int ret;
	char ver[] = "2.12.8-E";

	ret = ionic_encode_fw_version(ver, &maj, &min, &rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(maj, 2);
	EXPECT_EQ(min, 12);
	EXPECT_EQ(rel, 0x2045000);
}

TEST (IonicSppUtilTest, FwVerValidMajMinRelFull)
{
	uint32_t maj, min, rel;
	int ret;
	char ver[] = "2.12.8-E-xxx";

	ret = ionic_encode_fw_version(ver, &maj, &min, &rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(maj, 2);
	EXPECT_EQ(min, 12);
	EXPECT_EQ(rel, 0x2045000);
}

TEST (IonicSppUtilTest, FwDecodeRel1)
{
	uint32_t rel;
	char buffer[32];

	rel = 0x2000000;

	ionic_decode_release(rel, buffer, sizeof(buffer));
	ASSERT_STREQ("8", buffer);
}

TEST (IonicSppUtilTest, FwDecodeRel2)
{
	uint32_t rel;
	char buffer[32];

	rel = 0x2045000;

	ionic_decode_release(rel, buffer, sizeof(buffer));
	ASSERT_STREQ("8-E", buffer);
}

TEST (IonicSppUtilTest, FwDecodeRel3)
{
	uint32_t rel;
	char buffer[32];

	rel = 0x204500F;

	ionic_decode_release(rel, buffer, sizeof(buffer));
	ASSERT_STREQ("8-E-15", buffer);
}

TEST (IonicSppUtilTest, SPPVer2PenVer)
{
	uint32_t rel;
	char buffer[32];


	ionic_spp2pen_verstr("1.3.33837071", buffer, sizeof(buffer));
	ASSERT_STREQ("1.3.8-E-15", buffer);
}

TEST (IonicSppUtilTest, SPPVer2PenVerDev)
{
	uint32_t maj, min, rel;
	char buffer[32], spp_ver[32];
	char ver[] = "1.13.0-136-7-g2db2748ed0-dirty";
	int ret;

	ret = ionic_encode_fw_version(ver, &maj, &min, &rel);
	EXPECT_EQ(ret, 0);

	snprintf(spp_ver, sizeof(spp_ver), "%d.%d.%d", maj, min, rel);
	ASSERT_STREQ("1.13.557063", spp_ver);
	ionic_spp2pen_verstr(spp_ver, buffer, sizeof(buffer));
	ASSERT_STREQ("1.13.0-136-7", buffer);
}

TEST (IonicSppUtilTest, FwVerValidMajMinRelSingleType)
{
	uint32_t maj, min, rel1, rel2;
	int ret;
	char ver1[] = "2.12.8-E-xxx";
	char ver2[] = "2.12.8-EE-xxx";

	ret = ionic_encode_fw_version(ver1, &maj, &min, &rel1);
	EXPECT_EQ(ret, 0);
	ret = ionic_encode_fw_version(ver2, &maj, &min, &rel2);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(rel1, rel2);
}

TEST (IonicSppUtilTest, FwDecUpgrade)
{
	uint32_t new_maj, new_min, new_rel, cur_maj, cur_min, cur_rel;
	int ret;

	ret = ionic_encode_fw_version("2", &new_maj, &new_min, &new_rel);
	EXPECT_EQ(ret, 0);
	ret = ionic_encode_fw_version("1.3", &cur_maj, &cur_min, &cur_rel);
	EXPECT_EQ(ret, 0);

	ASSERT_STREQ("upgrade", ionic_fw_decision2(new_maj, new_min, new_rel, cur_maj, cur_min, cur_rel));
}

TEST (IonicSppUtilTest, FwDecDowngrade)
{
	uint32_t new_maj, new_min, new_rel, cur_maj, cur_min, cur_rel;
	int ret;

	ret = ionic_encode_fw_version("1.3.0-E-10", &new_maj, &new_min, &new_rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(new_maj, 1);
	EXPECT_EQ(new_min, 3);
	EXPECT_EQ(new_rel, 0x4500A);

	ret = ionic_encode_fw_version("1.3.0-E-15", &cur_maj, &cur_min, &cur_rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(cur_maj, 1);
	EXPECT_EQ(cur_min, 3);
	EXPECT_EQ(cur_rel, 0x4500F);

	ASSERT_STREQ("downgrade", ionic_fw_decision2(new_maj, new_min, new_rel, cur_maj, cur_min, cur_rel));
}

TEST (IonicSppUtilTest, FwDecUpgradeFullVer)
{
	uint32_t new_maj, new_min, new_rel, cur_maj, cur_min, cur_rel;
	int ret;
	char curVer[] = "1.3.0-E-20";
	char newVer[] = "1.3.1-E-15";

	ret = ionic_encode_fw_version(curVer, &cur_maj, &cur_min, &cur_rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(cur_maj, 1);
	EXPECT_EQ(cur_min, 3);
	EXPECT_EQ(cur_rel, 0x45014);

	ret = ionic_encode_fw_version(newVer, &new_maj, &new_min, &new_rel);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(new_maj, 1);
	EXPECT_EQ(new_min, 3);
	EXPECT_EQ(new_rel, 0x44500F);

	ASSERT_STREQ("upgrade", ionic_fw_decision(newVer, curVer));
}

TEST (IonicSppUtilTest, DecRewrite1)
{
	char curVer[] = "1.3.1-E-15";
	char newVer[] = "1.3.1-E-15";

	ASSERT_STREQ("rewrite", ionic_fw_decision(newVer, curVer));
}

TEST (IonicSppUtilTest, DecRewrite2)
{
	char curVer[] = "1.3.1-E-15";
	char newVer[] = "1.3.1-EE-15";

	ASSERT_STREQ("rewrite", ionic_fw_decision(newVer, curVer));
}

TEST (IonicSppUtilTest, DecDowngrade)
{
	char curVer[] = "1.3.1-E-15";
	char newVer[] = "1.3.1-C-15";

	ASSERT_STREQ("downgrade", ionic_fw_decision(newVer, curVer));
}

TEST (IonicSppUtilTest, DecUpgrade)
{
	char curVer[] = "1.3.1-E-15";
	char newVer[] = "1.3.2-E-10";

	ASSERT_STREQ("upgrade", ionic_fw_decision(newVer, curVer));
}

TEST (IonicSppUtilTest, DescNameValid)
{
	struct ionic ionic;
	int ret;
	char buf[64] = "PRE";

	strncpy(ionic.asicType, "CAPRI", sizeof(ionic.asicType));
	ionic.subDevId = 0x4008;
	ret = ionic_desc_name(&ionic, buf, sizeof(buf));
	EXPECT_EQ(ret, 0);
	ASSERT_STREQ(buf, "PEN_DSC_CAPRI_SFP28_10-25G_NIC");
}


TEST (IonicSppUtilTest, DescNameInvalid)
{
	struct ionic ionic;
	int ret;
	char buf[64] = "NONE";

	strncpy(ionic.asicType, "CAPRI", sizeof(ionic.asicType));
	ionic.subDevId = 0x4003;
	ret = ionic_desc_name(&ionic, buf, sizeof(buf));
	EXPECT_EQ(ret, EINVAL);
	ASSERT_STREQ(buf, "PEN_DSC_CAPRI_0x4003_NIC");
}

