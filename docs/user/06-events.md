# Events

ZenitUI has a small but precise input model. There are no DOM-like
events propagating through a virtual tree — instead, each `Layout` has a
set of **flags** (`isHovered`, `isPressed`, `isFocused`) and a set of
**callbacks** (`onClick`, `onPress`, `onHoverEnter`, …) that the runtime
fires when those flags change or when a click is validated.

The flags are also what `.zstyle` state pseudo-classes read, so styling
and behavior are driven by the same source of truth.

---

## 1. The state machine

Each node is always in exactly one of four states, computed once per
frame from its flags:

| State | Condition |
|-------|-----------|
| `Disabled` | `!isEnabled` |
| `Pressed`  | `isPressed` |
| `Hover`    | `isHovered` |
| `Idle`     | none of the above |

The runtime calls `InputController::computeNextState` in this priority
order. If the state changes, a style transition is started (so `:hover`
and `:pressed` rules can animate in), and the corresponding callbacks
are fired.

Flags are updated every frame **before** the state is computed:

- `isHovered` — true if the pointer is inside the node's `rect` and the
  node isn't blocked by an ancestor.
- `isPressed` — true if the pointer is down and the "press target" is
  this node or a descendant of it.
- `isFocused` — true if `UIContext::focusedNode` points to this node.

While a scroll is in progress, `isHovered` is **frozen** — the runtime
skips the recomputation so that scrolling a list doesn't fire
`onHoverEnter`/`onHoverExit` on every row the cursor passes over.

---

## 2. Callbacks

All callbacks are `std::function<void()>` except `onRightClick`, which
is also `void()`:

| Callback | Fires when |
|----------|------------|
| `onPress` | Pointer down inside the node (or its subtree). |
| `onRelease` | Pointer up while the press target was inside the node. |
| `onClick` | Press + release **on the same node**, and no descendant consumed the click first. |
| `onHoverEnter` | `isHovered` transitions from `false` to `true`. |
| `onHoverExit` | `isHovered` transitions from `true` to `false`. |
| `onRightClick` | Right-button up inside the node (and not consumed). |

Set them directly on the node:

```cpp
auto b = Btn("OK", []{ /* this is onClick */ });

b->onPress   = []{ std::printf("down\n"); };
b->onRelease = []{ std::printf("up\n"); };
b->onHoverEnter = []{ std::printf("enter\n"); };
b->onHoverExit  = []{ std::printf("leave\n"); };
b->onRightClick = []{ std::printf("right\n"); };
```

Or through the `UINode` handle after building from ZMarkup:

```cpp
ui.onClick("ok", []{ /* ... */ })
  .onPress("ok", []{ /* ... */ })
  .onHoverEnter("ok", []{ /* ... */ });
```

There's no built-in `onClickOutside`. To get that behavior, use
`UIContext::get().topmostConsumer` in an `onUpdate`, or use `Popup`
which already handles click-outside (see [Portals](08-portals.md)).

---

## 3. Click semantics

A "click" is a two-step process: **press**, then **release** while still
over the same target.

```
pointer down          pointer up
     │                    │
     ▼                    ▼
  onPress  ────────►   onRelease
              └──► onClick
```

The runtime uses three pointers to track this:

- `pressTarget` — set at pointer-down to the current `topmostConsumer`.
- `releaseTarget` — set at pointer-up to the current `topmostConsumer`.
- `clickConsumed` — a per-frame flag set by the first node whose
  `onClick` fires and that isn't a `passThrough` node.

A click is valid if the same node (or an ancestor/descendant pair within
the same subtree) was the press target **and** the release target.

```cpp
// Simple case
auto b = Btn("OK", []{ std::printf("clicked\n"); });

// Press on B, drag to C, release on C → no click on B, no click on C
// Press on B, drag to B, release on B → click on B
```

### 3.1 Bubbling

When a click happens on a node, the runtime walks the ancestor chain
from the innermost node outward:

1. The deepest hit node has its `onClick` called first.
2. If it's a normal node (not `passThrough`), it consumes the click —
   **ancestors don't see it**.
3. If it's a `passThrough` node, `clickConsumed` is not set, and the
   ancestor's `onClick` also fires.

```cpp
auto parentBox = VStack();
parentBox->setInteractive(true);
parentBox->onClick = []{ std::printf("parent\n"); };

auto btn = Btn("child", []{ std::printf("child\n"); });

parentBox->addChild(btn);
```

Clicking the button prints `child`. The parent doesn't see the click.
To let it through:

