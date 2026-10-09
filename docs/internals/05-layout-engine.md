# Layout Engine

The layout engine lives in `Measure.cpp` and consists of four phases
that run every frame, in this order:

1. **Measure** — bottom-up intrinsic size computation.
2. **Arrange** — top-down positioning of children into their allocated space.
3. **Hit-test** — z-ordered point query, used during `updateTree`.
4. **Scroll adjust** — expansion of arrange space and offset
   translation for scroll containers.

This document covers all four. It assumes you've read
[Lifecycle](02-lifecycle.md), which describes when each phase runs.

---

## 1. The data model

Every `Layout` node has a small set of fields that drive layout:

```cpp
Rect rect{0, 0, 0, 0};              // final position and size (set by arrange)
Vec2 measuredSize{0, 0};            // intrinsic size (set by measure)
bool subtreeDirty_{true};           // "needs re-measure"
float lastMeasureW_{-1.0f};         // cached parent width
float lastMeasureH_{-1.0f};         // cached parent height
LayoutType type;                    // Stack / Vertical / Horizontal
```

`rect` and `measuredSize` are different things:

- `measuredSize` is the node's desired size, computed bottom-up from
  its own content and children.
- `rect` is the node's actual allocated box, computed top-down from
  the parent's available space and the node's own measured size.

They're usually equal, but they diverge when the parent stretches the
node (`align: stretch`), constrains it (`max-width`), or shrinks it
(`shrink`). `rect` is the ground truth for drawing and hit-testing;
`measuredSize` is just an input to the arrange phase.

### 1.1 `ScrollController::contentSize`

`Measure.cpp` stores the uncut content size on the node's
`ScrollController`:

```cpp
scroll_.contentSize = contentSize;
```

This is the total extent of the node's children (before being clipped
by the node's own rect), used later by `arrange` to expand the arrange
space and by `drawScrollbar` to compute the thumb size.

---

## 2. `measure(parent_w, parent_h)`

Called bottom-up: the parent passes its inner available size, and the
node computes its intrinsic size from that.

### 2.1 Cache check

```cpp
if (!subtreeDirty_ &&
    parent_w == lastMeasureW_ &&
    parent_h == lastMeasureH_)
{
    return measuredSize;
}
lastMeasureW_ = parent_w;
lastMeasureH_ = parent_h;
```

Two conditions:

- The subtree is clean (nothing changed since last measure).
- The parent-provided space is the same as last time.

If both are true, return the cached `measuredSize` without recursing.
This is the primary performance optimization: a static UI measures once
and then returns instantly for every subsequent frame.

`subtreeDirty_` is set by `recomputeDirty` at the end of `update`, and
by `markInheritanceDirty` when a parent's inherited properties change.
When `subtreeDirty_` is true, the cache is invalidated.

### 2.2 Style resolution

```cpp
if (!styleInitialized) {
    styleInitialized = true;
    style_.currentStyle = style_.targetStyle =
        style_.transitionStartStyle = style_.resolveFor(*this);
    pendingTransition = false;
}

ComputedStyle calcStyle = pendingTransition
    ? style_.resolveFor(*this)
    : style_.currentStyle;
```

Two things:

- **First measure ever**: initialize the style. This is a safety net
  in case `update` didn't run first (which happens in some tests).
- **Pending transition**: if a property was changed via
  `getInlineBase()` or a state transition, re-resolve to get the
  target. Otherwise use `currentStyle`.

The choice of `currentStyle` vs re-resolved target matters for sizing:
during a state transition on `width`, `currentStyle` has the interpolated
value, but the re-resolved target has the final value. Measuring with
the target avoids layout jitter during transitions — the layout uses
the destination size, not the intermediate one.

### 2.3 Inner size computation

```cpp
float pl = calcStyle.padding.left.resolveH(parent_w, parent_h);
float pr = calcStyle.padding.right.resolveH(parent_w, parent_h);
float pt = calcStyle.padding.top.resolveV(parent_w, parent_h);
float pb = calcStyle.padding.bottom.resolveV(parent_w, parent_h);

float availW = calcStyle.width.isAuto()
    ? parent_w
    : calcStyle.width.resolveH(parent_w, parent_h);
float availH = calcStyle.height.isAuto()
    ? parent_h
    : calcStyle.height.resolveV(parent_w, parent_h);

float innerW = std::max(0.0f, availW - pl - pr);
float innerH = std::max(0.0f, availH - pt - pb);
```

`availW` / `availH` are the node's own "available space":

- If the node has an explicit width/height, that's the available space.
- If `auto`, it's the parent's space (which may itself be constrained).

`innerW` / `innerH` are the space inside the padding box — what
children can actually use.

### 2.4 Scroll container unbounded main axis

```cpp
if (type == LayoutType::Vertical && calcStyle.overflowY != Overflow::Visible)
    innerH = 1e9f;
if (type == LayoutType::Horizontal && calcStyle.overflowX != Overflow::Visible)
    innerW = 1e9f;
```

For a `Vertical` scroll container, the height available to children
becomes effectively infinite. This lets children be taller than the
container, which is what makes scrolling meaningful. Same for
`Horizontal` on the width axis.

The cross axis is still bounded by the container's size, so children
can't be wider than a vertical scroll container (unless they overflow,
which triggers horizontal scrolling if enabled).

`Stack` containers never use this path — `Stack` has no main axis, and
its children overlap regardless of size.

### 2.5 Intrinsic size

```cpp
Vec2 contentSize = computeIntrinsicSize(innerW, innerH);
```

The default implementation returns `{0, 0}`. Widgets override this to
declare their "natural" size:

- `Text`: measures the text with the renderer, wraps if necessary.
- `Checkbox`: `font-size × 1.4` square.
- `ProgressBar`: `120 × max(16, font-size × 0.8)`.
- `Slider`: `120 × max(24, font-size × 1.2)`.
- `TextInput`: `max(120, char-width × 12) × font-size × 1.6`.

This is the size the widget would be if it had no children. Children
sizes are combined **after** this call, per §2.6.

