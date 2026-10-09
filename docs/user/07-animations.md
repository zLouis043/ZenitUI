# Animations

ZenitUI has two animation systems that coexist and complement each other:

- **Transitions** — implicit animations driven by state changes. You
  declare them in `.zstyle` with `transition:`, and they automatically
  animate whenever a property changes between states.
- **Animations** — explicit, time-driven sequences. There are two kinds:
  - **CSS animations** declared with `@keyframes` in `.zstyle`,
    triggered by the presence of the `animation:` property.
  - **Imperative animations** built in C++ with `UIAnimation`, triggered
    manually with `playAnimation(name)`.

Both use the same easing functions (`EaseInQuad`, `EaseOutBack`,
`EaseOutElastic`, …) and both are ticked once per frame during
`Layout::update`.

This document is the reference for the user-facing API. For the internal
architecture (how CSS animations interact with the style resolver, how
imperative tracks are applied), see
[Animation System](../internals/04-animation-system.md).

---

## 1. Transitions

A transition animates **any property** when it changes between two style
states. The most common case is state transitions (`:hover`,
`:pressed`, `:focus`, `:checked`, `:disabled`), but the same mechanism
handles any style change on the node.

### 1.1 Declaring a transition

```css
Button {
    transition: background 0.25s ease-out;
}
```

Syntax: `<property> <duration> <easing>`.

