#include "TestFramework.hpp"
#include "Mocks.hpp"

#include "ZenitUIComponents.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

namespace {

struct NotifyingContainer : public TLayout<NotifyingContainer>
{
    NotifyingContainer() : TLayout<NotifyingContainer>(LayoutType::Stack) {}
    int notifyCount = 0;

protected:
    void onDescendantFocused(Layout*) override { notifyCount++; }
};

}


static std::shared_ptr<Layout> makeClickTarget(int& counter, float w = 100, float h = 50) {
    auto n = std::make_shared<Layout>(LayoutType::Stack);
    n->size(Px(w), Px(h));
    n->setInteractive(true);
    n->setBlocksRaycast(true);
    n->onClick = [&counter]() { counter++; };
    return n;
}


TEST(Interaction_click, simple_click_increments) {
    Env env;
    int count = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    auto btn = makeClickTarget(count, 100, 50);
    root->addChild(btn);

    env.frame(root);

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
    env.click(root, { 500, 500 });

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

    CHECK(count == 0);
}


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
    CHECK(parentCount == 0);
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
    CHECK(UIContext::get().hasFocus(a.get()));
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
        b->capturePointer();
    };
    root->addChild(btn);

    env.frame(root);

    env.platform.pressLeft({ 50, 25 });
    env.frame(root);
    CHECK(pressCount == 1);

    auto captured = UIContext::get().pointerCapture.lock();
    CHECK(captured.get() == btn.get());

    env.platform.releaseLeft({ 500, 500 });
    env.frame(root);

    auto after = UIContext::get().pointerCapture.lock();
    CHECK(!after);
}


TEST(Interaction_portal, portal_wins_over_overlap) {
    Env env;
    int normalCount  = 0;
    int portalCount  = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto normal = makeClickTarget(normalCount, 200, 200);
    root->addChild(normal);

    auto portal = std::make_shared<Layout>(LayoutType::Stack);
    portal->size(Px(100), Px(100));
    portal->setInteractive(true);
    portal->setBlocksRaycast(true);
    portal->setPortal(true);
    portal->onClick = [&portalCount]() { portalCount++; };
    portal->arrange({ 50, 50, 100, 100 });
    root->addChild(portal);

    env.frame(root);

    env.click(root, { 100, 100 });

    CHECK(portalCount == 1);
    CHECK(normalCount == 0);
}


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

    env.frame(root);
    env.frame(root);
    CHECK(knob->getRect().x > 10.5f);
    CHECK(knob->getRect().x < 60.0f);

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

    env.platform.moveMouse({50, 25});
    env.frame(root);
    CHECK(enters == 1);
    CHECK(exits  == 0);

    env.platform.pressLeft({50, 25});
    env.frame(root);
    CHECK(enters == 1);
    CHECK(exits  == 0);

    env.platform.releaseLeft({50, 25});
    env.frame(root);
    CHECK(exits == 0);

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

    sv->scrollToY(100.0f);
    env.frame(root);
    CHECK(sv->getScrollY() > 0.0f);

    sv->setEnabled(true);
    env.frame(root);
    CHECK(sv->getScrollY() > 0.0f);

    sv->getInlineBase().overflowY = Overflow::Hidden;
    env.frame(root);
    env.frame(root);
    CHECK_NEAR(sv->getScrollY(), 0.0f, 1e-4);
}

TEST(Interaction_scroll, portal_does_not_move_with_scroll) {
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

    auto portal = std::make_shared<Layout>(LayoutType::Stack);
    portal->size(Px(50), Px(50));
    portal->setPortal(true);
    portal->arrange({0, 0, 50, 50});
    sv->addChild(portal);

    root->addChild(sv);
    env.frame(root);

    float py0 = portal->getRect().y;

    sv->scrollToY(200.0f);
    env.frame(root);

    CHECK_NEAR(portal->getRect().y, py0, 0.5f);
}

