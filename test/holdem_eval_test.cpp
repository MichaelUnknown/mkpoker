#include <mkpoker/holdem/holdem_evaluation.hpp>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tholdem_eval, holdem_eval_unsafe_5_6_cards)
{
    for (auto i1 = 0; i1 < c_deck_size; ++i1)
    {
        for (auto i2 = i1 + 1; i2 < c_deck_size; ++i2)
        {
            for (auto i3 = i2 + 1; i3 < c_deck_size; ++i3)
            {
                for (auto i4 = i3 + 1; i4 < c_deck_size; ++i4)
                {
                    for (auto i5 = i4 + 1; i5 < c_deck_size; ++i5)
                    {
                        const cardset cs5{make_bitset(i1, i2, i3, i4, i5)};
                        EXPECT_NO_THROW(static_cast<void>(evaluate_safe(cs5)));
                        EXPECT_EQ(evaluate_unsafe(cs5), evaluate_safe(cs5));

                        for (auto i6 = i5 + 1; i6 < c_deck_size; ++i6)
                        {
                            const cardset cs6{make_bitset(i1, i2, i3, i4, i5, i6)};
                            EXPECT_NO_THROW(static_cast<void>(evaluate_safe(cs6)));
                            EXPECT_EQ(evaluate_unsafe(cs6), evaluate_safe(cs6));
                        }
                    }
                }
            }
        }
    }
}

TEST(tholdem_eval, holdem_eval_unsafe_7_cards)
{
    for (auto i1 = 0; i1 < c_deck_size; ++i1)
    {
        for (auto i2 = i1 + 1; i2 < c_deck_size; ++i2)
        {
            for (auto i3 = i2 + 1; i3 < c_deck_size; ++i3)
            {
                for (auto i4 = i3 + 1; i4 < c_deck_size; ++i4)
                {
                    for (auto i5 = i4 + 1; i5 < c_deck_size; ++i5)
                    {
                        for (auto i6 = i5 + 1; i6 < c_deck_size; ++i6)
                        {
                            for (auto i7 = i6 + 1; i7 < c_deck_size; ++i7)
                            {
                                const cardset cs{make_bitset(i1, i2, i3, i4, i5, i6, i7)};
                                EXPECT_NO_THROW(static_cast<void>(evaluate_safe(cs)));
                                EXPECT_EQ(evaluate_unsafe(cs), evaluate_safe(cs));
                            }
                        }
                    }
                }
            }
        }
    }
}

TEST(tholdem_eval, holdem_eval_safe)
{
    const auto res = make_he_result(c_full_house, c_rank_eight, c_rank_six, 0);
    const auto cs1 = cardset{"8c8d8h6c6s"};
    const auto cs2 = cardset{"8c8d8h6c6s4c4d"};
    EXPECT_EQ(res, evaluate_safe(cs1));
    EXPECT_EQ(res, evaluate_safe(cs2));
    EXPECT_THROW(static_cast<void>(evaluate_safe(cardset{"AcKcQhJsTc9d8h7s"})), std::runtime_error);
    EXPECT_THROW(static_cast<void>(evaluate_safe(cardset{"AcKcQhJs"})), std::runtime_error);
}

TEST(tholdem_eval, holdem_eval_combine)
{
    const auto res = make_he_result(c_full_house, c_rank_two, c_rank_ace, 0);
    EXPECT_EQ(res, evaluate_safe(cardset{"AcAd"}, cardset{"Kd4h2s"}, card{"2c"}, card{"2h"}));
    EXPECT_EQ(evaluate_unsafe(cardset{"AcAdKd4h2s2c2h"}), evaluate_safe(cardset{"AcAd"}, card{"2c"}, cardset{"Kd4h2s"}, card{"2h"}));
}
