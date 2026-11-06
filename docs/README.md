# ZenitUI

> A retained-mode UI framework for games and tools, with a CSS-like styling
> system, a Flexbox-inspired layout engine, and an optional declarative DSL
> (**ZMarkup**) for building interfaces without writing C++ boilerplate.

**Status:** 🗓️ *beta* — the core API is frozen. See [ROADMAP](ROADMAP.md)
for what's still planned.

---

## What is ZenitUI?

ZenitUI is a **retained-mode** UI framework written in modern C++.
It gives you the ergonomics of a web-like styling and layout system
(Flexbox, CSS transitions, `@keyframes`, `::part`, `var()`, `calc()`)
without pulling in a browser engine or a scripting runtime.

You write your UI in one of two ways (or both, mixed):

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

```css
/* game.zstyle */
.btn-primary {
    background: #0079F1;
    radius: 8px;
    padding: 1.5vh 2vw;
    transition: background 0.25s ease-out-quad, scale 0.12s ease-out-back;
}
.btn-primary:hover   { background: #3296FF; scale: 1.15; }
.btn-primary:pressed { background: #505050; scale: 0.90; }
```

```cpp
// C++
auto ui = ZMarkup::build(R"(
    VStack#root {
        Button.btn-primary#go "Click me"
    }
)");
ui.onClick("go", []{ std::printf("hello!\n"); });
```

Both approaches produce the same tree of `Layout` nodes, and both are
fully compatible with the same styling system.

---

## Feature highlights

| Area            | What you get |
|-----------------|--------------|
| **Layout**      | `Stack`, `Vertical`, `Horizontal` flow, `grow` / `shrink` / `gap`, `position: absolute`, `overflow: scroll \| auto`, `z-index`, portals |
| **ZMarkup**     | Declarative UI DSL, reusable **Prefabs** with `<Slot>` injection, inline data binding variables, zero-overhead AST cloning |
| **Units**       | `px`, `%`, `vw`, `vh`, `pw`, `ph`, `auto`, full `calc()` support |
| **Styling**     | CSS-like cascade, descendant selectors, `:hover` `:pressed` `:focus` `:checked` `:disabled`, `::part`, custom properties (`--x` / `var()`), media queries |
| **Animations**  | Per-property transitions (`transition: opacity 0.3s ease-out`), CSS `@keyframes`, imperative `UIAnimation`, per-`::part` animations |
| **Input**       | Hover / pressed / focus state machine, `Tab` navigation, pointer capture, `passThrough`, right-click, context menus |
| **Effects**     | `filter: blur(...) drop-shadow(...)`, arbitrary shaders, extensible `FilterRegistry`, `box-shadow` |
| **Backend**     | Pluggable. Reference implementation ships for **Raylib**, but the core only depends on `IRenderer` / `IPlatform` / `IAssetProvider` |
| **Debugging**   | Optional `Debug::dumpStyle` / `dumpTree` / `dumpTheme` / `dumpParts`, structured `Logger` with source locations |
| **Testing**     | Headless `MockRenderer` + `MockPlatform` allow full interaction tests without opening a window |

---

## Building ZenitUI

