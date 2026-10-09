# Custom Widgets

A widget is a class that derives from `Layout` (directly or through
`TLayout<Derived>`) and overrides a set of virtual hooks. Everything
the framework does — layout, styling, animations, input — is driven by
those hooks, so a custom widget only needs to implement the parts it
actually cares about.

This guide walks through building one from scratch, using the built-in
widgets as reference.

---

## 1. The `TLayout` CRTP helper

The simplest way to write a widget is to inherit from
`TLayout<Derived, Base>`:

```cpp
class MyWidget : public TLayout<MyWidget> {
public:
    MyWidget(int arg) : TLayout<MyWidget>(LayoutType::Stack), arg_(arg) {}

protected:
    void onBuild() override {
        // Called once, after construction, when shared_ptr is valid.
        // This is where you add children.
    }

private:
    int arg_;
};
```

`TLayout<Derived, Base = Layout>` provides:

- A `static create(args...)` factory that calls `make_shared<Derived>`
  and then `onBuild()`. **This is the only correct way to construct a
  widget that has children.**
- Fluent methods (`cls`, `id`, `size`, `with`, `passThrough`) that
  return `std::shared_ptr<Derived>` instead of `shared_ptr<Layout>`,
  so chaining preserves the concrete type.

The `Base` parameter lets you build on top of an existing widget:

```cpp
class Popup  : public TLayout<Popup, Panel>  { ... };
class Tooltip: public TLayout<Tooltip, Panel> { ... };
```

`Popup` inherits from `Panel` (which itself is a `TLayout<Panel>`), and
adds its own behavior. This is how the framework reuses the base
chrome rendering (background, border, radius) without duplicating it.

---

## 2. Construction: `create()` vs `make_shared`

The constructor of a widget runs **before** the `shared_ptr` exists.
Any code that calls `shared_from_this()` — which includes `addChild()`,
`cls()`, `size()`, `capturePointer()`, etc. — will throw or return an
invalid pointer inside the constructor.

For this reason, widgets with children must not build their tree in
the constructor. They must use the `create()` factory:

```cpp
// ✅ Correct — onBuild runs after the shared_ptr exists
auto t = Toggle::create(false);

// ❌ Wrong — onBuild never runs
auto t = std::make_shared<Toggle>(false);
```

`TLayout::create` is defined as:

```cpp
template <typename... Args>
static std::shared_ptr<Derived> create(Args &&...args) {
    auto p = std::make_shared<Derived>(std::forward<Args>(args)...);
    static_cast<TLayout<Derived, Base>*>(p.get())->onBuild();
    return p;
}
```

The cast to `TLayout<Derived, Base>*` is what makes `onBuild` virtual
across the hierarchy. It's fine because `Derived` is guaranteed to
inherit from that specialization.

### 2.1 Safety net

`Layout::addChild` refuses to run if `weak_from_this()` is expired. This
catches the mistake of building a tree in the constructor with a
warning instead of a crash:

```cpp
void addChild(std::shared_ptr<Layout> child) {
    if (!child) return;
    if (weak_from_this().expired()) {
        logWarn("Layout", "", 0, 0,
            "addChild called on a node not managed by shared_ptr. "
            "Use X::create() and put this logic in onBuild().");
        return;
    }
    // ...
}
```

The `Toggle::onBuild` example:

```cpp
protected:
    void onBuild() override {
        knob = std::make_shared<Layout>(LayoutType::Stack);
        knob->cls("toggle-knob");
        knob->setInteractive(false);
        knob->setBlocksRaycast(false);
        addChild(knob);
    }
```

`onBuild` runs after construction, so `addChild` works normally.

### 2.2 Widgets without children

If your widget has no children (like `Slider`, `ProgressBar`,
`Checkbox`), you can construct it with `make_shared` or `create`
indifferently. The framework's convention is still to use `create` for
consistency:

```cpp
auto s = Slider::create(0.5f);
auto p = ProgressBar::create(0.3f);
```

`Slider` and `ProgressBar` don't define `onBuild` — the base no-op runs
and nothing happens. They're "leaf" widgets that render themselves
entirely in `renderContent`.

---

## 3. The virtual hooks

The following virtual methods are the entire extension API. Override
only the ones you need.

### 3.1 `onBuild()`

```cpp
virtual void onBuild() {}
```

