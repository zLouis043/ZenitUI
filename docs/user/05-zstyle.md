# ZStyle

`.zstyle` is ZenitUI's stylesheet language. It looks like CSS, and it
borrows most of its syntax — selectors, cascade, `@keyframes`,
`@media`, custom properties, `calc()` — but it's not CSS. The property
set is different, and a few semantic rules have been simplified or
changed to fit a UI framework that has no DOM.

This document is the reference for the language. For internals (how the
cascade is resolved, how specificity is computed, how transitions are
ticked), see [Style System](../internals/03-style-system.md).

---

## 1. Loading stylesheets

There are two entry points:

```cpp
#include "StyleParser.hpp"
using namespace ZenitUI;

// From a file
ZMarkup::loadStyleFile("assets/game.zstyle");

// From a string
ZMarkup::loadStyleString(R"(
    Button {
        background: #0079F1;
        radius: 8px;
    }
)");
```

Both append rules to the global `Theme`. Rules accumulate — calling
`loadStyleFile` multiple times stacks the stylesheets in load order.
For the same specificity, later rules win.

To start over:

```cpp
Theme::get().clear();
```

That clears the rules, the keyframes, and the `:root` block.

### Registration order

Fonts, textures, and shaders referenced by name in `.zstyle` must be
registered with `IAssetProvider` **before** the first frame. The parser
doesn't resolve them; `Layout::resolveFont` / `resolveBgTexture` do, at
style-resolution time.

```cpp
assets.loadFont("calibri", "assets/calibri.ttf");
assets.loadTexture("bubble", "assets/bubble.png");
assets.loadEffect("blur", "assets/blur.fs");

ZMarkup::loadStyleFile("assets/game.zstyle");
```

---

## 2. Syntax overview

```css
/* A rule: selector { declarations } */
Button {
    background: #0079F1;
    radius: 8px;
}

/* State pseudo-classes */
Button:hover   { background: #3296FF; }
Button:pressed { background: #505050; }
Button:focus   { border-color: #FDF900; border-width: 2px; }
Toggle:checked { background: #00E430; }
Button:disabled{ opacity: 0.4; }

/* Descendant */
.card Button { margin: 4px; }
.card .card-title { color: #FF0000; }

/* ::part */
Slider::track { background: #1717cf; }
Slider::knob  { color: #d6ff32; }

/* Custom properties */
:root {
    --primary: #0079F1;
    --radius: 8px;
}
Button { background: var(--primary); }

/* Keyframes */
@keyframes pulse {
    0%   { scale: 1.00; }
    50%  { scale: 1.18; }
    100% { scale: 1.00; }
}
.btn-pulse { animation: pulse 1.2s ease-in-out infinite; }

/* Media queries */
@media (max-width: 700px) {
    .card-desc { color: #FF0000; }
}
```

Comments: `/* ... */` (block) and `// ...` (line). Line comments inside
strings are preserved — the stripper knows about quotes.

---

## 3. Selectors

### 3.1 Tag

Matches a node whose **style tag** equals the name. The style tag is
set automatically by widgets (`Button`, `Toggle`, `Text`, …) and can be
overridden with `Layout::setStyleTag()`.

```css
Button { ... }
Toggle { ... }
Slider { ... }
Text   { ... }
```

The special tags `VStack`, `HStack`, `Stack`, `Panel`, `Spacer`,
`ScrollView`, `Dropdown`, `Popup`, `Tooltip`, `Modal`, `TextInput`,
`Checkbox`, `ProgressBar`, `ImageContainer`, `Canvas` are set by the
corresponding helpers.

### 3.2 Class

Matches a node whose `getStyleClasses()` contains the name. Classes are
added via `Layout::cls("name")` or `.name` in ZMarkup.

```css
.card { ... }
.btn-primary { ... }
.setting-row { ... }
```

### 3.3 Id

Matches a node whose `nodeId` equals the name. Ids are set via
`Layout::id("name")` or `#name` in ZMarkup.

```css
#root { width: 100%; height: 100%; }
#save { background: green; }
```

### 3.4 Compound

A single token can combine a tag, any number of classes, and an id. All
components must match the **same node**.

