# Input System

The input system is split across three pieces:

- **`UIContext`** — holds the current input snapshot (pointer state,
  keyboard events, modifier keys) and the routing state (focus,
  pointer capture, press/release targets).
- **`InputController`** — a stateless struct with four methods that
  translate the snapshot into per-node flags and callbacks.
- **`Layout::updateTree`** — the orchestrator that calls `hitTest`,
  updates the routing state, and drives the per-node update.

This document covers all three, plus the related concepts of focus
scopes, pointer capture, and the click semantics.

For the user-facing behavior, see [Events](../user/06-events.md).

---

## 1. `UIContext` — the input snapshot

`UIContext` is a singleton. It holds everything that outlives a single
node: the backend pointers, the current frame's input, and the routing
state.

### 1.1 Per-frame input

These fields are overwritten every `beginFrame`:

```cpp
PointerState pointer;
InputEvents inputEvents;
ModifierState modifiers;
float dt{0.0f};
double time{0.0};
float dpiScale{1.0f};
EdgeInsets safeArea;
```

`PointerState`:

```cpp
struct PointerState {
    Vec2 pos;                   // logical coordinates
    float wheelY{0.0f};         // wheel delta this frame

    // Per-button state. Index with (size_t)MouseButton, or via the
    // operator[] overload. Extending MouseButton::Count extends this
    // automatically.
    std::array<ButtonState, (size_t)MouseButton::Count> buttons{};

    ButtonState&       operator[](MouseButton b);
    const ButtonState& operator[](MouseButton b) const;
};

struct ButtonState {
    bool down{false};           // button currently held
    bool pressed{false};        // rising edge this frame
    bool released{false};       // falling edge this frame
};
```

The `MouseButton` enum (`Left`, `Right`, `Middle`, `Count`) is the
canonical index into the array. Adding a new button means adding a
value before `Count` — the framework picks it up everywhere that
iterates the array (see §9.2).

`InputEvents`:

```cpp
struct InputEvents {
    std::vector<int> chars;     // Unicode codepoints typed this frame
    std::vector<int> keys;      // special keys pressed this frame (rising edge)
    std::vector<int> held;      // special keys currently down (level)
};
```

The distinction between `keys` (rising edge) and `held` (level) is
important for keyboard repeat: `keys` fires once per press, `held`
stays true while the key is down.

### 1.2 Per-frame flags

These are cleared at the start of each frame:

```cpp
bool wheelConsumedThisFrame{false};
std::array<bool, (size_t)MouseButton::Count> buttonConsumed{};

void consumeClick(MouseButton b) { buttonConsumed[(size_t)b] = true; }
bool wasConsumed(MouseButton b) const { return buttonConsumed[(size_t)b]; }
```

`wheelConsumedThisFrame` is a single flag (wheel events don't have a
per-axis consumer concept that needs separating).

`buttonConsumed` is per-button: pressing Middle while a Left press is
in flight doesn't interfere, and a node can consume `Right` without
suppressing the pending `Left` click. The two helpers `consumeClick` /
`wasConsumed` are the only public API and are used by
`InputController::fireCallbacks` and by user code that wants to mark
an event as handled manually.

They're set by the first consumer that handles an event, and checked by
later nodes to avoid double-handling. See §5.3.

### 1.3 Routing state

These are cleared in `beginFrame` and set during `updateTree`:

```cpp
Layout* topmostConsumer{nullptr};   // hit-test result
Layout* hoverTarget{nullptr};       // same as topmostConsumer, kept for clarity
Layout* pressTarget{nullptr};       // fixed at pointer-down
Layout* releaseTarget{nullptr};     // fixed at pointer-up
```

They're raw pointers, not `shared_ptr`. Lifetime is managed by the tree
— a `Layout` that's in the tree is alive; one that's been removed has
`internal_.wantsRemoval` set and will be culled. A dangling raw pointer to a
culled node is possible in theory but never happens in practice, because
`cullRemovedChildren` runs at the end of `update`, after all uses of
these pointers.

The exception: `pointerCapture` is a `weak_ptr`:

```cpp
std::weak_ptr<Layout> pointerCapture;
```

Because capture can span multiple frames (a drag), and the captured
node could be removed mid-drag. Using `weak_ptr` means a destroyed node
automatically releases capture.

### 1.4 Focus

```cpp
std::weak_ptr<Layout> focusedNode;
```

`focusedNode` is a `weak_ptr` for the same reason: focus can persist
across frames, and the focused node might be removed (e.g. a modal that
closes).

`requestFocus` and `releaseFocus` are the mutators:

```cpp
void UIContext::requestFocus(std::shared_ptr<Layout> n) {
    if (n && n->isFocusable())
        focusedNode = n;
    else
        focusedNode.reset();
}

void releaseFocus() { focusedNode.reset(); }

bool hasFocus(const Layout* n) const {
    auto sp = focusedNode.lock();
    return sp && sp.get() == n;
}
```

`requestFocus` silently refuses non-focusable nodes. `isFocusable()`
returns `config.focusable != 0 && config.enabled != 0` — so a disabled node can't be
focused.

---

## 2. `InputController` — the stateless translator

`InputController` is a struct with four methods. It has no state: all
state lives in `Layout` and `UIContext`.

### 2.1 `updateFlags(node, selfBlocked, scrolling)`

Called once per node per frame, from `Layout::update`.

```cpp
void InputController::updateFlags(Layout& node, bool selfBlocked, bool scrolling) {
    auto& ctx = UIContext::get();
    auto& pointer = ctx.pointer;

    if (scrolling) {
        // Freeze hover during scroll
        bool focusNow = ctx.hasFocus(&node);
        if (focusNow != (node.state.focused != 0)) {
            node.state.focused = focusNow ? 1 : 0;
            node.internal_.pendingTransition = 1;
        }
        return;
    }

    node.state.hovered = 0;
    if (!selfBlocked && node.rect.width > 0 && node.rect.height > 0)
        node.state.hovered = node.rect.contains(pointer.pos) ? 1 : 0;

    node.state.pressed = 0;
    if (pointer[MouseButton::Left].down && ctx.pressTarget) {
        Layout* n = ctx.pressTarget;
        while (n) {
            if (n == &node) { node.state.pressed = 1; break; }
            if (!n->getPassThrough()) break;
            n = n->getParent().get();
        }
    }

    bool focusNow = ctx.hasFocus(&node);
    if (focusNow != (node.state.focused != 0)) {
        node.state.focused = focusNow ? 1 : 0;
        node.internal_.pendingTransition = 1;
    }
}
```

Three flags are updated:

