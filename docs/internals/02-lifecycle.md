# Lifecycle

This document is the precise, step-by-step description of what happens
during a frame. It's a reference for contributors who need to know the
exact order of operations, which flags are set when, and what each
virtual hook sees.

If you're modifying the core, read this before touching `Layout::update`,
`Layout::measure`, `Layout::arrange`, or `Layout::draw`.

---

## 1. Frame-level flow

The user's main loop calls two methods on the root `Layout`:

```cpp
while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    root->runFrame(dt);      // logic: input, style, animation, layout

    BeginDrawing();
    ClearBackground(BLACK);
    root->renderFrame();     // draw
    EndDrawing();
}
```

Both methods are defined on `Layout` and are documented as "called by
the runtime, not overridable". They're the entry points from user code
into the framework.

```
runFrame(dt)
  ├── UIContext::beginFrame(dt)
  ├── updateTree(dt)
  │     ├── hit-test → topmostConsumer
  │     ├── focus handling
  │     └── update(dt)         [recursive, per node]
  ├── measure(viewport)
  └── arrange(viewport rect)

renderFrame()
  ├── renderer->beginFrame()
  ├── draw(1.0f)               [recursive, per node]
  └── renderer->endFrame()
```

The split between logic and draw is not arbitrary. `measure` / `arrange`
don't touch the GPU, and `draw` doesn't touch input. Keeping them
separate is what makes the framework testable headlessly and lets the
two phases run at different rates if needed.

---

## 2. `beginFrame(dt)`

Called once per frame, on the `UIContext` singleton.

```cpp
void UIContext::beginFrame(float delta_time) {
    dt = delta_time;
    time += delta_time;

    // Clear per-frame flags
    wheelConsumedThisFrame = false;
    clickConsumed = false;
    rightClickConsumed = false;
    topmostConsumer = nullptr;
    hoverTarget = nullptr;
    releaseTarget = nullptr;

    // Rotate the portal buffers
    activePortals = std::move(framePortals);
    framePortals.clear();

    // Poll platform
    if (platform) {
        Vec2 newVp = platform->viewportSize();
        EdgeInsets newSafe = platform->safeArea();
        if (newVp.x != Metrics::viewport.x ||
            newVp.y != Metrics::viewport.y ||
            newSafe != safeArea)
        {
            viewportGeneration++;
        }
        Metrics::viewport = newVp;
        safeArea = newSafe;
        pointer = platform->pointer();
        shiftHeld = platform->shiftHeld();
        inputEvents = platform->pollInputEvents();
        dpiScale = platform->dpiScale();

        if (renderer) renderer->setDpiScale(dpiScale);
    }
}
```

### What `beginFrame` **does not** do

- It doesn't hit-test. That's `updateTree`.
- It doesn't set `pressTarget`. That's `updateTree`.
- It doesn't grant or release focus. That's `updateTree`.
- It doesn't resolve styles. That's `update`.

### The `dt` snapshot

`UIContext::dt` is the only place `dt` is stored. Subsystems read it
via `UIContext::get().dt` rather than passing it down. This avoids
parameter threading through deep call chains (`ScrollController::tickInertia`
reads `ctx.dt`, `TextInput::onUpdate` receives `dt` as a parameter, but
inconsistent — see the source).

### The viewport generation counter

Whenever `Metrics::viewport` or `safeArea` changes, `viewportGeneration`
is incremented. Each `Layout` caches the last seen generation in
`lastViewportGen_` and compares it in `update`:

```cpp
uint32_t gen = UIContext::get().viewportGeneration;
if (gen != lastViewportGen_) {
    lastViewportGen_ = gen;
    pendingTransition = true;
}
```

This forces a style re-resolution on resize, which is what makes `@media`
queries react to viewport changes. Without it, a resize would change the
available space but not the resolved styles.

---

## 3. `updateTree(dt)`

Called on the root. Orchestrates input, focus, and the recursive update.

### 3.1 Hit-testing the pointer

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

- **No capture**: hit-test normally, get the topmost interactive or
  blocking node at the cursor.
- **Capture active and enabled**: the captured node becomes
  `topmostConsumer`, regardless of position. This is how drag
  interactions work.

If the captured node was disabled, capture is dropped and normal
hit-testing resumes.

### 3.2 Setting hover / press / release targets

```cpp
ctx.hoverTarget = ctx.topmostConsumer;
if (ctx.pointer.pressed)  ctx.pressTarget   = ctx.hoverTarget;
if (ctx.pointer.released) ctx.releaseTarget = ctx.hoverTarget;
```

