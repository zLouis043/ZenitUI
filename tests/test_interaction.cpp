#include "TestFramework.hpp"
#include "Mocks.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  Helper: piccolo nodo cliccabile
// ============================================================

static std::shared_ptr<Layout> makeClickTarget(int& counter, float w = 100, float h = 50) {
    auto n = std::make_shared<Layout>(LayoutType::Stack);
    n->size(Px(w), Px(h));
    n->setInteractive(true);
    n->setBlocksRaycast(true);
    n->onClick = [&counter]() { counter++; };
    return n;
}

// ============================================================
//  Click di base
// ============================================================

TEST(Interaction_click, simple_click_increments) {
    Env env;
    int count = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    auto btn = makeClickTarget(count, 100, 50);
    root->addChild(btn);

    // Primo frame per calcolare layout
    env.frame(root);

    // Il btn è in alto a sinistra di root (Stack, Start/Start)
    env.click(root, { 50, 25 });

    CHECK(count == 1);
}

TEST(Interaction_click, click_outside_does_nothing) {
    Env env;
    int count = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    auto btn = makeClickTarget(count, 100, 50);
    root->addChild(btn);

    env.frame(root);
    env.click(root, { 500, 500 });   // fuori dal btn

    CHECK(count == 0);
}

TEST(Interaction_click, press_only_no_click) {
    Env env;
    int count = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    auto btn = makeClickTarget(count, 100, 50);
    root->addChild(btn);

    env.frame(root);

    env.platform.pressLeft({ 50, 25 });
    env.frame(root);

    CHECK(count == 0);   // click scatta solo al release
}

// ============================================================
//  Bubbling e passThrough
// ============================================================

TEST(Interaction_bubbling, child_consumes_parent_does_not_fire) {
    Env env;
    int childCount = 0;
    int parentCount = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->setInteractive(true);
    root->setBlocksRaycast(true);
    root->onClick = [&parentCount]() { parentCount++; };

    auto child = makeClickTarget(childCount, 100, 50);
    root->addChild(child);

    env.frame(root);
    env.click(root, { 50, 25 });

    CHECK(childCount == 1);
    CHECK(parentCount == 0);   // il figlio ha consumato
}

TEST(Interaction_bubbling, passThrough_lets_parent_fire) {
    Env env;
    int childCount = 0;
    int parentCount = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->setInteractive(true);
    root->setBlocksRaycast(true);
    root->onClick = [&parentCount]() { parentCount++; };

    auto child = makeClickTarget(childCount, 100, 50);
    child->setPassThrough(true);
    root->addChild(child);

    env.frame(root);
    env.click(root, { 50, 25 });

    CHECK(childCount == 1);
    CHECK(parentCount == 1);
}

// ============================================================
//  Focus e Tab navigation
// ============================================================

TEST(Interaction_focus, click_gives_focus) {
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->setFocusable(true);
    root->addChild(btn);

    env.frame(root);
    env.click(root, { 50, 25 });

    CHECK(UIContext::get().hasFocus(btn.get()));
}

TEST(Interaction_focus, tab_cycles_through_focusables) {
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto a = std::make_shared<Layout>(LayoutType::Stack);
    a->size(Px(100), Px(50));
    a->setInteractive(true);
    a->setBlocksRaycast(true);
    a->setFocusable(true);

    auto b = std::make_shared<Layout>(LayoutType::Stack);
    b->size(Px(100), Px(50));
    b->setInteractive(true);
    b->setBlocksRaycast(true);
    b->setFocusable(true);

    root->addChild(a);
    root->addChild(b);

    env.frame(root);

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(b.get()));

    env.platform.pressKey(Key::Tab);
    env.frame(root);
    CHECK(UIContext::get().hasFocus(a.get()));   // wrap
}

TEST(Interaction_focus, click_outside_releases_focus) {
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->setFocusable(true);
    root->addChild(btn);

    env.frame(root);
    env.click(root, { 50, 25 });
    CHECK(UIContext::get().hasFocus(btn.get()));

    env.click(root, { 500, 500 });
    CHECK(!UIContext::get().hasFocus(btn.get()));
}

// ============================================================
//  Pointer capture
// ============================================================

TEST(Interaction_capture, captured_node_receives_events_outside) {
    Env env;
    int pressCount = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onPress = [&pressCount, b = btn.get()]() {
        pressCount++;
        b->capturePointer();      // <-- Cattura esplicita (come fa Slider)
    };
    root->addChild(btn);

    env.frame(root);

    // Press dentro → cattura
    env.platform.pressLeft({ 50, 25 });
    env.frame(root);
    CHECK(pressCount == 1);

    auto captured = UIContext::get().pointerCapture.lock();
    CHECK(captured.get() == btn.get());

    // Release fuori: la capture mantiene gli eventi su btn fino al rilascio
    env.platform.releaseLeft({ 500, 500 });
    env.frame(root);

    // Dopo il release, capture rilasciato automaticamente
    auto after = UIContext::get().pointerCapture.lock();
    CHECK(!after);
}

