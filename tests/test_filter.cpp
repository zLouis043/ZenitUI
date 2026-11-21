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

TEST(Filter_perf, lazy_allocation_per_kind)
{
    auto runChain = [](std::vector<FilterRef> chain) -> int
    {
        Env env;
        env.renderer.supportsEffects = true;
        env.renderer.supportsTargetSubRect = true;
        env.assets.registerEffect("blur");
        env.assets.registerEffect("dropShadow");

        auto node = std::make_shared<Layout>(LayoutType::Stack);
        node->size(Px(200), Px(200));
        node->getInlineBase().filters = std::move(chain);

        auto root = std::make_shared<Layout>(LayoutType::Stack);
        root->size(Percent(100), Percent(100));
        root->addChild(node);

        env.frame(root);
        CHECK(UIContext::get().renderStats.targetsCreated == (uint32_t)env.renderer.createTargetCount);
        CHECK(UIContext::get().renderStats.liveTargets == (uint32_t)env.renderer.liveTargets);
        return env.renderer.createTargetCount;
    };

    CHECK(runChain({{"drop-shadow", {"2px", "2px", "#000"}}}) == 1);
    CHECK(runChain({{"blur", {"4px"}}}) == 2);
    CHECK(runChain({{"blur", {"4px"}}, {"drop-shadow", {"2px", "2px", "#000"}}}) == 3);
    CHECK(runChain({{"drop-shadow", {"2px", "2px", "#000"}}, {"blur", {"4px"}}}) == 3);
}

TEST(Filter_perf, static_node_zero_churn_after_warmup)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.renderer.supportsTargetSubRect = true;
    env.assets.registerEffect("blur");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(200), Px(200));
    node->getInlineBase().filters = std::vector<FilterRef>{{"blur", {"4px"}}};

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    env.frame(root);
    CHECK(env.renderer.createTargetCount == 2);

    for (int i = 0; i < 60; ++i)
        env.frame(root);

    CHECK(env.renderer.createTargetCount == 2);
    CHECK(env.renderer.destroyTargetCount == 0);
}

TEST(Filter_perf, scale_rotation_clip_sweeps_bounded_by_buckets)
{
    // 1) Scale sweep 1.0 -> 1.15 su 200x200 con blur + drop-shadow (prima: 45)
    {
        Env env;
        env.renderer.supportsEffects = true;
        env.renderer.supportsTargetSubRect = true;
        env.assets.registerEffect("blur");
        env.assets.registerEffect("dropShadow");

        auto node = std::make_shared<Layout>(LayoutType::Stack);
        node->size(Px(200), Px(200));
        node->getInlineBase().translateX = Px(200);
        node->getInlineBase().translateY = Px(200);
        node->getInlineBase().filters = std::vector<FilterRef>{
            {"blur", {"4px"}}, {"drop-shadow", {"2px", "2px", "#000"}}};

        auto root = std::make_shared<Layout>(LayoutType::Stack);
        root->size(Percent(100), Percent(100));
        root->addChild(node);

        for (int i = 0; i < 60; ++i)
        {
            node->getInlineBase().scale = 1.0f + 0.15f * ((float)i / 59.0f);
            env.frame(root);
        }
        CHECK(env.renderer.createTargetCount <= 3);
    }

    // 2) Rotation sweep +3 deg/frame su 200x200 con blur (prima: 165)
    {
        Env env;
        env.renderer.supportsEffects = true;
        env.renderer.supportsTargetSubRect = true;
        env.assets.registerEffect("blur");

        auto node = std::make_shared<Layout>(LayoutType::Stack);
        node->size(Px(200), Px(200));
        node->getInlineBase().translateX = Px(200);
        node->getInlineBase().translateY = Px(200);
        node->getInlineBase().filters = std::vector<FilterRef>{{"blur", {"4px"}}};

        auto root = std::make_shared<Layout>(LayoutType::Stack);
        root->size(Percent(100), Percent(100));
        root->addChild(node);

        for (int i = 0; i < 60; ++i)
        {
            node->getInlineBase().rotation = (float)(i * 3);
            env.frame(root);
        }
        CHECK(env.renderer.createTargetCount <= 6);
    }

    // 3) Clip sweep oltre il bordo viewport su 200x200 con blur (prima: 177)
    {
        Env env;
        env.renderer.supportsEffects = true;
        env.renderer.supportsTargetSubRect = true;
        env.renderer.currentClip = {0, 0, 800, 600};
        env.assets.registerEffect("blur");

        auto node = std::make_shared<Layout>(LayoutType::Stack);
        node->size(Px(200), Px(200));
        node->getInlineBase().translateY = Px(100);
        node->getInlineBase().filters = std::vector<FilterRef>{{"blur", {"4px"}}};

        auto root = std::make_shared<Layout>(LayoutType::Stack);
        root->size(Percent(100), Percent(100));
        root->addChild(node);

        for (int i = 0; i < 60; ++i)
        {
            node->getInlineBase().translateX = Px(600.0f + (float)i * 3.0f);
            env.frame(root);
        }
        CHECK(env.renderer.createTargetCount <= 10);
    }
}

