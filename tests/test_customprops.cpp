#include "TestFramework.hpp"
#include "StyleParser.hpp"
#include "StyleAttr.hpp"
#include "Theme.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;

// ---------- Parsing: custom properties ----------

TEST(CustomProps_parse, root_stores_prop)
{
    Theme t;
    loadStyleString(":root { --primary: #FF0000; }", t);
    CHECK(t.root.customProps.count("--primary") == 1);
    CHECK(t.root.customProps.at("--primary") == "#FF0000");
}

TEST(CustomProps_parse, rule_stores_prop)
{
    Theme t;
    loadStyleString(".card { --accent: #00FF00; color: red; }", t);
    CHECK(t.rules.size() == 1);
    CHECK(t.rules[0].style.customProps.count("--accent") == 1);
    CHECK(t.rules[0].style.customProps.at("--accent") == "#00FF00");
    // La prop normale coesiste con la custom
    CHECK(t.rules[0].style.color.is_set);
}

TEST(CustomProps_parse, var_goes_into_unresolved)
{
    Theme t;
    loadStyleString("Button { background: var(--primary); }", t);
    CHECK(t.rules.size() == 1);
    CHECK(!t.rules[0].style.background.is_set);
    CHECK(t.rules[0].style.unresolvedProps.count("background") == 1);
    CHECK(t.rules[0].style.unresolvedProps.at("background") == "var(--primary)");
}

// ---------- substituteVarRefs ----------

TEST(CustomProps_substitute, simple_replacement)
{
    std::unordered_map<std::string, std::string> props{
        {"--primary", "#FF0000"}};
    CHECK(substituteVarRefs("var(--primary)", props) == "#FF0000");
}

TEST(CustomProps_substitute, unknown_left_unchanged)
{
    std::unordered_map<std::string, std::string> props{};
    CHECK(substituteVarRefs("var(--missing)", props) == "var(--missing)");
}

TEST(CustomProps_substitute, embedded_in_expression)
{
    std::unordered_map<std::string, std::string> props{
        {"--size", "100px"}};
    CHECK(substituteVarRefs("calc(var(--size) - 20px)", props) == "calc(100px - 20px)");
}

TEST(CustomProps_substitute, chained_replacement)
{
    // --a refers to --b, che ha valore concreto.
    std::unordered_map<std::string, std::string> props{
        {"--a", "var(--b)"},
        {"--b", "#00FF00"}};
    CHECK(substituteVarRefs("var(--a)", props) == "#00FF00");
}

// ---------- resolveUnresolvedProps ----------

TEST(CustomProps_resolve, color_from_var)
{
    std::unordered_map<std::string, std::string> unresolved{
        {"background", "var(--primary)"}};
    std::unordered_map<std::string, std::string> props{
        {"--primary", "#FF0000"}};
    Style out = resolveUnresolvedProps(unresolved, props);
    CHECK(out.background.is_set);
    CHECK((out.background.value == Color{0xFF, 0x00, 0x00, 0xFF}));
}

TEST(CustomProps_resolve, width_from_var)
{
    std::unordered_map<std::string, std::string> unresolved{
        {"width", "var(--w)"}};
    std::unordered_map<std::string, std::string> props{
        {"--w", "200px"}};
    Style out = resolveUnresolvedProps(unresolved, props);
    CHECK(out.width.is_set);
    CHECK_NEAR(out.width.value.resolveH(0, 0), 200.0f, 1e-4);
}

TEST(CustomProps_resolve, unknown_var_leaves_unset)
{
    std::unordered_map<std::string, std::string> unresolved{
        {"background", "var(--missing)"}};
    std::unordered_map<std::string, std::string> props{};
    Style out = resolveUnresolvedProps(unresolved, props);
    // parseColorToken fallisce su "var(--missing)" → background non set
    CHECK(!out.background.is_set);
}

// ---------- Ereditarietà via ComputedStyle::from ----------

TEST(CustomProps_inherit, from_parent)
{
    ComputedStyle parent;
    parent.customProps["--primary"] = "#FF0000";

    Style local; // nessuna custom prop
    auto c = ComputedStyle::from(local, &parent, nullptr);

    CHECK(c.customProps.count("--primary") == 1);
    CHECK(c.customProps.at("--primary") == "#FF0000");
}

TEST(CustomProps_inherit, local_overrides_parent)
{
    ComputedStyle parent;
    parent.customProps["--primary"] = "#FF0000";

    Style local;
    local.customProps["--primary"] = "#0000FF";

    auto c = ComputedStyle::from(local, &parent, nullptr);
    CHECK(c.customProps.at("--primary") == "#0000FF");
}

TEST(CustomProps_inherit, merges_parent_and_local)
{
    ComputedStyle parent;
    parent.customProps["--a"] = "1";

    Style local;
    local.customProps["--b"] = "2";

    auto c = ComputedStyle::from(local, &parent, nullptr);
    CHECK(c.customProps.size() == 2);
    CHECK(c.customProps.at("--a") == "1");
    CHECK(c.customProps.at("--b") == "2");
}