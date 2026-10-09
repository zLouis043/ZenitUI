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
bool shiftHeld{false};
float dt{0.0f};
double time{0.0};
float dpiScale{1.0f};
EdgeInsets safeArea;
```

`PointerState`:

```cpp
struct PointerState {
    Vec2 pos;                   // logical coordinates
    bool down{false};           // button currently held
    bool pressed{false};        // rising edge this frame
    bool released{false};       // falling edge this frame
    bool rightDown{false};
    bool rightPressed{false};
    bool rightReleased{false};
    float wheelY{0.0f};         // wheel delta this frame
};
```

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
bool clickConsumed{false};
bool rightClickConsumed{false};
```

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
`wantsRemoval` set and will be culled. A dangling raw pointer to a
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
returns `isFocusable_ && isEnabled` — so a disabled node can't be
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
        if (focusNow != node.isFocused) {
            node.isFocused = focusNow;
            node.pendingTransition = true;
        }
        return;
    }

    node.isHovered = false;
    if (!selfBlocked && node.rect.width > 0 && node.rect.height > 0)
        node.isHovered = node.rect.contains(pointer.pos);

    node.isPressed = false;
    if (pointer.down && ctx.pressTarget) {
        Layout* n = ctx.pressTarget;
        while (n) {
            if (n == &node) { node.isPressed = true; break; }
            if (!n->getPassThrough()) break;
            n = n->getParent().get();
        }
    }

    bool focusNow = ctx.hasFocus(&node);
    if (focusNow != node.isFocused) {
        node.isFocused = focusNow;
        node.pendingTransition = true;
    }
}
```

Three flags are updated:

**`isHovered`** — true if the pointer is inside the node's `rect`, the
node isn't self-blocked, and it has non-zero size. Note this is a
**geometric** check against the node's own rect, not the hit-test
result. So a node that's covered by a sibling still gets `isHovered =
true` if the pointer is over its rect. This is a known simplification:
`.zstyle` `:hover` matches based on the node's own rect, not on
whether it's the topmost node.

The rationale: it's cheap (no recursion) and matches most users'
expectations (hovering over a parent still hovers the child if the
pointer is within both rects). The alternative — using
`topmostConsumer` — would only hover the single topmost node, which
makes `:hover` on containers impossible.

**`isPressed`** — true if the pointer is down **and** the current press
target is this node or an ancestor reachable through `passThrough`
chains. The loop walks up from `pressTarget` until it finds `node` or
hits a non-`passThrough` boundary. This is what makes
`passThrough: true` work for press/hold states.

Note it walks **up** from `pressTarget`, not down from `node`. This is
intentional: the press target is fixed at pointer-down, so a node
further up the tree can still be considered "pressed" if the press
target is one of its descendants that passThrough.

**`isFocused`** — true if `UIContext::focusedNode` points to this
node. When this changes, `pendingTransition = true` triggers a style
re-resolve so `:focus` rules apply.

The `scrolling` parameter is the "hover freeze": during a scroll, only
`isFocused` is updated, not `isHovered` or `isPressed`. This prevents
`onHoverEnter`/`onHoverExit` from firing on every row that passes
under a stationary cursor.

### 2.2 `computeNextState(node)`

```cpp
UIState InputController::computeNextState(const Layout& node) const {
    if (!node.isEnabled) return UIState::Disabled;
    if (node.isPressed)  return UIState::Pressed;
    if (node.isHovered)  return UIState::Hover;
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
    if (!node.isFocused || !node.isEnabled || !node.keyboardActivates_)
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
`keyboardActivates_` set. Uses `ev.keys` (rising edge), so it fires
once per key press.

Only `Enter` and `Space` are handled. Other keys are the widget's
responsibility (e.g. `TextInput` handles `Backspace`, `Delete`, arrow
keys in its own `onUpdate`).

Note the `onClick` callback is invoked directly, **without** the
consumption mechanism used by pointer clicks. So a `Button` with
`keyboardActivates` that's focused fires `onClick` on `Enter` and the
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
    if (stateChanged) {
        if (nextState == UIState::Hover && prevState != UIState::Hover
            && node.onHoverEnter)
            node.onHoverEnter();
        else if (prevState == UIState::Hover && nextState != UIState::Hover
                 && node.onHoverExit)
            node.onHoverExit();
    }

    // ---- Press ----
    if (pointer.pressed && !ctx.clickConsumed) {
        bool inPressPath = (ctx.pressTarget == &node) ||
                            node.isAncestorOf(ctx.pressTarget);
        if (inPressPath) {
            if (node.onPress) node.onPress();
            if (!node.passThrough_) ctx.consumeClick();
        }
    }

    // ---- Release + Click ----
    if (pointer.released) {
        bool pressedHere = (ctx.pressTarget == &node) ||
                            node.isAncestorOf(ctx.pressTarget);
        bool releasedHere = (ctx.releaseTarget == &node) ||
                             node.isAncestorOf(ctx.releaseTarget);
        bool validClick = pressedHere && releasedHere;

        if (validClick) {
            if (node.onRelease) node.onRelease();
            if (node.onClick && !ctx.clickConsumed) {
                node.onClick();
                if (!node.passThrough_) ctx.consumeClick();
            }
        }
    }

    // ---- Right-click ----
    if (pointer.rightPressed && node.isHovered && node.onRightClick) {
        if (!ctx.rightClickConsumed) {
            node.onRightClick();
            if (!node.passThrough_) ctx.consumeRightClick();
        }
    }
}
```

Let's break down each phase.

**Hover transitions** fire only on `stateChanged`, and only when the
transition crosses the `Hover` state. So `Idle → Pressed` (directly,
without hover) doesn't fire `onHoverEnter` — but that can't happen
because `Pressed` implies `Hover` in the flag computation (pressing a
node means the pointer is over it). In practice, `onHoverEnter` fires
when the node becomes hovered from a non-hovered state.

`Hover → Pressed` doesn't fire `onHoverEnter` again (it was already
hovered). `Pressed → Hover` doesn't fire `onHoverExit` either (it's
still hovered). Only `Hover ↔ {Idle, Disabled}` transitions fire the
enter/exit callbacks.

Wait, let me re-check the logic:

```cpp
if (nextState == UIState::Hover && prevState != UIState::Hover && onHoverEnter)
    onHoverEnter();