### 2.6 Gap

```cpp
float gapX = calcStyle.gap.resolveH(innerW, innerH);
float gapY = calcStyle.gap.resolveV(innerW, innerH);
```

Gap is resolved against the **inner** size, not the parent size, so
`gap: 10%` is 10% of the inner width (or height, depending on axis).

### 2.7 Children measure

```cpp
float totalMainSize = 0.0f;
float maxCrossSize = 0.0f;
int visibleChildren = 0;

for (auto& c : children) {
    Vec2 cs = c->measure(innerW, innerH);
    if (c->getStyle().position == Position::Absolute || c->isPortal())
        continue;

    const auto& cst = c->getStyle();
    float ml = cst.margin.left.resolveH(innerW, innerH);
    float mr = cst.margin.right.resolveH(innerW, innerH);
    float mt = cst.margin.top.resolveV(innerW, innerH);
    float mb = cst.margin.bottom.resolveV(innerW, innerH);

    float childTotalW = cs.x + ml + mr;
    float childTotalH = cs.y + mt + mb;

    if (type == LayoutType::Horizontal) {
        totalMainSize += childTotalW;
        maxCrossSize = std::max(maxCrossSize, childTotalH);
    } else if (type == LayoutType::Vertical) {
        totalMainSize += childTotalH;
        maxCrossSize = std::max(maxCrossSize, childTotalW);
    } else if (type == LayoutType::Stack) {
        contentSize.x = std::max(contentSize.x, childTotalW);
        contentSize.y = std::max(contentSize.y, childTotalH);
    }
    visibleChildren++;
}
```

Every child is measured with `innerW × innerH` as the available space.
Then, for flow children (not absolute, not portal), the size with
margins is accumulated into `totalMainSize` and `maxCrossSize`.

For `Stack`, the semantics are different: instead of accumulating along
an axis, the size is the **max** of all children (in both axes). This
is because children overlap — the container needs to be big enough to
fit the largest child.

