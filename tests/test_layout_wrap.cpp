#include "TestFramework.hpp"
#include "Layout.hpp"
#include "Theme.hpp"

using namespace ZenitUI;

static void freshTheme() {
    Theme::get().clear();
}

static std::shared_ptr<Layout> makeBox(float w, float h) {
    auto n = std::make_shared<Layout>(LayoutType::Stack);
    n->size(Px(w), Px(h));
    n->getInlineBase().shrink = 0.0f;
    return n;
}


TEST(Layout_wrap, no_wrap_overflows) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = false;

    auto c1 = makeBox(60, 20);
    auto c2 = makeBox(60, 20);
    root->addChild(c1);
    root->addChild(c2);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    CHECK_NEAR(c1->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    CHECK_NEAR(c2->getRect().x, 60.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 0.0f, 1e-4);
}

TEST(Layout_wrap, wrap_moves_to_next_line) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = true;

    auto c1 = makeBox(60, 20);
    auto c2 = makeBox(60, 20);
    root->addChild(c1);
    root->addChild(c2);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    CHECK_NEAR(c1->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    
    CHECK_NEAR(c2->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 20.0f, 1e-4);
}

TEST(Layout_wrap, wrap_measures_correct_height) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Auto(), Auto()); 
    root->getInlineBase().wrap = true;
    root->getInlineBase().gap = Px(10);
    
    root->addChild(makeBox(60, 20));
    root->addChild(makeBox(60, 30));
    root->addChild(makeBox(60, 40));
    
    Vec2 s = root->measure(100, 1000);
    
    CHECK_NEAR(s.x, 60.0f, 1e-4);
    CHECK_NEAR(s.y, 110.0f, 1e-4);
}

TEST(Layout_wrap, wrap_multiple_lines_with_gap) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = true;
    root->getInlineBase().gap = Px(10);

    auto c1 = makeBox(40, 20);
    auto c2 = makeBox(40, 20);
    auto c3 = makeBox(40, 20);
    
    root->addChild(c1);
    root->addChild(c2);
    root->addChild(c3);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    CHECK_NEAR(c1->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    
    CHECK_NEAR(c2->getRect().x, 50.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 0.0f, 1e-4);

    CHECK_NEAR(c3->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c3->getRect().y, 30.0f, 1e-4);
}

TEST(Layout_wrap, wrap_justify_center) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = true;
    root->getInlineBase().justify = Justify::Center;

    auto c1 = makeBox(60, 20);
    auto c2 = makeBox(80, 20);
    
    root->addChild(c1);
    root->addChild(c2);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    CHECK_NEAR(c1->getRect().x, 20.0f, 1e-4);
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    
    CHECK_NEAR(c2->getRect().x, 10.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 20.0f, 1e-4);
}

TEST(Layout_wrap, wrap_cross_axis_alignment) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = true;
    root->getInlineBase().itemsV = Align::Center;

    auto c1 = makeBox(40, 40);
    auto c2 = makeBox(40, 20);
    
    root->addChild(c1);
    root->addChild(c2);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    
    CHECK_NEAR(c2->getRect().y, 10.0f, 1e-4);
}