// ============================================================
//  Portal
// ============================================================

TEST(Interaction_portal, portal_wins_over_overlap) {
    Env env;
    int normalCount  = 0;
    int portalCount  = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    // Nodo normale a (0,0) 200x200
    auto normal = makeClickTarget(normalCount, 200, 200);
    root->addChild(normal);

    // Portal sopra: si arrangia da solo
    auto portal = std::make_shared<Layout>(LayoutType::Stack);
    portal->size(Px(100), Px(100));
    portal->setInteractive(true);
    portal->setBlocksRaycast(true);
    portal->setPortal(true);
    portal->onClick = [&portalCount]() { portalCount++; };
    // In portal, l'arrange lo fa il portal stesso — lo posizioniamo manualmente
    portal->arrange({ 50, 50, 100, 100 });
    root->addChild(portal);

    env.frame(root);

    // Click nel punto di overlap (100,100) — dentro entrambi
    env.click(root, { 100, 100 });

    // Il portal dovrebbe vincere (portals hanno precedenza in hitTest)
    CHECK(portalCount == 1);
    CHECK(normalCount == 0);
}

// ============================================================
//  z-index
// ============================================================

TEST(Interaction_zindex, higher_z_wins) {
    Env env;
    int lowCount  = 0;
    int highCount = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto low = makeClickTarget(lowCount, 200, 200);
    low->getInlineBase().position = Position::Relative;
    low->getInlineBase().zIndex   = ZIndex(1);
    root->addChild(low);

    auto high = makeClickTarget(highCount, 200, 200);
    high->getInlineBase().position = Position::Relative;
    high->getInlineBase().zIndex   = ZIndex(10);
    root->addChild(high);

    env.frame(root);

    env.click(root, { 100, 100 });

    CHECK(highCount == 1);
    CHECK(lowCount == 0);
}

TEST(Interaction_subtree, checked_propagates_to_children) {
    Env env;
    Theme::get().clear();

    ZMarkup::loadStyleString(R"(
        Toggle { width: 100px; height: 50px; }
        .toggle-knob {
            position: absolute;
            width: 30px; height: 30px;
            top: 10px; left: 10px;
        }
        Toggle:checked .toggle-knob { left: 60px; }
    )");

    auto ui = ZMarkup::build(R"(Toggle#t)");
    auto toggle = ui.find("t");
    auto knob   = toggle->children[0];

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(toggle);

    env.frame(root);
    CHECK_NEAR(knob->getRect().x, 10.0f, 1.0f);

    toggle->setChecked(true);

    // Dopo 2 frame, il knob si deve essere mosso dalla posizione iniziale.
    env.frame(root);
    env.frame(root);
    CHECK(knob->getRect().x > 10.5f);   // si è mosso
    CHECK(knob->getRect().x < 60.0f);   // ma non è ancora arrivato

    // Dopo abbastanza frame, deve essere a 60.
    for (int i = 0; i < 30; ++i)
        env.frame(root);
    CHECK_NEAR(knob->getRect().x, 60.0f, 1.0f);
}

TEST(Interaction_hover, exit_does_not_fire_on_press) {
    Env env;
    int enters = 0, exits = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onHoverEnter = [&enters]() { enters++; };
    btn->onHoverExit  = [&exits]()  { exits++; };
    root->addChild(btn);

    env.frame(root);

    // Hover
    env.platform.moveMouse({50, 25});
    env.frame(root);
    CHECK(enters == 1);
    CHECK(exits  == 0);

    // Press (ancora sopra il bottone): non deve sparare onHoverExit
    env.platform.pressLeft({50, 25});
    env.frame(root);
    CHECK(enters == 1);
    CHECK(exits  == 0);   // <-- la regressione era qui

    // Release (ancora sopra)
    env.platform.releaseLeft({50, 25});
    env.frame(root);
    CHECK(exits == 0);

    // Uscita reale
    env.platform.moveMouse({500, 500});
    env.frame(root);
    CHECK(exits == 1);
}

TEST(Interaction_scroll, reset_on_overflow_change_only) {
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow: scroll; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto sv = std::make_shared<Layout>(LayoutType::Vertical);
    sv->size(Px(200), Px(100));
    sv->cls("scroller");
    sv->addChild(std::make_shared<Layout>(LayoutType::Stack)->size(Px(100), Px(500)));
    root->addChild(sv);

    env.frame(root);

    // Scroll a metà
    sv->scrollToY(100.0f);
    env.frame(root);
    CHECK(sv->getScrollY() > 0.0f);

    // Cambia un'altra proprietà: NON deve resettare lo scroll
    sv->setEnabled(true);   // già true, ma tocca il flag
    env.frame(root);
    CHECK(sv->getScrollY() > 0.0f);

    // Cambia overflow: DEVE resettare
    sv->getInlineBase().overflowY = Overflow::Hidden;
    env.frame(root);
    env.frame(root);
    CHECK_NEAR(sv->getScrollY(), 0.0f, 1e-4);
}