**Important detail:** `children` is iterated by reference, and inside
the loop `c->measure(...)` is called. If a child's `measure` **adds a
child** to the parent (which shouldn't happen, but is possible), the
iteration would be invalidated. In practice, no widget does this during
`measure`, and the framework doesn't protect against it. It's an
implicit contract.

### 2.8 Gap in totals

```cpp
if (type == LayoutType::Horizontal) {
    if (visibleChildren > 1)
        totalMainSize += gapX * (visibleChildren - 1);
    contentSize.x = std::max(contentSize.x, totalMainSize);
    contentSize.y = std::max(contentSize.y, maxCrossSize);
} else if (type == LayoutType::Vertical) {
    if (visibleChildren > 1)
        totalMainSize += gapY * (visibleChildren - 1);
    contentSize.x = std::max(contentSize.x, maxCrossSize);
    contentSize.y = std::max(contentSize.y, totalMainSize);
}
```

The gap is added between children only (not before the first or after
the last). `contentSize` combines the intrinsic size with the flow
size: the intrinsic size (from `computeIntrinsicSize`) and the flow
size (from children + gaps). The result is the max of the two, per
axis.

This means a `Text` with no children has `contentSize = measured text
size`, and a `VStack` with children has `contentSize = sum of children
heights + gaps`.

### 2.9 Final size

```cpp
scroll_.contentSize = contentSize;

float finalW = calcStyle.width.isAuto()
    ? (contentSize.x + pl + pr)
    : availW;
float finalH = calcStyle.height.isAuto()
    ? (contentSize.y + pt + pb)
    : availH;
```

If `width` is `auto`, the final width is the content size plus
padding. If it's explicit, the final width is the explicit value
(already resolved into `availW`).

The padding is added back for `auto` because `contentSize` was computed
against the inner (padding-excluded) space.

### 2.10 Min/max clamps

```cpp
float minW = calcStyle.minWidth.isAuto() ? 0.0f
    : calcStyle.minWidth.resolveH(parent_w, parent_h);
float maxW = calcStyle.maxWidth.isAuto() ? 1e9f
    : calcStyle.maxWidth.resolveH(parent_w, parent_h);
float minH = calcStyle.minHeight.isAuto() ? 0.0f
    : calcStyle.minHeight.resolveV(parent_w, parent_h);
float maxH = calcStyle.maxHeight.isAuto() ? 1e9f
    : calcStyle.maxHeight.resolveV(parent_w, parent_h);

measuredSize = {clamp(finalW, minW, maxW), clamp(finalH, minH, maxH)};
return measuredSize;
```

`std::clamp` is called with `minW` / `maxW` resolved against `parent_w`
(not `innerW`), because min/max constraints are resolved against the
parent's space, not the node's own available space.

Note: `std::clamp(v, lo, hi)` has undefined behavior if `hi < lo`. The
framework doesn't guard against `max-width < min-width`, so if you set
them inconsistently, you get whatever `clamp` does (usually `hi`).
The `clampSafe` helper used in `arrange` does guard against this, but
`measure` uses plain `std::clamp`.

---

## 3. `arrange(rect)`

Called top-down: the parent passes the final rect, and the node places
its children within it.

### 3.1 Set rect and detect movement

```cpp
const bool positionChanged = (space.x != rect.x || space.y != rect.y);
rect = space;
```

If the node's position changed since last arrange, scroll containers
may need a full re-arrange (their virtual content coordinates shift).
The `positionChanged` flag is passed to `needsFullArrange`.

### 3.2 Scroll vs normal

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

**Non-scroll nodes**: just `arrangeInto(space)`.

**Scroll nodes** (`overflow != visible` on either axis, and type
`Vertical` or `Horizontal`):

1. Check `needsFullArrange` — true if not initialized, or subtree dirty,
   or position changed.
2. If needed, compute an expanded arrange space (`computeArrangeSpace`)
   and arrange the children into it, then reset the applied offset.
3. Apply the scroll offset delta via `translateSubtree` — this shifts
   the subtree by the difference since the last frame.
4. Update `maxScroll`.

`Stack` scroll containers are excluded — a `Stack` has no flow, so
scrolling doesn't make sense in the same way. A `Stack` with
`overflow: scroll` behaves like a non-scroll container.

### 3.3 `ScrollController::computeArrangeSpace`

```cpp
Rect ScrollController::computeArrangeSpace(const Layout& node, Rect space) const
{
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

The arrange space is expanded on the main axis to fit the content if
the content is larger than the container. This is what makes
`translateSubtree` able to scroll the content by shifting it within
the (larger) virtual space.

The cross axis is not expanded. A vertical scroll container keeps its
width, so children are constrained to that width (unless they overflow
horizontally, which triggers horizontal scrolling if enabled).

### 3.4 `ScrollController::applyOffsetDelta`

```cpp
bool ScrollController::applyOffsetDelta(Layout& node)
{
    float dx = appliedOffset.x - state.offset.x;
    float dy = appliedOffset.y - state.offset.y;
    if (dx == 0.0f && dy == 0.0f) return false;

    node.translateSubtree(dx, dy);
    appliedOffset = state.offset;
    return true;
}
```

The subtree's children are translated by the delta between the last
applied offset and the current one. This is how scrolling works:
instead of re-arranging every frame, the children are simply
translated by the scroll delta.

`translateSubtree` recursively shifts every descendant's `rect.x` and
`rect.y` by the delta. It does **not** shift the node's own `rect` —
the container stays fixed, only its content moves.

### 3.5 `ScrollController::updateMaxScroll`

```cpp
void ScrollController::updateMaxScroll(const Layout& node, Rect space)
{
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

`maxScroll` is the amount by which the content exceeds the container,
on each axis. Zero means no scrolling needed. `clamp()` ensures the
current offset is within `[0, maxScroll]`.

Called after `applyOffsetDelta` — so if the content shrinks (e.g. an
item is removed), the offset is clamped to the new max, snapping the
scroll back if necessary.

### 3.6 `onLayout()`

```cpp
onLayout();
```

Called after all the above. Widgets can override it to react to the
final rect. `Text` uses it to re-wrap:

```cpp
void Text::onLayout() override {
    if (!wrap_) return;
    float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height);
    float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height);
    float newAvailW = std::max(0.0f, rect.width - pl - pr);
    if (newAvailW > 0.0f && newAvailW != cachedWrapW) {
        wrapText(newAvailW, /* ... */);
        cachedWrapW = newAvailW;
        // ...
    }
}
```

Note that `onLayout` doesn't change `measuredSize` — the contract is
that it can only cache results for the next frame's measure. Changing
`measuredSize` here would invalidate the layout without a re-measure.

### 3.7 `arrangeInto(space)`

```cpp
void Layout::arrangeInto(Rect space) {
    float pl = style_.currentStyle.padding.left.resolveH(space.width, space.height);
    float pr = style_.currentStyle.padding.right.resolveH(space.width, space.height);
    float pt = style_.currentStyle.padding.top.resolveV(space.width, space.height);
    float pb = style_.currentStyle.padding.bottom.resolveV(space.width, space.height);

    Rect inner = {
        space.x + pl,
        space.y + pt,
        std::max(0.0f, space.width - pl - pr),
        std::max(0.0f, space.height - pt - pb)
    };

    float gapX = style_.currentStyle.gap.resolveH(inner.width, inner.height);
    float gapY = style_.currentStyle.gap.resolveV(inner.width, inner.height);

    arrangeAbsoluteAndPortals(inner);

    if (type == LayoutType::Vertical)
        arrangeVerticalFlow(inner, gapY);
    else if (type == LayoutType::Horizontal)
        arrangeHorizontalFlow(inner, gapX);
    else if (type == LayoutType::Stack)
        arrangeStackFlow(inner);
}
```

Steps:

1. Compute the inner rect (space minus padding).
2. Compute gaps against the inner size.
3. Arrange absolute children and portals.
4. Arrange flow children by type.

Absolute and portal children are arranged **first**, so their positions
are computed independently of the flow. Then flow children are placed
in the remaining space.

---

## 4. Flow algorithms

### 4.1 `arrangeVerticalFlow(inner, gapY)`

The algorithm:

#### Pass 1: sum fixed sizes and grow/shrink

```cpp
float totalFixedH = 0.0f, totalGrow = 0.0f, totalShrink = 0.0f;
int visibleChildren = 0;

for (auto& c : children) {
    const auto& cst = c->getStyle();
    if (cst.position == Position::Absolute || c->isPortal()) continue;
    totalFixedH += cst.margin.top.resolveV(inner.width, inner.height)
                 + c->measuredSize.y
                 + cst.margin.bottom.resolveV(inner.width, inner.height);
    totalGrow += cst.grow;
    totalShrink += cst.shrink;
    visibleChildren++;
}
if (visibleChildren > 1) totalFixedH += gapY * (visibleChildren - 1);
```

Sum of `margin-top + height + margin-bottom` for all in-flow children,
plus gaps between them. Also sum `grow` and `shrink` values across
children.

Note: the sum uses `measuredSize.y`, not `rect.height` — the node's
*intrinsic* size, before any `grow` / `shrink` adjustment.

#### Pass 2: compute free space and justify

```cpp
float freeSpace = inner.height - totalFixedH;
float startY = inner.y;
float extraGap = 0.0f;

if (freeSpace > 0.0f && totalGrow <= 0.0f && visibleChildren > 0) {
    switch (style_.currentStyle.justify) {
    case Justify::Center:  startY += freeSpace * 0.5f; break;
    case Justify::End:     startY += freeSpace; break;
    case Justify::SpaceBetween:
        if (visibleChildren > 1)
            extraGap = freeSpace / (visibleChildren - 1);
        break;
    default: break;
    }
}
```

`justify` only applies when there's **positive free space** and **no
grow**. If any child grows, it absorbs the free space, so justify has
nothing to do.

`SpaceBetween` computes an extra gap that's added between children
(beyond the declared `gapY`).

#### Pass 3: place each child

```cpp
float curY = startY;

for (auto& c : children) {
    const auto& cst = c->getStyle();
    if (cst.position == Position::Absolute || c->isPortal()) continue;

    float ml = cst.margin.left.resolveH(inner.width, inner.height);
    float mr = cst.margin.right.resolveH(inner.width, inner.height);
    float mt = cst.margin.top.resolveV(inner.width, inner.height);
    float mb = cst.margin.bottom.resolveV(inner.width, inner.height);

    // Main axis: height
    float ch = c->measuredSize.y;
    if (freeSpace > 0.0f && totalGrow > 0.0f && cst.grow > 0.0f)
        ch += freeSpace * (cst.grow / totalGrow);
    else if (freeSpace < 0.0f && totalShrink > 0.0f && cst.shrink > 0.0f)
        ch += freeSpace * (cst.shrink / totalShrink);

    float minH = cst.minHeight.isAuto() ? 0.0f
        : cst.minHeight.resolveV(inner.width, inner.height);
    float maxH = cst.maxHeight.isAuto() ? 1e9f
        : cst.maxHeight.resolveV(inner.width, inner.height);
    ch = clampSafe(ch, minH, maxH);

    // Cross axis: width
    float cw = c->measuredSize.x;
    float availW = std::max(0.0f, inner.width - ml - mr);
    float minW = cst.minWidth.isAuto() ? 0.0f
        : cst.minWidth.resolveH(inner.width, inner.height);
    float maxW = cst.maxWidth.isAuto() ? 1e9f
        : cst.maxWidth.resolveH(inner.width, inner.height);
    cw = clampSafe(cw, minW, std::min(availW, maxW));

    float cx = inner.x + ml;
    Align aH = (cst.alignH == Align::Auto) ? style_.currentStyle.itemsH : cst.alignH;
    if (aH == Align::Stretch)
        cw = clampSafe(availW, minW, std::min(availW, maxW));
    else if (aH == Align::Center)
        cx += (availW - cw) * 0.5f;
    else if (aH == Align::End)
        cx += availW - cw;

    c->arrange({cx, curY + mt, cw, ch});
    curY += mt + ch + mb + gapY + extraGap;
}
```

For each child:

1. **Main axis (height)**:
   - Start with `measuredSize.y`.
   - If there's positive free space and this child grows, add its share.
   - If there's negative free space and this child shrinks, subtract
     its share.
   - Clamp to `min-height` / `max-height`.
2. **Cross axis (width)**:
   - Start with `measuredSize.x`.
   - Clamp to `min-width` / `max-width` and to the available width
     (`inner.width - margins`).
   - If `alignH == Stretch`, set to available width.
   - If `alignH == Center`, offset x by half the difference.
   - If `alignH == End`, offset x by the full difference.
3. Call `c->arrange(...)` with the final rect.
4. Advance `curY` by `margin-top + height + margin-bottom + gapY + extraGap`.

### 4.2 `arrangeHorizontalFlow(inner, gapX)`

Symmetric to vertical: swap width/height, gapX/gapY, `alignH`/`alignV`,
`minWidth`/`minHeight`, etc. Same three passes.

### 4.3 `arrangeStackFlow(inner)`

```cpp
void Layout::arrangeStackFlow(const Rect& inner) {
    for (auto& c : children) {
        const auto& cst = c->getStyle();
        if (cst.position == Position::Absolute || c->isPortal()) continue;

        float ml = cst.margin.left.resolveH(inner.width, inner.height);
        float mr = cst.margin.right.resolveH(inner.width, inner.height);
        float mt = cst.margin.top.resolveV(inner.width, inner.height);
        float mb = cst.margin.bottom.resolveV(inner.width, inner.height);

        float availW = std::max(0.0f, inner.width - ml - mr);
        float availH = std::max(0.0f, inner.height - mt - mb);

        float minW = cst.minWidth.isAuto() ? 0.0f
            : cst.minWidth.resolveH(inner.width, inner.height);
        float maxW = cst.maxWidth.isAuto() ? 1e9f
            : cst.maxWidth.resolveH(inner.width, inner.height);
        float minH = cst.minHeight.isAuto() ? 0.0f
            : cst.minHeight.resolveV(inner.width, inner.height);
        float maxH = cst.maxHeight.isAuto() ? 1e9f
            : cst.maxHeight.resolveV(inner.width, inner.height);

        float cw = clampSafe(c->measuredSize.x, minW, std::min(availW, maxW));
        float ch = clampSafe(c->measuredSize.y, minH, std::min(availH, maxH));

        float cx = inner.x + ml;
        float cy = inner.y + mt;

        Align aH = (cst.alignH == Align::Auto) ? style_.currentStyle.itemsH : cst.alignH;
        Align aV = (cst.alignV == Align::Auto) ? style_.currentStyle.itemsV : cst.alignV;

        if (aH == Align::Stretch)
            cw = clampSafe(availW, minW, std::min(availW, maxW));
        else if (aH == Align::Center)
            cx += (availW - cw) * 0.5f;
        else if (aH == Align::End)
            cx += availW - cw;

        if (aV == Align::Stretch)
            ch = clampSafe(availH, minH, std::min(availH, maxH));
        else if (aV == Align::Center)
            cy += (availH - ch) * 0.5f;
        else if (aV == Align::End)
            cy += availH - ch;

        c->arrange({cx, cy, cw, ch});
    }
}
```

Every in-flow child is positioned independently, without accumulating
along an axis. Default position is `(inner.x + margin-left,
inner.y + margin-top)`, then adjusted by `alignH` and `alignV` (or the
container's `itemsH` / `itemsV`).

Children overlap by default. To position them differently, use:

- **Margins** — push them in from the edges.
- **Absolute positioning** — remove them from the flow.
- **Alignment** — center, stretch, or push to the end.
- **Nested containers** — the usual approach: a `Stack` doesn't layout
  children flow-wise, so use `Vertical` / `Horizontal` inside for that.

### 4.4 `clampSafe`

```cpp
float Layout::clampSafe(float v, float lo, float hi) {
    if (hi < lo) return hi;
    return std::clamp(v, lo, hi);
}
```

`std::clamp` is UB if `hi < lo`. `clampSafe` returns `hi` in that case,
matching CSS's `clamp()` semantics (where the `max` wins if `max < min`).
Used throughout `arrange` where `min-*` and `max-*` are applied without
checking their relative order.

---

## 5. Absolute positioning

### 5.1 `arrangeAbsoluteAndPortals(inner)`

```cpp
void Layout::arrangeAbsoluteAndPortals(const Rect& inner) {
    for (auto& c : children) {
        const auto& cst = c->getStyle();
        if (c->isPortal()) {
            c->arrange({inner.x, inner.y,
                        c->measuredSize.x, c->measuredSize.y});
            continue;
        }
        if (cst.position == Position::Absolute)
            arrangeAbsolute(c.get(), cst, inner);
    }
}
```

Called before flow arrangement. Two cases:

- **Portal**: arranged at the inner origin with its own measured size.
  This is a default; most portals override `arrange` to position
  themselves.
- **Absolute**: positioned by `arrangeAbsolute`.

Portals are handled first, then absolutes. The order between them within
the list is the declaration order.

### 5.2 `arrangeAbsolute(c, cst, in)`

```cpp
void Layout::arrangeAbsolute(Layout* c, const ComputedStyle& cst, const Rect& in) {
    float ml = cst.margin.left.resolveH(in.width, in.height);
    float mr = cst.margin.right.resolveH(in.width, in.height);
    float mt = cst.margin.top.resolveV(in.width, in.height);
    float mb = cst.margin.bottom.resolveV(in.width, in.height);

    bool hasL = !cst.left.isAuto();
    bool hasR = !cst.right.isAuto();
    bool hasT = !cst.top.isAuto();
    bool hasB = !cst.bottom.isAuto();

    float w = c->measuredSize.x;
    float h = c->measuredSize.y;
    float x, y;

    // Horizontal
    if (hasL && hasR && cst.width.isAuto()) {
        float L = cst.left.resolveH(in.width, in.height);
        float R = cst.right.resolveH(in.width, in.height);
        w = std::max(0.0f, in.width - L - R - ml - mr);
        x = in.x + L + ml;
    } else if (hasL) {
        x = in.x + cst.left.resolveH(in.width, in.height) + ml;
    } else if (hasR) {
        x = in.x + in.width - w - cst.right.resolveH(in.width, in.height) - mr;
    } else {
        x = in.x + ml;
    }

    // Vertical
    if (hasT && hasB && cst.height.isAuto()) {
        float T = cst.top.resolveV(in.width, in.height);
        float B = cst.bottom.resolveV(in.width, in.height);
        h = std::max(0.0f, in.height - T - B - mt - mb);
        y = in.y + T + mt;
    } else if (hasT) {
        y = in.y + cst.top.resolveV(in.width, in.height) + mt;
    } else if (hasB) {
        y = in.y + in.height - h - cst.bottom.resolveV(in.width, in.height) - mb;
    } else {
        y = in.y + mt;
    }

    c->arrange({x, y, w, h});
}
```

For each axis:

| Set values | `width`/`height` | Behavior |
|------------|------------------|----------|
| `left` and `right` | `auto` | Stretch: `w = available - L - R - margins`, position at left |
| `left` only | any | Position at `left + margin-left` |
| `right` only | any | Position at `right - width - margin-right` |
| neither | any | Position at `margin-left` from the inner origin |
| `left` and `right` | explicit | `width` wins, `right` ignored |

Same for the vertical axis with `top` / `bottom` / `height`.

The margins are included in the stretch computation (subtracted) and in
the single-side positioning (added).

### 5.3 Absolute children and `measuredSize`

An absolute child's `measuredSize` is computed in `measure` — the
`arrangeAbsolute` function just positions it. This means an absolute
child with `width: auto` and no anchoring stretches to fill the
available space (minus margins), but its `measuredSize` was already
computed without that constraint.

The stretch case overrides `w` / `h` after `arrange`. The child's
`rect` becomes the stretched size, even if `measuredSize` was
different. This is fine because `rect` is the ground truth.

### 5.4 Absolute children outside the flow

Absolute children don't contribute to the parent's intrinsic size (the
`measure` loop skips them). So a `Stack` with only absolute children
has `measuredSize = {0, 0}` (plus padding).

This can be surprising if you expect the parent to grow to fit an
absolute child. It won't — that's the semantic of absolute positioning.

---

## 6. Hit-testing

### 6.1 `hitTest(p, ancestorBlocked)`

```cpp
Layout* Layout::hitTest(Vec2 p, bool ancestorBlocked) {
    if (ancestorBlocked || !isEnabled) return nullptr;

    if (!hasParent()) {
        // Root: test portals first, in reverse order
        for (auto it = UIContext::get().activePortals.rbegin();
             it != UIContext::get().activePortals.rend(); ++it)
        {
            auto sp = it->lock();
            if (!sp) continue;
            bool wasPortal = sp->isPortal();
            sp->setPortal(false);
            Layout* hit = sp->hitTest(p, false);
            sp->setPortal(wasPortal);
            if (hit) return hit;
        }
    }
    if (isPortal_) return nullptr;

    Transform2D tr = currentTransform(style_.currentStyle);
    Vec2 pChildren = applyInverseTransform(tr, p);

    std::vector<Layout*> negZ, normalFlow, posZ;
    for (auto& child : children) {
        if (child->isStackingContext()) {
            if (child->getZIndex() < 0) negZ.push_back(child.get());
            else posZ.push_back(child.get());
        } else {
            normalFlow.push_back(child.get());
        }
    }
    auto sortByZ = [](Layout* a, Layout* b) {
        return a->getZIndex() < b->getZIndex();
    };
    std::stable_sort(negZ.begin(), negZ.end(), sortByZ);
    std::stable_sort(posZ.begin(), posZ.end(), sortByZ);

    for (auto it = posZ.rbegin(); it != posZ.rend(); ++it)
        if (Layout* hit = (*it)->hitTest(pChildren, false)) return hit;
    for (auto it = normalFlow.rbegin(); it != normalFlow.rend(); ++it)
        if (Layout* hit = (*it)->hitTest(pChildren, false)) return hit;
    for (auto it = negZ.rbegin(); it != negZ.rend(); ++it)
        if (Layout* hit = (*it)->hitTest(pChildren, false)) return hit;

    if (rect.width > 0 && rect.height > 0 && rect.contains(p) &&
        (isInteractive || blocksRaycast))
    {
        return this;
    }
    return nullptr;
}
```

Steps:

1. If `ancestorBlocked` or disabled, return null.
2. If this is the root, test portals first (reverse declaration order).
   A portal can hide the entire tree from hit-testing.
3. If this is a portal being tested non-root, return null (portals are
   only tested at the root).
4. Transform the point to this node's local space.
5. Partition children by z-order:
   - **`negZ`**: children with `z-index < 0` (stacking contexts).
   - **`normalFlow`**: children without a stacking context.
   - **`posZ`**: children with `z-index > 0`.
6. Test `posZ` in **reverse order** (topmost last-declared or
   highest-z first). Then `normalFlow` reversed. Then `negZ` reversed.
7. If none hit, test this node's `rect`.

The order ensures the visually topmost child is tested first. Reversed
iteration means the last child (which is drawn last, hence on top)
is tested first.

### 6.2 Stacking context

```cpp
bool isStackingContext() const {
    return style_.currentStyle.position != Position::Static &&
           !style_.currentStyle.zIndex.isAuto;
}
```

A node is a stacking context if it has non-static position **and** an
explicit `z-index`. Nodes with `position: static` (the default) but
`z-index` set are **not** stacking contexts — they participate in the
parent's flow with default z-order.

Nodes with `position: absolute` or `relative` **and** `z-index` are
stacking contexts and are sorted by their z-index.

### 6.3 Transform

```cpp
Transform2D tr = currentTransform(style_.currentStyle);
Vec2 pChildren = applyInverseTransform(tr, p);
```

The point `p` is transformed by the inverse of this node's transform,
so children can be hit-tested in their own (pre-transform) coordinate
space. This makes hit-testing respect the visual transform: a node
with `scale: 2` is hit at twice its original size.

`applyInverseTransform`:

```cpp
inline Vec2 applyInverseTransform(const Transform2D& t, Vec2 p) {
    p.x -= t.translate.x;
    p.y -= t.translate.y;
    float dx = p.x - t.pivot.x, dy = p.y - t.pivot.y;
    float rad = -t.rotationDeg * 3.14159f / 180.0f;
    float c = std::cos(rad), s = std::sin(rad);
    float rx = dx * c - dy * s;
    float ry = dx * s + dy * c;
    rx /= t.scale;
    ry /= t.scale;
    return {rx + t.pivot.x, ry + t.pivot.y};
}
```

Inverse order: undo translate, then undo rotation (negated angle), then
undo scale (divide). Pivot is subtracted/added at the center.

Note: the node's own `rect.contains(p)` check uses the **original**
point `p`, not `pChildren`. This is because `p` is already in the
node's own coordinate space — the transform was applied to the parent's
point to get `pChildren`, and this node's rect is in this node's space.

Wait — that's subtle. Let me re-read:

```cpp
Transform2D tr = currentTransform(style_.currentStyle);
Vec2 pChildren = applyInverseTransform(tr, p);

// ... test children with pChildren ...

if (rect.width > 0 && rect.height > 0 && rect.contains(p)) {
    return this;
}
```

The point `p` is passed in **from the parent**, in the parent's local
space. The parent's `hitTest` calls `child->hitTest(pChildren, ...)` —
so `p` for the child is already in the child's local space (after the
parent applied its own inverse transform). So the child's `p` is what
the child tests its rect against.

Then `currentTransform` for the child computes the child's own
transform, `applyInverseTransform` converts `p` into the **child's
children's** space (for the recursive call).

So the design is: `hitTest(p)` receives `p` in its own coordinate space.
It uses `p` directly for its own rect check, and `pChildren` (the
inverse-transformed `p`) for recursing into children.

### 6.4 Blocking

```cpp
if (ancestorBlocked || !isEnabled) return nullptr;
```

Two cases where a node is invisible to hit-testing:

- **`ancestorBlocked`**: an ancestor had `blocksRaycast = true` and
  stopped the recursion. This is passed down explicitly.
- **`!isEnabled`**: a disabled node is not hit-testable, but its
  children **are** tested first (the early return is before the
  children loop). So a disabled container with enabled children still
  lets children be hit.

Wait, let me re-check. The early return is:

```cpp
if (ancestorBlocked || !isEnabled) return nullptr;
```

This is at the top of `hitTest`. So if the node is disabled, it returns
null **before testing children**. Children of a disabled node are not
hit-testable.

That's the intended behavior — a disabled container disables its whole
subtree.

### 6.5 `isInteractive || blocksRaycast`

```cpp
if (rect.width > 0 && rect.height > 0 && rect.contains(p) &&
    (isInteractive || blocksRaycast))
{
    return this;
}
```

A node is hit-testable if it's interactive **or** blocks raycast.
A non-interactive node with `blocksRaycast = true` stops the hit-test
at itself: nodes behind it don't get the event, but it doesn't fire
`onClick`. This is what `Panel` uses (it blocks raycast but isn't
interactive).

A non-interactive node with `blocksRaycast = false` is fully
transparent to hit-testing: events pass through.

A default `Layout` has `isInteractive = true` and `blocksRaycast = false`,
so it's hit-testable and doesn't block.

### 6.6 The root's special case

Only the root (no parent) iterates `activePortals`. This means:

- Portals are tested at the root, before anything else in the tree.
- A portal wins over any node in the normal tree, even if the node is
  drawn later.

The `activePortals` list is from the **previous** frame. So a portal
created this frame isn't hit-testable until next frame. See
[Portals](../user/08-portals.md) §8 for the discussion.

---

## 7. Dirty tracking

The dirty flag (`subtreeDirty_`) is recomputed at the end of `update`:

```cpp
void Layout::recomputeDirty(bool wasPending, const ComputedStyle& styleBefore) {
    bool anyChildDirty = false;
    for (auto& c : children) {
        if (c->subtreeDirty_) { anyChildDirty = true; break; }
    }

    bool styleChanged = (style_.currentStyle != styleBefore);
    bool inTransition = (style_.transitionTimer < 1.0f);

    subtreeDirty_ = wasPending || pendingTransition || anyChildDirty ||
                    styleChanged || inTransition;
}
```

A node is dirty if **any** of:

- `wasPending` — the node had `pendingTransition` at the start of this
  update.
- `pendingTransition` — the node still has it now (some setter ran
  during update).
- `anyChildDirty` — any direct child is dirty.
- `styleChanged` — `currentStyle` differs from the snapshot at the
  start of this update.
- `inTransition` — the node is in the middle of a CSS state transition.

`styleChanged` catches cases where `style_.tick` changed the current
style (during a transition), or where the cascade re-resolved to a
different value even though no explicit flag was set.

`inTransition` catches the case where a transition is still running
but the current style hasn't changed this frame (e.g. the timer is
still < 1.0 but the interpolation is paused because `dt = 0`).

### 7.1 How dirty propagates

Bottom-up. When `recomputeDirty` is called on the root, its children
have already been recomputed (they were visited before it in the
post-order walk). So `anyChildDirty` correctly captures the deepest
changes.

The result is that any change in a subtree marks the whole path to the
root as dirty, and the next `measure` will re-measure from the root
down to the changed node (and not beyond, thanks to the early-out in
`measure`).

### 7.2 Inheritance dirty

`markInheritanceDirty` is a separate propagation:

```cpp
void markInheritanceDirty() {
    pendingTransition = true;
    subtreeDirty_ = true;
    for (auto& c : children)
        c->markInheritanceDirty();
}
```

Called by `propagateInheritance` when a parent's inherited properties
or state flags change. It marks the **entire subtree** dirty — all
descendants, not just direct children. This is more aggressive than
`recomputeDirty`, which only marks the path.

The reason: an inherited property change affects every descendant, so
they all need to re-resolve their styles (and thus re-measure). A
change to `color` on a parent means every descendant's text color
needs to be re-resolved.

### 7.3 Measure cache

`measure` checks `subtreeDirty_` plus the parent size:

```cpp
if (!subtreeDirty_ && parent_w == lastMeasureW_ &&
    parent_h == lastMeasureH_)
{
    return measuredSize;
}
```

If the node is clean and the parent hasn't changed size, return the
cached value. Otherwise re-measure.

The `parent_w` / `parent_h` check is important: a node can be clean but
be measured with a different parent size (e.g. after a resize). In that
case, its own size may not change, but its children's sizes might (if
they use `%` or `grow`). The early-out only kicks in when both are the
same.

