// @doc   Flagship ZenitUI showcase: live Dark/Light theming, ZMarkup prefabs & keyed Each, Bindable<T>, UTF-8 TextArea, 10k-row VirtualView, portals, filters, and CanvasLayout.
// @group grp_core grp_widgets grp_zmarkup grp_zstyle grp_anim grp_backends

#include "Backend.hpp"
#include "ZenitUI.hpp"

#if defined(ZENITUI_DEMO_BACKEND_SDL3)
#include "SDL3Backend.hpp"
static std::unique_ptr<ZenitUI::IBackend> createDemoBackend()
{
    return std::make_unique<ZenitUI::SDL3Backend>();
}
#else
#include "RaylibBackend.hpp"
static std::unique_ptr<ZenitUI::IBackend> createDemoBackend()
{
    return std::make_unique<ZenitUI::RaylibBackend>();
}
#endif

#include "ShowcaseApp.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

using namespace ZenitUI;

int main()
{
    auto backend = createDemoBackend();
    UIContext ctx;

    WindowConfig cfg;
    cfg.title = "ZenitUI — Showcase (" + std::string(backend->name()) + ")";
    cfg.width = 1180;
    cfg.height = 740;
    cfg.minWidth = 640;
    cfg.minHeight = 480;
    cfg.targetFps = 60;
    cfg.resizable = true;
    cfg.vsync = true;
    cfg.msaa = true;

    if (!backend->initAndBind(cfg, ctx))
    {
        std::fprintf(stderr, "[00_demo] Failed to initialize backend '%.*s'\n",
                     static_cast<int>(backend->name().size()), backend->name().data());
        return 1;
    }

    // 1. Load SIL OFL Inter font weights (baked with Latin + common symbols).
    auto &assets = backend->assets();
    assets.setDefaultFontSize(48);
    assets.addDefaultCodepointRange(0x2000, 0x206F); // General punctuation (• — …)
    assets.addDefaultCodepointRange(0x2190, 0x21FF); // Arrows (→ ← ↑ ↓)
    assets.loadFont("inter", "assets/fonts/Inter_18pt-Regular.ttf");
    assets.loadFont("inter-semibold", "assets/fonts/Inter_18pt-SemiBold.ttf");
    assets.loadFont("inter-bold", "assets/fonts/Inter_18pt-Bold.ttf");

    // 2. Load shared nine-slice texture and post-processing shaders.
    assets.loadTexture("npatches", "assets/images/npatches_y.png");
    assets.loadEffect("blur", "assets/shaders/blur.fs");
    assets.loadEffect("dropShadow", "assets/shaders/drop_shadow.fs");
    assets.loadEffect("hueShift", "assets/shaders/hue_shift.fs");
    assets.loadEffect("sepia", "assets/shaders/sepia.fs");

    // 3. Register the custom `sepia` filter in FilterRegistry.
    ctx.filters.add(FilterDef{
        .name = "sepia",
        .shader = "sepia",
        .pattern = FilterPattern::SinglePass,
        .params = {{"amount", FilterParamType::Float, 0, 0.85f}},
    });

    // 4. Load the showcase stylesheet (co-located with the example).
    if (!ZMarkup::loadStyleFile("examples/00_demo/theme.zstyle", ctx.theme))
    {
        std::fprintf(stderr, "[00_demo] Failed to load examples/00_demo/theme.zstyle\n");
        backend->shutdown();
        return 1;
    }

    // 5. Build the multi-view showcase app and run the backend-agnostic loop.
    ShowcaseApp app(ctx);
    auto root = app.root();
    ctx.mount(root);

    constexpr ZenitUI::Color kClearColor{0x11, 0x12, 0x17, 0xFF};
    float smoothedDt = 1.0f / 60.0f;

    while (backend->pollFrame())
    {
        const float dt = backend->deltaTime();
        smoothedDt = smoothedDt * 0.9f + dt * 0.1f;
        const int fps = (smoothedDt > 0.0001f) ? static_cast<int>(std::lround(1.0f / smoothedDt)) : 0;

        ctx.runFrame(dt);

        backend->beginDraw(kClearColor);
        ctx.renderFrame();
        backend->endDraw();

        app.tickStats(fps, dt * 1000.0f);
    }

    backend->shutdown();
    return 0;
}