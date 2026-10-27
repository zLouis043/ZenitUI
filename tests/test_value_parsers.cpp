#include "TestFramework.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;

// ---------- parseValueToken ----------

TEST(ValueParsers_valueToken, empty_returns_nullopt) {
    CHECK(!parseValueToken("").has_value());
}

TEST(ValueParsers_valueToken, auto_kw) {
    auto v = parseValueToken("auto");
    CHECK(v.has_value());
    CHECK(v->isAuto());
}

TEST(ValueParsers_valueToken, px) {
    auto v = parseValueToken("10px");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(0, 0), 10.0f, 1e-6);
}

TEST(ValueParsers_valueToken, bare_number_is_px) {
    auto v = parseValueToken("42");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(0, 0), 42.0f, 1e-6);
}

TEST(ValueParsers_valueToken, percent) {
    auto v = parseValueToken("50%");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(200, 100), 100.0f, 1e-6);
}

TEST(ValueParsers_valueToken, vw) {
    auto v = parseValueToken("10vw");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(0, 0), 192.0f, 1e-4);
}

TEST(ValueParsers_valueToken, vh) {
    auto v = parseValueToken("10vh");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(0, 0), 108.0f, 1e-4);
}

TEST(ValueParsers_valueToken, pw) {
    auto v = parseValueToken("50pw");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(200, 100), 100.0f, 1e-4);
}

TEST(ValueParsers_valueToken, ph) {
    auto v = parseValueToken("50ph");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(200, 100), 50.0f, 1e-4);
}

TEST(ValueParsers_valueToken, unknown_unit_returns_nullopt) {
    CHECK(!parseValueToken("10foo").has_value());
}

TEST(ValueParsers_valueToken, calc_simple_sum) {
    auto v = parseValueToken("calc(10px + 5px)");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(0, 0), 15.0f, 1e-4);
}

TEST(ValueParsers_valueToken, calc_percent_minus_px) {
    auto v = parseValueToken("calc(100% - 20px)");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(200, 100), 180.0f, 1e-4);
}

TEST(ValueParsers_valueToken, calc_with_parens) {
    auto v = parseValueToken("calc((100% - 20px) / 2)");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(200, 100), 90.0f, 1e-3);
}

TEST(ValueParsers_valueToken, calc_mul_by_scalar) {
    auto v = parseValueToken("calc(50% * 2)");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(200, 100), 200.0f, 1e-3);
}

TEST(ValueParsers_valueToken, calc_invalid_returns_nullopt) {
    CHECK(!parseValueToken("calc(10px +)").has_value());
    CHECK(!parseValueToken("calc((10px").has_value());
}

// ---------- parseColorToken ----------

TEST(ValueParsers_colorToken, named) {
    auto c = parseColorToken("red");
    CHECK(c.has_value());
    CHECK(c->r == 230);
    CHECK(c->g == 41);
    CHECK(c->b == 55);
    CHECK(c->a == 255);
}

TEST(ValueParsers_colorToken, hex_3) {
    auto c = parseColorToken("#F00");
    CHECK(c.has_value());
    CHECK(c->r == 255);
    CHECK(c->g == 0);
    CHECK(c->b == 0);
    CHECK(c->a == 255);
}

TEST(ValueParsers_colorToken, hex_6) {
    auto c = parseColorToken("#1E90FF");
    CHECK(c.has_value());
    CHECK(c->r == 0x1E);
    CHECK(c->g == 0x90);
    CHECK(c->b == 0xFF);
}

TEST(ValueParsers_colorToken, hex_8_with_alpha) {
    auto c = parseColorToken("#FF000080");
    CHECK(c.has_value());
    CHECK(c->r == 255);
    CHECK(c->a == 0x80);
}

TEST(ValueParsers_colorToken, invalid) {
    CHECK(!parseColorToken("not-a-color").has_value());
    CHECK(!parseColorToken("").has_value());
}

// ---------- parseSpacingToken ----------

TEST(ValueParsers_spacingToken, one_value) {
    auto s = parseSpacingToken("10px");
    CHECK(s.has_value());
    CHECK_NEAR(s->top.resolveV(0, 0),    10.0f, 1e-6);
    CHECK_NEAR(s->right.resolveH(0, 0),  10.0f, 1e-6);
    CHECK_NEAR(s->bottom.resolveV(0, 0), 10.0f, 1e-6);
    CHECK_NEAR(s->left.resolveH(0, 0),   10.0f, 1e-6);
}

TEST(ValueParsers_spacingToken, two_values_vh) {
    auto s = parseSpacingToken("5px 10px");
    CHECK(s.has_value());
    CHECK_NEAR(s->top.resolveV(0, 0),    5.0f, 1e-6);
    CHECK_NEAR(s->right.resolveH(0, 0),  10.0f, 1e-6);
    CHECK_NEAR(s->bottom.resolveV(0, 0), 5.0f, 1e-6);
    CHECK_NEAR(s->left.resolveH(0, 0),   10.0f, 1e-6);
}

TEST(ValueParsers_spacingToken, four_values) {
    auto s = parseSpacingToken("1px 2px 3px 4px");
    CHECK(s.has_value());
    CHECK_NEAR(s->top.resolveV(0, 0),    1.0f, 1e-6);
    CHECK_NEAR(s->right.resolveH(0, 0),  2.0f, 1e-6);
    CHECK_NEAR(s->bottom.resolveV(0, 0), 3.0f, 1e-6);
    CHECK_NEAR(s->left.resolveH(0, 0),   4.0f, 1e-6);
}

