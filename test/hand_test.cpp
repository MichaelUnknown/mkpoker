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
        }
    }
}

TEST(thand_2c, hand2c_ctor_string)
{
    EXPECT_EQ(hand_2c("AsAc"), hand_2c("AcAs"));
    EXPECT_THROW(hand_2c("AsAcAdAh"), std::runtime_error);

#ifndef _DEBUG
    // these will trigger an assert in msvc/debug
    EXPECT_THROW(hand_2c("AsA"), std::runtime_error);
    EXPECT_THROW(hand_2c("AA"), std::runtime_error);
#endif
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
    EXPECT_TRUE(hand_2c("2c2d") < hand_2c("AcAd"));
}

//TEST(hand, hand_ctor_string)
//{
//    std::unordered_map<std::string, uint8_t> valid_input;
//    valid_input["c"] = 0;
//    valid_input["d"] = 1;
//    valid_input["h"] = 2;
//    valid_input["s"] = 3;
//    valid_input["C"] = 0;
//    valid_input["D"] = 1;
//    valid_input["H"] = 2;
//    valid_input["S"] = 3;
//
//    // test the char range, but convert to string
//    for (unsigned char c = 0; c < 255; ++c)
//    {
//        if (const auto s = std::string(1, c); valid_input.contains(s))
//        {
//            EXPECT_EQ(valid_input.at(s), hand{s}.m_hand);
//            EXPECT_EQ(std::string(1, static_cast<char>(std::tolower(c))), hand{s}.str());
//        }
//        else
//        {
//            EXPECT_THROW(hand{s}, std::runtime_error);
//        }
//    }
//
//#ifndef _DEBUG
//    // these will trigger an assert in msvc/debug
//    EXPECT_THROW(hand{""}, std::runtime_error);
//#endif
//    EXPECT_THROW(hand{"too long"}, std::runtime_error);
//}