- `<property>` — a property name from [ZStyle §4](05-zstyle.md), or `all`.
- `<duration>` — `0.25s` or `250ms`.
- `<easing>` — one of the [easing names](05-zstyle.md#6-easing-functions).

Multiple properties, comma-separated:

```css
.btn-primary {
    transition: background 0.25s ease-out-quad,
                scale      0.12s ease-out-back;
}
```

Now `background` and `scale` animate independently, with different
durations and easings:

```css
.btn-primary:hover   { background: #3296FF; scale: 1.15; }
.btn-primary:pressed { background: #505050; scale: 0.90; }
```

The `.zstyle` in the demo uses exactly this pattern.

### 1.2 `all`

`transition: all 0.2s ease` animates any property that changes. This is
convenient for prototyping but usually wasteful — you don't want to
interpolate `padding`, `border-width`, and `font-size` at 60 fps unless
you actually animate them.

```css
.panel { transition: all 0.2s ease-out; }
```

### 1.3 Longhand forms

```css
.element {
    transition: background 0.25s ease-out, scale 0.12s;

    transition-delay: 0.05s;              /* applies to all specs */
    transition-duration: 0.3s;            /* overrides all durations */
    transition-timing-function: linear;   /* overrides all easings */
}
```

The longhand forms only take effect if a `transition` shorthand was
declared first on the same rule. Using `transition-duration` alone
logs a warning and is ignored. (This mirrors CSS, where the shorthand
must come first.)

### 1.4 Per-property defaults

Properties that aren't mentioned in any `transition:` spec do **not**
animate — they snap to their new value instantly.

```css
/* Only background animates. */
Button {
    background: #0079F1;
    border-color: #505060;
    transition: background 0.25s;
}
Button:hover {
    background: #3296FF;      /* animates over 0.25s */
    border-color: #FDF900;    /* snaps immediately */
}
```

### 1.5 What can be interpolated

Interpolation depends on the type of the property:

| Type | Behavior |
|------|----------|
| `float` | Linear interpolation (using easing). |
| `Color` | Per-channel linear interpolation. |
| `Value` | Term-by-term. If both sides have the same units, they combine. If not, all units are present in the result. `auto` on either side causes a **snap** (no interpolation). |
| `Spacing` | Element-wise on all four sides. |
| Enums (`Align`, `Justify`, `Overflow`, `Position`) | Snap. No interpolation. |
| `ZIndex` | Snap. |
| `string` (`font`, `effect`) | Snap. |
| `TextureRef`, `FilterRef` list, `BoxShadow` | Snap. |

When a snap happens, the property jumps to the *target* value at the
beginning of the transition (`t = 0` if the source is `auto`, otherwise
at the midpoint). In practice, if you mix `auto` and a concrete value,
the transition looks like a hard cut.

### 1.6 Transition and state

The transition fires whenever `ComputedStyle` changes between two frames
and there's a matching spec in `transitions`. The state machine
(`:hover`, `:pressed`, …) is the most common source, but any style change
that re-resolves the target style works — including inline changes:

```cpp
btn->getInlineBase().scale = 1.2f;   // triggers a transition if declared
```

This is what the demo does for `Modal::show()` and `Modal::hide()`:

```cpp
void show() {
    Style vis;
    vis.opacity = 1.0f;
    setInlineBase(vis);      // triggers a transition on opacity
}
```

### 1.7 Nesting

A transition on a parent doesn't cascade to children. Each node declares
its own transitions. The exception is `::part` — see §4.

---

## 2. CSS animations (`@keyframes`)

A keyframe animation is a predefined sequence of style deltas. When a
node has `animation: <name> <duration> ...` in its style, the animation
runs.

### 2.1 Declaring a keyframe

```css
@keyframes pulse {
    0%   { scale: 1.00; }
    50%  { scale: 1.18; }
    100% { scale: 1.00; }
}
```

Frames use percentage timestamps, or `from` / `to` as aliases for
`0%` / `100%`:

```css
@keyframes fade {
    from { opacity: 0; }
    to   { opacity: 1; }
}
```

Frames are sorted by time at parse time, so you can write them in any
order.

### 2.2 Properties allowed in a keyframe

Every property from [ZStyle §4](05-zstyle.md) is allowed. The most
common ones are `opacity`, `scale`, `rotation`, `translate-x`,
`translate-y`, colors, and `radius`.

Nested keyframes (a keyframe inside another keyframe) are not supported.

### 2.3 Auto-fill of missing properties

If a property is set in some frames but not others, the missing values
are automatically filled from the **nearest neighbor** (previous frame
first, then next). A warning is logged for each filled slot.

```css
@keyframes example {
    0%   { scale: 1.00; }
    50%  { scale: 1.18; opacity: 0.5; }   /* warning logged */
    100% { scale: 1.00; }
}
```

Here `opacity` is auto-filled in `0%` and `100%` (from the `50%` frame,
since it's the only one that has it). The warning helps catch typos —
in practice you'll almost always want to write all the values
explicitly.

### 2.4 Using an animation

```css
.pulse {
    animation: pulse 1.2s ease-in-out infinite;
}
.blink {
    animation: blink 0.5s linear forwards;
}
.combo {
    animation: fadeIn 0.3s ease-out,
               glow   1s   ease-in-out infinite alternate;
}
```

Syntax: `<keyframe-name> <duration> [<easing>] [<iterations>] [alternate] [forwards]`.

| Token | Meaning | Default |
|-------|---------|---------|
| `1.2s` / `200ms` | Duration. | `1s` |
| `ease-in-out` | Easing function. | `linear` |
| `infinite` | Repeat forever. | `1` iteration |
| `3` | Number of iterations. | |
| `alternate` | Reverse direction every other iteration. | off |
| `forwards` | Keep the last frame's values after finishing. | off |

Multiple animations, comma-separated.

### 2.5 Longhand forms

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

Same rule as transitions — the longhand forms only take effect if
`animation` was declared first on the same rule.

### 2.6 Overlaying

The animated values **overlay** the current `ComputedStyle` during
rendering. This means:

- `getStyle()` returns the transition value, not the animated value.
- The render path (`renderChrome`, `renderContent`, `drawChildren`)
  sees the animated value through `Anim::overlayCssComputed`.

Concrete implication: **hit-testing uses the transition value**, not
the animated value. A button that visually slides left during a
`slideIn` animation is still hit-tested at its un-animated position.
This is a known limitation — for most cases, it doesn't matter.

### 2.7 Triggering

A CSS animation runs as long as the `animation` property is set on the
node. To start/stop it, add/remove a class (or change the inline style):

```cpp
btn->cls("btn-pulse");       // start the pulse animation
btn->removeFromParent();     // (there's no removeClass — see below)
```

There's no `removeClass()` in the current API. To stop a CSS animation,
you'd need to rebuild the node or override the inline style:

```cpp
// Alternative: set an inline animation that overrides
btn->getInlineBase().animation = ...;   // not supported via inline
```

In practice, the pattern is:

- Use `:hover`, `:checked`, `:pressed` on a `::part` for short pulses
  (see §4).
- Use imperative animations for start/stop control (see §3).

### 2.8 Infinite animations

`infinite` iterations run until the node is destroyed or the `animation`
property is removed. The engine ticks them every frame, so they have a
small per-frame cost. Don't have dozens of simultaneously-animating
nodes — you'll see it in the frame time.

---

## 3. Imperative animations

An `UIAnimation` is a C++ object holding a list of **tracks**. Each
track animates one property from a start value to an end value, using
an easing function.

Imperative animations are triggered by name with `playAnimation(name)`.

### 3.1 Creating an animation

```cpp
auto anim = std::make_shared<UIAnimation>(0.4f);   // 0.4s duration

anim->addTrack<float>(0.0f, 1.0f,
    [](Layout* l, float v) { l->getInlineBase().opacity = v; },
    TransitionFunction::EaseOutCubic);

anim->addTrack<float>(-30.0f, 0.0f,
    [](Layout* l, float v) { l->getInlineBase().translateY = Px(v); },
    TransitionFunction::EaseOutBack);
```

`addTrack<T>(start, end, setter, easing)`:
- `T` — the value type (`float`, `Color`, `Value`, `Spacing`, …).
- `start`, `end` — the values.
- `setter` — a lambda `void(Layout*, T)` that writes the interpolated
  value somewhere.
- `easing` — the transition function (defaults to `Linear`).

The setter typically writes to `getInlineBase()` — that's the mutable
style object that won't be overwritten by the CSS cascade.

### 3.2 Attaching to a node

```cpp
modal->addAnimation("Intro", anim);
```

You can attach multiple animations to the same node, each with its own
name:

```cpp
node->addAnimation("fadeIn", fadeAnim);
node->addAnimation("slideIn", slideAnim);
```

### 3.3 Playing an animation

```cpp
node->playAnimation("fadeIn");           // forward
node->playAnimation("fadeIn", true);     // reverse
```

`playAnimation` sets the animation's `playing` flag and resets its
elapsed time if it's at the end (or at the start, when reversing).

You can play it again later — for example, from a button callback:

```cpp
auto card = VStack();
card->addAnimation("enter", anim);

auto trigger = Btn("Animate", [card]{
    card->playAnimation("enter");
});
```

### 3.4 The `blocksInput` flag

By default, `UIAnimation::blocksInput = true`. While the animation is
playing, the node and its subtree do not receive input events. This is
useful for animations that shouldn't be interrupted (modal intros,
loading sequences).

```cpp
anim->blocksInput = false;   // allow interaction during the animation
```

### 3.5 The `onFinished` callback

```cpp
anim->onFinished = []{ std::printf("done\n"); };
```

Fires once when the animation reaches its end (in forward mode) or its
start (in reverse mode). The callback is copied before being invoked,
so it's safe to modify the animation map from within it (e.g. remove
the node).

