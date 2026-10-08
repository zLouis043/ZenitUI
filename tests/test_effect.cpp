#include "TestFramework.hpp"
#include "StyleParser.hpp"
#include "Theme.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;

TEST(Parser_effect, simple_name) {
    Theme t;
    loadStyleString(".btn { effect: hueShift; }", t);
    CHECK(t.rules.size() == 1);
    const auto& e = t.rules[0].style.effect;
    CHECK(e.is_set);
    CHECK(e.value == "hueShift");
}

TEST(Parser_effect, empty_not_set) {
    Theme t;
    loadStyleString(".btn { color: red; }", t);
    CHECK(!t.rules[0].style.effect.is_set);
}

TEST(Parser_effect, coexists_with_filter) {
    Theme t;
    loadStyleString(".btn { effect: hueShift; filter: blur(2px); }", t);
    CHECK(t.rules[0].style.effect.is_set);
    CHECK(t.rules[0].style.effect.value == "hueShift");
    CHECK(t.rules[0].style.filters.is_set);
    CHECK(t.rules[0].style.filters.value.size() == 1);
}