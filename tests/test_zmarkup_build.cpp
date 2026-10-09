#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  Widget composti devono avere figli dopo build
//  (verifica che X::create() sia stato usato, non make_shared)
// ============================================================

TEST(ZMarkupBuild, toggle_has_knob_after_build) {
    auto ui = ZMarkup::build(R"(Toggle#t checked=true)");
    auto toggle = ui.find("t");
    CHECK(toggle != nullptr);
    CHECK(toggle->children.size() == 1);   // knob

    // Il figlio deve avere la classe "toggle-knob"
    const auto& classes = toggle->children[0]->getStyleClasses();
    bool hasKnobClass = false;
    for (auto& c : classes) if (c == "toggle-knob") hasKnobClass = true;
    CHECK(hasKnobClass);
}

TEST(ZMarkupBuild, button_has_text_child) {
    auto ui = ZMarkup::build(R"(Button#b "Click me")");
    auto btn = ui.find("b");
    CHECK(btn != nullptr);
    CHECK(btn->children.size() == 1);   // label
}

TEST(ZMarkupBuild, dropdown_has_button_and_list) {
    auto ui = ZMarkup::build(R"(Dropdown#d options="A,B,C")");
    auto dd = ui.find("d");
    CHECK(dd != nullptr);
    // Il Dropdown compone button + listContainer, quindi ha almeno 2 figli
    CHECK(dd->children.size() >= 2);
}

TEST(ZMarkupBuild, checkbox_no_composite_children) {
    // Checkbox non usa onBuild, ma deve comunque funzionare
    auto ui = ZMarkup::build(R"(Checkbox#c checked=true)");
    auto cb = ui.find("c");
    CHECK(cb != nullptr);
}

TEST(ZMarkupBuild, slider_no_composite_children) {
    auto ui = ZMarkup::build(R"(Slider#s value=0.5)");
    auto s = ui.find("s");
    CHECK(s != nullptr);
}

TEST(ZMarkupBuild, textinput_no_composite_children) {
    auto ui = ZMarkup::build(R"(TextInput#ti value="hello")");
    auto ti = ui.find("ti");
    CHECK(ti != nullptr);
}

TEST(ZMarkupBuild, progressbar_no_composite_children) {
    auto ui = ZMarkup::build(R"(ProgressBar#pb value=0.3)");
    auto pb = ui.find("pb");
    CHECK(pb != nullptr);
}

TEST(ZMarkupBuild, scrollview_works) {
    auto ui = ZMarkup::build(R"(ScrollView#sv { Text "inside" })");
    auto sv = ui.find("sv");
    CHECK(sv != nullptr);
    CHECK(sv->children.size() == 1);   // testo
}

TEST(ZMarkupBuild, nested_layout) {
    auto ui = ZMarkup::build(R"(
        VStack#root {
            HStack#row {
                Text "a"
                Text "b"
            }
            Button "ok"
        }
    )");
    auto root = ui.find("root");
    auto row  = ui.find("row");
    CHECK(root != nullptr);
    CHECK(row != nullptr);
    CHECK(root->children.size() == 2);
    CHECK(row->children.size() == 2);
}

// ============================================================
//  Test integrato: costruire, fare un frame, verificare che
//  il toggle si arrangi con dimensioni > 0 (prova che onBuild
//  + measure + arrange funzionano end-to-end dal markup).
// ============================================================

TEST(ZMarkupBuild, toggle_arranges_with_size) {
    Env env;
    Theme::get().clear();

    // Stile minimo per dare al Toggle una dimensione calcolabile
    ZMarkup::loadStyleString(R"(
        Toggle { width: 100px; height: 50px; }
        .toggle-knob {
            position: absolute;
            width: 30px; height: 30px;
            top: 10px; left: 10px;
        }
        Toggle:checked .toggle-knob { left: 60px; }
    )");

    auto ui = ZMarkup::build(R"(Toggle#t checked=true)");
    auto toggle = ui.find("t");
    CHECK(toggle != nullptr);
    CHECK(toggle->children.size() == 1);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(toggle);

    env.frame(root);

    // Il knob deve avere una posizione calcolata (left != 0)
    auto knob = toggle->children[0];
    Rect knobRect = knob->getRect();
    CHECK(knobRect.width > 0.0f);
    CHECK(knobRect.height > 0.0f);
    // Con :checked, left=60px → knob spostato a destra
    CHECK(knobRect.x > 10.0f);
}

TEST(Widget_dropdown, css_can_override_trigger_height)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .dropdown-trigger { height: 60px; }
    )");

    auto dd = UI::Dropdown::create(
        std::vector<std::string>{"A", "B"}, 0);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(dd);

    env.frame(root);

    auto trigger = dd->children[0];
    CHECK_NEAR(trigger->getRect().height, 60.0f, 1.0f);
}