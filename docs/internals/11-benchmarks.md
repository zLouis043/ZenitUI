# Benchmarks

ZenitUI has a set of benchmarks that track the performance of the
core subsystems over time. They run on every push to `main`/`master`
and comment on every pull request.

## Running locally

From the repository root:

```bash
make bench-run
```

This builds `lib/libzenit.a` and `benchmarks/bin/bench`, then runs
the benchmarks with human-readable output.

For JSON output (used by CI):

```bash
./benchmarks/bin/bench --json
```

## What is measured

| Benchmark | Measures |
|-----------|----------|
| `Value x1 term (Px)` | Construction of a single-term `Value` (e.g. `Px(8)`). |
| `Value x2 terms (calc)` | Construction of a two-term `Value` (e.g. `calc(100% - 20px)`). |
| `ComputedStyle default + assign` | Constructing a `ComputedStyle` from scratch, then assigning it. |
| `ComputedStyle copy` | Copy-assigning an existing `ComputedStyle`. |
| `frame loop 100 nodes` | Full `frame()` on a 100-node tree with hover transitions, using the mock renderer. |

All benchmarks report the **best of 3 runs**, in microseconds per
iteration. Lower is better.

## Where results are stored

The `github-action-benchmark` action pushes results to the `gh-pages`
branch, under `dev/bench/`. The rendered chart is available at:

    https://zlouis043.github.io/ZenitUI/dev/bench/

Each benchmark has its own line, with a history of all pushes.

## Reading the chart

The absolute numbers depend on the CI runner (a shared GitHub-hosted
Ubuntu machine) and are **not** comparable to local numbers on your
own machine. What matters is the **trend**: a sudden jump on a specific
push indicates a regression introduced by that commit.

CI noise on the same runner is typically 5-15%. The alert threshold is
set to 150% (i.e. a benchmark that becomes 1.5× slower triggers a
comment on the PR), which is loose enough to avoid false positives.

## Adding a new benchmark

1. Open `benchmarks/bench.cpp`.
2. Add a new `bench_*` function using `best_of(...)`.
3. Call it from `main`, in the appropriate section.
4. The JSON output is generated automatically — no extra config.

Use the same iteration counts as the existing benchmarks (in the
millions for micro, hundreds of thousands for `ComputedStyle`, and
`NODES * FRAMES` for the frame loop). If a benchmark takes less than
a few milliseconds to run, it's too fast to be reliable; increase
`repeats`.

## Pitfalls

**Numbers vary between runs.** A single CI run is not authoritative;
look at the trend across several runs.

**Benchmarks are not precision measurements.** They are smoke tests
for "did something get significantly slower". Treat a 20% swing as
noise; treat a 200% swing as a real regression.

**The mock renderer is used.** No GPU work is measured. What's
measured is the layout, style, and update logic, which is where the
bulk of the per-frame CPU cost lives.

**The frame loop only exercises ~20 nodes of the 100.** The frustum
culling skips nodes outside the viewport. To stress the full tree,
either use a smaller viewport, or make the tree wider than the
viewport.

## See also

- [ROADMAP](../ROADMAP.md) — Phase 4 tracks the benchmarked
  optimizations.
- [Debugging](10-debug.md) — the test fixture used by the frame
  benchmark.