### 3.6 Reverse playback

```cpp
node->playAnimation("Intro", true);   // reverse
```

Reverse playback starts from the current elapsed time (or from the end
if the animation had already finished forward). It runs backwards until
`elapsed == 0`, then stops.

The `Modal` class uses this for `hide()`:

```cpp
void hide() {
    setInteractive(false);
    Style hid;
    hid.opacity = 0.0f;
    setInlineBase(hid);
    content->playAnimation("Intro", true);
}
```

### 3.7 Multiple tracks and order

Tracks in a single `UIAnimation` all run in parallel — they all sample
the same `elapsed` and apply to their respective targets. To sequence
animations, use the `onFinished` callback to chain them:

```cpp
anim1->onFinished = [node]{
    node->playAnimation("anim2");
};
```

### 3.8 Example: modal intro

From the `Modal` implementation:

```cpp
auto intro = std::make_shared<UIAnimation>(0.4f);
intro->addTrack<Value>(VH(100.0f), Value(0.0f),
    [](Layout* l, Value v) { l->getInlineBase().translateY = v; },
    TransitionFunction::EaseOutBack);

content->addAnimation("Intro", intro);
```

Then:

```cpp
modal->show();   // calls content->playAnimation("Intro")
modal->hide();   // calls content->playAnimation("Intro", true)
```

