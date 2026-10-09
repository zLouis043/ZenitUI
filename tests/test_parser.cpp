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
    CHECK(!t.rules[0].style.background.is_set);
    CHECK(t.rules[0].style.unresolvedProps.count("background") > 0);
    CHECK(t.rules[0].style.unresolvedProps.at("background") == "var(--primary)");
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

TEST(Parser_basic, compound_state_checked_hover) {
    Theme t;
    loadStyleString("Toggle:checked:hover { background: red; }", t);
    CHECK(t.rules.size() == 1);
    CHECK(t.rules[0].chain.size() == 1);
    CHECK(t.rules[0].chain[0].requireChecked);
    CHECK(t.rules[0].chain[0].requireHover);
}

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

TEST(Parser_boxshadow, x_y_blur_color) {
    Theme t;
    loadStyleString(".card { box-shadow: 4px 4px 8px #00000080; }", t);
    CHECK(t.rules.size() == 1);
    const auto& s = t.rules[0].style.boxShadow;
    CHECK(s.is_set);
    CHECK(s.value.enabled);
    CHECK_NEAR(s.value.x.resolveSelfH(0, 0), 4.0f, 1e-4);
    CHECK_NEAR(s.value.y.resolveSelfV(0, 0), 4.0f, 1e-4);
    CHECK_NEAR(s.value.blur.resolveSelfH(0, 0), 8.0f, 1e-4);
    CHECK((s.value.color == Color{0, 0, 0, 0x80}));
}

TEST(Parser_boxshadow, default_color_when_missing) {
    Theme t;
    loadStyleString(".card { box-shadow: 2px 2px 4px; }", t);
    const auto& s = t.rules[0].style.boxShadow;
    CHECK(s.is_set);
    CHECK((s.value.color == Color{0, 0, 0, 128}));
}

TEST(Parser_boxshadow, not_set_if_only_two_tokens) {
    Theme t;
    loadStyleString(".card { box-shadow: 2px 2px; }", t);
    CHECK(!t.rules[0].style.boxShadow.is_set);
}

// ---------- Compound selectors ----------

TEST(Parser_compound, tag_with_class) {
    Theme t;
    loadStyleString("Button.btn-primary { color: red; }", t);
    CHECK(t.rules.size() == 1);
    CHECK(t.rules[0].chain.size() == 1);

    const auto& ss = t.rules[0].chain[0];
    CHECK(ss.kind == SimpleSelector::Kind::Tag);
    CHECK(ss.name == "Button");
    CHECK(ss.extras.size() == 1);
    CHECK(ss.extras[0].first  == SimpleSelector::Kind::Class);
    CHECK(ss.extras[0].second == "btn-primary");
}

TEST(Parser_compound, class_and_class) {
    Theme t;
    loadStyleString(".card.elevated { color: red; }", t);
    const auto& ss = t.rules[0].chain[0];
    CHECK(ss.kind == SimpleSelector::Kind::Class);
    CHECK(ss.name == "card");
    CHECK(ss.extras.size() == 1);
    CHECK(ss.extras[0].first  == SimpleSelector::Kind::Class);
    CHECK(ss.extras[0].second == "elevated");
}

TEST(Parser_compound, tag_with_id) {
    Theme t;
    loadStyleString("Button#save { color: red; }", t);
    const auto& ss = t.rules[0].chain[0];
    CHECK(ss.kind == SimpleSelector::Kind::Tag);
    CHECK(ss.name == "Button");
    CHECK(ss.extras.size() == 1);
    CHECK(ss.extras[0].first  == SimpleSelector::Kind::Id);
    CHECK(ss.extras[0].second == "save");
}

TEST(Parser_compound, three_components) {
    Theme t;
    loadStyleString("Button.btn-primary#save { color: red; }", t);
    const auto& ss = t.rules[0].chain[0];
    CHECK(ss.kind == SimpleSelector::Kind::Tag);
    CHECK(ss.name == "Button");
    CHECK(ss.extras.size() == 2);
    CHECK(ss.extras[0].first  == SimpleSelector::Kind::Class);
    CHECK(ss.extras[0].second == "btn-primary");
    CHECK(ss.extras[1].first  == SimpleSelector::Kind::Id);
    CHECK(ss.extras[1].second == "save");
}

TEST(Parser_compound, tag_class_with_state) {
    Theme t;
    loadStyleString("Button.btn-primary:hover { color: red; }", t);
    const auto& ss = t.rules[0].chain[0];
    CHECK(ss.kind == SimpleSelector::Kind::Tag);
    CHECK(ss.name == "Button");
    CHECK(ss.extras.size() == 1);
    CHECK(ss.extras[0].second == "btn-primary");
    CHECK(ss.requireHover);
}

TEST(Parser_compound, descendant_of_compound) {
    Theme t;
    loadStyleString(".card Button.btn-primary { color: red; }", t);
    CHECK(t.rules[0].chain.size() == 2);
    CHECK(t.rules[0].chain[0].kind == SimpleSelector::Kind::Class);
    CHECK(t.rules[0].chain[0].name == "card");
    CHECK(t.rules[0].chain[1].kind == SimpleSelector::Kind::Tag);
    CHECK(t.rules[0].chain[1].name == "Button");
    CHECK(t.rules[0].chain[1].extras.size() == 1);
    CHECK(t.rules[0].chain[1].extras[0].second == "btn-primary");
}