```cpp
btn->setPassThrough(true);
```

Now clicking the button prints both `child` and `parent`.

In ZMarkup:

```
VStack#parentbox {
    Button.passthrough#child "Click me"
}
```

```cpp
ui.onClick("child",  []{ std::printf("child\n"); });
ui.onClick("parentbox", []{ std::printf("parent\n"); });
```

`passThrough` is a flag that can be set on any node via
`setPassThrough(true)` or the `passthrough` attribute.

### 3.2 The Bubbling order

Callbacks are fired **bottom-up** — the deepest node first. If a
descendant consumes the click, ancestors are skipped for that phase.

For `onPress` and `onRelease`, the same bottom-up order applies, and
consumption also stops the propagation.

### 3.3 Press outside, release inside

If you press outside a node and release inside it, no click fires. The
runtime requires `pressTarget == releaseTarget` (or the same subtree,
respecting passThrough).

```cpp
// No click:
platform.pressLeft({50, 50});   // outside the button
platform.releaseLeft({60, 60}); // inside the button (if button is at 60,60)
```

This is intentional — it prevents accidental clicks when dragging into a
button.

### 3.4 Press inside, release outside

Also no click. The release target won't match, and the click is dropped.

---

## 4. Hover

`isHovered` is recomputed every frame in `InputController::updateFlags`:

```cpp
node.isHovered = false;
if (!selfBlocked && node.rect.width > 0 && node.rect.height > 0)
    node.isHovered = node.rect.contains(pointer.pos);
```

where `selfBlocked` is true if the node is disabled, non-interactive
with `blocksRaycast`, or has an ancestor that is blocked.

`onHoverEnter` / `onHoverExit` fire on the transition, in
`fireCallbacks`:

```cpp
if (nextState == UIState::Hover && prevState != UIState::Hover)
    node.onHoverEnter();

if (prevState == UIState::Hover && nextState != UIState::Hover)
    node.onHoverExit();
```

### 4.1 Hover freeze during scroll

While a scroll is in progress (either an active drag or inertia), the
`updateFlags` function is called with `scrolling = true` and skips the
hover update. This is important: without it, scrolling a list of 200
buttons would fire `onHoverEnter`/`onHoverExit` for every row that
passes under a stationary cursor.

The focus state is still updated during the freeze — the runtime just
doesn't recompute the pointer-driven flag.

### 4.2 Hover and `setInteractive(false)`

A non-interactive node can still **block** raycast (with
`setBlocksRaycast(true)`), but it won't be hovered. It also won't
propagate hover to nodes behind it.

```cpp
auto panel = Pan();      // non-interactive, blocks raycast
panel->size(Px(200), Px(200));

auto btn = Btn("hidden by panel", []{});
```

If `btn` is behind `panel`, hovering the overlapping area hovers
`panel` (which does nothing) and not `btn`. To let hover through, use
`panel->setBlocksRaycast(false)`.

### 4.3 Tooltip integration

`Tooltip::attach(owner, ...)` hooks into `owner->onHoverEnter` /
`onHoverExit` and starts/stops a timer. It's a good example of
implementing hover-driven behavior without touching the input code:

```cpp
Tooltip::attach(btn, "Save your changes", 0.4f);
```

See [Portals](08-portals.md) §"Tooltip" for details.

---

## 5. Right-click

Right-click fires `onRightClick` if the pointer is hovering the node at
the moment of release. It uses the same consumption mechanism as a
normal click (`rightClickConsumed`), but there's no bubbling cascade —
only the topmost hovered node fires it.

```cpp
auto box = VStack();
box->onRightClick = [box]{
    // open a context menu
};
```

`Popup` / `ContextMenu` are typically opened from `onRightClick`:

```cpp
auto menu = ContextMenu({ ... });
box->addChild(menu);   // for lifetime
box->onRightClick = [box, menu]{
    menu->openAt(UIContext::get().pointer.pos);
};
```

Note: unlike `onClick`, `onRightClick` fires on the hovered node
directly, without needing a press. It's a single-step event.

---

## 6. Focus

A node can be focusable with `setFocusable(true)`. Only focusable nodes
receive focus.

```cpp
auto input = std::make_shared<TextInput>("hello");
input->setFocusable(true);   // TextInput does this in its constructor
```

### 6.1 How focus is granted

Focus is granted in `Layout::updateTree`:

1. **On pointer down** — if `topmostConsumer` is focusable, it becomes
   the focused node. Otherwise, focus is released.
2. **On Tab** — see §7.