- `hoverTarget` — updated every frame.
- `pressTarget` — set only on the frame of a press.
- `releaseTarget` — set only on the frame of a release.

These are used later by `InputController::updateFlags` and
`fireCallbacks`.

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

Clicking on a focusable node focuses it. Clicking anywhere else releases
focus. This is the only place focus changes from a click.

### 3.4 Tab navigation

```cpp
for (int k : ctx.inputEvents.keys) {
    if (k != Key::Tab) continue;

    Layout *scope = this;
    {
        auto focused = ctx.focusedNode.lock();
        if (focused) {
            Layout *n = focused.get();
            while (n) {
                if (n->isFocusScope()) { scope = n; break; }
                n = n->getParent().get();
            }
        }
    }

    std::vector<Layout *> focusables;
    collectFocusables(scope, focusables);
    if (focusables.empty()) break;

    auto cur = ctx.focusedNode.lock();
    Layout *curPtr = cur.get();
    int idx = -1;
    for (size_t i = 0; i < focusables.size(); ++i)
        if (focusables[i] == curPtr) { idx = (int)i; break; }

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
6. Notify all ancestors of the newly focused node (fires
   `onDescendantFocused`).

Only the **first** Tab event per frame is processed (the `break` after
the switch). Multiple Tab keys in the same frame are collapsed into one.

### 3.5 Recursive update

```cpp
update(dt, false);
```

Calls the per-node `update` (see §4) with `ancestorBlocked = false` and
`scrolling = false`. The arguments propagate down to children.

### 3.6 Post-update cleanup

```cpp
if (ctx.pointer.released)   ctx.pressTarget = nullptr;
if (!ctx.pointer.down && !ctx.pointerCapture.expired())
    ctx.pointerCapture.reset();
