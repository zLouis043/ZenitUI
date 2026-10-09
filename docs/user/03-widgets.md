# Widgets

All built-in widgets live in `namespace ZenitUI::UI`. Every one of them is
a subclass of `Layout`, so they all share the common API (see §2) and can
be styled with `.zstyle` exactly like a plain `Layout`.

If you know a widget from CSS or another UI framework, most of the naming
will feel familiar. Where it doesn't, this document explains the
differences.

---

## 1. Overview

| Widget | C++ class | Factory helper | ZMarkup tag | Interactive |
|--------|-----------|----------------|-------------|-------------|
| Text / Label | `Text` | `Label(text)` | `Text`, `Label` | no |
| Button | `Button` | `Btn(text, cb)` | `Button` | yes |
| Panel | `Panel` | `Pan()` | `Panel` | no (blocks) |
| Image | `ImageContainer` | — | — | no |
| Toggle | `Toggle` | — | `Toggle` | yes |
| Checkbox | `Checkbox` | — | `Checkbox` | yes |
| Slider | `Slider` | — | `Slider` | yes |
| Progress bar | `ProgressBar` | — | `ProgressBar` | no |
| Text input | `TextInput` | — | `TextInput` | yes |
| Scroll view | `ScrollView` | — | `ScrollView` | yes |
| Dropdown | `Dropdown` | — | `Dropdown` | yes |
| Canvas (render target) | `CanvasLayout` | — | — | — |
| Modal | `Modal` | — | — | yes |
| Tooltip | `Tooltip` | `Tooltip::attach(...)` | — | no |
| Popup / Context menu | `Popup` | `ContextMenu(items)` | — | yes |
| VStack / HStack | *(helpers)* | `VStack()`, `HStack()` | `VStack`, `HStack` | — |

All widgets except helpers are created via their `::create(...)` factory,
**not** via `std::make_shared`. The factory calls `onBuild()` after
construction, which is where composite widgets (Toggle, Dropdown, …)
attach their internal children.

```cpp
auto t = Toggle::create(true);      // ✅ right
auto t = std::make_shared<Toggle>(); // ❌ onBuild never runs
```

---

## 2. Common API

Every widget inherits from `Layout` and therefore has:

### Fluent helpers

```cpp
widget->cls("my-class");            // add a CSS class
widget->id("my-id");                // set the node id
widget->size(VW(10), VH(5));        // set width/height
widget->passThrough(true);          // click passes through to parent
```

### Inline style

```cpp
widget->getInlineBase().background = Colors::Blue;
widget->getInlineBase().radius     = Px(8);
```

`getInlineBase()` returns a mutable `Style&`. Inline styles have the
highest priority in the cascade (they beat any `.zstyle` rule).

### Tree

```cpp
parent->addChild(widget);
widget->removeFromParent();
widget->children;                   // std::vector<std::shared_ptr<Layout>>
```

### State

```cpp
widget->setEnabled(false);
widget->setInteractive(true);
widget->setFocusable(true);
widget->setKeyboardActivates(true);
widget->setChecked(true);           // for Toggle, Checkbox
```

### Callbacks

All widgets support:

| Callback | Signature | Fires when |
|----------|-----------|------------|
| `onClick` | `void()` | Press + release on the same node. |
| `onPress` | `void()` | Pointer down on the node. |
| `onRelease` | `void()` | Pointer up on the node (regardless of press). |
| `onHoverEnter` | `void()` | Pointer enters the node. |
| `onHoverExit` | `void()` | Pointer leaves the node. |
| `onRightClick` | `void()` | Right-click on the node. |

See [Events](06-events.md) for bubbling and pointer-capture details.

### ZMarkup attributes

Every ZMarkup element accepts the same inline style attributes:

```
Button "OK" width="100px" height="40px" background="#FF0000" radius="8px"
```

For the full attribute list, see [ZStyle](05-zstyle.md) §"Inline attributes".

---

## 3. Text / Label

Plain text with wrapping, alignment, and font selection.

### C++

```cpp
auto t = Label("Hello, world!");
t->cls("normal-text");

auto wrapped = Label("A long paragraph that will wrap on multiple lines.");
wrapped->setWrap(true);
wrapped->size(Percent(100), Auto());

auto rich = Text::create("Replaced later");
rich->setText("New content");
rich->setFont(assets.getFont("mont"));
```

### ZMarkup

