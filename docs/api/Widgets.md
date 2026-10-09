# Widgets — API Reference

Compact reference for every built-in widget in `namespace ZenitUI::UI`.
For usage examples and tutorials, see
[Widgets (guide)](../user/03-widgets.md).

All widgets derive from `Layout` (directly or via
`TLayout<Derived, Base>`). They share the entire `Layout` API —
only the additions are listed here.

---

## Overview

| Widget | Header | Base | Factory | ZMarkup tag | Interactive |
|--------|--------|------|---------|-------------|-------------|
| `Text` | `Text.hpp` | `TLayout<Text>` | `Label(text)` | `Text`, `Label` | no |
| `Button` | `Button.hpp` | `TLayout<Button, Panel>` | `Btn(text, cb)` | `Button` | yes |
| `Panel` | `Layouts.hpp` | `TLayout<Panel>` | `Pan()` | `Panel` | no |
| `ImageContainer` | `ImageContainer.hpp` | `TLayout<ImageContainer>` | — | — | no |
| `Toggle` | `Toggle.hpp` | `TLayout<Toggle>` | — | `Toggle` | yes |
| `Checkbox` | `Checkbox.hpp` | `TLayout<Checkbox>` | — | `Checkbox` | yes |
| `Slider` | `Slider.hpp` | `TLayout<Slider>` | — | `Slider` | yes |
| `ProgressBar` | `ProgressBar.hpp` | `TLayout<ProgressBar>` | — | `ProgressBar` | no |
| `TextInput` | `TextInput.hpp` | `TLayout<TextInput>` | — | `TextInput` | yes |
| `ScrollView` | `ScrollView.hpp` | `TLayout<ScrollView>` | — | `ScrollView` | yes |
| `Dropdown` | `Dropdown.hpp` | `TLayout<Dropdown>` | — | `Dropdown` | yes |
| `CanvasLayout` | `Canvas.hpp` | `TLayout<CanvasLayout>` | — | — | — |
| `Modal` | `Modal.hpp` | `TLayout<Modal>` | — | — | yes |
| `Tooltip` | `Tooltip.hpp` | `TLayout<Tooltip, Panel>` | `Tooltip::attach(...)` | — | no |
| `Popup` | `Popup.hpp` | `TLayout<Popup, Panel>` | `ContextMenu(items)` | — | yes |

---

## Layout helpers

Not widgets, but factory functions returning `Layout` instances with
specific types and style tags.

### `VStack(children = {})`

```cpp
std::shared_ptr<Layout> VStack(
    std::initializer_list<std::shared_ptr<Layout>> children = {});
```

Creates a `Layout(LayoutType::Vertical)` with style tag `"VStack"`.

### `HStack(children = {})`

```cpp
std::shared_ptr<Layout> HStack(
    std::initializer_list<std::shared_ptr<Layout>> children = {});
```

Creates a `Layout(LayoutType::Horizontal)` with style tag `"HStack"`.

---

## `Panel`

Plain box. Non-interactive, blocks raycast.

```cpp
class Panel : public TLayout<Panel> {
public:
    Panel();
};

std::shared_ptr<Panel> Pan();
```

- **Style tag**: `"Panel"`
- **Layout type**: `Stack`
- **Flags**: `setInteractive(false)`, `setBlocksRaycast(true)`
- **ZMarkup**: `Panel { ... }`

---

## `Text`

Text widget with optional wrapping and font selection.

```cpp
class Text : public TLayout<Text> {
public:
    Text(std::string text);

    void setText(std::string new_text);
    void setFont(FontHandle f);
    void setWrap(bool w);
    bool getWrap() const;
};

std::shared_ptr<Text> Label(std::string t);
```

### Methods

| Method | Effect |
|--------|--------|
| `setText(std::string)` | Replace content. Sets `text_dirty` and `pendingTransition`. |
| `setFont(FontHandle)` | Override the resolved font. |
| `setWrap(bool)` | Enable / disable word wrapping. |
| `getWrap()` | Current wrapping state. |

### Style tag

`"Text"`

### Flags

- `setInteractive(false)`

### ZMarkup

