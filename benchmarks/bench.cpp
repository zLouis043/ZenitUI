// ZenitUI — Benchmarks (Fase 4)
//
// Measures the performance impact of the SmallVector change in
// Unit.hpp, plus a baseline frame benchmark for future Phase 4 work.
//
// Build:   make bench
// Run:     ./benchmarks/bin/bench

#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

#include "Style.hpp"
#include "Theme.hpp"
#include "UI.hpp"
#include "Mocks.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

using clock_t_ = std::chrono::high_resolution_clock;

// ---------------------------------------------------------------------------
//  Reporting
// ---------------------------------------------------------------------------

static void report(const char* name,
                    int iterations,
                    clock_t_::time_point t0,
                    clock_t_::time_point t1)
{
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double us_per = (iterations > 0) ? (ms * 1000.0 / iterations) : 0.0;
    std::printf("%-40s  %8d iters  %8.3f ms  %10.4f us/iter\n",
                name, iterations, ms, us_per);
}

// Time the given function, running it `repeats` times and taking the
// minimum duration (best-of-N avoids spurious slow runs from the OS
// scheduler).
template <typename Fn>
static void best_of(const char* name, int iterations, int repeats, Fn fn)
{
    double best_ms = 1e30;
    for (int r = 0; r < repeats; ++r)
    {
        auto t0 = clock_t_::now();
        fn();
        auto t1 = clock_t_::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (ms < best_ms) best_ms = ms;
    }
    double us_per = (iterations > 0)
        ? (best_ms * 1000.0 / iterations) : 0.0;
    std::printf("%-40s  %8d iters  %8.3f ms  %10.4f us/iter\n",
                name, iterations, best_ms, us_per);
}

// ---------------------------------------------------------------------------
//  Benchmark 1: Value construction
// ---------------------------------------------------------------------------

static void bench_value_1term()
{
    constexpr int N = 1'000'000;
    std::vector<Value> v(N);

    best_of("Value x1 term (Px)", N, 3, [&] {
        for (int i = 0; i < N; ++i)
            v[i] = Value::px(100.0f);
    });
}

static void bench_value_2term()
{
    constexpr int N = 1'000'000;
    std::vector<Value> v(N);

    best_of("Value x2 terms (calc)", N, 3, [&] {
        for (int i = 0; i < N; ++i)
            v[i] = Value::percent(100.0f) - Value::px(20.0f);
    });
}

// ---------------------------------------------------------------------------
//  Benchmark 2: ComputedStyle
// ---------------------------------------------------------------------------

static void bench_computed_style()
{
    constexpr int N = 200'000;
    std::vector<ComputedStyle> v(N);

    best_of("ComputedStyle default + assign", N, 3, [&] {
        for (int i = 0; i < N; ++i)
        {
            ComputedStyle s;
            s.width    = Value::px(100.0f);
            s.height   = Value::px(50.0f);
            s.padding  = Spacing(Value::px(8.0f));
            s.margin   = Spacing(Value::px(4.0f));
            s.fontSize = Value::px(16.0f);
            v[i] = s;
        }
    });

    best_of("ComputedStyle copy", N, 3, [&] {
        for (int i = 1; i < N; ++i)
            v[i] = v[0];
    });
}

// ---------------------------------------------------------------------------
//  Benchmark 3: frame loop
// ---------------------------------------------------------------------------

static std::shared_ptr<Layout> make_tree(int nodes)
{
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .row {
            width: 100%;
            padding: 4px 8px;
            gap: 4px;
            transition: background 0.15s linear;
        }
        .row:hover { background: #4a9effff; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Percent(100), Percent(100));

    for (int i = 0; i < nodes; ++i)
    {
        auto row = std::make_shared<Layout>(LayoutType::Horizontal);
        row->cls("row");
        row->setInteractive(true);
        row->setBlocksRaycast(true);
        row->getInlineBase().background =
            (i % 2 == 0) ? Color{30, 30, 36, 255}
                         : Color{40, 40, 46, 255};

        auto label = Label("Item " + std::to_string(i));
        label->getInlineBase().width = Value::px(120.0f);

        auto spacer = std::make_shared<Layout>(LayoutType::Stack);
        spacer->getInlineBase().grow = 1;

        row->addChild(label);
        row->addChild(spacer);
        root->addChild(row);
    }
    return root;
}

static void bench_frame()
{
    constexpr int NODES  = 100;
    constexpr int FRAMES = 500;

    Env env;
    auto root = make_tree(NODES);

    // Warm-up: 10 frame per stabilizzare cache e layout.
    for (int i = 0; i < 10; ++i)
        env.frame(root);

    // Muovi il mouse per attivare le transizioni (l'animazione
    // coinvolge lo style resolver e la dirty propagation).
    env.platform.moveMouse({100.0f, 50.0f});

    best_of("frame loop 100 nodes", NODES * FRAMES, 3, [&] {
        for (int i = 0; i < FRAMES; ++i)
            env.frame(root);
    });
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    std::printf("\nZenitUI benchmarks — Phase 4.1\n");
    std::printf("==============================\n\n");

    std::printf("--- Value construction -----------------------------------------\n");
    bench_value_1term();
    bench_value_2term();

    std::printf("\n--- ComputedStyle ----------------------------------------------\n");
    bench_computed_style();

    std::printf("\n--- Frame loop -------------------------------------------------\n");
    bench_frame();

    std::printf("\n");
    return 0;
}