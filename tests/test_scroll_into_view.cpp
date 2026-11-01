#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;


struct ScrollScene
{
    std::shared_ptr<Layout> root;
    std::shared_ptr<Layout> sv;
    std::shared_ptr<Layout> target;
};

static ScrollScene makeScrollScene(Env& env,
                                    float h_tall = 500.0f,
                                    float h_short = 20.0f)
{
    ScrollScene s;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow-y: scroll; }
    )");

    s.root = std::make_shared<Layout>(LayoutType::Stack);
    s.root->size(Percent(100), Percent(100));

    s.sv = std::make_shared<Layout>(LayoutType::Vertical);
    s.sv->size(Px(200), Px(100));
    s.sv->cls("scroller");

    auto f1 = std::make_shared<Layout>(LayoutType::Stack);
    f1->size(Auto(), Px(h_tall));
    s.sv->addChild(f1);

    s.target = std::make_shared<Layout>(LayoutType::Stack);
    s.target->size(Px(100), Px(h_short));
    s.sv->addChild(s.target);

    auto f2 = std::make_shared<Layout>(LayoutType::Stack);
    f2->size(Auto(), Px(h_tall));
    s.sv->addChild(f2);

    s.root->addChild(s.sv);
    env.frame(s.root);
    env.frame(s.root);

    return s;
}

TEST(Widget_scrollIntoView, no_ancestor_is_noop)
{
    Env env;

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(100), Px(100));

    CHECK(!node->scrollIntoView());
}

TEST(Widget_scrollIntoView, already_visible_is_noop)
{
    Env env;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow-y: scroll; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto sv = std::make_shared<Layout>(LayoutType::Vertical);
    sv->size(Px(200), Px(100));
    sv->cls("scroller");

    auto target = std::make_shared<Layout>(LayoutType::Stack);
    target->size(Px(100), Px(20));
    sv->addChild(target);

    auto filler = std::make_shared<Layout>(LayoutType::Stack);
    filler->size(Auto(), Px(500));
    sv->addChild(filler);

    root->addChild(sv);
    env.frame(root);
    env.frame(root);

    CHECK(sv->getScrollY() == 0.0f);
    CHECK(!target->scrollIntoView());
    CHECK(sv->getScrollY() == 0.0f);
}


TEST(Widget_scrollIntoView, below_view_scrolls_down)
{
    Env env;
    auto scene = makeScrollScene(env);

    CHECK(scene.sv->getScrollY() == 0.0f);
    CHECK(scene.target->scrollIntoView());
    CHECK_NEAR(scene.sv->getScrollY(), 420.0f, 1.0f);
}


TEST(Widget_scrollIntoView, above_view_scrolls_up)
{
    Env env;
    auto scene = makeScrollScene(env);

    scene.sv->scrollToY(550.0f);
    env.frame(scene.root);
    env.frame(scene.root);

    CHECK(scene.target->scrollIntoView());
    CHECK_NEAR(scene.sv->getScrollY(), 500.0f, 1.0f);
}


TEST(Widget_scrollIntoView, margin_reserves_space)
{
    Env env;
    auto scene = makeScrollScene(env);

    CHECK(scene.target->scrollIntoView(20.0f));
    CHECK_NEAR(scene.sv->getScrollY(), 440.0f, 1.0f);
}


TEST(Widget_scrollIntoView, subrect_is_used_instead_of_full_rect)
{
    Env env;
    auto scene = makeScrollScene(env);

    Rect sub = scene.target->getRect();
    sub.height = 5.0f;

    CHECK(scene.target->scrollIntoView(sub));
    CHECK_NEAR(scene.sv->getScrollY(), 405.0f, 1.0f);
}


TEST(Widget_scrollIntoView, second_call_is_noop)
{
    Env env;
    auto scene = makeScrollScene(env);

    CHECK(scene.target->scrollIntoView());
    CHECK_NEAR(scene.sv->getScrollY(), 420.0f, 1.0f);

    env.frame(scene.root);
    env.frame(scene.root);

    CHECK(!scene.target->scrollIntoView());
    CHECK_NEAR(scene.sv->getScrollY(), 420.0f, 1.0f);
}