```

- `pressTarget` is cleared after the frame where the release happened.
  This is what makes `pressTarget != releaseTarget` checks in the next
  frame evaluate correctly.
- Pointer capture is released automatically when the button is up. A
  widget can hold capture across frames while the button is down, and
  it's dropped as soon as the button is released.

Note the ordering: `pressTarget` is cleared **after** `update` runs,
so during `update` both `pressTarget` and `releaseTarget` are available
for click validation.

---

## 4. Per-node `update(dt, ancestorBlocked, ancestorScrolling)`

Called for every node, depth-first. This is the bulk of the framework's
logic.

### 4.1 Snapshot

```cpp
bool wasPending = pendingTransition;
ComputedStyle styleBefore = style_.currentStyle;
```

Both are captured at the start so that `recomputeDirty` at the end can
compare against them.

### 4.2 Style initialization

```cpp
initStyleIfNeeded();
```

If this node has never been styled (first frame), resolve its initial
style:

```cpp
void initStyleIfNeeded() {
    if (!styleInitialized) {
        styleInitialized = true;
        style_.currentStyle = style_.targetStyle =
            style_.transitionStartStyle = style_.resolveFor(*this);
        pendingTransition = false;
        anim_.syncCss(*this);
    }
}
```

The initial style comes from the cascade, with the state flags as they
are (usually all false → `Idle`).

### 4.3 Scroll reset on overflow change

```cpp
scroll_.resetIfOverflowChanged(*this);
```

If the node's `overflow-x` or `overflow-y` changed since last frame,
reset the scroll state (offset, velocity, drag start). This prevents
"phantom scroll" when a container stops being scrollable.

### 4.4 Viewport generation check

```cpp
uint32_t gen = UIContext::get().viewportGeneration;
if (gen != lastViewportGen_) {
    lastViewportGen_ = gen;
    pendingTransition = true;
}
```

Forces a style re-resolution on viewport change, which is how media
queries react.

### 4.5 Imperative animation tick

```cpp
bool localAnimBlocks = anim_.tickImperative(*this, dt);
bool blockSubtree = ancestorBlocked || localAnimBlocks;
bool selfBlocked = blockSubtree || !isEnabled || (!isInteractive && blocksRaycast);
```

`tickImperative` walks the `imperative` map, advances `elapsed` for each
playing animation, applies each track's setter, and returns whether any
animation has `blocksInput == true`. The returned value **and** the
inherited `ancestorBlocked` compose to form `blockSubtree`.

`selfBlocked` is true if the subtree is blocked, the node is disabled,
or the node isn't interactive but blocks raycast. Blocked nodes don't
update their hover / pressed flags.

### 4.6 Flag updates

```cpp
const bool scrolling = ancestorScrolling || scroll_.scrolling;
input_.updateFlags(*this, selfBlocked, scrolling);
```

`updateFlags` (in `Input.cpp`):

- If `scrolling` is true: only update `isFocused`; skip hover and press
  (the "hover freeze").
- Otherwise: update `isHovered` (pointer inside `rect`, if not blocked),
  `isPressed` (pointer down and press target is this node or a
  descendant, walking up through `passThrough`), and `isFocused`.

### 4.7 State machine

```cpp
UIState prevState = style_.currentState;
UIState nextState = input_.computeNextState(*this);
bool stateChanged = (style_.currentState != nextState);
```

`computeNextState` returns:

- `Disabled` if `!isEnabled`.
- `Pressed` if `isPressed`.
- `Hover` if `isHovered`.
- `Idle` otherwise.

The priority is: Disabled > Pressed > Hover > Idle. A disabled node
never reports Pressed, even if the pointer is over it.

### 4.8 Style transition trigger

```cpp
if (stateChanged) {
    style_.beginStateTransition(*this, nextState);
    anim_.syncCss(*this);
} else if (pendingTransition) {
    style_.resolvePendingTransition(*this);
    anim_.syncCss(*this);
}
```

Two paths:

- **State changed**: snapshot the current style as the transition start,
  set `currentState = nextState`, resolve the new target style, and
  reset the transition timer.
- **Pending transition** (some property was set this frame, e.g. via
  `getInlineBase()`): re-resolve the target style and, if it differs,
  restart the transition.

In both paths, `anim_.syncCss` is called to keep the CSS animation list
in sync with the new `targetStyle.animations`.

### 4.9 Style and animation ticks

```cpp
style_.tick(*this, dt);
anim_.tickCss(*this, dt);
style_.propagateInheritance(*this);
```

- `style_.tick` advances the CSS transition timer. If the timer reaches
  1.0, `currentStyle = targetStyle`.
- `anim_.tickCss` advances each active CSS keyframe animation.
- `propagateInheritance` compares the node's current inherited properties
  and state flags to the last snapshot, and if anything changed, marks
  each child as inheritance-dirty (via `markInheritanceDirty`).

### 4.10 `onAnimationsFinished`

```cpp
{
    bool anyLocal = anim_.hasActiveLocal();
    if (hadLocalAnimationsLast_ && !anyLocal && onAnimationsFinished)
        onAnimationsFinished();
    hadLocalAnimationsLast_ = anyLocal;
}
```

The callback fires **once** when the node transitions from "has active
animations" to "no active animations". `hasActiveLocal` checks
imperative, CSS node, and CSS part animations on this node, but not its
descendants.

### 4.11 Recurse into children

```cpp
size_t n = children.size();
for (size_t i = 0; i < n; ++i)
    children[i]->update(dt, blockSubtree, scrolling);
```

Depth-first, in declaration order. The `blockSubtree` and `scrolling`
flags propagate down.

Note: the size is captured before the loop. This prevents issues if a
child is added during update (which can happen if a widget's callback
calls `addChild`). The new child will be updated on the **next** frame.

### 4.12 Input post-processing

```cpp
scroll_.tickInput(*this);
input_.handleKeyInput(*this);
input_.fireCallbacks(*this, prevState, nextState, stateChanged);
```

- `tickInput` handles scroll: wheel events, scrollbar drag, inertia.
  It calls `state.tickInertia(ctx.dt)` at the end.
- `handleKeyInput` fires `onClick` via `Enter` / `Space` if the node
  is focused and `keyboardActivates_` is true.
- `fireCallbacks` fires the input callbacks in the right order:
  - `onHoverEnter` / `onHoverExit` on state change.
  - `onPress` on `pointer.pressed` (if the press path includes this node).
  - `onRelease` on `pointer.released` (if the release path includes
    this node).
  - `onClick` on a valid click (press target and release target both
    include this node, and not consumed).
  - `onRightClick` on `pointer.rightPressed` if the node is hovered.

Note: callbacks are fired **after** children have been updated. So the
order of callback invocation across the tree is bottom-up within a
subtree (children before parents), which matches the bubbling semantics
of click events.

### 4.13 Cull removed children

```cpp
cullRemovedChildren();
```

Erases children whose `wantsRemoval` flag is set (`removeFromParent()`
was called). This is the only place the `children` vector is modified
due to removal.

### 4.14 Dirty recomputation

```cpp
recomputeDirty(wasPending, styleBefore);
```

Determines whether this node's subtree needs a re-measure. A node is
dirty if:

- It was pending at the start of the frame (`wasPending`).
- It's pending now (`pendingTransition`).
- Any child's subtree is dirty.
- Its style changed (`style_.currentStyle != styleBefore`).
- It's in the middle of a transition (`transitionTimer < 1.0`).

`subtreeDirty_` is the cached flag used by `measure` to skip work.

### 4.15 `onUpdate(dt)`

```cpp
if (isEnabled || updateWhenDisabled_)
    onUpdate(dt);
