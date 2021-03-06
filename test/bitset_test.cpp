#include <mkpoker/base/cardset.hpp>

#include <stdexcept>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tbit, bit_make_bitset)
{
    EXPECT_EQ(make_bitset(0), uint64_t(1));
    EXPECT_EQ(make_bitset(1, 2, 4, 6), uint64_t(0b0101'0110));
}

TEST(tbit, bit_ross_idx_low)
{
    for (auto i = 0; i < 16; ++i)
    {
        EXPECT_EQ(i, cross_idx_low16(uint16_t(1) << i));
        EXPECT_EQ(i, cross_idx_low16(uint16_t(0xFFFF) << i));
    }

    for (auto i = 0; i < 64; ++i)
    {
        EXPECT_EQ(i, cross_idx_low64(uint64_t(1) << i));
        EXPECT_EQ(i, cross_idx_low64(uint64_t(0xFFFFFFFFFFFFFFFF) << i));
    }

    // gtest seems to ignore the return type on the right side, so static cast it
    EXPECT_EQ(0, static_cast<uint8_t>(cross_idx_low16(0)));
    EXPECT_EQ(0, static_cast<uint8_t>(cross_idx_low64(0)));
    EXPECT_EQ(0, cross_idx_low16(0));
    EXPECT_EQ(0, cross_idx_low64(0));
    EXPECT_EQ(cross_idx_low64(0), 0);
    EXPECT_EQ(cross_idx_low16(0), 0);
}

TEST(tbit, bit_cross_idx_high)
{
    for (auto i = 0; i < 16; ++i)
    {
        EXPECT_EQ(i, cross_idx_high16(uint16_t(1) << i));
        EXPECT_EQ(i, cross_idx_high16(uint16_t(0xFFFF) >> (15 - i)));
    }

    for (auto i = 0; i < 64; ++i)
    {
        EXPECT_EQ(i, cross_idx_high64(uint64_t(1) << i));
        EXPECT_EQ(i, cross_idx_high64(uint64_t(0xFFFF'FFFF'FFFF'FFFF) >> (63 - i)));
    }

    EXPECT_EQ(0, cross_idx_high16(0));
    EXPECT_EQ(0, cross_idx_high64(0));
}