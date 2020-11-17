#include <mkpoker/base/card.hpp>
#include <mkpoker/base/hand.hpp>

#include <stdexcept>

#include <gtest/gtest.h>

using namespace mkpoker::base;

TEST(thand_2c, hand2c_ctor_card)
{
    for (uint8_t i = 0; i < c_cardindex_max; ++i)
    {
        for (uint8_t j = i + 1; j < c_cardindex_max; ++j)
        {
            card c1{i};
            card c2{j};

            EXPECT_EQ(hand_2c(c1, c2), hand_2c(c2, c1));
            EXPECT_THROW(hand_2c(c1, c1), std::runtime_error);
            EXPECT_THROW(hand_2c(c2, c2), std::runtime_error);

            std::string str = c1 < c2 ? c2.str() + c1.str() : c1.str() + c2.str();
            EXPECT_EQ(str, hand_2c(c1, c2).str());
            EXPECT_EQ(str, hand_2c(c2, c1).str());
        }
    }
}

TEST(thand_2c, hand2c_ctor_string)
{
    for (uint8_t i = 0; i < c_cardindex_max; ++i)
    {
        for (uint8_t j = i + 1; j < c_cardindex_max; ++j)
        {
            card c1{i};
            card c2{j};
            const auto str1 = c1.str() + c2.str();
            const auto str2 = c2.str() + c1.str();

            EXPECT_EQ(hand_2c(str1), hand_2c(str2));
            EXPECT_THROW(hand_2c(c1.str() + c1.str()), std::runtime_error);
            EXPECT_THROW(hand_2c(c2.str() + c2.str()), std::runtime_error);
            EXPECT_THROW(hand_2c(str1 + str2), std::runtime_error);

            std::string str = c1 < c2 ? c2.str() + c1.str() : c1.str() + c2.str();
            EXPECT_EQ(str, hand_2c(str1).str());
            EXPECT_EQ(str, hand_2c(str2).str());
        }
    }

    EXPECT_THROW(hand_2c("AsAcAd"), std::runtime_error);
    EXPECT_THROW(hand_2c("AsAcAdAh"), std::runtime_error);

#ifndef _DEBUG
    // these will trigger an assert in msvc/debug
    EXPECT_THROW(hand_2c("AsA"), std::runtime_error);
    EXPECT_THROW(hand_2c("AA"), std::runtime_error);
#endif
}

TEST(thand_2c, hand2c_ctor_fast)
{
    for (uint8_t i = 0; i < c_cardindex_max; ++i)
    {
        for (uint8_t j = i + 1; j < c_cardindex_max; ++j)
        {
            EXPECT_EQ(hand_2c(i, j), hand_2c(j, i));
            EXPECT_THROW(hand_2c(i, i), std::runtime_error);
            EXPECT_THROW(hand_2c(j, j), std::runtime_error);

            std::string str = card{i} < card{j} ? card{j}.str() + card{i}.str() : card{i}.str() + card{j}.str();
            EXPECT_EQ(str, hand_2c(i, j).str());
            EXPECT_EQ(str, hand_2c(j, i).str());
        }
    }
}

TEST(thand_2c, hand2c_ctor_cardset)
{
    for (uint8_t i = 0; i < c_cardindex_max; ++i)
    {
        for (uint8_t j = i + 1; j < c_cardindex_max; ++j)
        {
            card c1{i};
            card c2{j};
            cardset cs(std::array<card, 2>{c1, c2});

            card c1d{uint8_t(static_cast<uint8_t>(i - 1) % c_deck_size)};
            card c2i{uint8_t(static_cast<uint8_t>(j + 1) % c_deck_size)};

            EXPECT_EQ(hand_2c(cs), hand_2c(c2, c1));
            EXPECT_EQ(hand_2c(cs), hand_2c(c1, c2));
            EXPECT_EQ(hand_2c(cs), hand_2c(cs));
            EXPECT_THROW(hand_2c(cs.combine(cardset(std::array<card, 2>{c1d, c2i}))), std::runtime_error);
        }
    }
}

TEST(thand_2c, hand2c_comparison_operators)
{
    EXPECT_TRUE(hand_2c("2c2d") != hand_2c("AcAd"));
    EXPECT_TRUE(hand_2c("2c2d") < hand_2c("AcAd"));
    EXPECT_TRUE(hand_2c("2c2d") <= hand_2c("2c2d"));
    EXPECT_TRUE(hand_2c("2c2d") >= hand_2c("2c2d"));
    EXPECT_TRUE(hand_2c("AcAd") > hand_2c("2c2d"));
    EXPECT_TRUE(hand_2c("AcAd") == hand_2c("AcAd"));

    EXPECT_TRUE(hand_2c("2c2d") < hand_2c("2c2h"));
    EXPECT_TRUE(hand_2c("2c2s") > hand_2c("2d2h"));
}
