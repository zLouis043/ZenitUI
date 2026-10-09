# Layout

ZenitUI has three container types and a Flexbox-inspired property set.
If you know CSS Flexbox, most concepts will feel familiar — but the names
are simplified and the defaults differ in a few places.

---

## 1. Container types

Every `Layout` has a `LayoutType` set at construction:

| Type                | Direction       | Purpose                                            |
|---------------------|-----------------|----------------------------------------------------|
| `LayoutType::Stack` | none (overlay)  | Children are placed on top of each other, aligned by `items-h` / `items-v`. |
| `LayoutType::Vertical` | top → bottom | Children flow vertically. |
| `LayoutType::Horizontal` | left → right | Children flow horizontally. |

```cpp
auto col = std::make_shared<Layout>(LayoutType::Vertical);
auto row = std::make_shared<Layout>(LayoutType::Horizontal);
auto stk = std::make_shared<Layout>(LayoutType::Stack);
```

```js
VStack { ... }
HStack { ... }
Stack  { ... }
```

`VStack()` / `HStack()` are helpers that also set the style tag to
`"VStack"` / `"HStack"`, so `.zstyle` rules like `VStack { ... }` apply.

### Which one should I use?

- **Stack** — anything that is "a box with stuff inside, positioned freely"
  (Panels, Buttons, Cards, Toggles). Children are typically positioned
  with `align-h` / `align-v` on themselves, or with `position: absolute`.
- **Vertical** — settings lists, sidebars, scrollable content, forms.
- **Horizontal** — toolbars, button rows, label-value rows.

---

## 2. Units

Every size-like property accepts a `Value`, which is a **sum of terms**.
Each term has a coefficient and a unit. This is what makes `calc()` work.

### Supported units

| Unit | Meaning | Example |
|------|---------|---------|
| `px` | Logical pixels | `Px(12)` |
| `%`  | Percentage of the parent, along the relevant axis (width for horizontal props, height for vertical props) | `Percent(50)` |
| `vw` | 1% of the viewport width | `VW(10)` |
| `vh` | 1% of the viewport height | `VH(10)` |
| `pw` | Percentage of the **parent's width**, regardless of the axis | `PW(50)` |
| `ph` | Percentage of the **parent's height**, regardless of the axis | `PH(50)` |
| `auto` | Let the layout engine decide (see §3) | `Auto()` |
| number | A raw number, no unit. Useful inside `calc()` and `grow` / `shrink` | `Num(2)` |

`%` vs `pw` / `ph`: `%` is **axis-aware** — `width: 50%` resolves against
the parent's width, `height: 50%` resolves against the parent's height.
`pw` is **always** the parent's width, and `ph` is **always** the parent's
height — even if you use them in the "wrong" axis. They're useful for
things like a knob whose horizontal inset should scale with the parent's
width regardless of which property you're setting:

```css
Toggle:checked .toggle-knob {
    left: calc(100pw - 80ph);   /* right-aligned knob */
}
```

### `calc()`

`calc()` supports `+`, `-`, `*`, `/`, parentheses, and mixed units:

```css
width: calc(100% - 20px);
height: calc(50vh + 2 * 12px);
left: calc((100% - 40px) / 2);
```

The parser builds a `Value` with multiple terms; resolution happens lazily
against the parent's dimensions when the node is measured.

```cpp
auto v = parseValueToken("calc(100% - 20px)").value();
// v.terms = [ {100, Percent}, {-20, Pixel} ]
```

### Inline constructors

```cpp
btn->size(Px(120), Px(40));
btn->size(Percent(100), Auto());
btn->size(VW(30), VH(6));
```

```js
Button "OK" width="120px" height="40px"
Button "OK" width="100%" height="auto"
```

---

## 3. Sizing

Every node has `width`, `height`, `min-width`, `max-width`, `min-height`,
`max-height`. All accept any `Value`, including `auto`.

### `auto` behavior

- On the **root**: `auto` resolves to `0` unless the parent supplies a
  bounded space, in which case it takes the available space (only for the
  main axis in `Vertical`/`Horizontal`).