```
Text "Hello"
Label.normal-text "Hello"
Text wrap "Long text that wraps"
```

---

## `Button`

Interactive box with optional text child.

```cpp
class Button : public TLayout<Button, Panel> {
public:
    Button(std::function<void()> cb);
};

std::shared_ptr<Button> Btn(std::string text,
                            std::function<void()> cb = nullptr);
```

### Constructor

The `Button(std::function<void()> cb)` constructor sets:

- `onClick = cb`
- `setInteractive(true)`
- `setFocusable(true)`
- `setKeyboardActivates(true)`
- `setStyleTag("Button")`
- `itemsH = Center`, `itemsV = Center`

### Factory helpers

| Helper | Behavior |
|--------|----------|
| `Btn(text)` | Creates a `Button` with no callback, adds a `Text` child with class `"btn-text"`. |
| `Btn(text, cb)` | Same, with a callback. |
| `Button::create(cb)` | Full control — no text child added. |

### Style tag

`"Button"`

### ZMarkup

```
Button.btn-primary#go "Click me"
```

---

## `ImageContainer`

Non-interactive node drawing a texture.

```cpp
class ImageContainer : public TLayout<ImageContainer> {
public:
    ImageContainer(TextureHandle tex);

    void setTexture(TextureHandle t);
};
```

### Methods

| Method | Effect |
|--------|--------|
| `setTexture(TextureHandle)` | Change the texture. |

### Rendering

Draws the texture stretched to the node's rect, tinted by the style's
`tint` property.

### Style tag

`"ImageContainer"`

### Flags

- `setInteractive(false)`

---

## `Toggle`

On/off switch with a knob child.

```cpp
class Toggle : public TLayout<Toggle> {
public:
    Toggle(bool state = false);

    std::function<void(bool)> onToggle = nullptr;
    bool isChecked;
};
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| `isChecked` | `bool` | Current state. Kept in sync with `getChecked()`. |
| `onToggle` | `std::function<void(bool)>` | Fires when the user clicks. |

### Behavior

- `onBuild()` creates a child `Layout` with class `"toggle-knob"`.
- Clicking the toggle flips `isChecked` and fires `onToggle`.
- `setChecked(bool)` (from `Layout`) updates the state and triggers
  a style re-resolve.

### Style tag

`"Toggle"`

### Flags

- `setInteractive(true)`
- `setFocusable(true)`
- `setKeyboardActivates(true)`

### Structure

The knob is a `Layout(Stack)` with `interactive: false`,
`blocksRaycast: false`, and class `toggle-knob`. Its position must be
set entirely from `.zstyle`.

### ZMarkup

```
Toggle#t checked=true
```

Binding:

```cpp
ui.onToggle("t", [](bool v) { /* ... */ });
```

---

## `Checkbox`

Checkbox with a custom-rendered mark (no children).

```cpp
class Checkbox : public TLayout<Checkbox> {
public:
    Checkbox(bool state = false);

    std::function<void(bool)> onToggle = nullptr;
    bool isChecked;
};
```

### Fields

Same as `Toggle`: `isChecked`, `onToggle`.

### Intrinsic size

`font-size × 1.4` (square).

### Style tag

`"Checkbox"`

### Parts

| Part | Description |
|------|-------------|
| `::box` | The outer rounded rect (background color overrides). |
| `::mark` | The checkmark square (color). |

### Flags

- `setInteractive(true)`
- `setFocusable(true)`
- `setKeyboardActivates(true)`

### ZMarkup

```
Checkbox#c checked=true
```

---

## `Slider`

Horizontal slider with a value in `[0, 1]`.

```cpp
class Slider : public TLayout<Slider> {
public:
    Slider(float val = 0.5f);

    std::function<void(float)> onValueChanged = nullptr;