Called once, after construction, when the node is managed by a
`shared_ptr`. This is where you:

- Set the style tag: `setStyleTag("MyWidget")`.
- Configure the base state: `setInteractive(true)`, `setFocusable(true)`,
  `setKeyboardActivates(true)`.
- Add default inline styles: `getInlineBase().background = ...`.
- Add children.

Example from `Toggle`:

```cpp
void onBuild() override {
    setStyleTag("Toggle");
    setInteractive(true);
    setFocusable(true);
    setKeyboardActivates(true);

    setChecked(false);   // initial state

    onClick = [this] {
        isChecked = !isChecked;
        setChecked(isChecked);
        if (onToggle) onToggle(isChecked);
    };

    knob = std::make_shared<Layout>(LayoutType::Stack);
    knob->cls("toggle-knob");
    knob->setInteractive(false);
    knob->setBlocksRaycast(false);
    addChild(knob);
}
```

### 3.2 `computeIntrinsicSize(availW, availH)`

```cpp
virtual Vec2 computeIntrinsicSize(float availW, float availH) {
    return {0, 0};
}
```

Called during the measure phase, **before** children are measured. It
should return the widget's "natural" size, given the space it has
available. The default is `{0, 0}`, meaning "I have no intrinsic size,
just fit my children".

Examples:

```cpp
// Checkbox: a square based on font-size
Vec2 computeIntrinsicSize(float, float) override {
    float fs = style_.currentStyle.fontSize.resolveV(
        Metrics::viewport.x, Metrics::viewport.y);
    return {fs * 1.4f, fs * 1.4f};
}

// ProgressBar: 120 wide, at least 16 tall
Vec2 computeIntrinsicSize(float, float) override {
    float fs = style_.currentStyle.fontSize.resolveV(
        Metrics::viewport.x, Metrics::viewport.y);
    return {120.0f, std::max(16.0f, fs * 0.8f)};
}

// Text: measured by the renderer
Vec2 computeIntrinsicSize(float availW, float) override {
    FontHandle f = resolveFont(style_.currentStyle, defaultFont);
    if (wrap_ && availW > 0.0f)
        return measureWrapped(availW, f);
    return measureSingleLine(f);
}
```

The returned size is combined with the node's children and the
container's layout rules. For a `Stack`, it's the max of the two. For
`Vertical` / `Horizontal`, it's added to the child sizes along the main
axis.

**Important:** this method must not call `addChild` or mutate the
tree. It's called during measure, and the tree is considered stable at
that point.

### 3.3 `renderContent(op, style)`

```cpp
virtual void renderContent(float op, const ComputedStyle& style) {}
```

Called during the draw phase, after `renderChrome` (background, border)
and before the children are drawn. This is where the widget draws its
own visual content — text, tracks, marks, cursors, anything that's not
a child node.

```cpp
void renderContent(float op, const ComputedStyle& style) override {
    auto r = UIContext::get().renderer;

    // Draw a filled bar
    Color fillColor = style.color.withAlpha(op);
    Rect fill = {rect.x, rect.y, rect.width * value, rect.height};
    r->fillRoundedRect(fill, 4.0f, fillColor);
}
```

The `op` parameter is the node's computed opacity multiplied by all
ancestor opacities. Always apply it to any color you draw:

```cpp
r->fillRect(someRect, myColor.withAlpha(op));
```

If you forget `withAlpha(op)`, the node's own opacity and any ancestor
opacity will be ignored.

The `style` parameter is the node's `ComputedStyle` — the result of the
cascade plus the current CSS animation overlay. Use it to read colors,
sizes, and other properties:

```cpp
Color c = style.color;             // resolved color
float fs = style.fontSize.resolveV(...); // resolved font size in pixels
```

### 3.4 `renderChrome(op, style)`

```cpp
virtual void renderChrome(float op, const ComputedStyle& style);
```

Called before `renderContent`. The base implementation draws:

1. `box-shadow` (if any).
2. `background` color.
3. `background-texture` (with `tint`).
4. `border`.

Most widgets **don't override this**. Only override if you need to
customize how the background is drawn (e.g. a widget with a custom
background texture that shouldn't be affected by the `background-texture`
property).

`Panel` and its subclasses use the default `renderChrome`. `Button`
uses the default. `Slider` uses the default — the track and knob are
drawn in `renderContent`, on top of the background.