- Inside a container: `auto` means "use my intrinsic size". For most
  nodes that's `0×0` unless children or `computeIntrinsicSize` say
  otherwise. For `Text` it's the measured text box; for widgets like
  `Checkbox` and `ProgressBar` there's a default intrinsic size.

### Constraints

`min-*` and `max-*` clamp the final size after all other calculations:

```cpp
card->getInlineBase().minWidth  = Px(80);
card->getInlineBase().maxWidth  = Percent(60);
```

```css
.card {
    min-width: 80px;
    max-width: 60vw;
}
```

If `max-width` ends up smaller than `min-width`, `max-width` wins (matches
CSS behavior: the clamp function returns `hi` when `hi < lo`).

---

## 4. Padding, margin, gap

### `padding`

Inside the node, before children are placed.

```cpp
card->getInlineBase().padding = Spacing(VH(3), VW(2));
```

```css
.card { padding: 3vh 2vw; }        /* vertical | horizontal */
.card { padding: 8px; }            /* all sides */
.card { padding: 1px 2px 3px 4px; }/* top right bottom left */
```

### `margin`

Outside the node, pushing it away from its siblings.

```css
.card { margin: 0 0 8px 0; }
```

Margins are respected in `Vertical` / `Horizontal` flow and in absolute
positioning. In `Stack`, margins still apply to the node's own `rect`
but children are not pushed around each other.

### `gap`

Space between siblings in `Vertical` / `Horizontal`. Ignored by `Stack`.

```cpp
row->getInlineBase().gap = VW(0.5f);
```

```css
.row { gap: 8px; }
```

---

## 5. Flex: `grow`, `shrink`

When a `Vertical` / `Horizontal` container has leftover (or missing)
space along its main axis, children with `grow` / `shrink` absorb it.

| Property | Default | Meaning |
|----------|---------|---------|
| `grow`   | `0`     | If there's leftover space, distribute proportionally to `grow`. |
| `shrink` | `1`     | If space is negative, shrink proportionally to `shrink`. |

```cpp
sidebar->getInlineBase().shrink = 0;   // never shrink
content->getInlineBase().grow   = 1;   // take all leftover space
```

```css
.spacer-grow { grow: 1; }
.demo-sidebar { shrink: 0; }
```

Distribution algorithm (per container, main axis only):

1. Sum the fixed sizes (`margin + measured size + margin`) of all
   in-flow children, plus `gap × (N-1)`.
2. `freeSpace = containerInnerSize - totalFixedSize`.
3. If `freeSpace > 0` and `totalGrow > 0`:
   each child gets `freeSpace × (childGrow / totalGrow)` added to its size.
4. If `freeSpace < 0` and `totalShrink > 0`:
   each child gets `freeSpace × (childShrink / totalShrink)` added
   (i.e. removed) from its size.
5. Final sizes are clamped by `min-*` / `max-*`.

Common patterns:

```cpp
// Two-column layout: left fixed, right fills
auto row = HStack({ sidebar, content });
sidebar->getInlineBase().shrink = 0;
content->getInlineBase().grow   = 1;
```

```cpp
// Three columns with 1 : 2 : 1 ratio
a->getInlineBase().grow = 1;
b->getInlineBase().grow = 2;
c->getInlineBase().grow = 1;
```

---

## 6. Alignment

There are two axes and two levels of alignment:

- **`justify`** — main axis of the container (horizontal in `Horizontal`,
  vertical in `Vertical`). Applies to the whole group of children.
- **`items-h` / `items-v`** — cross axis of the container. Default for
  children that don't override.
- **`align-h` / `align-v`** — set on the **child**, overrides the
  container's `items-*` for that specific child.

### `justify` (main axis)

Only has an effect when `freeSpace > 0` and `totalGrow == 0`.

| Value             | Behavior |
|-------------------|----------|
| `Justify::Start`  | Children packed at the start (default). |
| `Justify::Center` | Children centered as a group. |
| `Justify::End`    | Children packed at the end. |
| `Justify::SpaceBetween` | First child at start, last at end, space distributed evenly between. |