    float getValue() const;
};
```

### Methods / fields

| Name | Type | Description |
|------|------|-------------|
| `getValue()` | `float` | Current value in `[0, 1]`. |
| `onValueChanged` | `std::function<void(float)>` | Fires during drag or on keyboard step. |

**Note:** there is no public `setValue()` — the value is only changed
by user input.

### Intrinsic size

`120 × max(24, font-size × 1.2)`.

### Interaction

- **Drag**: click and drag anywhere on the track. Uses pointer capture.
- **Keyboard**: `Left` / `Right` move by `0.05`; with `Shift`, by `0.01`.

### Style tag

`"Slider"`

### Parts

| Part | Description |
|------|-------------|
| `::track` | Background bar (`background`). |
| `::fill` | Filled portion (`color`). |
| `::knob` | Circular handle (`color`, `radius`). |

### Flags

- `setInteractive(true)`
- `setFocusable(true)`
- `setKeyboardActivates(false)` — keyboard handled in `onUpdate`.

### ZMarkup

```
Slider#s value=0.65 width="55%"
```

---

## `ProgressBar`

Non-interactive bar showing a value in `[0, 1]`.

```cpp
class ProgressBar : public TLayout<ProgressBar> {
public:
    ProgressBar(float v = 0.0f);

    void setValue(float v);
    float getValue() const;
};
```

### Methods

| Method | Description |
|--------|-------------|
| `setValue(float)` | Change value (clamped to `[0, 1]`). |
| `getValue()` | Current value. |

### Intrinsic size

`120 × max(16, font-size × 0.8)`.

### Style tag

`"ProgressBar"`

### Parts

| Part | Description |
|------|-------------|
| `::track` | Background bar (`background`). |
| `::fill` | Filled portion (`color`). |

### Flags

- `setInteractive(false)`

### ZMarkup

```
ProgressBar#pb value=0.3 width="55%"
```

---

## `TextInput`

Single-line editable text field.

```cpp
class TextInput : public TLayout<TextInput> {
public:
    TextInput(std::string initial = "");

    void setText(std::string t);
    const std::string& getText() const;
    void clear();
    void setFont(FontHandle f);

    std::function<void(const std::string&)> onTextChanged;
    std::function<void(const std::string&)> onSubmit;
};
```

### Methods / fields

| Name | Description |
|------|-------------|
| `setText(std::string)` | Replace content. |
| `getText()` | Current content. |
| `clear()` | Equivalent to `setText("")`. |
| `setFont(FontHandle)` | Override the resolved font. |
| `onTextChanged` | Fires on any edit. |
| `onSubmit` | Fires on `Enter` (before losing focus). |

### Keyboard behavior

| Key | Action |
|-----|--------|
| Printable ASCII | Insert at cursor. |
| `Backspace` / `Delete` | Delete, with repeat (0.40s initial, 0.04s interval). |
| `Left` / `Right` | Move cursor, with repeat. |
| `Home` / `End` | Jump to start / end. |
| `Enter` | Fire `onSubmit`, release focus. |
| `Escape` | Release focus without submitting. |

### Intrinsic size

`max(120, char-width × 12) × font-size × 1.6`.

### Style tag

`"TextInput"`

### Parts

| Part | Description |
|------|-------------|
| `::cursor` | Blinking caret. `color` = caret color, `border-width` = caret thickness. Blink period 1s. |

### Flags

- `setInteractive(true)`
- `setFocusable(true)`
- `setKeyboardActivates(false)` — `Enter` / `Escape` handled in
  `onUpdate`.
- `setUpdateWhenDisabled(false)` (default).

### ZMarkup

```
TextInput#name value="Player1" width="55%"
```

---

## `ScrollView`

Wrapper that sets `overflow: scroll` on both axes and appropriate
flags.

```cpp
class ScrollView : public TLayout<ScrollView> {
public:
    explicit ScrollView(LayoutType t = LayoutType::Vertical);
};
```

### Constructor

- Sets `setInteractive(true)`, `setBlocksRaycast(true)`.
- Sets style tag `"ScrollView"`.
- Sets `overflowX` and `overflowY` to `Scroll` in inline defaults.

### Style tag

`"ScrollView"`

### Scroll API

All inherited from `Layout`:

- `scrollTo`, `scrollToTop`, `scrollToBottom`, `scrollToLeft`,
  `scrollToRight`, `scrollToX`, `scrollToY`.
- `getScrollX`, `getScrollY`, `getMaxScrollX`, `getMaxScrollY`.
- `isScrollContainer`.

### ZMarkup

```
ScrollView#sv {
    Text "..."
    Text "..."
}
```

---

## `Dropdown`

Select-from-list widget with a portal-rendered option list.

```cpp
class Dropdown : public TLayout<Dropdown> {
public:
    Dropdown(std::vector<std::string> options, int selected = 0);

