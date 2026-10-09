// ZenitUI — Example 05: ZStyle
//
// Demonstrates a stylesheet living in an external file. The theme.zstyle
// next to this source is loaded at startup; every rule the example uses
// is defined there.
//
// Concepts covered:
//   - custom properties (--name) declared in :root
//   - var() substitution
//   - calc() with mixed units
//   - @media queries reacting to the viewport
//   - state rules (:hover, :checked, :focus)
//   - ::part styling
//   - transitions declared per-property
//
// Resize the window to see the @media rule kick in (breakpoint: 700px).
//
// Build:   make examples
// Run:     make run-05_zstyle

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

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
    InitWindow(900, 620, "ZenitUI — 05 ZStyle");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    // -----------------------------------------------------------------------
    //  Load the external stylesheet. Path is relative to the current
    //  working directory, which must be the repository root.
    // -----------------------------------------------------------------------
    if (!ZMarkup::loadStyleFile("examples/05_zstyle/theme.zstyle")) {
        std::fprintf(stderr, "Failed to load theme.zstyle\n");
        return 1;
    }

    // -----------------------------------------------------------------------
    //  Tree. All styling comes from the stylesheet — no inline styles here.
    // -----------------------------------------------------------------------
    auto page = std::make_shared<Layout>(LayoutType::Vertical);
    page->setStyleTag("Page");

    auto panel = std::make_shared<Layout>(LayoutType::Vertical);
    panel->setStyleTag("Panel");

    auto title = Label("ZStyle demo");
    title->cls("title");
    panel->addChild(title);

    // 1. var() + calc() -----------------------------------------------------
    {
        auto box = std::make_shared<Layout>(LayoutType::Stack);
        box->cls("value-box");
        box->addChild(Label("width: calc(100% - 260px)"));
        panel->addChild(Row("calc()", box));
    }

    // 2. :checked + transition on knob -------------------------------------
    {
        auto t = Toggle::create(false);
        t->onToggle = [](bool v){ std::printf("toggle: %d\n", v); };
        panel->addChild(Row(":checked + transition", t));
    }

    // 3. ::part -------------------------------------------------------------
    {
        auto cb = Checkbox::create(true);
        panel->addChild(Row("::part", cb));
    }

    // 4. ::part + :hover transition ----------------------------------------
    {
        auto s = Slider::create(0.5f);
        panel->addChild(Row("::part + :hover", s));
    }

    // 5. :focus -------------------------------------------------------------
    {
        auto input = TextInput::create("click me");
        panel->addChild(Row(":focus", input));
    }

    page->addChild(panel);

    // -----------------------------------------------------------------------
    //  Main loop
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