### 7.4 Dirtiness during the frame

A node's dirty flag is computed **at the end of its own update**. So
during the frame, a node's `subtreeDirty_` reflects the state from the
previous frame. This matters for `measure`, which runs after all
updates:

- If a setter ran during update and set `pendingTransition`, the node's
  `recomputeDirty` at the end of update will mark it dirty.
- The subsequent `measure` will see the dirty flag and re-measure.

If a setter runs **outside** of update (e.g. from user code between
frames), the flag isn't recomputed until the next update. But
`measure` on the same frame would then see the **previous** dirty flag,
which might be false — this is why the framework requires `runFrame`
to be called every frame.

There's no "force measure" API. To force a re-measure, set a property
that triggers `pendingTransition`, then call `runFrame`.

### 7.5 `viewportGeneration`

When `Metrics::viewport` changes, `viewportGeneration` is incremented.
Each node compares it in `update`:

```cpp
uint32_t gen = UIContext::get().viewportGeneration;
if (gen != lastViewportGen_) {
    lastViewportGen_ = gen;
    pendingTransition = true;
}
```

This forces a style re-resolution, which changes the target style (via
`resolveFor` and `evaluateMedia`), which then marks the node dirty via
`recomputeDirty`'s `styleChanged` check.

The purpose: media queries react to viewport changes. Without this,
a node's style would be resolved once and never re-evaluated, even
when the viewport crosses a media-query threshold.