```css
Button.btn-primary { ... }       /* Button with class btn-primary */
Button#save { ... }              /* Button with id save */
Panel.card.elevated { ... }      /* Panel with two classes */
Button.btn-primary#save { ... }  /* all three */
```

States can be attached to the whole compound:

```css
Button.btn-primary:hover { ... }
Toggle.primary:checked { ... }
```

The order between `.class` and `#id` doesn't matter, and the tag is
optional (as with `.btn-primary` alone). The one rule is: **no spaces**
inside a compound. A space starts a new descendant selector token.

```css
.card Button.btn-primary   /* two tokens: .card, then Button.btn-primary */
.cardButton.btn-primary    /* one token: tag ".cardButton" + class */
```

Note that the parser splits on the first `.` or `#` in each token; every
subsequent `.name` or `#name` is a component of the same compound. This
means a class name cannot contain a literal `.` or `#` — matching CSS.

### 3.5 Descendant

A space-separated chain matches a node that has an ancestor matching
each earlier token. Unlike CSS, **only descendant** (not child, not
sibling) combinators are supported.

```css
.card Text { ... }               /* any Text inside a .card */
.demo-content .card .card-title { ... }
```

The chain is matched right-to-left. `.card Text` matches a `Text` whose
**nearest** matching ancestor for the `.card` part exists somewhere up
the chain — intervening nodes are fine.

### 3.6 States

States are pseudo-classes appended to a compound selector, using a single
`:`. Multiple states can be chained:

```css
Button:hover { ... }
Button:pressed { ... }
Button:focus { ... }
Toggle:checked { ... }
Button:disabled { ... }

Toggle:checked:hover { ... }
Button:focus:pressed { ... }
```

Supported states:

| State | Matches when |
|-------|--------------|
| `:hover` | `Layout::isHoveredState()` is true. |
| `:pressed` | `Layout::isPressedState()` is true. |
| `:focus` | `Layout::isFocusedState()` is true. |
| `:checked` | `Layout::isCheckedState()` is true. |
| `:disabled` | `Layout::isDisabledState()` is true (i.e. `!isEnabled`). |

Unknown states are ignored silently.