**`state.hovered`** — true if the pointer is inside the node's `rect`, the
node isn't self-blocked, and it has non-zero size. Note this is a
**geometric** check against the node's own rect, not the hit-test
result. So a node that's covered by a sibling still gets `state.hovered =
true` if the pointer is over its rect. This is a known simplification:
`.zstyle` `:hover` matches based on the node's own rect, not on
whether it's the topmost node.

The rationale: it's cheap (no recursion) and matches most users'
expectations (hovering over a parent still hovers the child if the
pointer is within both rects). The alternative — using
`topmostConsumer` — would only hover the single topmost node, which
makes `:hover` on containers impossible.

**`state.pressed`** — true if the pointer is down **and** the current press
target is this node or an ancestor reachable through `config.passThrough`
chains. The loop walks up from `pressTarget` until it finds `node` or
hits a non-`config.passThrough` boundary. This is what makes
`config.passThrough = 1` work for press/hold states.

Note it walks **up** from `pressTarget`, not down from `node`. This is
intentional: the press target is fixed at pointer-down, so a node
further up the tree can still be considered "pressed" if the press
target is one of its descendants that has config.passThrough = 1.

**`state.focused`** — true if `UIContext::focusedNode` points to this
node. When this changes, `internal_.pendingTransition = 1` triggers a style
re-resolve so `:focus` rules apply.

The `scrolling` parameter is the "hover freeze": during a scroll, only
`state.focused` is updated, not `state.hovered` or `state.pressed`. This prevents
`onHoverEnter`/`onHoverExit` from firing on every row that passes
under a stationary cursor.

### 2.2 `computeNextState(node)`

```cpp
UIState InputController::computeNextState(const Layout& node) const {
    if (!node.config.enabled) return UIState::Disabled;
    if (node.state.pressed) return UIState::Pressed;
    if (node.state.hovered) return UIState::Hover;
    return UIState::Idle;
}
```

The state is derived from the flags in strict priority order:

1. `Disabled` — overrides everything.
2. `Pressed` — overrides `Hover`.
3. `Hover`.
4. `Idle`.

This state is used by `StyleResolver::beginStateTransition` to know
when a state change happened.

### 2.3 `handleKeyInput(node)`

```cpp
void InputController::handleKeyInput(Layout& node) {
    if (!node.state.focused || !node.config.enabled || !node.config.keyboardActivates)
        return;

    auto& ev = UIContext::get().inputEvents;
    for (int k : ev.keys) {
        if (k == Key::Enter || k == Key::Space) {
            if (node.onClick) node.onClick();
        }
    }
}
```

Fires `onClick` on `Enter` or `Space` for focused nodes that have
`config.keyboardActivates` set. Uses `ev.keys` (rising edge), so it fires
once per key press.

Only `Enter` and `Space` are handled. Other keys are the widget's
responsibility (e.g. `TextInput` handles `Backspace`, `Delete`, arrow
keys in its own `onUpdate`).

Note the `onClick` callback is invoked directly, **without** the
consumption mechanism used by pointer clicks. So a `Button` with
`config.keyboardActivates` that's focused fires `onClick` on `Enter` and the
event is not propagated further. This is correct for keyboard, since
there's no hierarchy of focused nodes.

### 2.4 `fireCallbacks(node, prev, next, stateChanged)`

The most complex method. Handles hover transitions, press, release,
click, and right-click.

```cpp
void InputController::fireCallbacks(Layout& node, UIState prevState,
                                     UIState nextState, bool stateChanged)
{
    auto& ctx = UIContext::get();
    auto& pointer = ctx.pointer;

    // ---- Hover transitions ----
    // Hover and Pressed are both "hovered" for the purpose of the
    // enter/exit callbacks: pressing a node is a continuation of
    // hovering it, not an exit.
    if (stateChanged) {
        bool wasHovered = (prevState == UIState::Hover ||
                           prevState == UIState::Pressed);
        bool isHovered  = (nextState == UIState::Hover ||
                           nextState == UIState::Pressed);

        if (isHovered && !wasHovered && node.onHoverEnter)
            node.onHoverEnter();
        else if (!isHovered && wasHovered && node.onHoverExit)
            node.onHoverExit();
    }

    // ---- Click-based buttons: Left and Middle ----
    // Both follow the standard press+release click model. The legacy
    // onPress / onRelease / onClick callbacks fire only for Left (the
    // historical API); the unified onButton* fire for both.
    auto fireClickButton = [&](MouseButton button, bool isLeftLegacy)
    {
        const auto& btn = pointer[button];

        // --- Press ---
        if (btn.pressed && !ctx.wasConsumed(button))
        {
            bool inPressPath = (ctx.pressTarget == &node) ||
                               node.isAncestorOf(ctx.pressTarget);
            if (inPressPath)
            {
                if (isLeftLegacy && node.onPress)
                    node.onPress();
                if (node.onButtonPress)
                    node.onButtonPress(button);
                if (!node.config.passThrough)
                    ctx.consumeClick(button);
            }
        }

        // --- Release + Click ---
        if (btn.released)
        {
            bool pressedHere  = (ctx.pressTarget == &node)   ||
                                node.isAncestorOf(ctx.pressTarget);
            bool releasedHere = (ctx.releaseTarget == &node) ||
                                node.isAncestorOf(ctx.releaseTarget);
            bool validClick   = pressedHere && releasedHere;

            if (validClick)
            {
                if (isLeftLegacy && node.onRelease)
                    node.onRelease();
                if (node.onButtonRelease)
                    node.onButtonRelease(button);

                if (!ctx.wasConsumed(button))
                {
                    if (isLeftLegacy && node.onClick)
                        node.onClick();
                    if (node.onButtonClick)
                        node.onButtonClick(button);
                    if (!node.config.passThrough)
                        ctx.consumeClick(button);
                }
            }
        }
    };

    fireClickButton(MouseButton::Left,   /*isLeftLegacy=*/true);
    fireClickButton(MouseButton::Middle, /*isLeftLegacy=*/false);

    // ---- Right button: onRightClick ----
    // Different semantic: fires on press while hovered, without
    // requiring a valid press+release pair. Right is a one-shot event
    // (typically used for context menus). onButton* don't fire for
    // Right.
    {
        const auto& btn = pointer[MouseButton::Right];

        if (btn.pressed && node.state.hovered && node.onRightClick)
        {
            if (!ctx.wasConsumed(MouseButton::Right))
            {
                node.onRightClick();
                if (!node.config.passThrough)
                    ctx.consumeClick(MouseButton::Right);
            }
        }
    }
}
```

Let's break down each phase.

**Hover transitions** fire only on `stateChanged`, and only when the
transition crosses the "hovered set" (`Hover ∪ Pressed`). So
`Idle → Pressed` (which can't happen because `Pressed` implies
`Hover`) and `Hover → Pressed` (pressing a button) don't fire
`onHoverEnter`/`onHoverExit`. Only a transition that enters or leaves
both states does.

**Press** fires on the per-button rising edge (`pointer[button].pressed`)
if the press path includes this node. The path check:

```cpp
bool inPressPath = (ctx.pressTarget == &node) || node.isAncestorOf(ctx.pressTarget);
```

`pressTarget` is set in `updateTree` to `topmostConsumer` on the frame
of any press. A node fires `onPress` if it **is** the press target, or
if the press target is a descendant. The `isAncestorOf` check walks up
the parent chain.

For each button that uses the click model (Left, Middle), the same
logic runs. `isLeftLegacy` controls whether the historical
`onPress`/`onRelease`/`onClick` callbacks fire — they only fire for
Left. The unified `onButtonPress`/`onButtonRelease`/`onButtonClick`
fire for both.

The consumption: if the node doesn't have `config.passThrough`, it sets
`buttonConsumed[(size_t)button] = true`. Subsequent nodes in the tree
walk (which proceeds bottom-up within a subtree) see the flag and skip
their own handling of *that button*. The Left and Middle flags are
independent, so a node that consumes Left doesn't suppress Middle.

**Release + Click**: similar logic, but requires both press and release
to include the node. The click is validated only if:

- `pressedHere`: the press target is this node or a descendant.
- `releasedHere`: the release target is this node or a descendant.

Both must be true. Then `onRelease` (and `onButtonRelease`) fires
unconditionally when the click is valid, and `onClick`/`onButtonClick`
fire only if not consumed.

**Right-click** fires on `pointer[MouseButton::Right].pressed` if the
node is currently hovered and has `onRightClick`. Unlike Left/Middle,
there's no press/release validation — right-click fires immediately on
press. And it uses `state.hovered` (geometric check), not the hit-test
result.

The consumption uses the same per-button flag: `wasConsumed(Right)` /
`consumeClick(Right)`. `onButton*` callbacks do **not** fire for Right:
its semantic is deliberately different.

### 2.5 Order of the phases

`fireCallbacks` runs the phases in this order:

1. Hover transitions.
2. `fireClickButton(Left,  isLeftLegacy=true)`  — press, then release+click.
3. `fireClickButton(Middle, isLeftLegacy=false)` — press, then release+click.
4. Right-click.

In practice, only one event fires per frame because the per-button
edges are one-frame. On the rare frame where two buttons change state
together (e.g. release Left and press Right), the order above is fixed.

---

## 3. `updateTree` — the orchestrator

Called on the root. Sets up the routing state, then calls `update`.

### 3.1 Hit-test

```cpp
auto captured = ctx.pointerCapture.lock();
if (!captured || !captured->getEnabled()) {
    ctx.pointerCapture.reset();
    ctx.topmostConsumer = hitTest(ctx.pointer.pos, false);
} else {
    ctx.topmostConsumer = captured.get();
}
```

Two cases:

- **No capture, or capture released**: normal hit-test.
- **Capture active and enabled**: bypass hit-test, use the captured
  node.

The `!captured->getEnabled()` check handles the case where a node was
captured and then disabled. `setEnabled(false)` already releases
capture (see `Layout::setEnabled`), but this is a safety net.

If the captured node was destroyed (weak_ptr expired), `lock()`
returns null and we fall through to normal hit-test.

### 3.2 Target updates

```cpp
ctx.hoverTarget = ctx.topmostConsumer;