### 3.5 `onUpdate(dt)`

```cpp
virtual void onUpdate(float dt) {}
```

Called once per frame during the update phase, after the state machine
has run. Use it for:

- Custom input handling (drag, multi-touch, gestures).
- Local logic that runs every frame.
- Reading input events that aren't covered by the standard callbacks.

`Slider` uses it for drag:

```cpp
void onUpdate(float) override {
    if (isFocused) {
        auto& ev = UIContext::get().inputEvents;
        constexpr float STEP = 0.05f;
        // ... read keys, update value ...
    }

    if (isPressed && !hasPointerCapture())
        capturePointer();

    if (hasPointerCapture() && rect.width > 0.0f) {
        float px = UIContext::get().pointer.pos.x;
        float percent = std::clamp((px - rect.x) / rect.width, 0.0f, 1.0f);
        if (percent != value) {
            value = percent;
            if (onValueChanged) onValueChanged(value);
        }
    }
}
```

`TextInput` uses it for keyboard input, cursor blinking, and horizontal
scroll:

```cpp
void onUpdate(float dt) override {
    hasFocusCache = UIContext::get().hasFocus(this);
    cursorBlink += dt;
    if (cursorBlink > 1.0f) cursorBlink -= 1.0f;
    if (!hasFocusCache) { repeatStates.clear(); return; }
    // ... process chars, keys, repeat ...
}
```

**When is `onUpdate` called?** The base `Layout::update` only calls it
if `isEnabled || updateWhenDisabled_`. If your widget needs to update
while disabled, call `setUpdateWhenDisabled(true)`.

**When is `onUpdate` NOT called?** During a `blocksInput` animation, or
if the node is disabled and `updateWhenDisabled_` is false.

### 3.6 `onLayout()`

```cpp
virtual void onLayout() {}
```

Called after `arrange` has set the node's `rect` and its children have
been arranged. Use it for things that depend on the **final** size:

```cpp
void onLayout() override {
    if (!wrap_) return;

    float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height);
    float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height);
    float newAvailW = std::max(0.0f, rect.width - pl - pr);

    if (newAvailW > 0.0f && newAvailW != cachedWrapW) {
        wrapText(newAvailW, /* ... */);
        cachedWrapW = newAvailW;
    }
}
```

`Text` uses this to re-wrap when the width changes. Most widgets don't
need it.

**Constraint:** `onLayout` must not modify `measuredSize`. It's called
after measure, and changing the size would invalidate the layout
without triggering a re-measure.

### 3.7 `onEnabledChanged(nowEnabled)`

```cpp
virtual void onEnabledChanged(bool nowEnabled) {}
```

Called when `setEnabled` changes the state. Use it for cleanup that
only makes sense while enabled:

```cpp
void onEnabledChanged(bool nowEnabled) override {
    if (!nowEnabled) {
        hasFocusCache = false;
        repeatStates.clear();
        cursorBlink = 0.0f;
    }
}
```

`TextInput` uses it to clear its keyboard repeat state and cursor blink
when disabled. `Dropdown` uses it to close the list:

```cpp
void onEnabledChanged(bool nowEnabled) override {
    if (!nowEnabled && isOpen) {
        isOpen = false;
        applyOpenState();
    }
}
```

### 3.8 `onDescendantFocused(descendant)`

```cpp
virtual void onDescendantFocused(Layout* descendant) {}
```

Called when a descendant of this node gains focus (via Tab navigation).
Useful for containers that want to highlight when any child is focused:

```cpp
void onDescendantFocused(Layout*) override {
    getInlineBase().borderColor = Colors::Yellow;
    pendingTransition = true;
}
```

Only `Layout::notifyFocusAncestors` calls this, and only during Tab
navigation. Clicking to focus a descendant doesn't fire it (the
behavioral difference is a known asymmetry).

---

## 4. The CRTP chain: `getInlineBase()` and `cls()` inside `onBuild`

Inside `onBuild`, you can call anything from `Layout`:

```cpp
void onBuild() override {
    setStyleTag("MyWidget");

    // Inline defaults (can be overridden by CSS)
    getInlineDefaults().background = Color{40, 40, 45, 255};
    getInlineDefaults().radius = Px(6.0f);

    // Inline base (wins over CSS)
    getInlineBase().width = Percent(100);

    // Fluent (returns shared_ptr<Derived>)
    cls("my-widget-class");

    // Children
    addChild(Label("Hello"));
}
```

