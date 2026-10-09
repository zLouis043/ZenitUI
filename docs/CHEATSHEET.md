# Cheatsheet

One-page reference for day-to-day work with ZenitUI. Dense, table-first,
no prose. For explanations, see the [user guide](user/01-getting-started.md).

---

## 1. Frame loop

```cpp
// Setup (once)
RaylibRenderer renderer;
RaylibPlatform platform;
RaylibAssetProvider assets(renderer);

UIContext::get().renderer = &renderer;
UIContext::get().platform = &platform;
UIContext::get().assets   = &assets;

assets.loadFont("calibri", "assets/calibri.ttf");
ZMarkup::loadStyleFile("assets/game.zstyle");

auto root = std::make_shared<Layout>(LayoutType::Stack);
root->size(Percent(100), Percent(100));

// Loop
while (!WindowShouldClose()) {
    root->runFrame(GetFrameTime());   // logic

    BeginDrawing();
    ClearBackground(BLACK);
    root->renderFrame();              // draw
    EndDrawing();
}
```

---

## 2. Units

| Unit | C++ | `.zstyle` | Resolves against |
|------|-----|-----------|------------------|
| Auto | `Auto()` | `auto` | — |
| Pixel | `Px(10)` | `10px` | 1:1 |
| Percent | `Percent(50)` | `50%` | parent, along axis |
| Viewport W | `VW(10)` | `10vw` | `Metrics::viewport.x` |
| Viewport H | `VH(10)` | `10vh` | `Metrics::viewport.y` |
| Parent W | `PW(50)` | `50pw` | parent width (any axis) |
| Parent H | `PH(50)` | `50ph` | parent height (any axis) |
| Number | `Num(2)` | `2` | used in `calc()` |

`calc()`: `+`, `-`, `*`, `/`, `()` with mixed units.

```css
width: calc(100% - 20px);
left:  calc((100% - 40px) / 2);
```

---

## 3. Layout types & alignment

| Type | C++ | ZMarkup | Main axis |
|------|-----|---------|-----------|
| Stack | `LayoutType::Stack` | `Stack` | none (overlay) |
| Vertical | `LayoutType::Vertical` | `VStack` | vertical |
| Horizontal | `LayoutType::Horizontal` | `HStack` | horizontal |

### Alignment properties

| Property | Where | Axis | Values |
|----------|-------|------|--------|
| `justify` | container | main | `start` `center` `end` `space-between` |
| `items-h` | container | cross H | `start` `center` `end` `stretch` |
| `items-v` | container | cross V | same |
| `align-h` | child | cross H | overrides `items-h` |
| `align-v` | child | cross V | overrides `items-v` |
| `text-align` | text | inline | `start` `center` `end` |

### Quick centering

```css
.parent { items-h: center; items-v: center; }
```

---

## 4. Layout properties

| C++ field | `.zstyle` | Type | Default |
|-----------|-----------|------|---------|
| `width` / `height` | `width` / `height` | Value | `auto` |
| `minWidth` / `minHeight` | `min-width` / `min-height` | Value | `auto` |
| `maxWidth` / `maxHeight` | `max-width` / `max-height` | Value | `auto` |
| `grow` / `shrink` | `grow` / `shrink` | float | `0` / `1` |
| `gap` | `gap` | Value | `0` |
| `padding` | `padding` | Spacing | `0` |
| `margin` | `margin` | Spacing | `0` |
| `alignH` / `alignV` | `align-h` / `align-v` | Align | `auto` |
| `itemsH` / `itemsV` | `items-h` / `items-v` | Align | `start` |
| `justify` | `justify` | Justify | `start` |
| `position` | `position` | enum | `static` |
| `top` / `left` / `right` / `bottom` | same | Value | `auto` |
| `zIndex` | `z-index` | int/auto | `auto` |
| `overflowX` / `overflowY` | `overflow-x` / `overflow-y` | Overflow | `visible` |

Spacing shorthand (CSS-style): `padding: a` / `a b` / `a b c` / `a b c d`.