```
Text "Hello, world"
Label.normal-text "Hello, world"
Text wrap "A long paragraph that will wrap..."
```

In ZMarkup, the string literal after the tag is the text content. `wrap`
can be a bare flag (defaults to `true`) or `wrap=false` / `wrap=true`.

### API

| Method | Description |
|--------|-------------|
| `setText(std::string)` | Replace the text. |
| `setFont(FontHandle)` | Override the resolved font. |
| `setWrap(bool)` | Enable / disable wrapping. |
| `getWrap()` | Query the current setting. |

### Styling

```css
Text {
    color: #FFFFFF;
    font-size: 3vh;
    letter-spacing: 2px;
    font: calibri;
    text-align: center;   /* start | center | end */
}
```

Text is not interactive. It has no pseudo-states.

### Notes

- Intrinsic size is the measured text box. With `wrap`, it depends on the
  available width — the parent must supply a bounded width, otherwise the
  text will never wrap.
- `text-align` only affects the horizontal position of each line inside
  the node's rect. To center the **node itself**, use the parent's
  `items-h: center`.

---

## 4. Button

A clickable box with a text child.

### C++

```cpp
auto b = Btn("Click me", []{
    std::printf("clicked\n");
});
b->cls("btn-primary");

// Full control — no auto text child
auto raw = Button::create([]{
    std::printf("clicked\n");
});
raw->addChild(Label("Custom")->cls("btn-text"));
```

### ZMarkup

```
Button.btn-primary#go "Click me"
```

The ZMarkup factory uses `Btn(text)`, so a `Text` child with class
`btn-text` is added automatically.

### API

`Button` itself only adds:

- `setInteractive(true)`
- `setFocusable(true)`
- `setKeyboardActivates(true)`
- default `items-h: center`, `items-v: center`
- style tag `"Button"`

Everything else (background, padding, radius, hover effects) comes from
`.zstyle`.

The callback is set through the constructor:

```cpp
auto b = Button::create([]{ /* ... */ });
```

or, after building from ZMarkup, through the `UINode` handle:

```cpp
ui.onClick("go", []{ /* ... */ });
```

### Styling

```css
Button {
    radius: 8px;
    padding: 1.5vh 2vw;
    transition: background 0.25s ease-out, scale 0.12s ease-out;
}
Button:hover   { background: #3296FF; scale: 1.05; }
Button:pressed { background: #505050; scale: 0.95; }
Button:focus   { border-color: #FDF900; border-width: 2px; }
Button:disabled{ opacity: 0.4; }

.btn-primary { background: #0079F1; }
.btn-danger  { background: #BE2137; }
```

`Button` is the style tag. `.btn-primary` / `.btn-danger` are classes,
typically applied together.

---

## 5. Panel

A plain box. No content, no interaction, just a rectangle you can style.

### C++

```cpp
auto p = Pan();
p->cls("card");
p->addChild(Label("Inside the panel"));
```

### ZMarkup

```
Panel.card {
    Text "Inside the panel"
}
```

`Panel` sets `setInteractive(false)` and `setBlocksRaycast(true)` — it
won't receive clicks itself, but it will stop them from reaching things
behind it.

### Styling

```css
Panel {
    background: #232328FF;
    radius: 8px;
}
.card {
    padding: 3vh;
    gap: 2vh;
    border-width: 1px;
    border-color: #3A3A40;
}
```

`Panel` is a `Stack` — children are overlaid, not flowed. For a flowed
layout, use `VStack` / `HStack` or set `display: vertical | horizontal`
in `.zstyle` (there's no `display` property in ZenitUI — see the note
below).

> **Note:** ZenitUI's `LayoutType` is set at construction, not through a
> style property. In `.zstyle` you cannot change `Stack` → `Vertical`.
> Use the right container from the start, or use `VStack()` / `HStack()`
> directly.

---

## 6. ImageContainer

A node that draws a texture scaled to fill its rect.

### C++

```cpp
auto img = ImageContainer::create(assets.getTexture("bubble"));
img->size(Px(200), Px(200));
```

### API

| Method | Description |
|--------|-------------|
| `setTexture(TextureHandle)` | Change the texture at runtime. |

### Styling

The texture is stretched to fill the rect. `tint` colors the texture:

```css
ImageContainer {
    tint: #FF0000FF;    /* multiplies the texture by red */
}
```

There is no `::part` and no interaction.

