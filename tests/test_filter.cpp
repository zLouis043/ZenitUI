#include "TestFramework.hpp"
#include "StyleParser.hpp"
#include "Theme.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;

TEST(Parser_filter, single_blur) {
    Theme t;
    loadStyleString(".card { filter: blur(4px); }", t);
    CHECK(t.rules.size() == 1);
    const auto& f = t.rules[0].style.filters;
    CHECK(f.is_set);
    CHECK(f.value.size() == 1);
    CHECK(f.value[0].name == "blur");
    CHECK(f.value[0].args.size() == 1);
    CHECK(f.value[0].args[0] == "4px");
}

TEST(Parser_filter, no_args) {
    Theme t;
    loadStyleString(".card { filter: grayscale; }", t);
    const auto& f = t.rules[0].style.filters;
    CHECK(f.is_set);
    CHECK(f.value.size() == 1);
    CHECK(f.value[0].name == "grayscale");
    CHECK(f.value[0].args.empty());
}

TEST(Parser_filter, comma_separated_two) {
    Theme t;
    loadStyleString(".card { filter: blur(2px), drop-shadow(2px, 2px, #000); }", t);
    const auto& f = t.rules[0].style.filters;
    CHECK(f.is_set);
    CHECK(f.value.size() == 2);

    CHECK(f.value[0].name == "blur");
    CHECK(f.value[0].args.size() == 1);
    CHECK(f.value[0].args[0] == "2px");

    CHECK(f.value[1].name == "drop-shadow");
    CHECK(f.value[1].args.size() == 3);
    CHECK(f.value[1].args[0] == "2px");
    CHECK(f.value[1].args[1] == "2px");
    CHECK(f.value[1].args[2] == "#000");
}

TEST(Parser_filter, empty_value_leaves_empty_vector) {
    Theme t;
    loadStyleString(".card { filter: ; }", t);
    // filters è set ma vuoto, o non set; in entrambi i casi il valore è []
    if (t.rules[0].style.filters.is_set)
        CHECK(t.rules[0].style.filters.value.empty());
    else
        CHECK(true);
}