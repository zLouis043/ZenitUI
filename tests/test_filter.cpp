#include "TestFramework.hpp"
#include "StyleParser.hpp"
#include "Theme.hpp"
#include "Mocks.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;
using namespace ZenitUI::Test;

TEST(Parser_filter, single_blur)
{
    Theme t;
    loadStyleString(".card { filter: blur(4px); }", t);
    CHECK(t.rules.size() == 1);
    const auto &f = t.rules[0].style.filters;
    CHECK(f.is_set);
    CHECK(f.value.size() == 1);
    CHECK(f.value[0].name == "blur");
    CHECK(f.value[0].args.size() == 1);
    CHECK(f.value[0].args[0] == "4px");
}

TEST(Parser_filter, no_args)
{
    Theme t;
    loadStyleString(".card { filter: grayscale; }", t);
    const auto &f = t.rules[0].style.filters;
    CHECK(f.is_set);
    CHECK(f.value.size() == 1);
    CHECK(f.value[0].name == "grayscale");
    CHECK(f.value[0].args.empty());
}

TEST(Parser_filter, comma_separated_two)
{
    Theme t;
    loadStyleString(".card { filter: blur(2px), drop-shadow(2px, 2px, #000); }", t);
    const auto &f = t.rules[0].style.filters;
    CHECK(f.is_set);
    CHECK(f.value.size() == 2);

    CHECK(f.value[0].name == "blur");
    CHECK(f.value[0].args.size() == 1);
    CHECK(f.value[0].args[0] == "2px");

    CHECK(f.value[1].name == "drop-shadow");
    CHECK(f.value[1].args.size() == 3);
    CHECK(f.value[1].args[0] == "2px");
    CHECK(f.value[1].args[1] == "2px");
    CHECK(f.value[1].args[2] == "#000");
}

TEST(Parser_filter, empty_value_leaves_empty_vector)
{
    Theme t;
    loadStyleString(".card { filter: ; }", t);
    if (t.rules[0].style.filters.is_set)
        CHECK(t.rules[0].style.filters.value.empty());
    else
        CHECK(true);
}

TEST(Filter_chaining, two_filters_ping_pong_through_targets)
{
    Env env;
    env.renderer.supportsEffects = true;

    env.assets.registerEffect("fx1");
    env.assets.registerEffect("fx2");

    FilterRegistry::get().add(FilterDef{
        .name = "fx1", .shader = "fx1", .pattern = FilterPattern::SinglePass, .params = {}});
    FilterRegistry::get().add(FilterDef{
        .name = "fx2", .shader = "fx2", .pattern = FilterPattern::SinglePass, .params = {}});

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(100), Px(100));
    node->getInlineBase().filters = std::vector<FilterRef>{
        {"fx1", {}}, {"fx2", {}}};

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    env.frame(root);

    CHECK(env.renderer.pushTargetCount == 2);
}

TEST(Filter_rotation, region_covers_rotated_bbox)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.assets.registerEffect("fx");

    FilterRegistry::get().add(FilterDef{
        .name = "fx", .shader = "fx", .pattern = FilterPattern::SinglePass, .params = {}});

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(100), Px(100));
    node->getInlineBase().filters = std::vector<FilterRef>{{"fx", {}}};

    node->getInlineBase().translateX = Px(200);
    node->getInlineBase().translateY = Px(200);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    env.frame(root);
    env.frame(root);

    const int wNoRot = env.renderer.lastCreatedTargetW;
    const int hNoRot = env.renderer.lastCreatedTargetH;
    CHECK(wNoRot == 100);
    CHECK(hNoRot == 100);

    node->getInlineBase().transitionTime = 0.0f;

    env.renderer.lastCreatedTargetW = 0;
    env.renderer.lastCreatedTargetH = 0;

    node->getInlineBase().rotation = 45.0f;
    env.frame(root);
    env.frame(root);

    const int wRot = env.renderer.lastCreatedTargetW;
    const int hRot = env.renderer.lastCreatedTargetH;
    CHECK(wRot >= 141);
    CHECK(hRot >= 141);

    env.renderer.lastCreatedTargetW = 0;
    env.renderer.lastCreatedTargetH = 0;

    node->getInlineBase().rotation = 30.0f;
    env.frame(root);
    env.frame(root);

    const int w30 = env.renderer.lastCreatedTargetW;
    const int h30 = env.renderer.lastCreatedTargetH;
    CHECK(w30 >= 136);
    CHECK(h30 >= 136);
}

TEST(Filter_nested, falls_back_to_inline_when_nested_targets_unsupported)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.renderer.supportsNestedTargets = false;
    env.assets.registerEffect("fx");

    FilterRegistry::get().add(FilterDef{
        .name = "fx", .shader = "fx", .pattern = FilterPattern::SinglePass, .params = {}});

    auto outer = std::make_shared<Layout>(LayoutType::Stack);
    outer->size(Px(200), Px(200));
    outer->getInlineBase().filters = std::vector<FilterRef>{{"fx", {}}};

    auto inner = std::make_shared<Layout>(LayoutType::Stack);
    inner->size(Px(100), Px(100));
    inner->getInlineBase().filters = std::vector<FilterRef>{{"fx", {}}};

    outer->addChild(inner);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(outer);

    env.frame(root);

    CHECK(env.renderer.pushTargetCount == 1);
    CHECK(env.renderer.activeTargetDepth == 0);
}

TEST(Filter_nested, renders_nested_layer_when_supported)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.renderer.supportsNestedTargets = true;
    env.assets.registerEffect("fx");

    FilterRegistry::get().add(FilterDef{
        .name = "fx", .shader = "fx", .pattern = FilterPattern::SinglePass, .params = {}});

    auto outer = std::make_shared<Layout>(LayoutType::Stack);
    outer->size(Px(200), Px(200));
    outer->getInlineBase().filters = std::vector<FilterRef>{{"fx", {}}};

    auto inner = std::make_shared<Layout>(LayoutType::Stack);
    inner->size(Px(100), Px(100));
    inner->getInlineBase().filters = std::vector<FilterRef>{{"fx", {}}};

    outer->addChild(inner);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(outer);

    env.frame(root);

    CHECK(env.renderer.pushTargetCount == 2);
    CHECK(env.renderer.activeTargetDepth == 0);
}