else if (prevState == UIState::Hover && nextState != UIState::Hover && onHoverExit)
    onHoverExit();
```

So:

- `Idle → Hover`: `onHoverEnter`.
- `Hover → Idle`: `onHoverExit`.
- `Hover → Pressed`: no callback (prev is Hover, next is Pressed; the
  first condition is `next != Hover` so false; the second is
  `prev == Hover && next != Hover` — true! so `onHoverExit` fires).

Wait, that's a problem. `Hover → Pressed` has `prev == Hover` and
`next != Hover`, so `onHoverExit` would fire. Is that the intent?

Hmm, looking at the source more carefully:

```cpp
if (nextState == UIState::Hover && prevState != UIState::Hover && node.onHoverEnter)
    node.onHoverEnter();
else if (prevState == UIState::Hover && nextState != UIState::Hover && node.onHoverExit)
    node.onHoverExit();
```

Yes, this would fire `onHoverExit` when transitioning from Hover to
Pressed. That's likely a bug or at least unexpected behavior. The user
documentation says `onHoverExit` fires when `isHovered` transitions
from true to false, but the state machine transitions through `Pressed`
without `isHovered` changing.

Actually, re-reading: the state machine runs `beginStateTransition`
before `fireCallbacks`. So `isHovered` is still true when the state
changes to `Pressed`. But the callback fires based on the state
comparison.

Let me trace: pointer enters a node.

- Frame 1: `isHovered = true`, `isPressed = false`. `nextState = Hover`.
  `prevState = Idle`. State changed. `onHoverEnter` fires. ✅

- Frame 2: pointer presses.
- `isHovered = true`, `isPressed = true`. `nextState = Pressed`.
  `prevState = Hover`. State changed.
  - First condition: `next == Hover`? No (it's `Pressed`).
  - Second condition: `prev == Hover && next != Hover`? Yes.
  - `onHoverExit` fires. ⚠️

So yes, pressing a node fires `onHoverExit`. This is likely a bug in
the current implementation. It should probably be:

```cpp
if ((nextState == UIState::Hover || nextState == UIState::Pressed)
    && prevState != UIState::Hover && prevState != UIState::Pressed
    && onHoverEnter)
    onHoverEnter();
