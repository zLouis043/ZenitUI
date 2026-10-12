#include "TestFramework.hpp"
#include "Theme.hpp"
#include "StyleParser.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;

// ============================================================
//  Theme index (rulesByTag / rulesNoTag)
// ============================================================

// ---- bucket population ----------------------------------------------

TEST(ThemeIndex, tag_rule_goes_to_tag_bucket)
{
    Theme t;
    loadStyleString("Button { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.count("Button") == 1);
    CHECK(t.rulesByTag.at("Button").size() == 1);
    CHECK(t.rulesByTag.at("Button")[0] == 0);
    CHECK(t.rulesNoTag.empty());
}

TEST(ThemeIndex, class_rule_goes_to_no_tag_bucket)
{
    Theme t;
    loadStyleString(".card { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.empty());
    CHECK(t.rulesNoTag.size() == 1);
    CHECK(t.rulesNoTag[0] == 0);
}

TEST(ThemeIndex, id_rule_goes_to_no_tag_bucket)
{
    Theme t;
    loadStyleString("#save { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.empty());
    CHECK(t.rulesNoTag.size() == 1);
}

TEST(ThemeIndex, compound_with_tag_is_indexed_under_tag)
{
    Theme t;
    loadStyleString("Button.btn-primary { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.count("Button") == 1);
    CHECK(t.rulesByTag.at("Button").size() == 1);
    CHECK(t.rulesNoTag.empty());
}

TEST(ThemeIndex, compound_without_tag_goes_to_no_tag_bucket)
{
    Theme t;
    loadStyleString(".card.elevated { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.empty());
    CHECK(t.rulesNoTag.size() == 1);
}

TEST(ThemeIndex, state_on_tag_still_indexed_under_tag)
{
    Theme t;
    loadStyleString("Button:hover { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.count("Button") == 1);
    CHECK(t.rulesNoTag.empty());
}

TEST(ThemeIndex, state_on_class_goes_to_no_tag_bucket)
{
    Theme t;
    loadStyleString(".btn:hover { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.empty());
    CHECK(t.rulesNoTag.size() == 1);
}

TEST(ThemeIndex, descendant_rule_uses_last_selector_for_indexing)
{
    Theme t;
    // .card Button → rightmost is Button → indexed under "Button"
    loadStyleString(".card Button { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.count("Button") == 1);
    CHECK(t.rulesNoTag.empty());
}

TEST(ThemeIndex, descendant_rule_with_class_rightmost)
{
    Theme t;
    // "Button .title" (spazio) → descendant con rightmost .title → no tag.
    // ("Button.title" senza spazio sarebbe un compound con tag, non
    // un descendant; è coperto da compound_with_tag_is_indexed_under_tag.)
    loadStyleString("Button .title { color: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.empty());
    CHECK(t.rulesNoTag.size() == 1);
}

TEST(ThemeIndex, part_rule_is_not_indexed)
{
    Theme t;
    loadStyleString("Slider::track { background: red; }", t);

    CHECK(t.rules.size() == 1);
    CHECK(t.rulesByTag.empty());
    CHECK(t.rulesNoTag.empty());
}

TEST(ThemeIndex, multiple_rules_same_tag_accumulate)
{
    Theme t;
    loadStyleString(R"(
        Button { color: red; }
        Button:hover { color: green; }
        Button.btn-primary { color: blue; }
    )", t);

    CHECK(t.rules.size() == 3);
    CHECK(t.rulesByTag.count("Button") == 1);
    CHECK(t.rulesByTag.at("Button").size() == 3);
    CHECK(t.rulesByTag.at("Button")[0] == 0);
    CHECK(t.rulesByTag.at("Button")[1] == 1);
    CHECK(t.rulesByTag.at("Button")[2] == 2);
    CHECK(t.rulesNoTag.empty());
}

TEST(ThemeIndex, mixed_rules_distribute_correctly)
{
    Theme t;
    loadStyleString(R"(
        Button { color: red; }
        .card { color: green; }
        Toggle { color: blue; }
        #save { color: yellow; }
    )", t);

    CHECK(t.rules.size() == 4);

    CHECK(t.rulesByTag.count("Button") == 1);
    CHECK(t.rulesByTag.at("Button").size() == 1);
    CHECK(t.rulesByTag.at("Button")[0] == 0);

    CHECK(t.rulesByTag.count("Toggle") == 1);
    CHECK(t.rulesByTag.at("Toggle").size() == 1);
    CHECK(t.rulesByTag.at("Toggle")[0] == 2);

    CHECK(t.rulesNoTag.size() == 2);
    CHECK(t.rulesNoTag[0] == 1);   // .card
    CHECK(t.rulesNoTag[1] == 3);   // #save
}

// ---- clear ----------------------------------------------------------

TEST(ThemeIndex, clear_empties_index)
{
    Theme t;
    loadStyleString("Button { color: red; } .card { color: blue; }", t);

    CHECK(!t.rulesByTag.empty());
    CHECK(!t.rulesNoTag.empty());

    t.clear();

    CHECK(t.rules.empty());
    CHECK(t.rulesByTag.empty());
    CHECK(t.rulesNoTag.empty());
    CHECK(t.keyframes.empty());
}

// ---- rule order matches index entries ------------------------------

TEST(ThemeIndex, index_entries_point_to_valid_rules)
{
    Theme t;
    loadStyleString(R"(
        Button { color: red; }
        .card { color: green; }
        Toggle { color: blue; }
    )", t);

    // Every index points to a rule with the expected rightmost tag.
    for (const auto &[tag, idxs] : t.rulesByTag)
    {
        for (size_t idx : idxs)
        {
            CHECK(idx < t.rules.size());
            const auto &r = t.rules[idx];
            CHECK(!r.chain.empty());
            CHECK(r.chain.back().kind == SimpleSelector::Kind::Tag);
            CHECK(r.chain.back().name == tag);
        }
    }

    for (size_t idx : t.rulesNoTag)
    {
        CHECK(idx < t.rules.size());
        const auto &r = t.rules[idx];
        CHECK(!r.chain.empty());
        CHECK(r.chain.back().kind != SimpleSelector::Kind::Tag);
    }
}