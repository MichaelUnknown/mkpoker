#include <mkpoker/game/game.hpp>

#include <array>
#include <cstdint>
#include <functional>    // test clang
#include <span>
#include <stdexcept>
#include <utility>    // test clang
#include <vector>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tgame, game_gamecards_ctor_arr)
{
    // ctor with two arrays
    const auto board = make_array<card, 5>([](const uint8_t i) { return card(i); });
    const auto board_var = make_array<card, 5>([](const uint8_t i) { return card(4 - i); });
    const auto hands2 = make_array<hand_2c, 2>([](const uint8_t i) { return hand_2c(card(5 + 2 * i), card(5 + 2 * i + 1)); });
    const auto hands2_dup = make_array<hand_2c, 2>([](const uint8_t i) { return hand_2c(card(2 * i), card(2 * i + 1)); });
    const auto hands6 = make_array<hand_2c, 6>([](const uint8_t i) { return hand_2c(card(5 + 2 * i), card(5 + 2 * i + 1)); });
    const auto hands6_dup = make_array<hand_2c, 6>([](const uint8_t i) { return hand_2c(card(2 * i), card(2 * i + 1)); });
    const auto c2a = gamecards<2>(board, hands2);
    const auto c2b = gamecards<2>(board_var, hands2);
    const auto c6a = gamecards<6>(board, hands6);
    const auto c6b = gamecards<6>(board_var, hands6);
    EXPECT_THROW(static_cast<void>(gamecards<2>(board, hands2_dup)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamecards<6>(board, hands6_dup)), std::runtime_error);
    EXPECT_EQ(c2a, c2a);
    EXPECT_EQ(c2b, c2b);
    EXPECT_NE(c2a, c2b);
    EXPECT_EQ(c6a, c6a);
    EXPECT_EQ(c6b, c6b);
    EXPECT_NE(c6a, c6b);
}

TEST(tgame, game_gamecards_ctor_span)
{
    // ctor with span of cards
    const auto cards = make_array<card, 17>([](const uint8_t i) { return card(i); });
    const auto same_cards = make_array<card, 17>([](const uint8_t i) { return i < 2 ? card("2c") : card(i); });
    std::vector<card> v_cards(cards.cbegin(), cards.cend());

    EXPECT_THROW(static_cast<void>(gamecards<2>(cards)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamecards<3>(cards)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamecards<4>(cards)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamecards<5>(cards)), std::runtime_error);

    const auto g2a1 = gamecards<2>(std::span<const card>(cards.data(), 9));
    const auto g2a2 = gamecards<2>(std::span<const card>(v_cards.data(), 9));
    const auto g2b = gamecards<2>(std::span<const card>(cards.data() + 1, 9));
    const auto g6a1 = gamecards<6>(cards);
    const auto g6a2 = gamecards<6>(v_cards);

    EXPECT_EQ(g2a1, g2a2);
    EXPECT_EQ(g6a1, g6a2);
    EXPECT_NE(g2a1, g2b);
    EXPECT_NE(g2a2, g2b);
    EXPECT_THROW(static_cast<void>(gamecards<2>(same_cards)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamecards<6>(same_cards)), std::runtime_error);
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
    //
    // test exceptions
    //
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

    //
    // test different actions
    //
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

    //
    // test showdown
    //
    auto game3 = gamestate<3>(10000);
    // everyone calls
    game3.execute_action(player_action_t{1000, gb_action_t::CALL, game3.active_player_v()});
    game3.execute_action(player_action_t{500, gb_action_t::CALL, game3.active_player_v()});
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    // state should be flop bet
    EXPECT_EQ(game3.gamestate_v(), gb_gamestate_t::FLOP_BET);
    // everyone calls
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    // state should be tirm bet
    EXPECT_EQ(game3.gamestate_v(), gb_gamestate_t::TURN_BET);
    // everyone calls
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    // state should be river bet
    EXPECT_EQ(game3.gamestate_v(), gb_gamestate_t::RIVER_BET);
    // everyone calls
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    game3.execute_action(player_action_t{0, gb_action_t::CHECK, game3.active_player_v()});
    // state should be game finished
    EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::GAME_FIN);
}

template <std::size_t N>
constexpr auto make_games_array() -> typename std::array<gamestate<N>, 3>
{
    return std::array<gamestate<N>, 3>{gamestate<N>(3000), gamestate<N>(3000), gamestate<N>(4000)};
}

TEST(tgame, game_gamestate_comp)
{
    auto games2 = make_games_array<2>();
    auto games3 = make_games_array<3>();
    auto games4 = make_games_array<4>();
    auto games5 = make_games_array<5>();
    auto games6 = make_games_array<6>();

    EXPECT_EQ(games2[0], games2[0]);
    EXPECT_EQ(games2[0], games2[1]);
    EXPECT_NE(games2[0], games2[2]);

    EXPECT_EQ(games3[0], games3[0]);
    EXPECT_EQ(games3[0], games3[1]);
    EXPECT_NE(games3[0], games3[2]);

    EXPECT_EQ(games4[0], games4[0]);
    EXPECT_EQ(games4[0], games4[1]);
    EXPECT_NE(games4[0], games4[2]);

    EXPECT_EQ(games5[0], games5[0]);
    EXPECT_EQ(games5[0], games5[1]);
    EXPECT_NE(games5[0], games5[2]);

    EXPECT_EQ(games6[0], games6[0]);
    EXPECT_EQ(games6[0], games6[1]);
    EXPECT_NE(games6[0], games6[2]);
}