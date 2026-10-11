#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  Focus scopes — 3.7
// ============================================================

// Helper: crea un Layout che è focusable, con nome per debug.
static std::shared_ptr<Layout> makeFocusable(const std::string& id,
                                               float w = 50.0f,
                                               float h = 30.0f)
{
    auto n = std::make_shared<Layout>(LayoutType::Stack);
    n->size(Px(w), Px(h));
    n->setInteractive(true);
    n->setBlocksRaycast(true);
    n->setFocusable(true);
    n->setId(id);
    return n;
}

// ---- Tab dentro scope ------------------------------------------------

TEST(FocusScope_tab, tab_stays_within_scope)
{
    Env env;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    // Un focusable FUORI dallo scope.
    auto outside = makeFocusable("outside");
    root->addChild(outside);

    // Uno scope con due focusable dentro.
    auto scope = std::make_shared<Layout>(LayoutType::Stack);
    scope->size(Px(200), Px(200));
    scope->setFocusScope(true);
    auto a = makeFocusable("a");
    auto b = makeFocusable("b");
    scope->addChild(a);
    scope->addChild(b);
    root->addChild(scope);

    env.frame(root);

    // Focus su a. Da lì, Tab deve andare a b, e poi tornare ad a.
    // Non deve MAI raggiungere outside.
    UIContext::get().requestFocus(a);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(b.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(b.get()));

    // Mai su outside.
    CHECK(!UIContext::get().hasFocus(outside.get()));
}

TEST(FocusScope_tab, shift_tab_reverses_within_scope)
{
    Env env;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto scope = std::make_shared<Layout>(LayoutType::Stack);
    scope->size(Px(200), Px(200));
    scope->setFocusScope(true);
    auto a = makeFocusable("a");
    auto b = makeFocusable("b");
    auto c = makeFocusable("c");
    scope->addChild(a);
    scope->addChild(b);
    scope->addChild(c);
    root->addChild(scope);

    env.frame(root);

    UIContext::get().requestFocus(b);
    env.frame(root);

    env.platform.shiftDown = true;
    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(c.get()));   // wrap-around
}

// ---- scope annidati --------------------------------------------------

TEST(FocusScope_nested, inner_scope_wins)
{
    Env env;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    // Scope esterno con due focusable.
    auto outerScope = std::make_shared<Layout>(LayoutType::Stack);
    outerScope->size(Px(400), Px(400));
    outerScope->setFocusScope(true);
    auto oa = makeFocusable("oa");
    auto ob = makeFocusable("ob");
    outerScope->addChild(oa);
    outerScope->addChild(ob);

    // Scope interno, dentro l'esterno, con due focusable.
    auto innerScope = std::make_shared<Layout>(LayoutType::Stack);
    innerScope->size(Px(200), Px(200));
    innerScope->setFocusScope(true);
    auto ia = makeFocusable("ia");
    auto ib = makeFocusable("ib");
    innerScope->addChild(ia);
    innerScope->addChild(ib);
    outerScope->addChild(innerScope);

    root->addChild(outerScope);
    env.frame(root);

    // Focus su ia (dentro lo scope interno). Tab deve restare
    // nello scope interno, NON passare a ob.
    UIContext::get().requestFocus(ia);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(ia.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(ib.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(ia.get()));   // wrap

    CHECK(!UIContext::get().hasFocus(ob.get()));
}

// ---- focus release ---------------------------------------------------

TEST(FocusScope_release, click_outside_releases_focus)
{
    Env env;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto a = makeFocusable("a");
    root->addChild(a);

    env.frame(root);

    UIContext::get().requestFocus(a);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));

    // Click in un punto vuoto (lontano dal focusable, che è a 0,0).
    env.click(root, {500.0f, 500.0f});
    env.frame(root);

    CHECK(!UIContext::get().hasFocus(a.get()));
}

TEST(FocusScope_release, disable_releases_focus)
{
    Env env;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto a = makeFocusable("a");
    root->addChild(a);

    env.frame(root);

    UIContext::get().requestFocus(a);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));

    a->setEnabled(false);
    env.frame(root);

    CHECK(!UIContext::get().hasFocus(a.get()));
}

// ---- no scope => root is the scope ----------------------------------

TEST(FocusScope_default, no_scope_uses_root)
{
    Env env;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    // Due focusable senza scope esplicito: Tab deve ciclare su entrambi
    // attraverso il root.
    auto a = makeFocusable("a");
    auto b = makeFocusable("b");
    root->addChild(a);
    root->addChild(b);

    env.frame(root);

    UIContext::get().requestFocus(a);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(b.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));   // wrap
}