```cpp
if (ctx.pointer.pressed)
{
    if (ctx.topmostConsumer && ctx.topmostConsumer->isFocusable())
        ctx.requestFocus(ctx.topmostConsumer->shared_from_this());
    else
        ctx.releaseFocus();
}
```

`requestFocus` also updates `isFocused` on every node in the tree
through the per-node `updateFlags` call.

### 6.2 Focus and `:focus` styling

`:focus` in `.zstyle` matches when `isFocused` is true:

```css
TextInput:focus { border-color: #FDF900; }
Button:focus    { border-color: #FDF900; border-width: 2px; }
```

The style transition from the focused state (border-color change) is
animated by the normal state transition system. `TextInput` in the demo
uses a 0.2s transition on `border-color`:

```css
TextInput {
    border-color: #505060;
    transition: border-color 0.2s ease-out-quad;
}
TextInput:focus { border-color: #FDF900; }
```

### 6.3 Releasing focus

Focus is released when:

- Another focusable node is clicked.
- The click doesn't hit any focusable node (click on empty space).
- The focused node is disabled (`setEnabled(false)` clears focus if it
  points to that node).
- `Escape` is pressed and the focused node handles it (e.g. `TextInput`
  releases focus on Escape).
- `UIContext::releaseFocus()` is called manually.

```cpp
UIContext::get().releaseFocus();
```

### 6.4 Focus and `setEnabled(false)`

When a node is disabled, `setEnabled(false)` checks whether it's the
current focused node and clears focus. It also releases any pointer
capture held by the node:

```cpp
void setEnabled(bool e) {
    ...
    if (!e) {
        if (ctx.focusedNode.lock().get() == this) ctx.focusedNode.reset();
        if (ctx.pointerCapture.lock().get() == this) ctx.pointerCapture.reset();
    }
    onEnabledChanged(e);
}
```

The virtual `onEnabledChanged(bool)` hook lets a widget clean up its own
state — `TextInput`, for example, clears its repeat states and cursor
blink when disabled.

---

## 7. Tab navigation

Tab is handled in `Layout::updateTree`:

1. When a `Key::Tab` event is seen, the runtime collects all focusable
   nodes in the current **focus scope** (see §8).
2. It finds the currently focused node in that list.
3. It advances to the next (or previous, with `Shift`) index, wrapping
   around.
4. It calls `requestFocus` on the new node.

The traversal is **depth-first, pre-order** — the same order as
`collectFocusables`:

```cpp
static void collectFocusables(Layout *node, std::vector<Layout *> &out) {
    if (node->isFocusable())
        out.push_back(node);
    for (auto &c : node->children)
        collectFocusables(c.get(), out);
}
```

So a `VStack` with three buttons and a nested `HStack` with two more
will yield a flat list in this order: outer buttons, then inner ones.

### 7.1 Shift+Tab

With `Shift` held, Tab walks the list backwards:

```cpp
next = ctx.shiftHeld ? (idx - 1 + n) % n : (idx + 1) % n;
```

The `+ n` is to handle `idx = 0` correctly (`-1` mod `n` would be `-1`
in C++).

### 7.2 Wrap-around

Tab wraps around: from the last focusable, Tab goes to the first; from
the first, Shift+Tab goes to the last.

### 7.3 Ancestors of the focused node are notified

When Tab changes focus, the runtime walks up the ancestor chain and
calls `notifyDescendantFocused` on each:

```cpp
static void notifyFocusAncestors(Layout *node) {
    auto parent = node->getParent();
    while (parent) {
        parent->notifyDescendantFocused(node);
        parent = parent->getParent();
    }
}
```

This fires the virtual `onDescendantFocused(Layout*)` hook. Useful for
widgets that want to know when focus enters their subtree — for
example, a wrapper that highlights itself when any child is focused.

---

## 8. Focus scopes

A **focus scope** restricts the Tab cycle. When a `Tab` event arrives,
the runtime finds the nearest ancestor of the focused node that has
`setFocusScope(true)`, and uses that scope as the root of the
`collectFocusables` walk.

```cpp
modal->setFocusScope(true);
```

This is what `Modal` and `SettingsScreen` do — while a modal is open,
Tab cycles only through the modal's focusable nodes, not the rest of
the tree.

### 8.1 How the scope is found

```cpp
Layout *scope = this;   // the root layout
auto focused = ctx.focusedNode.lock();
if (focused) {
    Layout *n = focused.get();
    while (n) {
        if (n->isFocusScope()) { scope = n; break; }
        n = n->getParent().get();
    }
}
```