---

## 7. Toggle

An on/off switch. Renders a track (the node itself) and a knob (a child
positioned by `.zstyle`).

### C++

```cpp
auto t = Toggle::create(false);
t->onToggle = [](bool checked){
    std::printf("toggle: %d\n", checked);
};
t->setChecked(true);   // programmatic
```

### ZMarkup

```
Toggle#t checked=true
```

```cpp
ui.onToggle("t", [](bool v){ /* ... */ });
```

### API

| Method / field | Description |
|----------------|-------------|
| `isChecked` | Public `bool`, the current state. |
| `onToggle` | `std::function<void(bool)>`, fires on user click. |
| `setChecked(bool)` | Programmatic state change (from `Layout`). |
| `getChecked()` | Current state (from `Layout`). |

`Toggle::create(state)` sets the initial `isChecked`.

### Internal structure

`Toggle::onBuild()` creates one child:

- a `Layout` with class `toggle-knob`.

Its geometry (position, size, radius) is set entirely through `.zstyle`.
The `.zstyle` **must** set `position: absolute` on the knob and use
`Toggle:checked .toggle-knob` to move it.

```css
Toggle {
    width: 10vh;
    height: 5vh;
    radius: 50ph;
    background: #505050;
    transition: background 0.15s linear;
}
Toggle:checked { background: #00E430; }

Toggle .toggle-knob {
    position: absolute;
    width:  60ph;
    height: 60ph;
    top:    20ph;
    left:   20ph;
    radius: 50%;
    background: #FFFFFF;
    transition: left 0.28s ease-out-cubic;
}
Toggle:checked .toggle-knob {
    left: calc(100pw - 80ph);
}
```

The `:checked` state is a real CSS state — see [Events](06-events.md) §"State flags".

---

## 8. Checkbox

A small box with a checkmark drawn directly (no child nodes).

### C++

```cpp
auto c = Checkbox::create(false);
c->onToggle = [](bool v){ /* ... */ };
```

### ZMarkup

```
Checkbox#c checked=true
```

### API

Same as `Toggle`: `isChecked`, `onToggle`, `setChecked`.

### Intrinsic size

`font-size × 1.4`. Override with `width` / `height` if you want a
different size.

### Styling

Checkbox draws its box and mark using the `::part` mechanism:

```css
Checkbox {
    background: #505050;   /* the box background */
    radius: 6px;
    transition: background 0.15s linear;
}
Checkbox:hover  { background: #606060; }
Checkbox:checked { background: #1574b0; }

Checkbox::box  { background: #1574b0; radius: 6px; }
Checkbox::mark { color: #FFFFFF; }
Checkbox:disabled::box  { background: #3A3A40; }
Checkbox:disabled::mark { color: #808080; }
```

`::box` is the outer rounded rect. `::mark` is the small centered square
that appears when checked. Both are painted by `renderContent`, not by
children.

---

## 9. Slider

A horizontal track with a draggable knob. Value is a `float` in `[0, 1]`.

### C++

```cpp
auto s = std::make_shared<Slider>(0.5f);
s->onValueChanged = [](float v){
    std::printf("value = %.2f\n", v);
};
```

Note: `Slider` is constructed directly (`std::make_shared<Slider>(...)`),
not via `::create`. There's no `onBuild` to run.

Actually, you can also use `Slider::create(0.5f)` if you prefer
consistency — it works because `TLayout` generates both.

### ZMarkup

```
Slider#s value=0.65 width="55%"
```

```cpp
ui.onValueChanged("s", [](float v){ /* ... */ });
```

### API

| Method / field | Description |
|----------------|-------------|
| `getValue()` | Current value in `[0, 1]`. |
| `onValueChanged` | `std::function<void(float)>`. |

There is **no** `setValue()` — the value is only changed by user input
(drag or keyboard). To set it programmatically, use
`getInlineBase()`… no, that doesn't work either. If you need to set the
value from code, keep a reference to the `Slider` and add a setter in
your own code, or wrap it.

> **Note:** the value is stored in a private field of `Slider`. There is
> currently no public API to set it programmatically after construction.
> The most common pattern is to build a small wrapper widget that owns
> both the `Slider` and your own state.

### Interaction

- **Drag**: click anywhere on the track and drag. The knob follows the
  pointer, and pointer capture keeps events flowing even if the cursor
  leaves the node.