```

Or the hover state should be tracked separately from the UI state.

I'll note this as a known issue rather than describe the behavior as
intended.

**Press** fires on `pointer.pressed` (rising edge) if the press path
includes this node. The path check:

```cpp
bool inPressPath = (ctx.pressTarget == &node) || node.isAncestorOf(ctx.pressTarget);
```

`pressTarget` is set in `updateTree` to `topmostConsumer` at the frame
of the press. A node fires `onPress` if it **is** the press target, or
if the press target is a descendant of it. The `isAncestorOf` check
walks up the parent chain.

The consumption: if the node doesn't have `passThrough`, it sets
`clickConsumed = true`. Subsequent nodes in the tree walk (which
proceeds bottom-up within a subtree, since children are updated before
their parents) see `clickConsumed` and skip their own `onPress`.

Order matters: **children update before parents**, so a child's
`onPress` fires before its parent's. If the child consumes the event,
the parent doesn't fire. If the child is `passThrough`, the parent
also fires.

**Release + Click**: similar logic, but requires both press and release
to include the node. The click is validated only if:

- `pressedHere`: the press target is this node or a descendant.
- `releasedHere`: the release target is this node or a descendant.
- Both true.

Then `onRelease` fires unconditionally (if the click is valid), and
`onClick` fires if not consumed.

A subtle point: `onRelease` fires on any valid click, but `onClick` is
gated on `clickConsumed`. If a descendant already fired `onClick` and
consumed, the ancestor still gets `onRelease` but not `onClick`. This
matches the "bubbling" model where release is more fundamental than
click.

Actually, looking again:

```cpp
if (validClick) {
    if (node.onRelease) node.onRelease();
    if (node.onClick && !ctx.clickConsumed) {
        node.onClick();
        if (!node.passThrough_) ctx.consumeClick();
    }
}
```

`onRelease` fires without checking `clickConsumed`. So an ancestor can
receive `onRelease` even after a descendant consumed the click. This is
intentional: `onRelease` is "the pointer released over this subtree",
`onClick` is "the click was handled here".

**Right-click** fires on `pointer.rightPressed` (rising edge) if the
node is currently hovered and has `onRightClick`. Unlike left-click,
there's no press/release validation — right-click fires immediately on
press. And it uses `isHovered` (geometric check), not the hit-test
result.

The consumption: `rightClickConsumed` is set if the node doesn't
passThrough. The walk order is the same (children before parents), so
the deepest hovered node fires first.

### 2.5 Order of the four phases

`fireCallbacks` runs the four phases in this order: hover, press,
release+click, right-click. On a frame where multiple events happen
(rare, but possible with simultaneous presses), the order is fixed.
In practice, only one event fires per frame because the pointer state
transitions are one per frame (`pressed` and `released` can't both be
true).

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
if (ctx.pointer.pressed)  ctx.pressTarget   = ctx.hoverTarget;
if (ctx.pointer.released) ctx.releaseTarget = ctx.hoverTarget;
```

- `hoverTarget` is set every frame.
- `pressTarget` is only updated on the frame of a press.
- `releaseTarget` is only updated on the frame of a release.

This means `pressTarget` persists across frames between press and
release. It's cleared at the end of `updateTree` (see §3.6).

### 3.3 Focus from click

```cpp
if (ctx.pointer.pressed) {
    if (ctx.topmostConsumer && ctx.topmostConsumer->isFocusable()) {
        ctx.requestFocus(ctx.topmostConsumer->shared_from_this());
    } else {
        ctx.releaseFocus();
    }
}
```

Clicking a focusable node focuses it. Clicking anywhere else releases
focus. This is the only place a click changes focus.