    int  getSelected() const;
    void setSelected(int idx);
    bool getOpen() const;
    void setOpen(bool o);

    std::function<void(int, const std::string&)> onChange;
};
```

### Methods / fields

| Name | Description |
|------|-------------|
| `getSelected()` | Current index. |
| `setSelected(int)` | Programmatic selection. Fires `onChange`. |
| `getOpen()` / `setOpen(bool)` | Open state control. |
| `onChange` | Fires when selection changes (user or programmatic). |

### Internal structure

`onBuild()` creates:

- A `Button` with class `dropdown-trigger` (the closed face).
- A `ScrollView` with `position: absolute` and `setPortal(true)` —
  the option list.

The list flips above or below the trigger based on available space,
and is shrunk if it fits in neither.

### Styling

The trigger's visual properties live in `inlineDefaults`, so any
`.zstyle` rule targeting `.dropdown-trigger` overrides them:

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
```

Only `width: 100%` (on the trigger) is baked into `inlineBase`,
because it's what makes the trigger fill its logical parent. To
constrain the dropdown's overall width, wrap it in a fixed-size
container:

```cpp
auto wrapper = std::make_shared<Layout>(LayoutType::Stack);
wrapper->size(Px(180), Auto());
wrapper->addChild(dropdown);
```

This is the recommended pattern when the dropdown lives inside an
`HStack` or any container that would otherwise stretch the trigger
to the full row width.

Individual options inherit whatever styling is applied to `Label`
inside the list. The list itself (`ScrollView`) can be targeted via
`Dropdown ScrollView { ... }`.

### Style tag

`"Dropdown"`

### Closing

- Outside click (left or right).
- An option click.
- On disable (via `onEnabledChanged`).

### ZMarkup

```
Dropdown#d options="A,B,C"
```

---

## `CanvasLayout`

Renders its subtree into an offscreen render target.

```cpp
class CanvasLayout : public TLayout<CanvasLayout> {
public:
    CanvasLayout();
    ~CanvasLayout();
};
```

### Behavior

- `LayoutType::Stack`, style tag `"Canvas"`.
- `setInteractive(true)`.
- On each draw: if the rect size changed, recreates the render target.
- Renders the subtree into the target, then draws the target as a
  texture.
- Applies a custom shader (via `setShader`) if `supports(Effects)`.

### Notes

- The target is destroyed in the destructor.
- Requires `supports(Feature::Effects)` for shader application.
- No ZMarkup tag by default. Register one via `ZMarkup::Registry`.

---

## `Modal`

Full-screen overlay with a centered content panel and intro animation.

```cpp
class Modal : public TLayout<Modal> {
public:
    Modal();

    void show();
    void hide();
};
```

### Methods

| Method | Description |
|--------|-------------|
| `show()` | Set opacity to 1, play `"Intro"` forward. |
| `hide()` | Set opacity to 0, play `"Intro"` in reverse. |

### Structure

`onBuild()` creates:

- A full-screen `Stack` with tag `"Modal"`, background black 0.6,
  opacity 0.
- One child: a `VStack` with class `modal-content`.

The `"Intro"` animation translates the content from `VH(100)` to `0`
with `ease-out-back`, over 0.4s.

### Flags

- `setInteractive(false)` (in `onBuild`; `show()` sets it to `true`).
- `setFocusScope(true)`.

### Content access

The content is `children[0]`:

```cpp
modal->children[0]->addChild(Label("Are you sure?"));
```

### Style tag

`"Modal"`

---

## `Tooltip`

Hover-triggered floating label.

