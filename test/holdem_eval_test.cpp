#include <mkpoker/holdem/holdem_evaluation.hpp>

//#include <array>
//#include <limits>
//#include <stdexcept>

#include <gtest/gtest.h>

using namespace mkp;

constexpr auto types = {c_no_pair, c_one_pair,   c_two_pair,       c_three_of_a_kind, c_straight,
                        c_flush,   c_full_house, c_four_of_a_kind, c_straight_flush};

constexpr std::array<uint8_t, 13> ranks = {c_rank_two,  c_rank_three, c_rank_four, c_rank_five,  c_rank_six,  c_rank_seven, c_rank_eight,
                                           c_rank_nine, c_rank_ten,   c_rank_jack, c_rank_queen, c_rank_king, c_rank_ace};

TEST(tholdem_eval, test)
{
    const auto res = make_he_result(c_full_house, c_rank_eight, c_rank_six, 0);
    const auto cs1 = cardset{"8c8d8h6c6s"};
    const auto cs2 = cardset{"8c8d8h6c6s4c4d"};
    EXPECT_EQ(res, evaluate_safe(cs1));
    EXPECT_EQ(res, evaluate_safe(cs2));
}