TEST(Filter_perf, pool_bounds_50_cards_and_trims_after_grace)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.renderer.supportsTargetSubRect = true;
    env.assets.registerEffect("blur");
    env.assets.registerEffect("dropShadow");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    for (int i = 0; i < 50; ++i)
    {
        auto card = std::make_shared<Layout>(LayoutType::Stack);
        card->size(Px(200), Px(200));
        card->getInlineBase().filters = std::vector<FilterRef>{
            {"blur", {"4px"}}, {"drop-shadow", {"2px", "2px", "#000"}}};
        root->addChild(card);
    }

    env.frame(root);
    CHECK(env.renderer.liveTargets <= 3);
    CHECK(UIContext::get().targetPool.liveCount() <= 3);
    CHECK(UIContext::get().renderStats.liveTargets == (uint32_t)env.renderer.liveTargets);

    // Rimuove le card e lascia passare il grace period (120 frame)
    root->removeAllChildren();
    for (int i = 0; i < 121; ++i)
        env.frame(root);

    CHECK(env.renderer.liveTargets == 0);
    CHECK(UIContext::get().targetPool.liveCount() == 0);
    CHECK(UIContext::get().renderStats.liveTargets == 0);
}

TEST(Filter_perf, pool_nested_layers_use_distinct_targets)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.renderer.supportsNestedTargets = true;
    env.renderer.supportsTargetSubRect = true;
    env.assets.registerEffect("blur");

    auto outer = std::make_shared<Layout>(LayoutType::Stack);
    outer->size(Px(200), Px(200));
    outer->getInlineBase().filters = std::vector<FilterRef>{{"blur", {"4px"}}};

    auto inner = std::make_shared<Layout>(LayoutType::Stack);
    inner->size(Px(200), Px(200));
    inner->getInlineBase().filters = std::vector<FilterRef>{{"blur", {"4px"}}};

    outer->addChild(inner);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(outer);

    env.frame(root);

    CHECK(env.renderer.activeTargetDepth == 0);
    CHECK(env.renderer.liveTargets >= 2);
    CHECK(env.renderer.liveTargets <= 4);
}

TEST(Filter_perf, fallback_without_subrect_uses_exact_size_and_pools)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.renderer.supportsTargetSubRect = false;
    env.assets.registerEffect("dropShadow");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    for (int i = 0; i < 50; ++i)
    {
        auto card = std::make_shared<Layout>(LayoutType::Stack);
        card->size(Px(100), Px(100));
        card->getInlineBase().filters = std::vector<FilterRef>{
            {"drop-shadow", {"2px", "2px", "#000"}}};
        root->addChild(card);
    }

    env.frame(root);
    CHECK(env.renderer.lastCreatedTargetW == 100);
    CHECK(env.renderer.lastCreatedTargetH == 100);
    CHECK(env.renderer.liveTargets == 1);
}

TEST(Filter_perf, texSize_equals_bucket_and_dst_equals_region)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.renderer.supportsTargetSubRect = true;

    Rect seenDst{};
    Vec2 seenTexSize{};
    FilterRegistry::get().addCustom("inspect_tex", [&](const FilterContext &c)
    {
        seenDst = c.dst;
        seenTexSize = c.texSize;
        c.renderer->drawTarget(c.src, c.dst, c.region, Colors::White);
    });

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(100), Px(100));
    node->getInlineBase().filters = std::vector<FilterRef>{{"inspect_tex", {}}};

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    env.frame(root);

    CHECK_NEAR(seenDst.width, 100.0f, 0.5f);
    CHECK_NEAR(seenDst.height, 100.0f, 0.5f);
    CHECK_NEAR(seenTexSize.x, 128.0f, 0.5f);
    CHECK_NEAR(seenTexSize.y, 128.0f, 0.5f);
    CHECK_NEAR(env.renderer.lastDrawTargetSrc.width, 100.0f, 0.5f);
    CHECK_NEAR(env.renderer.lastDrawTargetSrc.height, 100.0f, 0.5f);
}

TEST(Filter_perf, separable_downscale_allocates_half_res_intermediate)
{
    Env env;
    env.renderer.supportsEffects = true;
    env.renderer.supportsTargetSubRect = true;
    env.assets.registerEffect("blur");

    FilterRegistry::get().add(FilterDef{
        .name = "blur-fast",
        .shader = "blur",
        .pattern = FilterPattern::Separable,
        .params = {FilterParam{"radius", FilterParamType::Float, 0, 4.0f}},
        .custom = {},
        .downscale = 2});

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(200), Px(200));
    node->getInlineBase().filters = std::vector<FilterRef>{{"blur-fast", {"4px"}}};

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    env.frame(root);

    // Main target = 256x256 (bucket di 200), intermediate = 128x128 (bucket di 100)
    CHECK(env.renderer.createTargetCount == 2);
    CHECK(env.renderer.lastCreatedTargetW == 128);
    CHECK(env.renderer.lastCreatedTargetH == 128);
}