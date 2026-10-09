# Glossary

Terminology used across ZenitUI. The purpose of this document is to
pin down terms that are used in more than one place, and to
disambiguate ones that are sometimes confused.

Entries are alphabetical. Each has a short definition and, where
useful, a "not to be confused with" note.

---

## A

**Alignment (`align-h`, `align-v`)**
Cross-axis positioning set on a **child**, overriding the container's
`items-h` / `items-v`. Values: `auto`, `start`, `center`, `end`,
`stretch`.
*Not to be confused with* **justify** (main axis) or **items** (the
container-level default).

**Alpha**
The current development stage. The API is unstable and breaking changes
are expected. Contrast with **beta** and **1.0** in
[ROADMAP](ROADMAP.md).

**Anchor**
The reference node for a floating widget's position (Popup, Tooltip).
The anchor's `rect` determines where the floating widget is placed.

**API reference**
The section of the documentation under `docs/api/`, providing full
method signatures. Contrast with the **user guide** and **internals**.

**Asset**
A resource loaded by name: a font, texture, or effect (shader).
Registered with an `IAssetProvider` before the first frame. Referenced
by string name in `.zstyle` and ZMarkup.

**Auto**
A special `Value` state meaning "let the layout engine decide". Not a
length — an instruction to compute one from the node's content or
context.

---

## B

**Backend**
The concrete implementation of the three interfaces (`IRenderer`,
`IPlatform`, `IAssetProvider`). The reference backend targets Raylib.
ZenitUI itself has no drawing code.

**Base style (inline base)**
See **inline base**.

**Beta**
The next milestone after **alpha**. API frozen for the core, no known
critical bugs, test coverage on every widget. See [ROADMAP](ROADMAP.md).

**Bubble (input)**
The propagation of a click, press, or right-click event from the
innermost hit node up through its ancestors. Controlled by
`passThrough`.

---

## C

**Cascade**
The resolution of a node's final style from multiple sources, in
priority order: `inlineDefaults` < CSS rules (by specificity, then
order) < `inlineBase` < state-specific inline styles.

**Class**
A named modifier applied to a node via `addClass` / `.cls` / `.name` in
ZMarkup. Matched by `.classname` selectors in `.zstyle`.

**Computed style (`ComputedStyle`)**
The fully resolved style: every property has a concrete value. Produced
by `ComputedStyle::from(style, parent, root)`. Cached in
`Layout::style_.currentStyle`.

**Container**
A `Layout` with children. The three container types are **Stack**,
**Vertical**, and **Horizontal**.

**Cross axis**
The axis orthogonal to a container's main axis. For a `Horizontal`
container, the cross axis is vertical, and vice versa. Controls
`items-h` / `items-v`.

**Custom property**
A CSS variable (`--name`). Stored as a raw string, inherited by
descendants, and substituted at cascade time via `var()`.

---

## D

**Declared style (`Style`)**
A bag of optional properties (`Opt<T>`). The raw output of the parser.
Multiple `Style`s are composited via `overlay()` to produce a
`ComputedStyle`.

**Descendant selector**
A selector chain like `.card Button`, matching a node whose ancestor
chain contains the earlier components. Only descendant relationships
are supported — no child, sibling, or `>` combinators.

**Dirty (subtree)**
A flag on a `Layout` node indicating that its subtree needs
re-measuring. Set by state changes, style changes, pending transitions,
and inherited-property propagation.

**Disabled**
A node state (`!isEnabled`) that:
- Does not receive hit-test.
- Does not receive `onUpdate` (unless `updateWhenDisabled`).
- Is not focusable.
- Matches `:disabled` selectors.

---

## E

**Effect**
A shader applied to a node's own draw calls (via `pushEffect`). Set
with `effect: <name>` in `.zstyle`. Different from a **filter**, which
operates on the composited subtree.

**Easing function**
A curve that maps normalized time `[0, 1]` to an interpolated value.
Names are like `linear`, `ease-out-quad`, `ease-in-out-cubic`,
`ease-out-back`, `ease-out-elastic`, `ease-out-bounce`.

---

## F

**Filter**
A post-processing pass applied to a node's composited subtree. Set
with `filter: <name>(<args>)` in `.zstyle`. Requires a **layer** (the
subtree is rendered into a render target first). Registered with
`FilterRegistry`.

