// ZenitUI — Real-backend rendering & frame benchmarks (Raylib / SDL3 / ...).
//
// Usage:
//   ./bench_raylib          human-readable output
//   ./bench_raylib --json   JSON output for github-action-benchmark

#include "bench_framework.hpp"
#include "Backend.hpp"
#include "ZenitUI.hpp"

#if defined(ZENITUI_BENCH_BACKEND_SDL3)
#include "SDL3Backend.hpp"
static constexpr const char *kBackendTag = "sdl3";
static std::unique_ptr<ZenitUI::IBackend> createBenchBackend()
{
    return std::make_unique<ZenitUI::SDL3Backend>();
}
#elif defined(ZENITUI_BENCH_BACKEND_RAYLIB)
#include "RaylibBackend.hpp"
static constexpr const char *kBackendTag = "raylib";
static std::unique_ptr<ZenitUI::IBackend> createBenchBackend()
{
    return std::make_unique<ZenitUI::RaylibBackend>();
}
#else
#error "Define ZENITUI_BENCH_BACKEND_RAYLIB or ZENITUI_BENCH_BACKEND_SDL3"
#endif

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

namespace Bench
{
    float g_sink = 0.0f;
}

namespace
{
    using namespace ZenitUI::UI;
    using ZenitUI::FilterDef;
    using ZenitUI::FilterParamType;
    using ZenitUI::FilterPattern;
    using ZenitUI::FilterRef;
    using ZenitUI::FilterRegistry;
    using ZenitUI::Layout;
    using ZenitUI::LayoutType;
    using ZenitUI::Percent;
    using ZenitUI::Px;
    using ZenitUI::Theme;
    using ZenitUI::UIContext;
    using ZenitUI::UI::VirtualView;

    constexpr ZenitUI::Color kClearColor{0x11, 0x12, 0x17, 0xFF};
    constexpr float kFixedDt = 1.0f / 60.0f;

