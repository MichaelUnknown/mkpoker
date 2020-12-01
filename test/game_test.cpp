#include <mkpoker/game/game.hpp>

#include <array>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tgame, game_gb_cards_2p_ctor)
{
    std::array<card, 9> a_cards = {card("Ac"), card("Ac"), card("Ac"), card("Ac"), card("Ac"),
                                   card("Ac"), card("Ac"), card("Ac"), card("Ac")};
    std::vector<card> v_cards(a_cards.cbegin(), a_cards.cend());

    const auto g1 = gb_cards_2p(v_cards);
    const auto g2 = gb_cards_2p(v_cards);
    //const auto g3 = gb_cards_2p({card("Ac"), card("Ac"), card("Ac"), card("Ac"), card("Ac")}, {{{card("Ac"), card("Ac")}, {card("Ac"), card("Ac")}}});

    EXPECT_EQ(g1, g2);
}

TEST(tgame, game_ctor)
{
    const auto g1 = gamestate<3>(2000);
}
