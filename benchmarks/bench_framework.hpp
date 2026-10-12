#pragma once

// Small benchmark framework for ZenitUI.
//
// Design:
//   - Each benchmark is a named entry with setup + body callbacks.
//   - setup() is called before every run, so each run starts from a
//     known state. This eliminates contamination between benchmarks.
//   - The reported value is the MEDIAN of N runs (default 5). Median is
//     robust to scheduler noise on shared CI runners, unlike best-of-N
//     which hides regressions, and unlike mean which is skewed by
//     outliers.
//   - best / worst / stddev are also reported for diagnostics.
//   - Output: human-readable and JSON (for github-action-benchmark).

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

namespace Bench {

using Clock = std::chrono::high_resolution_clock;

struct Result {
    std::string name;
    std::string category;
    int         iterations{0};
    int         samples{0};
    double      median_us{0.0};
    double      best_us{0.0};
    double      worst_us{0.0};
    double      stddev_us{0.0};
};

class Runner {
public:
    explicit Runner(int repeats = 5) : repeats_(repeats) {}

    void add(const std::string& name,
             const std::string& category,
             int iterations,
             std::function<void()> setup,
             std::function<void()> body)
    {
        entries_.push_back({name, category, iterations,
                            std::move(setup), std::move(body)});
    }

    void runAll(std::ostream& log);

    const std::vector<Result>& results() const { return results_; }

    void printHuman(std::ostream& os) const;
    void printJson(std::ostream& os) const;

private:
    int repeats_;
    struct Entry {
        std::string name;
        std::string category;
        int iterations;
        std::function<void()> setup;
        std::function<void()> body;
    };
    std::vector<Entry>  entries_;
    std::vector<Result> results_;
};

extern float g_sink;

inline void Runner::runAll(std::ostream& log)
{
    results_.clear();
    results_.reserve(entries_.size());

    for (const auto& e : entries_)
    {
        log << "  running " << e.name << " ... " << std::flush;

        // Warm-up: one run, not counted.
        if (e.setup) e.setup();
        for (int i = 0; i < e.iterations; ++i)
            e.body();

        std::vector<double> times_us;
        times_us.reserve(repeats_);
        for (int r = 0; r < repeats_; ++r)
        {
            if (e.setup) e.setup();

            auto t0 = Clock::now();
            for (int i = 0; i < e.iterations; ++i)
                e.body();
            auto t1 = Clock::now();

            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            double us_per_iter = (e.iterations > 0)
                ? (ms * 1000.0 / e.iterations)
                : 0.0;
            times_us.push_back(us_per_iter);
        }

        std::sort(times_us.begin(), times_us.end());

        Result r;
        r.name       = e.name;
        r.category   = e.category;
        r.iterations = e.iterations;
        r.samples    = (int)times_us.size();
        r.median_us  = times_us[times_us.size() / 2];
        r.best_us    = times_us.front();
        r.worst_us   = times_us.back();

        double mean = 0.0;
        for (double v : times_us) mean += v;
        mean /= (double)times_us.size();
        double var = 0.0;
        for (double v : times_us) var += (v - mean) * (v - mean);
        var /= (double)times_us.size();
        r.stddev_us = std::sqrt(var);

        results_.push_back(r);

        double noise_pct = (r.median_us > 0.0)
            ? 100.0 * r.stddev_us / r.median_us
            : 0.0;
        log << "median " << r.median_us << " us";
        if (noise_pct > 15.0)
            log << "  [noisy: sd " << noise_pct << "%]";
        log << "\n";
    }
}

inline void Runner::printHuman(std::ostream& os) const
{
    std::string current_category;
    for (const auto& r : results_)
    {
        if (r.category != current_category)
        {
            os << "\n--- " << r.category << " ---\n";
            current_category = r.category;
        }
        os << "  " << r.name;
        for (size_t i = r.name.size(); i < 42; ++i)
            os << ' ';
        os << "  " << r.median_us << " us"
           << "  (best " << r.best_us
           << ", worst " << r.worst_us
           << ", sd " << r.stddev_us << ")\n";
    }
}

inline void Runner::printJson(std::ostream& os) const
{
    // Format for github-action-benchmark `customSmallerIsBetter`.
    os << "[\n";
    for (size_t i = 0; i < results_.size(); ++i)
    {
        const auto& r = results_[i];
        os << "  {\"name\": \"" << r.name << "\", "
           << "\"unit\": \"us\", "
           << "\"value\": " << r.median_us << "}";
        if (i + 1 < results_.size()) os << ",";
        os << "\n";
    }
    os << "]\n";
}

} // namespace Bench