`isFocusable()` returns `isFocusable_ && isEnabled`, so a disabled
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
        next = ctx.shiftHeld ? (idx - 1 + n) % n : (idx + 1) % n;
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
if (ctx.pointer.released)
    ctx.pressTarget = nullptr;
if (!ctx.pointer.down && !ctx.pointerCapture.expired())
    ctx.pointerCapture.reset();
```

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

The node's `isFocused` flag isn't updated here — it's updated in
`updateFlags` on the next `update`. So there's a one-frame delay
between `requestFocus` and `:focus` styling applying.

Actually, `requestFocus` is called during `updateTree`, before `update`.
So within the same frame:
1. `requestFocus` sets `focusedNode` to the new node.
2. `update` runs, and `updateFlags` sets `isFocused` on the new node
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
    if (isEnabled == e) return;
    isEnabled = e;
    pendingTransition = true;

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

For a `passThrough` chain:

1. Deepest node fires `onClick`. Doesn't consume (`passThrough`).
2. Parent fires `onClick`. Doesn't consume (if it's passThrough).
3. ... all the way up.

### 5.3 Consumption

```cpp
bool clickConsumed{false};
void consumeClick() { clickConsumed = true; }
```

`clickConsumed` is cleared at the start of each frame. It's set by
`fireCallbacks` when a node fires `onClick` **and** isn't `passThrough`.

The check is done **before** calling `onClick`:

```cpp
if (node.onClick && !ctx.clickConsumed) {
    node.onClick();
    if (!node.passThrough_) ctx.consumeClick();
}
```

So a node fires `onClick` only if no previous node (in the bottom-up
order) has consumed. And after firing, if the node isn't passThrough,
it consumes.

The same logic for `onPress` (using `clickConsumed` — same flag) and
`onRightClick` (using `rightClickConsumed`).

There's a subtle interaction: `onPress` fires on `pointer.pressed` and
can consume. Then on `pointer.released` (usually a different frame),
`clickConsumed` is false again (cleared at `beginFrame`), and the
`onClick` fires independently.

So the "click consumed" state is per-frame, not persistent. A press
in frame N and a release in frame N+1 both use the fresh
`clickConsumed` flag of their respective frames.

### 5.4 Right-click

```cpp
if (pointer.rightPressed && node.isHovered && node.onRightClick) {
    if (!ctx.rightClickConsumed) {
        node.onRightClick();
        if (!node.passThrough_) ctx.consumeRightClick();
    }
}
```

Right-click fires on the **hovered** node, not on the press target.
Since right-click uses `rightPressed` (rising edge), there's no
press/release validation. The hovered node gets the callback
immediately.

The walk order is the same (bottom-up), and consumption applies. So
the deepest hovered node with `onRightClick` fires first, and
ancestors are skipped unless the node is `passThrough`.

Since `isHovered` is geometric (pointer inside the node's rect), and
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
if (!ctx.pointer.down && !ctx.pointerCapture.expired())
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
    if (isPressed && !hasPointerCapture())
        capturePointer();

    // While captured, do the drag
    if (hasPointerCapture()) {
        Vec2 p = UIContext::get().pointer.pos;
        // process p
    }
    // No explicit release: capture is dropped on pointer-up
}
```

The `isPressed && !hasPointerCapture()` check starts a new capture only
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

Only `Shift` is tracked:

```cpp
bool shiftHeld{false};
```

Updated from `platform->shiftHeld()` each `beginFrame`. Used by:

- Tab navigation (reverse direction).
- Scroll (horizontal wheel).
- `Slider` (fine step).

Other modifiers (Ctrl, Alt, Meta) are not tracked. If you need them,
extend `IPlatform` and `InputEvents`.

---

## 8. Interaction with the frame pipeline

### 8.1 When flags are updated

`updateFlags` is called for each node during `update`. The order:

1. `updateTree` sets `topmostConsumer`, `pressTarget`, `releaseTarget`.
2. `update` recurses into children.
3. Each node's `update` calls `updateFlags`, which reads the current
   state and sets `isHovered`, `isPressed`, `isFocused`.
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

The `isFocused` flag is set in `updateFlags` (step 3), and
`pendingTransition = true` is set if the flag changed. So the style
re-resolve happens in the same frame.

