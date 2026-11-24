# ZenitUI

> A retained-mode C++20 UI framework with a Flexbox-inspired layout engine,
> a CSS-like styling language (**ZStyle**), a declarative UI DSL (**ZMarkup**),
> and pluggable rendering/platform backends.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Docs](https://img.shields.io/badge/docs-online-blue.svg)](https://zlouis043.github.io/ZenitUI/)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-beta-blue.svg)](#status)

---

## What is ZenitUI?

ZenitUI is a **retained-mode** UI framework written in modern C++20. It
brings the ergonomics of a web-like styling and layout model (Flexbox,
CSS transitions, `@keyframes`, `::part`, `var()`, `calc()`, `@media`)
without pulling in a browser engine or a scripting runtime.

You can build your UI imperatively in C++, declaratively with ZMarkup,
or mix both freely in the same node tree:

**1. Imperative C++ — fluent API**

```cpp
auto root = std::make_shared<Layout>(LayoutType::Stack);
root->size(Percent(100), Percent(100));
root->getInlineBase().itemsH = Align::Center;
root->getInlineBase().itemsV = Align::Center;

auto btn = Btn("Click me", [] {
    std::printf("hello!\n");
});
btn->cls("btn-primary");

root->addChild(btn);
```

**2. Declarative — ZMarkup + ZStyle**

```cpp
auto ui = ZMarkup::build(R"(
    VStack#root {
        Button.btn-primary#ok "Click me"
    }
)");
ui.onClick("ok", [] { std::printf("hello!\n"); });
```

```css
/* theme.zstyle */
.btn-primary {
    background: #0079F1;
    radius: 8px;
    padding: 12px 20px;
    transition: background 0.20s ease-out, scale 0.12s ease-out-back;
}
.btn-primary:hover   { background: #3296FF; scale: 1.05; }
.btn-primary:pressed { background: #505050; scale: 0.96; }
```

Both approaches produce the exact same tree of `Layout` nodes and share
the same ZStyle cascade, input routing, and rendering pipeline.

---

## Feature highlights

| Area | What you get |
|------|--------------|
| **Layout** | `Stack`, `Vertical`, and `Horizontal` Flexbox-inspired flows with iterative `grow` / `shrink`, `wrap`, `gap`, `margin: auto`, `setVisible` (`display: none`), `position: absolute`, `overflow: scroll \| auto`, `z-index`, and portals |
| **ZMarkup** | Declarative UI DSL (compiled in `libzenit`), reusable **Prefabs** with anonymous & named `<Slot>`s (`ZMarkup::instantiate`), `@ref` lookup helpers, structural **`If` & `Each`** (with keyed reconciliation), and cycle-free **`WeakUINode`** |
| **Units** | `px`, `%`, `vw`, `vh`, `pw`, `ph`, `auto`, and full `calc()` expressions |
| **Styling (ZStyle)** | CSS-like cascade (`#id` > `.class` > `Tag`), descendant selectors, `:hover` `:pressed` `:focus` `:checked` `:disabled`, `::part`, live inherited custom properties (`--token` / `var()`), `@media` queries, `background-texture` nine-slice, `text-overflow: ellipsis`, `white-space: nowrap`, and structured diagnostics with typo suggestions |
| **Animations** | Per-property transitions, CSS `@keyframes`, imperative `UIAnimation` timelines, and per-`::part` animations |
| **Input** | Hover / pressed / focus state machine, `Tab` navigation, pointer capture, `passThrough`, right-click, context menus, and system clipboard integration |
| **Effects** | `filter: blur(...) drop-shadow(...)`, custom GLSL shaders, extensible `FilterRegistry`, `box-shadow`, and automatic off-screen `TargetPool` reuse |
| **Widgets** | `Text`, `Button`, `Panel`, `Toggle`, `Checkbox`, `Slider`, `ProgressBar`, `TextInput`, `TextArea`, `ScrollView`, `VirtualView` (10k+ virtualized rows), `Dropdown`, `Modal`, `Tooltip`, `Popup`, `ContextMenu`, `CanvasLayout`, `ImageContainer` |
| **Data binding** | `Bindable<T>` — observable values with lifetime-safe RAII subscriptions and zero external dependencies |
| **Unicode & Text** | UTF-8 decoding (`Utf8`) and configurable font-atlas baking (`CodepointSet`) |
| **Backends** | Pluggable `IPlatform` / `IRenderer` / `IAssetProvider` architecture. Ships with a reference **Raylib** backend and a headless **Mock** backend for deterministic unit tests and CI |
| **Debugging** | Optional `Debug::dumpStyle` / `dumpTree` / `dumpTheme` / `dumpParts` / `dumpRenderStats` and structured `Logger` with source locations |

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

    root->addChild(Btn("Hello!", [] { std::printf("hi\n"); }));

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

See [`examples/01_hello_world/main.cpp`](examples/01_hello_world/main.cpp) for a ready-to-run version, or run `make run-00_demo` to launch the full multi-view showcase.

---

## Examples & Showcase

Every example in `examples/` is self-contained and compiled against the Raylib backend. Suggested reading order:

| Example | Focus |
|---------|-------|
| `01_hello_world` | Minimal Raylib wiring and frame loop |
| `02_layouts` & `03_widgets` | Flexbox containers, units, and built-in widgets |
| `04_animations` & `04b_imperative_animations` | CSS transitions, `@keyframes`, and imperative `UIAnimation` tracks |
| `05_zstyle` & `06_zmarkup` | Stylesheets, custom properties, and declarative markup |
| `07_effects` & `07b_effects_stress` | Post-processing filters, shaders, and `TargetPool` reuse |
| `08_portals`, `09_custom_widget`, `10_canvas` | Portals/overlays, custom `TLayout` components, and `CanvasLayout` |
| `11_ref_list`, `12_bindable`, `13_prefabs` | `@ref` lists, `Bindable<T>` reactivity, and reusable Prefabs with `<Slot>`s |
| `00_demo` | Complete 4-tab **ShowcaseApp**: Dashboard, Editor & Forms, 10,000-row Virtual Explorer + Portals, Effects & Motion, and live Dark/Light + Accent theme switching |

Run any example directly with `make run-<name>` (for example, `make run-00_demo`).

---

## Building

ZenitUI uses **CMake** as its primary build system. A thin `Makefile`
at the repository root delegates every step to CMake so the two never drift.

### Requirements

- CMake ≥ 3.20
- A C++20 compiler (GCC, Clang, MSVC, or MinGW-w64)
- Ninja (recommended; falls back to the default generator if absent)
- Git (used by `FetchContent` to pull Raylib on demand for examples)
- Doxygen + Graphviz (optional — only needed for `make docs`)

### From source

```bash
git clone [https://github.com/zLouis043/ZenitUI](https://github.com/zLouis043/ZenitUI)
cd ZenitUI

make               # build libzenit.a + unit tests
make test          # run the headless unit test suite
make examples      # build all examples (fetches Raylib automatically)
make run-00_demo   # build + run the ShowcaseApp demo
make docs          # generate the Doxygen HTML documentation
make bench         # build the benchmark harness
make dist          # install headers, lib, and CMake config into dist/
```

The `examples` and `run-*` targets fetch and build Raylib on demand via CMake `FetchContent`.

### Without the Makefile

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### `dist/` — consumable artifacts

```bash
make dist          # → dist/ with headers, libzenit.a, and CMake package config
```

```text
dist/
├── include/...                   # public headers
├── lib/libzenit.a                # static library
└── lib/cmake/ZenitUI/...         # CMake package files
```

---

## Integration with CMake

Whether consumed from source or from an install prefix, the core target is always `ZenitUI::ZenitUI` (propagating include paths and C++20 automatically).

### As a subdirectory

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

### Backends & Custom Ports

The core library is backend-agnostic and only talks to three interfaces in `include/UIContext.hpp`: `IPlatform`, `IRenderer`, and `IAssetProvider`. Built-in backends are opt-in:

| Backend | CMake target | Purpose |
|---------|--------------|---------|
| `raylib` | `ZenitUI::RaylibBackend` | Reference GPU/desktop implementation (`backends/raylib/`) |
| `mock`   | `ZenitUI::MockBackend`   | Headless testing & minimal backend skeleton (`backends/mock/`) |

Enable them via `-DZENITUI_BACKENDS="raylib;mock"` (tests pull in `mock` automatically; examples default to `raylib`):

```cmake
target_link_libraries(my_app PRIVATE
    ZenitUI::ZenitUI
    ZenitUI::RaylibBackend
)
```

Porting ZenitUI to SDL, GLFW + bgfx, Sokol, or a custom engine only requires implementing `IPlatform`, `IRenderer`, and `IAssetProvider`. Optional GPU capabilities (`Feature::Effects`, `Feature::NestedTargets`, `Feature::TargetSubRect`) can be opted into incrementally via `IRenderer::supports()`. See the **Writing a Custom Backend** guide in the Doxygen docs (`docs/custom_backends.dox`) and the reference implementations in `backends/`.

### Without CMake

```bash
make libzenit    # → build/libzenit.a
g++ main.cpp -I/path/to/ZenitUI/include -L/path/to/ZenitUI/build -lzenit -std=c++20
```

---

## Documentation

The full API reference, guides, and examples gallery are published online at:

👉 **[zlouis043.github.io/ZenitUI](https://zlouis043.github.io/ZenitUI/)**

You can also build the Doxygen site locally from source:

```bash
make docs
```

Then open `build/docs/html/index.html` in your browser. The documentation includes:

- Main Guide & Mental Model (`docs/Guides.dox`) — frame pipeline, module overview, and suggested reading order.

- Writing a Custom Backend (`docs/custom_backends.dox`) — wiring `UIContext`, minimal vs optional features, behavioral invariants, and references to `RaylibBackend` and `MockBackend`.

- Topic Modules — Core & Layout, Built-in Widgets, ZMarkup & Data Binding, ZStyle & Diagnostics, Animations & Filters, and Backends & Debugging.

- Examples Gallery — automatically assembled from `examples/` by `docs/gen_examples.cmake` (including sibling `.hpp`, `.zstyle`, `.zm`, and `.fs` files).

---

## Repository layout

```text
ZenitUI/
├── include/              # public headers (installed)
│   └── components/       # built-in widget headers
├── src/
│   ├── layout/           # layout engine, style resolver, measure/arrange, scroll
│   └── internal/         # animation, input, ZMarkup parser, diagnostics, …
├── backends/
│   ├── raylib/           # reference Raylib backend (IPlatform, IRenderer, IAssetProvider)
│   └── mock/             # headless mock backend & ZenitUI::Test::Env harness
├── assets/               # Inter fonts (SIL OFL), nine-slice textures, GLSL shaders
├── examples/             # self-contained runnable examples (00_demo … 13_prefabs)
├── tests/                # headless unit test suite
├── benchmarks/           # benchmark harness
├── docs/                 # Doxygen guides, custom theme, and gen_examples.cmake
├── build/                # CMake build tree & generated docs (gitignored)
└── dist/                 # installed artifacts via `make dist` (gitignored)
```

---
 
## Status

ZenitUI is in **beta**. The core API — `Layout` / `TLayout`, built-in widgets, ZStyle, ZMarkup, animations, input routing, and the `IPlatform` / `IRenderer` / `IAssetProvider` backend interfaces — is stable and covered by the headless test suite.

---

## Contributing

Contributions, bug reports, and custom backend ports are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup, testing guidelines, and pull request conventions.

---

## License

[MIT](LICENSE) © 2026 zLouis043 — Bundled Inter font assets are licensed under the [SIL Open Font License 1.1](assets/fonts/OFL.txt).