TEST(Interaction_scroll, dropdown_list_follows_trigger) {
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow: scroll; }
        Dropdown { width: 150px; height: 40px; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto sv = std::make_shared<Layout>(LayoutType::Vertical);
    sv->size(Px(300), Px(200));
    sv->cls("scroller");

    auto spacer = std::make_shared<Layout>(LayoutType::Stack);
    spacer->size(Px(10), Px(80));
    sv->addChild(spacer);

    auto dd = UI::Dropdown::create(std::vector<std::string>{"A", "B", "C"}, 0);
    sv->addChild(dd);

    auto filler = std::make_shared<Layout>(LayoutType::Stack);
    filler->size(Px(10), Px(500));
    sv->addChild(filler);

    root->addChild(sv);
    env.frame(root);

    dd->setOpen(true);
    env.frame(root);
    env.frame(root);

    auto list = dd->children[1];

    sv->scrollToY(60.0f);
    env.frame(root);
    env.frame(root);

    Rect btnRect  = dd->children[0]->getRect();
    Rect listRect = list->getRect();

    float gapBelow = listRect.y - (btnRect.y + btnRect.height);
    float gapAbove = btnRect.y - (listRect.y + listRect.height);

    bool okBelow = std::abs(gapBelow - 4.0f) < 2.0f;
    bool okAbove = std::abs(gapAbove - 4.0f) < 2.0f;
    CHECK(okBelow || okAbove);
}

TEST(Interaction_focus, click_fires_on_descendant_focused)
{
    Env env;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto container = NotifyingContainer::create();
    container->size(Px(200), Px(200));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->setFocusable(true);
    container->addChild(btn);
    root->addChild(container);

    env.frame(root);

    CHECK(container->notifyCount == 0);

    env.click(root, { 50, 25 });

    CHECK(UIContext::get().hasFocus(btn.get()));
    CHECK(container->notifyCount == 1);
}

TEST(Interaction_modal, hidden_modal_does_not_block_click)
{
    Env env;
    Theme::get().clear();

    int clicks = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(200), Px(60));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onClick = [&clicks]{ clicks++; };
    root->addChild(btn);

    auto modal = UI::Modal::create();
    modal->size(Percent(100), Percent(100));
    root->addChild(modal);

    env.frame(root);

    env.click(root, { 100, 30 });
    CHECK(clicks == 1);

    modal->show();
    env.frame(root);
    env.frame(root);

    env.click(root, { 100, 30 });
    CHECK(clicks == 1);

    modal->hide();
    env.frame(root);
    env.frame(root);

    env.click(root, { 100, 30 });
    CHECK(clicks == 2);
}

TEST(Interaction_modal, show_and_hide_callbacks)
{
    Env env;
    Theme::get().clear();

    auto modal = UI::Modal::create();
    int showCount = 0;
    int hideCount = 0;
    modal->onShow = [&] { showCount++; };
    modal->onHide = [&] { hideCount++; };

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(modal);

    env.frame(root);

    modal->show();
    CHECK(showCount == 1);
    CHECK(hideCount == 0);

    modal->hide();
    CHECK(showCount == 1);
    CHECK(hideCount == 1);

    modal->show();
    modal->hide();
    CHECK(showCount == 2);
    CHECK(hideCount == 2);
}


TEST(Interaction_buttonCallbacks, middle_press_fires_onButtonPress)
{
    Env env;
    int calls = 0;
    MouseButton lastBtn = MouseButton::Left;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onButtonPress = [&](MouseButton b) { calls++; lastBtn = b; };
    root->addChild(btn);

    env.frame(root);

    env.platform.pressMiddle({50, 25});
    env.frame(root);

    CHECK(calls == 1);
    CHECK(lastBtn == MouseButton::Middle);
}

TEST(Interaction_buttonCallbacks, middle_click_fires_onButtonClick)
{
    Env env;
    int calls = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onButtonClick = [&](MouseButton) { calls++; };
    root->addChild(btn);

    env.frame(root);

    env.platform.pressMiddle({50, 25});
    env.frame(root);
    env.platform.releaseMiddle({50, 25});
    env.frame(root);

    CHECK(calls == 1);
}

TEST(Interaction_buttonCallbacks, left_click_fires_both_legacy_and_unified)
{
    Env env;
    int legacyCalls = 0;
    int unifiedCalls = 0;
    MouseButton lastBtn = MouseButton::Right;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onClick = [&]() { legacyCalls++; };
    btn->onButtonClick = [&](MouseButton b) { unifiedCalls++; lastBtn = b; };
    root->addChild(btn);

    env.frame(root);

    env.click(root, {50, 25});

    CHECK(legacyCalls == 1);
    CHECK(unifiedCalls == 1);
    CHECK(lastBtn == MouseButton::Left);
}

TEST(Interaction_buttonCallbacks, right_click_does_not_fire_onButtonClick)
{
    Env env;
    int legacyRightCalls = 0;
    int unifiedCalls = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onRightClick = [&]() { legacyRightCalls++; };
    btn->onButtonClick = [&](MouseButton) { unifiedCalls++; };
    root->addChild(btn);

    env.frame(root);

    env.platform.pressRight({50, 25});
    env.frame(root);
    env.platform.releaseRight({50, 25});
    env.frame(root);

    CHECK(legacyRightCalls == 1);
    CHECK(unifiedCalls == 0);
}