The modal content slides up from below the viewport (100vh down),
settling at 0 with a small overshoot thanks to `EaseOutBack`.

---

## 4. `::part` animations

Individual parts (slider tracks, checkbox marks, text-input cursors)
can have their own animations. This is how the demo makes the slider
knob "glow" when hovered.

### 4.1 Declaring

```css
Slider:hover::knob {
    animation: knobGlow 0.6s ease-in-out infinite;
}
Slider:pressed::knob {
    animation: knobPop 0.2s ease-out;
}
```

The state (`:hover` / `:pressed`) is on the widget, the animation is
on the part. The part has its own style resolver (`partTransitions`)
that ticks independently from the node's.

### 4.2 How parts animate

The part's `Style` is resolved in `StyleResolver::partFor`, which:

1. Collects all rules matching `Tag::part` (with the current node
   state).
2. Runs the part's own transition (`partTransitions` map).
3. Calls `Anim::applyCssToPart`, which overlays the current keyframe
   values onto the part's style.

Then `renderContent` reads the part's style:

```cpp
Style knobStyle = partStyle("knob");
Color knobColor = knobStyle.color.is_set
    ? knobStyle.color.value
    : defaultColor;
r->fillCircle(center, radius, knobColor.withAlpha(op));
```

Note: `partStyle("knob")` triggers a re-tick of the part's animation
every time it's called. In practice, `renderContent` calls it once per
frame, which is what you want.

### 4.3 States on parts

```css
Slider:hover::knob    { ... }
Slider:pressed::knob  { ... }
Checkbox:disabled::mark { color: #808080; }
```

The state is checked against the **widget**, not the part — a part
doesn't have its own hover state.

### 4.4 Transition on parts

Parts support their own `transition:` declarations:

```css
Slider::knob {
    color: #3296FF;
    transition: color 0.2s ease-out;
}
Slider:hover::knob {
    color: #d6ff32;    /* animates over 0.2s */
}
```

The part transition is independent from the node's transition. They use
the same `TransitionSpec` list format but are stored separately in
`StyleResolver::partTransitions`.

### 4.5 Built-in parts

| Widget | Parts |
|--------|-------|
| `Slider` | `track`, `fill`, `knob` |
| `Checkbox` | `box`, `mark` |
| `ProgressBar` | `track`, `fill` |
| `TextInput` | `cursor` |

To add a part to a custom widget, call `partStyle("name")` inside
`renderContent` and use the returned `Style`. See
[Custom Widgets](10-custom-widgets.md).

---

## 5. Animation timing and the frame loop

Animations are ticked in `Layout::update`, once per frame, **before**
the state machine runs:

```cpp
bool localAnimBlocks = anim_.tickImperative(*this, dt);   // imperative first
bool blockSubtree = ancestorBlocked || localAnimBlocks;

// ... state flags, callbacks ...

style_.tick(*this, dt);         // CSS transitions
anim_.tickCss(*this, dt);       // CSS keyframes

// ... children update ...
```

Key implications:

- **Imperative animations run before the state machine.** This means an
  animation that changes `opacity` will affect the current frame's
  rendered output, but the node's `isEnabled` / `isHovered` flags are
  computed with the animation's effect already applied.
- **CSS transitions run after the state machine.** A `:hover` state
  change resolves the new target style and starts a transition, which
  then ticks from that frame onward.
- **CSS keyframes run after transitions.** If both a transition and a
  keyframe target the same property, the keyframe overlays the
  transition result at render time.

### 5.1 `blocksInput` and subtree blocking

If an imperative animation has `blocksInput = true` (the default), its
node and its subtree are marked as "blocked" for the current frame:

```cpp
bool blockSubtree = ancestorBlocked || localAnimBlocks;
bool selfBlocked = blockSubtree || !isEnabled || (!isInteractive && blocksRaycast);
```

