# Scroll System

The scroll system lets a `Layout` node hold content larger than its own
rect, and lets the user pan that content via wheel, drag, or programmatic
API. It lives in `ScrollController`, `ScrollState`, and a handful of
methods on `Layout` (in `Measure.cpp` and `Layout.cpp`).

This document covers:

- `ScrollState` — the pure state machine.
- `ScrollController` — the orchestration layer.
- How scroll interacts with `measure` / `arrange` / `draw`.
- Wheel, drag, and inertia input.
- Scrollbar geometry and rendering.
- The "hover freeze" behavior.

For the user-facing API, see [Layout §8](../user/02-layout.md#8-overflow)
and [Widgets §12 (ScrollView)](../user/03-widgets.md#12-scrollview).

---

## 1. When a node is a scroll container

```cpp
bool Layout::isScrollContainer() const {
    return style_.currentStyle.overflowX != Overflow::Visible ||
           style_.currentStyle.overflowY != Overflow::Visible;
}
```

A node is a scroll container if **either** axis has an `overflow` value
other than `visible`. `hidden` / `scroll` / `auto` all qualify — even
`hidden`, which doesn't show a scrollbar but still clips and can
technically hold scroll state.

The computed-value rule (see [Style System §1.2](03-style-system.md))
means that setting `overflow-x: hidden` alone promotes `overflow-y` to
`auto`, making the node a scroll container on both axes. This is the
CSS behavior and is intentional.

For the purposes of the arrange algorithm, `Stack` type nodes are
excluded:

```cpp
if (isScrollContainer() && type != LayoutType::Stack) { ... }
else { arrangeInto(space); ... }
```

A `Stack` has no main axis, so scrolling along "the flow" doesn't make
sense. A `Stack` with `overflow: scroll` behaves like a non-scroll
container with clipping — no scroll offset, no thumb. This is a
deliberate simplification; if you need scrolling in a `Stack`, use
`Vertical` or `Horizontal`.

---

## 2. `ScrollState` — the pure state

`ScrollState` is a POD struct with no knowledge of `Layout`:

```cpp
struct ScrollState {
    Vec2 offset{0.0f, 0.0f};        // current scroll position
    Vec2 maxScroll{0.0f, 0.0f};     // maximum offset per axis
    Vec2 velocity{0.0f, 0.0f};      // inertia velocity (px/s)

    float dragStartMouseY{0.0f};
    float dragStartOffsetY{0.0f};
    float dragStartMouseX{0.0f};
    float dragStartOffsetX{0.0f};

    static constexpr float WHEEL_IMPULSE = 400.0f;
    static constexpr float DECAY = 0.90f;
    static constexpr float VELOCITY_MIN = 5.0f;

    void clamp();
    void reset();
    void tickInertia(float dt);
};
```

### 2.1 `offset` and `maxScroll`

- `offset` is the current scroll position, always in `[0, maxScroll]`.
- `maxScroll` is `contentSize - containerSize`, per axis. Zero means
  no scrolling needed.

Both are `Vec2` so `x` and `y` scroll independently.

### 2.2 `velocity`

The inertia velocity. When a wheel event arrives, it adds an impulse to
`velocity` rather than directly modifying `offset`. Each frame,
`tickInertia` integrates velocity into offset and decays it.

This produces the "smooth scroll" behavior: a wheel tick adds a
velocity, the velocity is applied over several frames, and it decays
exponentially. The result is a decelerating scroll that settles at the
target.

### 2.3 Drag start snapshots

When a scrollbar thumb drag starts, the current mouse position and
offset are saved. On each frame of the drag, the delta is computed and
mapped to a new offset. This avoids drift from accumulated small
increments.

### 2.4 Constants

- `WHEEL_IMPULSE = 400.0f` — pixels of velocity added per wheel tick.
  A typical wheel tick is `±1.0` in Raylib's `GetMouseWheelMove`, so a
  single tick produces `±400 px/s` of velocity.
- `DECAY = 0.90f` — velocity multiplier per 60Hz frame. After 1 second
  (60 frames), velocity is `0.9^60 ≈ 0.0018` of its initial value.
- `VELOCITY_MIN = 5.0f` — below this threshold, velocity is zeroed.
  Prevents a slow drift that never settles.

### 2.5 `clamp`

```cpp
void clamp() {
    offset.x = std::clamp(offset.x, 0.0f, maxScroll.x);
    offset.y = std::clamp(offset.y, 0.0f, maxScroll.y);
}
```

Ensures `offset` is within `[0, maxScroll]`. Called after any
modification and at the end of `tickInertia`.

### 2.6 `reset`

```cpp
void reset() {
    offset = {0.0f, 0.0f};
    maxScroll = {0.0f, 0.0f};
    velocity = {0.0f, 0.0f};
}
```

Clears everything. Called when overflow changes (see §5.4).

### 2.7 `tickInertia`

```cpp
void tickInertia(float dt) {
    if (std::abs(velocity.x) > VELOCITY_MIN) {
        offset.x += velocity.x * dt;
        velocity.x *= std::pow(DECAY, dt * 60.0f);
    } else {
        velocity.x = 0.0f;
    }

    if (std::abs(velocity.y) > VELOCITY_MIN) {
        offset.y += velocity.y * dt;
        velocity.y *= std::pow(DECAY, dt * 60.0f);
    } else {
        velocity.y = 0.0f;
    }

    clamp();
}
```

Per-axis integration:

1. If `|velocity| > VELOCITY_MIN`, apply `velocity * dt` to offset and
   decay velocity.
2. Otherwise, zero the velocity (stop).
3. Clamp offset.

The decay is `DECAY^(dt * 60)` — the `dt * 60` factor makes the decay
rate independent of frame rate. At 60 fps, `dt = 1/60`, so the exponent
is `1.0` and velocity is multiplied by `0.90`. At 30 fps, `dt = 1/30`,
exponent is `2.0`, velocity is multiplied by `0.81`. Same decay per
second.

The offset integration doesn't use the same frame-rate normalization —
`offset += velocity * dt` is frame-rate correct because it's a direct
integration of velocity over time. The velocity itself is in px/s, so
`velocity * dt` is the pixels scrolled this frame.

### 2.8 A note on units

`offset`, `maxScroll`, and `velocity` are all in **logical pixels**.
`contentSize` and `rect.width/height` (which feed `maxScroll`) are also
logical pixels. The DPI scale is applied by the renderer, not by the
scroll system.

---

## 3. `ScrollController` — the orchestrator

`ScrollController` is a struct with:

```cpp
struct ScrollController {
    ScrollState state;
    Vec2        contentSize{0, 0};
    Vec2        appliedOffset{0, 0};
    bool        arrangeInitialized{false};
    bool        scrolling{false};
};
```

It's a member of every `Layout` node, but only used when
`isScrollContainer()` is true.

### 3.1 Fields

- **`state`** — the `ScrollState` from §2.
- **`contentSize`** — the uncut content extent, set by `measure`. This
  is what `maxScroll` is computed from.
- **`appliedOffset`** — the offset that has already been applied to the
  children via `translateSubtree`. This can differ from `state.offset`
  momentarily: `state.offset` is the target, `appliedOffset` is the
  current. The difference is applied each frame in `applyOffsetDelta`.
- **`arrangeInitialized`** — true after the first `arrange`. Used to
  detect the "first frame" case.
- **`scrolling`** — true during a drag or inertia. Used to freeze hover
  (see §8).

### 3.2 Lifecycle methods

The controller's methods are called from `Layout::arrange` and
`Layout::update`:

```
arrange:
├── resetIfOverflowChanged   (in update, actually)
├── needsFullArrange
├── computeArrangeSpace
├── arrangeInto
├── applyOffsetDelta
└── updateMaxScroll

update:
├── resetIfOverflowChanged
├── tickInput
│   ├── (wheel, drag, click-on-track)
│   ├── tickInertia
│   └── sets scrolling

draw:
└── drawScrollbar
    ├── verticalThumb
    └── horizontalThumb
```

Note that `resetIfOverflowChanged` is called from `update`, not
`arrange`. This means the overflow check runs before measure, which is
correct — the overflow determines whether the node measures with an
unbounded main axis.

### 3.3 `resetIfOverflowChanged`

```cpp
void ScrollController::resetIfOverflowChanged(const Layout& node) {
    const auto& cur = node.style_.currentStyle;
    const auto& prev = node.style_.lastInherited;
    if (cur.overflowX != prev.overflowX || cur.overflowY != prev.overflowY)
        reset();
}
```

Compares the current `overflow` to the last snapshot. If either axis
changed, resets the scroll state. This prevents "phantom scroll" when a
container stops being scrollable — the offset would otherwise stay at
a non-zero value and clamp to zero on the next `updateMaxScroll`, but
only after one frame of weird rendering.

**Wait, this uses `lastInherited` for the overflow comparison.** That's
the snapshot used by `propagateInheritance`, which has `overflowX` and
`overflowY` as fields. So the "last seen overflow" comes from that
snapshot. It's updated in `propagateInheritance` after the change is
processed. So:

1. Frame N: `overflow` changes from `visible` to `scroll`.
2. `update` runs. `resetIfOverflowChanged` sees the change (current
   differs from `lastInherited`), resets.
3. `propagateInheritance` updates `lastInherited.overflowX/Y`.
4. Frame N+1: `resetIfOverflowChanged` sees no change, no reset.

But there's a subtlety: `propagateInheritance` only updates
`lastInherited` if something changed (it has an early-out for "no
change"). Since overflow is part of the snapshot, a change to overflow
triggers the update. So the sequence works.

Actually, looking more carefully at `propagateInheritance`:

```cpp
bool changed =
    lastInherited.hovered != node.isHoveredState() ||
    // ... other flags ...
    !(lastInherited.overflowX == currentStyle.overflowX) ||
    !(lastInherited.overflowY == currentStyle.overflowY);

if (!changed) return;

// ... update snapshot ...
lastInherited.overflowX = currentStyle.overflowX;
lastInherited.overflowY = currentStyle.overflowY;
```

Wait, is overflow in the `changed` check? Let me look at the actual
source... From `StyleResolver.cpp`:

```cpp
bool changed =
    lastInherited.hovered != node.isHoveredState() ||
    lastInherited.pressed != node.isPressedState() ||
    lastInherited.focused != node.isFocusedState() ||
    lastInherited.enabled != node.getEnabled() ||
    lastInherited.checked != node.isCheckedState() ||
    lastInherited.font != currentStyle.font ||
    !(lastInherited.fontSize == currentStyle.fontSize) ||
    !(lastInherited.color == currentStyle.color) ||
    !(lastInherited.letterSpacing == currentStyle.letterSpacing) ||
    !(lastInherited.textAlign == currentStyle.textAlign);
```

Overflow is **not** in the `changed` check, but it **is** updated at the
end:

```cpp
lastInherited.overflowX = currentStyle.overflowX;
lastInherited.overflowY = currentStyle.overflowY;
```

This is a bug: `lastInherited.overflowX` is only updated when some
**other** property changed, because the early-return skips the update
if `changed` is false. So the overflow snapshot can be stale.

The impact: `resetIfOverflowChanged` might see a spurious change (if
`lastInherited.overflowX` hasn't been updated since some prior
overflow value), or might miss a change (if it was updated
"accidentally" by some other change). In practice, the reset is rare
enough that this doesn't cause visible problems. But it's a known
issue in the source.

For the rest of this document, I'll describe the intended behavior
(reset when overflow changes), noting that the implementation has this
edge case.

---

## 4. Scroll and measure

### 4.1 Unbounded main axis

In `measure`:

```cpp
if (type == LayoutType::Vertical && calcStyle.overflowY != Overflow::Visible)
    innerH = 1e9f;
if (type == LayoutType::Horizontal && calcStyle.overflowX != Overflow::Visible)
    innerW = 1e9f;
```

The main axis of a scroll container has effectively infinite space for
its children. This lets children compute their natural size without
being constrained by the container — which is the whole point of
scrolling.

The `1e9f` sentinel is large enough to never be hit in practice, but
finite to avoid `inf` propagation in the arithmetic.

### 4.2 `contentSize`

At the end of `measure`:

```cpp
scroll_.contentSize = contentSize;
```

`contentSize` is the accumulated size of the children (before the
container's own `measuredSize` is applied). For a `Vertical` container,
`contentSize.y` is the total height of the children plus gaps.

For a node that isn't a scroll container, `contentSize` is still stored
(the assignment is unconditional). It's just unused.

### 4.3 Measured size

The container's own `measuredSize` is computed normally from
`contentSize` (via the `auto` rules) and clamped by `min-*` / `max-*`.
So a scroll container with `height: auto` measures to fit its content
(no scrolling needed). To have an actual scroll, you need to give the
container an explicit height (or a `grow` context that fills the
available space).

Common patterns:

```css
/* ScrollView fills its parent */
ScrollView { height: 100%; overflow-y: auto; }

/* ScrollView grows to fill leftover space */
.settings-body { grow: 1; overflow-y: auto; }

/* ScrollView with a fixed height */
.terminal { height: 30vh; overflow-y: scroll; }
```

### 4.4 The "unbounded main axis" and content size

A subtle point: the `contentSize` measured with an unbounded main axis
might differ from what the same content would measure with a bounded
main axis. This is because `Text` with `wrap: true` measures to fit
its parent's width — if the parent is a scroll container, the text
measures against the container's width (bounded, cross axis), and the
height is the wrapped height (unbounded, main axis).

For a vertical scroll container, this is correct: text wraps at the
container's width, and the container's height accommodates the wrapped
text. But if the container is horizontal (unbounded width), text
won't wrap (it has all the width it needs) and will overflow
horizontally.

This is a known asymmetry — it matches CSS's "scroll container measures
its content with unbounded main axis" behavior. For most UIs, this is
fine because scroll containers are vertical.

---

## 5. Scroll and arrange

### 5.1 The arrange flow for a scroll container

```cpp
if (isScrollContainer() && type != LayoutType::Stack) {
    if (scroll_.needsFullArrange(*this, positionChanged)) {
        Rect arrangeSpace = scroll_.computeArrangeSpace(*this, space);
        arrangeInto(arrangeSpace);
        scroll_.arrangeInitialized = true;
        scroll_.appliedOffset = {0.0f, 0.0f};
    }
    scroll_.applyOffsetDelta(*this);
    scroll_.updateMaxScroll(*this, space);
} else {
    arrangeInto(space);
    scroll_.arrangeInitialized = true;
}
```

Three sub-steps:

1. **Full arrange** (if needed): expand the arrange space and lay out
   children.
2. **Apply offset delta**: shift the subtree by the scroll delta.
3. **Update max scroll**: recompute `maxScroll` from the new
   `contentSize` and the container's `rect`.

Non-scroll containers just call `arrangeInto(space)`.

### 5.2 `needsFullArrange`

```cpp
bool ScrollController::needsFullArrange(const Layout& node, bool positionChanged) const {
    return !arrangeInitialized || node.subtreeDirty_ || positionChanged;
}
```

A full arrange is needed if:

- This is the first arrange (`!arrangeInitialized`).
- The subtree is dirty (something changed).
- The container moved (position changed).

If none of these, the arrange is "incremental" — only the offset is
applied, no children are repositioned.

The rationale: since scroll works by `translateSubtree`, the children's
positions are relative to the "base" arrange (offset 0). When the
container moves but the subtree doesn't change, the base arrange is
already correct; only the offset needs to be reapplied.

Wait, that's not quite right. If the container moves, the children's
`rect` positions were computed for the old container position. The
`translateSubtree` accumulated the scroll offset, but the base positions
are in the old container's frame.

Actually, re-reading `arrange`:

```cpp
const bool positionChanged = (space.x != rect.x || space.y != rect.y);
rect = space;
```

The `rect` is updated first. Then `needsFullArrange` decides whether to
re-run `arrangeInto`. If `positionChanged` is true, it re-arranges, so
the children's positions are recomputed for the new container rect.

If `positionChanged` is false (container didn't move), and the subtree
isn't dirty, the children keep their positions from the last arrange,
and only the scroll delta is applied. This is the fast path: a scroll
container whose content didn't change, scrolling smoothly over many
frames.

### 5.3 `computeArrangeSpace`

```cpp
Rect ScrollController::computeArrangeSpace(const Layout& node, Rect space) const {
    const auto& st = node.style_.currentStyle;

    float pl = st.padding.left.resolveH(space.width, space.height);
    float pr = st.padding.right.resolveH(space.width, space.height);
    float pt = st.padding.top.resolveV(space.width, space.height);
    float pb = st.padding.bottom.resolveV(space.width, space.height);

    Rect arranged = space;

    if (node.type == LayoutType::Vertical && st.overflowY != Overflow::Visible) {
        float contentH = contentSize.y + pt + pb;
        arranged.height = std::max(space.height, contentH);
    } else if (node.type == LayoutType::Horizontal && st.overflowX != Overflow::Visible) {
        float contentW = contentSize.x + pl + pr;
        arranged.width = std::max(space.width, contentW);
    }

    return arranged;
}
```

The arrange space is the container's rect, expanded on the main axis if
the content is larger. If the content fits (contentSize < container
size), the space is unchanged.

Only the main axis is expanded — the cross axis stays at the container
size. So a vertical scroll container's children are arranged within the
container's width, regardless of their content height.

### 5.4 `applyOffsetDelta`

```cpp
bool ScrollController::applyOffsetDelta(Layout& node) {
    float dx = appliedOffset.x - state.offset.x;
    float dy = appliedOffset.y - state.offset.y;
    if (dx == 0.0f && dy == 0.0f) return false;

    node.translateSubtree(dx, dy);
    appliedOffset = state.offset;
    return true;
}
```

Computes the delta between the applied offset (from last frame) and the
current offset (this frame), and translates the subtree by that delta.

The delta is `appliedOffset - state.offset`:
- If we're scrolling **down** (state.offset increased), the delta is
  negative, so children move up (negative y).
- If we're scrolling **up**, the delta is positive, children move down.

This is the correct sign: scrolling down means content moves up.

`translateSubtree`:

```cpp
void Layout::translateSubtree(float dx, float dy) {
    for (auto& c : children) {
        c->rect.x += dx;
        c->rect.y += dy;
        c->translateSubtree(dx, dy);
    }
}
```

Recursively shifts every descendant's `rect`. The node's own `rect` is
not shifted — the container stays fixed, only its content moves.

**Note:** absolute children and portals are also translated. Absolute
children of a scroll container will scroll with the content, which is
usually what you want. Portals, however, are supposed to be
independent of the scroll (they position themselves in screen space).
Translating them will break their positioning.

Looking at the code, there's a commented-out check:

```cpp
//if (c->isPortal())
//    continue;
```

So the portal exclusion was considered but not enabled. In practice,
portals inside a scroll container are rare, and the issue is masked by
the portal's `onUpdate` setting its own `rect` each frame. The
translate shift is overwritten by the portal's position computation.
But the recursion still shifts the portal's children, which can be
visible.

This is a known issue.

### 5.5 `updateMaxScroll`

```cpp
void ScrollController::updateMaxScroll(const Layout& node, Rect space) {
    const auto& st = node.style_.currentStyle;

    float pl = st.padding.left.resolveH(space.width, space.height);
    float pr = st.padding.right.resolveH(space.width, space.height);
    float pt = st.padding.top.resolveV(space.width, space.height);
    float pb = st.padding.bottom.resolveV(space.width, space.height);

    float contentW = contentSize.x + pl + pr;
    float contentH = contentSize.y + pt + pb;
    state.maxScroll.x = std::max(0.0f, contentW - node.rect.width);
    state.maxScroll.y = std::max(0.0f, contentH - node.rect.height);
    state.clamp();
}
```

`maxScroll` is `contentSize (plus padding) - container size`. If
negative, clamped to zero.

Called after `applyOffsetDelta`, so if `maxScroll` shrinks (content
shrank), the offset is re-clamped. This can cause a visible "snap back"
if content is removed while scrolled past the new max. That's the
expected behavior.

**Padding is added to content size** — this means a container with
`padding: 8px` and content of height 100 has `contentH = 116`, so
`maxScroll.y = 116 - containerHeight`. The padding is part of the
scrollable content, so you can scroll to see the padding too. This
matches CSS's `box-sizing: content-box` default (which ZenitUI uses —
padding is added outside the content size).

---

## 6. Scroll input

`ScrollController::tickInput` handles all user input for scrolling:
wheel, scrollbar drag, and track click.

### 6.1 The entry condition

```cpp
void ScrollController::tickInput(Layout& node) {
    const auto& st = node.style_.currentStyle;
    if (!acceptsAnyInput(st) || node.type == LayoutType::Stack)
        return;
    // ...
}
```

Returns early if:

- The container doesn't accept input (`acceptsInput` returns true only
  for `Scroll` and `Auto`, not `Hidden`).
- The type is `Stack`.

So a `Hidden` overflow clips but doesn't accept scroll input. A `Stack`
with `Scroll` doesn't either (the arrange path already excluded it).

`acceptsInput`:

```cpp
static bool acceptsInput(Overflow o) {
    return o == Overflow::Scroll || o == Overflow::Auto;
}
```

### 6.2 The main scroll body

```cpp
const bool canScrollY = state.maxScroll.y > 0.0f &&
                        st.overflowY != Overflow::Hidden;
const bool canScrollX = state.maxScroll.x > 0.0f &&
                        st.overflowX != Overflow::Hidden;
```

Each axis can scroll if `maxScroll > 0` on that axis, and the overflow
isn't `Hidden`. So `Hidden` on one axis still allows scrolling on the
other (if it's `Scroll` or `Auto`).

The scrollbar drag, wheel, and track click all check these flags.

### 6.3 Drag start and track click

```cpp
if (!node.hasPointerCapture()) {
    if (pointer.pressed && ctx.topmostConsumer == &node) {
        bool handled = false;

        // Try vertical thumb
        if (canScrollY) {
            Rect thumb = verticalThumb(node);
            if (thumb.contains(pointer.pos)) {
                node.capturePointer();
                state.dragStartMouseY = pointer.pos.y;
                state.dragStartOffsetY = state.offset.y;
                state.velocity = {0.0f, 0.0f};
                handled = true;
            }
        }

        // Try horizontal thumb
        if (!handled && canScrollX) {
            Rect thumb = horizontalThumb(node);
            if (thumb.contains(pointer.pos)) {
                node.capturePointer();
                state.dragStartMouseX = pointer.pos.x;
                state.dragStartOffsetX = state.offset.x;
                state.velocity = {0.0f, 0.0f};
                handled = true;
            }
        }

        // Click on track: jump
        if (!handled && canScrollY && !canScrollX) {
            float ratio = (rect.height > 0.0f)
                ? ((pointer.pos.y - rect.y) / rect.height)
                : 0.0f;
            state.offset.y = std::clamp(
                ratio * contentSize.y - rect.height * 0.5f,
                0.0f, state.maxScroll.y);
        }
        else if (!handled && canScrollX && !canScrollY) {
            float ratio = (rect.width > 0.0f)
                ? ((pointer.pos.x - rect.x) / rect.width)
                : 0.0f;
            state.offset.x = std::clamp(
                ratio * contentSize.x - rect.width * 0.5f,
                0.0f, state.maxScroll.x);
        }
    }
}
```

The logic, when the pointer is pressed on this container (topmost
consumer):

1. Check the vertical thumb rect. If the pointer is on it, start a
   vertical drag.
2. Otherwise, check the horizontal thumb rect. If the pointer is on it,
   start a horizontal drag.
3. Otherwise, if only one axis is scrollable, treat the click as a
   "jump to position" on the track. The new offset is
   `ratio * contentSize - rect.size * 0.5`, so the clicked point ends up
   in the middle of the container.

The thumb drag snapshots the current mouse position and offset. On
subsequent frames (see §6.4), the delta is applied.

The track click is only enabled if exactly one axis is scrollable. If
both are, the click on the track is ignored (to avoid ambiguity about
which axis to jump on). In practice, both scrollable is rare.

### 6.4 Drag update

```cpp
else if (pointer.down) {
    if (canScrollY) {
        float dy = pointer.pos.y - state.dragStartMouseY;
        float ratio = (rect.height > 0.0f)
            ? (contentSize.y / rect.height)
            : 1.0f;
        state.offset.y = std::clamp(
            state.dragStartOffsetY + dy * ratio,
            0.0f, state.maxScroll.y);
    }
    if (canScrollX) {
        float dx = pointer.pos.x - state.dragStartMouseX;
        float ratio = (rect.width > 0.0f)
            ? (contentSize.x / rect.width)
            : 1.0f;
        state.offset.x = std::clamp(
            state.dragStartOffsetX + dx * ratio,
            0.0f, state.maxScroll.x);
    }
}
```

While the pointer is down (and capture is set), the offset is computed
from the drag start:

```
new_offset = drag_start_offset + (mouse_delta * ratio)
```

where `ratio = contentSize / containerSize`. This maps the mouse delta
in container pixels to the corresponding content pixels. If the content
is 3× the container, dragging the mouse by 10px scrolls the content by
30px. This matches the "thumb is a small version of the content"
intuition.

The `dragStartOffset` snapshot prevents drift: each frame's offset is
computed from the original, not from the previous frame's offset. This
keeps the drag stable.

### 6.5 Wheel

```cpp
if (!ctx.wheelConsumedThisFrame && node.isInteractive &&
    !node.hasPointerCapture() && rect.contains(pointer.pos))
{
    if (pointer.wheelY != 0.0f) {
        if (ctx.shiftHeld && canScrollX) {
            state.velocity.x -= pointer.wheelY * ScrollState::WHEEL_IMPULSE;
            ctx.wheelConsumedThisFrame = true;
        }
        else if (!ctx.shiftHeld && canScrollY) {
            state.velocity.y -= pointer.wheelY * ScrollState::WHEEL_IMPULSE;
            ctx.wheelConsumedThisFrame = true;
        }
    }
}
```

Conditions:

- `!wheelConsumedThisFrame` — no node has consumed the wheel this frame.
- `node.isInteractive` — the node is interactive (default true for most).
- `!hasPointerCapture` — not currently in a capture (avoid double-handling).
- `rect.contains(pointer.pos)` — the pointer is over the container.

If all four are true and there's a wheel delta:

- With `Shift`: scroll horizontally.
- Without `Shift`: scroll vertically.

The wheel adds an **impulse to velocity**, not to offset. So the actual
scroll happens through `tickInertia`, giving smooth deceleration.

The sign is negated: `velocity.y -= wheelY * IMPULSE`. A positive
`wheelY` (scroll down / away from user) decreases velocity, which
decreases offset, which scrolls up (content moves down). This matches
the convention where scrolling down moves content up... actually, it
depends on the backend's sign convention. Raylib's
`GetMouseWheelMove()` returns positive for scrolling up and negative
for scrolling down (by default). So `wheelY = -1` for a "scroll down"
action. Then `velocity.y -= (-1) * IMPULSE = velocity.y + IMPULSE`,
increasing the offset, moving content up. Correct.

The `wheelConsumedThisFrame` flag prevents nested scroll containers
from both scrolling. The `topmostConsumer` (the innermost scroll
container) gets the event first (it's deeper in the tree, updated
first), consumes it, and outer containers skip.

### 6.6 `state.tickInertia(ctx.dt)` and `scrolling`

```cpp
state.tickInertia(ctx.dt);

scrolling = node.hasPointerCapture() ||
            std::abs(state.velocity.x) > ScrollState::VELOCITY_MIN ||
            std::abs(state.velocity.y) > ScrollState::VELOCITY_MIN;
```

`tickInertia` applies velocity to offset and decays it. Then `scrolling`
is set to true if:

- The pointer is captured (drag in progress), OR
- There's still significant velocity (inertia in progress).

This flag is propagated up to `Layout::update` via
`ancestorScrolling`, and used by `updateFlags` to freeze hover (see §8).

---

## 7. Scrollbar geometry

The scrollbar thumb geometry is computed on demand by
`verticalThumb(node)` and `horizontalThumb(node)`. Both are const
methods that return a `Rect`.

### 7.1 `verticalThumb`

```cpp
Rect ScrollController::verticalThumb(const Layout& node) const {
    const auto& st = node.style_.currentStyle;
    const Rect& rect = node.rect;

    const float TRACK_W = 6.0f;
    const float MARGIN  = 4.0f;

    const bool wantH = isBarVisible(st.overflowX, state.maxScroll.x);
    const float reservedH = wantH ? (TRACK_W + MARGIN) : 0.0f;

    float trackH = std::max(0.0f, rect.height - MARGIN * 2.0f - reservedH);
    float contentH = contentSize.y;
    float viewportRatio = (contentH > 0.0f) ? (rect.height / contentH) : 1.0f;
    float thumbH = std::min(trackH, std::max(30.0f, trackH * viewportRatio));
    float scrollRatio = (state.maxScroll.y > 0.0f)
        ? (state.offset.y / state.maxScroll.y)
        : 0.0f;
    float available = std::max(0.0f, trackH - thumbH);
    float thumbY = rect.y + MARGIN + std::clamp(scrollRatio * available, 0.0f, available);

    return {rect.x + rect.width - TRACK_W - MARGIN, thumbY, TRACK_W, thumbH};
}
```

Steps:

1. Check if the horizontal bar is visible. If so, the vertical track
   shrinks by `TRACK_W + MARGIN` to make room for the horizontal bar's
   thickness at the bottom.
2. `trackH` — the vertical space available for the track, after margins
   and the horizontal bar.
3. `viewportRatio = rect.height / contentH` — the fraction of content
   visible at once. If content is 3× the container, this is `1/3`.
4. `thumbH` — the thumb height, `trackH * viewportRatio`, clamped to
   minimum 30px and maximum `trackH`.
5. `scrollRatio = offset.y / maxScroll.y` — the scroll position, 0 at
   top, 1 at bottom.
6. `available = trackH - thumbH` — the free space the thumb can move
   through.
7. `thumbY` — the thumb's top edge, offset by `scrollRatio * available`.

The result is a rect on the right side of the container
(`rect.x + rect.width - TRACK_W - MARGIN`), with the computed height
and position.

### 7.2 `horizontalThumb`

Symmetric, with `trackW`, `reservedV`, and returning a rect on the
bottom of the container.

### 7.3 `isBarVisible`

```cpp
static bool isBarVisible(Overflow o, float maxScroll) {
    if (o == Overflow::Scroll) return true;
    if (o == Overflow::Auto)   return maxScroll > 0.0f;
    return false;
}
```

- `Scroll` → always visible.
- `Auto` → visible only when `maxScroll > 0`.
- `Hidden` / `Visible` → never visible.

This is the CSS behavior.

### 7.4 Constant dimensions

The track width (`TRACK_W = 6.0f`) and margin (`MARGIN = 4.0f`) are
hardcoded. There's no `.zstyle` property for them — a future extension
would make them configurable.

---

## 8. Hover freeze

The most subtle scroll behavior is the "hover freeze": during a scroll,
hover state is not recomputed for any node.

### 8.1 The propagation

In `Layout::update`:

```cpp
const bool scrolling = ancestorScrolling || scroll_.scrolling;
input_.updateFlags(*this, selfBlocked, scrolling);
```

The `scrolling` flag is computed as "this node is scrolling OR an
ancestor is scrolling". It propagates down through the tree.

`updateFlags`:

```cpp
if (scrolling) {
    bool focusNow = ctx.hasFocus(&node);
    if (focusNow != node.isFocused) {
        node.isFocused = focusNow;
        node.pendingTransition = true;
    }
    return;
}
```

During scroll, only `isFocused` is updated. `isHovered` and `isPressed`
are not touched — they keep their previous values.

### 8.2 Why

Without this, scrolling a list of 100 buttons would fire `onHoverEnter`
and `onHoverExit` on each button as it passed under the stationary
cursor. For a 60fps scroll, that's 100 events per second across the
list. Catastrophic for any hover-driven behavior (tooltips, animations).

With the freeze, the hover state is "frozen" until the scroll ends.
Then the normal update runs, and the correct node (the one under the
cursor after the scroll stops) gets `onHoverEnter`.

### 8.3 Which scroll counts

`scrolling` is true if:

- The node has pointer capture (a drag on its scrollbar).
- The node's velocity is above `VELOCITY_MIN` (inertia in progress).

A node with a scroll but no active drag or inertia (e.g. just sitting
at a scroll position) doesn't set `scrolling`. So the hover freeze only
applies during active scrolling.

### 8.4 The freeze is global, not per-subtree

Because `scrolling` propagates down, **all descendants** of a scrolling
container are frozen. A node that's a sibling of the scroll container
is not frozen — the `ancestorScrolling` flag comes from the container's
own update, not from the root.

So a page with a scrollable sidebar: the sidebar and its content are
frozen during scroll, but the main content area is not. Hover works
normally in the main area.

### 8.5 Focus is still updated

Focus changes (from `requestFocus` during `updateTree`) are still
applied during the freeze. So if a node is focused and then a scroll
starts, the node's `:focus` styling is unaffected. This is correct —
focus isn't spatial.

---

## 9. Scrollbar rendering

`ScrollController::drawScrollbar(node, parentOpacity)` is called from
`Layout::draw` after content and children:

```cpp
scroll_.drawScrollbar(*this, parentOpacity);
```

The implementation:

```cpp
void ScrollController::drawScrollbar(const Layout& node, float parentOpacity) const {
    const auto& st = node.style_.currentStyle;
    const Rect& rect = node.rect;

    const bool wantV = isBarVisible(st.overflowY, state.maxScroll.y);
    const bool wantH = isBarVisible(st.overflowX, state.maxScroll.x);
    if (!wantV && !wantH) return;

    auto r = UIContext::get().renderer;
    if (!r) return;

    float op = st.opacity * parentOpacity;
    if (op <= 0.001f) return;

    const float MARGIN  = 4.0f;
    const float TRACK_W = 6.0f;

    r->pushClip(rect);

    const float reservedV = wantV ? (TRACK_W + MARGIN) : 0.0f;
    const float reservedH = wantH ? (TRACK_W + MARGIN) : 0.0f;

    if (wantV) {
        Rect track = {
            rect.x + rect.width - TRACK_W - MARGIN,
            rect.y + MARGIN,
            TRACK_W,
            rect.height - MARGIN * 2 - reservedH
        };
        r->fillRoundedRect(track, TRACK_W * 0.5f,
                            Color{40, 40, 40, 180}.withAlpha(op));

        Rect thumb = verticalThumb(node);
        Color thumbColor = node.hasPointerCapture()
            ? Color{180, 180, 180, 255}
            : Color{130, 130, 130, 220};
        r->fillRoundedRect(thumb, TRACK_W * 0.5f, thumbColor.withAlpha(op));
    }

    if (wantH) {
        // ... symmetric for horizontal ...
    }

    r->popClip();
}
```

The scrollbar is drawn:

1. Only if at least one bar is visible.
2. With clipping to the container's rect (so the bar doesn't spill).
3. With a semi-transparent track color.
4. With a lighter thumb color when the container has capture (i.e.
   during a drag).

The thumb rect is reused from `verticalThumb` / `horizontalThumb`, so
the drawing and the input hit-test always agree on where the thumb is.

### 9.1 Opacity

The scrollbar's opacity is `st.opacity * parentOpacity`. Note the
`st.opacity` — the container's own opacity. So a faded container has a
faded scrollbar. If the container has `opacity: 0`, the scrollbar is
not drawn (early-out at `op <= 0.001f`).

The scrollbar color doesn't respond to the container's `color`
property — it's hardcoded (dark gray track, lighter gray thumb). A
future extension could make it stylable.

### 9.2 Clip

The scrollbar is drawn **inside** a clip to `rect`. This prevents the
bar from extending beyond the container (e.g. if the track is longer
than the container due to some edge case). The clip is pushed and
popped around the scrollbar drawing.

### 9.3 Order relative to children

The scrollbar is drawn **after** children. So it's on top of everything
the container draws, including any children. This is usually what you
want (the bar shouldn't be hidden by the content).

---

## 10. `ScrollView` widget

`ScrollView` is a thin wrapper:

```cpp
class ScrollView : public TLayout<ScrollView> {
public:
    explicit ScrollView(LayoutType t = LayoutType::Vertical)
        : TLayout<ScrollView>(t)
    {
        setInteractive(true);
        setBlocksRaycast(true);
        setStyleTag("ScrollView");
        getInlineDefaults().overflowX = Overflow::Scroll;
        getInlineDefaults().overflowY = Overflow::Scroll;
    }
};
```

It sets:

- `interactive: true` — so the wheel can be received.
- `blocksRaycast: true` — so clicks don't pass through to nodes behind.
- `styleTag: "ScrollView"` — for `.zstyle` targeting.
- `overflow-x` and `overflow-y` to `Scroll` **in inline defaults**.

The inline defaults mean a `.zstyle` rule can override them (e.g.
`ScrollView { overflow-y: auto; overflow-x: hidden; }`). The
`inlineDefaults` level is below CSS rules in the cascade, so any
stylesheet rule wins.

The default layout type is `Vertical`. Pass `Horizontal` explicitly for
horizontal scrolling.

### 10.1 Why not `Stack`?

A `ScrollView` with `Stack` would be excluded from the scroll arrange
path (see §1). So a `Stack` `ScrollView` would clip but not scroll.
The default `Vertical` is deliberate — it's the common case.

If you want a `Stack` with clipping only, use `Panel` with
`overflow: hidden`.

---

## 11. Common tasks

### 11.1 Adding a new scroll input source

For example, a gamepad joystick that scrolls. In the backend's
`pollInputEvents` or a custom extension to `IPlatform`, add the axis
value to `PointerState`. Then in `ScrollController::tickInput`, treat
it like a wheel event:

```cpp
if (pointer.scrollAxis != 0.0f) {
    state.velocity.y -= pointer.scrollAxis * AXIS_IMPULSE;
    ctx.wheelConsumedThisFrame = true;
}
```

### 11.2 Smooth scroll to a target

The framework doesn't have a built-in "smooth scroll to". You can
implement it in user code by setting velocity or offset over time:

```cpp
// In a custom widget's onUpdate:
float target = 0.0f;   // top
float current = getScrollY();
float delta = target - current;
if (std::abs(delta) < 0.5f) {
    scrollToY(target);
} else {
    scrollToY(current + delta * 0.15f);   // exponential ease
}
```

Or use the velocity mechanism directly for a spring-like feel.

### 11.3 Custom scrollbar rendering

Override `drawScrollbar` if you need a custom look. It's a const
method, and the fields it reads are public:

```cpp
struct CustomScrollView : public ScrollView {
    void drawScrollbar(const Layout& node, float op) const override {
        // ... custom drawing ...
    }
};
```

Wait, `drawScrollbar` isn't virtual — it's a concrete method of
`ScrollController`, not `Layout`. To customize, you'd need to change
`ScrollController` or add a hook.

A cleaner approach: don't use the built-in scrollbar. Set `overflow:
hidden` to disable it (and disable scroll input), and implement your
own bar as a separate widget that reads `getScrollY()` / `getMaxScrollY()`
and calls `scrollToY()`.

### 11.4 Custom inertia

The inertia parameters (`WHEEL_IMPULSE`, `DECAY`, `VELOCITY_MIN`) are
`static constexpr` in `ScrollState`. To change them, you'd need to
modify the header or shadow them with a subclass of `ScrollState`
(which isn't possible for `Layout`'s member without changing the type).

In practice, the parameters are tuned for a "one wheel tick = a small
scroll with medium deceleration" feel. If you need very different
behavior, extend the API.

---

## 12. Pitfalls

**`contentSize` is only meaningful for scroll containers.**
For a non-scroll node, `contentSize` is set in `measure` but never used.
If you read it from a non-scroll node, it's the intrinsic size of the
children — usually the same as `measuredSize` minus padding.

**`maxScroll` is clamped to zero.**
If the content fits the container, `maxScroll = 0` and no scroll input
has any effect. `canScrollY` / `canScrollX` correctly return false, so
the wheel / drag / track click are all no-ops.

**The scrollbar is always drawn on top.**
It's drawn after children in `draw`, so it's on top of everything the
container renders. If you have a child that draws over the right edge
of the container, the scrollbar is on top of it. This is intentional.

**The scrollbar is not stylable.**
Its width (6px), margin (4px), and colors are hardcoded. A future
extension could expose them via `.zstyle` (`::scrollbar-track` /
`::scrollbar-thumb` parts), but it's not implemented.

**The hover freeze is not per-axis.**
If either axis is scrolling, both are frozen. A node that's scrolling
vertically but not horizontally still has both `isHovered` and
`isPressed` frozen. In practice, scrolling is usually mono-axis, so
this doesn't matter.

**Absolute children scroll with the content.**
`translateSubtree` doesn't skip absolute children — they're shifted
along with the flow children. So an absolute child of a scroll
container moves when the container scrolls. This is usually correct
(an absolute badge inside a card should scroll with the card), but if
you want a fixed-position child inside a scroll container, this won't
work.

Workaround: put the fixed child as a **sibling** of the scroll
container, positioned absolutely over it. Or use a portal (though
portals have their own issues).

**Portals inside scroll containers.**
`translateSubtree` doesn't skip portals either (the check is commented
out). A portal's `onUpdate` recomputes its `rect` each frame, which
overrides the translation on the portal itself. But the portal's
**children** are translated, so they can be offset unexpectedly.

The fix: use portals **outside** the scroll container whenever possible.
If you need a portal inside a scroll container (e.g. a dropdown inside
a scrolled list), test carefully. The `Dropdown` widget does this and
works because its list portal is repopulated each frame based on the
trigger's position.

**The `wheelConsumedThisFrame` flag is per-frame.**
A wheel event consumed by one scroll container is consumed for **all**
containers in the same frame. A wheel event on a nested scroll
container is only seen by the innermost one. This is intentional
(matches most browsers' behavior), but if you want the outer container
to also react, you'd need to disable consumption.

**`scrollTo` doesn't reset velocity.**
If you call `scrollTo(x, y)` during inertia, the velocity is still
non-zero and will continue to move the offset on the next frame. To
stop inertia, set `velocity = {0, 0}`:

```cpp
auto& scroll = getScrollState();   // not a public API
```

Actually, there's no public API to access the `ScrollState`. The
`scrollTo` methods only set `offset`. To stop inertia, you'd need to
add a method or extend the API.

In practice, `scrollTo` is used for programmatic jumps, and inertia is
usually not active when that happens. But it's a footgun.

**`scrollToBottom` uses `maxScroll` from the last arrange.**
If the content just changed (e.g. new items added), `maxScroll` might
be stale. The typical pattern is:

```cpp
addChildren(...);         // modifies tree, marks dirty
// next runFrame recomputes maxScroll
// then scrollToBottom works correctly
```

Calling `scrollToBottom` in the same frame as adding children might not
scroll all the way. In practice, this is rare.

**The scrollbar thumb size has a minimum.**
`thumbH = std::min(trackH, std::max(30.0f, trackH * viewportRatio))` —
the minimum is 30px. For a very long content (e.g. 10000 items in a
300px container), the thumb would be `300 * (300 / 10000) = 9px`
without the minimum, but it's clamped to 30px. This makes the thumb
usable but breaks the "thumb is a proportional representation"
intuition for very long content.

**`applyOffsetDelta` translates by the delta, not the absolute offset.**
This means the translation is cumulative. If some code resets the
subtree (e.g. by calling `arrangeInto` without resetting the applied
offset), the next `applyOffsetDelta` would double-count. The arrange
code handles this by resetting `appliedOffset = {0, 0}` after a full
arrange.

**Scroll input requires `isInteractive`.**
The wheel check has `node.isInteractive &&`. A non-interactive scroll
container (e.g. one with `setInteractive(false)`) won't respond to the
wheel. If you want a non-interactive container that scrolls, you'd
need to either make it interactive (and accept the click interception)
or add the input handling elsewhere.

Actually, `ScrollView::ScrollView` sets `setInteractive(true)`. So the
default is interactive. But if you build a scrollable node without
using `ScrollView` (just a `Layout` with `overflow: scroll`), you need
to set interactive manually.

**`Stack` with `overflow: scroll` doesn't scroll.**
The `arrange` code skips scroll handling for `Stack`. The `tickInput`
code also skips it (`node.type == LayoutType::Stack`). So a `Stack`
with `overflow: scroll` clips but doesn't scroll. Use `Vertical` or
`Horizontal` for scrollable content.

**The `anchor` for `scrollToTop` etc.**
`scrollToTop()` calls `scrollToY(0.0f)`. It assumes the "top" is
offset 0. This is correct for a vertical scroll, but the semantic
("show the first item") depends on the content layout. For a `Stack`
container (which is excluded from scrolling), this doesn't work.

**The scrollbar is not drawn during the hover freeze.**
Actually, it is — the freeze only affects hover state, not rendering.
The scrollbar is drawn every frame the container is visible and has a
visible bar. During a drag, the thumb color changes (lighter) but the
drawing continues.

**Focus and scroll interact.**
`requestFocus` on a node inside a scroll container doesn't
automatically scroll it into view. If you want focus to scroll the
container, you need to call `scrollToY` manually in your focus
handling. There's no `scrollIntoView` API.

**The scroll controller state is per-node, not shared.**
Nested scroll containers have independent state. Scrolling the inner
one doesn't affect the outer. This is the expected behavior, but it
means there's no "synchronized scroll" or "linked scroll" feature.

**`ScrollState::reset` clears maxScroll too.**
This is important: after a reset, `maxScroll = {0, 0}`, so no scrolling
is possible until the next `updateMaxScroll`. This happens in the same
`arrange` call that follows, so the reset is transient. But if you
read `maxScroll` between the reset and the next `updateMaxScroll`,
it's zero.

**The scrollbar's "capture" state is used to lighten the thumb.**
`node.hasPointerCapture()` returns true during a scrollbar drag. The
thumb color is `{180, 180, 180, 255}` during capture, else
`{130, 130, 130, 220}`. This gives the visual feedback that the user is
dragging. But capture can also be held by other interactions (a slider
drag inside the container), so the scrollbar might lighten unexpectedly.

**`WHEEL_IMPULSE` and the initial velocity.**
A single wheel tick produces `WHEEL_IMPULSE = 400 px/s` of velocity.
With exponential decay `0.9^60 ≈ 0.0018` per second, the total distance
traveled is:

```
∫ v(t) dt = v0 * ∫ 0.9^(60t) dt = v0 / (60 * ln(1/0.9)) ≈ v0 / 6.3
```

For `v0 = 400`, total distance ≈ 63px. So one wheel tick scrolls about
63px, which is a reasonable "row or two" per tick.

If you want more aggressive scrolling, increase `WHEEL_IMPULSE`. The
`DECAY` controls the "feel" — lower decay (faster decay) means a
shorter, more snappy scroll; higher decay means a longer, smoother
scroll.