#include "TestFramework.hpp"
#include "Theme.hpp"

using namespace ZenitUI;

static SimpleSelector tagSel(const char* n) {
    SimpleSelector s; s.kind = SimpleSelector::Kind::Tag;   s.name = n; return s;
}
static SimpleSelector classSel(const char* n) {
    SimpleSelector s; s.kind = SimpleSelector::Kind::Class; s.name = n; return s;
}
static SimpleSelector idSel(const char* n) {
    SimpleSelector s; s.kind = SimpleSelector::Kind::Id;    s.name = n; return s;
}

// ---------- computeSpecificity ----------

TEST(Theme_specificity, empty_is_zero) {
    std::vector<SimpleSelector> chain;
    auto s = computeSpecificity(chain);
    CHECK(s.ids == 0 && s.classes == 0 && s.tags == 0);
}

TEST(Theme_specificity, single_tag) {
    auto s = computeSpecificity({ tagSel("Toggle") });
    CHECK(s.ids == 0 && s.classes == 0 && s.tags == 1);
}

TEST(Theme_specificity, single_class) {
    auto s = computeSpecificity({ classSel("card") });
    CHECK(s.ids == 0 && s.classes == 1 && s.tags == 0);
}

TEST(Theme_specificity, single_id) {
    auto s = computeSpecificity({ idSel("main") });
    CHECK(s.ids == 1 && s.classes == 0 && s.tags == 0);
}

TEST(Theme_specificity, hover_weighs_as_class) {
    SimpleSelector s = tagSel("Toggle");
    s.requireHover = true;
    auto spec = computeSpecificity({ s });
    CHECK(spec.classes == 1 && spec.tags == 1);
}

TEST(Theme_specificity, checked_weighs_as_class) {
    SimpleSelector s = tagSel("Toggle");
    s.requireChecked = true;
    auto spec = computeSpecificity({ s });
    CHECK(spec.classes == 1 && spec.tags == 1);
}

TEST(Theme_specificity, descendant_accumulates) {
    auto spec = computeSpecificity({ classSel("card"), classSel("title") });
    CHECK(spec.classes == 2);
}

TEST(Theme_specificity, mixed_lexicographic) {
    auto spec = computeSpecificity({ idSel("main"), classSel("btn"), tagSel("Text") });
    CHECK(spec.ids == 1 && spec.classes == 1 && spec.tags == 1);
}

TEST(Theme_specificity, one_id_beats_many_classes) {
    std::vector<SimpleSelector> oneId = { idSel("x") };
    std::vector<SimpleSelector> many;
    for (int i = 0; i < 100; ++i) many.push_back(classSel("c"));
    CHECK(computeSpecificity(many) < computeSpecificity(oneId));
}

// ---------- addRule / addKeyframes / clear ----------

TEST(Theme_addRule, assigns_specificity) {
    Theme t;
    ThemeRule r; r.chain = { classSel("card") };
    t.addRule(r);
    CHECK(t.rules.size() == 1);
    auto s = t.rules[0].specificity;
    CHECK(s.ids == 0 && s.classes == 1 && s.tags == 0);
}

TEST(Theme_addRule, assigns_sequential_order) {
    Theme t;
    ThemeRule a; a.chain = { tagSel("A") };
    ThemeRule b; b.chain = { tagSel("B") };
    ThemeRule c; c.chain = { tagSel("C") };
    t.addRule(a); t.addRule(b); t.addRule(c);
    CHECK(t.rules[0].order == 0);
    CHECK(t.rules[1].order == 1);
    CHECK(t.rules[2].order == 2);
}

TEST(Theme_clear, resets_all) {
    Theme t;
    ThemeRule r; r.chain = { tagSel("X") };
    t.addRule(r);
    KeyframeAnimation kf; kf.name = "k";
    t.addKeyframes(kf);
    t.root.opacity = 0.5f;
    t.clear();
    CHECK(t.rules.empty());
    CHECK(t.keyframes.empty());
    CHECK(!t.root.opacity.is_set);
}

// ---------- Specificity with compound selectors ----------

TEST(Theme_specificity, tag_plus_class) {
    SimpleSelector ss;
    ss.kind = SimpleSelector::Kind::Tag;
    ss.name = "Button";
    ss.extras = {{SimpleSelector::Kind::Class, "btn-primary"}};

    auto spec = computeSpecificity({ ss });
    CHECK(spec.ids == 0);
    CHECK(spec.classes == 1);
    CHECK(spec.tags == 1);
}

TEST(Theme_specificity, class_plus_class) {
    SimpleSelector ss;
    ss.kind = SimpleSelector::Kind::Class;
    ss.name = "card";
    ss.extras = {{SimpleSelector::Kind::Class, "elevated"}};

    auto spec = computeSpecificity({ ss });
    CHECK(spec.classes == 2);
}

TEST(Theme_specificity, tag_plus_class_beats_class) {
    SimpleSelector compound;
    compound.kind = SimpleSelector::Kind::Tag;
    compound.name = "Button";
    compound.extras = {{SimpleSelector::Kind::Class, "primary"}};

    SimpleSelector simple;
    simple.kind = SimpleSelector::Kind::Class;
    simple.name = "primary";

    CHECK(computeSpecificity({simple}) < computeSpecificity({compound}));
}