// Any button edge updates the targets. A press on ANY button makes
// pressTarget point to whatever was hit; a release on ANY button
// makes releaseTarget point to whatever was hit.
bool anyPressed  = false;
bool anyReleased = false;
bool anyDown     = false;
for (size_t i = 0; i < (size_t)MouseButton::Count; ++i)
{
    const auto& b = ctx.pointer.buttons[i];
    if (b.pressed)  anyPressed  = true;
    if (b.released) anyReleased = true;
    if (b.down)     anyDown     = true;
}

if (anyPressed)  ctx.pressTarget   = ctx.hoverTarget;
if (anyReleased) ctx.releaseTarget = ctx.hoverTarget;
```

`pressTarget` and `releaseTarget` are single pointers, not per-button.
The per-button validation happens later, in `fireCallbacks`, where each
button's `pressed`/`released` edge is checked against the same targets.
This means `fireCallbacks` uses `pressedHere`/`releasedHere` as "the
press/release happened on this subtree", without caring which button
triggered the target update. `anyDown` is used in §3.6 to release
pointer capture automatically.

- `hoverTarget` is set every frame.
- `pressTarget` is only updated on the frame of a press.
- `releaseTarget` is only updated on the frame of a release.

This means `pressTarget` persists across frames between press and
release. It's cleared at the end of `updateTree` (see §3.6).

### 3.3 Focus from click

```cpp
if (ctx.pointer[MouseButton::Left].pressed) {
    if (ctx.topmostConsumer && ctx.topmostConsumer->isFocusable()) {
        ctx.requestFocus(ctx.topmostConsumer->shared_from_this());
        notifyFocusAncestors(ctx.focusedNode.lock().get());
    } else {
        ctx.releaseFocus();
    }
}
```

Focus changes only on Left press. Middle and Right don't move focus.
`notifyFocusAncestors` fires the `onDescendantFocused` hook on every
ancestor of the newly focused node — same as in the Tab path. This
makes click-focus and Tab-focus symmetric (see §4.6 in the user guide).

Clicking a focusable node focuses it. Clicking anywhere else releases
focus. This is the only place a click changes focus.

`isFocusable()` returns `config.focusable != 0 && config.enabled != 0`, so a disabled
node can't be focused by clicking.

### 3.4 Tab navigation

```cpp
for (int k : ctx.inputEvents.keys) {
    if (k != Key::Tab) continue;

    Layout* scope = this;
    {
        auto focused = ctx.focusedNode.lock();
        if (focused) {
            Layout* n = focused.get();
            while (n) {
                if (n->isFocusScope()) { scope = n; break; }
                n = n->getParent().get();
            }
        }
    }

    std::vector<Layout*> focusables;
    collectFocusables(scope, focusables);
    if (focusables.empty()) break;

    auto cur = ctx.focusedNode.lock();
    Layout* curPtr = cur.get();

    int idx = -1;
    for (size_t i = 0; i < focusables.size(); ++i) {
        if (focusables[i] == curPtr) { idx = (int)i; break; }
    }

    int next = 0;
    if (idx >= 0) {
        int n = (int)focusables.size();
        next = ctx.modifiers.shift ? (idx - 1 + n) % n : (idx + 1) % n;
    }
    ctx.requestFocus(focusables[next]->shared_from_this());
    notifyFocusAncestors(ctx.focusedNode.lock().get());
}
```

Steps:

1. Find the focus scope: walk up from the currently focused node until
   a node with `isFocusScope()` is found, or fall back to the root.
2. Collect all focusable nodes within that scope, in pre-order.
3. Find the current focus index.
4. Compute the next index (`+1` or `-1` with `Shift`, modulo size).
5. Focus the next node.
6. Notify all ancestors of the newly focused node.

The `break` after the switch means only the first Tab event per frame
is processed. Multiple tabs in the same frame collapse to one.

`notifyFocusAncestors` walks up from the newly focused node and calls
`notifyDescendantFocused` on each ancestor. This fires the
`onDescendantFocused` virtual hook.

**Note:** `collectFocusables` is pre-order DFS, so a container that's
focusable itself is visited **before** its children. This means a
focusable container with focusable children will have the container
visited first in Tab order. In practice, containers are rarely
focusable, so this doesn't matter much.

### 3.5 Recursive update

```cpp
update(dt, false);
```

The single call that drives the per-node `update` recursion. All the
input processing, style resolution, and animation ticking happens
inside this call. See [Lifecycle §4](02-lifecycle.md) for the full
sequence.

### 3.6 Post-update cleanup

```cpp
if (anyReleased)
    ctx.pressTarget = nullptr;