    const char *kBenchStyle = R"zstyle(
        .root {
            width: 100%;
            height: 100%;
            padding: 16px;
            gap: 12px;
            background: #111217;
            font-family: inter;
            color: #e6e8f0;
        }
        .card {
            background: #1c1f2b;
            border: 1px solid #32384c;
            border-radius: 8px;
            padding: 10px 12px;
            gap: 6px;
        }
        .card-title {
            font-size: 16px;
            color: #ffffff;
        }
        .muted {
            font-size: 13px;
            color: #9ba3b8;
        }
        .grid-wrap {
            width: 100%;
            flex-wrap: wrap;
            gap: 8px;
        }
        .mini-card {
            width: 112px;
            height: 48px;
            background: #1e2333;
            border: 1px solid #3a425c;
            border-radius: 10px;
            padding: 6px 8px;
            justify-content: center;
        }
        .vrow {
            width: 100%;
            height: 40px;
            padding: 0 12px;
            gap: 12px;
            align-items: center;
            background: #181b25;
            border-bottom: 1px solid #262b3b;
        }
        .vrow.alt {
            background: #1d212e;
        }
        .fx-card {
            width: 160px;
            height: 90px;
            background: #2b3554;
            border: 1px solid #5a72b8;
            border-radius: 10px;
            padding: 10px;
            align-items: center;
            justify-content: center;
        }
    )zstyle";

    std::shared_ptr<Layout> buildDashboardTree()
    {
        auto root = VStack()->cls("root");

        auto kpiRow = HStack();
        kpiRow->getInlineBase().width = Percent(100);
        kpiRow->getInlineBase().gap = Px(12);
        for (int i = 0; i < 4; ++i)
        {
            auto card = VStack()->cls("card");
            card->getInlineBase().grow = 1.0f;
            card->addChild(Label("KPI Metric #" + std::to_string(i + 1))->cls("card-title"));
            card->addChild(Label("Status: nominal · UTF-8 café → OK")->cls("muted"));
            kpiRow->addChild(card);
        }
        root->addChild(kpiRow);

        for (int c = 0; c < 6; ++c)
        {
            auto section = VStack()->cls("card");
            section->getInlineBase().width = Percent(100);
            section->addChild(Label("Panel " + std::to_string(c + 1))->cls("card-title"));

            auto wrapText = Label(
                "Retained-mode layout, ZStyle cascade, rounded border-box strokes, "
                "and wrapped UTF-8 text benchmark paragraph.");
            wrapText->cls("muted");
            wrapText->setWrap(true);
            section->addChild(wrapText);

            auto row = HStack();
            row->getInlineBase().gap = Px(8);
            for (int b = 0; b < 5; ++b)
                row->addChild(Btn("Action " + std::to_string(b + 1), nullptr));
            section->addChild(row);

            root->addChild(section);
        }
        return root;
    }

    std::shared_ptr<Layout> buildRoundedCardsTree(int count)
    {
        auto root = HStack()->cls("root");
        root->addClass("grid-wrap");
        for (int i = 0; i < count; ++i)
        {
            auto card = VStack()->cls("mini-card");
            card->addChild(Label("Card #" + std::to_string(i + 1))->cls("muted"));
            root->addChild(card);
        }
        return root;
    }

    std::shared_ptr<Layout> buildVirtualScrollTree(std::shared_ptr<VirtualView> &outList)
    {
        auto root = VStack()->cls("root");
        outList = VirtualView::create(
            [](int idx) -> std::shared_ptr<Layout>
            {
                auto row = HStack()->cls("vrow");
                if (idx % 2 == 1)
                    row->addClass("alt");
                row->addChild(Label("#" + std::to_string(idx + 1))->cls("muted"));
                row->addChild(Label("VirtualView catalog entry number " + std::to_string(idx + 1)));
                return row;
            },
            10000);
        outList->size(Percent(100), Percent(100));
        outList->setUniformHeight(40.0f);
        root->addChild(outList);
        return root;
    }

    std::shared_ptr<Layout> buildFilteredCardsTree(int count)
    {
        auto root = HStack()->cls("root");
        root->addClass("grid-wrap");

        const std::vector<FilterRef> chain = {
            {"blur", {"4px"}},
            {"drop-shadow", {"2px", "2px", "#000000aa"}},
        };

        for (int i = 0; i < count; ++i)
        {
            auto card = VStack()->cls("fx-card");
            card->getInlineBase().filters = chain;
            card->addChild(Label("Filter #" + std::to_string(i + 1))->cls("card-title"));
            root->addChild(card);
        }
        return root;
    }

    void renderOneFrame(ZenitUI::IBackend &backend, Layout &root, bool runLogic)
    {
        backend.pollFrame();
        if (runLogic)
            root.runFrame(kFixedDt);

        backend.beginDraw(kClearColor);
        root.renderFrame();
        backend.endDraw();
        Bench::g_sink += root.getRect().width;
    }
} // namespace