Overflow values: `visible`, `hidden`, `scroll`, `auto`.

---

## 5. Visual properties

| C++ field | `.zstyle` | Type |
|-----------|-----------|------|
| `background` | `background` | Color |
| `backgroundTexture` | `background-texture` | `name [l t r b]` |
| `tint` | `tint` | Color |
| `borderColor` | `border-color` | Color |
| `borderWidth` | `border-width` | Value |
| `radius` | `radius` | Value |
| `opacity` | `opacity` | float |
| `boxShadow` | `box-shadow` | `x y blur color` |
| `scale` | `scale` | float |
| `rotation` | `rotation` | degrees |
| `translateX` / `translateY` | `translate-x` / `translate-y` | Value (self) |

### Typography (inherited)

| C++ field | `.zstyle` | Type | Default |
|-----------|-----------|------|---------|
| `font` | `font` | string | `""` |
| `fontSize` | `font-size` | Value | `20` |
| `letterSpacing` | `letter-spacing` | Value | `2` |
| `color` | `color` | Color | `#FFFFFF` |
| `textAlign` | `text-align` | Align | `auto` |

### Colors

Named: `white`, `black`, `transparent`, `blank`, `red`, `maroon`,
`green`, `blue`, `yellow`, `gray`, `darkgray`, `lightgray`.

Hex: `#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA`.

---

## 6. Selectors (`.zstyle`)

| Syntax | Matches |
|--------|---------|
| `Button` | style tag `Button` |
| `.btn-primary` | class `btn-primary` |
| `#save` | id `save` |
| `Button.btn-primary` | tag + class |
| `Button#save` | tag + id |
| `.card Button` | descendant |
| `.card .title` | descendant (class) |
| `Button:hover` | state |
| `Button:pressed` | state |
| `Button:focus` | state |
| `Toggle:checked` | state |
| `Button:disabled` | state |
| `Toggle:checked:hover` | states combined |
| `Slider::track` | part |
| `Slider:hover::knob` | state + part |

Specificity: `(ids, classes, tags)`. States count as classes.

Inline styles (C++ `getInlineBase()` / ZMarkup attributes) win over
everything.

---

## 7. Easing functions

| Name | Behavior |
|------|----------|
| `linear` | constant rate |
| `ease-in-quad` | slow start, quadratic |
| `ease-out-quad` | fast start, quadratic |
| `ease-in-out-quad` | symmetric |
| `ease-in-cubic` / `ease-out-cubic` / `ease-in-out-cubic` | cubic variants |
| `ease-in-back` | overshoots at start |
| `ease-out-back` | overshoots at end |
| `ease-out-elastic` | elastic oscillation |
| `ease-out-bounce` | bounce at end |

Aliases: `ease` = `ease-in-out-quad`, `ease-in` = `ease-in-quad`,
`ease-out` = `ease-out-quad`, `ease-in-out` = `ease-in-out-quad`.

Parsing is lenient: `easeoutback`, `ease-out-back`, `EASE_OUT_BACK`
all work.

---

## 8. Transitions and animations (`.zstyle`)

```css
/* Transition */
.element {
    transition: background 0.25s ease-out-quad,
                scale      0.12s ease-out-back;
}

/* Keyframe */
@keyframes pulse {
    0%   { scale: 1.0; }
    50%  { scale: 1.2; }
    100% { scale: 1.0; }
}
.pulse { animation: pulse 1.2s ease-in-out infinite; }
```

`animation` tokens: `<name> <duration> [<easing>] [<iterations>|infinite]
[alternate] [forwards]`.

Longhands: `transition-delay`, `transition-duration`,
`transition-timing-function`, `animation-delay`,
`animation-duration`, `animation-timing-function`,
`animation-iteration-count`, `animation-direction`,
`animation-fill-mode`.

Requires the shorthand first.

---

## 9. Widgets — factories and callbacks

