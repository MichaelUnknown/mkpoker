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
    const auto same_cards = make_array<card, 17>([](const uint8_t i) { return i < 2 ? card("Ac") : card(i); });
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
    EXPECT_EQ(g3.active_player(), 2);    // should be UTG for the rest
    EXPECT_EQ(g4.active_player(), 2);    //
    EXPECT_EQ(g5.active_player(), 2);
    EXPECT_EQ(g6.active_player(), 2);

    EXPECT_EQ(g2.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    EXPECT_EQ(g3.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    EXPECT_EQ(g4.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    EXPECT_EQ(g5.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    EXPECT_EQ(g6.gamestate_v(), gb_gamestate_t::PREFLOP_BET);

    EXPECT_THROW(static_cast<void>(g2.effective_payouts()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g3.effective_payouts()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g4.effective_payouts()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g5.effective_payouts()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g6.effective_payouts()), std::runtime_error);
}

TEST(tgame, game_gamestate_execute_action)
{
    auto game1 = gamestate<2>(3000);
    EXPECT_GT(game1.get_possible_actions().size(), 0);
#if !defined(NDEBUG)
    EXPECT_THROW(game1.execute_action(player_action_t{1500, gb_action_t::RAISE, gb_pos_t::SB}), std::runtime_error);
    EXPECT_THROW(game1.execute_action(player_action_t{1499, gb_action_t::RAISE, game1.active_player_v()}), std::runtime_error);
#endif

    game1.execute_action(player_action_t{1500, gb_action_t::RAISE, game1.active_player_v()});
    EXPECT_EQ(game1.active_player(), 0);
    EXPECT_EQ(game1.in_terminal_state(), false);
    EXPECT_EQ(game1.gamestate_v(), gb_gamestate_t::PREFLOP_BET);

    game1.execute_action(player_action_t{2000, gb_action_t::ALLIN, game1.active_player_v()});
    game1.execute_action(player_action_t{1000, gb_action_t::CALL, game1.active_player_v()});
    EXPECT_EQ(game1.in_terminal_state(), true);
    EXPECT_EQ(game1.is_showdown(), true);
#if !defined(NDEBUG)
    EXPECT_THROW(static_cast<void>(game1.effective_payouts()), std::runtime_error);
#endif
    EXPECT_EQ(game1.get_possible_actions().size(), 0);

    auto game2 = gamestate<3>(10000);
    // state should be preflop bet
    EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
    // UTG bets 2BB (3000 total)
    game2.execute_action(player_action_t{3000, gb_action_t::RAISE, game2.active_player_v()});
    // SB folds
    game2.execute_action(player_action_t{0, gb_action_t::FOLD, game2.active_player_v()});
    // BB calls (2000 total)
    game2.execute_action(player_action_t{2000, gb_action_t::CALL, game2.active_player_v()});
    // state should be flop bet
    EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::FLOP_BET);
    // active player should be BB
    EXPECT_EQ(game2.active_player(), 1);
    // BB checks
    game2.execute_action(player_action_t{0, gb_action_t::CHECK, game2.active_player_v()});
    // UTG bets 1BB (1000 total)
    game2.execute_action(player_action_t{1000, gb_action_t::RAISE, game2.active_player_v()});
    // BB calls (1000 total)
    game2.execute_action(player_action_t{1000, gb_action_t::CALL, game2.active_player_v()});
    // state should be turn bet
    EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::TURN_BET);
    // active player should be BB
    EXPECT_EQ(game2.active_player(), 1);
    // BB checks
    game2.execute_action(player_action_t{0, gb_action_t::CHECK, game2.active_player_v()});
    // UTG checks
    game2.execute_action(player_action_t{0, gb_action_t::CHECK, game2.active_player_v()});
    // state should be river bet
    EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::RIVER_BET);
    // active player should be BB
    EXPECT_EQ(game2.active_player(), 1);
    // BB goes all in (6000 total)
    game2.execute_action(player_action_t{6000, gb_action_t::ALLIN, game2.active_player_v()});
    // UTG folds
    game2.execute_action(player_action_t{0, gb_action_t::FOLD, game2.active_player_v()});
    // state should be game finished
    EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::GAME_FIN);
    // effective payouts should be {-500,+4500,-4000}
    EXPECT_EQ(game2.effective_payouts(), (std::array<int, 3>{-500, 4500, -4000}));
}