TEST(ValueParsers_spacingToken, empty) {
    CHECK(!parseSpacingToken("").has_value());
}

// ---------- parseAlignToken / parseJustifyToken ----------

TEST(ValueParsers_alignToken, all) {
    CHECK(parseAlignToken("auto")    == Align::Auto);
    CHECK(parseAlignToken("start")   == Align::Start);
    CHECK(parseAlignToken("center")  == Align::Center);
    CHECK(parseAlignToken("end")     == Align::End);
    CHECK(parseAlignToken("stretch") == Align::Stretch);
    CHECK(!parseAlignToken("xyz").has_value());
}

TEST(ValueParsers_justifyToken, all) {
    CHECK(parseJustifyToken("start")         == Justify::Start);
    CHECK(parseJustifyToken("center")        == Justify::Center);
    CHECK(parseJustifyToken("end")           == Justify::End);
    CHECK(parseJustifyToken("space-between") == Justify::SpaceBetween);
    CHECK(!parseJustifyToken("spacearound").has_value());
}

TEST(ValueParsers_colorToken, rgb_with_spaces)
{
    auto c = parseColorToken("rgb(255, 128, 0)");
    CHECK(c.has_value());
    CHECK(c->r == 255);
    CHECK(c->g == 128);
    CHECK(c->b == 0);
    CHECK(c->a == 255);
}

TEST(ValueParsers_colorToken, rgba_with_alpha)
{
    auto c = parseColorToken("rgba(255, 128, 0, 0.5)");
    CHECK(c.has_value());
    CHECK(c->r == 255);
    CHECK(c->g == 128);
    CHECK(c->b == 0);
    // 0.5 * 255 = 127.5, rounded to 128
    CHECK(c->a == 128);
}

TEST(ValueParsers_colorToken, rgb_clamps_out_of_range)
{
    auto c = parseColorToken("rgb(300, -20, 128)");
    CHECK(c.has_value());
    CHECK(c->r == 255);
    CHECK(c->g == 0);
    CHECK(c->b == 128);
}

TEST(ValueParsers_colorToken, rgba_full_alpha)
{
    auto c = parseColorToken("rgba(0, 0, 0, 1.0)");
    CHECK(c.has_value());
    CHECK(c->a == 255);
}

TEST(ValueParsers_colorToken, rgba_zero_alpha)
{
    auto c = parseColorToken("rgba(255, 255, 255, 0.0)");
    CHECK(c.has_value());
    CHECK(c->a == 0);
}

TEST(ValueParsers_colorToken, rgb_wrong_arg_count)
{
    CHECK(!parseColorToken("rgb(255, 128)").has_value());
    CHECK(!parseColorToken("rgb(255, 128, 0, 0.5)").has_value());
    CHECK(!parseColorToken("rgba(255, 128, 0)").has_value());
}

TEST(ValueParsers_valueToken, func_min) {
    auto v = parseValueToken("min(30px, 20px)");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(0, 0), 20.0f, 1e-4);
}

TEST(ValueParsers_valueToken, func_max) {
    auto v = parseValueToken("max(10px, 20px)");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(0, 0), 20.0f, 1e-4);
}

TEST(ValueParsers_valueToken, func_clamp) {
    auto v1 = parseValueToken("clamp(10px, 15px, 20px)");
    auto v2 = parseValueToken("clamp(10px, 5px, 20px)");
    auto v3 = parseValueToken("clamp(10px, 25px, 20px)");
    
    CHECK(v1.has_value() && v2.has_value() && v3.has_value());
    CHECK_NEAR(v1->resolveH(0, 0), 15.0f, 1e-4);
    CHECK_NEAR(v2->resolveH(0, 0), 10.0f, 1e-4);
    CHECK_NEAR(v3->resolveH(0, 0), 20.0f, 1e-4);
}

TEST(ValueParsers_valueToken, func_mixed_units_max) {
    auto v = parseValueToken("max(50%, 150px)");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(200, 100), 150.0f, 1e-4);
}

TEST(ValueParsers_valueToken, func_mod_and_rounding) {
    auto vMod = parseValueToken("mod(17px, 5px)");
    CHECK(vMod.has_value());
    CHECK_NEAR(vMod->resolveH(0, 0), 2.0f, 1e-4);

    auto vRound = parseValueToken("round(15.6px)");
    CHECK(vRound.has_value());
    CHECK_NEAR(vRound->resolveH(0, 0), 16.0f, 1e-4);
    
    auto vFloor = parseValueToken("floor(15.6px)");
    CHECK_NEAR(vFloor->resolveH(0, 0), 15.0f, 1e-4);
    
    auto vCeil = parseValueToken("ceil(15.2px)");
    CHECK_NEAR(vCeil->resolveH(0, 0), 16.0f, 1e-4);
}

TEST(ValueParsers_valueToken, func_nested_ast) {
    auto v = parseValueToken("calc(min(10px, 20px) + max(5px, 15px))");
    CHECK(v.has_value());
    CHECK_NEAR(v->resolveH(0, 0), 25.0f, 1e-4);
}