- **Keyboard**: `Left` / `Right` move by `0.05`. With `Shift` held, the
  step becomes `0.01`.

### Styling

```css
Slider {
    color: #3296FF;   /* used by fill and knob if no override */
}
Slider::track { background: #1717cf; radius: 4px; }
Slider::fill  { color: #d6ff32; }
Slider::knob  { color: #d6ff32; }
Slider:hover::knob { animation: knobGlow 0.6s ease-in-out infinite; }
Slider:pressed::knob { animation: knobPop 0.2s ease-out; }
```

- `::track` is the background bar.
- `::fill` is the portion from the left to the current value.
- `::knob` is the circular handle.

All three are painted by `renderContent`. `::part` rules support state
pseudo-classes (`:hover`, `:pressed`, `:checked`, `:focus`, `:disabled`)
— see [ZStyle](05-zstyle.md) §"::part".

---

## 10. ProgressBar

A non-interactive bar showing a value in `[0, 1]`.

### C++

```cpp
auto pb = std::make_shared<ProgressBar>(0.3f);
pb->setValue(0.7f);
```

### ZMarkup

```
ProgressBar#pb value=0.3 width="55%"
```

### API

| Method | Description |
|--------|-------------|
| `setValue(float)` | Change the value (clamped to `[0, 1]`). |
| `getValue()` | Current value. |

### Intrinsic size

`120 × max(16, font-size × 0.8)`.

### Styling

```css
ProgressBar {
    background: #2A2A32;
    color: #69ff32;   /* fill color */
    radius: 6px;
}
ProgressBar::track { background: #cc1c9d; }
ProgressBar::fill  { color: #69FF32; }
```

`::track` is the background bar. `::fill` is the filled portion.

---

## 11. TextInput

A single-line editable text field with a cursor, horizontal scrolling,
and keyboard repeat.

### C++

```cpp
auto input = std::make_shared<TextInput>("Initial text");
input->onTextChanged = [](const std::string& s){
    std::printf("now: %s\n", s.c_str());
};
input->onSubmit = [](const std::string& s){
    std::printf("submitted: %s\n", s.c_str());
};
```

### ZMarkup

```
TextInput#name value="Player1" width="55%"
```

```cpp
ui.onClick("name", ...);   // not needed — click focuses automatically
```

The `value` attribute sets the initial text.

### API

| Method / field | Description |
|----------------|-------------|
| `setText(std::string)` | Replace the content. |
| `getText()` | Current content. |
| `clear()` | Empty the input. |
| `setFont(FontHandle)` | Override the resolved font. |
| `onTextChanged` | `std::function<void(const std::string&)>`, fires on any edit. |
| `onSubmit` | `std::function<void(const std::string&)>`, fires on `Enter`. |

### Interaction

- Click → focuses the node.
- Typing → inserts characters at the cursor. Only printable ASCII
  (`0x20`–`0x7E`) is accepted by default.
- `Backspace` / `Delete` — delete around the cursor, with key repeat.
- `Left` / `Right` — move the cursor, with key repeat.
- `Home` / `End` — jump to start / end.
- `Enter` — fires `onSubmit`, then releases focus.
- `Escape` — releases focus without submitting.

Key repeat uses a 0.40s initial delay and 0.04s interval.

### Styling

```css
TextInput {
    background: #1A1A20;
    color: #FFFFFF;
    radius: 6px;
    padding: 1.2vh 1.5vw;
    border-color: #505060;
    border-width: 2px;
    font-size: 3vh;
    transition: border-color 0.2s ease-out;
}
TextInput:focus { border-color: #FDF900; }

TextInput::cursor {
    color: #FF6600;
    border-width: 3px;
}
TextInput:hover::cursor {
    color: #FDF900;
    border-width: 10px;
}
```

`::cursor` is the blinking caret. Its `border-width` becomes the caret
thickness; its `color` becomes the caret color. Blink period is 1s
(half-visible, half-hidden).

---

## 12. ScrollView

A `Layout` with `overflow: scroll` on both axes by default. Its actual
behavior (clip, scrollbar, inertia, wheel) lives in `Layout` itself; the
`ScrollView` class is just a convenience.

### C++

```cpp
auto sv = ScrollView::create(LayoutType::Vertical);
sv->addChild(Label("..."));
sv->addChild(Label("..."));
// many more children...
```

### ZMarkup

```
ScrollView#sv {
    Text "..."
    Text "..."
    // ...
}
```

