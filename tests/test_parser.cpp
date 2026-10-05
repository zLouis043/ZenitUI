#include "TestFramework.hpp"
#include "StyleParser.hpp"
#include "Theme.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;

TEST(Parser_basic, empty_string) {
    Theme t;
    loadStyleString("", t);
    CHECK(t.rules.empty());
}

TEST(Parser_basic, single_tag_rule) {
    Theme t;
    loadStyleString("Toggle { color: red; }", t);
    CHECK(t.rules.size() == 1);
    CHECK(t.rules[0].chain.size() == 1);
    CHECK(t.rules[0].chain[0].kind == SimpleSelector::Kind::Tag);
    CHECK(t.rules[0].chain[0].name == "Toggle");
    CHECK(t.rules[0].style.color.is_set);
    CHECK(t.rules[0].style.color.value == Colors::Red);
}

TEST(Parser_basic, class_rule) {
    Theme t;
    loadStyleString(".card { background: #FF0000; }", t);
    CHECK(t.rules.size() == 1);
    CHECK(t.rules[0].chain[0].kind == SimpleSelector::Kind::Class);
    CHECK(t.rules[0].chain[0].name == "card");
}

TEST(Parser_basic, hover_state) {
    Theme t;
    loadStyleString("Toggle:hover { background: blue; }", t);
    CHECK(t.rules.size() == 1);
    CHECK(t.rules[0].chain.size() == 1);
    CHECK(t.rules[0].chain[0].requireHover);
}

TEST(Parser_basic, descendant_selector) {
    Theme t;
    loadStyleString(".card .title { color: red; }", t);
    CHECK(t.rules.size() == 1);
    CHECK(t.rules[0].chain.size() == 2);
    CHECK(t.rules[0].chain[0].name == "card");
    CHECK(t.rules[0].chain[1].name == "title");
}

TEST(Parser_basic, part_selector) {
    Theme t;
    loadStyleString("Slider::track { background: red; }", t);
    CHECK(t.rules.size() == 1);
    CHECK(t.rules[0].part == "track");
    CHECK(t.rules[0].chain.size() == 1);
    CHECK(t.rules[0].chain[0].name == "Slider");
}

TEST(Parser_basic, comma_splits_into_two_rules) {
    Theme t;
    loadStyleString("Toggle, Checkbox { color: red; }", t);
    CHECK(t.rules.size() == 2);
}

TEST(Parser_var, substitution) {
    Theme t;
    loadStyleString(R"(
        :root { --primary: #FF0000; }
        Button { background: var(--primary); }
    )", t);
    CHECK(t.rules.size() == 1);
    auto& bg = t.rules[0].style.background;
    CHECK(bg.is_set);
    CHECK(bg.value.r == 255);
    CHECK(bg.value.g == 0);
    CHECK(bg.value.b == 0);
    CHECK(bg.value.a == 255);
}

TEST(Parser_calc, percent_minus_px) {
    Theme t;
    loadStyleString(".box { width: calc(100% - 20px); }", t);
    CHECK(t.rules.size() == 1);
    auto& w = t.rules[0].style.width;
    CHECK(w.is_set);
    CHECK(w.value.terms.size() == 2);
    CHECK_NEAR(w.value.resolveH(200, 100), 180.0f, 1e-4);
}

TEST(Parser_keyframes, percent_frames) {
    Theme t;
    loadStyleString(R"(
        @keyframes fade {
            0%   { opacity: 0; }
            100% { opacity: 1; }
        }
    )", t);
    CHECK(t.keyframes.count("fade") == 1);
    CHECK(t.keyframes.at("fade").keyframes.size() == 2);
}

TEST(Parser_keyframes, from_to) {
    Theme t;
    loadStyleString(R"(
        @keyframes fade {
            from { opacity: 0; }
            to   { opacity: 1; }
        }
    )", t);
    auto& kf = t.keyframes.at("fade");
    CHECK_NEAR(kf.keyframes[0].t, 0.0f, 1e-6);
    CHECK_NEAR(kf.keyframes[1].t, 1.0f, 1e-6);
}

TEST(Parser_comments, ignored) {
    Theme t;
    loadStyleString(R"(
        // line comment
        Toggle { color: red; } /* block comment */
    )", t);
    CHECK(t.rules.size() == 1);
}