```cpp
row->getInlineBase().justify = Justify::SpaceBetween;
```

```css
.row { justify: space-between; }
```

### `items-h` / `items-v` (cross axis)

Set on the container, applies to all children:

| Value             | Behavior |
|-------------------|----------|
| `Align::Start`    | Child at the start of the cross axis (default). |
| `Align::Center`   | Child centered on the cross axis. |
| `Align::End`      | Child at the end of the cross axis. |
| `Align::Stretch`  | Child is stretched to fill the cross axis. |
| `Align::Auto`     | Child picks its own (usually means Start). |

```cpp
root->getInlineBase().itemsH = Align::Center;
root->getInlineBase().itemsV = Align::Center;
```

```css
#root {
    items-h: center;
    items-v: center;
}
```

### `align-h` / `align-v` (per-child override)

Set on the child itself:

```cpp
auto label = Label("hi");
label->getInlineBase().alignH = Align::End;   // overrides parent's itemsH
```

```css
.setting-label { align-h: end; }
```

### Quick reference

| Container   | `justify` axis | `items-h` axis | `items-v` axis |
|-------------|----------------|----------------|----------------|
| `Horizontal`| horizontal     | vertical       | *(unused)*     |
| `Vertical`  | vertical       | *(unused)*     | horizontal     |
| `Stack`     | *(ignored)*    | horizontal     | vertical       |

> Note: in `Horizontal`, `items-v` is the cross-axis alignment; in
> `Vertical`, `items-h` is the cross-axis alignment. `items-h` / `items-v`
> are still both read in `Stack` because there is no main axis — the
> container behaves like a Stack of one child per position.

### Centering anything

```cpp
// Center a widget in its parent
parent->getInlineBase().itemsH = Align::Center;
parent->getInlineBase().itemsV = Align::Center;
```

```css
.parent {
    items-h: center;
    items-v: center;
}
```

This works for both `Stack` (positions the single child) and
`Vertical` / `Horizontal` (positions all children along the cross axis).

---

## 7. Absolute positioning

A node with `position: absolute` is removed from the flow and positioned
relative to the nearest positioned ancestor's **inner box** (i.e. inside
its padding).

```cpp
badge->getInlineBase().position = Position::Absolute;
badge->getInlineBase().top      = Px(8);
badge->getInlineBase().right    = Px(8);
```

```css
.badge {
    position: absolute;
    top: 8px;
    right: 8px;
}
```

### Anchoring rules

| Set on the node         | Effect |
|-------------------------|--------|
| `left` only             | Positioned from the parent's left edge. |
| `right` only            | Positioned from the parent's right edge. |
| `left` + `right` + `width: auto` | The node stretches horizontally between `left` and `right`. |
| `top` only              | Positioned from the parent's top edge. |
| `bottom` only           | Positioned from the parent's bottom edge. |
| `top` + `bottom` + `height: auto` | The node stretches vertically. |

Any combination works. If neither `left` nor `right` is set, the node sits
at the start of the parent's inner box (`margin-left` still applies).

### Centering with absolute positioning

To center a node whose size is known:

```css
.center {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 100px;
    translate-x: -50%;
    translate-y: -50%;
}
```

`translate-x: -50%` is resolved against **the node's own width** (`resolveSelfH`),
not the parent's. Same for `translate-y`.

> **Note:** `position: relative` in ZenitUI currently behaves like
> `static` for layout purposes (the node stays in flow). It only affects
> stacking (see §10). If you need to visually offset a node, use
> `translate-x` / `translate-y`.

---

## 8. Overflow

`overflow-x` and `overflow-y` control both **clipping** and **scrolling**.

| Value               | Clip | Scrollable | Notes |
|---------------------|------|------------|-------|
| `Overflow::Visible` | no   | no         | Default. |
| `Overflow::Hidden`  | yes  | no         | Children are clipped. |
| `Overflow::Scroll`  | yes  | yes        | Scrollbar always visible. |
| `Overflow::Auto`    | yes  | yes        | Scrollbar only when needed. |

