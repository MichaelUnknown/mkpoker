#include <mkpoker/holdem/holdem_evaluation.hpp>
#include <mkpoker/holdem/holdem_result.hpp>
#include <mkpoker/util/mtp.hpp>

#include <array>
#include <stdexcept>

#include <gtest/gtest.h>

using namespace mkp;

constexpr auto types = {c_no_pair, c_one_pair,   c_two_pair,       c_three_of_a_kind, c_straight,
                        c_flush,   c_full_house, c_four_of_a_kind, c_straight_flush};

constexpr std::array<uint8_t, 13> ranks = {c_rank_two,  c_rank_three, c_rank_four, c_rank_five,  c_rank_six,  c_rank_seven, c_rank_eight,
                                           c_rank_nine, c_rank_ten,   c_rank_jack, c_rank_queen, c_rank_king, c_rank_ace};

TEST(tholdem_eval_result, ctor)
{
    // ctor does not check the input, so not much to be tested here...
    const auto res = holdem_result(c_full_house, c_rank_eight, c_rank_six, 0);
    EXPECT_EQ(res.type(), c_full_house);
    EXPECT_EQ(res.major_rank(), rank(rank_t{c_rank_eight}));
    EXPECT_EQ(res.minor_rank(), rank(rank_t{c_rank_six}));
    EXPECT_EQ(res.kickers(), 0);
    EXPECT_EQ(res.as_bitset(), uint32_t(0b0000'0000'1100'1100'1000'0000'0000'0000));
}

TEST(tholdem_eval_result, hevr_type)
{
    for (auto&& e : types)
    {
        EXPECT_EQ(holdem_result(e, 0, 0, 0).type(), e);
    }
}

TEST(tholdem_eval_result, hevr_major)
{
    for (auto&& e : ranks)
    {
        EXPECT_EQ(holdem_result(c_full_house, e, 0, 0).major_rank(), rank(rank_t{e}));
    }
}

TEST(tholdem_eval_result, hevr_minor)
{
    for (auto&& e : ranks)
    {
        EXPECT_EQ(holdem_result(c_full_house, 0, e, 0).minor_rank(), rank(rank_t{e}));
    }
}

TEST(tholdem_eval_result, hevr_kickers)
{
    static_assert(ranks.size() == c_num_ranks);

    for (auto i1 = 0; i1 < c_num_ranks; ++i1)
    {
        for (auto i2 = i1 + 1; i2 < c_num_ranks; ++i2)
        {
            for (auto i3 = i2 + 1; i3 < c_num_ranks; ++i3)
            {
                for (auto i4 = i3 + 1; i4 < c_num_ranks; ++i4)
                {
                    for (auto i5 = i4 + 1; i5 < c_num_ranks; ++i5)
                    {
                        const uint16_t kickers = uint16_t(1 << ranks[i1]) | uint16_t(1 << ranks[i2]) | uint16_t(1 << ranks[i3]) |
                                                 uint16_t(1 << ranks[i4]) | uint16_t(1 << ranks[i5]);
                        EXPECT_EQ(holdem_result(c_no_pair, 0, 0, kickers).kickers(), kickers);
                    }
                }
            }
        }
    }
}

TEST(tholdem_eval_result, hevr_make_hev_result)
{
    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_no_pair, 0, 0, 0b0010'1111)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_no_pair, c_rank_eight, 0, 0)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_no_pair, 0, c_rank_eight, 0)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_no_pair, 0, 0, 63)), std::runtime_error);

    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_one_pair, c_rank_eight, 0, 0b0111)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_one_pair, 13, 0, 0b0111)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_one_pair, c_rank_eight, c_rank_seven, 0b0111)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_one_pair, c_rank_eight, 0, 0b1111)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_one_pair, c_rank_eight, 0, uint16_t(1) << c_rank_eight)), std::runtime_error);

    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_two_pair, c_rank_eight, c_rank_five, 0b0001)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_two_pair, 13, c_rank_five, 0b0001)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_two_pair, c_rank_eight, 13, 0b0001)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_two_pair, c_rank_eight, c_rank_five, 0b0011)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_two_pair, c_rank_eight, c_rank_eight, 0b0001)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_two_pair, c_rank_five, c_rank_eight, 0b0001)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_two_pair, c_rank_eight, c_rank_five, uint16_t(1) << c_rank_eight)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_two_pair, c_rank_eight, c_rank_five, uint16_t(1) << c_rank_five)), std::runtime_error);

    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_three_of_a_kind, c_rank_ten, 0, 0b0011)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_three_of_a_kind, 13, 0, 0b0011)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_three_of_a_kind, c_rank_ten, 0, 0b0111)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_three_of_a_kind, c_rank_ten, c_rank_seven, 0b0011)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_three_of_a_kind, c_rank_ten, 0, uint16_t(1) << c_rank_ten)), std::runtime_error);

    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_straight, c_rank_six, 0, 0)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_straight, 13, 0, 0)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_straight, c_rank_six, c_rank_three, 0)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_straight, c_rank_six, 0, 0b0001)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_straight, c_rank_four, 0, 0)), std::runtime_error);

    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_flush, 0, 0, 0b0010'1111)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_flush, c_rank_five, 0, 0b0010'1111)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_flush, 0, c_rank_five, 0b0010'1111)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_flush, 0, 0, 0b0110'1111)), std::runtime_error);

    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_full_house, c_rank_king, c_rank_jack, 0)));
    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_full_house, c_rank_jack, c_rank_king, 0)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_full_house, c_rank_king, c_rank_king, 0)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_full_house, c_rank_king, c_rank_jack, 0b0001)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_full_house, 13, c_rank_jack, 0)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_full_house, c_rank_king, 13, 0)), std::runtime_error);

    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_four_of_a_kind, c_rank_jack, 0, 0b0001)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_four_of_a_kind, 13, 0, 0b0001)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_four_of_a_kind, c_rank_jack, c_rank_three, 0b0001)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_four_of_a_kind, c_rank_jack, 0, 0b0011)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_four_of_a_kind, c_rank_jack, 0, uint16_t(1) << c_rank_jack)), std::runtime_error);

    EXPECT_NO_THROW(static_cast<void>(make_he_result(c_straight_flush, c_rank_jack, 0, 0)));
    EXPECT_THROW(static_cast<void>(make_he_result(c_straight_flush, 13, 0, 0)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_straight_flush, c_rank_jack, c_rank_three, 0)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(make_he_result(c_straight_flush, c_rank_jack, 0, 0b0001)), std::runtime_error);

    EXPECT_THROW(static_cast<void>(make_he_result(c_straight_flush + 1, 0, 0, 0)), std::runtime_error);

    // check for equality
    for (auto&& ty : types)
    {
        for (auto&& ma : ranks)
        {
            for (auto&& mi : ranks)
            {
                // test for the first 5 kickers only where the lowest 5 kicker bitset is 0b0010'1111
                for (uint16_t ki = 0; ki < 0b0010'1111; ++ki)
                {
                    try
                    {
                        const auto res = make_he_result(ty, ma, mi, ki);
                        EXPECT_EQ(res, holdem_result(ty, ma, mi, ki));
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
    }
}

TEST(tholdem_eval_result, print_all_hev_results)
{
    // expect a throw for invalid results
    EXPECT_THROW(static_cast<void>(holdem_result(c_straight_flush + 1, c_rank_ace, 0, 0).str()), std::runtime_error);
    // todo: add all the invalid cases
    // checking at construction might not be suitable if we ever want to evaluate partial hands,
    // like evaluating 4 cards with trips would trips with one kicker when there should be two kickers

    for (uint8_t i = 0; i < 52; ++i)
    {
        for (uint8_t j = i + 1; j < 52; ++j)
        {
            for (uint8_t k = j + 1; k < 52; ++k)
            {
                for (uint8_t l = k + 1; l < 52; ++l)
                {
                    for (uint8_t m = l + 1; m < 52; ++m)
                    {
                        EXPECT_NO_THROW(const auto her = evaluate_safe(mkp::cardset(mkp::make_bitset(i, j, k, l, m))););
                        EXPECT_NO_THROW(std::string str = evaluate_safe(mkp::cardset(mkp::make_bitset(i, j, k, l, m))).str(););
                    }
                }
            }
        }
    }
}
