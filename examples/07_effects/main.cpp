// ZenitUI — Example 07: Effects
//
// Two ways to post-process a node:
//
//   filter: <name>(...)   — renders the subtree into a layer, then runs
//                           the filter chain on the composited image.
//                           Used for blur, drop-shadow, color grading.
//
//   effect: <shader-name> — pushes a shader around the node's own draw
//                           calls. Cheaper, but the shader sees the
//                           individual primitives, not the composited image.
//
// The example also registers a custom filter (`sepia`) with the
// FilterRegistry, showing how to plug a new shader into the pipeline.
//
// Build:   make examples
// Run:     make run-07_effects

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<Layout> Row(const std::string& title,
                                   std::shared_ptr<Layout> content)
{
    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->cls("row");

    auto l = Label(title);
    l->cls("row-label");

    row->addChild(l);
    row->addChild(content);
    return row;
}

static std::shared_ptr<Layout> Card(const std::string& text, const char* cls)
{
    auto c = std::make_shared<Layout>(LayoutType::Stack);
    c->cls("card");
    c->cls(cls);
    c->addChild(Label(text));
    return c;
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(760, 540, "ZenitUI — 07 Effects");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    // -----------------------------------------------------------------------
    //  Load shaders used by both `filter:` and `effect:`.
    //  Paths are relative to the repository root.
    // -----------------------------------------------------------------------
    assets.loadEffect("blur",       "assets/blur.fs");
    assets.loadEffect("dropShadow", "assets/drop_shadow.fs");
    assets.loadEffect("hueShift",   "assets/hue_shift.fs");
    assets.loadEffect("sepia",      "assets/sepia.fs");

    // -----------------------------------------------------------------------
    //  Register the sepia filter. `blur` and `drop-shadow` are built-in
    //  (registered lazily on first use); `sepia` is user-defined.
    // -----------------------------------------------------------------------
    FilterRegistry::get().add(FilterDef{
        .name    = "sepia",
        .shader  = "sepia",
        .pattern = FilterPattern::SinglePass,
        .params  = {
            {"amount", FilterParamType::Float, 0, 0.8f}
        }
    });

    // -----------------------------------------------------------------------
    //  Stylesheet.
    // -----------------------------------------------------------------------
    ZMarkup::loadStyleFile("examples/07_effects/theme.zstyle");

    // -----------------------------------------------------------------------
    //  Tree.
    // -----------------------------------------------------------------------
    auto page = std::make_shared<Layout>(LayoutType::Vertical);
    page->setStyleTag("Stack");
    page->id("page");

    auto panel = std::make_shared<Layout>(LayoutType::Vertical);
    panel->setStyleTag("Panel");

    auto title = Label("Effects demo");
    title->cls("title");
    panel->addChild(title);

    panel->addChild(Row("1. filter: blur(3px)", Card("blur", "filter-blur")));
    panel->addChild(Row("2. filter: drop-shadow", Card("shadow", "filter-shadow")));
    panel->addChild(Row("3. filter: sepia (custom)", Card("sepia", "filter-sepia")));
    panel->addChild(Row("4. effect: hueShift", Card("hue shift", "effect-hue")));
    panel->addChild(Row("5. two filters (only last shown)", Card("both", "filter-both")));

    page->addChild(panel);

    // -----------------------------------------------------------------------
    //  Main loop.
    // -----------------------------------------------------------------------
    while (!WindowShouldClose())
    {
        page->runFrame(GetFrameTime());

        BeginDrawing();
        ClearBackground(::BLACK);
        page->renderFrame();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}