### API

`ScrollView` adds no new methods beyond `Layout`. All scroll APIs are on
`Layout`:

| Method | Description |
|--------|-------------|
| `scrollTo(x, y)` | Absolute scroll position. |
| `scrollToTop()` / `scrollToBottom()` | Vertical. |
| `scrollToLeft()` / `scrollToRight()` | Horizontal. |
| `getScrollX()` / `getScrollY()` | Current offset. |
| `getMaxScrollX()` / `getMaxScrollY()` | Maximum offset. |
| `isScrollContainer()` | True if overflow is not `visible` on either axis. |

### Styling

```css
ScrollView {
    overflow: scroll;
    background: #1A1A20;
    radius: 8px;
    border-color: #505060;
    border-width: 2px;
    padding: 8px;
}
```

Set `overflow-x: hidden` / `overflow-y: auto` to disable one axis or make
it appear only when needed. See [Layout §8 Overflow](02-layout.md).

### Notes

- The container's intrinsic size is `{0, 0}` — you must give it an
  explicit size (or a `grow` / `shrink` context) so it can actually
  scroll. A `ScrollView` with `height: auto` will never need to scroll.
- Children with `position: absolute` inside a `ScrollView` are **not**
  scrolled (they're removed from flow before the scroll offset is
  applied). Use a normal child if you want it to move with the content.

---

## 13. Dropdown

A button that opens a scrollable list of options. The list is a
**portal** — it renders above everything else, ignoring clipping and
z-index of the surrounding tree.

### C++

```cpp
auto dd = Dropdown::create(
    std::vector<std::string>{"Easy", "Normal", "Hard"},
    1   // default selection index
);

dd->onChange = [](int idx, const std::string& name){
    std::printf("picked %d: %s\n", idx, name.c_str());
};
```

### ZMarkup

```
Dropdown#diff options="Facile,Normale,Difficile"
```

```cpp
ui.find<Dropdown>("diff")->onChange = [](int i, const std::string& n){ /* ... */ };
```

### API

| Method / field | Description |
|----------------|-------------|
| `getSelected()` | Current index. |
| `setSelected(int)` | Programmatically change selection. Fires `onChange`. |
| `getOpen()` / `setOpen(bool)` | Control the open state. |
| `onChange` | `std::function<void(int, const std::string&)>`. |

### Internal structure

`Dropdown::onBuild()` creates:

1. A `Button` with class `dropdown-trigger` — the closed-state face.
2. A `ScrollView` with `position: absolute` and `setPortal(true)` — the
   list of options. Each option is a `Label` with `padding` and an
   `onClick` that sets the selection.

The list's position is computed every frame in `arrange()`:

- Default: below the trigger, offset by `4px`.
- If there isn't enough space below, it flips above.
- If it doesn't fit on either side, it's shrunk to the largest available
  space.

Closing on outside click is handled in `onUpdate`. Clicking anywhere
that isn't the trigger or the list closes the dropdown.

### Styling

The trigger's visual properties live in `inlineDefaults`, so a
`.zstyle` rule can override them freely:

```css
.dropdown-trigger {
    height: 48px;                       /* default: 3vh */
    background: #232328;
    radius: 6px;
    padding: 8px 14px;
    border-width: 1px;
    border-color: #505060;
    font-size: 16px;
}
.dropdown-trigger .btn-text {
    font-size: 16px;
}

Dropdown { background: #232328; }   /* affects the trigger face */
```

Only `width: 100%` is baked into the trigger's `inlineBase` — it's
what makes the trigger fill its logical parent. To constrain the
dropdown's overall width, wrap it in a fixed-size container:

```cpp
auto wrapper = std::make_shared<Layout>(LayoutType::Stack);
wrapper->size(Px(180), Auto());
wrapper->addChild(dropdown);
parent->addChild(wrapper);
```

This is the recommended pattern when placing a dropdown inside an
`HStack` or any other container that would otherwise let the trigger
stretch to the full row width.

Individual options inherit whatever styling you apply to `Label` inside
the list. The list itself (`ScrollView`) can be targeted via
`Dropdown ScrollView { ... }`.

### Notes

- The list is a portal, so it is **not** clipped by ancestor
  `overflow: hidden`. This is the entire reason portals exist.
- `setOpen(false)` when the dropdown is disabled — the implementation
  closes it automatically in `onEnabledChanged`.
