/*

Copyright (C) Michael Knörzer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <mkpoker/game/game.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tgame, game_to_string)
{
    EXPECT_EQ(to_string(gb_gamestate_t::PREFLOP_BET), "PREFLOP_BET");
    EXPECT_EQ(to_string(gb_gamestate_t::FLOP_BET), "FLOP_BET");
    EXPECT_EQ(to_string(gb_gamestate_t::TURN_BET), "TURN_BET");
    EXPECT_EQ(to_string(gb_gamestate_t::RIVER_BET), "RIVER_BET");
    EXPECT_EQ(to_string(gb_gamestate_t::GAME_FIN), "GAME_FIN");
    EXPECT_THROW(static_cast<void>(to_string(static_cast<gb_gamestate_t>(5))), std::runtime_error);

    EXPECT_EQ(to_string(gb_playerstate_t::ALIVE), "ALIVE");
    EXPECT_EQ(to_string(gb_playerstate_t::OUT), "OUT");
    EXPECT_EQ(to_string(gb_playerstate_t::ALIVE), "ALIVE");
    EXPECT_EQ(to_string(gb_playerstate_t::ALLIN), "ALLIN");
    EXPECT_THROW(static_cast<void>(to_string(static_cast<gb_playerstate_t>(4))), std::runtime_error);

    EXPECT_EQ(to_string(gb_action_t::FOLD), "FOLD");
    EXPECT_EQ(to_string(gb_action_t::CHECK), "CHECK");
    EXPECT_EQ(to_string(gb_action_t::CALL), "CALL");
    EXPECT_EQ(to_string(gb_action_t::RAISE), "RAISE");
    EXPECT_EQ(to_string(gb_action_t::ALLIN), "ALLIN");
    EXPECT_THROW(static_cast<void>(to_string(static_cast<gb_action_t>(5))), std::runtime_error);
}

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

    const std::array<card, 9> cards1{// board
                                     card("2c"), card("3c"), card("4c"), card("5c"), card("6c"),
                                     // sb
                                     card("2d"), card("3d"),
                                     // bb
                                     card("2h"), card("3h")};
    const std::array<card, 9> cards2{// board
                                     card("2c"), card("3c"), card("4c"), card("5c"), card("6c"),
                                     // sb same cards, different order -> gc should be identical
                                     card("3d"), card("2d"),
                                     // bb
                                     card("2h"), card("3h")};
    const std::array<card, 9> cards3{// board
                                     card("2c"), card("3c"), card("4c"), card("5c"), card("6c"),
                                     // sb and bb exchanged -> gc should be different
                                     card("2h"), card("3h"),
                                     // bb
                                     card("2d"), card("3d")};
    const auto g1 = gamecards<2>(cards1);
    const auto g2 = gamecards<2>(cards2);
    const auto g3 = gamecards<2>(cards3);

    EXPECT_EQ(g1, g2);
    EXPECT_EQ(g2, g1);
    EXPECT_NE(g1, g3);
    EXPECT_NE(g2, g3);
}

TEST(tgame, game_gamecards_board_n)
{
    const std::array<card, 9> cards{// board
                                    card("2c"), card("3c"), card("4c"), card("5c"), card("6c"),
                                    // sb
                                    card("2d"), card("3d"),
                                    // bb
                                    card("2h"), card("3h")};
    const auto gc = gamecards<2>{cards};
    const auto board3 = gc.board_n(3);
    const auto board4 = gc.board_n(4);
    const auto board5 = gc.board_n(5);
    const auto sub3 = std::span<const card>(cards.data(), 3);
    const auto sub4 = std::span<const card>(cards.data(), 4);
    const auto sub5 = std::span<const card>(cards.data(), 5);
    EXPECT_EQ(board3.size(), 3);
    EXPECT_EQ(std::equal(board3.begin(), board3.end(), sub3.begin(), sub3.end()), true);
    EXPECT_EQ(board4.size(), 4);
    EXPECT_EQ(std::equal(board4.begin(), board4.end(), sub4.begin(), sub4.end()), true);
    EXPECT_EQ(board5.size(), 5);
    EXPECT_EQ(std::equal(board5.begin(), board5.end(), sub5.begin(), sub5.end()), true);
    EXPECT_THROW(static_cast<void>(gc.board_n(6)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gc.board_n(7)), std::runtime_error);

    EXPECT_EQ(gc.board_n_as_cs(3), cardset{"2c3c4c"});
    EXPECT_EQ(gc.board_n_as_cs(4), cardset{"2c3c4c5c"});
    EXPECT_EQ(gc.board_n_as_cs(5), cardset{"2c3c4c5c6c"});
    EXPECT_THROW(static_cast<void>(gc.board_n_as_cs(6)), std::runtime_error);
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
    EXPECT_THROW(static_cast<void>(gamestate<2, 0, 1>(999)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<3, 0, 1>(999)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<4, 0, 1>(999)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<5, 0, 1>(999)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<6, 0, 1>(999)), std::runtime_error);

    const auto g2 = gamestate<2, 0, 1>(2000);
    const auto g3 = gamestate<3, 0, 1>(2000);
    const auto g4 = gamestate<4, 0, 1>(2000);
    const auto g5 = gamestate<5, 0, 1>(2000);
    const auto g6 = gamestate<6, 0, 1>(2000);
    // all players should be on state init
    EXPECT_EQ(g6.all_players_state(), make_array<6>(gb_playerstate_t::INIT));

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

    EXPECT_THROW(static_cast<void>(g2.payouts_noshowdown()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g3.payouts_noshowdown()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g4.payouts_noshowdown()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g5.payouts_noshowdown()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g6.payouts_noshowdown()), std::runtime_error);
}

TEST(tgame, game_gamestate_ctor_chips)
{
    std::array<int32_t, 2> chips2ok{1000, 500};
    std::array<int32_t, 2> chips2a{999, 500};
    std::array<int32_t, 2> chips2b{1000, 499};
    std::array<int32_t, 3> chips3ok{500, 1000, 1};
    std::array<int32_t, 3> chips3a{499, 1000, 1000};
    std::array<int32_t, 3> chips3b{500, 999, 1000};
    std::array<int32_t, 4> chips4{1000, 500, 1000, 1000};
    std::array<int32_t, 5> chips5{1000, 500, 1000, 1000, 1000};
    std::array<int32_t, 6> chips6{1000, 500, 1000, 1000, 1000, 1000};

    EXPECT_NO_THROW(static_cast<void>(gamestate<2, 0, 1>(chips2ok)));
    EXPECT_THROW(static_cast<void>(gamestate<2, 0, 1>(chips2a)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<2, 0, 1>(chips2b)), std::runtime_error);
    EXPECT_NO_THROW(static_cast<void>(gamestate<3, 0, 1>(chips3ok)));
    EXPECT_THROW(static_cast<void>(gamestate<3, 0, 1>(chips3a)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<3, 0, 1>(chips3b)), std::runtime_error);

    EXPECT_THROW(static_cast<void>(gamestate<4, 0, 1>(chips4)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<5, 0, 1>(chips5)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<6, 0, 1>(chips6)), std::runtime_error);
}

TEST(tgame, game_gamestate_execute_action)
{
    //
    // test exceptions
    //
    {
        auto game1 = gamestate<2, 0, 1>(3000);
        EXPECT_GT(game1.possible_actions().size(), 0);
#if !defined(NDEBUG)
        EXPECT_THROW(game1.execute_action(player_action_t{1500, gb_action_t::RAISE, gb_pos_t::SB}), std::runtime_error);
        EXPECT_THROW(game1.execute_action(player_action_t{1499, gb_action_t::RAISE, game1.active_player_v()}), std::runtime_error);
#endif

        EXPECT_EQ(game1.minraise(), 1000);
        game1.execute_action(player_action_t{1500, gb_action_t::RAISE, game1.active_player_v()});
        EXPECT_EQ(game1.active_player(), 0);
        EXPECT_EQ(game1.in_terminal_state(), false);
        EXPECT_EQ(game1.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
        EXPECT_EQ(game1.minraise(), 1000);    // last raise was a min raise

        game1.execute_action(player_action_t{2000, gb_action_t::ALLIN, game1.active_player_v()});
        game1.execute_action(player_action_t{1000, gb_action_t::ALLIN, game1.active_player_v()});
        EXPECT_EQ(game1.in_terminal_state(), true);
        EXPECT_EQ(game1.is_showdown(), true);
        EXPECT_THROW(static_cast<void>(game1.payouts_noshowdown()), std::runtime_error);
        EXPECT_EQ(game1.possible_actions().size(), 0);
    }

    //
    // test different actions
    //
    {
        auto game2 = gamestate<3, 0, 1>(10000);
        // state should be preflop bet
        EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
        // all players should be on state init
        EXPECT_EQ(game2.all_players_state(), make_array<3>(gb_playerstate_t::INIT));
        // UTG bets 2BB (3000 total)
        EXPECT_EQ(game2.minraise(), 1000);
        game2.execute_action(player_action_t{3000, gb_action_t::RAISE, game2.active_player_v()});
        EXPECT_EQ(game2.minraise(), 2000);
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
        // payouts should be {-500,+4500,-4000}
        EXPECT_EQ(game2.payouts_noshowdown(), (std::array<int32_t, 3>{-500, 4500, -4000}));
    }

    //
    // test showdown
    //
    {
        auto game3 = gamestate<3, 0, 1>(10000);
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
        // state should be turn bet
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
        EXPECT_EQ(game3.gamestate_v(), gb_gamestate_t::GAME_FIN);
    }
}

template <std::size_t N>
constexpr auto make_games_array() -> typename std::array<gamestate<N, 0, 1>, 3>
{
    return std::array<gamestate<N, 0, 1>, 3>{gamestate<N, 0, 1>(3000), gamestate<N, 0, 1>(3000), gamestate<N, 0, 1>(4000)};
}

TEST(tgame, game_gamestate_comp)
{
    // todo: rework

    auto games2 = make_games_array<2>();
    auto games3 = make_games_array<3>();
    auto games4 = make_games_array<4>();
    auto games5 = make_games_array<5>();
    auto games6 = make_games_array<6>();

    EXPECT_EQ(games2[0], games2[0]);
    EXPECT_EQ(games2[0], games2[1]);
    EXPECT_NE(games2[0], games2[2]);
    EXPECT_EQ(games2[0].str_state(), games2[0].str_state());
    EXPECT_EQ(games2[0].str_state(), games2[1].str_state());
    EXPECT_NE(games2[0].str_state(), games2[2].str_state());

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
    EXPECT_EQ(games6[0].str_state(), games6[0].str_state());
    EXPECT_EQ(games6[0].str_state(), games6[1].str_state());
    EXPECT_NE(games6[0].str_state(), games6[2].str_state());
}

TEST(tgame, game_gamestate_payouts)
{
    const std::array<card, 11> cards{// board
                                     card("2c"), card("2d"), card("6h"), card("7s"), card("Jd"),
                                     // SB
                                     card("Tc"), card("Td"),
                                     // BB
                                     card("Qc"), card("Jc"),
                                     // UTG
                                     card("Qs"), card("Js")};
    const gamecards<3> gc{cards};

    // no showdown
    {
        auto g0 = gamestate<3, 0, 1>(3000);
        // should not be in terminal state
        EXPECT_THROW(static_cast<void>(g0.payouts_showdown(gc)), std::runtime_error);
        EXPECT_THROW(static_cast<void>(g0.all_pots()), std::runtime_error);

        g0.execute_action(player_action_t{3000, gb_action_t::ALLIN, g0.active_player_v()});
        g0.execute_action(player_action_t{0, gb_action_t::FOLD, g0.active_player_v()});
        g0.execute_action(player_action_t{0, gb_action_t::FOLD, g0.active_player_v()});
        EXPECT_EQ(g0.gamestate_v(), gb_gamestate_t::GAME_FIN);
        EXPECT_THROW(static_cast<void>(g0.payouts_showdown(gc)), std::runtime_error);
    }

    // showdown with 3 all in
    {
        auto g1 = gamestate<3, 0, 1>(3000);
        // everyone calls
        g1.execute_action(player_action_t{3000, gb_action_t::ALLIN, g1.active_player_v()});
        g1.execute_action(player_action_t{2500, gb_action_t::ALLIN, g1.active_player_v()});
        g1.execute_action(player_action_t{2000, gb_action_t::ALLIN, g1.active_player_v()});
        //// state should be fin
        EXPECT_EQ(g1.gamestate_v(), gb_gamestate_t::GAME_FIN);
        const std::array<int32_t, 3> expected_payouts1{-3000, 1500, 1500};
        EXPECT_EQ(g1.payouts_showdown(gc), expected_payouts1);
    }

    // showdown with 2
    {
        auto g2 = gamestate<3, 0, 1>(3000);
        // everyone calls
        g2.execute_action(player_action_t{1000, gb_action_t::CALL, g2.active_player_v()});     // utg
        g2.execute_action(player_action_t{500, gb_action_t::CALL, g2.active_player_v()});      // sb
        g2.execute_action(player_action_t{1000, gb_action_t::RAISE, g2.active_player_v()});    // bb
        g2.execute_action(player_action_t{2000, gb_action_t::ALLIN, g2.active_player_v()});    // utg
        g2.execute_action(player_action_t{2000, gb_action_t::ALLIN, g2.active_player_v()});    // sb
        g2.execute_action(player_action_t{0, gb_action_t::FOLD, g2.active_player_v()});        // bb fold
        // state should be fin
        EXPECT_EQ(g2.gamestate_v(), gb_gamestate_t::GAME_FIN);
        const std::array<int32_t, 3> expected_payouts2{-3000, -2000, 5000};
        EXPECT_EQ(g2.payouts_showdown(gc), expected_payouts2);
    }

    // showdown with 4, side pot
    {
        const std::array<card, 13> cards_4{// board
                                           card("2c"), card("2d"), card("6h"), card("7s"), card("Jd"),
                                           // SB 2000
                                           card("Ac"), card("Ad"),
                                           // BB 2000
                                           card("Ah"), card("As"),
                                           // UTG 5000
                                           card("Ks"), card("Qh"),
                                           // MP  5000
                                           card("Qs"), card("Js")};
        const gamecards<4> gc4{cards_4};

        const std::array<int32_t, 4> chips_start4{2000, 2000, 5000, 5000};
        auto g4 = gamestate<4, 0, 1>(chips_start4);
        const std::array<int32_t, 4> chips_front_expected4{500, 1000, 0, 0};
        const std::array<int32_t, 4> chips_behind_expected4{1500, 1000, 5000, 5000};
        // everyone calls
        EXPECT_EQ(g4.chips_front(), chips_front_expected4);
        EXPECT_EQ(g4.chips_behind(), chips_behind_expected4);
        g4.execute_action(player_action_t{5000, gb_action_t::ALLIN, g4.active_player_v()});    // utg
        g4.execute_action(player_action_t{5000, gb_action_t::ALLIN, g4.active_player_v()});    // mp
        g4.execute_action(player_action_t{1500, gb_action_t::ALLIN, g4.active_player_v()});    // sb
        g4.execute_action(player_action_t{1000, gb_action_t::ALLIN, g4.active_player_v()});    // bb
        // state should be fin
        EXPECT_EQ(g4.gamestate_v(), gb_gamestate_t::GAME_FIN);
        const std::array<int32_t, 4> expected_payouts4{2000, 2000, -5000, 1000};
        EXPECT_EQ(g4.payouts_showdown(gc4), expected_payouts4);
    }
}

TEST(tgame, game_gamestate_execute_action_rake)
{
    //
    // test exceptions
    //
    {
        auto game1 = gamestate<2, 10, 100>(3000);
        EXPECT_GT(game1.possible_actions().size(), 0);
#if !defined(NDEBUG)
        EXPECT_THROW(game1.execute_action(player_action_t{1500, gb_action_t::RAISE, gb_pos_t::SB}), std::runtime_error);
        EXPECT_THROW(game1.execute_action(player_action_t{1499, gb_action_t::RAISE, game1.active_player_v()}), std::runtime_error);
#endif

        game1.execute_action(player_action_t{1500, gb_action_t::RAISE, game1.active_player_v()});
        EXPECT_EQ(game1.active_player(), 0);
        EXPECT_EQ(game1.in_terminal_state(), false);
        EXPECT_EQ(game1.gamestate_v(), gb_gamestate_t::PREFLOP_BET);

        game1.execute_action(player_action_t{2000, gb_action_t::ALLIN, game1.active_player_v()});
        game1.execute_action(player_action_t{1000, gb_action_t::ALLIN, game1.active_player_v()});
        EXPECT_EQ(game1.in_terminal_state(), true);
        EXPECT_EQ(game1.is_showdown(), true);
        EXPECT_THROW(static_cast<void>(game1.payouts_noshowdown()), std::runtime_error);
        EXPECT_EQ(game1.possible_actions().size(), 0);
    }

    //
    // test different actions
    //
    {
        auto game2 = gamestate<3, 10, 100>(10000);
        // state should be preflop bet
        EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::PREFLOP_BET);
        // all players should be on state init
        EXPECT_EQ(game2.all_players_state(), make_array<3>(gb_playerstate_t::INIT));
        // UTG bets 2BB (3'000 total)
        game2.execute_action(player_action_t{3000, gb_action_t::RAISE, game2.active_player_v()});
        // SB folds
        game2.execute_action(player_action_t{0, gb_action_t::FOLD, game2.active_player_v()});
        // BB calls (2'000 total)
        game2.execute_action(player_action_t{2000, gb_action_t::CALL, game2.active_player_v()});
        // total pot: 6'500
        // state should be flop bet
        EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::FLOP_BET);
        // active player should be BB
        EXPECT_EQ(game2.active_player(), 1);
        // BB checks
        game2.execute_action(player_action_t{0, gb_action_t::CHECK, game2.active_player_v()});
        // UTG bets 1BB (1'000 total)
        game2.execute_action(player_action_t{1000, gb_action_t::RAISE, game2.active_player_v()});
        // BB calls (1'000 total)
        game2.execute_action(player_action_t{1000, gb_action_t::CALL, game2.active_player_v()});
        // total pot: 8'500
        // state should be turn bet
        EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::TURN_BET);
        // active player should be BB
        EXPECT_EQ(game2.active_player(), 1);
        // BB checks
        game2.execute_action(player_action_t{0, gb_action_t::CHECK, game2.active_player_v()});
        // UTG checks
        game2.execute_action(player_action_t{0, gb_action_t::CHECK, game2.active_player_v()});
        // total pot: 8'500
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
        // payouts should be {-500,+4500,-4000}
        // after 10% rake: payout is 8'500 - 850 = 7'650
        const int32_t rake = static_cast<int32_t>(8'500 * (0.1));
        const int32_t payout = 8'500 - rake;
        const int32_t winnings = payout - 4'000;
        EXPECT_EQ(game2.payouts_noshowdown(), (std::array<int32_t, 3>{-500, winnings, -4000}));
    }

    //
    // test showdown
    //
    {
        auto game3 = gamestate<3, 10, 100>(10000);
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
        // state should be turn bet
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
        EXPECT_EQ(game3.gamestate_v(), gb_gamestate_t::GAME_FIN);
    }
}

TEST(tgame, game_gamestate_payouts_rake)
{
    // rake - reuse test from before but add 50% and 4.375% (PS rake)

    const std::array<card, 11> cards{// board
                                     card("2c"), card("2d"), card("6h"), card("7s"), card("Jd"),
                                     // SB
                                     card("Tc"), card("Td"),
                                     // BB
                                     card("Qc"), card("Jc"),
                                     // UTG
                                     card("Qs"), card("Js")};
    const gamecards<3> gc{cards};
    {
        // 50% rake
        auto g_rake_1 = gamestate<3, 5, 10>(3000);
        g_rake_1.execute_action(player_action_t{3000, gb_action_t::ALLIN, g_rake_1.active_player_v()});
        g_rake_1.execute_action(player_action_t{2500, gb_action_t::ALLIN, g_rake_1.active_player_v()});
        g_rake_1.execute_action(player_action_t{2000, gb_action_t::ALLIN, g_rake_1.active_player_v()});
        EXPECT_EQ(g_rake_1.gamestate_v(), gb_gamestate_t::GAME_FIN);
        // old payouts (0% rake)
        const std::array<int32_t, 3> expected_payouts1{-3000, 1500, 1500};
        // from the 9'000 pot, 50% is taken by the casino as rake, so 4500 are
        // distributed to the winners -> 2'250 each so they should have a net
        // loss of (-)750 each
        const auto diff = 1500 - (-750);
        // which is a difference of (-)2'250 to the old expected winnings of +1500
        const std::array<int32_t, 3> expected_payouts1_after_rake = {expected_payouts1[0], expected_payouts1[1] - diff,
                                                                     expected_payouts1[2] - diff};
        EXPECT_EQ(g_rake_1.payouts_showdown(gc), expected_payouts1_after_rake);
    }
    {
        // 4.375% rake
        auto g_rake_2 = gamestate<3, 4'375, 100'000>(3000);
        g_rake_2.execute_action(player_action_t{3000, gb_action_t::ALLIN, g_rake_2.active_player_v()});
        g_rake_2.execute_action(player_action_t{2500, gb_action_t::ALLIN, g_rake_2.active_player_v()});
        g_rake_2.execute_action(player_action_t{2000, gb_action_t::ALLIN, g_rake_2.active_player_v()});
        EXPECT_EQ(g_rake_2.gamestate_v(), gb_gamestate_t::GAME_FIN);
        // old payouts (0% rake)
        const std::array<int32_t, 3> expected_payouts2{-3000, 1500, 1500};
        // from the 9'000 pot, 4.375% is taken by the casino as rake (393.75), so the remaining
        // 9'000 - 393 are distributed to the winners -> 4'303 each so they should have a net
        // plus 1'303
        const auto rake = static_cast<int32_t>(9'000 * 0.04375);
        const auto payout = ((9'000 - rake) / 2) - 3000;
        const auto diff = 1500 - payout;
        // which is a difference of (-)2'250 to the old expected winnings of +1500
        const std::array<int32_t, 3> expected_payouts2_after_rake = {expected_payouts2[0], expected_payouts2[1] - diff,
                                                                     expected_payouts2[2] - diff};
        EXPECT_EQ(g_rake_2.payouts_showdown(gc), expected_payouts2_after_rake);
    }
}

TEST(tgame, game_gamestate_all_pots)
{
    std::pair<gb_pos_t, int32_t> chips_ret1{gb_pos_t{2}, 2000};
    std::pair<gb_pos_t, int32_t> chips_ret2{gb_pos_t{0}, 0};

    auto g_1 = gamestate<3, 0, 1>(3000);
    g_1.execute_action(player_action_t{3000, gb_action_t::ALLIN, g_1.active_player_v()});
    g_1.execute_action(player_action_t{0, gb_action_t::FOLD, g_1.active_player_v()});
    g_1.execute_action(player_action_t{0, gb_action_t::FOLD, g_1.active_player_v()});
    EXPECT_EQ(g_1.gamestate_v(), gb_gamestate_t::GAME_FIN);
    EXPECT_EQ(g_1.all_pots().size(), 1);
    EXPECT_EQ(g_1.chips_to_return(), chips_ret1);

    auto g_2 = gamestate<3, 0, 1>(3000);
    g_2.execute_action(player_action_t{3000, gb_action_t::ALLIN, g_2.active_player_v()});
    g_2.execute_action(player_action_t{2500, gb_action_t::ALLIN, g_2.active_player_v()});
    g_2.execute_action(player_action_t{2000, gb_action_t::ALLIN, g_2.active_player_v()});
    EXPECT_EQ(g_2.gamestate_v(), gb_gamestate_t::GAME_FIN);
    EXPECT_EQ(g_2.all_pots().size(), 1);
    EXPECT_EQ(g_2.chips_to_return(), chips_ret2);
}

TEST(tgame, game_gamestate_hand_ended_branch2)
{
    // reach branch 2 in execute_action / entire hand ended, where
    // one player is all in, one player bets/raises and everyone else folds
    std::array<int32_t, 3> chips{3000, 5000, 5000};
    auto game = gamestate<3, 0, 1>(chips);
    game.execute_action(player_action_t{3000, gb_action_t::RAISE, game.active_player_v()});
    game.execute_action(player_action_t{2500, gb_action_t::ALLIN, game.active_player_v()});
    game.execute_action(player_action_t{2000, gb_action_t::CALL, game.active_player_v()});
    EXPECT_EQ(game.gamestate_v(), gb_gamestate_t::FLOP_BET);
    game.execute_action(player_action_t{0, gb_action_t::FOLD, game.active_player_v()});
    EXPECT_EQ(game.gamestate_v(), gb_gamestate_t::GAME_FIN);

    std::array<int32_t, 3> chips2{3000, 5000, 1000};
    std::pair<gb_pos_t, int32_t> chips_ret{gb_pos_t{2}, 2000};
    auto game2 = gamestate<3, 0, 1>(chips2);
    game2.execute_action(player_action_t{1000, gb_action_t::ALLIN, game2.active_player_v()});
    game2.execute_action(player_action_t{0, gb_action_t::FOLD, game2.active_player_v()});
    EXPECT_EQ(game2.gamestate_v(), gb_gamestate_t::GAME_FIN);
}
