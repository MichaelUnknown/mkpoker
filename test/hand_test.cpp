#include <mkpoker/base/card.hpp>
#include <mkpoker/base/hand.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>

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

//TEST(suit, suit_ctor_int)
//{
//    // test the valid unsigned int range
//    for (uint8_t i = 0; i <= 3; ++i)
//    {
//        suit s{suit_t{i}};
//        EXPECT_EQ(i, s.m_suit);
//    }
//    // test the valid int range
//    for (int i = 0; i <= 3; ++i)
//    {
//        suit s{suit_t{uint8_t(i)}};
//        EXPECT_EQ(i, s.m_suit);
//    }
//
//    // some invalid range
//    for (uint8_t i = 4; i < 255; ++i)
//    {
//        EXPECT_THROW(suit{suit_t{i}}, std::runtime_error);
//    }
//}

//TEST(suit, suit_ctor_string)
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
//            EXPECT_EQ(valid_input.at(s), suit{s}.m_suit);
//            EXPECT_EQ(std::string(1, static_cast<char>(std::tolower(c))), suit{s}.str());
//        }
//        else
//        {
//            EXPECT_THROW(suit{s}, std::runtime_error);
//        }
//    }
//
//#ifndef _DEBUG
//    // these will trigger an assert in msvc/debug
//    EXPECT_THROW(suit{""}, std::runtime_error);
//#endif
//    EXPECT_THROW(suit{"too long"}, std::runtime_error);
//}

//TEST(suit, suit_comparison_operators)
//{
//    EXPECT_TRUE(suit{suit_t::diamonds} > suit{suit_t::clubs});
//    EXPECT_TRUE(suit{suit_t::diamonds} < suit{suit_t::spades});
//    EXPECT_TRUE(suit{suit_t::hearts} == suit{suit_t::hearts});
//    EXPECT_TRUE(suit{suit_t::hearts} != suit{suit_t::spades});
//    EXPECT_TRUE(suit{suit_t::diamonds} >= suit{suit_t::diamonds});
//    EXPECT_TRUE(suit{suit_t::clubs} <= suit{suit_t::clubs});
//}