```

The per-widget update. Skipped if the node is disabled and
`updateWhenDisabled_` is false.

This is where custom widgets handle their own per-frame logic (drag,
input processing, local state updates).

---

## 5. `measure(parentW, parentH)`

Recursive, but with an early-out for clean subtrees.

### 5.1 Cache check

```cpp
if (!subtreeDirty_ && parent_w == lastMeasureW_ && parent_h == lastMeasureH_)
    return measuredSize;
lastMeasureW_ = parent_w;
lastMeasureH_ = parent_h;
```

If the subtree is clean and the parent size hasn't changed, return the
cached `measuredSize`. Otherwise proceed.

### 5.2 Style resolution

```cpp
if (!styleInitialized) {
    styleInitialized = true;
    style_.currentStyle = style_.targetStyle =
        style_.transitionStartStyle = style_.resolveFor(*this);
    pendingTransition = false;
}

ComputedStyle calcStyle = pendingTransition ? style_.resolveFor(*this)
                                            : style_.currentStyle;
```

Uses `currentStyle` for measurement unless there's a pending transition,
in which case it re-resolves to get the target size. This means
mid-transition sizing uses the target (not the interpolated) value —
deliberate, to avoid layout jitter during transitions.

### 5.3 Available space

```cpp
float pl = calcStyle.padding.left.resolveH(parent_w, parent_h);
float pr = calcStyle.padding.right.resolveH(parent_w, parent_h);
float pt = calcStyle.padding.top.resolveV(parent_w, parent_h);
float pb = calcStyle.padding.bottom.resolveV(parent_w, parent_h);

float availW = calcStyle.width.isAuto()  ? parent_w : calcStyle.width.resolveH(parent_w, parent_h);
float availH = calcStyle.height.isAuto() ? parent_h : calcStyle.height.resolveV(parent_w, parent_h);

float innerW = std::max(0.0f, availW - pl - pr);
float innerH = std::max(0.0f, availH - pt - pb);

// Scroll containers measure without main-axis constraint
if (type == LayoutType::Vertical && calcStyle.overflowY != Overflow::Visible)
    innerH = 1e9f;
if (type == LayoutType::Horizontal && calcStyle.overflowX != Overflow::Visible)
    innerW = 1e9f;
```

`innerW` / `innerH` are the space available for children. For scroll
containers, the main axis is unbounded so children can be larger than
the container.

### 5.4 Intrinsic size

```cpp
Vec2 contentSize = computeIntrinsicSize(innerW, innerH);
```

The widget's own size, before children are accounted for. For `Text`
it's the measured text box, for `Checkbox` it's `font-size × 1.4`, etc.

### 5.5 Children measure

```cpp
for (auto& c : children) {
    Vec2 cs = c->measure(innerW, innerH);
    if (c->getStyle().position == Position::Absolute || c->isPortal())
        continue;

    // accumulate margins + sizes
    float childTotalW = cs.x + ml + mr;
    float childTotalH = cs.y + mt + mb;

    if (type == Horizontal) {
        totalMainSize += childTotalW;
        maxCrossSize = max(maxCrossSize, childTotalH);
    } else if (type == Vertical) {
        totalMainSize += childTotalH;
        maxCrossSize = max(maxCrossSize, childTotalW);
    } else if (type == Stack) {
        contentSize.x = max(contentSize.x, childTotalW);
        contentSize.y = max(contentSize.y, childTotalH);
    }
    visibleChildren++;
}
```

Children are measured first (so they know their own intrinsic sizes),
then combined. Absolute children and portals are measured but not
counted in the flow size.

### 5.6 Gap

```cpp
if (type == Horizontal) {
    if (visibleChildren > 1) totalMainSize += gapX * (visibleChildren - 1);
    contentSize.x = max(contentSize.x, totalMainSize);
    contentSize.y = max(contentSize.y, maxCrossSize);
} else if (type == Vertical) {
    if (visibleChildren > 1) totalMainSize += gapY * (visibleChildren - 1);
    contentSize.x = max(contentSize.x, maxCrossSize);
    contentSize.y = max(contentSize.y, totalMainSize);
}
```

`Stack` doesn't use gaps.

### 5.7 Final size

```cpp
scroll_.contentSize = contentSize;   // save for scroll

