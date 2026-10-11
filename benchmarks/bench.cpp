// ZenitUI — Benchmarks (Fase 4)
//
// Usage:
//   ./bench                 human-readable output
//   ./bench --json          JSON output for CI (github-action-benchmark)

#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
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
//  Results
// ---------------------------------------------------------------------------

struct BenchResult
{
    std::string name;
    std::string unit;      // "us" (microseconds) for all our benchmarks
    double      value;     // per-iteration value, in `unit`
};

static std::vector<BenchResult>& results()
{
    static std::vector<BenchResult> r;
    return r;
}

static void record(const std::string& name, double us_per_iter)
{
    results().push_back({ name, "us", us_per_iter });
}

// ---------------------------------------------------------------------------
//  Timing helper: best of N runs
// ---------------------------------------------------------------------------

template <typename Fn>
static void best_of(const std::string& name,
                     int iterations,
                     int repeats,
                     bool quiet,
                     Fn fn)
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

    if (!quiet)
        std::printf("%-40s  %8d iters  %8.3f ms  %10.4f us/iter\n",
                    name.c_str(), iterations, best_ms, us_per);

    record(name, us_per);
}

// ---------------------------------------------------------------------------
//  Benchmark 1: Value construction
// ---------------------------------------------------------------------------

static void bench_value_1term(bool quiet)
{
    constexpr int N = 1'000'000;
    std::vector<Value> v(N);

    best_of("Value x1 term (Px)", N, 3, quiet, [&] {
        for (int i = 0; i < N; ++i)
            v[i] = Value::px(100.0f);
    });
}

static void bench_value_2term(bool quiet)
{
    constexpr int N = 1'000'000;
    std::vector<Value> v(N);

    best_of("Value x2 terms (calc)", N, 3, quiet, [&] {
        for (int i = 0; i < N; ++i)
            v[i] = Value::percent(100.0f) - Value::px(20.0f);
    });
}

// ---------------------------------------------------------------------------
//  Benchmark 2: ComputedStyle
// ---------------------------------------------------------------------------

static void bench_computed_style(bool quiet)
{
    constexpr int N = 200'000;
    std::vector<ComputedStyle> v(N);

    best_of("ComputedStyle default + assign", N, 3, quiet, [&] {
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

    best_of("ComputedStyle copy", N, 3, quiet, [&] {
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

static void bench_frame(bool quiet)
{
    constexpr int NODES  = 100;
    constexpr int FRAMES = 500;

    Env env;
    auto root = make_tree(NODES);

    for (int i = 0; i < 10; ++i)
        env.frame(root);

    env.platform.moveMouse({100.0f, 50.0f});

    best_of("frame loop 100 nodes", NODES * FRAMES, 3, quiet, [&] {
        for (int i = 0; i < FRAMES; ++i)
            env.frame(root);
    });
}

// ---------------------------------------------------------------------------
//  JSON output
// ---------------------------------------------------------------------------

static void print_json()
{
    // Format accepted by github-action-benchmark `customSmallerIsBetter`:
    // an array of {name, unit, value}.
    std::printf("[\n");
    const auto& r = results();
    for (size_t i = 0; i < r.size(); ++i)
    {
        std::printf(
            "  {\"name\": \"%s\", \"unit\": \"%s\", \"value\": %.4f}%s\n",
            r[i].name.c_str(),
            r[i].unit.c_str(),
            r[i].value,
            (i + 1 < r.size()) ? "," : "");
    }
    std::printf("]\n");
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv)
{
    bool json = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--json") == 0)
            json = true;
    }

    const bool quiet = json;

    if (!json)
    {
        std::printf("\nZenitUI benchmarks — Phase 4.1\n");
        std::printf("==============================\n\n");

        std::printf("--- Value construction -----------------------------------------\n");
    }
    bench_value_1term(quiet);
    bench_value_2term(quiet);

    if (!json)
        std::printf("\n--- ComputedStyle ----------------------------------------------\n");
    bench_computed_style(quiet);

    if (!json)
        std::printf("\n--- Frame loop -------------------------------------------------\n");
    bench_frame(quiet);

    if (json)
        print_json();
    else
        std::printf("\n");

    return 0;
}