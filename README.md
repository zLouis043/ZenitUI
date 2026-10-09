# ZenitUI

> A retained-mode UI framework for C++ with a CSS-like styling system,
> a Flexbox-inspired layout engine, and a declarative DSL (**ZMarkup** +
> **ZStyle**) for building interfaces without writing boilerplate.

**Status:** 🧪 *alpha* — the API is still evolving. Expect breaking
changes.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-alpha-orange.svg)](#status)

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
| **Backend** | Pluggable. Reference implementation ships for **Raylib**. The core only depends on `IRenderer` / `IPlatform` / `IAssetProvider`. |
| **Debugging** | Optional `Debug::dumpStyle` / `dumpTree` / `dumpTheme` / `dumpParts`, structured `Logger` with source locations |
| **Testing** | Headless `MockRenderer` + `MockPlatform` allow full interaction tests without opening a window |

---

## Quick start

```cpp
#include <raylib.h>
#include "RaylibBackend.hpp"
#include "UI.hpp"

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

The project is built with a plain **Makefile**. It expects:

- A C++20 compiler (`g++` by default).
- **Raylib** unpacked in `deps/raylib/`, with:
  - headers in `deps/raylib/include/`
  - libraries in `deps/raylib/lib/`

### Common targets

```bash
make          # builds ./bin/testui (the demo app)
make test     # builds and runs the headless test suite
make clean    # removes build artifacts
make all      # main + test
```

### Portability note

The current Makefile targets **MinGW / GCC on Windows** (it links
`-lopengl32 -lgdi32 -lwinmm`). On Linux and macOS, the same targets work
if Raylib is present and the link flags are adjusted for the platform
(`-lGL -lm -lpthread -ldl -lrt -lX11` on Linux, no extra flags on
macOS).

The **test suite is fully portable** — it doesn't link Raylib at all.
On Linux and macOS, `make test` works out of the box.

A CMake build for full cross-platform support (including MSVC) is
planned — see [docs/ROADMAP.md](docs/ROADMAP.md).

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

[`Layout`](docs/api/Layout.md) ·
[`Widgets`](docs/api/Widgets.md) ·
[`Unit`](docs/api/Unit.md) ·
[`Style`](docs/api/Style.md) ·
[`Theme`](docs/api/Theme.md) ·
[`ZMarkup`](docs/api/ZMarkup.md) ·
[`ZStyle`](docs/api/ZStyle.md) ·
[`FilterRegistry`](docs/api/FilterRegistry.md) ·
[`UIContext`](docs/api/UIContext.md)

---

## Repository layout

```
src/
  pch.hpp                     Precompiled header (Raylib app only)
  main.cpp                    Demo entry point
  ui/
    Layout.hpp                Core node type
    Style.hpp                 Style / ComputedStyle
    Theme.hpp                 Rules, keyframes, specificity
    StyleResolver.hpp         Cascade + transitions
    StyleParser.hpp           .zstyle parser
    StyleAttr.hpp             Attribute value parsers
    AnimationPlayer.hpp       Imperative + CSS animation
    AnimPrimitives.hpp        Keyframe types, evaluators
    Easing.hpp                TransitionFunction
    InputController.hpp       Input FSM
    ScrollController.hpp      Scroll orchestration
    ScrollState.hpp           Scroll state (POD)
    FilterRegistry.hpp        Filter registration
    UIContext.hpp             The three backend interfaces + context
    Media.hpp                 Media queries
    CoreTypes.hpp             Vec2, Rect, Color, ...
    Common.hpp                Standard includes
    Logger.hpp                Structured logging
    Debug.hpp                 Debug dumps
    UI.hpp                    Aggregator header
    UIComponents.hpp          Widget aggregator
    UIEnums.hpp               LayoutType, UIState
    Unit.hpp                  Value / Unit
    ZMarkup.hpp               DSL parser + builder
    components/               Widgets (header-only)
      Text.hpp  Button.hpp  Panel.hpp  ImageContainer.hpp
      Toggle.hpp  Checkbox.hpp  Slider.hpp  ProgressBar.hpp
      TextInput.hpp  ScrollView.hpp  Dropdown.hpp
      Canvas.hpp  Modal.hpp  Tooltip.hpp  Popup.hpp
      Layouts.hpp
    layout/                   Core implementation
      Layout.cpp
      Measure.cpp
      Render.cpp
      StyleResolver.cpp
      AnimationPlayer.cpp
      Input.cpp
      ScrollController.cpp
      FilterRegistry.cpp
  backend/
    RaylibBackend.hpp
    RaylibBackend.cpp
deps/
  raylib/                     Vendored Raylib (include/, lib/)
tests/
  TestFramework.hpp
  Mocks.hpp
  test_main.cpp
  test_*.cpp
docs/
  README.md  CHEATSHEET.md  GLOSSARY.md  ROADMAP.md
  user/      internals/     api/
assets/                       .zstyle files, fonts, textures, shaders
```

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Status and roadmap

ZenitUI is in **alpha**. The core architecture is stable, but the API
is still evolving — breaking changes are expected between releases.

See [docs/ROADMAP.md](docs/ROADMAP.md) for the detailed plan from
alpha to beta to 1.0, including known issues and planned features.

---

## License

[MIT](LICENSE) © 2026 zLouis043