if (!anyDown && !ctx.pointerCapture.expired())
    ctx.pointerCapture.reset();
```

`anyReleased` and `anyDown` are the aggregate flags computed in §3.2.
`pressTarget` is cleared on any release (not just Left), so a
multi-button sequence like "press Left, press Right, release Right,
release Left" leaves `pressTarget` valid for the second release.
Capture is only released when **all** buttons are up — a node holding
capture during a drag isn't released by a Middle press elsewhere.

- After the frame where a release happens, `pressTarget` is cleared.
  This is what makes `pressTarget == nullptr` on the frame **after**
  a release — the click validation is done, so the pointer is no
  longer "pressing" anything.
- If the pointer is up and capture is still set, clear it. This is the
  automatic release: a widget doesn't need to call `releasePointer()`
  explicitly; capture is dropped when the button is released.

**Ordering matters:** `pressTarget` is cleared **after** `update`
runs, so during `update` both `pressTarget` and `releaseTarget` are
available for click validation.

---

## 4. Focus

### 4.1 `isFocusable()` vs `isFocusScope()`

Two separate concepts:

- **`isFocusable()`** — the node itself can receive focus (via click,
  Tab, or `requestFocus`). Returns `isFocusable_ && isEnabled`.
- **`isFocusScope()`** — the node defines a boundary for Tab navigation.
  Tab cycles through focusables **within** the scope, not across it.

A node can be both, neither, or either. In practice:

- Interactive widgets (Button, TextInput, Toggle, Checkbox) are
  focusable.
- Containers (Modal, SettingsScreen) are focus scopes but not
  focusable themselves.

### 4.2 `requestFocus` / `releaseFocus`

```cpp
void UIContext::requestFocus(std::shared_ptr<Layout> n) {
    if (n && n->isFocusable())
        focusedNode = n;
    else
        focusedNode.reset();
}

void releaseFocus() { focusedNode.reset(); }
```

`requestFocus(null)` is equivalent to `releaseFocus()`. Both silently
succeed regardless of the current state.

The node's `state.focused` flag isn't updated here — it's updated in
`updateFlags` on the next `update`. So there's a one-frame delay
between `requestFocus` and `:focus` styling applying.

Actually, `requestFocus` is called during `updateTree`, before `update`.
So within the same frame:
1. `requestFocus` sets `focusedNode` to the new node.
2. `update` runs, and `updateFlags` sets `state.focused` on the new node
   (and clears it on the old one).
3. `computeNextState` sees the new flag.
4. `beginStateTransition` resolves the `:focus` style.
5. Draw uses the new style.

So the delay is zero frames in the normal case. The node is focused
and styled in the same frame.

### 4.3 Focus and `setEnabled(false)`

`setEnabled(false)` on a node that's currently focused clears focus:

```cpp
void setEnabled(bool e) {
    if ((config.enabled != 0) == e) return;
    config.enabled = e ? 1 : 0;
    internal_.pendingTransition = 1;

    if (!e) {
        auto& ctx = UIContext::get();
        if (ctx.focusedNode.lock().get() == this)
            ctx.focusedNode.reset();
        if (ctx.pointerCapture.lock().get() == this)
            ctx.pointerCapture.reset();
    }

    onEnabledChanged(e);
}
```

The same for pointer capture. This ensures a disabled node doesn't
hold focus or capture, which could otherwise be used to bypass the
disabled check in some code paths.

### 4.4 Focus scope resolution

```cpp
Layout* scope = this;   // default: the root
auto focused = ctx.focusedNode.lock();
if (focused) {
    Layout* n = focused.get();
    while (n) {
        if (n->isFocusScope()) { scope = n; break; }
        n = n->getParent().get();
    }
}
```

Walk up from the focused node until a focus scope is found. If no
focused node, or no scope above it, the scope is the root.

The scope is used as the starting point for `collectFocusables`. So a
Tab inside a modal (which is a focus scope) doesn't reach nodes outside
the modal.

### 4.5 `collectFocusables`

```cpp
static void collectFocusables(Layout* node, std::vector<Layout*>& out) {
    if (node->isFocusable()) out.push_back(node);
    for (auto& c : node->children)
        collectFocusables(c.get(), out);
}
```

Pre-order DFS. The order is:

1. The node itself, if focusable.
2. Each child, recursively, in declaration order.

So a `VStack` with three buttons and an inner `HStack` with two more
yields: `[button1, button2, button3, innerButton1, innerButton2]`.

### 4.6 `notifyFocusAncestors`

```cpp
static void notifyFocusAncestors(Layout* node) {
    if (!node) return;
    auto parent = node->getParent();
    while (parent) {
        parent->notifyDescendantFocused(node);
        parent = parent->getParent();
    }
}
```

Called after `requestFocus` in the Tab path. Walks up from the newly
focused node and calls `notifyDescendantFocused` on each ancestor. The
`onDescendantFocused` hook lets containers react to focus changes in
their subtree.

**Note:** this is only called from the Tab navigation path. Clicking a
focusable node doesn't call it. This is an asymmetry — a container
that wants to react to focus via click would need to override
`requestFocus` or check `focusedNode` in its `onUpdate`.

---

## 5. Click semantics in detail

### 5.1 The three targets

The click validation uses three pointers:

- `pressTarget` — set at pointer-down to `topmostConsumer`.
- `releaseTarget` — set at pointer-up to `topmostConsumer`.
- `topmostConsumer` — recomputed every frame by hit-test (or by capture).

A click is valid if `pressTarget` and `releaseTarget` are the same
node, **or** they're in an ancestor/descendant relationship within the
subtree.

Actually, looking at the code:

```cpp
bool pressedHere = (ctx.pressTarget == &node) || node.isAncestorOf(ctx.pressTarget);
bool releasedHere = (ctx.releaseTarget == &node) || node.isAncestorOf(ctx.releaseTarget);
bool validClick = pressedHere && releasedHere;
```

For a given node, `pressedHere` is true if `pressTarget` is the node
itself or a descendant. `releasedHere` is similar. The click is valid
if both are true.

This means:

- **Same node**: `pressTarget == releaseTarget == node`. Both true.
- **Press on child, release on child**: same as above, but at the child.
- **Press on child, release on parent**: child's click: `pressedHere`
  true, `releasedHere` false (release is on an ancestor, not a
  descendant). Parent's click: `pressedHere` true (press is on a
  descendant), `releasedHere` true (release is on itself). So the
  parent's click is valid, but the child's isn't.
- **Press on parent, release on child**: child's click: `pressedHere`
  false (`pressTarget` is not the child or a descendant). Parent's
  click: `pressedHere` true (self), `releasedHere` true (descendant
  released). So the parent's click is valid.

So the click always "resolves" to the deepest common ancestor of press
and release. In practice, if press and release are on the same node,
that node fires. If they're on different nodes, their common ancestor
fires (if it's interactive).

The typical drag-away-and-back-in case:

- Press on button A, drag to button B, release on button B.
- A's click: `pressedHere` true, `releasedHere` false → no click.
- B's click: `pressedHere` false, `releasedHere` true → no click.
- Root's click: `pressedHere` true (A is a descendant), `releasedHere`
  true (B is a descendant) → root's click fires if root is interactive.

So a drag from A to B doesn't click either button, but might click a
container. This is usually what you want.

### 5.2 Bubbling order

The tree is traversed depth-first, with children updated **before**
their parents (the recursion in `Layout::update` is
`children[i]->update(...)` before `onUpdate` on the parent). Actually
no — looking at the order:

```cpp
for (size_t i = 0; i < n; ++i)
    children[i]->update(dt, blockSubtree, scrolling);