Blocked nodes don't update their hover/pressed flags, and their
descendants inherit the block. This is why `Modal` with `blocksInput`
prevents clicks on the underlying screen while the intro is playing.

To disable blocking:

```cpp
anim->blocksInput = false;
```

### 5.2 `onAnimationsFinished`

A node's `onAnimationsFinished` callback fires once when it transitions
from "has active animations" to "has none":

```cpp
node->onAnimationsFinished = []{ std::printf("all done\n"); };
```

"Active" means: imperative animations in play, CSS node animations,
or `::part` animations. It's tracked with a per-node
`hadLocalAnimationsLast_` flag.

This is useful for cleanup:

```cpp
auto fade = std::make_shared<UIAnimation>(0.3f);
fade->addTrack<float>(1.0f, 0.0f,
    [](Layout* l, float v) { l->getInlineBase().opacity = v; });
fade->onFinished = [node]{
    node->removeFromParent();
};
```

Or, with `onAnimationsFinished`:

```cpp
node->onAnimationsFinished = [node]{
    node->removeFromParent();
};
```

Both work. `anim->onFinished` is per-animation; `onAnimationsFinished`
is per-node and fires when **all** animations (including CSS) are done.

---

## 6. Complete examples

### 6.1 Hover scale (transition)

```css
.btn-primary {
    background: #0079F1;
    transition: background 0.25s ease-out-quad,
                scale      0.12s ease-out-back;
}
.btn-primary:hover {
    background: #3296FF;
    scale: 1.15;
}
```

Nothing else needed. Hovering the button animates both properties in
parallel, each with its own duration and easing.

### 6.2 Pulse (CSS keyframe, infinite)

```css
@keyframes pulse {
    0%   { scale: 1.00; }
    50%  { scale: 1.18; }
    100% { scale: 1.00; }
}
.btn-pulse {
    animation: pulse 1.2s ease-in-out infinite;
}
```

```cpp
auto btn = Btn("Pulsing", []{});
btn->cls("btn-primary")->cls("btn-pulse");
```

The button pulses forever until it's removed or the animation property
is removed.

### 6.3 Slide-in on build

```cpp
auto card = VStack();

auto anim = std::make_shared<UIAnimation>(0.6f);
anim->addTrack<float>(-20.0f, 0.0f,
    [](Layout* l, float v) { l->getInlineBase().translateX = VW(v * 0.01f); },
    TransitionFunction::EaseOutBack);
anim->blocksInput = false;

card->addAnimation("enter", anim);
card->playAnimation("enter");
```

The card slides in from the left over 0.6s with a slight overshoot.

The CSS equivalent:

```css
@keyframes slideIn {
    0%   { translate-x: -20vw; opacity: 0.0; }
    100% { translate-x: 0vw;   opacity: 1.0; }
}
.slide-in-box {
    animation: slideIn 0.6s ease-out-back;
}
```

The CSS version runs once and stops (no `infinite`); to replay it, you'd
need to rebuild the node or toggle the class.

### 6.4 Modal intro (imperative, with reverse)

```cpp
auto modal = Modal::create();
modal->children[0]->addChild(Label("Hello"));
modal->children[0]->addChild(Btn("Close", [modal]{ modal->hide(); }));

parent->addChild(modal);
modal->show();
```

`Modal::onBuild` sets up the intro animation:

```cpp
auto intro = std::make_shared<UIAnimation>(0.4f);
intro->addTrack<Value>(VH(100.0f), Value(0.0f),
    [](Layout* l, Value v) { l->getInlineBase().translateY = v; },
    TransitionFunction::EaseOutBack);

content->addAnimation("Intro", intro);
```

And `show()` / `hide()` trigger it forward / reverse:

```cpp
void show() {
    setInteractive(true);
    Style vis; vis.opacity = 1.0f;
    setInlineBase(vis);
    content->playAnimation("Intro", false);
}
void hide() {
    setInteractive(false);
    Style hid; hid.opacity = 0.0f;
    setInlineBase(hid);
    content->playAnimation("Intro", true);
}
```