**Flag (state)**
A boolean on `Layout` that describes runtime state: `isHovered`,
`isPressed`, `isFocused`, `isEnabled`, `isChecked_`, `isInteractive`,
`isPortal_`, `passThrough_`, etc. Flags drive `:hover`, `:checked`,
and other state selectors.

**Focus**
The routing of keyboard events to a single node. Set by click, Tab,
or `requestFocus`. Only one node is focused at a time, globally.

**Focus scope**
A boundary for Tab navigation. A node with `setFocusScope(true)`
restricts the Tab cycle to its descendants. Used by `Modal` and
`SettingsScreen`.

**Frustum culling**
Skipping a node during `draw` if its rect is entirely outside the
current clip. The main optimization for large scrollable lists.

---

## H

**Handle**
An opaque identifier for a backend resource. `FontHandle`,
`TextureHandle`, `EffectHandle`, `TargetHandle`. Handles are small
structs with a `uint32_t id` and (for textures and targets) dimensions.
`id == 0` means invalid.

**Hit-test**
The point query that finds the topmost node at a given screen position.
Respects z-order, portal priority, and the interactive / blocking
flags.

---

## I

**Id**
A string identifier on a `Layout` node (`#name` in ZMarkup). Matched
by `#name` selectors. Used by `UINode::find` for lookups.

**Inherited property**
One of `font`, `fontSize`, `color`, `letterSpacing`, `textAlign`. All
other properties are not inherited.

**Inline base (`inlineBase`)**
The "author inline style" layer of the cascade. Wins over everything
except state-specific inline styles. Set via `getInlineBase()` or by
ZMarkup attributes.

**Inline defaults (`inlineDefaults`)**
The "user agent" layer of the cascade. Lowest priority — a CSS rule
overrides it. Used by widgets to declare reasonable defaults.

**Intrinsic size**
The size a node would have from its own content, ignoring its parent
and siblings. Computed by `computeIntrinsicSize`. Empty for most
layouts; the text box for `Text`, a square for `Checkbox`, etc.

**Items (`items-h`, `items-v`)**
Cross-axis alignment defaults set on a **container**. Children can
override with `align-h` / `align-v`.

---

## J

**Justify**
Main-axis alignment of a container's children as a group. Values:
`start`, `center`, `end`, `space-between`. Only applies when there's
positive free space and no child grows.

---

## L

**Layer**
The offscreen render target path used when a node has `filter:`.
Contrast with **inline** rendering, the direct path.

**Layout**
Two meanings:
1. The `Layout` class — the base node type of the tree.
2. The act of computing positions and sizes (measure + arrange).

Context usually disambiguates. The documentation writes **layout
phase** for (2) and **`Layout` node** for (1) when it matters.

**Layout type**
The `Stack` / `Vertical` / `Horizontal` enum. Determines how children
are arranged.

**Log level**
One of `Debug`, `Info`, `Warning`, `Error`. Used by the `Logger` for
severity filtering.

---

## M

**Main axis**
The primary axis of a container's flow. Horizontal for `Horizontal`,
vertical for `Vertical`. `Stack` has no main axis.

**Margin**
Space **outside** a node's box, pushing it away from siblings.

**Measure**
The bottom-up phase that computes each node's `measuredSize` from its
own intrinsic size and its children's sizes. Cached; short-circuits if
the subtree is clean.

**Media query**
A conditional selector (`@media (...)`) that only matches when the
condition is satisfied. Evaluated against `Metrics::viewport` at
style-resolution time.

**Modal**
A widget (not just a state) that covers the viewport with a
semi-transparent background and consumes input. Not a portal.

---

## N

**Node**
An instance of `Layout` or a subclass. Every node has a `rect`, a
`parent`, and a list of `children`.

**Not to be confused with**: **widget**, which is a subclass with
specific behavior (Button, Slider, ...). A widget is a node; a node is
not necessarily a widget.

---

## O

**Overflow**
The `visible` / `hidden` / `scroll` / `auto` enum controlling clipping
and scrolling per axis. A node with non-`visible` overflow on either
axis is a **scroll container**.

**Overlay (Style)**
The operation of copying all set properties from one `Style` onto
another. The core cascade primitive.

---

## P

**Padding**
Space **inside** a node's box, between the border and the content.

**Parent**
The node that owns a child in the tree. Held as a `weak_ptr` by the
child. The root has no parent.