---

## 8. Common tasks

### 8.1 Adding a new container type

1. Add the enum value to `LayoutType` in `UIEnums.hpp`.
2. Add a branch in `measure` for the new type's size accumulation.
3. Add a branch in `arrangeInto` for the new type.
4. Implement the flow function (e.g. `arrangeWrapFlow` for a
   wrapping container).

The `Layout` type is a simple enum, so this is straightforward.

### 8.2 Adding a new alignment mode

1. Add the enum value to `Align` in `CoreTypes.hpp`.
2. Add a branch in the flow functions where alignment is applied
   (`arrangeVerticalFlow`, `arrangeHorizontalFlow`, `arrangeStackFlow`).
3. Add a parser case in `parseAlignToken`.
4. Add a `toString(Align)` case for dumps.

### 8.3 Adding a new unit

1. Add the enum value to `Unit` in `Unit.hpp`.
2. Add a factory method (e.g. `Value::em(float)`).
3. Add a case in each of the four `resolve*` functions
   (`resolveH`, `resolveV`, `resolveSelfH`, `resolveSelfV`).
4. Add a parser case in `parseValueToken`.
5. Add a `unitName` case in `Debug.hpp`.

### 8.4 Adding a constraint

A constraint is a min/max on a specific property. The current set
(`min-width`, `max-width`, etc.) is hardcoded in `measure` and in the
flow functions. To add a new one:

