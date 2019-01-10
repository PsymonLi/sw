#include "lib/bm_allocator/bm_allocator.hpp"
#include <gtest/gtest.h>
#include "nic/hal/test/utils/hal_base_test.hpp"
#include <stdio.h>

TEST(BitmapTest, TestALL)
{
  sdk::lib::Bitmap bm(22);

  ASSERT_FALSE(bm.IsBitSet(21));
  ASSERT_TRUE(bm.IsBitClear(21));
  ASSERT_TRUE(bm.IsBitClear(22));
  ASSERT_FALSE(bm.IsBitSet(22));

  ASSERT_TRUE(bm.AreBitsClear(4, 16));
  bm.SetBits(2, 19);
  ASSERT_TRUE(bm.AreBitsSet(2, 19));
  bm.ResetBits(12, 1);
  ASSERT_FALSE(bm.AreBitsSet(2, 19));
  bm.ResetBits(15, 1);
  ASSERT_TRUE(bm.AreBitsClear(15, 1));
  ASSERT_FALSE(bm.IsBitSet(0));
  ASSERT_FALSE(bm.IsBitSet(1));
  ASSERT_TRUE(bm.IsBitSet(2));
  ASSERT_FALSE(bm.IsBitSet(12));
  ASSERT_TRUE(bm.IsBitSet(11));
  ASSERT_TRUE(bm.IsBitSet(13));
  ASSERT_TRUE(bm.IsBitSet(14));
  ASSERT_FALSE(bm.IsBitSet(15));
  ASSERT_TRUE(bm.IsBitSet(16));
}

TEST(BMAllocatorTest, TestALL)
{
  sdk::lib::BMAllocator bma(4096);

  ASSERT_TRUE(bma.Alloc(20) == 0);
  ASSERT_TRUE(bma.Alloc(20) == 20);
  bma.Free(0, 20);
  ASSERT_TRUE(bma.Alloc(50) == 40);
  ASSERT_TRUE(bma.Alloc(5000) == -1);
  bma.Free(20, 20);
  bma.Free(40, 50);
  ASSERT_TRUE(bma.Alloc(4096) == 0);
  bma.Free(0, 4096);
  ASSERT_TRUE(bma.Alloc(1024) == 0);
  ASSERT_TRUE(bma.Alloc(1024) == 1024);
  ASSERT_TRUE(bma.Alloc(1024) == 2048);
  bma.Free(0, 1024);
  bma.Free(1024, 1024);
  ASSERT_TRUE(bma.Alloc(2048) == 0);
}

TEST(BMAllocatorTest, TestAlign)
{
  sdk::lib::BMAllocator bma(4096);

  ASSERT_EQ(0, bma.Alloc(3, 3));
  ASSERT_EQ(4, bma.Alloc(3, 4));
  ASSERT_EQ(16, bma.Alloc(3, 16));
  ASSERT_EQ(24, bma.Alloc(3, 8));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