**Part (`::part`)**
A pseudo-element painted by a widget's `renderContent`. Styled via
`Tag::partName { ... }` in `.zstyle`, resolved with
`partStyle(name)`. Examples: `Slider::track`, `Checkbox::mark`.

**Pass through**
A node flag that lets click, press, and right-click events bubble to
its ancestors after firing on the node.

**Pointer capture**
A state in which all pointer events are routed to a specific node,
regardless of cursor position. Used for drag interactions.

**Portal**
A node that draws at the root's z-top and is hit-tested before the
main tree, regardless of its logical position. Used for Dropdown,
Popup, Tooltip.

**Property**
An entry in `Style` / `ComputedStyle` (e.g. `width`, `background`).
Every property is defined by the `BUBBLE_STYLE_PROPS` macro.

---

## R

**Rect**
A `{x, y, width, height}` struct. Every node has a `rect`, set during
`arrange`.

**Render target**
An offscreen framebuffer (`TargetHandle`). Used for filters (`layer`)
and by `CanvasLayout`. The Raylib backend doesn't support nested
targets.

**Root**
The topmost node in the tree. Its parent is null. `runFrame` and
`renderFrame` are called on it.

---

## S

**Scroll container**
A node with non-`visible` overflow on at least one axis. Its children
can exceed its size, and can be scrolled via wheel, drag, or
`scrollTo`.

**Selector (`.zstyle`)**
The left-hand side of a rule. A chain of compound selectors joined by
descendant relationships. May include states and a `::part`.

**Specificity**
The triple `(ids, classes, tags)` used to rank rules of equal origin.
Compared lexicographically.

**Stack**
Two meanings:
1. `LayoutType::Stack` — a container with no main axis; children
   overlay.
2. The visual ordering of draw calls in `drawChildren`: negative
   z-index, then normal flow, then positive z-index.

Disambiguated by context. The documentation says **Stack container**
for (1) and **stacking context** for (2).

**State (UI state)**
The FSM value computed each frame from a node's flags: `Idle`, `Hover`,
`Pressed`, `Disabled`. Not to be confused with **flag**, which is the
underlying boolean.

**Style**
Two meanings:
1. The `Style` struct — a bag of optional properties.
2. A CSS rule's contents, as in "the style of this rule".

Disambiguated by context. The type is always capitalized (`Style`).

**Style tag**
A string on a node (e.g. `"Button"`, `"Toggle"`). Matched by
`TagName { ... }` selectors. Set with `setStyleTag`.

**Subsystem**
One of the four structs that a `Layout` delegates to:
`StyleResolver`, `AnimationPlayer`, `InputController`,
`ScrollController`. They're friends of `Layout`.

---

## T

**Target (render)**
See **render target**.

**Tooltip**
A floating label shown on hover after a delay. A `Panel` subclass with
`setPortal(true)` and `setBlockRaycast(false)`.

**Transition**
An implicit animation triggered by a style change (usually a state
change). Declared with `transition:` in `.zstyle`.

---

## U

**UI state**
See **state**.

**Unit**
An enum (`Pixel`, `Percent`, `VW`, `VH`, `PW`, `PH`, `Number`,
`Auto`) used in a `Value` term to describe how a coefficient resolves.

---

## V

**Value**
The length type used by most style properties. A sum of terms, each a
coefficient + unit. Represents `10px`, `50%`, and `calc(100% - 20px)`
with the same type.

**Viewport**
The logical screen size, in UI pixels. Stored in `Metrics::viewport`
and updated each frame from `IPlatform::viewportSize`.

**`var()`**
A reference to a **custom property**. Substituted at cascade time.

---

## W

**Widget**
A `Layout` subclass with specific behavior (Button, Slider, Toggle,
...). Every widget is a node; not every node is a widget.

**Width / height (auto)**
A `Value` state meaning "compute from content or context". Not a fixed
number.

---

## Z

**ZMarkup**
The declarative DSL for building UI trees. Produces `Layout` nodes via
a registry of factory functions. Parsed at runtime.

**ZStyle**
The CSS-like stylesheet language. Parsed into `Theme` rules.

**Z-index**
The `int` (or `auto`) property that orders siblings within a stacking
context. Only effective with `position: relative` or `absolute`.

---

## See also

- [Cheatsheet](CHEATSHEET.md) — quick reference for day-to-day work.
- [README](README.md) — project overview.
- [Roadmap](ROADMAP.md) — planned work and known issues.