int main(int argc, char **argv)
{
    bool json = false;
    std::string outPath;
    int repeats = 5;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--json") == 0)
            json = true;
        else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc)
            outPath = argv[++i];
        else if (std::strcmp(argv[i], "--repeats") == 0 && i + 1 < argc)
            repeats = std::max(1, std::atoi(argv[++i]));
    }

    auto backend = createBenchBackend();

    ZenitUI::WindowConfig cfg;
    cfg.title = std::string("ZenitUI Bench (") + kBackendTag + ")";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.targetFps = 0;
    cfg.resizable = false;
    cfg.vsync = false;
    cfg.msaa = false;

    if (!backend->init(cfg))
    {
        std::fprintf(stderr, "[bench_%s] Failed to initialize backend\n", kBackendTag);
        return 1;
    }

    // Load shared real assets (same paths as 00_demo, run from repo root).
    auto &assets = backend->assets();
    assets.setDefaultFontSize(48);
    assets.addDefaultCodepointRange(0x2000, 0x206F);
    assets.addDefaultCodepointRange(0x2190, 0x21FF);
    assets.loadFont("inter", "assets/fonts/Inter_18pt-Regular.ttf");
    assets.loadEffect("blur", "assets/shaders/blur.fs");
    assets.loadEffect("dropShadow", "assets/shaders/drop_shadow.fs");
    assets.loadEffect("sepia", "assets/shaders/sepia.fs");

    FilterRegistry::get().add(FilterDef{
        .name = "sepia",
        .shader = "sepia",
        .pattern = FilterPattern::SinglePass,
        .params = {{"amount", FilterParamType::Float, 0, 0.85f}},
    });

    const std::string prefix = std::string(kBackendTag) + ".";

    {
        std::shared_ptr<Layout> activeRoot;
        std::shared_ptr<VirtualView> virtualList;
        int scrollStep = 0;

        auto resetScene = [&]
        {
            virtualList.reset();
            activeRoot.reset();
            auto &ctx = UIContext::get();
            ctx.targetPool.clear(backend->renderer(), &ctx.renderStats);
            Theme::get().clear();
            ZenitUI::ZMarkup::loadStyleString(kBenchStyle);
        };

        Bench::Runner runner(repeats);

        // 1. Full frame (update + layout + render + swap) on static dashboard.
        runner.add(
            prefix + "frame.static_dashboard", "backend_frame", /*iterations=*/30,
            /*setup=*/[&]
            {
                resetScene();
                activeRoot = buildDashboardTree();
                for (int i = 0; i < 3; ++i)
                    renderOneFrame(*backend, *activeRoot, true);
            },
            /*body=*/[&]
            { renderOneFrame(*backend, *activeRoot, true); });

        // 2. Render-only pass on static dashboard (isolates renderer/GL from update & layout).
        runner.add(
            prefix + "render.static_dashboard", "backend_render", /*iterations=*/30,
            /*setup=*/[&]
            {
                resetScene();
                activeRoot = buildDashboardTree();
                for (int i = 0; i < 3; ++i)
                    renderOneFrame(*backend, *activeRoot, true);
            },
            /*body=*/[&]
            { renderOneFrame(*backend, *activeRoot, false); });

        // 3. 200 rounded cards with border + label (tessellation & batching stress).
        runner.add(
            prefix + "frame.rounded_cards_200", "backend_geometry", /*iterations=*/25,
            /*setup=*/[&]
            {
                resetScene();
                activeRoot = buildRoundedCardsTree(200);
                for (int i = 0; i < 3; ++i)
                    renderOneFrame(*backend, *activeRoot, true);
            },
            /*body=*/[&]
            { renderOneFrame(*backend, *activeRoot, true); });

        // 4. 10,000-row VirtualView with continuous scrolling.
        runner.add(
            prefix + "frame.virtual_scroll_10k", "backend_scroll", /*iterations=*/30,
            /*setup=*/[&]
            {
                resetScene();
                scrollStep = 0;
                activeRoot = buildVirtualScrollTree(virtualList);
                for (int i = 0; i < 3; ++i)
                    renderOneFrame(*backend, *activeRoot, true);
            },
            /*body=*/[&]
            {
                scrollStep = (scrollStep + 7) % 9000;
                if (virtualList)
                    virtualList->scrollToItem(scrollStep);
                renderOneFrame(*backend, *activeRoot, true);
            });

        // 5. 10 cards with multi-pass filters (blur + drop-shadow FBO stress).
        runner.add(
            prefix + "frame.filters_10", "backend_effects", /*iterations=*/15,
            /*setup=*/[&]
            {
                resetScene();
                activeRoot = buildFilteredCardsTree(10);
                for (int i = 0; i < 3; ++i)
                    renderOneFrame(*backend, *activeRoot, true);
            },
            /*body=*/[&]
            { renderOneFrame(*backend, *activeRoot, true); });

        if (!json)
        {
            std::printf("\nZenitUI real-backend benchmarks (%s)\n"
                        "====================================\n\n",
                        backend->name().data());
        }

        runner.runAll(json ? std::cerr : std::cout);

        if (!outPath.empty())
        {
            std::ofstream ofs(outPath);
            if (!ofs)
            {
                std::fprintf(stderr, "[bench_%s] Failed to open '%s' for writing\n",
                             kBackendTag, outPath.c_str());
                return 1;
            }
            runner.printJson(ofs);
        }
        else if (json)
        {
            runner.printJson(std::cout);
        }
        else
        {
            runner.printHuman(std::cout);
            std::printf("\n(sink: %g)\n", Bench::g_sink);
        }

        virtualList.reset();
        activeRoot.reset();
    }

    backend->shutdown();
    return 0;
}