float finalW = calcStyle.width.isAuto()  ? (contentSize.x + pl + pr) : availW;
float finalH = calcStyle.height.isAuto() ? (contentSize.y + pt + pb) : availH;

float minW = calcStyle.minWidth.isAuto() ? 0.0f : calcStyle.minWidth.resolveH(parent_w, parent_h);
float maxW = calcStyle.maxWidth.isAuto() ? 1e9f : calcStyle.maxWidth.resolveH(parent_w, parent_h);
float minH = calcStyle.minHeight.isAuto() ? 0.0f : calcStyle.minHeight.resolveV(parent_w, parent_h);
float maxH = calcStyle.maxHeight.isAuto() ? 1e9f : calcStyle.maxHeight.resolveV(parent_w, parent_h);

measuredSize = {clamp(finalW, minW, maxW), clamp(finalH, minH, maxH)};
return measuredSize;
```

The `contentSize` is stored on the `ScrollController` so it can be used
later to determine `maxScroll`. The final size applies `min-*` / `max-*`
clamps as the last step.

---

## 6. `arrange(rect)`

Recursive, top-down.

### 6.1 Set rect and detect movement

```cpp
const bool positionChanged = (space.x != rect.x || space.y != rect.y);
rect = space;
```

If the position changed since last arrange, scroll containers may need
a full re-arrange (their virtual content coordinates shift).

### 6.2 Scroll containers vs normal

```cpp
if (isScrollContainer() && type != LayoutType::Stack) {
    if (scroll_.needsFullArrange(*this, positionChanged)) {
        Rect arrangeSpace = scroll_.computeArrangeSpace(*this, space);
        arrangeInto(arrangeSpace);
        scroll_.arrangeInitialized = true;
        scroll_.appliedOffset = {0, 0};
    }
    scroll_.applyOffsetDelta(*this);
    scroll_.updateMaxScroll(*this, space);
} else {
    arrangeInto(space);
    scroll_.arrangeInitialized = true;
}
```

- Normal nodes: `arrangeInto(space)` directly.
- Scroll containers: check if a full re-arrange is needed, expand the
  arrange space if so, then apply the scroll offset and update
  `maxScroll`.

`arrangeInto` sets the positions of all children. `applyOffsetDelta`
translates the whole subtree by the difference between the current
scroll offset and the last applied one (`translateSubtree`).

### 6.3 `onLayout()`

```cpp
onLayout();
```

Called after `arrangeInto` and the scroll handling. This is where widgets
can react to their final size (Text wrapping uses it).

### 6.4 `arrangeInto(space)`

```cpp
float pl = ... padding left ...
float pr = ... padding right ...
float pt = ... padding top ...
float pb = ... padding bottom ...

Rect inner = {
    space.x + pl,
    space.y + pt,
    max(0, space.width - pl - pr),
    max(0, space.height - pt - pb)
};

arrangeAbsoluteAndPortals(inner);

if (type == Vertical)    arrangeVerticalFlow(inner, gapY);
else if (type == Horizontal) arrangeHorizontalFlow(inner, gapX);
else if (type == Stack)  arrangeStackFlow(inner);
```

The inner rect is the arrange space minus padding. Absolute and portal
children are arranged first into the inner rect. Then flow children are
laid out according to the container type.

### 6.5 Flow arrangement (Vertical)

The algorithm, in `arrangeVerticalFlow`:

1. Sum the fixed heights of all in-flow children: `margin-top + measuredSize.y + margin-bottom`.
2. Sum `grow` and `shrink` across all children.
3. Add `gapY × (N - 1)`.
4. Compute `freeSpace = inner.height - totalFixedH`.
5. If `freeSpace > 0` and `totalGrow == 0`, apply `justify`:
   - `Center` → offset start by `freeSpace / 2`.
   - `End` → offset start by `freeSpace`.
   - `SpaceBetween` → distribute `freeSpace / (N-1)` between children.
6. For each child:
   - Height: `measuredSize.y + grow-share` or `+ shrink-share`,
     clamped by `min-height` / `max-height`.
   - Width: `measuredSize.x`, clamped by `min-width` / `max-width`,
     clamped by available width (inner width - margins).
   - Cross-axis alignment (`alignH` on the child or `itemsH` on the
     container):
     - `Stretch` → width = available width.
     - `Center` → offset x by `(availW - w) / 2`.
     - `End` → offset x by `availW - w`.
   - Call `c->arrange({x, y, w, h})`.

The `grow` / `shrink` computation uses the total across all children
with that property:

```cpp
if (freeSpace > 0 && totalGrow > 0 && cst.grow > 0)
    ch += freeSpace * (cst.grow / totalGrow);