scroll_.tickInput(*this);
input_.handleKeyInput(*this);
input_.fireCallbacks(*this, prevState, nextState, stateChanged);
```

Children are updated (recursively) first. Each child, in its own
`update`, calls `fireCallbacks` on itself. So the callbacks fire
bottom-up: the deepest node fires first, then its parent, etc.

The consumption mechanism then prunes the propagation: once a node
consumes the click, its ancestors skip `onClick` (but still fire
`onRelease` if valid).

So for a click on a deeply nested button, the order is:

1. Deepest node fires (if it's a target). Consumes.
2. Its parent fires (skipped due to consumption for `onClick`, but
   `onRelease` fires).
3. ... up to the root, all skipped for `onClick`.

For a `config.passThrough = 1` chain:

1. Deepest node fires `onClick`. Doesn't consume (`config.passThrough`).
2. Parent fires `onClick`. Doesn't consume (if it's `config.passThrough`).
3. ... all the way up.

### 5.3 Consumption

```cpp
std::array<bool, (size_t)MouseButton::Count> buttonConsumed{};
void consumeClick(MouseButton b) { buttonConsumed[(size_t)b] = true; }
bool wasConsumed(MouseButton b) const { return buttonConsumed[(size_t)b]; }
```

`buttonConsumed` is cleared at the start of each frame. The flag is
per-button: consuming Left doesn't affect Middle or Right. It's set by
`fireCallbacks` when a node fires the corresponding click callback
**and** isn't `config.passThrough`.

The check is done **before** calling `onClick`:

```cpp
if (!ctx.wasConsumed(button)) {
    if (isLeftLegacy && node.onClick)
        node.onClick();
    if (node.onButtonClick)
        node.onButtonClick(button);
    if (!node.config.passThrough)
        ctx.consumeClick(button);
}
```

So a node fires `onClick` only if no previous node (in the bottom-up
order) has consumed. And after firing, if the node isn't passThrough,
it consumes.

The same logic applies to `onPress` — it uses the same
`buttonConsumed[button]` flag as `onClick`, per button.

There's a subtle interaction: `onPress` fires on
`pointer[button].pressed` and can consume, setting
`buttonConsumed[button]`. On `pointer[button].released` (usually a
different frame), the same `buttonConsumed[button]` slot has been
cleared by `beginFrame`, so `onClick` fires independently. The
Left and Middle flags are separate; consuming Left has no effect on
the Middle flag.

So the "click consumed" state is per-frame **and per-button**, not
persistent. A press in frame N and a release in frame N+1 both use the
fresh flag of their respective frames, and a press on Middle doesn't
touch the Left flag.

### 5.4 Right-click

```cpp
if (pointer[MouseButton::Right].pressed &&
    node.state.hovered &&
    node.onRightClick)
{
    if (!ctx.wasConsumed(MouseButton::Right)) {
        node.onRightClick();
        if (!node.config.passThrough)
            ctx.consumeClick(MouseButton::Right);
    }
}
```

Right-click fires on the **hovered** node, not on the press target.
Since right-click uses `rightPressed` (rising edge), there's no
press/release validation. The hovered node gets the callback
immediately.

The walk order is the same (bottom-up), and consumption applies. So
the deepest hovered node with `onRightClick` fires first, and
ancestors are skipped unless the node is `config.passThrough = 1`.

Since `state.hovered` is geometric (pointer inside the node's rect), and
children are updated before parents, the deepest node whose rect
contains the pointer fires first. Usually correct.

---

## 6. Pointer capture

### 6.1 Capture API

```cpp
void Layout::capturePointer() {
    UIContext::get().pointerCapture = shared_from_this();
}

void Layout::releasePointer() {
    auto& ctx = UIContext::get();
    auto sp = ctx.pointerCapture.lock();
    if (sp.get() == this) ctx.pointerCapture.reset();
}