```cpp
panel->getInlineBase().overflowY = Overflow::Scroll;
panel->getInlineBase().overflowX = Overflow::Hidden;
```

```css
.panel {
    overflow-y: auto;
    overflow-x: hidden;
}
```

### Computed value rule

If one axis is `Visible` and the other is not, the `Visible` axis is
**promoted to `Auto`**. This mirrors the CSS computed-value rule and
prevents the "visible in one direction, clip in the other" ambiguity that
no backend can represent cleanly. The promotion happens in
`ComputedStyle::from`.

```css
.thing { overflow-x: scroll; overflow-y: visible; }
/* computed: overflow-x = scroll, overflow-y = auto */
```

### Scrollable containers

When a node has `overflow: scroll | auto`, the layout engine:

1. Measures children **without the main-axis constraint**, so they can
   be taller (or wider) than the container.
2. Arranges children in a virtual space that is `max(containerSize, contentSize)`.
3. Tracks scroll offset in a `ScrollController` (see
   [Scroll System](../internals/08-scroll-system.md)).
4. Draws a scrollbar thumb along the relevant axis, and clips the subtree.

Scrolling behavior:

- Mouse wheel — vertical by default, horizontal when `Shift` is held.
- Dragging the scrollbar thumb.
- Clicking on the scrollbar track jumps to that position.
- Inertia after a wheel event (exponential decay).

See [Events](06-events.md) for how scroll interacts with hover and press
states (the "hover freeze" behavior: hover is not recomputed while a
scroll is in progress).

---

## 9. Transform

Every node can be visually transformed without affecting layout:

| Property | Type | Meaning |
|----------|------|---------|
| `translate-x` | `Value` | Horizontal offset, resolved against the node's **own width**. |
| `translate-y` | `Value` | Vertical offset, resolved against the node's **own height**. |
| `scale` | `float` | Uniform scale factor around the node's center. |
| `rotation` | `float` | Degrees, clockwise, around the node's center. |

```cpp
card->getInlineBase().scale      = 1.15f;
card->getInlineBase().rotation   = 3.0f;
card->getInlineBase().translateY = Px(-4);
```

```css
.card {
    scale: 1.05;
    translate-y: -4px;
}
```

Pivot is always the node's **center**. Transform is applied during
rendering and hit-testing, not during measure / arrange — a translated
node keeps its original `rect` for layout purposes.

> **Note:** `scale` also interacts with the "layer" render path used by
> filters — the render target is sized to the transformed bounding box.
> See [Render Pipeline](../internals/06-render-pipeline.md).

---

## 10. `z-index` and stacking contexts

By default, siblings are drawn in declaration order (last = on top).
`z-index` overrides this.

```cpp
panel->getInlineBase().position = Position::Relative;
panel->getInlineBase().zIndex   = ZIndex(10);
```

```css
.panel {
    position: relative;
    z-index: 10;
}
```

### Stacking context rules

A node becomes a **stacking context** if:

- `position != static`, **and**
- `z-index != auto`.

Inside a stacking context, children are drawn in three groups:

1. Children with negative `z-index` (in ascending order).
2. Children without a stacking context (declaration order).
3. Children with positive `z-index` (in ascending order).

Hit-testing uses the same order, **reversed** — topmost first, so
clicking on an overlapping child reaches the visually topmost one.

A node with `z-index` set but `position: static` **is not** a stacking
context and participates in the parent's normal flow.

---

## 11. Backgrounds and `box-shadow` (layout-adjacent)

These don't change layout, but they're usually set alongside layout
properties, so here's a quick reference:

```css
.card {
    background: #2A2A32;               /* solid color, RGBA or hex */
    background-texture: bubble;        /* a texture registered by name */
    background-texture: npatches 16 16 16 16;  /* nine-slice with insets */
    radius: 8px;                       /* rounded corners */
    border-width: 1px;
    border-color: #3A3A40;
    box-shadow: 4px 4px 8px #00000080; /* x y blur color */
}
```