| Widget | Factory | Callback |
|--------|---------|----------|
| `Button` | `Btn("text", cb)` / `Button::create(cb)` | `onClick` |
| `Text` | `Label("text")` / `Text::create("text")` | — |
| `Panel` | `Pan()` | — |
| `Toggle` | `Toggle::create(bool)` | `onToggle(bool)` |
| `Checkbox` | `Checkbox::create(bool)` | `onToggle(bool)` |
| `Slider` | `Slider::create(float)` | `onValueChanged(float)` |
| `ProgressBar` | `ProgressBar::create(float)` | — |
| `TextInput` | `TextInput::create("initial")` | `onTextChanged`, `onSubmit` |
| `ScrollView` | `ScrollView::create(LayoutType)` | — |
| `Dropdown` | `Dropdown::create({"A","B"}, selected)` | `onChange(int, string)` |
| `ImageContainer` | `ImageContainer::create(tex)` | — |
| `Modal` | `Modal::create()` | — |
| `Tooltip` | `Tooltip::attach(owner, text, delay)` | — |
| `Popup` | `Popup::create()` / `ContextMenu({...})` | `onClose` |

### Common widget methods

| Method | Widgets |
|--------|---------|
| `setText(string)` / `getText()` | `Text`, `TextInput` |
| `setValue(float)` / `getValue()` | `Slider`, `ProgressBar` |
| `setChecked(bool)` / `getChecked()` | `Toggle`, `Checkbox` |
| `setSelected(int)` / `getSelected()` | `Dropdown` |
| `setWrap(bool)` | `Text` |
| `setOpen(bool)` / `getOpen()` | `Dropdown` |
| `openBelow/Above/At()` / `close()` | `Popup` |
| `show()` / `hide()` | `Modal`, `Tooltip` |

### `Layout` callbacks (all widgets)

`onClick`, `onPress`, `onRelease`, `onHoverEnter`, `onHoverExit`,
`onRightClick`, `onAnimationsFinished`.

---

## 10. ZMarkup syntax

```
Tag.class1.class2#id attr1="value" attr2=value flag "text content" {
    Child
    Child
}
```

### Built-in tags

`Stack`, `VStack`, `HStack`, `Spacer`, `Text`, `Label`, `Button`,
`Panel`, `Toggle`, `Checkbox`, `Slider`, `ProgressBar`, `TextInput`,
`ScrollView`, `Dropdown`.

### Reserved attributes

| Attribute | Effect |
|-----------|--------|
| `passthrough` | Sets `passThrough`. |
| `value` | Initial value for `TextInput`, `Slider`, `ProgressBar`. |
| `checked` | Initial state for `Toggle`, `Checkbox`. |
| `options` | Comma-separated list for `Dropdown`. |
| `wrap` | Enables wrapping on `Text`. |

Everything else is applied as an inline style.

### C++ — build and bind

```cpp
auto ui = ZMarkup::build(R"(
    VStack#root {
        Text "Hello"
        Button.btn-primary#ok "OK"
    }
)");

ui.onClick("ok", []{ /* ... */ })
  .onToggle("mute", [](bool v){ /* ... */ })
  .onValueChanged("volume", [](float v){ /* ... */ });

root->addChild(ui.root());
```

### Manual lookup

```cpp
auto node   = ui.find("ok");                  // shared_ptr<Layout>
auto slider = ui.find<Slider>("volume");      // typed
```

---

## 11. Loading stylesheets

```cpp
ZMarkup::loadStyleFile("assets/game.zstyle");
ZMarkup::loadStyleString(R"(
    Button { background: red; }
)");

Theme::get().clear();   // reset
```

Rules accumulate. Later rules win at equal specificity.

Custom properties in `:root`:

```css
:root {
    --primary: #0079F1;
    --radius: 8px;
}
.btn { background: var(--primary); radius: var(--radius); }
```

---

## 12. Filters and effects

### Built-in filters

| Filter | Syntax | Pattern |
|--------|--------|---------|
| `blur` | `blur(4px)` | Separable |
| `drop-shadow` | `drop-shadow(2px, 2px, #000)` | Silhouette |

