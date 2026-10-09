# Getting Started

This guide walks you through the minimum viable setup: initialize a backend,
create a root layout, add a widget, and run the frame loop.

By the end you will have a window with a styled, clickable button.

---

## 1. Prerequisites

- **C++17** or newer.
- A **backend**. ZenitUI ships a reference implementation for
  [Raylib](https://www.raylib.com/), but the core only depends on three
  abstract interfaces declared in `UIContext.hpp`:

  | Interface       | Responsibility                                                   |
  |-----------------|------------------------------------------------------------------|
  | `IRenderer`     | Primitive drawing, text measurement, clip / transform / target   |
  | `IPlatform`     | Window, pointer, keyboard, timers, DPI, safe area                |
  | `IAssetProvider`| Fonts, textures, shaders, keyed by string name                   |

  If you want to use a different graphics library, implement those three
  and you're done. Nothing else in the framework touches the GPU.

---

## 2. Initialize the backend

ZenitUI is a **library**, not a framework that owns `main()`. You initialize
the backend, hand it to `UIContext`, and drive the frame loop yourself.

```cpp
#include <raylib.h>
#include "RaylibBackend.hpp"
#include "UI.hpp"

using namespace ZenitUI;

int main() {
    InitWindow(800, 600, "ZenitUI");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    // 1. Backend instances
    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    // 2. Register them on the global context
    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    // 3. Load any asset you'll reference by name from .zstyle / ZMarkup
    assets.loadFont("calibri", "assets/calibri.ttf", 64);
    assets.loadTexture("bubble", "assets/bubble.png");
    assets.loadEffect("blur",    "assets/blur.fs");

    // ...
}
```

**Rule of thumb:** anything you want to reference by *name* (in a
`.zstyle` file or in a ZMarkup attribute) must be registered with the
`IAssetProvider` **before** the first frame. Fonts, textures, and shaders
are all referenced by string.

`UIContext` is a singleton. It owns:

- the three backend pointers,
- the current input snapshot (`pointer`, `inputEvents`, `shiftHeld`),
- the focus / pointer-capture / portal registries,
- the frame clock (`dt`, `time`).

You generally don't touch it directly after setup, except for advanced
cases (see [Events](06-events.md) and [Portals](08-portals.md)).

---

## 3. Create a root layout

The **root** is just a `Layout` that fills the viewport. Everything else is
a child of it.

```cpp
auto root = std::make_shared<Layout>(LayoutType::Stack);
root->size(Percent(100), Percent(100));
root->getInlineBase().itemsH = Align::Center;
root->getInlineBase().itemsV = Align::Center;
```

`LayoutType::Stack` lays out children on top of each other, positioned by
`itemsH` / `itemsV`. Use `LayoutType::Vertical` for a column and
`LayoutType::Horizontal` for a row. See [Layout](02-layout.md) for the full
list of container properties.

### Equivalent root in ZMarkup

```cpp
auto ui = ZMarkup::build(R"(
    Stack#root {
        // children go here
    }
)");
auto root = ui.root();
```

Then put the sizing / alignment into a `.zstyle` file:

```css
#root {
    width: 100%;
    height: 100%;
    items-h: center;
    items-v: center;
}
```

Both approaches are equivalent. Choose whichever is more comfortable —
you can mix them freely in the same project.

---

## 4. Add a widget

Widgets live in `namespace ZenitUI::UI` and are created via their
`::create()` factory (or a helper like `Btn` / `Label`).

```cpp
auto btn = Btn("Click me", []{
    std::printf("clicked!\n");
});
btn->cls("btn-primary");

root->addChild(btn);
```

`Btn(text, callback)` is a small helper that creates a `Button` **and** adds
a `Text` child with class `"btn-text"`. If you want full control, use
`Button::create(cb)` directly and add whatever child you want.

Every widget is a `Layout` subclass, so it inherits:

- `.cls("name")` / `.id("name")` / `.size(w, h)` fluent helpers,
- `getInlineBase()` for inline style overrides,
- `addChild()` / `removeFromParent()`,
- callback fields `onClick`, `onPress`, `onRelease`, `onHoverEnter`,
  `onHoverExit`, `onRightClick`.

### The same widget in ZMarkup

```cpp
auto ui = ZMarkup::build(R"(
    Stack#root {
        Button.btn-primary#go "Click me"
    }
)");
ui.onClick("go", []{ std::printf("clicked!\n"); });
```

You can also bind to `#root` if you want the click to fire when clicking
outside the button — see [Events](06-events.md) for bubbling rules.

### Styling the button

Without `.zstyle`, the button will use only the widget's built-in defaults.
To actually make it look like something, load a stylesheet:

```cpp
ZMarkup::loadStyleFile("assets/game.zstyle");
```

```css
/* assets/game.zstyle */
.btn-primary {
    background: #0079F1;
    radius: 8px;
    padding: 1.5vh 2vw;
    min-width: 80px;
    transition: background 0.25s ease-out-quad, scale 0.12s ease-out-back;
}
.btn-primary:hover   { background: #3296FF; scale: 1.15; }
.btn-primary:pressed { background: #505050; scale: 0.90; }
```

`loadStyleFile` appends rules to the global `Theme`. Calling it multiple
times is fine — rules accumulate, and later rules with the same specificity
win. See [ZStyle](05-zstyle.md) for the full language.

---

## 5. Run the frame loop

Each frame has **two phases**: a *logic* phase and a *draw* phase.

```cpp
while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // ---- logic phase (no GL calls inside) ----
    root->runFrame(dt);

    // ---- draw phase ----
    BeginDrawing();
    ClearBackground(BLACK);
    root->renderFrame();
    EndDrawing();
}
```

`runFrame(dt)` expands to:

```cpp
UIContext::get().beginFrame(dt);   // poll platform, refresh input, advance clock
root->updateTree(dt);              // state FSM, style resolve, animations, focus
root->measure(viewport.x, viewport.y);   // bottom-up intrinsic size
root->arrange({0, 0, viewport.x, viewport.y}); // top-down positioning
```

`renderFrame()` expands to:

```cpp
renderer->beginFrame();   // e.g. push DPI scale matrix
root->draw();
renderer->endFrame();
```

**Why the split?** Because `measure` / `arrange` don't touch the GPU, and
`draw` doesn't touch input. Keeping them apart makes headless tests
trivial (see [`Mocks.hpp`](../internals/10-debug.md)) and lets you run the
logic at a different rate from rendering if you ever need to.

---

## 6. The complete program

```cpp
#include <raylib.h>
#include "RaylibBackend.hpp"
#include "UI.hpp"
#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

int main() {
    InitWindow(800, 600, "ZenitUI");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    assets.loadFont("calibri", "assets/calibri.ttf", 64);
    ZMarkup::loadStyleFile("assets/game.zstyle");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().itemsH = Align::Center;
    root->getInlineBase().itemsV = Align::Center;

    auto btn = Btn("Click me", []{ std::printf("clicked!\n"); });
    btn->cls("btn-primary");
    root->addChild(btn);

    while (!WindowShouldClose()) {
        root->runFrame(GetFrameTime());

        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        root->renderFrame();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

You now have a running ZenitUI app with a styled button that reacts to
hover, press, and click — the rules in `game.zstyle` do the styling, the
`Btn(...)` helper does the rest.

---

## 7. Optional: keyboard focus

Add `setFocusable(true)` to a node if you want it to be reachable by
`Tab` and to react to `Enter` / `Space`:

```cpp
auto btn = Btn("Click me", []{ /* ... */ });
btn->setFocusable(true);
btn->setKeyboardActivates(true);   // Enter / Space fire onClick
```

Then press `Tab` at runtime — the button gains focus, `:focus` rules from
`.zstyle` apply, and pressing `Enter` fires the click callback. See
[Events](06-events.md) for the full focus / navigation model.

---

## 8. Next steps

- **[Layout](02-layout.md)** — `grow`, `shrink`, `gap`, `justify`, `items-*`, absolute positioning, units.
- **[Widgets](03-widgets.md)** — every built-in component with props and events.
- **[ZMarkup](04-zmarkup.md)** — build complex trees declaratively.
- **[ZStyle](05-zstyle.md)** — selector syntax, states, `::part`, `var()`, `calc()`, `@media`.
- **[Events](06-events.md)** — bubbling, pointer capture, focus, right-click, context menus.
- **[Animations](07-animations.md)** — transitions, `@keyframes`, imperative animations.
- **[Portals](08-portals.md)** — Popup, Tooltip, Dropdown, Modal.
- **[Effects](09-effects.md)** — filters, shaders, `FilterRegistry`.
- **[Custom Widgets](10-custom-widgets.md)** — build your own component.

---

## Common pitfalls

**"My widget doesn't appear."**
Check that (a) the parent has a non-zero size, (b) the widget has a
non-zero intrinsic size or an explicit `width` / `height`, and (c) you
called `runFrame` *before* `renderFrame` at least once — the first
`runFrame` is what computes `rect`.

**"My click callback never fires."**
The node must be `interactive` and reachable by hit-test. `Layout` defaults
to `interactive = true`, but custom widgets sometimes set it to `false`
(e.g. `Panel`, `Text`). Check `setBlocksRaycast` too — a node that only
*blocks* raycast won't receive `onClick`.

**"My style rules don't apply."**
- Selector specificity matters. `.card Button` beats `Button`.
- Inline styles (`getInlineBase()`) beat any rule.
- State rules (`:hover`, `:checked`) only match while the state flag is
  set on the node — see [Events](06-events.md).
- `:focus` requires `setFocusable(true)`.
- Media queries (`@media`) are evaluated against `Metrics::viewport`, not
  the OS window size — see [ZStyle](05-zstyle.md).

**"My text is not wrapping."**
Set `setWrap(true)` on the `Text` (or `wrap` in ZMarkup). Wrapping also
needs an available width — an auto-sized container gives the text an
infinite width, so it will never wrap. Give the parent a fixed or
percentage width.

**"My font name doesn't resolve."**
Font names in `.zstyle` (`font: calibri;`) and ZMarkup
(`font="calibri"`) must match a name registered with
`IAssetProvider::loadFont`. Names are case-sensitive.