The distinction between `inlineDefaults` and `inlineBase`:

- **`inlineDefaults`** — the "user agent stylesheet" for the widget.
  It's the lowest-priority source in the cascade, below all CSS rules.
  Use it for "reasonable defaults the user can override".
- **`inlineBase`** — the "inline style" level. It wins over everything,
  including `:hover` rules. Use it for "the widget's contract" — things
  that shouldn't be changed by a stylesheet.

Example from `Checkbox`:

```cpp
void onBuild() override {
    // ...
    getInlineBase().background = Colors::DarkGray;
    getInlineBase().radius = Px(6.0f);
    // ...
}
```

These are inline base — a user's `.zstyle` rule for `Checkbox { background: ... }`
won't override them. The `Checkbox::box` part can still change the
visual, because `renderContent` reads the part style, not the base.

Example from `Toggle`:

```cpp
void onBuild() override {
    setStyleTag("Toggle");
    setInteractive(true);
    setFocusable(true);
    setKeyboardActivates(true);
    // No inline base — everything comes from CSS.
}
```

`Toggle` has no inline base styles; all its visual comes from the
`.zstyle`. This is the "flexible" pattern, used when the widget is
meant to be fully styled by the user.

---

## 5. Interaction patterns

### 5.1 Handling clicks

The simplest pattern:

```cpp
void onBuild() override {
    setInteractive(true);
    onClick = [this] {
        if (onClicked) onClicked();
    };
}

std::function<void()> onClicked;
```

But `Layout` already has `onClick`, `onPress`, `onRelease`,
`onHoverEnter`, `onHoverExit`, `onRightClick` as public fields. So you
often don't need to do anything — the user sets them directly on the
widget.

`Button` does exactly this: it inherits the callbacks from `Layout` and
just sets up the interactive flags:

```cpp
Button(std::function<void()> cb) {
    onClick = std::move(cb);
    setInteractive(true);
    setFocusable(true);
    setKeyboardActivates(true);
    setStyleTag("Button");
    getInlineBase().itemsH = Align::Center;
    getInlineBase().itemsV = Align::Center;
}
```

### 5.2 Toggle-style state

For widgets that have a binary state (checked / unchecked), use the
built-in `isChecked_` flag and `setChecked`:

```cpp
class MyToggle : public TLayout<MyToggle> {
public:
    MyToggle(bool state = false) : TLayout<MyToggle>(LayoutType::Stack) {
        setInteractive(true);
        setFocusable(true);
        setKeyboardActivates(true);
        setStyleTag("MyToggle");

        setChecked(state);

        onClick = [this] {
            bool newState = !getChecked();
            setChecked(newState);
            if (onToggle) onToggle(newState);
        };
    }

    std::function<void(bool)> onToggle;
};
```

`setChecked` updates the flag and sets `pendingTransition`, which
re-resolves the style on the next update. So a `.zstyle` rule like
`MyToggle:checked { background: green; }` will apply automatically.

### 5.3 Focus-based input

Widgets that handle keyboard input should set `setFocusable(true)` and
read `UIContext::get().inputEvents` in `onUpdate`:

```cpp
void onUpdate(float dt) override {
    if (!UIContext::get().hasFocus(this))
        return;

    auto& ev = UIContext::get().inputEvents;

    for (int c : ev.chars) {
        // handle printable char
    }
    for (int k : ev.keys) {
        // handle special key (rising edge)
    }
    // ev.held contains keys currently down (for repeat behavior)
}
```

`ev.chars` contains Unicode codepoints typed this frame.
`ev.keys` contains special key codes (`Key::Enter`, `Key::Escape`,
`Key::Left`, `Key::Right`, `Key::Home`, `Key::End`, `Key::Backspace`,
`Key::Delete`, `Key::Tab`, `Key::Space`) pressed this frame.
`ev.held` contains special keys currently held.

For key repeat, you need to implement it yourself — see `TextInput` for
a full example using `repeatStates`.

### 5.4 Pointer capture (drag)

For drag-style interactions, use the pointer capture API:

```cpp
void onUpdate(float) override {
    if (isPressed && !hasPointerCapture())
        capturePointer();

    if (hasPointerCapture()) {
        // The cursor is captured — pointer events are routed to this node
        // regardless of position.
        Vec2 p = UIContext::get().pointer.pos;
        // do something with p
    }
}
```

Capture is released automatically when the mouse button is released.
Call `releasePointer()` explicitly if you need to end the drag early.

See `Slider::onUpdate` for a complete example.

### 5.5 Portal widgets

If your widget floats above the tree (dropdown list, tooltip, popup),
set `setPortal(true)` in `onBuild` and compute the rect in `onUpdate`:

```cpp
void onBuild() override {
    setPortal(true);
    setInteractive(false);
    setBlocksRaycast(false);
    style_.inlineBase.position = Position::Absolute;
    style_.inlineBase.opacity = 0.0f;
    setEnabled(false);
}

void arrange(Rect) override { /* no-op, we position manually */ }

void onUpdate(float) override {
    if (!isOpen) return;
    Vec2 s = getMeasuredSize();
    rect = {pos_.x, pos_.y, s.x, s.y};
    arrangeInto(rect);
}
```

See [Portals](08-portals.md) for the full pattern.

---

## 6. Styling your widget

A widget has a **style tag** set with `setStyleTag("MyWidget")`. This is
what `MyWidget { ... }` selectors in `.zstyle` match:

```css
MyWidget {
    background: #232328;
    radius: 8px;
}
```

If the widget has children with `cls("my-part")`, they can be targeted
with descendant selectors:

```css
MyWidget .my-part {
    color: red;
}
```

For pseudo-elements drawn in `renderContent`, use `::part`:

```cpp
void renderContent(float op, const ComputedStyle&) override {
    Style trackStyle = partStyle("track");
    Style fillStyle  = partStyle("fill");

    Color trackColor = trackStyle.background.is_set
        ? trackStyle.background.value
        : Color{40, 40, 45, 255};

    // draw track
    r->fillRect(trackRect, trackColor.withAlpha(op));

    // draw fill
    Color fillColor = fillStyle.color.is_set
        ? fillStyle.color.value
        : Colors::White;
    r->fillRect(fillRect, fillColor.withAlpha(op));
}
```

Then:

```css
MyWidget::track { background: #1717cf; }
MyWidget::fill  { color: #d6ff32; }
```

`partStyle(name)` resolves the part's `Style` from the theme, applying
its own transitions and CSS animations. It's the only correct way to
read a part's properties — do not try to resolve rules manually.

The result of `partStyle` is a `Style` with optional fields. Check
`is_set` before using:

```cpp
if (style.background.is_set) {
    r->fillRect(rect, style.background.value.withAlpha(op));
} else {
    r->fillRect(rect, Colors::DarkGray.withAlpha(op));
}
```

See `Slider`, `Checkbox`, `ProgressBar`, `TextInput` for examples of
parts in use.

---

## 7. Registering with ZMarkup

To use a custom widget in ZMarkup, register a factory:

```cpp
ZMarkup::Registry reg;
ZMarkup::registerBuiltins(reg);   // if you want the defaults too

reg.reg("MyWidget", [](const ZMarkup::Element& e) {
    return MyWidget::create(e.attrFloat("value", 0.5f));
});
```

The factory:

- Receives the parsed `Element`.
- Returns a `shared_ptr<Layout>` (or `nullptr` to skip).
- Doesn't need to handle classes, id, attrs, or children — the registry
  does that after calling the factory.