### 8.4 When capture changes

Capture changes happen in user code (widget's `onUpdate`) or
automatically at the end of `updateTree`. If a widget calls
`capturePointer()` during its `onUpdate`, the capture takes effect on
the **next** frame's `updateTree` (since `updateTree` already ran and
set `topmostConsumer` for this frame).

In practice, the drag pattern works because:

- Frame N: press, `isPressed` becomes true, widget captures.
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

1. Add fields to `PointerState` (`middleDown`, `middlePressed`,
   `middleReleased`).
2. Populate them in the platform's `pointer()` method.
3. Add handling in `updateTree` (track `middlePressTarget`,
   `middleReleaseTarget`) if you want click semantics.
4. Add a callback field to `Layout` (`onMiddleClick`).
5. Fire it in `fireCallbacks`.

The existing `rightDown` / `rightPressed` / `rightReleased` and
`onRightClick` are the template.

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
`isHovered` is set if the pointer is inside the node's `rect`, even if
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

**`isPressed` walks up from `pressTarget` via `passThrough`.**
If a child is `passThrough` and the parent isn't, the parent is
considered "pressed" when the child is pressed. This means `:pressed`
styling applies to the parent. If you don't want that, don't use
`passThrough` on interactive children.

**Pointer capture is not transitive.**
Capturing a node's parent doesn't affect the node. Capture is a single
`weak_ptr` — the captured node is the routing target.

**`collectFocusables` doesn't skip disabled subtrees.**
It collects nodes with `isFocusable()`, which is
`isFocusable_ && isEnabled`. But the disabled flag is checked at the
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

There's no check for `isEnabled` on the parent. So this is a bug or at
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
`isFocused` flags are already set, so the style doesn't change until
the next frame. Usually fine, but if you need immediate visual
feedback, you'd need to force a re-resolve.

**The `passThrough` check for `isPressed` walks up, not down.**
The loop starts at `pressTarget` and walks up. If `pressTarget` is a
deeply nested node, its ancestors are checked. But the node's own
descendants aren't. So if you press on a parent (with a `passThrough`
child), the child is not "pressed" — only the parent is. This is
correct: pressing a container shouldn't press all its children.

**`onRightClick` uses `isHovered`, not `topmostConsumer`.**
A right-click on an overlapping area fires the callback on the deepest
node whose rect contains the pointer and has `onRightClick`. This can
differ from `topmostConsumer` if a higher-z node doesn't have
`onRightClick`. In that case, the lower node fires.

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
A click that's captured by a `blocksRaycast` non-interactive node
doesn't focus anything. The focus is released (since `topmostConsumer`
isn't focusable). This can be surprising — clicking a `Panel` (which
blocks raycast) releases focus from whatever was focused.

**`setKeyboardActivates(true)` requires `setFocusable(true)`.**
Without focus, `handleKeyInput` returns early. A node that sets
`keyboardActivates` but not `focusable` will never fire `onClick` from
the keyboard.

**Focus state doesn't affect `isInteractive`.**
A focused but non-interactive node can still be focused (via
`requestFocus`). The `isInteractive` flag only affects hit-testing and
`onClick`/`onPress` firing. A non-interactive focused node can still
have `:focus` styling and can still be reached by Tab (if
`isFocusable` is true).

**Hover during pointer capture.**
When a node has pointer capture, `topmostConsumer` is the captured
node, so `hoverTarget` is also the captured node. Every frame, the
captured node's `isHovered` is set based on... wait, no.
`updateFlags` uses `node.rect.contains(pointer.pos)` regardless of
capture. So the captured node's `isHovered` is geometric — it may be
false if the pointer has moved off the node.

But `topmostConsumer` is the captured node, so events are routed there.
The `isHovered` flag doesn't affect routing, only styling and
`onHoverEnter`/`Exit` callbacks. So during a drag that leaves the
node, the node fires `onHoverExit` (because `isHovered` becomes false)
but still receives `onPress`/`onRelease`/`onClick` (because of capture).

This is the standard behavior — a slider's knob stops being "hovered"
when the cursor drags away, but the drag continues.