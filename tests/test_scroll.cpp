#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "ZenitUIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;


struct VScrollScene
{
    std::shared_ptr<Layout> root;
    std::shared_ptr<Layout> sv;
    float contentH{0.0f};
};

static VScrollScene makeVScroll(Env& env,
                                 float svW = 200.0f,
                                 float svH = 100.0f,
                                 float contentH = 500.0f)
{
    VScrollScene s;
    s.contentH = contentH;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow-y: scroll; }
    )");

    s.root = std::make_shared<Layout>(LayoutType::Stack);
    s.root->size(Percent(100), Percent(100));

    s.sv = std::make_shared<Layout>(LayoutType::Vertical);
    s.sv->size(Px(svW), Px(svH));
    s.sv->cls("scroller");

    auto content = std::make_shared<Layout>(LayoutType::Stack);
    content->size(Auto(), Px(contentH));
    s.sv->addChild(content);

    s.root->addChild(s.sv);
    env.frame(s.root);
    env.frame(s.root);
    return s;
}


TEST(Scroll_offset, scrollTo_sets_absolute_position)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollTo(0.0f, 250.0f);
    CHECK_NEAR(s.sv->getScrollY(), 250.0f, 1e-3);
}

TEST(Scroll_offset, scrollToX_sets_only_x)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollTo(0.0f, 100.0f);
    s.sv->scrollToX(0.0f);
    CHECK_NEAR(s.sv->getScrollY(), 100.0f, 1e-3);
}

TEST(Scroll_offset, scrollToY_sets_only_y)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToY(300.0f);
    CHECK_NEAR(s.sv->getScrollY(), 300.0f, 1e-3);
}

TEST(Scroll_offset, scrollToTop_snaps_to_zero)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToY(200.0f);
    s.sv->scrollToTop();
    CHECK_NEAR(s.sv->getScrollY(), 0.0f, 1e-3);
}

TEST(Scroll_offset, scrollToBottom_snaps_to_max)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToBottom();
    CHECK_NEAR(s.sv->getScrollY(), s.sv->getMaxScrollY(), 1e-3);
    CHECK(s.sv->getScrollY() > 0.0f);
}

TEST(Scroll_offset, scroll_is_clamped_to_max)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToY(99999.0f);
    CHECK_NEAR(s.sv->getScrollY(), s.sv->getMaxScrollY(), 1e-3);
}

TEST(Scroll_offset, scroll_is_clamped_to_zero)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToY(-100.0f);
    CHECK_NEAR(s.sv->getScrollY(), 0.0f, 1e-3);
}


TEST(Scroll_max, max_is_content_minus_viewport)
{
    Env env;
    auto s = makeVScroll(env, 200.0f, 100.0f, 500.0f);

    CHECK_NEAR(s.sv->getMaxScrollY(), 400.0f, 1.0f);
}

TEST(Scroll_max, zero_when_content_fits)
{
    Env env;
    auto s = makeVScroll(env, 200.0f, 100.0f, 50.0f);

    CHECK_NEAR(s.sv->getMaxScrollY(), 0.0f, 1e-3);
    CHECK_NEAR(s.sv->getMaxScrollX(), 0.0f, 1e-3);
}


TEST(Scroll_wheel, adds_velocity_not_offset_directly)
{
    Env env;
    auto s = makeVScroll(env);

    Rect r = s.sv->getRect();
    env.platform.moveMouse({r.x + r.width * 0.5f,
                            r.y + r.height * 0.5f});
    env.frame(s.root);

    env.platform.scrollWheel(-1.0f);
    env.frame(s.root);

    CHECK(s.sv->getScrollY() > 0.0f);

    env.frame(s.root);
    CHECK(s.sv->getScrollY() > 0.0f);
}

TEST(Scroll_wheel, wheel_outside_does_nothing)
{
    Env env;
    auto s = makeVScroll(env);

    env.platform.moveMouse({600.0f, 400.0f});
    env.frame(s.root);

    env.platform.scrollWheel(-1.0f);
    env.frame(s.root);

    CHECK_NEAR(s.sv->getScrollY(), 0.0f, 1e-3);
}

