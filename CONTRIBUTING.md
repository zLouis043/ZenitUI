# Contributing to ZenitUI

Thanks for considering contributing. This document describes how to
report bugs, set up a development environment, write tests, and submit
pull requests.

---

## Code of conduct

Be respectful. Critique ideas, not people. Assume good faith.

---

## Reporting bugs

Before opening an issue:

1. **Check the existing issues.** Someone may have already reported the
   same problem.
2. **Check the documentation.** Browse the online guides and API
   reference at [zlouis043.github.io/ZenitUI](https://zlouis043.github.io/ZenitUI/)
   to confirm whether the behavior is intentional.

When you open an issue, include:

- **A minimal reproduction.** The smaller the better. Use the `Env`
  fixture from `tests/Mocks.hpp` (`ZenitUI::MockBackend`) if the bug is
  about layout, style, or input — it reproduces deterministically
  without a window.
- **Expected behavior.** What you thought would happen.
- **Actual behavior.** What actually happened.
- **Environment.** OS, compiler, and backend (Raylib version if relevant).
- **Log output**, if any (`Logger` writes to `stderr` by default).

Example issue template:

````markdown
### Description
Short description.

### Reproduction
```cpp
// Minimal snippet using Env from Mocks.hpp
```

### Expected
...

### Actual
...

### Environment
- OS: Windows 11
- Compiler: GCC / Clang / MSVC version
- Backend: Raylib 5.5 (or MockBackend)
````

---

## Proposing features

Before writing code for a non-trivial feature:

1. **Open an issue first.** Describe the use case, the proposed API,
   and any alternatives you considered. This avoids wasted work if the
   design needs to change.
2. **Reference a real use case.** "It would be nice to have X" is
   weaker than "I'm building Y and X is blocking me because Z."
3. **Respect the core architecture.** ZenitUI is retained-mode,
   CSS-like in styling, and backend-agnostic (`IPlatform`, `IRenderer`,
   `IAssetProvider`). Changes that couple core logic to a specific
   graphics library will not be accepted.

For small fixes (typos, clear bugs, doc improvements), feel free to open
a PR directly.

---

## Development setup

### Requirements

- CMake ≥ 3.20
- A C++20 compiler (GCC, Clang, MSVC, or MinGW-w64)
- Ninja (recommended)
- Git (for fetching Raylib on demand when building examples)
- Doxygen + Graphviz (optional, for `make docs`)

The core library and headless test suite have **no external runtime
dependencies** (Raylib is only fetched when building examples).

### Building and running tests

Using the root `Makefile` wrapper:

```bash
make               # build libzenit.a + unit tests
make test          # run the headless test suite via CTest
make examples      # build all examples (fetches Raylib automatically)
make run-00_demo   # build and launch the ShowcaseApp demo
make docs          # regenerate Doxygen HTML docs into build/docs/html/
```

Or directly with CMake:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite runs completely headless using `MockRenderer`,
`MockPlatform`, and `MockAssetProvider` from
`backends/mock/MockBackend.hpp` (exposed to tests via `tests/Mocks.hpp`).

---

## Pull request guidelines

### Before you submit

- **Run the full test suite (`make test`).** All existing tests must pass.
- **Add tests for new behavior or bug fixes.** See [Writing tests](#writing-tests).
- **Build the examples (`make examples`)** if you touched public APIs,
  styling, layout, or the Raylib backend.
- **Update documentation and run `make docs`.** Public APIs in `include/`
  and `backends/` should have Doxygen comments, and `make docs` should
  complete cleanly without new warnings.

### PR description

Include:

- **What** the PR changes.
- **Why** — the motivation, ideally linking to an issue.
- **How** — a brief summary of the approach.
- **Testing** — what you tested and how.

### Commits

Small, focused commits are preferred over one large commit. Each commit
should compile and pass tests on its own whenever possible.

Commit message style: short imperative subject line, blank line, then
details if needed.

```txt
fix(input): stop firing onHoverExit on Hover -> Pressed

The state comparison treated Pressed as "not hovered", which fired
onHoverExit when pressing a hovered node. Hover and Pressed are both
geometrically hovered, so the condition is updated.

Closes #42.
```

### Code style

Match the surrounding file's style:

- 4-space indentation.
- `camelCase` for methods and local variables, `PascalCase` for types.
- Trailing underscores for private fields (`knob_`, `value_`, `res_`).
- `auto` when the type is obvious, explicit types otherwise.
- Prefer `const&` or `std::string_view` for read-only parameters, and
  `const` methods where possible.
- Comments explain **why**, not **what**.

---

## Writing tests

Tests live in `tests/`, in files named `test_<area>.cpp`. They use a
lightweight macro framework defined in `tests/TestFramework.hpp`:

```cpp
TEST(SuiteName, test_name) {
    CHECK(condition);
    CHECK_NEAR(a, b, tolerance);
}
```

For tests that need a running UI (layout, input, style resolution,
widgets, targets), use the `Env` fixture from `Mocks.hpp`:

```cpp
TEST(Interaction_click, simple_click_increments) {
    Env env;
    int count = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);
    btn->onClick = [&count]() { count++; };

    root->addChild(btn);
    env.frame(root);           // layout settles
    env.click(root, {50, 25}); // press + release

    CHECK(count == 1);
}
```

### What to test

- **Parsers** — pure functions, no `Env` needed. Test the AST, resolved
  `Theme` / `Style`, and diagnostics.
- **Layout** — build a tree, run `env.frame(root)`, assert `getRect()`
  and `getMeasuredSize()`.
- **Input** — drive `env.platform.pressLeft()`, `releaseLeft()`,
  `moveMouse()`, `pressKey()`, `typeChar()`, `scrollWheel()`, then
  `env.frame(root)`.
- **Widgets** — test state transitions, callbacks, and intrinsic sizing.
- **Animations** — advance N frames and verify interpolated values.
- **Effects & TargetPool** — toggle `env.renderer.supportsEffects`,
  `supportsNestedTargets`, or `supportsTargetSubRect` to test shader
  uniform capture (`capturedFloat`, `capturedVec2`, …) and target
  lifecycle (`liveTargets`, `createTargetCount`, `destroyTargetCount`).

### What can't be tested with the mocks

- Visual pixel output and GPU shader execution (no pixel readback).
- Real font glyph metrics (the mock uses fixed monospace `charWidth × codepoints`).
- Windowing / OS-specific behavior (verify manually with `make run-00_demo`
  or the relevant example under `examples/`).

---

## Documentation

Documentation is generated with **Doxygen** (`Doxyfile`) and published
to [zlouis043.github.io/ZenitUI](https://zlouis043.github.io/ZenitUI/):

- **API reference** — Doxygen comments directly in `include/`,
  `include/components/`, and `backends/`. Document interface contracts on
  base classes (`IPlatform`, `IRenderer`, `IAssetProvider` in
  `include/UIContext.hpp`) so overrides inherit them automatically.
- **Guides** — narrative pages in `docs/`:
  - `docs/Guides.dox` — main page, mental model, and module definitions (`grp_*`).
  - `docs/custom_backends.dox` — step-by-step guide to implementing new backends.
- **Examples gallery** — `docs/gen_examples.cmake` automatically pulls
  every `examples/*/main.cpp` (plus sibling `.hpp`, `.zstyle`, `.zm`,
  and `.fs` files). Add `// @doc` and `// @group` tags at the top of any
  new example's `main.cpp` so it is indexed properly.

Always run `make docs` locally and check for broken `\ref` links or
Doxygen warnings before opening a PR.

---

## What we're looking for

- **Bug reports & regression tests** — especially around Flexbox edge
  cases, scrolling, text editing/wrapping, and ZStyle cascade rules.
- **Custom backends** — ports of `IPlatform`, `IRenderer`, and
  `IAssetProvider` to other ecosystems (SDL3, GLFW + bgfx, Sokol, etc.).
- **Widgets & ZStyle coverage** — refinements to existing widgets,
  accessibility/keyboard navigation, and ZStyle diagnostics.
- **Documentation & examples** — clearer docstrings, walkthroughs, and
  focused examples.

Look for issues tagged `good first issue` or `help wanted` if you are
not sure where to start.

---

## Questions?

Open a GitHub Discussion (if enabled) or an issue with the `question`
label.

---

## See also

- [README](README.md) — project overview and quick start.
- [Online Documentation](https://zlouis043.github.io/ZenitUI/) — full
  guides, API reference, and examples gallery.
- [Writing a Custom Backend](docs/custom_backends.dox) — guide for
  porting ZenitUI to other graphics/windowing libraries.