TEST(Interaction_buttonCallbacks, middle_click_on_empty_area_does_not_fire)
{
    Env env;
    int calls = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onButtonClick = [&](MouseButton) { calls++; };
    root->addChild(btn);

    env.frame(root);

    env.platform.pressMiddle({500, 500});
    env.frame(root);
    env.platform.releaseMiddle({500, 500});
    env.frame(root);

    CHECK(calls == 0);
}

TEST(Interaction_lifetime, removeFromParent_during_onClick_does_not_dangle)
{
    Env env;

    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Percent(100), Percent(100));

    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->size(Px(200), Px(40));
    root->addChild(row);

    std::weak_ptr<Layout> weakRow = row;

    {
        auto btn = std::make_shared<Layout>(LayoutType::Stack);
        btn->size(Px(40), Px(40));
        btn->setInteractive(true);
        btn->setBlocksRaycast(true);
        btn->onClick = [weakRow]{
            if (auto r = weakRow.lock()) r->removeFromParent();
        };
        row->addChild(btn);
    }

    auto sibling = std::make_shared<Layout>(LayoutType::Stack);
    sibling->size(Px(200), Px(40));
    root->addChild(sibling);

    env.frame(root);

    row.reset();

    env.click(root, {20, 20});
    env.frame(root);

    CHECK(root->children.size() == 1);
    CHECK(root->children[0] == sibling);
    CHECK(weakRow.expired());
}

TEST(Interaction_scroll, container_shrinks_after_child_removal)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller {
            overflow-y: auto;
            max-height: 100px;
            width: 200px;
        }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Percent(100), Percent(100));

    auto scroller = std::make_shared<Layout>(LayoutType::Vertical);
    scroller->cls("scroller");
    root->addChild(scroller);

    std::vector<std::shared_ptr<Layout>> rows;
    for (int i = 0; i < 10; ++i)
    {
        auto r = std::make_shared<Layout>(LayoutType::Stack);
        r->size(Px(200), Px(20));
        scroller->addChild(r);
        rows.push_back(r);
    }

    env.frame(root);
    env.frame(root);

    CHECK(scroller->getMaxScrollY() > 0.0f);
    CHECK_NEAR(scroller->getRect().height, 100.0f, 1.0f);

    for (int i = 0; i < 9; ++i)
        rows[i]->removeFromParent();

    env.frame(root);
    env.frame(root);

    CHECK_NEAR(scroller->getMaxScrollY(), 0.0f, 0.5f);
    CHECK_NEAR(scroller->getRect().height, 20.0f, 2.0f);

    rows[9]->removeFromParent();
    env.frame(root);
    env.frame(root);

    CHECK_NEAR(scroller->getRect().height, 0.0f, 1.0f);
}

TEST(Interaction_tooltip, clamps_to_viewport_and_flips_below) {
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    
    auto anchor = std::make_shared<Layout>(LayoutType::Stack);
    anchor->size(Px(100), Px(50));
    anchor->getInlineBase().position = Position::Absolute;
    anchor->getInlineBase().top = Px(0);
    anchor->getInlineBase().left = Px(0);
    anchor->setInteractive(true);
    root->addChild(anchor);
    
    auto tip = UI::Tooltip::attach(anchor, "Test tip", 0.0f);
    
    env.frame(root);
    env.platform.moveMouse({50, 25});
    env.frame(root);
    env.frame(root);
    
    CHECK(tip->getRect().y >= 56.0f);
}

TEST(Interaction_subtree, class_change_propagates_to_children)
{
    Env env;
    Theme::get().clear();

    ZMarkup::loadStyleString(R"(
        Toggle { width: 100px; height: 50px; }
        .toggle-knob {
            position: absolute;
            width: 30px; height: 30px;
            top: 10px; left: 10px;
        }
        Toggle.active .toggle-knob { left: 60px; }
    )");

    auto ui = ZMarkup::build(R"(Toggle#t)");
    auto toggle = ui.find("t");
    auto knob   = toggle->children[0];

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(toggle);

    env.frame(root);
    CHECK_NEAR(knob->getRect().x, 10.0f, 1.0f);
    
    toggle->addClass("active");

    env.frame(root);
    env.frame(root);
    CHECK(knob->getRect().x > 10.5f);
    CHECK(knob->getRect().x < 60.0f);

    for (int i = 0; i < 30; ++i)
        env.frame(root);
    CHECK_NEAR(knob->getRect().x, 60.0f, 1.0f);
}