The state applies to the compound it's attached to. `Toggle:checked
.toggle-knob` matches a `.toggle-knob` **descendant** of a checked
`Toggle`. `Toggle:checked.toggle-knob` (no space) would match a
`Toggle` **that also has** the `.toggle-knob` class, which is not what
you want.

### 3.7 `::part`

Double colon introduces a **part** name. Parts are pseudo-elements
painted by a widget's `renderContent` — a slider track, a checkbox
mark, a text-input cursor. They are styled by dedicated rules, not by
the normal cascade.

```css
Slider::track { background: #1717cf; }
Slider::fill  { color: #d6ff32; }
Slider::knob  { color: #d6ff32; }

Checkbox::box  { background: #1574b0; }
Checkbox::mark { color: #ea0404; }

ProgressBar::track { background: #cc1c9d; }
ProgressBar::fill  { color: #69FF32; }

TextInput::cursor { color: #FF6600; border-width: 3px; }
```

A `::part` selector cannot have descendants (`Slider::track Text` is
invalid). It can have states on the compound before `::`:

```css
Slider:hover::knob { animation: knobGlow 0.6s ease-in-out infinite; }
Checkbox:disabled::mark { color: #808080; }
```

The state is checked against the **widget**, not against the part. So
`Checkbox:disabled::mark` matches while the Checkbox is disabled.

### 3.8 Multiple selectors

A comma splits a rule into several independent rules, each with its own
specificity:

```css
Button, Toggle, Checkbox {
    border-width: 2px;
}
```

### 3.9 Specificity

Specificity is computed like CSS but with only three buckets:

| Component | Weight |
|-----------|--------|
| Ids       | 1 (per id) |
| Classes + states | 1 (per class or pseudo-class) |
| Tags      | 1 (per tag) |

Compared lexicographically: `(ids, classes, tags)`. So one id beats any
number of classes, one class beats any number of tags.

```css
Button             /* (0, 0, 1) */
.btn-primary       /* (0, 1, 0) */
Button.btn-primary /* (0, 1, 1) */
#save              /* (1, 0, 0) */
Button:hover       /* (0, 1, 1) */
Toggle:checked     /* (0, 1, 1) */
.card Button       /* (0, 1, 1) */
```

Ties are broken by **declaration order** — later rules win. Loading two
stylesheets stacks them: everything from the second file is "later"
than everything from the first.

**Inline styles win regardless of specificity.** `getInlineBase()` and
ZMarkup attributes are applied after the cascade, so `Button
background="red"` beats any `.zstyle` rule for the same property.

---

## 4. Property reference

Every property listed here can be set in three places:

1. `.zstyle` rules (subject to cascade).
2. Inline style, via `getInlineBase()`.
3. Inline style, via ZMarkup attribute.

The parser understands every property as a *string* and dispatches it
to a typed setter. Unknown properties are logged as warnings and
ignored — they never break the parse.

### 4.1 Sizing

| Property | Type | Default |
|----------|------|---------|
| `width` | Value | `auto` |
| `height` | Value | `auto` |
| `min-width` | Value | `auto` |
| `min-height` | Value | `auto` |
| `max-width` | Value | `auto` |
| `max-height` | Value | `auto` |

```css
.card {
    width: 100%;
    height: auto;
    min-width: 80px;
    max-width: 60vw;
}
```

### 4.2 Spacing

| Property | Type | Default |
|----------|------|---------|
| `padding` | Spacing | `0` |
| `margin` | Spacing | `0` |
| `gap` | Value | `0` |

Spacing accepts 1, 2, 3, or 4 values (CSS-style):

```css
.pad { padding: 8px; }                    /* all sides */
.pad { padding: 8px 16px; }               /* vertical | horizontal */
.pad { padding: 1px 2px 3px; }            /* top | horizontal | bottom */
.pad { padding: 1px 2px 3px 4px; }        /* top | right | bottom | left */
```

Each value can be any supported unit, including `vw` / `vh` / `pw` /
`ph`.

### 4.3 Flex

| Property | Type | Default |
|----------|------|---------|
| `grow` | float | `0` |
| `shrink` | float | `1` |

```css
.spacer-grow { grow: 1; }
.demo-sidebar { shrink: 0; }
```

### 4.4 Alignment

| Property | Type | Default |
|----------|------|---------|
| `justify` | Justify | `start` |
| `items-h` | Align | `start` |
| `items-v` | Align | `start` |
| `align-h` | Align | `auto` |
| `align-v` | Align | `auto` |

`Justify`: `start`, `center`, `end`, `space-between`.
`Align`: `auto`, `start`, `center`, `end`, `stretch`.

```css
.row { justify: space-between; }
.root { items-h: center; items-v: center; }
.card { align-h: stretch; }
```

### 4.5 Position

| Property | Type | Default |
|----------|------|---------|
| `position` | `static` \| `relative` \| `absolute` | `static` |
| `top` | Value | `auto` |
| `left` | Value | `auto` |
| `right` | Value | `auto` |
| `bottom` | Value | `auto` |
| `z-index` | number \| `auto` | `auto` |

```css
.badge {
    position: absolute;
    top: 8px;
    right: 8px;
    z-index: 10;
}
```

`relative` currently behaves like `static` for layout — it only affects
stacking (a node with `position != static` and `z-index != auto` becomes
a stacking context).

### 4.6 Transform

| Property | Type | Default |
|----------|------|---------|
| `translate-x` | Value | `0` |
| `translate-y` | Value | `0` |
| `scale` | float | `1` |
| `rotation` | float (degrees) | `0` |

`translate-x` / `translate-y` resolve against **the node's own width /
height** (self-relative). Pivot is always the node's center.

```css
.hover-box {
    transition: scale 0.15s ease-out-back;
}
.hover-box:hover {
    scale: 1.05;
    translate-y: -2px;
}
```

### 4.7 Visual

| Property | Type | Default |
|----------|------|---------|
| `background` | Color | `transparent` |
| `background-texture` | TextureRef | — |
| `tint` | Color | `#FFFFFF` |
| `border-color` | Color | `transparent` |
| `border-width` | Value | `0` |
| `radius` | Value | `0` |
| `opacity` | float | `1` |
| `box-shadow` | BoxShadow | — |

```css
.card {
    background: #2A2A32FF;
    background-texture: bubble;
    background-texture: npatches 16 16 16 16;   /* nine-slice */
    border-color: #3A3A40FF;
    border-width: 1px;
    radius: 8px;
    opacity: 1.0;
    box-shadow: 4px 4px 8px #00000080;
    tint: #FFFFFFFF;
}
```

`box-shadow` syntax: `x y blur color`. Color is optional (defaults to
`#00000080`). Blur is approximated with 5 stacked rounded rects, not a
true Gaussian blur.

### 4.8 Typography

| Property | Type | Default |
|----------|------|---------|
| `font` | string | `""` (backend default) |
| `font-size` | Value | `20` |
| `letter-spacing` | Value | `2` |
| `color` | Color | `#FFFFFF` |
| `text-align` | Align | `auto` |

```css
Text {
    font: calibri;
    font-size: 3vh;
    letter-spacing: 2px;
    color: #FFFFFF;
    text-align: center;
}
```

`font` is resolved at style time via `IAssetProvider::getFont(name)`. If
the name isn't registered, the backend default font is used.

The properties `font`, `font-size`, `color`, `letter-spacing`, and
`text-align` are **inherited** by descendants. All other properties are
not.

### 4.9 Overflow

| Property | Type | Default |
|----------|------|---------|
| `overflow` | Overflow | `visible` |
| `overflow-x` | Overflow | `visible` |
| `overflow-y` | Overflow | `visible` |

`Overflow`: `visible`, `hidden`, `scroll`, `auto`.

```css
.panel {
    overflow: hidden;
}
.scrollable {
    overflow-x: hidden;
    overflow-y: auto;
}
```

**Computed value rule:** if one axis is `visible` and the other isn't,
the `visible` axis is promoted to `auto`. This matches CSS's
computed-value rule so that clipping behaves predictably. This happens
in `ComputedStyle::from`, so `getStyle()` reports the promoted value.

### 4.10 Effects

| Property | Type | Default |
|----------|------|---------|
| `filter` | comma-separated filter list | — |
| `effect` | string (shader name) | `""` |

```css
.card {
    filter: blur(4px);
    filter: blur(2px), drop-shadow(2px, 2px, #000000);
    filter: sepia(0.8);
}

.btn-primary {
    effect: hueShift;
}
```

- `filter` puts the node in a **layer**. Its subtree is rendered into a
  render target and then the filter chain is applied. This has a cost
  (one target per filtered node, ping-pong for separable filters). Use
  it sparingly.
- `effect` applies an arbitrary shader as a single pass. `effect` and
  `filter` can coexist.

For the built-in filters and how to register custom ones, see
[Effects](09-effects.md).

### 4.11 Transitions

`transition` is a comma-separated list of specs:

```css
.element {
    transition: background 0.25s ease-out, scale 0.12s ease-out-back;
}
```

Each spec is: `<property> <duration> <easing>`, where:

- `<property>` is a property name, or `all`.
- `<duration>` is `0.25s` or `250ms`.
- `<easing>` is one of the easing names (see §6).

Longhand forms override the shorthand for the properties they mention:

```css
.element {
    transition: background 0.25s ease-out, scale 0.12s;
    transition-delay: 0.05s;         /* applies to all specs */
    transition-duration: 0.3s;       /* overrides the duration of all specs */
    transition-timing-function: linear;  /* overrides the easing of all specs */
}
```

The longhand forms require a preceding `transition` — using
`transition-delay` alone logs a warning and is ignored.

### 4.12 Animations

`animation` is a comma-separated list of specs:

```css
.pulse {
    animation: pulse 1.2s ease-in-out infinite;
}
.blink {
    animation: blink 0.5s linear forwards;
}
.combo {
    animation: fadeIn 0.3s ease-out, glow 1s infinite alternate;
}
```

Each spec is: `<keyframe-name> <duration> <easing> <iterations> <direction> <fill-mode>`, where:

- `<duration>` is `0.25s` or `250ms`.
- `<iterations>` is a number or `infinite`.
- `<direction>` is `alternate` (or absent).
- `<fill-mode>` is `forwards` (or absent).

Longhand overrides:

```css
.element {
    animation: pulse 1.2s;
    animation-delay: 0.2s;
    animation-duration: 0.5s;
    animation-timing-function: ease-out;
    animation-iteration-count: 3;
    animation-direction: alternate;
    animation-fill-mode: forwards;
}
```

The `animation-name` is resolved against `Theme::get().keyframes` at
tick time. Unknown names are silently treated as "no animation".

For the semantics (fill-forwards, alternate, how CSS animations
interact with imperative animations), see
[Animations](07-animations.md) and
[Animation System](../internals/04-animation-system.md).

---

## 5. Custom properties

Custom properties start with `--`. They're stored as **raw strings**,
resolved at cascade time, and can hold any value — including whole
`calc()` expressions, color tokens, or references to other custom
properties.

```css
:root {
    --primary:    #0079F1;
    --primary-hi: #3296FF;
    --danger:     #BE2137;
    --pad:        1.5vh 2vw;
    --radius:     8px;
}
```

### Using a custom property

`var(--name)`:

```css
.btn-primary {
    background: var(--primary);
    radius: var(--radius);
    padding: var(--pad);
}
```

`var()` works **anywhere** a value is expected, and it can appear inside
`calc()` or be part of a larger expression:

```css
.box {
    width: calc(100% - var(--inset) * 2);
}
```

### Scoping and inheritance

Custom properties are **inherited** by descendants, like in CSS. The
resolution happens in `StyleResolver::resolveFor`:

1. Start with the parent's custom props (or `theme.root.customProps` if
   the node has no parent).
2. Merge the node's own custom props (winning on conflicts).
3. Resolve any pending `var()` references using the merged set.

This means a custom property defined on `:root` is available everywhere,
and a rule that redefines `--primary` on `.card` only affects `.card`
and its descendants.

### Chained references

A custom property can reference another custom property:

```css
:root {
    --a: var(--b);
    --b: #00FF00;
}
.x { color: var(--a); }    /* resolves to #00FF00 */
```

The resolver iterates up to 10 times to expand nested references. If a
name doesn't resolve, the `var()` is left as-is and the property setter
fails (usually logging a warning and leaving the property unset).

### Important: custom props don't type-check

`--radius: 8px` and `--radius: 8foo` are both valid strings. The value
is only validated when it's substituted into a concrete property. If
`--radius` is `"banana"`, `radius: var(--radius)` fails to parse and
leaves `radius` unset.

---

## 6. Easing functions

The `TransitionFunction` enum supports a fixed set of curves:

| Name | CSS equivalent |
|------|----------------|
| `linear` | `linear` |
| `ease-in-quad` | `cubic-bezier(0.55, 0.085, 0.68, 0.53)` (approx) |
| `ease-out-quad` | `cubic-bezier(0.25, 0.46, 0.45, 0.94)` (approx) |
| `ease-in-out-quad` | symmetric in/out quadratic |
| `ease-in-cubic` | `t³` |
| `ease-out-cubic` | `1 - (1-t)³` |
| `ease-in-out-cubic` | symmetric in/out cubic |
| `ease-in-back` | overshoots at start |
| `ease-out-back` | overshoots at end (bounce back) |
| `ease-out-elastic` | elastic oscillation |
| `ease-out-bounce` | bounce at end |

Short aliases are also accepted:

| Alias | Maps to |
|-------|---------|
| `ease` | `ease-in-out-quad` |
| `ease-in` | `ease-in-quad` |
| `ease-out` | `ease-out-quad` |
| `ease-in-out` | `ease-in-out-quad` |

Parsing is lenient about formatting: `EaseOutBack`, `ease-out-back`,
`EASE_OUT_BACK`, `easeoutback` all parse to `EaseOutBack`.

Unknown names cause `parseEasing` to return false; the caller keeps its
default (`Linear` for transitions, `Linear` for animations).

---

## 7. `@keyframes`

```css
@keyframes pulse {
    0%   { scale: 1.00; }
    50%  { scale: 1.18; }
    100% { scale: 1.00; }
}

@keyframes fade {
    from { opacity: 0; }
    to   { opacity: 1; }
}
```

Frame timestamps can be:

- `0%` … `100%`.
- `from` (equivalent to `0%`).
- `to` (equivalent to `100%`).

Frames are **sorted by time** at parse time. The engine does **not**
require the first frame to be `0%` or the last to be `100%`.

### Auto-fill of missing properties

If a property appears in some frames but not others, the missing ones
are auto-filled from the **nearest neighbor** (previous frame wins, then
next frame). A warning is logged for each auto-filled keyframe:

```
@keyframes pulse {
    0%   { scale: 1.00; }
    50%  { scale: 1.18; opacity: 0.5; }
    100% { scale: 1.00; }
}
```

Here `opacity` is missing in frames `0%` and `100%`. It gets auto-filled
from the nearest frame that has it (the `50%` frame), so all three
frames end up with `opacity: 0.5`.

The warning is there to nudge you to be explicit — auto-fill is
convenient but can hide bugs.

### Property whitelist

Every property is allowed in a keyframe. All the properties from §4
work, including `scale`, `rotation`, `translate-x/y`, `opacity`, colors,
and `radius`.

Because keyframes are stored as a `Style` (same struct as CSS rules),
any property that can appear in a normal rule can also be animated.

---

## 8. `@media`

```css
@media (max-width: 700px) {
    .card-desc { color: #FF0000; }
}

@media (min-width: 900px) {
    .card-desc { color: #00FF00; }
}

@media (min-width: 600px) and (orientation: landscape) {
    .demo-panel { padding: 2vh 2vw; }
}
```

### Supported conditions

| Condition | Values |
|-----------|--------|
| `min-width` | pixels |
| `max-width` | pixels |
| `min-height` | pixels |
| `max-height` | pixels |
| `orientation` | `landscape`, `portrait` |
| `min-aspect-ratio` | `16/9` or `1.77` |
| `max-aspect-ratio` | `16/9` or `1.77` |

Conditions can be combined with `and`. There is no `or`, no `not`, no
comma-separated list.

### Evaluation context

Media conditions are evaluated against `Metrics::viewport` — the
**logical** viewport size reported by `IPlatform::viewportSize()`, not
the physical window size. On HiDPI or with a DPI scale other than 1.0,
these two differ.

When `Metrics::viewport` changes between frames, `UIContext` bumps its
`viewportGeneration` counter. Each node compares it against its last
seen value and forces a style re-resolution if it changed — so media
queries react to window resizes without any extra code.

---

## 9. Inline attributes (ZMarkup)

Any attribute in ZMarkup that isn't one of the reserved ones
(`passthrough`, `value`, `checked`, `options`, `wrap`) is passed to
`applyStyleAttr(getInlineBase(), key, value)`. This is the same function
the `.zstyle` parser uses, so the following keys are accepted:

```
width          height         min-width      min-height
max-width      max-height     gap            grow
shrink         padding        margin         background
color          tint           border-color   border-width
radius         opacity        scale          rotation
translate-x    translate-y    top            left
right          bottom         font-size      letter-spacing
items-h        items-v        align-h        align-v
justify        text-align     position       z-index
font           background-texture            effect
```

Values follow the same grammar as in `.zstyle`, including `calc()` and
`var()` (though `var()` inside an inline attribute is unusual — you
already have the value in C++, so just use it).

```html
Button "OK" width="120px" height="40px" background="#0079F1" radius="8px"
Panel position="absolute" top="8px" right="8px" z-index="10"
Slider value=0.5 width="55%"
```

Inline attributes have **higher priority than any `.zstyle` rule**,
including state rules. If you set `background` inline, a `.zstyle` rule
`Button:hover { background: ... }` will have no visible effect on that
button. Prefer classes for anything you want to vary by state.

---

## 10. Full example

A real stylesheet from the demo, abbreviated:

```css
:root {
    --primary:    #0079F1;
    --primary-hi: #3296FF;
    --danger:     #BE2137;
    --danger-hi:  #E62937;
    font: calibri;
}

Toggle {
    width: 10vh;
    height: 5vh;
    radius: 50ph;
    background: #505050;
    transition: background 0.15s linear, opacity 0.2s ease-out;
}
Toggle:hover    { background: #606060; }
Toggle:focus    { border-color: #FDF900; border-width: 2px; }
Toggle:disabled { background: #3A3A40; opacity: 0.5; }
Toggle:checked  { background: #00E430; }

Toggle .toggle-knob {
    position: absolute;
    width:  60ph;
    height: 60ph;
    top:    20ph;
    left:   20ph;
    radius: 50%;
    background: #FFFFFF;
    transition: left 0.28s ease-out-cubic;
}
Toggle:checked .toggle-knob {
    left: calc(100pw - 80ph);
}

Checkbox::box  { background: #1574b0; radius: 6px; }
Checkbox::mark { color: #ea0404; }

@keyframes knobGlow {
    0%   { color: #d6ff32; }
    50%  { color: #ffffff; }
    100% { color: #d6ff32; }
}

Slider::track { background: #1717cf; radius: 4px; }
Slider::knob  { color: #d6ff32; }
Slider:hover::knob {
    animation: knobGlow 0.6s ease-in-out infinite;
}

.btn-primary {
    background: var(--primary);
    radius: 8px;
    padding: 1.5vh 2vw;
    min-width: 80px;
    margin: 0 4px;
    effect: hueShift;
    transition: background 0.25s ease-out-quad, scale 0.12s ease-out-back;
}
.btn-primary:hover   { background: var(--primary-hi); scale: 1.15; }
.btn-primary:pressed { background: #505050; scale: 0.90; }
.btn-primary:focus   { border-color: #FDF900; border-width: 2px; }
.btn-primary:disabled{ background: #000000; color: #808080FF; opacity: 0.4; }

@keyframes pulse {
    0%   { scale: 1.00; }
    50%  { scale: 1.18; }
    100% { scale: 1.00; }
}
.btn-pulse { animation: pulse 1.2s ease-in-out infinite; }

@media (max-width: 700px) {
    .card-desc { color: #FF0000; }
}
@media (min-width: 900px) {
    .card-desc { color: #00FF00; }
}
```

---

## 11. Pitfalls

**Inline style vs `.zstyle` state.**
```css
/* Never applies: inline background beats this rule */
Button:hover { background: red; }
```
```cpp
auto b = Btn("x");
b->getInlineBase().background = Colors::Blue;   // wins
```
Use classes for anything state-dependent.

**`%` in `translate-x`.**
`translate-x: 50%` resolves against the node's own width, not the
parent's. Same for `translate-y`.

**`overflow: visible` + `overflow-x: hidden`.**
The computed value promotes `overflow-y` to `auto`, and the node becomes
a scroll container. If you only want horizontal clipping with no scroll
on the vertical axis, set both explicitly (`overflow-y: hidden`).

**Custom properties don't cascade through states.**
`--x` defined in `Button:hover` doesn't retroactively apply to
non-hovered descendants in the same frame. Custom props are resolved
per node per frame in the cascade order.

**`var()` in a value that isn't typed.**
```css
--x: not-a-color;
.color { background: var(--x); }
```
`background` fails to parse and stays unset. There's no error — the
warning goes through `Logger`, which the default `ConsoleLogger` prints
to `stderr`.

**Selector order matters within the same specificity.**
```css
Button { background: blue; }
Button { background: red; }
```
The second rule wins (same specificity, later order). If you intended
one to be a fallback, you need higher specificity on the second rule.

**Media queries don't re-evaluate when the viewport changes size
mid-frame.** They're checked during style resolution, which happens
inside `updateTree`. If the viewport changes between `beginFrame` and
`updateTree` (which it does — the platform is polled at the start of
the frame), the new value is used for that same frame. This is usually
what you want, but it's worth knowing.

**`::part` cannot be inherited.**
`Slider::track { color: ... }` doesn't affect anything inside the track;
there's no DOM. The part is painted with the resolved style and nothing
else.

**Animation and transition on the same property.**
If both a CSS animation and a transition target the same property, the
animation overlays the transition result. The overlay happens in
`Layout::draw` via `Anim::overlayCssComputed`, *not* in the current
style. So `getStyle()` reports the transition value; only the render
path sees the animated value. This is a known limitation of the current
implementation.