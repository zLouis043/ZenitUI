// ZenitUI — Example 10: Canvas
//
// CanvasLayout renders its subtree into an offscreen render target, then
// draws the target as a single texture. This is different from `filter:`,
// which also uses a target but processes it through the filter chain:
//
//   - `filter:` puts a node into a layer transparently, driven by CSS.
//   - `CanvasLayout` makes the "layer" an explicit node you can position,
//     size, and apply a single shader to via `setShader`.
//
// The example draws the same small scene twice — once plain, once with a
// hue-shift shader applied to the whole canvas — so the difference is
// visible at a glance.
//
// Build:   make examples
// Run:     make run-10_canvas

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ---------------------------------------------------------------------------
//  The scene that goes inside each canvas. Same tree for both, so any
//  difference between the two is purely the shader.
// ---------------------------------------------------------------------------

static std::shared_ptr<Layout> MakeScene()
{
    auto scene = std::make_shared<Layout>(LayoutType::Vertical);
    scene->cls("scene");

    // Three colour swatches.
    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->getInlineBase().gap = Px(6);

    auto red = std::make_shared<Layout>(LayoutType::Stack);
    red->cls("swatch");
    red->getInlineBase().background = ZenitUI::Color{220, 60, 60, 255};

    auto green = std::make_shared<Layout>(LayoutType::Stack);
    green->cls("swatch");
    green->getInlineBase().background = ZenitUI::Color{60, 200, 90, 255};

    auto blue = std::make_shared<Layout>(LayoutType::Stack);
    blue->cls("swatch");
    blue->getInlineBase().background = ZenitUI::Color{60, 120, 220, 255};

    row->addChild(red);
    row->addChild(green);
    row->addChild(blue);

    scene->addChild(row);

    auto label = Label("RGB");
    label->cls("scene-label");
    scene->addChild(label);

    return scene;
}

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<Layout> Row(const std::string& label,
                                   std::shared_ptr<Layout> content)
{
    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->cls("row");

    auto l = Label(label);
    l->cls("row-label");

    row->addChild(l);
    row->addChild(content);
    return row;
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(800, 480, "ZenitUI — 10 Canvas");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    // Load the shader used by the second canvas.
    assets.loadEffect("hueShift", "assets/hue_shift.fs");

    ZMarkup::loadStyleFile("examples/10_canvas/theme.zstyle");

    // -----------------------------------------------------------------------
    //  Root + page
    // -----------------------------------------------------------------------
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->setStyleTag("Stack");
    root->id("root");

    auto page = std::make_shared<Layout>(LayoutType::Vertical);
    page->cls("page");

    auto panel = std::make_shared<Layout>(LayoutType::Vertical);
    panel->cls("panel");

    auto title = Label("CanvasLayout: the same scene, twice");
    title->cls("title");
    panel->addChild(title);

    // -----------------------------------------------------------------------
    //  1. Plain canvas: no shader. Renders the scene exactly as it is.
    // -----------------------------------------------------------------------
    {
        auto canvas = CanvasLayout::create();
        canvas->size(Px(200), Px(140));
        canvas->addChild(MakeScene());

        panel->addChild(Row("1. Plain canvas", canvas));
    }

    // -----------------------------------------------------------------------
    //  2. Canvas with a hue-shift shader applied to the whole target.
    //     Unlike `effect:` on a normal node, the shader here receives the
    //     composited image (all three swatches at once), not one draw
    //     call per child.
    // -----------------------------------------------------------------------
    {
        auto canvas = CanvasLayout::create();
        canvas->size(Px(200), Px(140));
        canvas->addChild(MakeScene());

        // setShader() is the old API, kept for CanvasLayout: it takes an
        // EffectHandle directly instead of a CSS `effect:` name.
        canvas->setShader(assets.getEffect("hueShift"));

        panel->addChild(Row("2. hueShift applied", canvas));
    }

    page->addChild(panel);
    root->addChild(page);

    // -----------------------------------------------------------------------
    //  Main loop
    // -----------------------------------------------------------------------
    while (!WindowShouldClose())
    {
        root->runFrame(GetFrameTime());

        BeginDrawing();
        ClearBackground(::BLACK);
        root->renderFrame();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}