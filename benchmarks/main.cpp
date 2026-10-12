// ZenitUI — Benchmarks entry point.
//
// Usage:
//   ./bench                 human-readable output
//   ./bench --json          JSON output for CI
//   ./bench --update        write benchmarks/baseline.json
//   ./bench --check         run and compare with baseline (default)

#include "bench_framework.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace Bench { void registerAll(Runner&); }
namespace Bench { extern float g_sink; }

// ---------------------------------------------------------------------------
//  Baseline file format (simple, line-based):
//
//     <name> <median_us>
//
//  One entry per line. Comments start with '#'.
// ---------------------------------------------------------------------------

static const char* kBaselinePath = "baseline.json";

using Baseline = std::map<std::string, double>;

static Baseline loadBaseline(const std::string& path)
{
    Baseline b;
    std::ifstream f(path);
    if (!f) return b;

    std::string line;
    while (std::getline(f, line))
    {
        // Skip blanks and comments.
        size_t i = 0;
        while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
        if (i >= line.size() || line[i] == '#') continue;

        std::istringstream iss(line.substr(i));
        std::string name;
        double value = 0.0;
        if (iss >> name >> value)
            b[name] = value;
    }
    return b;
}

static void saveBaseline(const std::string& path,
                         const std::vector<Bench::Result>& results)
{
    std::ofstream f(path);
    f << "# ZenitUI benchmark baseline.\n";
    f << "# Format: <name> <median_us>. Lower is better.\n";
    f << "# Regenerate with: ./benchmarks/bin/bench --update\n";
    f << "#\n";
    f << "# A change of more than 20% (either direction) is flagged by\n";
    f << "# `bench --check`.\n\n";

    for (const auto& r : results)
        f << r.name << " " << r.median_us << "\n";
}

static void printComparison(const std::vector<Bench::Result>& results,
                            const Baseline& baseline)
{
    if (baseline.empty())
    {
        std::printf("\n--- Baseline comparison ---\n");
        std::printf("No baseline found. Run with --update to create one.\n");
        return;
    }

    std::printf("\n--- Baseline comparison ---\n");
    std::printf("  %-42s  %10s  %10s  %8s\n",
                "name", "baseline", "current", "delta");
    std::printf("  %-42s  %10s  %10s  %8s\n",
                "", "(us)", "(us)", "");

    int regressions = 0;
    int improvements = 0;

    for (const auto& r : results)
    {
        auto it = baseline.find(r.name);
        if (it == baseline.end())
        {
            std::printf("  %-42s  %10s  %10.4f  %8s\n",
                        r.name.c_str(), "NEW", r.median_us, "");
            continue;
        }

        double base = it->second;
        double delta_pct = (base > 0.0)
            ? 100.0 * (r.median_us - base) / base
            : 0.0;

        const char* flag = "";
        if (delta_pct > 20.0) { flag = " ! REG"; regressions++; }
        else if (delta_pct < -20.0) { flag = " + IMP"; improvements++; }

        std::printf("  %-42s  %10.4f  %10.4f  %+7.1f%%%s\n",
                    r.name.c_str(), base, r.median_us, delta_pct, flag);
    }

    std::printf("\n  %d regression(s), %d improvement(s).\n",
                regressions, improvements);
}

int main(int argc, char** argv)
{
    bool json   = false;
    bool update = false;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--json") == 0)   json = true;
        if (std::strcmp(argv[i], "--update") == 0) update = true;
    }

    Bench::Runner runner(/*repeats=*/5);
    Bench::registerAll(runner);

    if (!json)
        std::printf("\nZenitUI benchmarks\n"
                    "==================\n\n");

    runner.runAll(json ? std::cerr : std::cout);

    if (json)
    {
        runner.printJson(std::cout);
        return 0;
    }

    runner.printHuman(std::cout);

    if (update)
    {
        saveBaseline(kBaselinePath, runner.results());
        std::printf("\nBaseline written to %s\n", kBaselinePath);
        return 0;
    }

    Baseline baseline = loadBaseline(kBaselinePath);
    printComparison(runner.results(), baseline);
    std::printf("\n(sink: %g)\n", Bench::g_sink);

    return 0;
}