1. Add the property to `BUBBLE_STYLE_PROPS`.
2. Add the clamp in `measure`'s final size computation.
3. Add the clamp in the flow function that applies the constraint.

For example, `min-main-size` for a flow container would require a new
clamp in `arrangeVerticalFlow` after the main-axis size is computed.

---

## 9. Pitfalls

**`measure` may call `resolveFor` twice per frame.**
If `pendingTransition` is true when `measure` runs, it calls
`style_.resolveFor(*this)` to get the target style. This is in addition
to the `resolvePendingTransition` already called during `update`. Two
full cascades per frame for a node in transition. Measurable but usually
not the bottleneck.

**`arrange` uses `style_.currentStyle`, not `calcStyle`.**
Unlike `measure`, `arrange` reads `style_.currentStyle` directly for
padding, gap, etc. This means during a transition on padding, `arrange`
uses the interpolated value, while `measure` uses the target. The
result is a mismatch: `measure` computes size with target padding,
`arrange` positions with current padding. This is intentional (measure
shouldn't jitter, arrange can), but it's a subtle asymmetry.

**`arrangeInto` doesn't clamp negative sizes.**
If the parent's size is smaller than the padding, `inner.width` becomes
`max(0, space.width - pl - pr)`. The `max(0, ...)` guards against
negative inner dimensions, but individual child sizes can still be
negative if `shrink` is aggressive. The `clampSafe` helper handles
some of this, but not all cases.

**The order of `arrangeAbsoluteAndPortals` matters.**
Absolutes are arranged **before** flow children. This means an absolute
child's `arrange` can't rely on flow children being positioned yet.
In practice, this doesn't matter because absolute children don't
interact with flow.

**`arrange` doesn't re-measure.**
If you call `arrange` without a preceding `measure`, the node uses
stale `measuredSize` values. This can happen if you manually call
`arrange` (which is discouraged), or if a widget's `onLayout` changes
something that should have affected measurement. The correct pattern
is `runFrame(dt)`, which does measure then arrange.

**Portals in `measure` are counted as children but not in the flow.**
A portal's `measuredSize` is computed (so it's not zero), but it's
excluded from `totalMainSize` and `maxCrossSize` (the loop skips it
after measuring). So a container with a portal child has its own size
unaffected by the portal — correct, since the portal positions itself
independently.

