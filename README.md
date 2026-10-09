# ZenitUI

> A retained-mode UI framework for C++ with a CSS-like styling system,
> a Flexbox-inspired layout engine, and a declarative DSL (**ZMarkup** +
> **ZStyle**) for building interfaces without writing boilerplate.

**Status:** 🧪 *alpha* — the API is still evolving. Expect breaking
changes.

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/license-TBD-lightgrey.svg)](#license)
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
include/
  Common.hpp            CoreTypes.hpp       Unit.hpp
  Layout.hpp            Style.hpp           Theme.hpp
  StyleResolver.hpp     StyleParser.hpp     StyleAttr.hpp
  AnimationPlayer.hpp   AnimPrimitives.hpp  Easing.hpp
  InputController.hpp   ScrollController.hpp ScrollState.hpp
  FilterRegistry.hpp    UIContext.hpp       Media.hpp
  UI.hpp                UIComponents.hpp    UIEnums.hpp
  Logger.hpp            Debug.hpp
  ZMarkup.hpp
  components/           Button, Toggle, Slider, Checkbox, TextInput,
                        ProgressBar, Dropdown, ScrollView, Modal,
                        Tooltip, Popup, Canvas, Text, ImageContainer
src/
  Layout.cpp            Measure.cpp         Render.cpp
  StyleResolver.cpp     AnimationPlayer.cpp Input.cpp
  ScrollController.cpp  FilterRegistry.cpp
backends/
  raylib/               RaylibBackend.{hpp,cpp}
tests/
  Mocks.hpp             test_*.cpp
docs/
  README.md  CHEATSHEET.md  GLOSSARY.md  ROADMAP.md
  user/      internals/    api/
```

---

## Building

*TODO — describe the build system here (CMake / Bazel / manual).*

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Status and roadmap

ZenitUI is in **alpha**. The core architecture is stable, but the API
is still evolving — breaking changes are expected between releases.

See [docs/ROADMAP.md](docs/ROADMAP.md) for the detailed plan from alpha
to beta to 1.0, including known issues and planned features.

---

## License

*TODO — see [LICENSE](LICENSE).*