```cpp
card->getInlineBase().background = Color{42, 42, 50, 255};
card->getInlineBase().radius     = Px(8);
card->setBackgroundTexture(assets.getTexture("bubble"));
```

Draw order inside a node:

1. `box-shadow` (behind everything).
2. `background` color.
3. `background-texture` (stretched or nine-sliced).
4. `border`.
5. `renderContent` (widget-specific: slider track, checkbox mark, text…).
6. Children (in z-order).

For the full list of paintable properties, see [ZStyle](05-zstyle.md).

---

## 12. Common patterns

### Full-viewport root

```cpp
auto root = std::make_shared<Layout>(LayoutType::Stack);
root->size(Percent(100), Percent(100));
```

```css
#root { width: 100%; height: 100%; }
```

### Sidebar + content (two-column)

```cpp
auto row = HStack({ sidebar, content });
sidebar->size(VW(20), Percent(100));
sidebar->getInlineBase().shrink = 0;
content->getInlineBase().grow   = 1;
content->getInlineBase().height = Percent(100);
```

```css
.demo-panel  { display: horizontal; gap: 2vw; }
.demo-sidebar{ width: 20vw; height: 100%; shrink: 0; }
.demo-content{ grow: 1; height: 100%; }
```

### Button row

```cpp
auto row = HStack({ Btn("A"), Btn("B"), Btn("C") });
row->getInlineBase().gap = Px(4);
row->getInlineBase().itemsV = Align::Center;
```

```css
.button-row { gap: 4px; items-v: center; }
```

### Card with fixed padding

```cpp
auto card = VStack();
card->getInlineBase().padding = Spacing(VH(3), VW(2));
card->getInlineBase().gap     = VH(2);
card->getInlineBase().radius  = Px(8);
```

```css
.card {
    padding: 3vh 2vw;
    gap: 2vh;
    radius: 8px;
}
```

### Centered modal

```cpp
overlay->getInlineBase().itemsH = Align::Center;
overlay->getInlineBase().itemsV = Align::Center;
overlay->size(Percent(100), Percent(100));
```

```css
.modal-overlay {
    width: 100%; height: 100%;
    items-h: center;
    items-v: center;
}
```

### Spacer that eats leftover space

```cpp
sidebar->addChild(std::make_shared<Layout>()->cls("spacer-grow"));
```

```css
.spacer-grow { grow: 1; }
```

---

## 13. Pitfalls

**`width: 100%` on a child of an auto-sized parent.**
`%` resolves against the parent's width, which is being computed from its
children — a circular dependency. ZenitUI breaks it by resolving the
parent's `auto` size first (ignoring `%` children for that pass), then
arranging. The result is usually "the child takes zero width". Give the
parent an explicit width, or use `grow: 1`.

**Forgetting to `measure` before `arrange`.**
`arrange` reads `measuredSize` from the previous `measure`. If you call
`arrange` without a `measure`, sizes will be stale. `runFrame` does both
in order; you only need to care if you're writing a custom loop.

**`gap` on a `Stack`.**
Ignored. `Stack` has no main axis. Use `padding` on the container or
`margin` on children.

**`justify` has no effect when `grow` is set.**
`justify` only applies to *leftover* space. If a child has `grow > 0`,
it will consume all the space, leaving nothing to justify. Remove `grow`
if you want `justify: center` or `space-between` to work.

**`items-*` vs `align-*`.**
`items-h` / `items-v` go on the **container** and are the default for all
children. `align-h` / `align-v` go on the **child** and override the
container's default. If you set them in both places, the child wins.

**Absolute node with `left: 0; right: 0; width: 100px`.**
`width` wins — the node is 100px wide, positioned 0 from the left.
`right: 0` is ignored because `width` is not `auto`. Remove `width` if
you want the node to stretch.

**`overflow: scroll` without a fixed size.**
If the container has `height: auto`, its height grows to fit the content,
so `maxScroll` is always zero and the scrollbar never appears. Give the
container a bounded height (`%`, `vh`, or a parent with bounded height).