// @doc   GPU stress benchmark for 50 filtered cards testing TargetPool reuse, downscaled separable blur, and live RenderStats HUD.
// @group grp_anim grp_backends

#include "ZenitUI.hpp"
#include "RaylibBackend.hpp"
#include <raylib.h>

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace ZenitUI;
using namespace ZenitUI::UI;

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 760, "ZenitUI - 07b Effects Stress Bench");
    // Uncapped FPS to measure actual GPU frame time
    SetTargetFPS(0);

    RaylibRenderer renderer;
    RaylibPlatform platform;
    RaylibAssetProvider assets(renderer);

    UIContext ctx;
    ctx.attachBackend(&renderer, &platform, &assets);

    ctx.textEngine.loadFont("default", "assets/fonts/Inter_18pt-Regular.ttf");

    // Load fragment shaders used by the built-in and custom filters
    assets.loadEffect("blur", "assets/shaders/blur.fs");
    assets.loadEffect("dropShadow", "assets/shaders/drop_shadow.fs");

    ctx.filters.add(FilterDef{
        .name = "blur-fast",
        .shader = "blur",
        .pattern = FilterPattern::Separable,
        .params = {FilterParam{"radius", FilterParamType::Float, 0, 4.0f}},
        .custom = {},
        .downscale = 2});

    ZMarkup::loadStyleString(R"(
        .root {
            width: 100%;
            height: 100%;
            background: #121216;
            padding: 16px;
            gap: 12px;
        }
        .toolbar {
            width: 100%;
            gap: 8px;
            align-items: center;
            wrap: true;
        }
        .hud {
            width: 100%;
            background: #1b1b22;
            border-width: 1px;
            border-color: #333340;
            radius: 6px;
            padding: 8px 12px;
            color: #c4b5fd;
            font-size: 14px;
        }
        .btn {
            background: #23232e;
            border-width: 1px;
            border-color: #3f3f52;
            radius: 6px;
            padding: 6px 12px;
            color: #e5e7eb;
            font-size: 13px;
        }
        .btn:hover { background: #2f2f3f; }
        .btn.active {
            background: #5b21b6;
            border-color: #8b5cf6;
            color: #ffffff;
        }
        .grid {
            width: 100%;
            flex: 1;
            min-height: 0;
            overflow-y: auto;
            gap: 12px;
            wrap: true;
            align-items: start;
        }
        .card {
            width: 180px;
            height: 96px;
            background: #1e1e28;
            border-width: 1px;
            border-color: #4c1d95;
            radius: 8px;
            padding: 10px 12px;
            gap: 6px;
        }
        .card-title { color: #f3f4f6; font-size: 15px; }
        .card-sub   { color: #9ca3af; font-size: 12px; }
    )", ctx.theme);

    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->cls("root");

    auto hud = Text::create("HUD");
    hud->cls("hud");
    root->addChild(hud);

    auto toolbar = std::make_shared<Layout>(LayoutType::Horizontal);
    toolbar->cls("toolbar");
    root->addChild(toolbar);

    auto grid = std::make_shared<Layout>(LayoutType::Horizontal);
    grid->cls("grid");
    root->addChild(grid);

    constexpr int CARD_COUNT = 50;
    std::vector<std::shared_ptr<Layout>> cards;
    cards.reserve(CARD_COUNT);

    for (int i = 0; i < CARD_COUNT; ++i)
    {
        auto c = std::make_shared<Layout>(LayoutType::Vertical);
        c->cls("card");
        auto t = Text::create("Card #" + std::to_string(i + 1));
        t->cls("card-title");
        auto s = Text::create("Filter stress test item");
        s->cls("card-sub");
        c->addChild(t);
        c->addChild(s);
        grid->addChild(c);
        cards.push_back(c);
    }

    int filterMode = 0; // 0: blur+shadow, 1: blur 1x, 2: blur 2x, 3: none
    int animMode = 0;   // 0: static, 1: scale, 2: rotate

    auto applyFilterMode = [&]()
    {
        std::vector<FilterRef> chain;
        if (filterMode == 0)
            chain = {{"blur", {"4px"}}, {"drop-shadow", {"2px", "2px", "#000000cc"}}};
        else if (filterMode == 1)
            chain = {{"blur", {"4px"}}};
        else if (filterMode == 2)
            chain = {{"blur-fast", {"4px"}}};

        for (auto &c : cards)
            c->getInlineBase().filters = chain;
    };

    auto makeBtn = [&](const std::string &label, std::function<void()> cb)
    {
        auto b = Button::create(std::move(cb));
        b->cls("btn");
        b->addChild(Text::create(label));
        toolbar->addChild(b);
        return b;
    };

    std::vector<std::shared_ptr<Button>> filterBtns;
    std::vector<std::shared_ptr<Button>> animBtns;

    auto refreshBtns = [&]()
    {
        for (size_t i = 0; i < filterBtns.size(); ++i)
        {
            if ((int)i == filterMode) filterBtns[i]->addClass("active");
            else filterBtns[i]->removeClass("active");
        }
        for (size_t i = 0; i < animBtns.size(); ++i)
        {
            if ((int)i == animMode) animBtns[i]->addClass("active");
            else animBtns[i]->removeClass("active");
        }
    };

    filterBtns.push_back(makeBtn("Blur + Shadow", [&] { filterMode = 0; applyFilterMode(); refreshBtns(); }));
    filterBtns.push_back(makeBtn("Blur 1x",       [&] { filterMode = 1; applyFilterMode(); refreshBtns(); }));
    filterBtns.push_back(makeBtn("Blur Fast 2x",  [&] { filterMode = 2; applyFilterMode(); refreshBtns(); }));
    filterBtns.push_back(makeBtn("No Filter",     [&] { filterMode = 3; applyFilterMode(); refreshBtns(); }));

    animBtns.push_back(makeBtn("Static", [&] { animMode = 0; refreshBtns(); }));
    animBtns.push_back(makeBtn("Scale",  [&] { animMode = 1; refreshBtns(); }));
    animBtns.push_back(makeBtn("Rotate", [&] { animMode = 2; refreshBtns(); }));

    applyFilterMode();
    refreshBtns();
    ctx.mount(root);

    float elapsed = 0.0f;

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        elapsed += dt;

        for (auto &c : cards)
        {
            if (animMode == 1)
            {
                c->getInlineBase().scale = 1.0f + 0.08f * std::sin(elapsed * 3.0f);
                c->getInlineBase().rotation = 0.0f;
            }
            else if (animMode == 2)
            {
                c->getInlineBase().scale = 1.0f;
                c->getInlineBase().rotation = std::fmod(elapsed * 45.0f, 360.0f);
            }
            else
            {
                c->getInlineBase().scale = 1.0f;
                c->getInlineBase().rotation = 0.0f;
            }
        }

        ctx.runFrame(*root, dt);

        BeginDrawing();
        ClearBackground({18, 18, 22, 255});
        ctx.renderFrame();
        EndDrawing();

        const auto &st = ctx.renderStats;
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "50 cards | FPS: %d (%.2f ms) | targets[created=%u destroyed=%u live=%u] | pushTarget=%u | passes=%u",
                      GetFPS(), dt * 1000.0f,
                      st.targetsCreated, st.targetsDestroyed, st.liveTargets,
                      st.pushTargetCalls, st.filterPasses);
        hud->setText(buf);
    }

    ctx.detachBackend();
    CloseWindow();
    return 0;
}