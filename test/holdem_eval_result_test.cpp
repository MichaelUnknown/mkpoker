#include <mkpoker/holdem/holdem_evaluation_result.hpp>

#include <stdexcept>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tholdem_eval_result, ctor)
{
    // ctor does not check the input, so not much to be tested here...
    const auto res = holdem_evaluation_result(c_full_house, c_rank_eight, c_rank_six, 0);
    EXPECT_EQ(res.type(), c_full_house);
    EXPECT_EQ(res.major_rank(), rank(rank_t{c_rank_eight}));
    EXPECT_EQ(res.minor_rank(), rank(rank_t{c_rank_six}));
    EXPECT_EQ(res.kickers(), 0);
    EXPECT_EQ(res.m_result, uint32_t(0b0000'0000'1100'1100'1000'0000'0000'0000));
}

TEST(tholdem_eval_result, hevr_type)
{
    const auto v = {c_no_pair, c_one_pair,   c_two_pair,       c_three_of_a_kind, c_straight,
                    c_flush,   c_full_house, c_four_of_a_kind, c_straight_flush};

    for (auto&& e : v)
    {
        EXPECT_EQ(holdem_evaluation_result(e, 0, 0, 0).type(), e);
    }
}

TEST(tholdem_eval_result, hevr_major)
{
    const auto v = {c_rank_two,  c_rank_three, c_rank_four, c_rank_five,  c_rank_six,  c_rank_seven, c_rank_eight,
                    c_rank_nine, c_rank_ten,   c_rank_jack, c_rank_queen, c_rank_king, c_rank_ace};

    for (auto&& e : v)
    {
        EXPECT_EQ(holdem_evaluation_result(c_full_house, e, 0, 0).major_rank(), rank(rank_t{e}));
    }
}

TEST(tholdem_eval_result, hevr_minor)
{
    const auto v = {c_rank_two,  c_rank_three, c_rank_four, c_rank_five,  c_rank_six,  c_rank_seven, c_rank_eight,
                    c_rank_nine, c_rank_ten,   c_rank_jack, c_rank_queen, c_rank_king, c_rank_ace};

    for (auto&& e : v)
    {
        EXPECT_EQ(holdem_evaluation_result(c_full_house, 0, e, 0).minor_rank(), rank(rank_t{e}));
    }
}

TEST(tholdem_eval_result, hevr_kickers)
{
    const std::array<uint8_t, 13> a = {c_rank_two,  c_rank_three, c_rank_four, c_rank_five,  c_rank_six,  c_rank_seven, c_rank_eight,
                                       c_rank_nine, c_rank_ten,   c_rank_jack, c_rank_queen, c_rank_king, c_rank_ace};
    for (auto i1 = 0; i1 < a.size(); ++i1)
    {
        for (auto i2 = i1 + 1; i2 < a.size(); ++i2)
        {
            for (auto i3 = i2 + 1; i3 < a.size(); ++i3)
            {
                for (auto i4 = i3 + 1; i4 < a.size(); ++i4)
                {
                    for (auto i5 = i4 + 1; i5 < a.size(); ++i5)
                    {
                        const uint16_t kickers = uint16_t(1 << a[i1]) | uint16_t(1 << a[i2]) | uint16_t(1 << a[i3]) | uint16_t(1 << a[i4]) |
                                                 uint16_t(1 << a[i5]);
                        EXPECT_EQ(holdem_evaluation_result(c_no_pair, 0, 0, kickers).kickers(), kickers);
                    }
                }
            }
        }
    }
}
