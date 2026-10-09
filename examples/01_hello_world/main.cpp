// ZenitUI — Example 01: Hello World
//
// The smallest working ZenitUI program: a window, a root layout, and
// a clickable button.
//
// Build:   make examples         (from the repository root)
// Run:     make run-01_hello_world
//
// All styling is applied inline via getInlineBase(). See the later
// examples for .zstyle files, ZMarkup, and custom widgets.

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

int main()
{
    // -------------------------------------------------------------------
    //  Window + backend setup
    // -------------------------------------------------------------------
    InitWindow(640, 360, "ZenitUI — 01 Hello World");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    // -------------------------------------------------------------------
    //  Root layout: fills the viewport, centers its children
    // -------------------------------------------------------------------
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().background = ZenitUI::Color{24, 24, 28, 255};
    root->getInlineBase().itemsH     = Align::Center;
    root->getInlineBase().itemsV     = Align::Center;

    // -------------------------------------------------------------------
    //  The button
    // -------------------------------------------------------------------
    auto button = Btn("Click me", []{
        std::printf("clicked!\n");
    });

    button->getInlineBase().background = ZenitUI::Color{60, 120, 220, 255};
    button->getInlineBase().radius     = Px(8);
    button->getInlineBase().padding    = Spacing(VH(1.5f), VW(3.0f));

    root->addChild(button);

    // -------------------------------------------------------------------
    //  Main loop
    // -------------------------------------------------------------------
    while (!WindowShouldClose())
    {
        root->runFrame(GetFrameTime());    // logic

        BeginDrawing();
        ClearBackground(::BLACK);
        root->renderFrame();               // draw
        EndDrawing();
    }

    CloseWindow();
    return 0;
}