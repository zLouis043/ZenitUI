# Benchmarks

ZenitUI has a set of benchmarks that track the performance of the
core subsystems over time. They run on every push to `main`/`master`,
comment on every pull request, and generate a chart on
[the project's GitHub Pages site](https://zlouis043.github.io/ZenitUI/dev/bench/).

## Running locally

From the repository root:

```bash
make bench-run
```

This builds `lib/libzenit.a` and `benchmarks/bin/bench`, runs all
benchmarks, and compares the results against `benchmarks/baseline.json`.

Output is human-readable: one line per benchmark with median, best,
worst, and standard deviation. At the end, a comparison table shows the
delta against the baseline, with `! REG` on regressions above 20% and
`+ IMP` on improvements above 20%.

For JSON output (used by CI):

```bash
./benchmarks/bin/bench --json
```

### Updating the baseline

When a benchmark change is intentional (an optimization that shifts
values by more than noise), regenerate the baseline:

```bash
make bench-update
```

This overwrites `benchmarks/baseline.json` with the current medians.
The file is versioned and should be committed together with the change
that motivated it.

**Don't update the baseline to silence a regression.** If a benchmark
got slower, investigate first. The baseline tracks the current *known
good* state of the framework.

## Framework design

The benchmarks live in `benchmarks/`:

- `bench_framework.hpp` — the runner (header-only).
- `benches.cpp` — the benchmark definitions.
- `main.cpp` — entry point.
- `baseline.json` — the current reference values.

Each benchmark is a `Runner::add` call with:

- A **name** (`area.subarea.name`, e.g. `micro.Value.construct_1term`).
- A **category** (used for grouping in the human output).
- **Iterations** per run (the body is called this many times between
  the two clock reads).
- A **setup** callback (called before every run, including the
  warm-up, to restore a known state).
- A **body** callback (called `iterations` times per run).

The runner:

1. Runs the body once as a warm-up (not measured).
2. For each of 5 repeats: calls setup, then times `iterations` calls
   to body.
3. Reports the **median** of the 5 runs.

### Why median, not best-of-N

- **Best-of-N hides regressions.** A regression that only slows the
  typical case but leaves the best case untouched will not show up.
- **Median is robust to scheduler noise.** On shared CI runners,
  occasional slow runs are common; the median ignores them.
- **Median is stable with few runs.** Five samples are enough for a
  reliable median, while best-of-N needs many more to be meaningful.

The `best`, `worst`, and `stddev` are reported for diagnostics. If
`stddev / median` exceeds 15%, the runner flags the entry as noisy.

### Preventing dead-code elimination

Bodies write to a global `g_sink` (a `float`), which `main` reads once
at the end. Without this, the compiler would eliminate the entire body
of a micro-benchmark that has no observable output.

## Coverage

The 27 benchmarks cover every area of the framework where per-frame
or per-operation cost matters. The areas are fixed: a new one is added
only if the framework gains a new subsystem, and removed only if a
subsystem disappears in a rewrite.

| Category | Benchmarks | What they measure |
|----------|------------|-------------------|
| `parsing` | `zstyle.200rules`, `zmarkup.100elements` | `loadStyleString` and `ZMarkup::build` |
| `value` | `construct_1term`, `construct_2term`, `lerp` | `Value` construction and interpolation |
| `computed_style` | `from`, `copy` | `ComputedStyle` resolution and copy |
| `animation` | `evaluateKeyframes`, `tickCss.100`, `tickImperative.100` | Keyframe evaluation and animation ticks |
| `tree` | `addChild.1000`, `removeAllChildren.1000` | Tree mutation |
| `small_vector` | `push_back_inline` | The `SmallVector` container |
| `frame` | `static100`, `layout_stress`, `200rules`, `transitions100`, `animated100`, `wide200`, `mixed200`, `large1000` | Full frame loop in different scenarios |

### The frame scenarios

| Name | Nodes | Rules | Active state |
|------|-------|-------|--------------|
| `system.frame.static100` | 100 | 0 | None (measures tree traversal + draw culling only) |
| `system.frame.layout_stress` | 100 | 0 | Container width toggles each frame (forces measure + arrange) |
| `system.frame.200rules` | 100 | 200 | Style re-resolve on every node per frame |
| `system.frame.transitions100` | 100 | 2 | Hover transitions on alternating nodes |
| `system.frame.animated100` | 100 | 1 (`@keyframes`) | CSS animation on every node |
| `system.frame.wide200` | 200 siblings | 0 | Z-index sort on every frame |
| `system.frame.mixed200` | 200 | 2 | 1 animated, 1 hovered, 10 dirty, 188 static |
| `system.frame.large1000` | 1000 | 0 | Long list with frustum culling |

The five original scenarios cover individual hot paths in isolation.
The three added after review cover the cases that don't show up in
those isolated tests:

- `layout_stress` measures the real measure + arrange cost, not the
  early-exit tree traversal that `static100` measures.
- `mixed200` measures the shape of a real frame, where only a
  fraction of the nodes change in any given frame.
- `large1000` measures the cost of iterating a large tree with culling
  active — the realistic case for a long scrollable list.

## Where results are stored

The `github-action-benchmark` action pushes results to the `gh-pages`
branch, under `dev/bench/`. The rendered chart is at:

    https://zlouis043.github.io/ZenitUI/dev/bench/

Each benchmark has its own line. The values are medians in
microseconds; lower is better.

## Reading the chart

The absolute numbers depend on the CI runner (a shared GitHub-hosted
Ubuntu machine) and are **not** comparable to local numbers on your
own machine. What matters is the **trend**: a sudden jump on a
specific push indicates a regression introduced by that commit.

CI noise on the same runner is typically 5-15%. The alert threshold
is set to 150% — a benchmark has to become 1.5× slower to trigger a
PR comment. This is loose enough to avoid false positives while still
catching real regressions.

The `baseline.json` file in the repo is a **local** reference,
separate from the CI chart. It captures the developer's "known good"
state on their machine and is used by `make bench-run`.

## Adding a new benchmark

1. Open `benchmarks/benches.cpp`.
2. Add a new `R.add(...)` call in the appropriate category.
3. If the body operates on pre-built data, add the shared storage to
   the anonymous namespace at the top of the file and rebuild it in
   `setup()`.

Use iteration counts that give each run at least a few milliseconds
of work:

- Micro-benchmarks (single operations): millions of iterations.
- Small scenarios (100-element loops): thousands of iterations.
- Frame loops: hundreds of frames.

If a run takes less than 1 ms, increase `iterations`.

## Pitfalls

**Numbers vary between runs.** A single run is not authoritative; look
at the trend across several runs.

**Benchmarks are smoke tests, not precision measurements.** They catch
"did something get significantly slower". Treat a 20% swing as noise;
treat a 200% swing as a real regression.

**The mock renderer is used.** No GPU work is measured. What's
measured is the layout, style, and update logic — the CPU cost per
frame. Backend timing is out of scope; use a profiler for that.

**Local numbers and CI numbers are not comparable.** Different CPUs,
different scheduling, different OS. Only the trend on a single system
is meaningful.

**The baseline is per-machine.** If you develop on two machines (say,
a desktop and a laptop), the baseline committed by one may not match
the other. Either keep a single-machine baseline and ignore the
comparison output on the other, or use the `--update` flag per machine
and don't commit.

**Frame benchmarks use the mock renderer with a small viewport.** Only
about 20 of 100 nodes are visible after frustum culling. To stress the
full tree, either use a smaller viewport or a wider tree.

## See also

- [ROADMAP](../ROADMAP.md) — Phase 4 tracks the benchmarked
  optimizations.
- [Debugging](10-debug.md) — the test fixture used by the frame
  benchmarks.