- The trigger's `height`, `background`, `radius`, `border-*`,
  `items-h`, and `items-v` are all overridable via `.zstyle`. Only
  `width: 100%` is fixed.
- If the trigger's text is invisible, check the trigger's padding
  against its height. If `padding-top + padding-bottom` is larger than
  the trigger's content box, the text gets clipped to zero height.

---

## 14. Canvas (render target)

A `CanvasLayout` renders its subtree into an offscreen render target,
then draws that target as a texture. Useful for effects that need to be
applied to a subtree as a whole, or for caching complex subtrees.

### C++

```cpp
auto canvas = std::make_shared<CanvasLayout>();
canvas->size(Px(400), Px(300));
canvas->addChild(someHeavySubtree);

// Optional: apply a shader to the whole canvas
canvas->setShader(assets.getEffect("hueShift"));
```

### Notes

- The render target is **recreated** whenever the node's `rect` size
  changes.
- The target is destroyed in the destructor.
- `supports(Feature::Effects)` is required for the shader to be applied;
  the Raylib backend supports it.
- `CanvasLayout` is `Stack` type.

There is no ZMarkup tag for `Canvas` by default. You can register one
yourself via `ZMarkup::Registry::reg("Canvas", ...)`.

---

## 15. Modal

A full-screen overlay with a centered content panel and an intro
animation.

### C++

```cpp
auto modal = Modal::create();
modal->children[0]->addChild(Label("Are you sure?"));
modal->addChild(...) → don't. Use the content VStack inside.

// Actually: children[0] is the content VStack with class "modal-content"
auto content = modal->children[0];
content->addChild(Label("Are you sure?"));
content->addChild(Btn("OK", [modal]{ modal->hide(); }));

parent->addChild(modal);
modal->show();
```

### API

| Method | Description |
|--------|-------------|
| `show()` | Set opacity to 1 and play the intro animation. |
| `hide()` | Set opacity to 0 and play the intro in reverse. |

### Structure

`Modal::onBuild()` creates:

- A full-screen `Stack` with tag `"Modal"`, black background at 0.6
  opacity, `setFocusScope(true)`.
- One child: a `VStack` with class `modal-content`. This is where you
  put your content.

The intro animation (`"Intro"`) translates the content from `VH(100)`
(bottom) to `0` using `ease-out-back`.

### Styling

```css
Modal { background: #000000CC; }
.modal-content {
    width: 60vw;
    padding: 4vh;
    gap: 2vh;
    background: #1E1E24;
    radius: 12px;
}
```

`Modal` is a focus scope — pressing `Tab` inside it cycles through its
focusable descendants only, not the whole tree. See
[Events](06-events.md) §"Focus scopes".

---

## 16. Tooltip

A small floating box shown when hovering its owner, after a delay.

### C++ — attach to an owner

```cpp
auto btn = Btn("Save", []{});
Tooltip::attach(btn, "Save your changes", 0.4f);
```

`attach(owner, text, showDelay)`:

1. Creates the tooltip.
2. Sets `owner` as its anchor.
3. Hooks into `owner->onHoverEnter` / `onHoverExit` to start / stop the
   show timer.
4. Adds the tooltip as a **child of owner** (for lifetime) but renders
   it as a portal.

### Manual control

```cpp
auto tip = Tooltip::create("Hello");
tip->setAnchor(someNode);
tip->setShowDelay(0.5f);
someNode->addChild(tip);

tip->show();   // starts the delay timer
tip->hide();   // hides immediately
```

### API

| Method | Description |
|--------|-------------|
| `attach(owner, text, delay)` | Static. Convenience. |
| `setAnchor(Layout*)` | Node the tooltip is positioned relative to. |
| `setShowDelay(float)` | Seconds before the tooltip appears. |
| `show()` / `hide()` | Manual control. |

### Positioning

The tooltip is centered above its anchor, offset by `6px` vertically. If
the anchor has not been measured yet, the tooltip is not positioned.

### Styling

```css
Tooltip {
    background: #000000E6;
    color: #FFFFFF;
    padding: 0.6vh 0.8vw;
    radius: 4px;
    border-color: #606070;
    border-width: 1px;
    transition: opacity 0.2s ease-out;
}
```

Tooltips are `Panel` subclasses with `setPortal(true)` and
`setBlocksRaycast(false)`. They don't capture input.

---

