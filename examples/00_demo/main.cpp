// @doc   Flagship ZenitUI showcase: live Dark/Light theming, ZMarkup prefabs & keyed Each, Bindable<T>, UTF-8 TextArea, 10k-row VirtualView, portals, filters, and CanvasLayout.
// @group grp_core grp_widgets grp_zmarkup grp_zstyle grp_anim grp_backends

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "ZenitUI.hpp"
#include "ShowcaseApp.hpp"

#include <cstdio>

using namespace ZenitUI;

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1180, 740, "ZenitUI — Showcase");
    SetTargetFPS(60);
    SetWindowMinSize(640, 480);

    // 1. Bind the Raylib backend to the global UIContext.
    RaylibRenderer renderer;
    RaylibPlatform platform;
    RaylibAssetProvider assets(renderer);

    auto &ctx = UIContext::get();
    ctx.renderer = &renderer;
    ctx.platform = &platform;
    ctx.assets = &assets;

    // 2. Load SIL OFL Inter font weights (baked with Latin + common symbols).
    assets.defaultCodepoints().addRange(0x2000, 0x206F); // General punctuation (• — …)
    assets.defaultCodepoints().addRange(0x2190, 0x21FF); // Arrows (→ ← ↑ ↓)
    assets.loadFont("inter", "assets/fonts/Inter_18pt-Regular.ttf", 48);
    assets.loadFont("inter-semibold", "assets/fonts/Inter_18pt-SemiBold.ttf", 48);
    assets.loadFont("inter-bold", "assets/fonts/Inter_18pt-Bold.ttf", 48);

    // 3. Load shared nine-slice texture and post-processing shaders.
    assets.loadTexture("npatches", "assets/images/npatches_y.png");
    assets.loadEffect("blur", "assets/shaders/blur.fs");
    assets.loadEffect("dropShadow", "assets/shaders/drop_shadow.fs");
    assets.loadEffect("hueShift", "assets/shaders/hue_shift.fs");
    assets.loadEffect("sepia", "assets/shaders/sepia.fs");

    // 4. Register the custom `sepia` filter in FilterRegistry.
    FilterRegistry::get().add(FilterDef{
        .name = "sepia",
        .shader = "sepia",
        .pattern = FilterPattern::SinglePass,
        .params = {{"amount", FilterParamType::Float, 0, 0.85f}},
    });

    // 5. Load the showcase stylesheet (co-located with the example).
    if (!ZMarkup::loadStyleFile("examples/00_demo/theme.zstyle"))
    {
        std::fprintf(stderr, "[00_demo] Failed to load examples/00_demo/theme.zstyle\n");
        CloseWindow();
        return 1;
    }

    // 6. Build the multi-view showcase app and run the standard frame loop.
    ShowcaseApp app;
    auto root = app.root();

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        root->runFrame(dt);

        BeginDrawing();
        ClearBackground(GetColor(0x111217FF));
        root->renderFrame();
        EndDrawing();

        app.tickStats(GetFPS(), dt * 1000.0f);
    }

    ctx.targetPool.clear(renderer, &ctx.renderStats);
    CloseWindow();
    return 0;
}