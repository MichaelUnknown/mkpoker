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
    EXPECT_THROW(static_cast<void>(gamestate<2>(999)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<3>(999)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<4>(999)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<5>(999)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<6>(999)), std::runtime_error);

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

    EXPECT_THROW(static_cast<void>(g2.payouts_noshodown()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g3.payouts_noshodown()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g4.payouts_noshodown()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g5.payouts_noshodown()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(g6.payouts_noshodown()), std::runtime_error);
}

TEST(tgame, game_gamestate_ctor_chips)
{
    std::array<int, 2> chips2{1000, 500};
    std::array<int, 3> chips3{1000, 500, 1000};
    std::array<int, 4> chips4{1000, 500, 1000, 1000};
    std::array<int, 5> chips5{1000, 500, 1000, 1000, 1000};
    std::array<int, 6> chips6{1000, 500, 1000, 1000, 1000, 1000};

    EXPECT_THROW(static_cast<void>(gamestate<2>(chips2)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<3>(chips3)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<4>(chips4)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<5>(chips5)), std::runtime_error);
    EXPECT_THROW(static_cast<void>(gamestate<6>(chips6)), std::runtime_error);
}

TEST(tgame, game_gamestate_execute_action)
{
    //
    // test exceptions
    //
    {
        auto game1 = gamestate<2>(3000);
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
        //#if !defined(NDEBUG)
        EXPECT_THROW(static_cast<void>(game1.payouts_noshodown()), std::runtime_error);
        //#endif
        EXPECT_EQ(game1.possible_actions().size(), 0);
    }

    //
    // test different actions
    //
    {
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
        // payouts should be {-500,+4500,-4000}
        const auto cards2 = make_array<card, 11>([](const uint8_t i) { return card(i); });
        const auto gcards2 = gamecards<3>(cards2);
        EXPECT_THROW(static_cast<void>(game2.payouts_showdown(gcards2)), std::runtime_error);
        EXPECT_EQ(game2.payouts_noshodown(), (std::array<int, 3>{-500, 4500, -4000}));
    }

    //
    // test showdown
    //
    {
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
constexpr auto make_games_array() -> typename std::array<gamestate<N>, 3>
{
    return std::array<gamestate<N>, 3>{gamestate<N>(3000), gamestate<N>(3000), gamestate<N>(4000)};
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

    // showdown with 3 all in
    {
        auto g1 = gamestate<3>(3000);
        EXPECT_THROW(static_cast<void>(g1.all_pots()), std::runtime_error);
        // todo:
        //EXPECT_THROW(static_cast<void>(g1.payouts_showdown()), std::runtime_error);
        //const auto cards2 = make_array<card, 11>([](const uint8_t i) { return card(i); });
        //const auto gcards2 = gamecards<3>(cards2);
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
        auto g2 = gamestate<3>(3000);
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
        auto g4 = gamestate<4>(chips_start4);
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