**The `visibleChildren` count includes disabled children.**
A disabled child is still measured and arranged normally. It just isn't
hit-tested. So `visibleChildren` in the flow functions is really
"non-absolute non-portal children", not "visible" in the sense of
rendered. The name is a bit misleading.

**`contentSize` is the uncut content.**
For a scroll container, `scroll_.contentSize` is the total extent of
the children, before the container's size clamps it. This is what
`maxScroll` is computed from. For a non-scroll container, the same
field is set but unused (except `measure`'s `scroll.contentSize =
contentSize` is unconditional).

**Absolute positioning ignores the parent's alignment.**
`arrangeAbsolute` doesn't consult `items-h` / `items-v` / `align-h` /
`align-v`. An absolute child is positioned only by its `left` / `right`
/ `top` / `bottom` / margins. If none are set, it's at `(inner.x +
margin-left, inner.y + margin-top)` — the inner origin.

**Stretch on the cross axis overrides the child's intrinsic size.**
An `align: stretch` child gets its cross-axis size set to the available
space, ignoring its `measuredSize`. This is correct (that's the point
of stretch), but it means a stretched `Text` with `width: auto` will
have its rect stretched to the parent's width, and the text will be
positioned within that. Text wrapping uses `rect.width`, so a stretched
`Text` with `wrap: true` wraps to the parent's width — the intended
behavior.

**`clampSafe(v, lo, hi)` returns `hi` if `hi < lo`.**
This means `min-width: 100px; max-width: 50px` results in a 50px width,
not 100px. Matches CSS's `clamp()` semantics, but is worth knowing.

**Nested scroll containers don't compose.**
A `ScrollView` inside another `ScrollView` — the inner one scrolls
within the outer one's viewport. The wheel event goes to the
`topmostConsumer` (the inner `ScrollView`, since it's on top), which
consumes it via `wheelConsumedThisFrame = true`. The outer one doesn't
see it. This is the intended behavior (matches most UI frameworks).

**`hitTest` doesn't handle `clip` boundaries.**
A node partially clipped by an ancestor's `overflow: hidden` is still
fully hit-testable. Clicking on the invisible (clipped) part still
hits the node. This is a known limitation — the `rect.contains(p)`
check doesn't account for the clip rect.

Workaround: use `setInteractive(false)` on the clipped portion, or
make the clip explicit via separate nodes. In practice, this rarely
matters because clipped content is usually off-screen or hidden behind
a visible element.

**`translateSubtree` translates `rect` of descendants, not `measuredSize`.**
This is correct — `measuredSize` is intrinsic and doesn't change with
scrolling. `rect` is the actual position, and it's what's used for
drawing and hit-testing. The translation is cumulative across frames:
`applyOffsetDelta` applies the delta between the last and current
offset, so the total translation equals the current offset.

**Rotation is ignored in hit-testing's bounding box.**
`rect.contains(p)` uses an axis-aligned rect, but the visual transform
can rotate the node. For a rotated node, the hit-test area is still
the original AABB, not the rotated bounding box. This is a
simplification; rotating nodes are rare in UI.

**`arrangeStackFlow` doesn't apply `justify`.**
`justify` only makes sense with a main axis. `Stack` has no main axis,
so `justify` is ignored. Use `align-h` / `align-v` on children, or the
container's `items-h` / `items-v`, for positioning.

**The order of the dirty checks in `recomputeDirty`.**
`anyChildDirty` is computed by iterating children **before** the node's
own flags are checked. If a child was just added this frame, it's in
`children` and its `subtreeDirty_` is `true` (from its constructor),
so the parent is marked dirty. Correct.

If a child was just removed this frame (via `cullRemovedChildren`),
it's no longer in `children`, so it doesn't affect the parent. Also
correct.