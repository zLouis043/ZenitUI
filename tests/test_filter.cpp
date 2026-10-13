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
    // filters è set ma vuoto, o non set; in entrambi i casi il valore è []
    if (t.rules[0].style.filters.is_set)
        CHECK(t.rules[0].style.filters.value.empty());
    else
        CHECK(true);
}

TEST(Filter_chaining, two_filters_ping_pong_through_targets)
{
    Env env;
    env.renderer.supportsEffects = true; // <-- attiva il path "layer"

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

    // Subtree render: 1 pushTarget (into layerTarget_)
    // Filter 0 (fx1, not last): 1 pushTarget (into currentWrite)
    // Filter 1 (fx2, last): 0 pushTarget (writes to framebuffer)
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

    // Translate the node so its pivot sits well inside the mock viewport.
    // Without this, a 45-degree rotation would push the AABB past the
    // origin and the region would be clipped, making the test fragile.
    node->getInlineBase().translateX = Px(200);
    node->getInlineBase().translateY = Px(200);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    env.frame(root);
    env.frame(root);

    // No rotation: the region equals the node's rect.
    const int wNoRot = env.renderer.lastCreatedTargetW;
    const int hNoRot = env.renderer.lastCreatedTargetH;
    CHECK(wNoRot == 100);
    CHECK(hNoRot == 100);

    // Rotate 45 degrees. The AABB of a 100x100 square rotated 45 degrees
    // is 100 * sqrt(2) ~= 141.4. The target must grow to cover it.
    //
    // Disable the transition first: the default transitionTime is 0.15s,
    // so without this the rotation would only be ~5 degrees after one
    // frame, and ~10 after two, producing an AABB below the threshold.
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

    // Rotation also affects the AABB for arbitrary angles. Sanity check
    // for 30 degrees: the AABB is w*cos(a) + h*sin(a) in each axis.
    // For a 100x100 square: 100*cos(30) + 100*sin(30) ~= 86.6 + 50 = 136.6.
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