bool Layout::hasPointerCapture() const {
    return UIContext::get().pointerCapture.lock().get() == this;
}
```

Capture is a `weak_ptr<Layout>` in `UIContext`. Setting it to
`shared_from_this()` captures a weak reference. The `Layout` that
captured is kept alive by the tree (it's in a parent's `children`), so
the weak ref is valid as long as the node is alive.

### 6.2 Effect of capture

When `pointerCapture` is active and the captured node is enabled:

1. `updateTree` uses the captured node as `topmostConsumer` instead of
   running hit-test.
2. So `hoverTarget = captured`, and on `pressed`/`released`, the
   targets are the captured node.

This means all pointer events are routed to the captured node,
**regardless of the pointer position**. A drag that leaves the node's
rect still delivers events to it.

### 6.3 Automatic release

```cpp
bool anyDown = false;
for (size_t i = 0; i < (size_t)MouseButton::Count; ++i)
    if (ctx.pointer.buttons[i].down) { anyDown = true; break; }

if (!anyDown && !ctx.pointerCapture.expired())
    ctx.pointerCapture.reset();
```

At the end of `updateTree`, if the pointer isn't down, capture is
released. So a widget doesn't need to call `releasePointer()` — it's
dropped automatically on mouse-up.

The widget still needs to check `hasPointerCapture()` in its
`onUpdate` to know whether it's currently capturing.

### 6.4 The capture pattern (drag)

Standard drag pattern:

```cpp
void onUpdate(float) override {
    // Start capture on press
    if (state.pressed && !hasPointerCapture())
        capturePointer();

    // While captured, do the drag
    if (hasPointerCapture()) {
        Vec2 p = UIContext::get().pointer.pos;
        // process p
    }
    // No explicit release: capture is dropped on pointer-up
}
```

The `state.pressed && !hasPointerCapture()` check starts a new capture only
on the first frame of the press. Subsequent frames have
`hasPointerCapture()` true, so the first branch is skipped.

If the widget wants to end the drag early (e.g. on `Escape`), it calls
`releasePointer()` explicitly.

### 6.5 Capture vs focus

Capture and focus are independent. A node can be captured without being
focused (a slider drag doesn't focus the slider), and can be focused
without being captured.

When `setEnabled(false)` is called on a captured node, both focus and
capture are cleared:

```cpp
if (ctx.pointerCapture.lock().get() == this)
    ctx.pointerCapture.reset();
```

This prevents a disabled node from holding capture.

---

## 7. Keyboard input

### 7.1 The two channels

Keyboard events come in two flavors:

- `ev.keys` — rising edge events. `Key::Enter`, `Key::Space`,
  `Key::Backspace`, etc. Fires once per press. Used by
  `handleKeyInput` (for Enter/Space activation), and by widget code
  that wants per-press handling (e.g. `TextInput` for `Escape`, arrow
  keys, etc.).
- `ev.held` — level events. The keys currently down. Used for repeat
  behavior. `TextInput` uses `held` to implement key repeat for
  `Backspace`, `Delete`, arrows.

`TextInput`'s repeat logic:

```cpp
auto handleRepeat = [&](int key) {
    bool isHeld = std::find(ev.held.begin(), ev.held.end(), key) != ev.held.end();
    auto it = repeatStates.find(key);

    if (!isHeld) {
        if (it != repeatStates.end()) repeatStates.erase(it);
        return;
    }
    if (it == repeatStates.end()) {
        doAction(key);
        RepeatState st;
        st.holdTime = 0.0f;
        st.nextFireTime = REPEAT_DELAY;
        repeatStates[key] = st;
    } else {
        it->second.holdTime += dt;
        while (it->second.holdTime >= it->second.nextFireTime) {
            doAction(key);
            it->second.nextFireTime += REPEAT_INTERVAL;
        }
    }
};
```

On the first frame the key is held, the action fires immediately and
the state is created. On subsequent frames, `holdTime` accumulates and
`doAction` fires every `REPEAT_INTERVAL` seconds after `REPEAT_DELAY`.

### 7.2 Character input

`ev.chars` contains Unicode codepoints for characters typed this frame.
It's not the same as `keys` — `chars` comes from the platform's text
input (e.g. `GetCharPressed` in Raylib), while `keys` comes from
physical key presses.

A `TextInput` reads `chars` for insertions:

```cpp
for (int c : ev.chars) {
    if (c < 32 || c > 126) continue;
    text.insert(text.begin() + cursorPos, (char)c);
    cursorPos++;
    changed = true;
}
```

The `c < 32 || c > 126` filter restricts input to printable ASCII.
Support for Unicode would require `std::u32string` or similar — not
currently implemented.

`ev.chars` and `ev.keys` can overlap: pressing `A` produces both a
char `'a'` (or `'A'` with shift) and possibly a key code if the
backend maps it. In the Raylib backend, only the keys in `kMap` are
mapped to `Key::*` codes, and `A` isn't among them.

### 7.3 Modifier keys

```cpp
struct ModifierState {
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
    bool super{false};
};
```

Updated from `platform->modifiers()` each `beginFrame`. All four
modifiers are populated by the platform. The framework itself only
reads `shift`, for three purposes:

- Tab navigation (reverse direction).
- Scroll (horizontal wheel).
- `Slider` (fine step).

`ctrl`, `alt`, and `super` are exposed for user code that wants them:
read `UIContext::get().modifiers.ctrl` in a callback or `onUpdate`:

```cpp
if (UIContext::get().modifiers.ctrl && IsKeyPressed(KEY_S))
    save();
