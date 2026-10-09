#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "ZMarkup.hpp"
#include "Theme.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  Selector matching: compound selectors must match the SAME node.
// ============================================================

TEST(Selectors_compound, tag_and_class_on_same_node) {
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        Button.btn-primary { color: #ff0000; }
    )");

    auto btn = UI::Button::create(nullptr);
    btn->cls("btn-primary");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(btn);

    env.frame(root);

    CHECK((btn->getStyle().color == Color{0xff, 0x00, 0x00, 0xff}));
}

TEST(Selectors_compound, tag_matches_but_class_missing) {
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        Button.btn-primary { color: #ff0000; }
    )");

    auto btn = UI::Button::create(nullptr);
    // niente classe btn-primary

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(btn);

    env.frame(root);

    // Il colore di default (bianco) resta
    CHECK(btn->getStyle().color == Colors::White);
}

TEST(Selectors_compound, class_matches_but_tag_missing) {
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        Button.btn-primary { color: #ff0000; }
    )");

    // Un Panel con la classe btn-primary non deve matchare
    auto panel = UI::Panel::create();
    panel->cls("btn-primary");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(panel);

    env.frame(root);

    CHECK(panel->getStyle().color == Colors::White);
}

TEST(Selectors_compound, two_classes_on_same_node) {
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .card.elevated { color: #00ff00; }
    )");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->cls("card");
    node->cls("elevated");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    env.frame(root);

    CHECK((node->getStyle().color == Color{0x00, 0xff, 0x00, 0xff}));
}

TEST(Selectors_compound, two_classes_only_one_present) {
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .card.elevated { color: #00ff00; }
    )");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->cls("card");   // manca elevated

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    env.frame(root);

    CHECK(node->getStyle().color == Colors::White);
}

TEST(Selectors_compound, id_and_tag) {
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        Button#save { color: #0000ff; }
    )");

    auto btn = UI::Button::create(nullptr);
    btn->id("save");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(btn);

    env.frame(root);

    CHECK((btn->getStyle().color == Color{0x00, 0x00, 0xff, 0xff}));
}