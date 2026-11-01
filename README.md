# ZenitUI

> A retained-mode UI framework for C++ with a CSS-like styling system,
> a Flexbox-inspired layout engine, and a declarative DSL (**ZMarkup** +
> **ZStyle**) for building interfaces without writing boilerplate.

**Status:** 🗓️ *beta* — the core API is frozen. See
[docs/ROADMAP.md](docs/ROADMAP.md) for what's still planned.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-beta-blue.svg)](#status)

---

## What is ZenitUI?

ZenitUI is a **retained-mode** UI framework written in modern C++. It
gives you the ergonomics of a web-like styling and layout system
(Flexbox, CSS transitions, `@keyframes`, `::part`, `var()`, `calc()`)
without pulling in a browser engine or a scripting runtime.

You write your UI in one of two ways, or both, mixed freely:

**1. Imperative C++ — with a fluent API**

```cpp
auto root = std::make_shared<Layout>(LayoutType::Stack);
root->size(Percent(100), Percent(100));
root->getInlineBase().itemsH = Align::Center;
root->getInlineBase().itemsV = Align::Center;

auto btn = Btn("Click me", []{
    std::printf("hello!\n");
});
btn->cls("btn-primary");

root->addChild(btn);
```

**2. Declarative — with ZMarkup + ZStyle**

```cpp
auto ui = ZMarkup::build(R"(
    VStack#root {
        Button.btn-primary#ok "Click me"
    }
)");
ui.onClick("ok", []{ std::printf("hello!\n"); });
```

```css
/* assets/game.zstyle */
.btn-primary {
    background: #0079F1;
    radius: 8px;
    padding: 1.5vh 2vw;
    transition: background 0.25s ease-out, scale 0.12s ease-out-back;
}
.btn-primary:hover   { background: #3296FF; scale: 1.15; }
.btn-primary:pressed { background: #505050; scale: 0.90; }
```

Both approaches produce the same tree of `Layout` nodes, and both are
fully compatible with the same styling system.

---

## Feature highlights

| Area | What you get |
|------|--------------|
| **Layout** | `Stack`, `Vertical`, `Horizontal` flow, `grow` / `shrink` / `gap`, absolute positioning, `overflow: scroll \| auto`, `z-index`, portals |
| **Units** | `px`, `%`, `vw`, `vh`, `pw`, `ph`, `auto`, and full `calc()` support |
| **Styling** | CSS-like cascade, descendant selectors, `:hover` `:pressed` `:focus` `:checked` `:disabled`, `::part`, custom properties (`--x` / `var()`), media queries |
| **Animations** | Per-property transitions, CSS `@keyframes`, imperative `UIAnimation`, per-`::part` animations |
| **Input** | Hover / pressed / focus state machine, `Tab` navigation, pointer capture, `passThrough`, right-click, context menus |
| **Effects** | `filter: blur(...) drop-shadow(...)`, arbitrary shaders, extensible `FilterRegistry`, `box-shadow` |
| **Widgets** | Text, Button, Panel, Toggle, Checkbox, Slider, ProgressBar, TextInput, ScrollView, Dropdown, Modal, Tooltip, Popup, Canvas, ImageContainer |
| **Backends** | Pluggable. Reference implementation for **Raylib**, plus a headless **mock** backend for tests and CI. The core only depends on `IRenderer` / `IPlatform` / `IAssetProvider`. |
| **Debugging** | Optional `Debug::dumpStyle` / `dumpTree` / `dumpTheme` / `dumpParts`, structured `Logger` with source locations |
| **Data binding** | `Bindable<T>` — observable values with RAII subscriptions, lifetime-safe, zero dependencies |
| **Testing** | The `ZenitUI::MockBackend` target provides `MockRenderer`, `MockPlatform`, and `MockAssetProvider` for full headless interaction tests. |

---

## Quick start

```cpp
#include <raylib.h>
#include "RaylibBackend.hpp"
#include "ZenitUI.hpp"

using namespace ZenitUI;

int main() {
    InitWindow(800, 600, "ZenitUI");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().itemsH = Align::Center;
    root->getInlineBase().itemsV = Align::Center;

    root->addChild(Btn("Hello!", []{ std::printf("hi\n"); }));

    while (!WindowShouldClose()) {
        root->runFrame(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        root->renderFrame();
        EndDrawing();
    }
    CloseWindow();
}
```

A full walkthrough is in **[Getting Started](docs/user/01-getting-started.md)**.

---

## Building

ZenitUI uses **CMake** as its primary build system. A thin `Makefile`
at the repository root preserves the classic developer workflow and
delegates every step to CMake — there is no drift between the two.

### Requirements

- CMake ≥ 3.20
- A C++20 compiler (GCC, Clang, MSVC, or MinGW-w64)
- Ninja (recommended; falls back to the default generator if absent)
- Git (used by `FetchContent` to pull Raylib for the examples)

### From source

```bash
git clone https://github.com/zLouis043/ZenitUI
cd ZenitUI

make               # build libzenit.a + unit tests
make test          # run the test suite
make examples      # build all examples (fetches Raylib automatically)
make run-00_demo   # build + run a specific example
make bench         # build the benchmark harness
make dist          # install into dist/ for downstream consumption
```

The `examples` and `run-*` targets fetch and build Raylib on demand.
There is **no `deps/raylib/`** folder in the repo — the build is
self-contained.

### Without the Makefile

The Makefile is purely a convenience. The same workflow works with
plain CMake:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### `dist/` — consumable artifacts

```bash
make dist          # → dist/ with headers, libzenit.a, and CMake config
```

`dist/` contains everything a consumer project needs, in a stable
layout:

```
dist/
├── include/...                   # public headers
├── lib/libzenit.a                # static library
└── lib/cmake/ZenitUI/...         # CMake package files
```

---

## Integration with CMake

The target is always `ZenitUI::ZenitUI`, whether you consume the
library from source or from an install. Your `CMakeLists.txt` doesn't
change.

### As a subdirectory (source tree)

```cmake
add_subdirectory(third_party/ZenitUI)
target_link_libraries(my_app PRIVATE ZenitUI::ZenitUI)
```

### As an installed package

```cmake
find_package(ZenitUI REQUIRED)
target_link_libraries(my_app PRIVATE ZenitUI::ZenitUI)
```

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/ZenitUI/dist
```

The public interface (include dirs, C++20, warning flags) is propagated
automatically.

### Backends

The core is backend-agnostic; backends are opt-in. Currently available:

| Backend | CMake target | Purpose |
|---------|--------------|---------|
| `raylib` | `ZenitUI::RaylibBackend` | Reference implementation |
| `mock`   | `ZenitUI::MockBackend`   | Headless testing (no GPU, no window) |

Enable them with `-DZENITUI_BACKENDS="raylib;mock"` (semicolon-separated).
Tests automatically pull in `mock`; examples default to `raylib`.

```cmake
target_link_libraries(my_app PRIVATE
    ZenitUI::ZenitUI
    ZenitUI::RaylibBackend
)
```

### Without CMake

```bash
make libzenit    # → build/libzenit.a
g++ main.cpp -I/path/to/ZenitUI/include -L/path/to/ZenitUI/build -lzenit -std=c++20
```

---

## Repository layout

```
ZenitUI/
├── include/              # public headers (installed)
│   └── components/       # widget headers
├── src/
│   ├── layout/           # layout engine, style resolver, measure/arrange
│   └── internal/         # animation, input, scroll, ZMarkup parser, …
├── backends/
│   ├── raylib/           # reference backend (fetches Raylib)
│   └── mock/             # headless mock backend for tests
├── examples/             # one folder per example (each has main.cpp)
├── tests/                # unit tests
├── benchmarks/           # benchmark harness
├── docs/                 # user, internals, api
├── build/                # CMake build tree (gitignored)
└── dist/                 # installed artifacts via `make dist` (gitignored)
```

---

## Documentation

The full documentation lives in [`docs/`](docs/):

### New here?

- [**Getting Started**](docs/user/01-getting-started.md) — setup, first layout, first widget
- [**Cheatsheet**](docs/CHEATSHEET.md) — one-page reference for day-to-day work
- [**Glossary**](docs/GLOSSARY.md) — terminology used across the framework

### User guide

- [Layout](docs/user/02-layout.md) — Stack / Vertical / Horizontal, flexbox, units
- [Widgets](docs/user/03-widgets.md) — reference for every built-in widget
- [ZMarkup](docs/user/04-zmarkup.md) — the declarative DSL
- [ZStyle](docs/user/05-zstyle.md) — the `.zstyle` styling language
- [Events](docs/user/06-events.md) — click, hover, focus, bubbling, pointer capture
- [Animations](docs/user/07-animations.md) — transitions and keyframes
- [Portals](docs/user/08-portals.md) — Popup, Tooltip, Dropdown, Modal
- [Effects](docs/user/09-effects.md) — filters, shaders, `FilterRegistry`
- [Custom Widgets](docs/user/10-custom-widgets.md) — build your own component
- [Data Binding](docs/user/11-bindable.md) — `Bindable<T>` for model ↔ UI

### Internals (for contributors)

- [Architecture](docs/internals/01-architecture.md) — high-level view, the frame pipeline
- [Lifecycle](docs/internals/02-lifecycle.md) — `update` / `measure` / `arrange` / `draw`
- [Style System](docs/internals/03-style-system.md) — cascade, specificity, inheritance
- [Animation System](docs/internals/04-animation-system.md) — imperative vs CSS vs `::part`
- [Layout Engine](docs/internals/05-layout-engine.md) — intrinsic size, flow, dirty tracking
- [Render Pipeline](docs/internals/06-render-pipeline.md) — inline vs layer, filters, portals
- [Input System](docs/internals/07-input-system.md) — FSM, focus, pointer capture
- [Scroll System](docs/internals/08-scroll-system.md) — `ScrollState`, arrange, inertia
- [Backend](docs/internals/09-backend.md) — implementing `IRenderer` / `IPlatform`
- [Debugging](docs/internals/10-debug.md) — dumps, logging, headless tests

### API reference

- [`Layout`](docs/api/Layout.md) ·
- [`Widgets`](docs/api/Widgets.md) ·
- [`Unit`](docs/api/Unit.md) ·
- [`Style`](docs/api/Style.md) ·
- [`Theme`](docs/api/Theme.md) ·
- [`ZMarkup`](docs/api/ZMarkup.md) ·
- [`ZStyle`](docs/api/ZStyle.md) ·
- [`FilterRegistry`](docs/api/FilterRegistry.md) ·
- [`UIContext`](docs/api/UIContext.md) ·
- [`Bindable`](docs/api/Bindable.md)

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Status and roadmap

ZenitUI is in **beta**. The core API is frozen: widgets, styling,
layout, animations, events, and the backend interfaces described in
the user guide and API reference are stable. Breaking changes are
still possible for work that lands in Phases 4–8 of the roadmap
(performance, feature completion, tooling), but the surface described
in Phases 1–3 will not change.

See [docs/ROADMAP.md](docs/ROADMAP.md) for the detailed plan from
alpha to beta to 1.0, including known issues and planned features.

---

## License

[MIT](LICENSE) © 2026 zLouis043