Two animations run in parallel: the modal's `opacity` is a CSS
transition (via `setInlineBase`), and the content's `translateY` is the
imperative one. Both are driven by the same `dt`.

### 6.5 Glowing slider knob (part animation)

```css
@keyframes knobGlow {
    0%   { color: #d6ff32; }
    50%  { color: #ffffff; }
    100% { color: #d6ff32; }
}

Slider::knob {
    color: #d6ff32;
    transition: color 0.2s ease-out;
}
Slider:hover::knob {
    animation: knobGlow 0.6s ease-in-out infinite;
}
```

Hover the slider and the knob pulses between two colors. Move the mouse
away and it settles back to the base color (via the part transition).

### 6.6 Chained imperative animations

```cpp
auto node = VStack();
node->addChild(Label("Loading..."));

auto fadeIn = std::make_shared<UIAnimation>(0.3f);
fadeIn->addTrack<float>(0.0f, 1.0f,
    [](Layout* l, float v) { l->getInlineBase().opacity = v; });

auto slideUp = std::make_shared<UIAnimation>(0.4f);
slideUp->addTrack<Value>(Px(30), Px(0),
    [](Layout* l, Value v) { l->getInlineBase().translateY = v; },
    TransitionFunction::EaseOutBack);

fadeIn->onFinished = [node]{ node->playAnimation("slide"); };

node->addAnimation("fade", fadeIn);
node->addAnimation("slide", slideUp);

node->playAnimation("fade");
```

The node fades in first, then slides up.

---

## 7. Interaction with other systems

### 7.1 With `setEnabled(false)`

Disabling a node doesn't stop its animations. The node's state machine
returns `Disabled`, but imperative animations still tick (unless
`blocksInput` prevents it — that affects input, not ticking). To
explicitly pause animations on disable, override `onEnabledChanged`:

```cpp
void onEnabledChanged(bool nowEnabled) override {
    for (auto& [name, state] : anim_.imperative)
        state.playing = nowEnabled;
}
```

`TextInput` uses `onEnabledChanged` for a different purpose — clearing
its repeat state and cursor blink.

### 7.2 With `removeFromParent()`

When a node is removed from its parent, it's erased during the next
`cullRemovedChildren` pass. Any animations still running on it are
destroyed with it. If you want a fade-out before removal, chain the
removal to `onFinished` or `onAnimationsFinished`:

```cpp
auto fade = std::make_shared<UIAnimation>(0.3f);
fade->addTrack<float>(1.0f, 0.0f,
    [](Layout* l, float v) { l->getInlineBase().opacity = v; });
fade->onFinished = [node]{ node->removeFromParent(); };

node->addAnimation("fadeOut", fade);
node->playAnimation("fadeOut");
```

### 7.3 With `.zstyle` state rules

A CSS animation declared under `:hover` runs as long as the state is
active. When the state changes, the animation stops **immediately** —
there's no "finish the current iteration" behavior. This is different
from transitions, which interpolate back.

```css
.btn:hover { animation: pulse 1s infinite; }
```

Hover → animation starts. Un-hover → animation stops, node snaps to
its non-animated style.

If you need a graceful exit, use a transition instead:

```css
.btn {
    transition: scale 0.2s ease-out;
}
.btn:hover {
    scale: 1.1;
    transition: scale 0.6s ease-in-out;
}
```

### 7.4 With filters

A node with a `filter:` goes through the "layer" render path — its
subtree is rendered to a target, then filters are applied. Animations
on the node's own properties (scale, opacity) still work, but they
happen **around** the render target operation, not inside it. See
[Render Pipeline](../internals/06-render-pipeline.md) for details.

---

## 8. Performance notes

- **Transitions** are the cheapest: they only tick when the target
  style changes, and the tick itself is a set of `lerpProp` calls on
  the properties declared in `transition:`.
- **CSS animations** tick every frame as long as they're running.
  `infinite` animations never stop. Keep the count low.
