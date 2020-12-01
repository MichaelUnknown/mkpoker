#include <mkpoker/game/game.hpp>

#include <array>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tgame, game_gb_cards_2p_ctor)
{
    std::array<card, 9> a_cards = {card("Ac"), card("Ad"), card("Kc"), card("Qh"), card("Ts"),
                                   card("7c"), card("6c"), card("9d"), card("9s")};
    std::vector<card> v_cards(a_cards.cbegin(), a_cards.cend());

    const auto g1 = gb_cards_2p(v_cards);
    const auto g2 = gb_cards_2p(v_cards);

    EXPECT_EQ(g1, g2);

    std::vector<card> v_duplicate(a_cards.cbegin(), a_cards.cend() - 1);
    v_duplicate.push_back(card("Ac"));
    EXPECT_THROW(static_cast<void>(gb_cards_2p(v_duplicate)), std::runtime_error);
}

TEST(tgame, game_ctor)
{
    const auto g2 = gamestate<2>(2000);
    const auto g3 = gamestate<3>(2000);
    const auto g4 = gamestate<4>(2000);
    const auto g5 = gamestate<5>(2000);
    const auto g6 = gamestate<6>(2000);

    EXPECT_EQ(g2.active_player(), 1);    // should be BB
    EXPECT_EQ(g3.active_player(), 2);    // should be UTG
    EXPECT_EQ(g4.active_player(), 2);    // etc.
    EXPECT_EQ(g5.active_player(), 2);
    EXPECT_EQ(g6.active_player(), 2);

    EXPECT_EQ(g2.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    EXPECT_EQ(g3.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    EXPECT_EQ(g4.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    EXPECT_EQ(g5.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    EXPECT_EQ(g6.gamestate_v(), gb_gamestate_t::PREFLOP_BET);

    EXPECT_THROW(static_cast<void>(g2.get_payouts()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g3.get_payouts()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g4.get_payouts()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g5.get_payouts()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g6.get_payouts()), std::runtime_error);
}