## 17. Popup / ContextMenu

A floating panel that can be opened below, above, or at an arbitrary
screen position. Used for context menus, dropdown menus, and tooltips
that need more interactivity than `Tooltip`.

### C++ — as a menu

```cpp
auto menu = ContextMenu({
    { "Copy",  []{ /* ... */ } },
    { "Paste", []{ /* ... */ } },
    { "Delete",[]{ /* ... */ } },
});

someNode->addChild(menu);   // lifetime
someNode->onClick      = [menu, someNode]{ menu->openBelow(someNode); };
someNode->onRightClick = [menu, someNode]{ menu->openBelow(someNode); };
```

`ContextMenu(items)` returns a `Popup` with a `VStack` content and one
`Button` per item, each with class `popup-item`. Clicking an item fires
its callback and closes the popup automatically.

### C++ — custom content

```cpp
auto popup = Popup::create();
auto content = VStack();
content->addChild(Label("Custom popup"));
content->addChild(Btn("Close", [popup]{ popup->close(); }));
popup->setContent(content);

parent->addChild(popup);
popup->openAt({100, 100});
```

### API

| Method | Description |
|--------|-------------|
| `setContent(shared_ptr<Layout>)` | Set (and add) the content node. |
| `openBelow(anchor)` | Open below a node. |
| `openAbove(anchor)` | Open above a node. |
| `openAt(Vec2 screenPos)` | Open at an absolute screen position. |
| `close()` | Hide the popup. |
| `isOpen()` | Current state. |
| `closeOnClickOutside` | `bool`, default `true`. |
| `closeOnEscape` | `bool`, default `true`. |
| `offsetBelow` | `float`, default `4.0f`. |
| `onClose` | `std::function<void()>`. |

### Positioning and behavior

On every frame while open, the popup:

1. Computes its size from the content's measured size + its own padding.
2. Computes its `x` / `y` based on the anchor or `pos_`.
3. Flips above if it would overflow the viewport.
4. Clamps to the viewport edges.
5. Calls `arrangeInto()` with the final rect.

It closes on:

- Left click outside (if `closeOnClickOutside`).
- Right click outside (same).
- `Escape` (if `closeOnEscape`).
- An item click (if built via `ContextMenu`).

### Styling

```css
Popup {
    background: #1E1E24F2;
    border-color: #505060;
    border-width: 1px;
    radius: 6px;
    padding: 6px;
    transition: opacity 0.12s ease-out;
}
.popup-item {
    background: transparent;
    color: #FFFFFF;
    radius: 4px;
    padding: 1vh 1.2vw;
    transition: background 0.1s linear;
}
.popup-item:hover   { background: #3296FF60; }
.popup-item:pressed { background: #3296FFB0; }
```

`Popup` is a `Panel` subclass with `setPortal(true)`. Its opacity is
animated by a state transition; the content can have its own animations.

---

## 18. VStack / HStack

Thin helpers that create a `Layout` of the right type and set the style
tag. They're not classes.

### C++

```cpp
auto col = VStack({ Label("a"), Label("b"), Label("c") });
auto row = HStack({ Btn("A"), Btn("B") });

// Empty, fill later
auto col = VStack();
col->addChild(...);
```

### ZMarkup

```
VStack {
    Text "a"
    Text "b"
}
HStack gap="8px" {
    Text "x"
    Text "y"
}
```

### Styling

`VStack` and `HStack` set the style tag to `"VStack"` / `"HStack"`, so
you can target them:

```css
VStack { gap: 2vh; }
HStack { gap: 1vw; items-v: center; }
```

The underlying class is `Layout`; the tag is only for styling and
debugging.

---

## 19. Widget composition

The factories `Btn`, `Label`, `VStack`, `HStack` return the widget
directly. For composite widgets (`Toggle`, `Dropdown`, `Modal`,
`Tooltip`, `Popup`), use `::create()` so that `onBuild()` runs.

If you write a custom widget with children, follow the same pattern:

```cpp
class MyWidget : public TLayout<MyWidget> {
public:
    MyWidget(int arg) : TLayout<MyWidget>(LayoutType::Vertical), arg(arg) {}

protected:
    void onBuild() override {
        addChild(Label("built"));
    }
    int arg;
};

auto w = MyWidget::create(42);   // ✅ onBuild called
```

See [Custom Widgets](10-custom-widgets.md) for a full walkthrough.