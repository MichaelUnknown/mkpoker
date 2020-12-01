#include <mkpoker/game/game.hpp>

#include <array>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace mkp;

TEST(tgame, game_ctor)
{
    std::array<card, 9> a_cards = {card("Ac"), card("Ac"), card("Ac"), card("Ac"), card("Ac"),
                                   card("Ac"), card("Ac"), card("Ac"), card("Ac")};
    std::vector<card> v_cards(a_cards.cbegin(), a_cards.cend());

    const auto g1 = gb_cards_2p(v_cards);
    const auto g2 = gb_cards_2p(v_cards);
    const auto g3 =
        gb_cards_2p({card("Ac"), card("Ac"), card("Ac"), card("Ac"), card("Ac")}, {{{card("Ac"), card("Ac")}, {card("Ac"), card("Ac")}}});

    EXPECT_EQ(g1, g2);

    struct mys
    {
        std::array<std::array<card, 2>, 2> m_m1;
        std::array<card, 3> m_m2;

        //mys(const std::array<card, 2>& m1) : m_m1(m1) {}
        mys(const std::array<std::array<card, 2>, 2>& m1, const std::array<card, 3>& m2) : m_m1(m1), m_m2(m2) {}
    };

    mys mmm({{{card("Ac"), card("Ac")}, {card("Ac"), card("Ac")}}}, {card("Ac"), card("Ac"), card("Ac")});
}
