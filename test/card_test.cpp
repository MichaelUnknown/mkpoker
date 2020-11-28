#include <mkpoker/base/card.hpp>

#include <array>
#include <bit>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

using namespace mkp;

constexpr std::string_view cardstrings =
    "2c3c4c5c6c7c8c9cTcJcQcKcAc"
    "2d3d4d5d6d7d8d9dTdJdQdKdAd"
    "2h3h4h5h6h7h8h9hThJhQhKhAh"
    "2s3s4s5s6s7s8s9sTsJsQsKsAs";

TEST(tcard, card_ctor_int)
{
    // valid range
    for (uint8_t i = 0; i < c_deck_size; ++i)
    {
        EXPECT_EQ(i, card{i}.m_card);
        const auto str = cardstrings.substr(size_t(i) * 2, 2);
        EXPECT_EQ(str, card{i}.str());
    }

    // invalid range
    for (uint8_t i = c_deck_size; i < 255; ++i)
    {
        EXPECT_THROW(card{i}, std::runtime_error);
    }
}

TEST(tcard, card_ctor_string)
{
    // valid range
    for (size_t i = 0; i < cardstrings.length(); i += 2)
    {
        auto str = cardstrings.substr(i, 2);
        EXPECT_EQ(str, card(str).str());
    }

    // invalid inputs
    EXPECT_THROW(card{"xy"}, std::runtime_error);
    EXPECT_THROW(card{"AcAsAdAh"}, std::runtime_error);
    EXPECT_THROW(card{"AA"}, std::runtime_error);
    EXPECT_THROW(card{"C2"}, std::runtime_error);
    EXPECT_THROW(card{"dA"}, std::runtime_error);
    EXPECT_THROW(card{"dd"}, std::runtime_error);

#ifndef _DEBUG
    // these will trigger an assert in msvc/debug
    EXPECT_THROW(card{"c"}, std::runtime_error);
    EXPECT_THROW(card{"5"}, std::runtime_error);
#endif
}

TEST(tcard, card_ctor_ranksuit)
{
    // valid range
    for (uint8_t i = 0; i < c_num_suits; ++i)
    {
        for (uint8_t j = 0; j < c_num_ranks; ++j)
        {
            auto str = cardstrings.substr((size_t(i) * 13 + j) * 2, 2);
            rank r{rank_t{j}};
            suit s{suit_t{i}};
            EXPECT_EQ(str, card(r, s).str());
            EXPECT_EQ(str, card(rank_t{j}, suit_t{i}).str());
        }
    }

    // invalid input
    EXPECT_THROW(card(static_cast<rank_t>(13), static_cast<suit_t>(4)), std::runtime_error);
    EXPECT_THROW(card(static_cast<rank_t>(13), static_cast<suit_t>(0)), std::runtime_error);
    EXPECT_THROW(card(static_cast<rank_t>(0), static_cast<suit_t>(4)), std::runtime_error);
}

TEST(tcard, card_accessors)
{
    constexpr char ranks[] = {'2', '3', 'K', 'A'};
    constexpr char suits[] = {'c', 'd', 'h', 's', 'C', 'D', 'H', 'S'};
    for (auto&& s : suits)
    {
        for (auto&& r : ranks)
        {
            EXPECT_EQ(rank(r), card(std::string(1, r) + s).rank());
            EXPECT_EQ(suit(s), card(std::string(1, r) + s).suit());
            // todo: uncomment, once popcount is available
            //EXPECT_EQ(1, std::popcount(card(std::string(1, r) + s).as_bitset()));
        }
    }
}

TEST(tcard, card_comparison_operators)
{
    EXPECT_TRUE(card("Ac") > card("Ks"));
    EXPECT_TRUE(card("As") > card("Ac"));
    EXPECT_TRUE(card("Kc") < card("Ac"));
    EXPECT_TRUE(card("Th") >= card("Th"));
    EXPECT_TRUE(card("7s") <= card("8h"));
    EXPECT_TRUE(card("4h") == card("4h"));
    EXPECT_TRUE(card("5h") != card("5c"));
}