else if (freeSpace < 0 && totalShrink > 0 && cst.shrink > 0)
    ch += freeSpace * (cst.shrink / totalShrink);
```

Horizontal is symmetric (swap width/height, gapX/gapY, alignH/alignV).

### 6.6 Stack arrangement

`arrangeStackFlow` iterates in-flow children and positions each one
independently:

- `availW` = inner width minus the child's margins.
- `availH` = inner height minus the child's margins.
- `cw` = clamped `measuredSize.x` against `min-*`, `max-*`, and `availW`.
- `ch` = same for height.
- Positioned at `(inner.x + margin-left, inner.y + margin-top)`,
  then adjusted by `alignH` / `alignV` / `itemsH` / `itemsV`.

Since there's no flow, children overlap unless positioned differently
(by margins, alignment, or absolute positioning).

### 6.7 Absolute positioning

`arrangeAbsolute(c, cst, in)` handles a single absolute child:

- Parse `left`, `right`, `top`, `bottom` — each can be set or `auto`.
- Horizontal:
  - `left + right` set and `width: auto` → stretch.
  - `left` only → position from left.
  - `right` only → position from right.
  - Neither → position at start (with margin).
- Vertical: symmetric.

The stretch case computes `w = in.width - L - R - ml - mr` and positions
at `in.x + L + ml`.

### 6.8 Portals

```cpp
if (c->isPortal()) {
    c->arrange({inner.x, inner.y, c->measuredSize.x, c->measuredSize.y});
    continue;
}
```

Portals get a default arrangement at the parent's inner origin with
their own measured size. They typically override `arrange` to be a
no-op and position themselves in `onUpdate`.

---

## 7. `draw(parentOpacity)`

Recursive, top-down. Draws the subtree.

### 7.1 Portal registration

```cpp
if (isPortal_) {
    UIContext::get().framePortals.push_back(weak_from_this());
    return;
}
```

Portal nodes don't draw themselves; they register for later drawing at
the root.

### 7.2 Frustum culling

```cpp
if (rect.width > 0.0f && rect.height > 0.0f) {
    Rect clip = renderer->getClipRect();
    float m = 40.0f;
    if (rect.x > clip.x + clip.width + m ||
        rect.x + rect.width < clip.x - m ||
        rect.y > clip.y + clip.height + m ||
        rect.y + rect.height < clip.y - m)
        return;
}
```

If the node is entirely outside the current clip (with a 40px margin
for anti-aliasing and blurs), skip it entirely. This is the primary
performance optimization for large scrollable lists.

### 7.3 Render style

```cpp
ComputedStyle renderStyle = style_.currentStyle;
Anim::overlayCssComputed(renderStyle, anim_.css);
resolveEffectIfNeeded(renderStyle);

float globalOp = renderStyle.opacity * parentOpacity;
if (globalOp <= 0.001f) return;
```

`renderStyle` is the style used for drawing, with CSS animation values
overlaid on top of the current style. `resolveEffectIfNeeded` looks up
the effect name to a shader handle if it changed.

If the effective opacity is below a threshold, skip entirely.

### 7.4 Choose inline or layer

```cpp
const bool isLayer =
    !renderStyle.filters.empty() &&
    renderer->supports(Feature::Effects) &&
    rect.width > 0.0f && rect.height > 0.0f &&
    !renderer->inTarget();

