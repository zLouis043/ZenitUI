# ZenitUI

> A retained-mode UI framework for games and tools, with a CSS-like styling
> system, a Flexbox-inspired layout engine, and an optional declarative DSL
> (**ZMarkup**) for building interfaces without writing C++ boilerplate.

**Status:** 🧪 *alpha* — the API is still evolving. Expect breaking changes.

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
| **Units**       | `px`, `%`, `vw`, `vh`, `pw`, `ph`, `auto`, full `calc()` support |
| **Styling**     | CSS-like cascade, descendant selectors, `:hover` `:pressed` `:focus` `:checked` `:disabled`, `::part`, custom properties (`--x` / `var()`), media queries |
| **Animations**  | Per-property transitions (`transition: opacity 0.3s ease-out`), CSS `@keyframes`, imperative `UIAnimation`, per-`::part` animations |
| **Input**       | Hover / pressed / focus state machine, `Tab` navigation, pointer capture, `passThrough`, right-click, context menus |
| **Effects**     | `filter: blur(...) drop-shadow(...)`, arbitrary shaders, extensible `FilterRegistry`, `box-shadow` |
| **Backend**     | Pluggable. Reference implementation ships for **Raylib**, but the core only depends on `IRenderer` / `IPlatform` / `IAssetProvider` |
| **Debugging**   | Optional `Debug::dumpStyle` / `dumpTree` / `dumpTheme` / `dumpParts`, structured `Logger` with source locations |
| **Testing**     | Headless `MockRenderer` + `MockPlatform` allow full interaction tests without opening a window |

---

## Quick start

```cpp
#include <raylib.h>
#include "RaylibBackend.hpp"
#include "UI.hpp"

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

---

## Repository layout (overview)

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
```

---

## License

*To be defined.*