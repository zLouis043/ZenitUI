// ZenitUI — Example 04b: Imperative Animations
//
// UIAnimation gives you programmatic, event-driven animations: played by
// name, reversible, with custom math and finish callbacks.
//
// Use imperative animations when you need to:
//   - start an animation from a click or any runtime event
//   - control the direction (forward / reverse) explicitly
//   - compute the interpolated value with custom math (shake, spring, ...)
//   - react when the animation finishes (chain, cleanup, ...)
//
// Use CSS animations (example 04) when the animation is tied to a style
// state: hover, checked, focus, or a class the user can toggle.
//
// Build:   make examples
// Run:     make run-04b_imperative_animations

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cmath>
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
    row->getInlineBase().gap        = Px(16);
    row->getInlineBase().itemsV     = Align::Center;
    row->getInlineBase().padding    = Spacing(Px(14));
    row->getInlineBase().background = ZenitUI::Color{32, 32, 38, 255};
    row->getInlineBase().radius     = Px(6);
    row->getInlineBase().width      = Percent(100);

    auto label = Label(title);
    label->size(Px(200), Auto());
    label->getInlineBase().color = ZenitUI::Color{180, 180, 190, 255};

    row->addChild(label);
    row->addChild(content);
    return row;
}

static std::shared_ptr<Button> ActionBtn(const std::string& text,
                                         std::function<void()> cb)
{
    auto b = Btn(text, std::move(cb));
    b->getInlineBase().background = ZenitUI::Color{74, 158, 255, 255};
    b->getInlineBase().color      = Colors::White;
    b->getInlineBase().radius     = Px(6);
    b->getInlineBase().padding    = Spacing(Px(8), Px(14));
    return b;
}