- **Imperative animations** tick every frame while `playing`. Each
  track calls its setter once per frame. Tracks are cheap, but the
  setter writes to `getInlineBase()`, which triggers a
  `pendingTransition` flag — that then forces the node's style to be
  re-resolved. While an imperative animation is running, the resolver
  skips the CSS transition and snaps `currentStyle` to the target, so
  the animated value is visible on the same frame. The cost is one
  style re-resolution per animated node per frame; for dozens of
  simultaneous animations this can become noticeable, so keep the
  count moderate.
- **`::part` animations** tick every time `partStyle(name)` is called,
  which is once per frame during `renderContent`. They're essentially
  free unless you have hundreds of parts animating.
- **The "dirty" system**: a node in an active transition or animation
  keeps `subtreeDirty_ = true`, which prevents measure caching. Once
  all animations stop, the subtree can be cached again.

For 60fps budgets, the rule of thumb is: transitions and part
animations are free, CSS animations are cheap, imperative animations
are fine in the dozens, not in the thousands.

---

## 9. Pitfalls

**CSS animation on a property that also has a transition.**
The animation overlays the transition **at render time**. `getStyle()`
returns the transition value; hit-testing uses the transition value;
only the rendered output sees the animated value. Don't rely on
`getStyle()` to observe the animation.

**`animation` set inline via `getInlineBase()` doesn't work.**
The `animations` field of `Style` is `Opt<std::vector<AnimationRef>>`,
but `applyStyleAttr` doesn't have a branch for `animation` — the parser
does. To start/stop a CSS animation from C++, you need a class or a
`.zstyle` rule:

```cpp
node->cls("pulse");   // ✅ if .pulse declares animation: pulse ...;
```

There's no `removeClass` in the current API. To stop a CSS animation
from C++, either rebuild the node or use an imperative animation.

**Forgetting `blocksInput = false` on UIAnimation.**
The default is `true`. If you animate a widget that should stay
interactive (e.g. a button that scales on hover, implemented as an
imperative animation), set `blocksInput = false` or the button won't
respond to clicks during the animation.

**`onFinished` firing twice.**
If an animation is played to completion, then played in reverse to
completion, `onFinished` fires each time. If you need it to fire only
on forward completion:

```cpp
anim->onFinished = [node]{
    // check some state to decide whether to act
};
```

There's no `direction` parameter passed to the callback.

**`playAnimation` on a name that doesn't exist.**
`AnimationPlayer::play` returns early if the name isn't in the
`imperative` map. No error, no log. Typos silently do nothing.

**Reverse playback starting from the middle.**
`playAnimation(name, true)` starts from the **current** elapsed time
(or from the end if the animation had already finished forward). If
you want it to always start from the end, you'd need to reset
`elapsed` manually — but there's no public API for that, since
`imperative` is a private member of `AnimationPlayer` that's
`friend`-accessible only from `Layout`.

**Part animations don't inherit from node transitions.**
A `transition:` on the node doesn't affect parts. Each part needs its
own `transition:` declaration under `::part`.

**Interpolating `Value` with different units.**
`Px(10)` to `Percent(50)` interpolates term-by-term on the union of
units, which is often what you want, but the intermediate values can
look odd. `auto` on either side forces a snap. If you need smooth
interpolation, keep both endpoints in the same unit.

**`translate-x: -50%` in a keyframe.**
`translate-x` resolves against the node's **own width**. In a keyframe,
this is resolved at tick time using the node's current `rect`. If the
node's size changes during the animation, the translation value
changes with it. Usually fine, occasionally surprising.

**Chaining via `onFinished` requires capturing the node.**
`anim->onFinished = [node]{ node->playAnimation("next"); };` captures a
`shared_ptr` to the node, which keeps it alive as long as the animation
object is alive. If the animation is stored on the node itself, you
have a reference cycle:

```cpp
node->addAnimation("a", anim);
anim->onFinished = [node]{ /* ... */ };   // cycle!
```

Break it with a `weak_ptr`:

```cpp
std::weak_ptr<Layout> weak = node;
anim->onFinished = [weak]{
    if (auto n = weak.lock()) n->playAnimation("next");
};
```

The engine itself doesn't create these cycles — it's an issue only for
your own callbacks.