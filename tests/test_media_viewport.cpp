#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;


struct MediaScene
{
    std::shared_ptr<Layout> root;
    std::shared_ptr<Layout> node;
};

static MediaScene makeMediaScene(Env& env, float initialW)
{
    MediaScene s;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .target { background: #000000ff; }
        @media (min-width: 600px) {
            .target { background: #ff0000ff; }
        }
    )");

    s.node = std::make_shared<Layout>(LayoutType::Stack);
    s.node->cls("target");
    s.node->size(Px(100), Px(100));

    s.root = std::make_shared<Layout>(LayoutType::Stack);
    s.root->size(Percent(100), Percent(100));
    s.root->addChild(s.node);

    env.platform.viewport = { initialW, 600.0f };
    env.frame(s.root);
    env.frame(s.root);

    return s;
}


TEST(Media_viewport, applies_at_start_when_matching)
{
    Env env;
    auto s = makeMediaScene(env, 800.0f);

    CHECK(s.node->getStyle().background.r == 0xff);
    CHECK(s.node->getStyle().background.g == 0x00);
    CHECK(s.node->getStyle().background.b == 0x00);
}

TEST(Media_viewport, does_not_apply_at_start_when_not_matching)
{
    Env env;
    auto s = makeMediaScene(env, 400.0f);

    CHECK(s.node->getStyle().background.r == 0x00);
    CHECK(s.node->getStyle().background.g == 0x00);
    CHECK(s.node->getStyle().background.b == 0x00);
}


TEST(Media_viewport, viewport_shrink_disables_media)
{
    Env env;
    auto s = makeMediaScene(env, 800.0f);

    CHECK(s.node->getStyle().background.r == 0xff);

    env.platform.viewport = { 400.0f, 600.0f };
    env.frame(s.root);
    for (int i = 0; i < 20; ++i)
        env.frame(s.root);

    CHECK(s.node->getStyle().background.r == 0x00);
}

TEST(Media_viewport, viewport_grow_enables_media)
{
    Env env;
    auto s = makeMediaScene(env, 400.0f);

    CHECK(s.node->getStyle().background.r == 0x00);

    env.platform.viewport = { 800.0f, 600.0f };
    env.frame(s.root);
    for (int i = 0; i < 20; ++i)
        env.frame(s.root);

    CHECK(s.node->getStyle().background.r == 0xff);
}

TEST(Media_viewport, crossing_threshold_toggles_media)
{
    Env env;
    auto s = makeMediaScene(env, 800.0f);

    CHECK(s.node->getStyle().background.r == 0xff);

    env.platform.viewport = { 599.0f, 600.0f };
    env.frame(s.root);
    for (int i = 0; i < 20; ++i) env.frame(s.root);
    CHECK(s.node->getStyle().background.r == 0x00);

    env.platform.viewport = { 600.0f, 600.0f };
    env.frame(s.root);
    for (int i = 0; i < 20; ++i) env.frame(s.root);
    CHECK(s.node->getStyle().background.r == 0xff);
}


TEST(Media_viewport, boundary_is_inclusive)
{
    Env env;
    auto s = makeMediaScene(env, 600.0f);

    CHECK(s.node->getStyle().background.r == 0xff);
}

TEST(Media_viewport, just_below_boundary_does_not_match)
{
    Env env;
    auto s = makeMediaScene(env, 599.0f);

    CHECK(s.node->getStyle().background.r == 0x00);
}


TEST(Media_viewport, viewport_same_size_does_not_reresolve)
{
    Env env;
    auto s = makeMediaScene(env, 800.0f);

    Color before = s.node->getStyle().background;

    env.platform.viewport = { 800.0f, 600.0f };
    env.frame(s.root);
    env.frame(s.root);

    Color after = s.node->getStyle().background;
    CHECK(before == after);
}