```

Adding a new modifier means adding a bool to `ModifierState` and
populating it in each backend's `IPlatform::modifiers()`. The framework
doesn't need changes.

---

## 8. Interaction with the frame pipeline

### 8.1 When flags are updated

`updateFlags` is called for each node during `update`. The order:

1. `updateTree` sets `topmostConsumer`, `pressTarget`, `releaseTarget`.
2. `update` recurses into children.
3. Each node's `update` calls `updateFlags`, which reads the current
   state and sets `state.hovered`, `state.pressed`, `state.focused`.
4. `computeNextState` derives the UI state from the flags.
5. `beginStateTransition` (if state changed) or
   `resolvePendingTransition` (if a property changed) re-resolves the
   style.
6. `fireCallbacks` fires the input callbacks.

So the flag update happens in step 3, and the callbacks in step 6, all
within the same node's update. By the time a callback fires, the flags
are up to date.

### 8.2 When callbacks fire relative to children

Children are updated (recursively) before their parent's callbacks.
So the callback order for a subtree is bottom-up: deepest first.

This matches the bubbling semantics: a child's `onClick` fires before
its parent's, and if the child consumes, the parent doesn't fire
`onClick`.

### 8.3 When focus changes

Focus changes happen in `updateTree`, **before** `update`. So when the
recursion runs, the new focus is already set, and each node's
`updateFlags` picks up the change.

The `state.focused` flag is set in `updateFlags` (step 3), and
`internal_.pendingTransition = 1` is set if the flag changed. So the style
re-resolve happens in the same frame.

### 8.4 When capture changes

Capture changes happen in user code (widget's `onUpdate`) or
automatically at the end of `updateTree`. If a widget calls
`capturePointer()` during its `onUpdate`, the capture takes effect on
the **next** frame's `updateTree` (since `updateTree` already ran and
set `topmostConsumer` for this frame).

In practice, the drag pattern works because:

- Frame N: press, `state.pressed` becomes 1, widget captures.
- Frame N+1: `updateTree` sees capture, uses captured node as
  `topmostConsumer`, delivers events.

The pointer-down and the capture happen in the same frame (during the
widget's `onUpdate`), but the effect is only visible next frame. This
is fine because the pointer is still down on frame N, and the widget
already received the press event.

---

## 9. Common tasks

### 9.1 Adding a new key code

1. Add a constant in `Key::` namespace in `CoreTypes.hpp`:

   ```cpp
   constexpr int Ctrl = 11;
   ```

2. Map it in the platform's `pollInputEvents` (e.g. in
   `RaylibPlatform::pollInputEvents`, add to `kMap`).

3. Widgets that care about it check `ev.keys` or `ev.held` for the
   constant.

### 9.2 Adding a new pointer button

Thanks to the per-button refactor, extending the framework to a new
mouse button is now a two-step change:

1. **Add a value to the `MouseButton` enum** in `CoreTypes.hpp`,
   before `Count`:

   ```cpp
   enum class MouseButton : uint8_t {
       Left,
       Right,
       Middle,
       Back,        // new
       Forward,     // new
       Count
   };
   ```

2. **Populate it in the backend's `PointerState`** inside
   `IPlatform::pointer()`. In `RaylibPlatform`:

   ```cpp
   auto& back = s[MouseButton::Back];
   back.down     = IsMouseButtonDown(MOUSE_BUTTON_BACK);
   back.pressed  = IsMouseButtonPressed(MOUSE_BUTTON_BACK);
   back.released = IsMouseButtonReleased(MOUSE_BUTTON_BACK);
   ```

That's it. The framework propagates the new button automatically:

- `PointerState::buttons` grows because it's a `std::array` sized by
  `MouseButton::Count`.
- `UIContext::buttonConsumed` grows for the same reason.
- `updateTree`'s `anyPressed`/`anyReleased`/`anyDown` loop picks it up
  because it iterates `MouseButton::Count`.
- `fireClickButton` can be called for the new button with a one-line
  addition:

  ```cpp
  fireClickButton(MouseButton::Back,    /*isLeftLegacy=*/false);
  fireClickButton(MouseButton::Forward, /*isLeftLegacy=*/false);
  ```

If you want `onButtonPress` / `onButtonRelease` / `onButtonClick` to
fire for the new button, that's the only code change needed on top of
the enum value. If you want a legacy dedicated callback (like
`onRightClick`), add a `std::function<void()>` field on `Layout` and
a small block in `fireCallbacks`.

The old per-field approach (separate `down`/`pressed`/`released` for
each button on `PointerState`, separate `middlePressTarget` on
`UIContext`, separate callback field on `Layout`) is gone. Everything
is driven by the enum now.

### 9.3 Adding a new modifier

1. Add a `bool` to `UIContext` (e.g. `ctrlHeld`).
2. Populate it in `beginFrame` from a new `IPlatform::ctrlHeld()`.
3. Implement `ctrlHeld` in `RaylibPlatform`.
4. Widgets read `UIContext::get().ctrlHeld`.

### 9.4 Custom focus behavior

To make a node focusable but not part of Tab navigation:

- Set `setFocusable(true)` (so click can focus it).
- Override `onDescendantFocused` or add a check in `collectFocusables`
  — but `collectFocusables` is a static function, not a virtual.

The cleanest way is to add a `bool excludeFromTab{false}` flag to
`Layout`, set it on the node, and check it in `collectFocusables`:

```cpp
if (node->isFocusable() && !node->excludeFromTab)
    out.push_back(node);