For the full registry API, see [ZMarkup §6](04-zmarkup.md#6-extending-the-registry).

Note: the default `ZMarkup::build` uses a **private** registry that
already has the built-ins. To add your own tags to that registry, you
need to build the tree yourself using `parse` + your own registry +
`UINode`:

```cpp
auto el = ZMarkup::parse(source);
auto ctx = std::make_shared<ZMarkup::BuildContext>();

ZMarkup::Registry reg;
ZMarkup::registerBuiltins(reg);
reg.reg("MyWidget", ...);

auto root = reg.create(el, *ctx);
ZMarkup::UINode ui(root, ctx);
```

There's no global way to mutate the private registry used by
`ZMarkup::build` — this is intentional, to keep the default set stable.

---

## 8. Complete example: a `Rating` widget

Let's build a 5-star rating widget from scratch. It:

- Displays `maxStars` stars.
- Shows the current rating (0..maxStars).
- Fills stars up to `rating` with a "filled" color.
- Lets the user click a star to set the rating.
- Exposes `onRatingChanged`.
- Supports `.zstyle` through a `::star` part.

### 8.1 The header

```cpp
#pragma once
#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

class Rating : public TLayout<Rating> {
public:
    Rating(int maxStars = 5, int rating = 0)
        : TLayout<Rating>(LayoutType::Horizontal),
          maxStars_(maxStars),
          rating_(std::clamp(rating, 0, maxStars))
    {
        setStyleTag("Rating");
        setInteractive(true);
        setFocusable(true);
        setKeyboardActivates(false);
        getInlineBase().gap = Px(4.0f);
    }

    int getRating() const { return rating_; }

    void setRating(int r) {
        int old = rating_;
        rating_ = std::clamp(r, 0, maxStars_);
        if (rating_ != old) {
            pendingTransition = true;
            if (onRatingChanged) onRatingChanged(rating_);
        }
    }

    std::function<void(int)> onRatingChanged;

protected:
    Vec2 computeIntrinsicSize(float, float) override {
        float fs = style_.currentStyle.fontSize.resolveV(
            Metrics::viewport.x, Metrics::viewport.y);
        float starSize = fs * 1.2f;
        return {starSize * maxStars_ + 4.0f * (maxStars_ - 1), starSize};
    }

    void renderContent(float op, const ComputedStyle& style) override {
        auto r = UIContext::get().renderer;
        if (!r) return;

        float fs = style.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
        float starSize = fs * 1.2f;
        float gap = 4.0f;
        float totalW = starSize * maxStars_ + gap * (maxStars_ - 1);
        float startX = rect.x + (rect.width - totalW) * 0.5f;
        float centerY = rect.center().y;

        Style starStyle = partStyle("star");
        Color filledColor = starStyle.color.is_set
            ? starStyle.color.value
            : Colors::Yellow;
        Color emptyColor = starStyle.background.is_set
            ? starStyle.background.value
            : Color{80, 80, 90, 255};

        for (int i = 0; i < maxStars_; ++i) {
            float cx = startX + i * (starSize + gap) + starSize * 0.5f;
            Color c = (i < rating_) ? filledColor : emptyColor;
            r->fillCircle({cx, centerY}, starSize * 0.5f, c.withAlpha(op));
        }
    }

    void onUpdate(float) override {
        // Check for click on a star
        if (!UIContext::get().pointer.pressed) return;
        if (!rect.contains(UIContext::get().pointer.pos)) return;

        float fs = style_.currentStyle.fontSize.resolveV(
            Metrics::viewport.x, Metrics::viewport.y);
        float starSize = fs * 1.2f;
        float gap = 4.0f;
        float totalW = starSize * maxStars_ + gap * (maxStars_ - 1);
        float startX = rect.x + (rect.width - totalW) * 0.5f;

        float localX = UIContext::get().pointer.pos.x - startX;
        int index = (int)(localX / (starSize + gap));
        index = std::clamp(index, 0, maxStars_ - 1);

        setRating(index + 1);
    }

private:
    int maxStars_;
    int rating_;
};

} // namespace ZenitUI::UI
```

### 8.2 Usage

```cpp
auto rating = Rating::create(5, 3);
rating->onRatingChanged = [](int r){
    std::printf("rating: %d\n", r);
};
root->addChild(rating);
```

### 8.3 Styling

```css
Rating {
    font-size: 4vh;
    gap: 6px;
}
Rating::star {
    color: #FFD700;         /* filled star */
    background: #505060;    /* empty star */
}
Rating:focus::star {
    color: #FFAA00;         /* focused: warmer gold */
}
```

### 8.4 Registering in ZMarkup

```cpp
reg.reg("Rating", [](const ZMarkup::Element& e) {
    return Rating::create(
        (int)e.attrFloat("max", 5.0f),
        (int)e.attrFloat("value", 0.0f)
    );
});
```

Then:

```
Rating#stars max=5 value=3
```

```cpp
ui.find<Rating>("stars")->onRatingChanged = [](int r){ /* ... */ };
```

---

## 9. Registering with `UIComponents.hpp`

`UIComponents.hpp` is the aggregator that pulls in all widget headers:

```cpp
// Base
#include "components/Layouts.hpp"
#include "components/Text.hpp"
#include "components/Button.hpp"
#include "components/ImageContainer.hpp"

// Controls
#include "components/Toggle.hpp"
#include "components/Checkbox.hpp"
#include "components/Slider.hpp"
#include "components/ProgressBar.hpp"
#include "components/TextInput.hpp"

// Containers
#include "components/ScrollView.hpp"
#include "components/Dropdown.hpp"
#include "components/Canvas.hpp"
#include "components/Modal.hpp"
#include "components/Tooltip.hpp"
#include "components/Popup.hpp"
```

If your widget lives in `components/`, add its header here so that
including `UI.hpp` pulls it in.

If your widget lives in a separate module, you can simply include its
header where needed — the framework doesn't require registration
beyond `ZMarkup::Registry` (which is separate from C++ includes).

---

## 10. Common pitfalls

**Calling `addChild` in the constructor.**

```cpp
Toggle(bool state) {
    addChild(knob);   // ❌ weak_from_this() is expired
}
```

Move it to `onBuild()` and construct with `Toggle::create(state)`.

**Using `shared_from_this()` in the constructor.**

```cpp
Checkbox() {
    onClick = [this, sp = shared_from_this()] { ... };   // ❌ throws
}
```

`shared_from_this()` is only valid after the `shared_ptr` exists. Use
`[this]` and store the callback as a member field instead.

**Not applying `op` to drawn colors.**

```cpp
r->fillRect(rect, myColor);                // ❌ ignores node opacity
r->fillRect(rect, myColor.withAlpha(op));  // ✅
```

Every color you draw must have `.withAlpha(op)` applied. Otherwise the
node's `opacity` and all ancestor opacities are silently ignored.

**Forgetting `setStyleTag`.**

```cpp
void onBuild() override {
    // no setStyleTag → MyWidget { } rules never match
}
```

Without a style tag, `.zstyle` rules that target `MyWidget` will never
apply. The tag defaults to an empty string, which no rule matches.

**Modifying `measuredSize` in `onLayout`.**

`onLayout` is called after measure. Changing the size there means the
layout was computed with stale data. If you need the size to depend on
final dimensions (like Text wrapping), cache the result and use it in
the next frame's measure.

**Reading part styles outside `renderContent`.**

`partStyle(name)` triggers a re-tick of the part's animation and
transition. It's designed to be called once per frame per part, inside
`renderContent`. Calling it from `onUpdate` or `onLayout` works but
wastes cycles and can trigger the part's transition in the wrong frame.

**Focus and Tab.**

If your widget handles keyboard input but isn't focusable, it will
never receive `Enter`/`Space` via `keyboardActivates`. Set
`setFocusable(true)` and, if you want Enter/Space to fire `onClick`,
`setKeyboardActivates(true)`.

**Child widgets should be non-interactive.**

```cpp
knob->setInteractive(false);
knob->setBlocksRaycast(false);
```

A child used purely for visuals (a knob, a mark, a track) should not
intercept input. Otherwise the parent's click won't reach it when the
child is under the cursor. This is what `Toggle::onBuild` does for its
knob.

**Callbacks that capture `this` after destruction.**

```cpp
onClick = [this] { ... };   // fine: `this` outlives the node
```

But:

```cpp
someExternalNode->onClick = [this] { ... };  // ⚠️ dangling if `this` dies first
```

If you attach callbacks to external nodes, use a `weak_ptr` or make
sure the lifetimes are ordered. The `Tooltip::attach` helper captures a
`shared_ptr<Tooltip>`, so it's safe. The `ContextMenu` helper captures
a `weak_ptr<Popup>` specifically to avoid a cycle. Follow the same
pattern when the target outlives the source.

**Widgets that read `style_.currentStyle` in `onBuild`.**

At `onBuild` time, `style_.currentStyle` hasn't been resolved yet — the
first `update` populates it. If you need the resolved style, override
`onLayout` or `renderContent`, or read the inline style you just set.

**`updateWhenDisabled`.**

By default, `onUpdate` is not called when the widget is disabled. If
your widget needs to keep running (e.g. to animate its own disabled
fade-out), call `setUpdateWhenDisabled(true)`.