if (isLayer) drawLayer(renderStyle, globalOp);
else         drawInline(renderStyle, globalOp);
```

`drawLayer` renders the subtree to a target and applies filters.
`drawInline` draws directly. See [Render Pipeline](06-render-pipeline.md)
for the details.

### 7.5 Scrollbar

```cpp
scroll_.drawScrollbar(*this, parentOpacity);
```

Drawn after the content and children, on top. Uses `parentOpacity`
(not `globalOp`) — the scrollbar's own opacity comes from the node's
style.

### 7.6 Portal drain

```cpp
if (!hasParent()) {
    auto portals = UIContext::get().framePortals;
    for (auto &wp : portals) {
        auto sp = wp.lock();
        if (!sp) continue;
        bool wasPortal = sp->isPortal();
        sp->setPortal(false);
        sp->draw(1.0f);
        sp->setPortal(wasPortal);
    }
}
```

Only the root runs this. Each portal is drawn with the flag temporarily
cleared, so it renders normally (with its own children and effects).

Note: `framePortals` is copied first. This is important because
`sp->draw()` might register **new** portals (a portal's child can also
be a portal), and iterating while modifying would invalidate the
iterator.

Also: `activePortals` for the current frame was already moved out of
`framePortals` in `beginFrame`. So `framePortals` here contains only
the portals registered **this** frame — plus any registered by earlier
portals in this same loop.

---

## 8. When things happen that surprise people

A few non-obvious ordering facts:

**Callbacks fire bottom-up, during update, before measure.**
`onClick` fires during `Layout::update`, which is before `measure` and
`arrange` in the same frame. So if your callback reads `getRect()`, it
sees the rect from the **previous** frame. This is fine in practice
(you usually just want to change state, which will be reflected next
frame), but worth knowing.

**Style changes take effect next frame.**
If `onClick` sets `getInlineBase().background = ...`, the change is
`pendingTransition = true` for the current node. The current frame's
`draw` uses the old style; the next frame's `update` resolves the new
style, and the next frame's `draw` shows it. If you want the change
immediate, you'd need to call `style_.resolvePendingTransition(*this)`
manually — but this is not public API.

**Portals have a one-frame delay to appear.**
A portal registers itself in `framePortals` when it's drawn. The root
drains `framePortals` at the end of the same `draw`. So a portal created
and made visible this frame **is drawn this frame**. But hit-testing
uses `activePortals`, which is the *previous* frame's `framePortals`.
So the portal becomes clickable next frame.

**`removeFromParent` is deferred.**
`removeFromParent()` sets `wantsRemoval = true`. The actual removal
from `children` happens in `cullRemovedChildren`, at the end of the
same node's update. Until then, the node is still in the tree and
participates in measure/arrange/draw.

**`setEnabled(false)` clears focus and capture immediately.**
Unlike `removeFromParent`, disabling doesn't defer. It clears
`UIContext::focusedNode` and `UIContext::pointerCapture` if they point
to this node, and calls `onEnabledChanged(false)`.

**Multiple Tab presses in the same frame.**
Only the first is processed. The loop in `updateTree` breaks after
handling the first `Key::Tab`.

**`update` recurses before firing callbacks.**
Children are updated first, then the node's own callbacks fire. So a
`Button` inside a `VStack` will have its `onClick` fire **before** the
`VStack`'s `onClick` would fire (if it weren't consumed). This is the
"bubble up" order.

**`measure` may re-resolve style.**
If a node has `pendingTransition`, `measure` calls
`style_.resolveFor(*this)` again to get the target size. This is in
addition to the `update` call that already resolved it. The cost is
acceptable because `pendingTransition` is a one-frame flag, but it means
`resolveFor` can run twice on a single frame for a node.

**Animation overlays are not visible to `getStyle()`.**
`getStyle()` returns `style_.currentStyle`, which does **not** include
CSS animation values. The overlay is applied in `draw` on a local
`renderStyle`. So if you read `getStyle().opacity` during a CSS animation,
you get the transition value, not the animated one.

---

## 9. Ordering summary (one-page cheat sheet)

For a single node, in one frame:

```
update(dt)
├── snapshot wasPending, styleBefore
├── initStyleIfNeeded (first frame only)
├── scroll.resetIfOverflowChanged
├── viewportGeneration check
├── tickImperative → blockSubtree
├── input.updateFlags → isHovered, isPressed, isFocused
├── computeNextState
├── beginStateTransition OR resolvePendingTransition
├── style.tick (transition)
├── anim.tickCss (keyframes)
├── style.propagateInheritance
├── onAnimationsFinished (if just finished)
├── for each child: child.update(...)
├── scroll.tickInput
├── input.handleKeyInput
├── input.fireCallbacks → onHoverEnter/Exit, onPress, onRelease, onClick, onRightClick
├── cullRemovedChildren
├── recomputeDirty
└── onUpdate(dt) [if enabled or updateWhenDisabled]