```cpp
class Tooltip : public TLayout<Tooltip, Panel> {
public:
    Tooltip(std::string text);

    static std::shared_ptr<Tooltip> attach(
        std::shared_ptr<Layout> owner,
        std::string text,
        float showDelay = 0.4f);

    void setAnchor(Layout* a);
    void setShowDelay(float s);
    void show();
    void hide();
};
```

### Methods

| Method | Description |
|--------|-------------|
| `attach(owner, text, delay)` | Static helper: creates the tooltip, hooks into `owner`'s hover callbacks, adds it as a child. |
| `setAnchor(Layout*)` | Manually set the anchor node. |
| `setShowDelay(float)` | Delay in seconds before showing. |
| `show()` | Start the timer. |
| `hide()` | Hide immediately. |

### Positioning

Centered above the anchor, offset by `6px` vertically. If the anchor
isn't measured yet, the tooltip isn't positioned.

### Style tag

`"Tooltip"`

### Flags

- `setPortal(true)`
- `setInteractive(false)`
- `setBlocksRaycast(false)`

---

## `Popup`

Generic floating panel. Supports `openBelow` / `openAbove` / `openAt`.

```cpp
class Popup : public TLayout<Popup, Panel> {
public:
    Popup();

    void setContent(std::shared_ptr<Layout> content);
    void openBelow(std::shared_ptr<Layout> anchor);
    void openAbove(std::shared_ptr<Layout> anchor);
    void openAt(Vec2 screenPos);
    void close();
    bool isOpen() const;

    bool closeOnClickOutside{true};
    bool closeOnEscape{true};
    float offsetBelow{4.0f};
    std::function<void()> onClose;

    void arrange(Rect) override;   // no-op
};
```

### Methods / fields

| Name | Description |
|------|-------------|
| `setContent(content)` | Sets (and adds) the content node. |
| `openBelow(anchor)` | Open below an anchor node. Flips above if no space. |
| `openAbove(anchor)` | Open above. |
| `openAt(pos)` | Open at an absolute screen position. |
| `close()` | Hide. Fires `onClose`. |
| `isOpen()` | Current state. |
| `closeOnClickOutside` | Close on outside click. Default `true`. |
| `closeOnEscape` | Close on `Escape`. Default `true`. |
| `offsetBelow` | Distance from anchor. Default `4.0f`. |
| `onClose` | Fires on close. |

### Behavior

On each frame while open:

1. Compute size from content + padding.
2. Compute position from anchor or `pos_`.
3. Flip above if it would overflow.
4. Clamp to viewport.
5. Call `arrangeInto()` with the final rect.

### Style tag

`"Popup"`

### Flags

- `setPortal(true)`
- `setInteractive(true)`
- `setFocusable(true)`
- `setBlocksRaycast(true)`

### Factory helper — `ContextMenu`

```cpp
std::shared_ptr<Popup> ContextMenu(
    std::vector<std::pair<std::string, std::function<void()>>> items);
```

Creates a `Popup` with a vertical `VStack` content and one `Button`
per item. Each button has class `popup-item` and, when clicked, fires
its callback and closes the popup.

---

## Common patterns

### Building a widget

```cpp
auto btn = Btn("OK", []{ /* ... */ });
btn->cls("btn-primary");
root->addChild(btn);
```

### From ZMarkup

```cpp
auto ui = ZMarkup::build(R"(
    VStack {
        Text "Hello"
        Button.btn-primary#ok "OK"
    }
)");
ui.onClick("ok", []{ /* ... */ });
root->addChild(ui.root());
```

### Binding callbacks after build

```cpp
ui.onClick("ok", []{ /* ... */ });
ui.onToggle("mute", [](bool v){ /* ... */ });
ui.onValueChanged("volume", [](float v){ /* ... */ });
```

### Accessing a widget by type

```cpp
auto slider = ui.find<Slider>("volume");
slider->onValueChanged = [](float v){ /* ... */ };
```

---

## See also

- [Widgets (guide)](../user/03-widgets.md) — tutorials and patterns.
- [Layout](Layout.md) — base node API.
- [Custom Widgets](../user/10-custom-widgets.md) — writing your own.
- [Events](../user/06-events.md) — click, hover, focus semantics.