```

This is not currently implemented but is a small extension.

---

## 10. Pitfalls

**The hover state is geometric, not hit-test based.**
`state.hovered` is set if the pointer is inside the node's `rect`, even if
another node is drawn on top. So a button under a semi-transparent
overlay is "hovered" when the pointer is over it, and `:hover` applies.
This can be surprising if you expect `:hover` to match the topmost
node only.

**`onHoverExit` fires when pressing.**
As noted in §2.4, `Hover → Pressed` triggers `onHoverExit` because of
the state comparison. This is likely a bug. In practice, the callback
fires briefly and then `onHoverEnter` doesn't re-fire until the
pointer leaves and re-enters. If you rely on `onHoverExit` for
tooltip dismissal, you might see unexpected dismissals during clicks.

**Focus and Tab don't work if no node has focus.**
If `focusedNode` is null, the first `Tab` press focuses the first
focusable in the root scope. If `focusables.empty()` is true, the
`break` stops the loop and focus is unchanged.

**`setEnabled(false)` clears focus but not `onHoverExit`.**
A focused node that's disabled loses focus immediately (in
`setEnabled`). But the `onHoverExit` callback doesn't fire — the state
transition to `Disabled` happens on the next `update`, and
`computeNextState` returns `Disabled` (not `Hover`), so the
`onHoverExit` fires based on the state comparison. Actually,
`Disabled` is `nextState`, `Hover` is `prevState`, and the condition
`prev == Hover && next != Hover` is true, so `onHoverExit` does fire.
Wait, but only if the node was hovered. If the node was focused but
not hovered, no hover callback fires.

OK so this pitfall isn't accurate. Let me remove it.

**`state.pressed` walks up from `pressTarget` via `config.passThrough`.**
If a child is `config.passThrough = 1` and the parent isn't, the parent is
considered "pressed" when the child is pressed. This means `:pressed`
styling applies to the parent. If you don't want that, don't use
`config.passThrough` on interactive children.

**Pointer capture is not transitive.**
Capturing a node's parent doesn't affect the node. Capture is a single
`weak_ptr` — the captured node is the routing target.

**`collectFocusables` doesn't skip disabled subtrees.**
It collects nodes with `isFocusable()`, which is
`config.focusable != 0 && config.enabled != 0`. But the disabled flag is checked at the
node level, not at the subtree level. So a disabled container doesn't
prevent its enabled children from being collected — they'd be focusable
if the container itself isn't blocking.

Actually, wait. Let me re-read `collectFocusables`:

```cpp
static void collectFocusables(Layout* node, std::vector<Layout*>& out) {
    if (node->isFocusable()) out.push_back(node);
    for (auto& c : node->children)
        collectFocusables(c.get(), out);
}
```

It recurses into children regardless of the parent's state. So a
disabled container's enabled children **are** collected. This is
probably not what you want — a disabled subtree should not be
focusable.

There's no check for `config.enabled` on the parent. So this is a bug or at
least a limitation. If you disable a modal, its buttons are still
Tab-reachable.

**`isFocusable()` on the scope itself.**
The scope is typically a `Modal` or a wrapper, which doesn't call
`setFocusable(true)`. So it's not collected. But if you make the scope
focusable, it would be the first Tab stop within itself.

**Pointer capture across frames uses a weak_ptr.**
If the captured node is destroyed during a drag, capture is
automatically released. The drag continues to receive events if the
node is still alive (via the tree), or the events go to whatever
`hitTest` returns (if the node was removed).

**`requestFocus` during `update` (from a callback).**
If a callback calls `requestFocus`, the new focus takes effect on the
next frame's `updateFlags`. Within the same frame, the current
`state.focused` flags are already set, so the style doesn't change until
the next frame. Usually fine, but if you need immediate visual
feedback, you'd need to force a re-resolve.

**The `config.passThrough` check for `state.pressed` walks up, not down.**
The loop starts at `pressTarget` and walks up. If `pressTarget` is a
deeply nested node, its ancestors are checked. But the node's own
descendants aren't. So if you press on a parent (with a `config.passThrough`
child), the child is not "pressed" — only the parent is. This is
correct: pressing a container shouldn't press all its children.

**`onRightClick` uses `state.hovered`, not `topmostConsumer`.**
A right-click on an overlapping area fires the callback on the deepest
node whose rect contains the pointer and has `onRightClick`. This can
differ from `topmostConsumer` if a higher-z node doesn't have
`onRightClick`. In that case, the lower node fires.

**Context pointers are cleared in `~Layout`.**
`UIContext::pressTarget`, `releaseTarget`, `topmostConsumer`, and
`hoverTarget` are raw `Layout*`. When a node is destroyed mid-frame
(the common case is `onClick` calling `removeFromParent()` on its own
item, and the item being culled at the end of `update`), `~Layout`
resets any of those four pointers that equal `this`. This is what
makes the ancestor's `isAncestorOf(pressTarget)` check safe after the
descendant is gone.

Before this fix, the pattern "delete button removes its own row"
crashed with a use-after-free on the frame of the release, because
`pressTarget` still pointed at the removed button while the row's
ancestors were still running their `fireCallbacks`.

**Tab traversal is not stable across removals.**
If the focused node is removed from the tree (via
`removeFromParent`), `focusedNode` becomes invalid (weak_ptr expires).
The next Tab finds no current focus, so it focuses the first focusable
in the scope. This is reasonable but might surprise users who expected
focus to go to the "next" node.

**`handleKeyInput` fires `onClick` for `Enter` and `Space` together.**
Both keys fire the same callback. If you need to distinguish, override
`onUpdate` and read `ev.keys` directly.

**Focus is only set from clicks on `topmostConsumer`.**
A click that's captured by a `config.blocksRaycast` non-interactive node
doesn't focus anything. The focus is released (since `topmostConsumer`
isn't focusable). This can be surprising — clicking a `Panel` (which
blocks raycast) releases focus from whatever was focused.

**`setKeyboardActivates(true)` requires `setFocusable(true)`.**
Without focus, `handleKeyInput` returns early. A node that sets
`config.keyboardActivates` but not `config.focusable` will never fire `onClick` from
the keyboard.

**Focus state doesn't affect `config.interactive`.**
A focused but non-interactive node can still be focused (via
`requestFocus`). The `config.interactive` flag only affects hit-testing and
`onClick`/`onPress` firing. A non-interactive focused node can still
have `:focus` styling and can still be reached by Tab (if
`config.focusable` is true).

**Hover during pointer capture.**
When a node has pointer capture, `topmostConsumer` is the captured
node, so `hoverTarget` is also the captured node. Every frame, the
captured node's `state.hovered` is set based on... wait, no.
`updateFlags` uses `node.rect.contains(pointer.pos)` regardless of
capture. So the captured node's `state.hovered` is geometric — it may be
false if the pointer has moved off the node.

But `topmostConsumer` is the captured node, so events are routed there.
The `state.hovered` flag doesn't affect routing, only styling and
`onHoverEnter`/`Exit` callbacks. So during a drag that leaves the
node, the node fires `onHoverExit` (because `state.hovered` becomes false)
but still receives `onPress`/`onRelease`/`onClick` (because of capture).

This is the standard behavior — a slider's knob stops being "hovered"
when the cursor drags away, but the drag continues.

**`onRightClick` fires on press, not on click.**
Unlike `onClick` / `onButtonClick`, which wait for a matching
release, `onRightClick` fires on the press edge. This is intentional —
right-click is a one-shot event (open a menu). If you need the full
press+release semantic for Right, add a separate callback and wire it
through `fireClickButton(MouseButton::Right, false)`.

**`onButton*` don't cover Right.**
`onButtonPress` / `onButtonRelease` / `onButtonClick` fire for Left and
Middle only. Right is deliberately excluded because its semantic (fire
on press, no click validation) is different. If you want a unified
callback that covers all three, read `pointer[MouseButton::Right].pressed`
directly in `onUpdate`, or add a wrapper.

**`buttonConsumed` is per-button, not global.**
Two nodes can both fire `onButtonClick(MouseButton::Left)` in the same
frame if they're in different subtrees — but only if neither consumed
the event. In practice this doesn't happen because `pressTarget` is a
single node and only ancestors of that node can match `pressedHere`.
The per-button flag matters when you mix buttons: consuming Right has
no effect on the Left click.

**Middle press and drag still routes to `pressTarget`.**
`pressTarget` is updated on the press edge of ANY button. So pressing
Middle on a button sets `pressTarget` to it. If you then press Left
on a different node in the same frame... you can't, because press
edges are one-frame and the platform reports one press per frame per
button. But across frames: Middle press on A (sets `pressTarget = A`,
stays until release), then Left press on B (overwrites `pressTarget =
B`). The Middle click on A is lost. This is a known limitation of
having a single `pressTarget` — multi-button simultaneous interactions
aren't supported. In practice, mouse users don't press two buttons at
once.