measure(parentW, parentH)
├── early-out if clean and parent size unchanged
├── initStyleIfNeeded (safety)
├── calcStyle = pendingTransition ? resolveFor : currentStyle
├── compute innerW, innerH (with scroll main-axis unbounded)
├── computeIntrinsicSize → contentSize
├── for each child: child.measure(innerW, innerH)
├── accumulate totalMainSize, maxCrossSize, gap
├── scroll.contentSize = contentSize
├── finalW, finalH, clamped by min/max
└── measuredSize = {finalW, finalH}

arrange(rect)
├── positionChanged = space.xy != rect.xy
├── rect = space
├── if scroll container:
│     ├── if needsFullArrange: arrangeInto(computeArrangeSpace)
│     ├── applyOffsetDelta → translateSubtree
│     └── updateMaxScroll
│   else:
│     └── arrangeInto(space)
└── onLayout()

draw(parentOpacity)
├── if portal: register in framePortals; return
├── frustum cull
├── renderStyle = currentStyle + css animation overlay
├── resolveEffectIfNeeded
├── globalOp = renderStyle.opacity * parentOpacity; early-out if ~0
├── drawLayer OR drawInline
├── scroll.drawScrollbar
└── if root: drain framePortals
```

---

## 10. What to override and what not to

When you write a custom widget, the rules are:

- **Override**: `onBuild`, `computeIntrinsicSize`, `renderContent`,
  `renderChrome` (rarely), `onUpdate`, `onLayout`, `onEnabledChanged`,
  `onDescendantFocused`.
- **Do not override**: `update`, `measure`, `arrange`, `draw`,
  `updateTree`, `runFrame`, `renderFrame`, `hitTest`. These are the
  framework's entry points and are called by the runtime.

The only exception is `arrange`. Floating widgets (Popup, Tooltip,
Dropdown's list) override it to compute their own position:

```cpp
void arrange(Rect) override { /* no-op */ }
```

If you override `arrange`, you're responsible for calling `arrangeInto`
yourself, with the rect you want. The base implementation calls
`arrangeInto(space)` for you.

---

## 11. Debugging the lifecycle

The `ZENITUI_DEBUG` macro enables `Debug::dump*` helpers:

```cpp
#define ZENITUI_DEBUG
#include "Debug.hpp"

Debug::dumpTree(*root);
Debug::dumpStyle(*someNode);
Debug::dumpTheme();
Debug::dumpParts(*slider);
```

`dumpTree` prints the tree with rects and state flags:

```
- Stack#root  [800x600 @ 0,0]
  - Button.btn-primary  [120x40 @ 340,280] {H}
    - Text.btn-text  [80x20 @ 360,290]
```

The state flags legend:

| Flag | Meaning |
|------|---------|
| `H` | Hovered |
| `P` | Pressed |
| `F` | Focused |
| `C` | Checked |
| `D` | Disabled |

`dumpStyle` prints the matching rules for a node, with their specificity:

```
=== Node ===
  tag      : Button
  classes  : .btn-primary
  state    : H
  rect     : (340, 280  120x40)
  style    : size=auto x auto bg=#0079F1FF ...
  matches  : 3 rule(s)
spec=0.1.0] .btn-primary
      -> { background=#0079F1FF, radius=8px, ... }
spec=0.1.0] .btn-primary:hover
      -> { background=#3296FFFF, scale=1.15 }
```

For runtime logging, `Logger` routes to a `ConsoleLogger` by default,
which prints to stderr:

```
[WARN ] [StyleParser] assets/game.zstyle(45:10): unknown property 'foo'. Ignored.
```

To install a custom logger:

```cpp
Logger::set(&myLogger);
```

---

## 12. Where to go next

- **[Style System](03-style-system.md)** — how `resolveFor`, `tick`,
  and `propagateInheritance` work in detail.
- **[Animation System](04-animation-system.md)** — imperative, CSS,
  `::part`, and the overlay mechanism.
- **[Layout Engine](05-layout-engine.md)** — the measure/arrange
  algorithms in depth.
- **[Render Pipeline](06-render-pipeline.md)** — inline vs layer,
  filters, transforms, clipping.
- **[Input System](07-input-system.md)** — the flag/state model,
  callbacks, focus.
- **[Scroll System](08-scroll-system.md)** — `ScrollState`, arrange
  expansion, thumb geometry.
- **[Backend](09-backend.md)** — the interfaces you implement to port
  ZenitUI.
- **[Debugging](10-debug.md)** — dumps, logging, the test fixture.