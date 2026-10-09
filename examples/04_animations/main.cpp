// ZenitUI — Example 04: Animations
//
// The three animation channels, all driven from .zstyle:
//
//   1. Transitions     — implicit, driven by state changes (:hover, :pressed, ...)
//   2. CSS keyframes   — explicit, driven by @keyframes + `animation:`
//   3. ::part animations — same as (2), but applied to a widget's ::part
//
// Build:   make examples
// Run:     make run-04_animations

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<Layout> Row(const std::string& title,
                                   std::shared_ptr<Layout> content)
{
    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->getInlineBase().gap        = Px(16);
    row->getInlineBase().itemsV     = Align::Center;
    row->getInlineBase().padding    = Spacing(Px(14));
    row->getInlineBase().background = ZenitUI::Color{32, 32, 38, 255};
    row->getInlineBase().radius     = Px(6);
    row->getInlineBase().width      = Percent(100);

    auto label = Label(title);
    label->size(Px(220), Auto());
    label->getInlineBase().color = ZenitUI::Color{180, 180, 190, 255};

    row->addChild(label);
    row->addChild(content);
    return row;
}

static std::shared_ptr<Layout> Swatch(const std::string& text,
                                      const std::string& cls)
{
    auto box = std::make_shared<Layout>(LayoutType::Stack);
    box->cls(cls);
    box->getInlineBase().minWidth = Px(140);
    box->getInlineBase().padding  = Spacing(Px(14), Px(20));
    box->getInlineBase().itemsH   = Align::Center;
    box->getInlineBase().itemsV   = Align::Center;
    box->addChild(Label(text));
    return box;
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(760, 480, "ZenitUI — 04 Animations");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    // -----------------------------------------------------------------------
    //  All animations live here. No C++ code drives them.
    // -----------------------------------------------------------------------
    ZMarkup::loadStyleString(R"(
        /* ---- 1. Transition on hover ---- */
        .hover-box {
            background: #4a9eff;
            color: #ffffff;
            radius: 6px;
            transition: background 0.25s ease-out-quad,
                        scale      0.15s ease-out-back;
        }
        .hover-box:hover   { background: #7bc0ff; scale: 1.08; }
        .hover-box:pressed { background: #2a7eef; scale: 0.94; }

        /* ---- 2. Keyframes: pulse ---- */
        @keyframes pulse {
            0%   { scale: 1.00; opacity: 1.0; }
            50%  { scale: 1.15; opacity: 0.7; }
            100% { scale: 1.00; opacity: 1.0; }
        }
        .pulse-box {
            background: #f4a72c;
            color: #ffffff;
            radius: 6px;
            animation: pulse 1.4s ease-in-out infinite;
        }

        /* ---- 2b. Keyframes: slide-in ---- */
        @keyframes slideIn {
            0%   { translate-x: -30vw; opacity: 0.0; }
            100% { translate-x: 0vw;   opacity: 1.0; }
        }
        .slide-box {
            background: #7bc043;
            color: #ffffff;
            radius: 6px;
            animation: slideIn 0.9s ease-out-back forwards;
        }

        /* ---- 3. Part animation: slider knob glow on hover ---- */
        @keyframes knobGlow {
            0%   { color: #ffffff; }
            50%  { color: #4a9eff; }
            100% { color: #ffffff; }
        }
        Slider {
            width: 220px;
            height: 24px;
        }
        Slider::track { background: #303030; radius: 4px; }
        Slider::fill  { color: #4a9eff; }
        Slider::knob  { color: #ffffff; }
        Slider:hover::knob {
            animation: knobGlow 0.7s ease-in-out infinite;
        }
    )");

    // -----------------------------------------------------------------------
    //  Root
    // -----------------------------------------------------------------------
    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().background = ZenitUI::Color{20, 20, 24, 255};
    root->getInlineBase().padding    = Spacing(Px(20));
    root->getInlineBase().gap        = Px(14);

    // --- 1. Transition -----------------------------------------------------
    {
        auto content = std::make_shared<Layout>(LayoutType::Stack);
        content->addChild(Swatch("hover me", "hover-box"));
        root->addChild(Row("1. Transition (hover)", content));
    }

    // --- 2. Pulse keyframes ------------------------------------------------
    {
        auto content = std::make_shared<Layout>(LayoutType::Stack);
        content->addChild(Swatch("pulsing", "pulse-box"));
        root->addChild(Row("2a. @keyframes pulse", content));
    }

    // --- 2b. Slide-in ------------------------------------------------------
    {
        auto content = std::make_shared<Layout>(LayoutType::Stack);
        content->addChild(Swatch("slide-in", "slide-box"));
        root->addChild(Row("2b. @keyframes slideIn", content));
    }

    // --- 3. Part animation -------------------------------------------------
    {
        auto slider = Slider::create(0.6f);
        auto content = std::make_shared<Layout>(LayoutType::Stack);
        content->addChild(slider);
        root->addChild(Row("3. ::part animation (hover)", content));
    }

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