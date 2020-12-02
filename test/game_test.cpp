#include <mkpoker/game/game.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tgame, game_gamecards_ctor)
{
    const auto cards = make_array<card, 17>([](const uint8_t i) { return card(i); });
    const auto same_cards = make_array<card, 17>([](auto) { return card("Ac"); });
    std::vector<card> v_cards(cards.cbegin(), cards.cend());

    EXPECT_THROW(static_cast<void>(gb_cards<2>(cards)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gb_cards<3>(cards)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gb_cards<4>(cards)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gb_cards<5>(cards)), std::runtime_error);

    const auto g1a = gb_cards<2>(std::span<const card>(cards.data(), 9));
    const auto g1b = gb_cards<2>(std::span<const card>(v_cards.data(), 9));
    const auto g6a = gb_cards<6>(cards);
    const auto g6b = gb_cards<6>(v_cards);

    EXPECT_EQ(g1a, g1b);
    EXPECT_EQ(g6a, g6b);
    EXPECT_THROW(static_cast<void>(gb_cards<2>(same_cards)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gb_cards<6>(same_cards)), std::runtime_error);
}

TEST(tgame, game_gamestate_ctor)
{
    EXPECT_THROW(static_cast<void>(gamestate<2>(999)), std::runtime_error);

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

TEST(tgame, game_gamestate_execute_action)
{
    auto start = gamestate<2>(3000);
    EXPECT_GT(start.get_possible_actions().size(), 0);
    EXPECT_THROW(start.execute_action(player_action_t{1499, gb_action_t::RAISE, start.active_player_v()}), std::runtime_error);

    start.execute_action(player_action_t{1500, gb_action_t::RAISE, start.active_player_v()});
    EXPECT_EQ(start.active_player(), 0);
    EXPECT_EQ(start.in_terminal_state(), false);
    EXPECT_EQ(start.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
}