#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

// ============================================================
//  Dropdown
// ============================================================

// Helper: dropdown isolato in una scena minima.
struct DropdownScene
{
    std::shared_ptr<Layout>   root;
    std::shared_ptr<Dropdown> dd;
};

static DropdownScene makeDropdown(Env& env,
                                    std::vector<std::string> options = {"A", "B", "C"},
                                    int initial = 0)
{
    DropdownScene s;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        Dropdown { width: 200px; height: 40px; }
        .dropdown-trigger { min-height: 30px; }
    )");

    s.dd = Dropdown::create(std::move(options), initial);

    s.root = std::make_shared<Layout>(LayoutType::Stack);
    s.root->size(Percent(100), Percent(100));
    s.root->addChild(s.dd);

    env.frame(s.root);
    env.frame(s.root);

    return s;
}

// ---- setSelected / getSelected ---------------------------------------

TEST(Dropdown_basic, initial_selection)
{
    Env env;
    auto s = makeDropdown(env, {"Alpha", "Beta", "Gamma"}, 1);
    CHECK(s.dd->getSelected() == 1);
}

TEST(Dropdown_basic, setSelected_updates_index)
{
    Env env;
    auto s = makeDropdown(env, {"Alpha", "Beta", "Gamma"}, 0);

    s.dd->setSelected(2);
    CHECK(s.dd->getSelected() == 2);
}

TEST(Dropdown_basic, setSelected_out_of_range_is_ignored)
{
    Env env;
    auto s = makeDropdown(env, {"A", "B"}, 0);

    s.dd->setSelected(-1);
    CHECK(s.dd->getSelected() == 0);

    s.dd->setSelected(10);
    CHECK(s.dd->getSelected() == 0);
}

TEST(Dropdown_basic, setSelected_fires_onChange)
{
    Env env;
    auto s = makeDropdown(env, {"A", "B", "C"}, 0);

    int calls = 0;
    int lastIdx = -1;
    std::string lastName;
    s.dd->onChange = [&](int idx, const std::string& name) {
        calls++;
        lastIdx = idx;
        lastName = name;
    };

    s.dd->setSelected(2);

    CHECK(calls == 1);
    CHECK(lastIdx == 2);
    CHECK(lastName == "C");
}

TEST(Dropdown_basic, setSelected_same_value_fires_onChange)
{
    // Il Dropdown attuale non confronta old/new: setSelected(0) con
    // selected == 0 chiama comunque onChange. Questo è un test di
    // documentazione del comportamento attuale.
    Env env;
    auto s = makeDropdown(env, {"A", "B"}, 1);

    int calls = 0;
    s.dd->onChange = [&](int, const std::string&) { calls++; };

    s.dd->setSelected(1);
    CHECK(calls == 1);
}

// ---- open / close ----------------------------------------------------

TEST(Dropdown_open, initial_state_closed)
{
    Env env;
    auto s = makeDropdown(env);
    CHECK(!s.dd->getOpen());
}

TEST(Dropdown_open, setOpen_true)
{
    Env env;
    auto s = makeDropdown(env);

    s.dd->setOpen(true);
    CHECK(s.dd->getOpen());

    env.frame(s.root);
    env.frame(s.root);
}

TEST(Dropdown_open, toggle_via_setOpen)
{
    Env env;
    auto s = makeDropdown(env);

    s.dd->setOpen(true);
    CHECK(s.dd->getOpen());

    s.dd->setOpen(false);
    CHECK(!s.dd->getOpen());
}

// ---- click on trigger ------------------------------------------------

TEST(Dropdown_click, clicking_trigger_opens)
{
    Env env;
    auto s = makeDropdown(env);

    // Il trigger è il primo figlio.
    auto trigger = s.dd->children[0];
    Rect r = trigger->getRect();

    env.click(s.root, { r.x + r.width * 0.5f, r.center().y });
    env.frame(s.root);

    CHECK(s.dd->getOpen());
}

TEST(Dropdown_click, clicking_trigger_again_closes)
{
    Env env;
    auto s = makeDropdown(env);

    auto trigger = s.dd->children[0];
    Rect r = trigger->getRect();

    // Apri.
    env.click(s.root, { r.x + r.width * 0.5f, r.center().y });
    env.frame(s.root);
    CHECK(s.dd->getOpen());

    // Richiudi.
    env.click(s.root, { r.x + r.width * 0.5f, r.center().y });
    env.frame(s.root);
    CHECK(!s.dd->getOpen());
}

// ---- click outside closes --------------------------------------------

