# Contributing to ZenitUI

Thanks for considering contributing. This document describes how to
report bugs, propose changes, and submit pull requests.

---

## Code of conduct

Be respectful. Critique ideas, not people. Assume good faith.

---

## Reporting bugs

Before opening an issue:

1. **Check the existing issues.** Someone may have already reported the
   same problem.
2. **Check the documentation.** Some behavior that looks like a bug is
   intentional — see the "Pitfalls" sections in the docs.
3. **Check the roadmap.** Known issues and planned work are listed in
   [docs/ROADMAP.md](docs/ROADMAP.md).

When you open an issue, include:

- **A minimal reproduction.** The smaller the better. Use the
  `Mocks.hpp` fixture if the bug is about layout, style, or input —
  it lets you reproduce without a window.
- **Expected behavior.** What you thought would happen.
- **Actual behavior.** What actually happened.
- **Environment.** OS, compiler, backend (Raylib version if relevant).
- **Log output** if any. `Logger` writes to `stderr` by default.

Example issue template:

```markdown
### Description
Short description.

### Reproduction
```cpp
// Minimal snippet using Mocks.hpp
```

### Expected
...

### Actual
...

### Environment
- OS: Windows 11
- Compiler: MSVC 19.38
- Backend: Raylib 5.5
```

---

## Proposing features

Before writing code for a non-trivial feature:

1. **Open an issue first.** Describe the use case, the proposed API,
   and any alternatives you considered. This avoids wasted work if the
   design needs to change.
2. **Reference a real use case.** "It would be nice to have X" is
   weaker than "I'm building Y and X is blocking me because Z."
3. **Consider the existing design.** ZenitUI has opinions (retained
   mode, CSS-like styling, pluggable backends). A feature that fights
   those opinions is unlikely to be accepted.

For small fixes (typos, obvious bugs, doc improvements), just open the
PR directly.

---

## Development setup

*TODO — fill in once the build system is finalized.*

Minimum required:

- A C++17 compiler (GCC, Clang, or MSVC).
- Raylib (for the reference backend and the manual test apps).

To build the tests without a window, only the core library and the
mock backend are needed — no Raylib.

### Running the tests

```bash
# Once the build is set up:
ctest --test-dir build
# or:
./build/tests/tests
```

The test suite is headless — it uses `MockRenderer` and `MockPlatform`
from `tests/Mocks.hpp`, so no window is created.

---

## Pull request guidelines

### Before you submit

- **Run the tests.** All existing tests must pass.
- **Add tests for new behavior.** See "Writing tests" below.
- **Update the documentation.** If you add a widget, an API, or a
  behavior, add or update the corresponding file in `docs/`.
- **Update the roadmap.** If you complete an item from
  [docs/ROADMAP.md](docs/ROADMAP.md), check it off.

### PR description

Include:

- **What** the PR changes.
- **Why** — the motivation, ideally linking to an issue.
- **How** — a brief description of the approach.
- **Testing** — what you tested and how.

### Commits

Small, focused commits are preferred over one large commit. A commit
should compile and pass tests on its own when possible.

Commit message style: short imperative subject line, blank line, then
details if needed.

```
Fix onHoverExit firing on Hover -> Pressed

The state comparison treated Pressed as "not hovered", which fired
onHoverExit when pressing a hovered node. Hover and Pressed are both
geometrically hovered, so the condition is updated.

Closes #42.
```

### Code style

There's no formal style guide yet. Match the surrounding code:

- 4-space indentation.
- `snake_case` for private fields, `camelCase` for public methods,
  `PascalCase` for types.
- Trailing underscores for private fields (`knob_`, `value_`).
- `auto` when the type is obvious, explicit otherwise.
- Prefer `const&` for parameters, `const` methods where possible.
- Comments explain **why**, not **what**.

If you see a clear inconsistency, matching the file you're editing is
better than introducing a new style.

---

## Writing tests

Tests live in `tests/`, in files named `test_<area>.cpp`. They use a
small macro-based framework defined in `TestFramework.hpp`:

```cpp
TEST(SuiteName, test_name) {
    CHECK(condition);
    CHECK_NEAR(a, b, tolerance);
}
```

For tests that need a running UI (layout, input, style resolution),
use the `Env` fixture from `Mocks.hpp`:

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

- **Parsers** — pure functions, no `Env` needed. Test the AST and the
  resulting `Theme` / `Style`.
- **Layout** — build a tree, run `env.frame(root)`, check `getRect()`
  and `getMeasuredSize()`.
- **Input** — use `platform.pressLeft()`, `releaseLeft()`,
  `moveMouse()`, `pressKey()`, then `env.frame(root)`.
- **Widgets** — test the widget's specific behavior (state changes,
  callbacks, intrinsic size).
- **Animations** — run N frames and check the interpolated values.

### What can't be tested with the mocks

- Visual output (no pixel readback).
- Shaders and filters (the mock renderer reports
  `supports(Effects) == false`).
- Render targets (the mock returns invalid handles).
- Exact text measurement (the mock uses `charWidth × length`).

For these, test the logic in isolation or use a real backend with an
offscreen framebuffer.

---

## Documentation

Documentation lives in `docs/`, split into three sections:

- **`docs/user/`** — for people writing UI. Tutorials, patterns,
  pitfalls. Examples in C++ and ZMarkup.
- **`docs/internals/`** — for people extending the core. Architecture,
  subsystem mechanics, invariants.
- **`docs/api/`** — compact reference. Method signatures, fields,
  enum values.

When you change a public API, update the corresponding file in
`docs/api/`. When you change behavior that users rely on, update the
relevant `docs/user/` file.

### Style

- Markdown, one sentence per line where it helps readability.
- Fenced code blocks with language tags (`cpp`, `css`, `markup`).
- Cross-reference between files with relative links.
- Each doc file ends with a "See also" section.

---

## Reporting documentation issues

Typos, unclear paragraphs, broken examples — open an issue or a PR.
Documentation PRs are welcome and reviewed quickly.

---

## What we're looking for

The current priorities (see [docs/ROADMAP.md](docs/ROADMAP.md)):

- **Phase 1** — bug fixes. See the specific items in the roadmap.
- **Phase 2** — small API additions (`Slider::setValue`,
  `removeClass`, etc.).
- **Phase 3** — test coverage for scroll, TextInput, Dropdown, text
  wrapping.
- **Phase 4** — performance (allocation reduction, rule indexing).

If you want to help but don't know where to start, look for issues
tagged `good first issue` or `help wanted`.

---

## Questions?

Open a discussion (if enabled) or an issue with the `question` label.

---

## See also

- [README](README.md) — project overview.
- [Roadmap](docs/ROADMAP.md) — planned work and known issues.
- [Architecture](docs/internals/01-architecture.md) — how the framework
  is built.