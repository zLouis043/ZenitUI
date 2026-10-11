#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  Layout: addClass / removeClass / hasClass / clearStyleClasses
// ============================================================

// ---- addClass --------------------------------------------------------

TEST(Widget_classes, addClass_appends)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);

    node->addClass("a");
    node->addClass("b");

    const auto& classes = node->getStyleClasses();
    CHECK((int)classes.size() == 2);
    CHECK(classes[0] == "a");
    CHECK(classes[1] == "b");
}

TEST(Widget_classes, addClass_avoids_duplicates)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);

    node->addClass("a");
    node->addClass("a");
    node->addClass("a");

    CHECK((int)node->getStyleClasses().size() == 1);
    CHECK(node->getStyleClasses()[0] == "a");
}

// ---- hasClass --------------------------------------------------------

TEST(Widget_classes, hasClass_true_when_present)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->addClass("card");
    node->addClass("elevated");

    CHECK(node->hasClass("card"));
    CHECK(node->hasClass("elevated"));
}

TEST(Widget_classes, hasClass_false_when_absent)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->addClass("card");

    CHECK(!node->hasClass("elevated"));
    CHECK(!node->hasClass(""));
}

// ---- removeClass -----------------------------------------------------

TEST(Widget_classes, removeClass_removes_present)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->addClass("a");
    node->addClass("b");
    node->addClass("c");

    node->removeClass("b");

    CHECK(!node->hasClass("b"));
    CHECK(node->hasClass("a"));
    CHECK(node->hasClass("c"));
    CHECK((int)node->getStyleClasses().size() == 2);
}

TEST(Widget_classes, removeClass_noop_when_absent)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->addClass("a");

    node->removeClass("missing");

    CHECK((int)node->getStyleClasses().size() == 1);
    CHECK(node->hasClass("a"));
}

TEST(Widget_classes, removeClass_removes_all_occurrences)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);

    // addClass deduplica, quindi per forzare dei duplicati
    // scriviamo direttamente nel vettore via getStyleClasses().
    // (getStyleClasses() ritorna const&, quindi non possiamo;
    // usiamo addClass e poi verifichiamo che non ci siano duplicati
    // già in partenza.)
    node->addClass("dup");
    node->addClass("other");
    node->addClass("dup");   // ignorato

    CHECK((int)node->getStyleClasses().size() == 2);

    node->removeClass("dup");
    CHECK(!node->hasClass("dup"));
    CHECK(node->hasClass("other"));
    CHECK((int)node->getStyleClasses().size() == 1);
}

// ---- clearStyleClasses -----------------------------------------------

TEST(Widget_classes, clearStyleClasses_empties_vector)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->addClass("a");
    node->addClass("b");

    node->clearStyleClasses();

    CHECK(node->getStyleClasses().empty());
    CHECK(!node->hasClass("a"));
    CHECK(!node->hasClass("b"));
}

TEST(Widget_classes, clearStyleClasses_on_empty_is_noop)
{
    Env env;
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->clearStyleClasses();   // non deve crashare
    CHECK(node->getStyleClasses().empty());
}

// ---- integration: removeClass triggers a re-resolve ------------------

TEST(Widget_classes, removeClass_triggers_style_recompute)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .highlight { background: #ff0000ff; }
    )");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(100), Px(100));
    node->addClass("highlight");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    // Lascia che la transizione iniziale si completi.
    for (int i = 0; i < 15; ++i) env.frame(root);
    CHECK(node->getStyle().background == (Color{0xff, 0x00, 0x00, 0xff}));

    node->removeClass("highlight");
    for (int i = 0; i < 15; ++i) env.frame(root);
    CHECK(node->getStyle().background == Colors::Blank);
}

TEST(Widget_classes, addClass_triggers_style_recompute)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .highlight { background: #00ff00ff; }
    )");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(100), Px(100));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    for (int i = 0; i < 15; ++i) env.frame(root);
    CHECK(node->getStyle().background == Colors::Blank);

    node->addClass("highlight");
    for (int i = 0; i < 15; ++i) env.frame(root);
    CHECK(node->getStyle().background == (Color{0x00, 0xff, 0x00, 0xff}));
}