TEST(Dropdown_click, outside_click_closes)
{
    Env env;
    auto s = makeDropdown(env);

    s.dd->setOpen(true);
    env.frame(s.root);
    env.frame(s.root);

    CHECK(s.dd->getOpen());

    // Click lontano dal dropdown.
    env.click(s.root, {700.0f, 500.0f});
    env.frame(s.root);

    CHECK(!s.dd->getOpen());
}

// ---- disabled --------------------------------------------------------

TEST(Dropdown_disabled, disabling_closes)
{
    Env env;
    auto s = makeDropdown(env);

    s.dd->setOpen(true);
    env.frame(s.root);
    CHECK(s.dd->getOpen());

    s.dd->setEnabled(false);
    env.frame(s.root);

    CHECK(!s.dd->getOpen());
}

// ---- list container --------------------------------------------------

TEST(Dropdown_list, has_one_item_per_option)
{
    Env env;
    auto s = makeDropdown(env, {"A", "B", "C", "D"}, 0);

    // Il secondo figlio è il listContainer.
    CHECK(s.dd->children.size() >= 2);
    auto listContainer = s.dd->children[1];
    CHECK((int)listContainer->children.size() == 4);
}

TEST(Dropdown_list, list_container_is_portal)
{
    Env env;
    auto s = makeDropdown(env);

    CHECK(s.dd->children.size() >= 2);
    auto listContainer = s.dd->children[1];
    CHECK(listContainer->isPortal());
}

// ---- option click ----------------------------------------------------

TEST(Dropdown_option, clicking_option_selects)
{
    Env env;
    auto s = makeDropdown(env, {"A", "B", "C"}, 0);

    int lastIdx = -1;
    s.dd->onChange = [&](int idx, const std::string&) { lastIdx = idx; };

    s.dd->setOpen(true);
    env.frame(s.root);
    env.frame(s.root);   // il portal si registra

    // Il listContainer è figlio del dropdown (children[1]).
    auto listContainer = s.dd->children[1];
    auto option2 = listContainer->children[1];
    Rect optRect = option2->getRect();

    env.click(s.root, { optRect.center().x, optRect.center().y });
    env.frame(s.root);
    env.frame(s.root);

    CHECK(lastIdx == 1);
    CHECK(s.dd->getSelected() == 1);
    CHECK(!s.dd->getOpen());   // si chiude dopo la selezione
}

TEST(Dropdown_callbacks, onOpenChanged_fires_on_open_and_close)
{
    Env env;
    auto s = makeDropdown(env);

    std::vector<bool> states;
    s.dd->onOpenChanged = [&](bool v) { states.push_back(v); };

    s.dd->setOpen(true);
    s.dd->setOpen(false);
    s.dd->setOpen(false);   // no-op, non deve chiamare il callback

    CHECK(states.size() == 2);
    CHECK(states[0] == true);
    CHECK(states[1] == false);
}

TEST(Dropdown_callbacks, onOpenChanged_fires_on_disable_close)
{
    Env env;
    auto s = makeDropdown(env);

    int calls = 0;
    bool lastValue = true;
    s.dd->onOpenChanged = [&](bool v) { calls++; lastValue = v; };

    s.dd->setOpen(true);
    calls = 0;   // reset, ci interessa solo la chiusura per disable

    s.dd->setEnabled(false);

    CHECK(calls == 1);
    CHECK(lastValue == false);
}

// ---- keyboard navigation ---------------------------------------------
TEST(Dropdown_keyboard, navigation_and_escape) {
    Env env;
    auto s = makeDropdown(env, {"A", "B", "C"}, 0);
    
    // Assegna il focus al dropdown
    UIContext::get().requestFocus(s.dd);
    env.frame(s.root);
    
    // Freccia Giù -> Apre la tendina
    env.platform.pressKey(Key::Down);
    env.frame(s.root);
    CHECK(s.dd->getOpen());
    
    // Freccia Giù -> Seleziona "B" (1)
    env.platform.pressKey(Key::Down);
    env.frame(s.root);
    CHECK(s.dd->getSelected() == 1);
    
    // Escape -> Chiude la tendina
    env.platform.pressKey(Key::Escape);
    env.frame(s.root);
    CHECK(!s.dd->getOpen());
    CHECK(s.dd->getSelected() == 1);
}

TEST(Dropdown_styling, items_use_checked_state) {
    Env env;
    auto s = makeDropdown(env, {"A", "B", "C"}, 1); // Seleziona "B"
    s.dd->setOpen(true);
    env.frame(s.root);
    env.frame(s.root);
    
    auto listContainer = s.dd->children[1];
    CHECK(!listContainer->children[0]->isCheckedState());
    CHECK(listContainer->children[1]->isCheckedState()); // L'elemento "B" deve avere il flag checked
    CHECK(!listContainer->children[2]->isCheckedState());
}