static std::shared_ptr<Layout> Card(ZenitUI::Color bg)
{
    auto card = std::make_shared<Layout>(LayoutType::Stack);
    card->getInlineBase().padding    = Spacing(Px(16), Px(24));
    card->getInlineBase().background = bg;
    card->getInlineBase().radius     = Px(8);
    card->getInlineBase().minWidth   = Px(140);
    card->getInlineBase().itemsH     = Align::Center;
    card->getInlineBase().itemsV     = Align::Center;
    return card;
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(800, 520, "ZenitUI — 04b Imperative Animations");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().background = ZenitUI::Color{20, 20, 24, 255};
    root->getInlineBase().padding    = Spacing(Px(20));
    root->getInlineBase().gap        = Px(14);

    // =======================================================================
    //  1. Fade + slide, playable forward and reverse
    // =======================================================================
    {
        auto card = Card(ZenitUI::Color{60, 120, 220, 255});
        auto lbl = Label("Hello");
        lbl->getInlineBase().color = Colors::White;
        card->addChild(lbl);

        // Initial state: invisible and pushed down.
        card->getInlineBase().opacity    = 0.0f;
        card->getInlineBase().translateY = Px(40.0f);

        // Two tracks: opacity 0→1, translateY 40→0.
        auto anim = std::make_shared<UIAnimation>(0.5f);
        anim->addTrack<float>(0.0f, 1.0f,
            [](Layout* l, float v) { l->getInlineBase().opacity = v; },
            TransitionFunction::EaseOutQuad);
        anim->addTrack<Value>(Px(40.0f), Px(0.0f),
            [](Layout* l, Value v) { l->getInlineBase().translateY = v; },
            TransitionFunction::EaseOutBack);
        anim->onFinished = []{ std::printf("[1] appear finished\n"); };

        card->addAnimation("appear", anim);

        auto playBtn = ActionBtn("Play", [card]{
            // playAnimation resumes if the animation is mid-flight, and
            // restarts from 0 if it already completed forward.
            card->playAnimation("appear", false);
        });
        auto revBtn = ActionBtn("Reverse", [card]{
            card->playAnimation("appear", true);
        });

        auto buttons = std::make_shared<Layout>(LayoutType::Horizontal);
        buttons->getInlineBase().gap = Px(8);
        buttons->addChild(playBtn);
        buttons->addChild(revBtn);

        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap    = Px(16);
        content->getInlineBase().itemsV = Align::Center;
        content->addChild(card);
        content->addChild(buttons);

        root->addChild(Row("1. play / reverse", content));
    }

    // =======================================================================
    //  2. Custom math: a damped shake
    // =======================================================================
    {
        auto card = Card(ZenitUI::Color{240, 167, 44, 255});
        auto lbl = Label("Shake me");
        lbl->getInlineBase().color = Colors::White;
        card->addChild(lbl);

        // The track receives t ∈ [0, 1] and computes the offset itself.
        // The framework does no interpolation here: whatever the setter
        // writes is what the node uses.
        auto anim = std::make_shared<UIAnimation>(0.5f);
        anim->addTrack<float>(0.0f, 1.0f,
            [](Layout* l, float t) {
                float amp   = (1.0f - t) * 12.0f;   // amplitude decays
                float angle = t * 30.0f;            // fast oscillation
                l->getInlineBase().translateX = Px(std::sin(angle) * amp);
            },
            TransitionFunction::Linear);
        anim->onFinished = []{ std::printf("[2] shake finished\n"); };

        card->addAnimation("shake", anim);

        auto trigger = ActionBtn("Shake", [card]{
            card->playAnimation("shake");
        });

        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap    = Px(16);
        content->getInlineBase().itemsV = Align::Center;
        content->addChild(card);
        content->addChild(trigger);

        root->addChild(Row("2. custom math", content));
    }

    // =======================================================================
    //  3. Chained animations via onFinished
    // =======================================================================
    {
        auto card = Card(ZenitUI::Color{123, 192, 67, 255});
        auto lbl = Label("Chain");
        lbl->getInlineBase().color = Colors::White;
        card->addChild(lbl);

        // Phase 1: fade in + slide right.
        auto phase1 = std::make_shared<UIAnimation>(0.35f);
        phase1->addTrack<float>(0.0f, 1.0f,
            [](Layout* l, float v) { l->getInlineBase().opacity = v; });
        phase1->addTrack<Value>(Px(0.0f), Px(60.0f),
            [](Layout* l, Value v) { l->getInlineBase().translateX = v; },
            TransitionFunction::EaseOutQuad);

        // Phase 2: slide back + scale bump.
        auto phase2 = std::make_shared<UIAnimation>(0.35f);
        phase2->addTrack<Value>(Px(60.0f), Px(0.0f),
            [](Layout* l, Value v) { l->getInlineBase().translateX = v; },
            TransitionFunction::EaseOutBack);
        phase2->addTrack<float>(1.0f, 1.15f,
            [](Layout* l, float v) { l->getInlineBase().scale = v; },
            TransitionFunction::EaseOutQuad);

        // Weak references to avoid a reference cycle: the node owns the
        // animations, so a lambda that captured it strongly would create
        // a cycle node → animation → lambda → node.
        std::weak_ptr<Layout> weak = card;

        phase1->onFinished = [weak]{
            if (auto b = weak.lock()) b->playAnimation("phase2");
        };
        phase2->onFinished = [weak]{
            std::printf("[3] chain finished\n");
            if (auto b = weak.lock()) b->getInlineBase().scale = 1.0f;
        };

        card->addAnimation("phase1", phase1);
        card->addAnimation("phase2", phase2);

        auto trigger = ActionBtn("Run chain", [card]{
            // Reset to the initial state so the chain can be replayed.
            card->getInlineBase().opacity    = 0.0f;
            card->getInlineBase().translateX = Px(0.0f);
            card->getInlineBase().scale      = 1.0f;
            card->playAnimation("phase1");
        });

        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap    = Px(16);
        content->getInlineBase().itemsV = Align::Center;
        content->addChild(card);
        content->addChild(trigger);

        root->addChild(Row("3. chained via onFinished", content));
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