TEST(Scroll_wheel, shift_switches_to_horizontal)
{
    Env env;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .hscroller { overflow-x: scroll; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto sv = std::make_shared<Layout>(LayoutType::Horizontal);
    sv->size(Px(100), Px(80));
    sv->cls("hscroller");

    auto content = std::make_shared<Layout>(LayoutType::Stack);
    content->size(Px(500), Auto());
    sv->addChild(content);

    root->addChild(sv);
    env.frame(root);
    env.frame(root);

    Rect r = sv->getRect();
    env.platform.moveMouse({r.x + r.width * 0.5f,
                            r.y + r.height * 0.5f});
    env.platform.mods.shift = true;
    env.frame(root);

    env.platform.scrollWheel(-1.0f);
    env.frame(root);

    CHECK(sv->getScrollX() > 0.0f);
    CHECK_NEAR(sv->getScrollY(), 0.0f, 1e-3);
}


TEST(Scroll_inertia, velocity_decays_over_time)
{
    Env env;
    auto s = makeVScroll(env);

    Rect r = s.sv->getRect();
    env.platform.moveMouse({r.x + r.width * 0.5f,
                            r.y + r.height * 0.5f});
    env.frame(s.root);

    env.platform.scrollWheel(-3.0f);
    env.frame(s.root);

    float y0 = s.sv->getScrollY();

    for (int i = 0; i < 3; ++i)
        env.frame(s.root);

    float y1 = s.sv->getScrollY();
    CHECK(y1 > y0);

    for (int i = 0; i < 60; ++i)
        env.frame(s.root);

    float y2 = s.sv->getScrollY();
    for (int i = 0; i < 10; ++i)
        env.frame(s.root);

    float y3 = s.sv->getScrollY();
    CHECK_NEAR(y3, y2, 0.5f);

    CHECK(y3 <= s.sv->getMaxScrollY() + 0.5f);
}

TEST(Scroll_inertia, clamps_at_max_during_fling)
{
    Env env;
    auto s = makeVScroll(env, 200.0f, 100.0f, 200.0f);

    CHECK_NEAR(s.sv->getMaxScrollY(), 100.0f, 1.0f);

    Rect r = s.sv->getRect();
    env.platform.moveMouse({r.x + r.width * 0.5f,
                            r.y + r.height * 0.5f});
    env.frame(s.root);

    env.platform.scrollWheel(-10.0f);
    env.frame(s.root);

    for (int i = 0; i < 30; ++i)
        env.frame(s.root);

    CHECK(s.sv->getScrollY() <= s.sv->getMaxScrollY() + 0.5f);
}


TEST(Scroll_nested, inner_consumes_wheel)
{
    Env env;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow-y: scroll; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto outer = std::make_shared<Layout>(LayoutType::Vertical);
    outer->size(Px(300), Px(300));
    outer->cls("scroller");

    auto outerContent = std::make_shared<Layout>(LayoutType::Stack);
    outerContent->size(Auto(), Px(800));
    outer->addChild(outerContent);

    auto inner = std::make_shared<Layout>(LayoutType::Vertical);
    inner->size(Px(200), Px(100));
    inner->cls("scroller");

    auto innerContent = std::make_shared<Layout>(LayoutType::Stack);
    innerContent->size(Auto(), Px(500));
    inner->addChild(innerContent);
    outerContent->addChild(inner);

    root->addChild(outer);
    env.frame(root);
    env.frame(root);

    Rect ri = inner->getRect();
    env.platform.moveMouse({ri.x + ri.width * 0.5f,
                            ri.y + ri.height * 0.5f});
    env.frame(root);

    env.platform.scrollWheel(-3.0f);
    env.frame(root);
    env.frame(root);

    CHECK(inner->getScrollY() > 0.0f);
    CHECK_NEAR(outer->getScrollY(), 0.0f, 1e-3);
}

TEST(Scroll_nested, outer_consumes_wheel_outside_inner)
{
    Env env;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow-y: scroll; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto outer = std::make_shared<Layout>(LayoutType::Vertical);
    outer->size(Px(300), Px(300));
    outer->cls("scroller");

    auto outerContent = std::make_shared<Layout>(LayoutType::Stack);
    outerContent->size(Auto(), Px(800));
    outer->addChild(outerContent);

    auto inner = std::make_shared<Layout>(LayoutType::Vertical);
    inner->size(Px(100), Px(50));
    inner->cls("scroller");

    auto innerContent = std::make_shared<Layout>(LayoutType::Stack);
    innerContent->size(Auto(), Px(500));
    inner->addChild(innerContent);
    outerContent->addChild(inner);

    root->addChild(outer);
    env.frame(root);
    env.frame(root);

    Rect ro = outer->getRect();
    env.platform.moveMouse({ro.x + 250.0f, ro.y + 200.0f});
    env.frame(root);

    env.platform.scrollWheel(-3.0f);
    env.frame(root);
    env.frame(root);

    CHECK(outer->getScrollY() > 0.0f);
    CHECK_NEAR(inner->getScrollY(), 0.0f, 1e-3);
}


TEST(Scroll_thumb, click_on_thumb_starts_capture)
{
    Env env;
    auto s = makeVScroll(env);

    env.platform.pressLeft({193.0f, 19.0f});
    env.frame(s.root);

    CHECK(s.sv->hasPointerCapture());

    env.platform.releaseLeft({193.0f, 19.0f});
    env.frame(s.root);
    CHECK(!s.sv->hasPointerCapture());
}

TEST(Scroll_thumb, drag_down_scrolls_content)
{
    Env env;
    auto s = makeVScroll(env);

    env.platform.pressLeft({193.0f, 19.0f});
    env.frame(s.root);
    CHECK(s.sv->getScrollY() == 0.0f);

    env.platform.holdLeft({193.0f, 39.0f});
    env.frame(s.root);

    CHECK(s.sv->getScrollY() > 50.0f);
    CHECK(s.sv->getScrollY() <= s.sv->getMaxScrollY());

    env.platform.releaseLeft({193.0f, 39.0f});
    env.frame(s.root);
    CHECK(!s.sv->hasPointerCapture());
}

TEST(Scroll_thumb, click_outside_thumb_does_not_capture)
{
    Env env;
    auto s = makeVScroll(env);

    env.platform.pressLeft({50.0f, 50.0f});
    env.frame(s.root);

    CHECK(!s.sv->hasPointerCapture());

    env.platform.releaseLeft({50.0f, 50.0f});
    env.frame(s.root);
}

TEST(Scroll_thumb, click_on_track_jumps_to_position)
{
    Env env;
    auto s = makeVScroll(env);

    env.platform.pressLeft({193.0f, 80.0f});
    env.frame(s.root);

    CHECK_NEAR(s.sv->getScrollY(), 350.0f, 1.0f);

    env.platform.releaseLeft({193.0f, 80.0f});
    env.frame(s.root);
}