If no focused node exists, or the focused node has no focus-scope
ancestor, the scope falls back to the root (`this` in `updateTree`).

### 8.2 Nested scopes

Scopes can nest. The nearest one wins. So a modal inside a modal cycles
through the innermost modal's focusables only.

### 8.3 Focus scope and `onBuild`

`Modal::onBuild` sets `setFocusScope(true)` and calls `setInteractive(false)`
by default. Wait, no — it sets it in `onBuild`:

```cpp
void onBuild() override {
    setInteractive(false);
    setStyleTag("Modal");
    setFocusScope(true);
    ...
}
```

The scope itself isn't focusable (it's a container), but it's the
boundary for Tab navigation inside it.

---

## 9. Keyboard activation

A focusable node can also react to `Enter` and `Space` if
`setKeyboardActivates(true)` is set:

```cpp
auto btn = Btn("OK", []{ /* fires on click AND Enter/Space */ });
btn->setFocusable(true);
btn->setKeyboardActivates(true);
```

The runtime handles this in `InputController::handleKeyInput`:

```cpp
if (!node.isFocused || !node.isEnabled || !node.keyboardActivates_)
    return;

for (int k : ev.keys) {
    if (k == Key::Enter || k == Key::Space) {
        if (node.onClick) node.onClick();
    }
}
```

The keys come from `InputEvents::keys`, which is populated only for
**rising edge** key presses (not for held keys). So `Enter` fires the
callback once per press, not per frame.

`Button` sets `keyboardActivates = true` by default. `Toggle` and
`Checkbox` do too, so `Enter`/`Space` toggles them.

### 9.1 Interaction with `TextInput`

`TextInput` does **not** set `keyboardActivates` because it wants to
handle `Enter` itself:

```cpp
case Key::Enter:
    if (onSubmit) onSubmit(text);
    UIContext::get().releaseFocus();
    return;
```

If `keyboardActivates` were true, `Enter` would fire `onClick` (which
for `TextInput` just requests focus) — not what you want.

### 9.2 Key repeat

`keyboardActivates` uses only the rising edge (`ev.keys`), so it doesn't
auto-repeat. If you need repeat behavior (like a slider or text cursor),
handle `ev.held` in your `onUpdate` — see [Slider](03-widgets.md) and
[TextInput](03-widgets.md) for examples.

---

## 10. Pointer capture

Sometimes a widget wants to receive all pointer events until the button
is released, even if the cursor leaves the widget. This is what
`Slider` does during a drag.

```cpp
void capturePointer() {
    UIContext::get().pointerCapture = shared_from_this();
}
void releasePointer() {
    auto sp = UIContext::get().pointerCapture.lock();
    if (sp.get() == this)
        UIContext::get().pointerCapture.reset();
}
bool hasPointerCapture() const {
    return UIContext::get().pointerCapture.lock().get() == this;
}
```

### 10.1 Effect of capture

When a node has pointer capture:

- `topmostConsumer` is set to that node every frame, regardless of the
  pointer position.
- The node's `isHovered` and `isPressed` flags are updated as if the
  pointer were on top of it.
- All `onPress` / `onRelease` / `onClick` events are routed to it.

### 10.2 Automatic release

Capture is released automatically when the mouse button is no longer
down:

```cpp
if (!ctx.pointer.down && !ctx.pointerCapture.expired())
    ctx.pointerCapture.reset();
```

So you don't strictly need to call `releasePointer()` — but you should
anyway, if you want to end the drag early (e.g. on `Escape`).

### 10.3 Example: Slider drag

```cpp
void Slider::onUpdate(float) override {
    if (isPressed && !hasPointerCapture())
        capturePointer();

    if (hasPointerCapture() && rect.width > 0) {
        float px = UIContext::get().pointer.pos.x;
        float percent = std::clamp((px - rect.x) / rect.width, 0.0f, 1.0f);
        if (percent != value) {
            value = percent;
            if (onValueChanged) onValueChanged(value);
        }
    }
}
```

Press starts the capture. While captured, the slider reads
`pointer.pos.x` directly and maps it to `[0, 1]`. Release happens
automatically because the runtime clears capture when `pointer.down`
becomes false.

### 10.4 Disabling releases capture

`setEnabled(false)` also clears capture if this node holds it — see §6.4.

---

## 11. Wheel events

Wheel events are delivered to the node under the cursor, if it's a
scroll container and `isInteractive`:

```cpp
if (pointer.wheelY != 0.0f) {
    if (ctx.shiftHeld && canScrollX) {
        state.velocity.x -= pointer.wheelY * ScrollState::WHEEL_IMPULSE;
        ctx.wheelConsumedThisFrame = true;
    } else if (!ctx.shiftHeld && canScrollY) {
        state.velocity.y -= pointer.wheelY * ScrollState::WHEEL_IMPULSE;
        ctx.wheelConsumedThisFrame = true;
    }
}
```

- `wheelY` is the raw wheel delta.
- Without `Shift`, the wheel scrolls vertically.
- With `Shift`, the wheel scrolls horizontally.
- Only the **first** node that consumes the wheel does so —
  `wheelConsumedThisFrame` prevents nested scroll containers from both
  scrolling.

The wheel is applied as **velocity**, not direct offset, and decays
exponentially. See [Scroll System](../internals/08-scroll-system.md) for
the inertia model.

---

## 12. State flags in CSS

Every state flag has a matching pseudo-class in `.zstyle`:

| Flag | Pseudo-class | Set by |
|------|--------------|--------|
| `isHovered` | `:hover` | Pointer inside the node. |
| `isPressed` | `:pressed` | Pointer down on the node. |
| `isFocused` | `:focus` | Node is the focused one. |
| `isDisabled` | `:disabled` | `!isEnabled`. |
| `isChecked` | `:checked` | `setChecked(true)`. |

```css
Button:hover   { background: #3296FF; }
Button:pressed { background: #505050; }
Button:focus   { border-color: #FDF900; border-width: 2px; }
Button:disabled{ opacity: 0.4; }
Toggle:checked { background: #00E430; }
```

States can be combined:

```css
Toggle:checked:hover { background: #00FF44; }
Button:focus:pressed { background: #404040; }
```

The state flags are also readable in C++ through `isHoveredState()`,
`isPressedState()`, `isFocusedState()`, `isDisabledState()`,
`isCheckedState()`. Widgets override `renderContent` and use these to
draw differently — see `Checkbox`, `Slider`, and `TextInput`.

### 12.1 State changes trigger transitions

When a flag changes and the state changes with it, the runtime calls
`StyleResolver::beginStateTransition`, which:

1. Snapshots the current `ComputedStyle` as `transitionStartStyle`.
2. Resolves the new target style (with the new flag).
3. Starts the per-property transition.

So a `:hover { scale: 1.05 }` rule animates over the specified duration
if `transition: scale 0.15s` is declared. Without a transition, the
change is immediate.

---

## 13. Complete example

A button with hover, focus, keyboard activation, and a custom right-click
handler:

```cpp
#include "UI.hpp"
using namespace ZenitUI;
using namespace ZenitUI::UI;

auto root = std::make_shared<Layout>(LayoutType::Stack);
root->size(Percent(100), Percent(100));
root->getInlineBase().itemsH = Align::Center;
root->getInlineBase().itemsV = Align::Center;

auto btn = Btn("Click me", []{
    std::printf("clicked!\n");
});
btn->cls("btn-primary");
btn->setFocusable(true);
btn->setKeyboardActivates(true);

btn->onPress   = []{ std::printf("down\n"); };
btn->onRelease = []{ std::printf("up\n"); };
btn->onHoverEnter = []{ std::printf("enter\n"); };
btn->onHoverExit  = []{ std::printf("leave\n"); };
btn->onRightClick = []{ std::printf("right\n"); };

root->addChild(btn);

// In the frame loop, call runFrame(dt) as usual.
// Click → prints "down", "up", "clicked"
// Hover → prints "enter", "leave"
// Right-click → prints "right"
// Tab + Enter → prints "clicked"
```

Same setup, using ZMarkup:

```cpp
auto ui = ZMarkup::build(R"(
    Stack#root {
        Button.btn-primary#go "Click me"
    }
)");

ui.onClick("go", []{ std::printf("clicked!\n"); })
  .onPress("go", []{ std::printf("down\n"); })
  .onHoverEnter("go", []{ std::printf("enter\n"); })
  .onHoverExit("go", []{ std::printf("leave\n"); });

auto btn = ui.find("go");
btn->setFocusable(true);
btn->setKeyboardActivates(true);
btn->onRightClick = []{ std::printf("right\n"); };

root->addChild(ui.root());
```

The `.zstyle` for the button (assuming `game.zstyle` is loaded):

