#include "TestFramework.hpp"
#include "Style.hpp"

using namespace ZenitUI;


TEST(Style_Opt, default_not_set) {
    Opt<float> o;
    CHECK(!o.is_set);
    CHECK_NEAR(o.get_or(42.0f), 42.0f, 1e-6);
}

TEST(Style_Opt, set_via_ctor) {
    Opt<float> o(3.14f);
    CHECK(o.is_set);
    CHECK_NEAR(o.value, 3.14f, 1e-6);
    CHECK_NEAR(o.get_or(0.0f), 3.14f, 1e-6);
}

TEST(Style_Opt, set_via_assign) {
    Opt<int> o;
    o = 7;
    CHECK(o.is_set);
    CHECK(o.value == 7);
}

TEST(Style_Opt, reset) {
    Opt<int> o(5);
    o.reset();
    CHECK(!o.is_set);
}


TEST(Style_overlay, empty_overlay_does_nothing) {
    Style a;
    a.background = Colors::Red;
    Style b;
    a.overlay(b);
    CHECK(a.background.is_set);
    CHECK(a.background.value == Colors::Red);
}

TEST(Style_overlay, overlay_wins_if_set) {
    Style a;
    a.background = Colors::Red;
    Style b;
    b.background = Colors::Blue;
    a.overlay(b);
    CHECK(a.background.value == Colors::Blue);
}

TEST(Style_overlay, overlay_does_not_clear_unset) {
    Style a;
    a.background  = Colors::Red;
    a.color       = Colors::White;
    Style b;
    b.background  = Colors::Blue;
    a.overlay(b);
    CHECK(a.background.value == Colors::Blue);
    CHECK(a.color.is_set);
    CHECK(a.color.value == Colors::White);
}


TEST(Style_inherited, known_props) {
    CHECK(isInheritedProp("font"));
    CHECK(isInheritedProp("fontSize"));
    CHECK(isInheritedProp("color"));
    CHECK(isInheritedProp("letterSpacing"));
    CHECK(isInheritedProp("textAlign"));
}

TEST(Style_inherited, non_inherited) {
    CHECK(!isInheritedProp("background"));
    CHECK(!isInheritedProp("width"));
    CHECK(!isInheritedProp("padding"));
    CHECK(!isInheritedProp("margin"));
    CHECK(!isInheritedProp("opacity"));
}


TEST(Style_from, local_wins_over_parent_and_root) {
    Style local;
    local.color = Colors::Red;

    ComputedStyle parent;
    parent.color = Colors::Green;

    Style root;
    root.color = Colors::Blue;

    auto c = ComputedStyle::from(local, &parent, &root);
    CHECK(c.color == Colors::Red);
}

TEST(Style_from, parent_wins_for_inherited) {
    Style local;
    ComputedStyle parent;
    parent.color = Colors::Green;

    Style root;
    root.color = Colors::Blue;

    auto c = ComputedStyle::from(local, &parent, &root);
    CHECK(c.color == Colors::Green);
}

TEST(Style_from, root_used_when_parent_no_inherit) {
    Style local;
    ComputedStyle parent;
    Style root;
    root.color = Colors::Blue;

    auto c = ComputedStyle::from(local, &parent, &root);
    CHECK(c.color == parent.color);
}

TEST(Style_from, root_only) {
    Style local;
    Style root;
    root.color = Colors::Blue;

    auto c = ComputedStyle::from(local, nullptr, &root);
    CHECK(c.color == Colors::Blue);
}

TEST(Style_from, no_parent_no_root_uses_default) {
    Style local;
    auto c = ComputedStyle::from(local, nullptr, nullptr);
    CHECK(c.color == Colors::White);
    CHECK(c.background == Colors::Blank);
}

TEST(Style_from, non_inherited_ignores_parent) {
    Style local;
    ComputedStyle parent;
    parent.background = Colors::Green;

    Style root;

    auto c = ComputedStyle::from(local, &parent, &root);
    CHECK(c.background == Colors::Blank);
}


TEST(Style_eq, two_default_are_equal) {
    ComputedStyle a, b;
    CHECK(a == b);
}

TEST(Style_eq, differ_on_color) {
    ComputedStyle a, b;
    a.color = Colors::Red;
    b.color = Colors::Blue;
    CHECK(a != b);
}


TEST(Style_lerp, float) {
    CHECK_NEAR(lerpProp(0.0f, 10.0f, 0.5f), 5.0f, 1e-6);
}

TEST(Style_lerp, color) {
    Color a{ 0,   0,   0,   0   };
    Color b{ 100, 200, 100, 100 };
    Color c = lerpProp(a, b, 0.5f);
    CHECK(c.r == 50);
    CHECK(c.g == 100);
    CHECK(c.b == 50);
    CHECK(c.a == 50);
}

TEST(Style_lerp, value_same_shape) {
    Value a = Px(0.0f);
    Value b = Px(100.0f);
    Value c = lerpProp(a, b, 0.5f);
    CHECK_NEAR(c.resolveH(0, 0), 50.0f, 1e-6);
}

