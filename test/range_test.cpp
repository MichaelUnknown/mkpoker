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
        EXPECT_EQ(empty_range.value_of(i), 0);
    }
}

TEST(trange, range_ctor_vector)
{
    const std::vector<hand_2r> vhr{hand_2r{"AA"}, hand_2r{"43"}, hand_2r{"QK"}};
    EXPECT_EQ(range(vhr).size(), 3);
    EXPECT_EQ(range(vhr).size_total(), 22);
}

TEST(trange, range_ctor_array)
{
    const std::array<hand_2r, 3> vhr{hand_2r{"AA"}, hand_2r{"43"}, hand_2r{"QK"}};
    EXPECT_EQ(range(vhr).size(), 3);
    EXPECT_EQ(range(vhr).size_total(), 22);
}

TEST(trange, range_ctor_str)
{
    const range r2{"99+"};
    EXPECT_EQ(r2.size(), 6);
    EXPECT_EQ(r2.size_total(), 36);

    EXPECT_EQ(range("77").size(), 1);
    EXPECT_EQ(range("77").size_total(), 6);

    EXPECT_EQ(range("43o,67o").size(), 2);
    EXPECT_EQ(range("43o,67o").size_total(), 24);

    EXPECT_EQ(range("QJs,89s").size(), 2);
    EXPECT_EQ(range("QJs,89s").size_total(), 8);

    EXPECT_EQ(range("A3s+").size(), 11);
    EXPECT_EQ(range("A3s+").size_total(), 44);
    EXPECT_EQ(range("A3o+").size(), 11);
    EXPECT_EQ(range("A3o+").size_total(), 11 * 12);

    // too many / few tokens
    EXPECT_THROW(const range r1{"A"}, std::runtime_error);
    EXPECT_THROW(const range r1{"AAKs+"}, std::runtime_error);

    // not a pair
    EXPECT_THROW(const range r1{"AK"}, std::runtime_error);

    // wrong 3rd token
    EXPECT_THROW(const range r1{"AKx"}, std::runtime_error);
    EXPECT_THROW(const range r1{"AK+"}, std::runtime_error);
    EXPECT_THROW(const range r1{"AAo"}, std::runtime_error);
    EXPECT_THROW(const range r1{"JJs"}, std::runtime_error);

    // wrong 4th token
    EXPECT_THROW(const range r1{"AA++"}, std::runtime_error);
    EXPECT_THROW(const range r1{"AKso"}, std::runtime_error);
    EXPECT_THROW(const range r1{"99+s"}, std::runtime_error);

    // one trailing comma should be fine
    EXPECT_NO_THROW(const range r1{"AKs,"});
    // wrong commas
    EXPECT_THROW(const range r1{"AKs,,"}, std::runtime_error);
    EXPECT_THROW(const range r1{"AJs+,,QJo"}, std::runtime_error);
}

TEST(trange, range_static_functions)
{
    // range / index cheat sheet:
    // A|K|Q|J|T|9|8|7|6|5|4|3|2
    // -+-+-+-+-+-+-+-+-+-+-+-+-
    // 0|1|2|3|4|5|6|7|8|9|A|B|C
    // suited combos have the higher value first:
    // AK = AKs, KA = AKo etc.

    // index
    // AA = 0*13+0, AK = 0*13+1, AQ=0*13+2 etc.
    // KA = 1*13+0, QA = 2*13+0, etc.
    // T5s (T5) should be: 4*13+9
    // T5o (5T) should be: 9*13+4
    hand_2r T5s{"T5"};
    hand_2r T5o{"5T"};
    hand_2r JJ{"JJ"};
    EXPECT_EQ(range::index(T5s), 4 * 13 + 9);
    EXPECT_EQ(range::index(T5o), 9 * 13 + 4);
    EXPECT_EQ(range::index(JJ), 3 * 13 + 3);
    EXPECT_EQ(range::hand(4 * 13 + 9), T5s);
    EXPECT_EQ(range::hand(9 * 13 + 4), T5o);
    EXPECT_EQ(range::hand(3 * 13 + 3), JJ);

    // index also works with hand_2c, order of cards doesn't matter here, suits are relevant
    hand_2c h2c_T5s{"Tc5c"};
    hand_2c h2c_T5o{"Tc5d"};
    hand_2c h2c_JJ{"JcJh"};
    EXPECT_EQ(range::index(h2c_T5s), 4 * 13 + 9);
    EXPECT_EQ(range::index(h2c_T5o), 9 * 13 + 4);
    EXPECT_EQ(range::index(h2c_JJ), 3 * 13 + 3);

    // max value
    EXPECT_EQ(range::get_max_value(T5s), 400);
    EXPECT_EQ(range::get_max_value(T5o), 1200);
    EXPECT_EQ(range::get_max_value(hand_2r("AA")), 600);
    EXPECT_EQ(range::get_max_value(hand_2r("KK")), 600);
    EXPECT_EQ(range::get_max_value(hand_2r("33")), 600);
    EXPECT_EQ(range::get_max_value(hand_2r("22")), 600);
    EXPECT_EQ(range::get_max_value(4 * 13 + 9), 400);
    EXPECT_EQ(range::get_max_value(9 * 13 + 4), 1200);
    EXPECT_EQ(range::get_max_value(0), 600);
    EXPECT_EQ(range::get_max_value(14), 600);
    EXPECT_EQ(range::get_max_value(28), 600);
    EXPECT_EQ(range::get_max_value(154), 600);
    EXPECT_EQ(range::get_max_value(168), 600);
}

TEST(trange, range_accessors)
{
    // size, size_total
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
    EXPECT_EQ(0, r1.value_of(hand_2r(1, 2)));
    EXPECT_EQ(0, r1.value_of(168));
    EXPECT_THROW(static_cast<void>(r1.value_of(169)), std::runtime_error);
}

TEST(trange, range_mutators)
{
    range r0{};
    r0.fill();
    EXPECT_EQ(r0.size(), 169);
    EXPECT_EQ(r0.size_total(), 1326);
    r0.clear();
    EXPECT_EQ(r0.size(), 0);

    hand_2r aces{"AA"};
    hand_2r kings{"KK"};

    r0.set_value(0, 100);
    EXPECT_EQ(r0.size(), 1);
    EXPECT_EQ(r0.size_total(), 6);
    EXPECT_EQ(r0.value_of(aces), 100);

    EXPECT_THROW(r0.set_value(c_range_size, 100), std::runtime_error);
    EXPECT_THROW(r0.set_value(0, range::get_max_value(0) + 1), std::runtime_error);
    EXPECT_THROW(r0.set_value(range::hand(0), range::get_max_value(0) + 1), std::runtime_error);

    r0.set_value(kings, 100);
    EXPECT_EQ(r0.size(), 2);
    EXPECT_EQ(r0.size_total(), 12);
    EXPECT_EQ(r0.value_of(14), 100);
}

TEST(trange, range_hand_index_roundtrip)
{
    for (uint8_t i = 0; i < c_range_size; i++)
    {
        EXPECT_EQ(i, range::index(range::hand(i)));
    }

    EXPECT_THROW(static_cast<void>(range::hand(c_range_size)), std::runtime_error);
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