```css
.btn-primary {
    background: #0079F1;
    radius: 8px;
    padding: 1.5vh 2vw;
    transition: background 0.25s ease-out-quad, scale 0.12s ease-out-back;
}
.btn-primary:hover   { background: #3296FF; scale: 1.15; }
.btn-primary:pressed { background: #505050; scale: 0.90; }
.btn-primary:focus   { border-color: #FDF900; border-width: 2px; }
```

---

## 14. Interaction with the frame loop

A single frame goes through these stages, in order:

1. `UIContext::beginFrame(dt)` — polls `IPlatform`, updates
   `pointer`, `inputEvents`, `shiftHeld`, and **clears**:
   `topmostConsumer`, `hoverTarget`, `releaseTarget`,
   `clickConsumed`, `rightClickConsumed`, `wheelConsumedThisFrame`.
2. `Layout::updateTree(dt)` — hit-tests the pointer, updates
   `topmostConsumer`, sets `pressTarget` / `releaseTarget`, processes
   `Tab`, calls `requestFocus` / `releaseFocus`.
3. `Layout::update(dt)` — for each node: update flags, compute state,
   fire state-change callbacks and per-frame callbacks
   (`onPress`, `onRelease`, `onClick`, `onRightClick`), run `onUpdate`.
4. At the end of `updateTree`, if the pointer was released:
   `pressTarget = nullptr`.
5. If the pointer is up and capture is set: capture is released.
6. `measure` / `arrange` / `draw`.

The important consequence: **callbacks are fired during `update`, not
during `beginFrame`**. If your `onClick` reads `UIContext::get().pointer`,
it will see the values from the *current* frame, not the frame where the
press started.

---

## 15. Pitfalls

**Non-interactive nodes don't receive events.**

```cpp
auto p = Pan();           // setInteractive(false) by default
p->onClick = []{ ... };   // never fires
```

Make it interactive first:

```cpp
p->setInteractive(true);
p->onClick = []{ ... };
```

`setBlocksRaycast(true)` (which `Panel` also sets by default) is
different — it makes the node **stop** hit-testing at that point, so
nodes behind it don't get events, but the node itself doesn't fire
`onClick` unless it's interactive.

**`onClick` requires both press and release on the same node.**

```cpp
env.platform.pressLeft({50, 50});
env.frame(root);
env.platform.releaseLeft({500, 500});   // moved away
env.frame(root);
// No onClick fires.
```

If you need "press starts an action, release anywhere ends it", use
`onPress` + pointer capture, or check `hasPointerCapture()` in
`onUpdate`.

**Focus is released on pointer down outside a focusable node.**

```cpp
// Focused TextInput
input->setText("hello");

env.click(root, {500, 500});   // click on empty space
// input loses focus, cursor stops blinking, :focus rules stop applying
```

If you want to keep focus during a click elsewhere, you'd need to
override the focus logic — but there's no built-in for that.

**`passThrough` on a focusable node doesn't route focus to the parent.**

`setPassThrough(true)` only affects **click** propagation. It doesn't
change which node gets focus. The innermost focusable node always
becomes focused if it's hit.

**Hover doesn't fire during scroll.**

If you're testing with a mock and scrolling, don't be surprised if
`onHoverEnter` doesn't fire. `scroll_.scrolling` is true during a drag
or inertia, and hover updates are skipped.

**Wheel events on `Stack` containers.**

`ScrollController::tickInput` returns early for `LayoutType::Stack`.
Only `Vertical` and `Horizontal` scroll containers accept wheel input.
If you have a `Stack` with `overflow: scroll`, the wheel won't scroll
it — you need to set the layout type to `Vertical` or `Horizontal`.

**`onRightClick` requires hover at the moment of release.**

Unlike `onClick`, there's no press-then-release matching. If you press
the right button, drag away, then release, the node under the cursor
at release time gets the callback (if the pointer is over one).

**Tab doesn't cycle through non-focusable nodes.**

Only `setFocusable(true)` nodes are visited. If you want a container to
participate in Tab navigation (e.g. to skip over disabled children),
you need to make it focusable, or add a wrapper node. There's no
`tabindex` — nodes are visited in pre-order.

**Callbacks are `void()`. There's no event object.**

To get more information (which mouse button, pointer position, modifier
keys), read `UIContext::get()`:

```cpp
btn->onClick = []{
    auto &ctx = UIContext::get();
    std::printf("clicked at %.0f,%.0f shift=%d\n",
        ctx.pointer.pos.x, ctx.pointer.pos.y, ctx.shiftHeld);
};
```

This is by design — most widgets don't need the details, and dropping
the event object keeps the callback signature simple.