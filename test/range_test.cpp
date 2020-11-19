#include <mkpoker/base/range.hpp>

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using namespace mkpoker::base;

TEST(trange, range_ctor_default)
{
    const range empty_range{};
    for (uint8_t i = 0; i < c_range_size; ++i)
    {
        EXPECT_EQ(empty_range[i], 0);
        EXPECT_EQ(empty_range.get_value(i), 0);
    }
}

TEST(trange, range_ctor_vector)
{
    const std::vector<hand_2r> vhr{hand_2r{"AA"}, hand_2r{"43"}, hand_2r{"QK"}};
    EXPECT_EQ(range(vhr).size(), 3);
    EXPECT_EQ(range(vhr).size_total(), 22);
    EXPECT_EQ(range(vhr).get_value(hand_2r{"AA"}), 600);
    EXPECT_EQ(range(vhr).get_normalized_value(hand_2r{"AA"}), 100);
}

TEST(trange, range_ctor_array)
{
    const std::array<hand_2r, 3> vhr{hand_2r{"AA"}, hand_2r{"43"}, hand_2r{"QK"}};
    EXPECT_EQ(range(vhr).size(), 3);
    EXPECT_EQ(range(vhr).size_total(), 22);
}

TEST(trange, range_ctor_str)
{
    EXPECT_THROW(const range r1{"xy"}, std::runtime_error);

    const range r2{"99+"};
    EXPECT_EQ(r2.size(), 6);
    EXPECT_EQ(r2.size_total(), 36);

    EXPECT_EQ(range("77").size(), 1);
    EXPECT_EQ(range("77").size_total(), 6);

    EXPECT_EQ(range("43o,67o").size(), 2);
    EXPECT_EQ(range("43o,67o").size_total(), 24);

    EXPECT_EQ(range("A3s+").size(), 11);
    EXPECT_EQ(range("A3o+").size(), 11);

    range r3{};
    r3.fill();
    EXPECT_EQ(r3.size(), 169);
    EXPECT_EQ(r3.size_total(), 1326);
    r3.clear();
    EXPECT_EQ(r3.size(), 0);
}

/*
TEST(trange, range_ctor_handmap)
{
    const handmap hm{"AQs+"};
    const range r{hm};
    const handmap hm2 = r.to_handmap();
    const range r2{hm2};
    EXPECT_EQ(r.str(), r2.str());
    EXPECT_EQ(hm.str(), hm2.str());
}
*/

TEST(trange, range_hand_index_roundtrip)
{
    EXPECT_THROW(range{}.hand(c_range_size), std::runtime_error);
    for (uint8_t i = 0; i < c_range_size; i++)
    {
        EXPECT_EQ(i, range().index(range().hand(i)));
    }
}

/*
TEST(trange, range_handmap_roundtrip)
{
    const range r{"88+,A6s+,2Qo+"};
    //std::cout << r.str();
    const range r2{r.to_handmap()};
    EXPECT_EQ(r.str(), r2.str());
    EXPECT_EQ(r, r2);
}
*/

TEST(trange, range_static_functions)
{
    // max value
    //EXPECT_EQ(100, mkpoker::base::range::get_max_value(1));
}

TEST(trange, range_accessors)
{
    // size
    const range r1{"KK+"};
    EXPECT_EQ(2, r1.size());
    EXPECT_EQ(2 * 6, r1.size_total());
    const range r2{"ATs+"};
    EXPECT_EQ(4, r2.size());
    EXPECT_EQ(4 * 4, r2.size_total());
    const range r3{"K9o+"};
    EXPECT_EQ(4, r3.size());
    EXPECT_EQ(4 * 12, r3.size_total());
    range r4{};
    r4.fill();
    EXPECT_EQ(c_range_size, r4.size());
    EXPECT_EQ(1326, r4.size_total());
    const range r5{"88+,A6s+,2Qo+"};
    EXPECT_EQ(7 + 8 + 10, r5.size());
    EXPECT_EQ(7 * 6 + 8 * 4 + 10 * 12, r5.size_total());

    // get value
    EXPECT_EQ(0, r1.get_value(hand_2r(1, 2)));
    EXPECT_EQ(0, r1.get_value(168));
    EXPECT_THROW(r1.get_value(169), std::runtime_error);
}

TEST(trange, range_comp_operators)
{
    const range empty1{};
    const range empty2{};
    EXPECT_EQ(empty1, empty1);
    EXPECT_EQ(empty1, empty2);
    EXPECT_EQ(empty2, empty2);

    range r1 = empty1;
    range r2 = empty2;
    r1.fill();
    r2.fill();
    EXPECT_EQ(r1, r1);
    EXPECT_EQ(r1, r2);
    EXPECT_EQ(r2, r2);
}