TEST(Style_lerp, value_different_shapes_unions_units) {
    Value a = PH(20.0f);
    Value b = PW(100.0f) - PH(80.0f);
    Value mid = lerpProp(a, b, 0.5f);

    CHECK_NEAR(mid.resolveH(100, 50), 35.0f, 1e-4);
}

TEST(Style_lerp, value_auto_snaps) {
    Value a = Value::autoSize();
    Value b = Px(100.0f);
    CHECK(lerpProp(a, b, 0.0f) == a);
    CHECK(lerpProp(a, b, 1.0f) == b);
    CHECK(lerpProp(a, b, 0.5f) == b);
}

TEST(Style_lerp, spacing) {
    Spacing a(Px(0.0f));
    Spacing b(Px(10.0f));
    Spacing c = lerpProp(a, b, 0.5f);
    CHECK_NEAR(c.top.resolveV(0, 0),    5.0f, 1e-6);
    CHECK_NEAR(c.right.resolveH(0, 0),  5.0f, 1e-6);
    CHECK_NEAR(c.bottom.resolveV(0, 0), 5.0f, 1e-6);
    CHECK_NEAR(c.left.resolveH(0, 0),   5.0f, 1e-6);
}

TEST(Style_lerp, enum_snaps) {
    CHECK(lerpProp(Align::Start, Align::Center, 0.0f) == Align::Start);
    CHECK(lerpProp(Align::Start, Align::Center, 0.5f) == Align::Center);
    CHECK(lerpProp(Align::Start, Align::Center, 1.0f) == Align::Center);
}


TEST(Style_lerpStyle, color_and_opacity) {
    ComputedStyle a, b;
    a.color   = Colors::Black;
    b.color   = Colors::White;
    a.opacity = 0.0f;
    b.opacity = 1.0f;

    auto c = lerpStyle(a, b, 0.5f);
    CHECK_NEAR(c.opacity, 0.5f, 1e-6);
    CHECK(c.color.r == 127 || c.color.r == 128);
}


TEST(Style_lerpTimed, no_transitions_falls_to_global) {
    ComputedStyle a, b;
    a.opacity = 0.0f;
    b.opacity = 1.0f;
    b.transitionTime = 0.5f;
    b.ease = TransitionFunction::Linear;

    auto mid = lerpStyleTimed(a, b, 0.25f);
    CHECK_NEAR(mid.opacity, 0.5f, 1e-4);
}

TEST(Style_lerpTimed, per_property_override) {
    ComputedStyle a, b;
    a.opacity = 0.0f;
    b.opacity = 1.0f;
    b.transitionTime = 1.0f;
    b.ease = TransitionFunction::Linear;

    TransitionSpec spec;
    spec.prop     = "opacity";
    spec.duration = 0.5f;
    spec.ease     = TransitionFunction::Linear;
    b.transitions.push_back(spec);

    auto mid = lerpStyleTimed(a, b, 0.25f);
    CHECK_NEAR(mid.opacity, 0.5f, 1e-4);

    auto done = lerpStyleTimed(a, b, 0.5f);
    CHECK_NEAR(done.opacity, 1.0f, 1e-4);
}


TEST(Style_overlayComputed, updates_only_set_fields) {
    ComputedStyle c;
    c.color = Colors::Red;

    Style s;
    s.background = Colors::Blue;

    overlayComputed(c, s);
    CHECK(c.color == Colors::Red);
    CHECK(c.background == Colors::Blue);
}

TEST(Style_overlayComputed, empty_style_no_change) {
    ComputedStyle c;
    c.color = Colors::Red;
    Style empty;

    overlayComputed(c, empty);
    CHECK(c.color == Colors::Red);
}


TEST(Style_propHelpers, forEach_visits_set_only) {
    Style s;
    s.color = Colors::Red;
    s.font  = "mont";

    int count = 0;
    forEachSetStyleProp(s, [&](const char* /*name*/) { count++; });
    CHECK(count == 2);
}

TEST(Style_propHelpers, hasStyleProp) {
    Style s;
    s.color = Colors::Red;
    CHECK(hasStyleProp(s, "color"));
    CHECK(!hasStyleProp(s, "background"));
}

TEST(Style_propHelpers, copyStyleProp) {
    Style src;
    src.color = Colors::Red;

    Style dst;
    CHECK(copyStyleProp(dst, src, "color"));
    CHECK(dst.color.is_set);
    CHECK(dst.color.value == Colors::Red);

    CHECK(!copyStyleProp(dst, src, "background"));
    CHECK(!dst.background.is_set);
}


TEST(Style_stylesDiffer, empty_equal) {
    Style a, b;
    CHECK(!stylesDiffer(a, b));
}

TEST(Style_stylesDiffer, differs_on_any_set) {
    Style a;
    a.color = Colors::Red;
    Style b;
    CHECK(stylesDiffer(a, b));
}

TEST(Style_stylesDiffer, differs_on_value) {
    Style a; a.color = Colors::Red;
    Style b; b.color = Colors::Blue;
    CHECK(stylesDiffer(a, b));
}

TEST(Style_stylesDiffer, same_shape_same_values) {
    Style a; a.color = Colors::Red; a.opacity = 0.5f;
    Style b; b.color = Colors::Red; b.opacity = 0.5f;
    CHECK(!stylesDiffer(a, b));
}

