#include <mkpoker/holdem/holdem_evaluation.hpp>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tholdem_eval, test)
{
    const auto res = make_he_result(c_full_house, c_rank_eight, c_rank_six, 0);
    const auto cs1 = cardset{"8c8d8h6c6s"};
    const auto cs2 = cardset{"8c8d8h6c6s4c4d"};
    EXPECT_EQ(res, evaluate_safe(cs1));
    EXPECT_EQ(res, evaluate_safe(cs2));
}

TEST(tholdem_eval, holdem_eval_combine)
{
    const auto res = make_he_result(c_full_house, c_rank_two, c_rank_ace, 0);
    EXPECT_EQ(res, evaluate_safe(cardset{"AcAd"}, cardset{"Kd4h2s"}, card{"2c"}, card{"2h"}));
    EXPECT_EQ(evaluate_unsafe(cardset{"AcAdKd4h2s2c2h"}), evaluate_safe(cardset{"AcAd"}, card{"2c"}, cardset{"Kd4h2s"}, card{"2h"}));
}