ZenitUI uses **CMake** as its primary build system. A thin `Makefile` at
the repo root preserves the classic developer workflow (`make test`,
`make examples`, …) and delegates every step to CMake — so there's no
drift between the two.

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
```

The `examples` and `run-*` targets use CMake's `FetchContent` to
download and build Raylib on demand. There is **no `deps/raylib/`**
folder in the repo — the build is self-contained.

### `dist/` — consumable artifacts

```bash
make dist          # → dist/ with headers, libzenit.a, and CMake config
```

`dist/` contains everything a consumer project needs, in a stable layout:

```
dist/
├── include/ZenitUI/...          # public headers
├── lib/libzenit.a               # static library
└── lib/cmake/ZenitUI/...        # CMake package files
```

## Integration with CMake

The target is always `ZenitUI::ZenitUI`, whether you consume the library
from source or from an install — your `CMakeLists.txt` doesn't change.

### Backends

The core is backend-agnostic; backends are opt-in. Currently available:

| Backend | CMake target | Purpose |
|---------|--------------|---------|
| `raylib` | `ZenitUI::RaylibBackend` | Reference implementation |
| `mock` | `ZenitUI::MockBackend` | Headless testing (no GPU, no window) |

Enable them with `-DZENITUI_BACKENDS="raylib;mock"` (comma-separated).
Tests automatically pull in `mock`; examples default to `raylib`.

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
automatically. To use the Raylib reference backend, link it too:

```cmake
target_link_libraries(my_app PRIVATE
    ZenitUI::ZenitUI
    ZenitUI::RaylibBackend
)
```

### Without CMake

```bash
make libzenit      # → build/libzenit.a
g++ main.cpp -Idist/include -Ldist/lib -lzenit -std=c++20
```

## Repository layout

```
ZenitUI/
├── include/              # public headers (installed)
│   └── components/       # widget headers
├── src/
│   ├── layout/           # layout engine, style resolver, measure/arrange
│   └── internal/         # animation, input, scroll, z-markup parser, …
├── backends/raylib/      # reference backend (optional, fetched Raylib)
├── examples/             # one folder per example (each has main.cpp)
├── tests/                # unit tests
├── benchmarks/           # benchmark harness
├── build/                # CMake build tree (gitignored)
└── dist/                 # installed artifacts via `make dist` (gitignored)
```

---

## Quick start

```cpp
#include <raylib.h>
#include "RaylibBackend.hpp"
#include "ZenitUI.hpp"

using namespace ZenitUI;

int main() {
    InitWindow(800, 600, "ZenitUI");

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(Btn("Hello!", []{ std::printf("hi\n"); }));

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        root->runFrame(dt);        // input, layout, animation

        BeginDrawing();
        ClearBackground(BLACK);
        root->renderFrame();
        EndDrawing();
    }
    CloseWindow();
}
```

See **[Getting Started](user/01-getting-started.md)** for a full walkthrough.

---

## Documentation map

### For users (writing UI)

- [Getting Started](user/01-getting-started.md) — set up the backend, first layout, first widget
- [Layout](user/02-layout.md) — Stack / Vertical / Horizontal, flexbox, units
- [Widgets](user/03-widgets.md) — reference for every built-in widget
- [ZMarkup](user/04-zmarkup.md) — the declarative DSL
- [ZStyle](user/05-zstyle.md) — the `.zstyle` styling language
- [Events](user/06-events.md) — click, hover, focus, bubbling, pointer capture
- [Animations](user/07-animations.md) — transitions and keyframes
- [Portals](user/08-portals.md) — Popup, Tooltip, Dropdown, Modal
- [Effects](user/09-effects.md) — filters, shaders, `FilterRegistry`
- [Custom Widgets](user/10-custom-widgets.md) — build your own component
- [Data Binding](user/11-bindable.md) — `Bindable<T>` for model ↔ UI
- [VirtualView](user/12-virtual_view.md) — virtualized lists for 10k+ rows

### For contributors (extending the core)

- [Architecture](internals/01-architecture.md) — high-level view, the frame pipeline
- [Lifecycle](internals/02-lifecycle.md) — `update` / `measure` / `arrange` / `draw`
- [Style System](internals/03-style-system.md) — cascade, specificity, inheritance
- [Animation System](internals/04-animation-system.md) — imperative vs CSS vs `::part`
- [Layout Engine](internals/05-layout-engine.md) — intrinsic size, flow, dirty tracking
- [Render Pipeline](internals/06-render-pipeline.md) — inline vs layer, filters, portals
- [Input System](internals/07-input-system.md) — FSM, focus, pointer capture
- [Scroll System](internals/08-scroll-system.md) — `ScrollState`, arrange, inertia
- [Backend](internals/09-backend.md) — implementing `IRenderer` / `IPlatform`
- [Debugging](internals/10-debug.md) — dumps, logging, headless tests
- [Benchmarks](internals/11-benchmarks.md) — performance tracking and the CI chart

### API reference

- [`Layout`](api/Layout.md)
- [`Widgets`](api/Widgets.md)
- [`Unit`](api/Unit.md)
- [`Style`](api/Style.md)
- [`Theme`](api/Theme.md)
- [`ZMarkup`](api/ZMarkup.md)
- [`ZStyle`](api/ZStyle.md)
- [`FilterRegistry`](api/FilterRegistry.md)
- [`UIContext`](api/UIContext.md)
- [`Bindable`](api/Bindable.md)

---

## License

MIT License

Copyright (c) 2026 zLouis043

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.