TEST(Style_stylesDiffer, animations_are_compared) {
    Style a;
    AnimationRef r1; r1.name = "glow"; r1.duration = 1.0f;
    a.animations = std::vector<AnimationRef>{ r1 };

    Style b;
    AnimationRef r2; r2.name = "glow"; r2.duration = 2.0f;
    b.animations = std::vector<AnimationRef>{ r2 };

    CHECK(stylesDiffer(a, b));
}


TEST(Style_lerpStyleParts, both_set_is_interpolated) {
    Style a; a.opacity = 0.0f;
    Style b; b.opacity = 1.0f;

    Style r = lerpStyleParts(a, b, 0.5f);
    CHECK(r.opacity.is_set);
    CHECK_NEAR(r.opacity.value, 0.5f, 1e-6);
}

TEST(Style_lerpStyleParts, only_target_set_snaps_to_target) {
    Style a;
    Style b; b.opacity = 1.0f;

    Style r = lerpStyleParts(a, b, 0.5f);
    CHECK(r.opacity.is_set);
    CHECK_NEAR(r.opacity.value, 1.0f, 1e-6);
}

TEST(Style_lerpStyleParts, only_source_set_snaps_to_target_unset) {
    Style a; a.opacity = 1.0f;
    Style b;

    Style r = lerpStyleParts(a, b, 0.5f);
    CHECK(!r.opacity.is_set);
}


TEST(Style_customProps, overlay_merges) {
    Style a;
    a.customProps["--a"] = "1";
    Style b;
    b.customProps["--b"] = "2";
    a.overlay(b);
    CHECK(a.customProps.size() == 2);
    CHECK(a.customProps.at("--a") == "1");
    CHECK(a.customProps.at("--b") == "2");
}

TEST(Style_customProps, overlay_same_key_wins) {
    Style a;
    a.customProps["--x"] = "red";
    Style b;
    b.customProps["--x"] = "blue";
    a.overlay(b);
    CHECK(a.customProps.at("--x") == "blue");
}

TEST(Style_unresolvedProps, overlay_clears_typed) {
    Style a;
    a.background = Colors::Red;
    CHECK(a.background.is_set);

    Style b;
    b.unresolvedProps["background"] = "var(--x)";
    a.overlay(b);

    CHECK(!a.background.is_set);
    CHECK(a.unresolvedProps.count("background") == 1);
}


TEST(Style_computedValue, visible_x_with_scroll_y_becomes_auto) {
    Style s;
    s.overflowX = Overflow::Visible;
    s.overflowY = Overflow::Scroll;
    auto c = ComputedStyle::from(s, nullptr, nullptr);
    CHECK(c.overflowX == Overflow::Auto);
    CHECK(c.overflowY == Overflow::Scroll);
}

TEST(Style_computedValue, visible_y_with_scroll_x_becomes_auto) {
    Style s;
    s.overflowX = Overflow::Scroll;
    s.overflowY = Overflow::Visible;
    auto c = ComputedStyle::from(s, nullptr, nullptr);
    CHECK(c.overflowX == Overflow::Scroll);
    CHECK(c.overflowY == Overflow::Auto);
}

TEST(Style_computedValue, both_visible_stay_visible) {
    Style s;
    s.overflowX = Overflow::Visible;
    s.overflowY = Overflow::Visible;
    auto c = ComputedStyle::from(s, nullptr, nullptr);
    CHECK(c.overflowX == Overflow::Visible);
    CHECK(c.overflowY == Overflow::Visible);
}

TEST(Style_computedValue, both_scroll_stay_scroll) {
    Style s;
    s.overflowX = Overflow::Scroll;
    s.overflowY = Overflow::Scroll;
    auto c = ComputedStyle::from(s, nullptr, nullptr);
    CHECK(c.overflowX == Overflow::Scroll);
    CHECK(c.overflowY == Overflow::Scroll);
}

TEST(Style_isInheritedProp, all_known_props)
{
    CHECK(isInheritedProp("font"));
    CHECK(isInheritedProp("fontSize"));
    CHECK(isInheritedProp("color"));
    CHECK(isInheritedProp("letterSpacing"));
    CHECK(isInheritedProp("textAlign"));
}

TEST(Style_isInheritedProp, same_length_non_inherited)
{
    CHECK(!isInheritedProp("width"));
    CHECK(!isInheritedProp("grow"));
    CHECK(!isInheritedProp("minWidth"));
    CHECK(!isInheritedProp("maxWidth"));
    CHECK(!isInheritedProp("minHeight"));
    CHECK(!isInheritedProp("maxHeight"));
    CHECK(!isInheritedProp("position"));
    CHECK(!isInheritedProp("overflow"));
    CHECK(!isInheritedProp("tint"));
    CHECK(!isInheritedProp("left"));
}

TEST(Style_isInheritedProp, empty_and_unknown)
{
    CHECK(!isInheritedProp(""));
    CHECK(!isInheritedProp("nonexistent"));
    CHECK(!isInheritedProp("Font"));
}