```css
.card { filter: blur(4px), drop-shadow(2px, 2px, #000); }
.btn  { effect: hueShift; }
```

Requires the shader to be loaded:

```cpp
assets.loadEffect("blur", "assets/blur.fs");
assets.loadEffect("dropShadow", "assets/drop_shadow.fs");
```

### Custom filter

```cpp
FilterRegistry::get().add(FilterDef{
    .name    = "sepia",
    .shader  = "sepia",
    .pattern = FilterPattern::SinglePass,
    .params  = {{"amount", FilterParamType::Float, 0, 0.7f}}
});
```

---

## 13. Common patterns

### Full-viewport root

```cpp
auto root = std::make_shared<Layout>(LayoutType::Stack);
root->size(Percent(100), Percent(100));
```

### Sidebar + content

```cpp
auto row = HStack({ sidebar, content });
sidebar->size(VW(20), Percent(100));
sidebar->getInlineBase().shrink = 0;
content->getInlineBase().grow   = 1;
```

### Spacer that eats leftover space

```cpp
sidebar->addChild(std::make_shared<Layout>()->cls("spacer-grow"));
```

```css
.spacer-grow { grow: 1; }
```

### Centered modal

```css
.modal-overlay {
    width: 100%; height: 100%;
    items-h: center; items-v: center;
    background: #000000CC;
}
```

### Tooltip on hover

```cpp
Tooltip::attach(btn, "Save your progress", 0.3f);
```

### Right-click context menu

```cpp
auto menu = ContextMenu({
    { "Copy", []{ /* ... */ } },
    { "Paste", []{ /* ... */ } },
});
box->addChild(menu);
box->onRightClick = [box, menu]{
    menu->openAt(UIContext::get().pointer.pos);
};
```

### Scrollable list

```cpp
auto sv = ScrollView::create(LayoutType::Vertical);
sv->getInlineBase().height = VH(50);
for (int i = 0; i < 100; ++i)
    sv->addChild(Label("Item " + std::to_string(i)));
```

### Focus + keyboard activation

```cpp
auto btn = Btn("OK", []{ /* ... */ });
btn->setFocusable(true);
btn->setKeyboardActivates(true);   // Enter / Space fire onClick
```

### Hover scale (`.zstyle`)

```css
.btn-primary {
    background: #0079F1;
    transition: background 0.25s ease-out, scale 0.12s ease-out-back;
}
.btn-primary:hover {
    background: #3296FF;
    scale: 1.15;
}
```

---

## 14. Debug

Define `ZENITUI_DEBUG` before including `UI.hpp`:

```cpp
#define ZENITUI_DEBUG
#include "UI.hpp"

Debug::dumpStyle(*node, std::cout);
Debug::dumpTree(*root, std::cout);
Debug::dumpTheme(std::cout);
Debug::dumpParts(*slider, std::cout);
```

Log:

```cpp
logWarn("MyCategory", "", 0, 0, "message");
```

Install a custom logger:

```cpp
Logger::set(&myLogger);
```

---

## 15. Common pitfalls (quick list)

- Inline styles beat `:hover` — put visual properties in classes.
- `%` resolves against the parent, along the axis of the property.
- `translate-x: 50%` uses the **node's own width**.
- Filters don't chain — each reads the original target.
- `%` in a `Text` with `wrap` needs a bounded parent.
- Forgetting `::create()` on composite widgets skips `onBuild`.
- `overflow: hidden` on one axis promotes the other to `auto`.
- Missing assets fall back silently — check the logs.
- Portals are hit-tested one frame after they render.
- Missing `onClick` callbacks: check `setInteractive(true)`.

---

## See also

- [Getting Started](user/01-getting-started.md) — full walkthrough.
- [Layout](user/02-layout.md) — units, flex, alignment.
- [ZStyle](user/05-zstyle.md) — the full stylesheet language.
- [Widgets](user/03-widgets.md) — every widget with examples.
- [API reference](api/Layout.md) — full method signatures.