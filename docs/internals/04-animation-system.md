# Animation System

ZenitUI has three animation channels that coexist and are all ticked
from the same place (`Layout::update`):

1. **Transitions** — implicit, driven by style changes between two
   states. Handled entirely by `StyleResolver` (see
   [Style System](03-style-system.md) §4). Not covered here.
2. **Imperative animations** — C++ `UIAnimation` objects with explicit
   tracks, triggered by name via `playAnimation()`.
3. **CSS animations** — `@keyframes` declared in `.zstyle`, driven by
   the `animation:` property. There are two sub-channels:
   - **Node CSS**: applied to the node itself, ticked incrementally.
   - **Part CSS**: applied to `::part` styles, ticked absolutely.

This document covers the imperative channel and the two CSS channels,
plus the shared primitives in `Anim`.

---

## 1. The data model

All animation types live in `AnimPrimitives.hpp`, split into three
groups: imperative, keyframe definition, and active-instance.

### 1.1 Imperative: `UIAnimation` and `AnimState`

```cpp
class UIAnimation {
public:
    struct TrackBase {
        virtual ~TrackBase() = default;
        virtual void apply(float raw_t, Layout* target) = 0;
    };

    template <typename T>
    struct Track : TrackBase {
        T start_val, end_val;
        std::function<void(Layout*, T)> setter;
        TransitionFunction transition;

        void apply(float raw_t, Layout* target) override {
            float ratio = getRatio(raw_t, transition);
            setter(target, lerpProp(start_val, end_val, ratio));
        }
    };

    float duration;
    float delay{0.0f};
    bool blocksInput{true};
    std::vector<std::unique_ptr<TrackBase>> tracks;
    std::function<void()> onFinished;
};
```

`Track<T>` is type-erased via `TrackBase` so different value types can
live in the same `tracks` vector. `apply` normalizes the raw progress
(`raw_t` in `[0, 1]`) through `getRatio(raw_t, transition)` before
interpolating.

```cpp
struct AnimState {
    std::shared_ptr<UIAnimation> anim;
    float elapsed{0.0f};
    float delayElapsed{0.0f};
    bool playing{false};
    bool reverse{false};
};
```

`AnimState` is the per-animation runtime state: which animation, how
long it's been running, whether it's currently playing, and in which
direction.

`AnimationPlayer::imperative` is an `unordered_map<string, AnimState>`
keyed by the name given to `addAnimation`.

### 1.2 Keyframe definition: `Keyframe` and `KeyframeAnimation`

```cpp
struct Keyframe {
    float t{0.0f};
    Style delta;
};

struct KeyframeAnimation {
    std::string name;
    std::vector<Keyframe> keyframes;
};
```

A `Keyframe` has a normalized time (`t` in `[0, 1]`) and a `Style` with
the deltas — only the properties that are animated need to be set. The
`Style` is the same struct used everywhere else in the framework, so
every property is animatable.

`KeyframeAnimation` is the pure definition: no timing, no state. It's
stored in `Theme::keyframes` keyed by name, populated by the
`@keyframes` parser.

### 1.3 Active instance: `ActiveCssAnimation`

```cpp
struct ActiveCssAnimation {
    std::string name;
    float elapsed{0.0f};
    double startTime{0.0};
    float duration{1.0f};
    float delay{0.0f};
    int iterations{1};          // -1 = infinite
    bool alternate{false};
    bool fillForwards{false};
    bool finished{false};
    TransitionFunction ease{TransitionFunction::Linear};
};
```

This is the runtime instance of a `KeyframeAnimation`. It carries the
timing (from the CSS `animation:` shorthand) and the current elapsed
time.

`AnimationPlayer` has two containers for these:

- `std::vector<ActiveCssAnimation> css;` — for the node's own
  animations.
- `std::unordered_map<std::string, std::vector<ActiveCssAnimation>> parts;`
  — for `::part` animations, keyed by part name.

---

## 2. The shared primitives (`Anim` namespace)

Four free functions in `namespace Anim`, declared in
`AnimationPlayer.hpp` and defined in `AnimationPlayer.cpp`.

### 2.1 `syncRefs`

```cpp
void Anim::syncRefs(std::vector<ActiveCssAnimation>& anims,
                    const std::vector<AnimationRef>& refs,
                    double now)
{
    // 1. Remove orphans
    anims.erase(
        std::remove_if(anims.begin(), anims.end(),
            [&](const ActiveCssAnimation& a) {
                for (const auto& r : refs)
                    if (r.name == a.name) return false;
                return true;
            }),
        anims.end());

    // 2. Update existing / add new
    for (const auto& r : refs) {
        bool found = false;
        for (auto& a : anims) {
            if (a.name == r.name) {
                a.duration = r.duration;
                a.delay = r.delay;
                a.iterations = r.iterations;
                a.alternate = r.alternate;
                a.fillForwards = r.fillForwards;
                a.ease = r.ease;
                found = true;
                break;
            }
        }
        if (!found) {
            ActiveCssAnimation a;
            a.name = r.name;
            a.startTime = now;
            // copy r's fields
            anims.push_back(std::move(a));
        }
    }
}
```

Aligns the active instance list against a list of references (the
`animations` field of a `Style`). Three cases:

- **Orphan**: an active animation whose name isn't in `refs` is removed.
- **Existing**: an animation whose name matches a ref has its timing
  fields updated from the ref (so a live edit of the shorthand takes
  effect without restarting).
- **New**: a ref with no matching active instance gets a fresh
  `ActiveCssAnimation` with `startTime = now`.

The `startTime` parameter is `double`, coming from
`UIContext::get().time`. It's used for absolute ticking (parts).

Called from `AnimationPlayer::syncCss` (node CSS) and from
`applyCssToPart` (part CSS).

### 2.2 `tickIncremental`

```cpp
void Anim::tickIncremental(std::vector<ActiveCssAnimation>& anims, float dt)
{
    for (auto& anim : anims) {
        if (anim.finished) continue;
        anim.elapsed += dt;
        auto it = Theme::get().keyframes.find(anim.name);
        if (it == Theme::get().keyframes.end()) {
            anim.finished = true;
            continue;
        }
        bool fin = false;
        (void)sampleActive(anim, fin);
        if (fin) anim.finished = true;
    }

    anims.erase(
        std::remove_if(anims.begin(), anims.end(),
            [](const ActiveCssAnimation& a) {
                return a.finished && !a.fillForwards;
            }),
        anims.end());
}
```

Used for the **node's own** CSS animations. Advances `elapsed` by `dt`,
then samples the animation to see if it's finished. Removes finished
animations that don't have `fillForwards`.

The `elapsed += dt` accumulation is why node CSS animations are called
"incremental" — they depend on `dt` being stable. A frame drop causes a
larger `dt` for that frame, which advances the animation further.

### 2.3 `tickAbsolute`

```cpp
void Anim::tickAbsolute(std::vector<ActiveCssAnimation>& anims, double now)
{
    for (auto& anim : anims) {
        if (anim.finished) continue;
        anim.elapsed = (float)(now - anim.startTime);
        // ...
    }
    // ...
}
```

Used for **part** CSS animations. Instead of accumulating `dt`, it
recomputes `elapsed` from the absolute time. This makes part animations
insensitive to frame drops — they always progress at wall-clock rate.

Why the difference? Node animations are tied to the frame loop; if the
frame takes 30ms instead of 16ms, the animation should still feel
"1 frame slower". Parts are visual flourishes (glowing, popping) that
look better at wall-clock rate. It's a small semantic difference, but
intentional.

### 2.4 `sampleActive`

```cpp
inline float sampleActive(const ActiveCssAnimation& a, bool& finished) {
    finished = false;
    float t = a.elapsed - a.delay;
    if (t < 0.0f) return 0.0f;
    if (a.duration <= 0.0f) { finished = true; return 1.0f; }

    float total = (a.iterations < 0) ? -1.0f
                                     : (a.duration * (float)a.iterations);
    if (a.iterations >= 0 && t >= total) {
        t = total;
        finished = true;
    }
    float iter = std::floor(t / a.duration);

    if (a.iterations > 0 && finished && iter > 0.0f) {
        iter = std::min(iter, (float)(a.iterations - 1));
    }

    float local = (t - iter * a.duration) / a.duration;
    if (a.alternate && ((int)iter % 2 == 1))
        local = 1.0f - local;
    return std::clamp(local, 0.0f, 1.0f);
}
```

Returns the **local** progress within the current iteration, `[0, 1]`.
Sets `finished` if the total elapsed exceeds the total duration.

Handles:

- **Delay**: if `elapsed < delay`, returns 0 (progress hasn't started).
- **Zero duration**: immediately finished at `1.0`.
- **Infinite iterations** (`iterations < 0`): never finished.
- **Alternate**: on odd iterations, reverses the local time.
- **Overrun**: clamps to the last iteration's end when finished.

The `iter > 0` clamp for finished non-infinite animations handles the
edge case where `elapsed` exactly equals `total`: without the clamp,
`floor(t / duration)` would point to the *next* iteration, which
doesn't exist.

### 2.5 `evaluateKeyframes`

```cpp
inline Style evaluateKeyframes(const KeyframeAnimation& anim,
                                float localT,
                                TransitionFunction ease)
{
    Style out;
    if (anim.keyframes.empty()) return out;
    if (anim.keyframes.size() == 1) return anim.keyframes[0].delta;

    // Find the two keyframes surrounding localT
    const Keyframe* k0 = &anim.keyframes.front();
    const Keyframe* k1 = &anim.keyframes.back();
    for (size_t i = 0; i + 1 < anim.keyframes.size(); ++i) {
        if (localT >= anim.keyframes[i].t &&
            localT <= anim.keyframes[i + 1].t)
        {
            k0 = &anim.keyframes[i];
            k1 = &anim.keyframes[i + 1];
            break;
        }
    }

    float span = k1->t - k0->t;
    float u = (span > 0.0f) ? (localT - k0->t) / span : 0.0f;
    u = getRatio(std::clamp(u, 0.0f, 1.0f), ease);

    #define X(T, name, def)                                              \
    if (k0->delta.name.is_set && k1->delta.name.is_set)                  \
        out.name = lerpProp(k0->delta.name.value, k1->delta.name.value, u); \
    else if (k0->delta.name.is_set) out.name = k0->delta.name.value;     \
    else if (k1->delta.name.is_set) out.name = k1->delta.name.value;
    BUBBLE_STYLE_PROPS(X)
    #undef X

    return out;
}
```

Given a `localT` in `[0, 1]`, finds the segment between two keyframes
and interpolates each set property. The output is a `Style` (with `Opt`
fields) — this will be overlaid onto the node's `ComputedStyle` later.

The `BUBBLE_STYLE_PROPS` macro expands to all ~45 properties. For each
property:

- If both endpoints have it set, lerp.
- If only one has it, use it (this is the **auto-fill** behavior from
  the parser's perspective; but note the parser already auto-fills
  missing properties, so in practice both endpoints usually have it set
  unless the animation is defined programmatically).

The easing is applied to `u`, not to the segment boundaries. So an
animation with `ease-out-back` will overshoot at the end of **each**
segment, not just the last one. This matches CSS.

### 2.6 `overlayCssStyle` / `overlayCssComputed`

```cpp
void Anim::overlayCssStyle(Style& out,
                            const std::vector<ActiveCssAnimation>& anims)
{
    for (const auto& anim : anims) {
        auto it = Theme::get().keyframes.find(anim.name);
        if (it == Theme::get().keyframes.end()) continue;
        bool fin = false;
        float localT = sampleActive(anim, fin);
        if (fin && !anim.fillForwards) continue;
        Style frame = evaluateKeyframes(it->second, localT, anim.ease);
        out.overlay(frame);
    }
}
```

Iterates the active animations, evaluates each keyframe, and overlays
them in order. The last animation wins on conflicts.

Two variants:

- `overlayCssStyle(Style&, anims)` — used in `StyleResolver::partFor`
  to overlay CSS animations onto a part's `Style`.
- `overlayCssComputed(ComputedStyle&, anims)` — used in `Layout::draw`
  to overlay animations onto the node's render style.

Both call `overlayComputed` internally, which is the same macro-driven
"apply all set fields" as `Style::overlay`.

---

## 3. Imperative animations

### 3.1 `AnimationPlayer::play`

```cpp
void AnimationPlayer::play(Layout& node, const std::string& name,
                           bool playReverse)
{
    auto it = imperative.find(name);
    if (it == imperative.end()) return;

    auto& state = it->second;
    state.reverse = playReverse;
    state.playing = true;

    if (!state.reverse && state.elapsed >= state.anim->duration)
        state.elapsed = 0.0f;
    else if (state.reverse && state.elapsed <= 0.0f)
        state.elapsed = state.anim->duration;

    state.delayElapsed = state.anim->delay;
}
```

Called from `Layout::playAnimation`, which is called by user code.
Behavior:

- If the animation isn't registered, do nothing silently.
- Set the direction.
- Set `playing = true`.
- **Reset `elapsed` only if the animation is at the wrong end.** If
  you play forward and the animation already finished, reset to `0`
  (restart). If it's in the middle, resume from where it was.
- Reset the delay counter so the delay is applied again.

This means an animation that's already playing forward and gets
`playAnimation(name, false)` again does nothing visible — it just
continues. A reverse call on a finished-forward animation starts from
the end and runs backwards.

The delay is applied on every play, not just the first. If you want a
delay only on the first play, clear it after the initial call (there's
no API for this — set `anim->delay = 0` from the callback).

### 3.2 `tickImperative`

```cpp
bool AnimationPlayer::tickImperative(Layout& node, float dt)
{
    bool localBlocks = false;

    for (auto& [n, s] : imperative) {
        if (!s.playing) continue;

        if (s.anim->blocksInput) localBlocks = true;

        if (s.delayElapsed > 0.0f) {
            s.delayElapsed -= dt;
            continue;
        }

        s.elapsed += (s.reverse ? -dt : dt);
        bool justFinished = false;

        if (s.elapsed <= 0.0f) {
            s.elapsed = 0.0f;
            s.playing = false;
        }
        if (s.elapsed >= s.anim->duration) {
            s.elapsed = s.anim->duration;
            if (s.playing && !s.reverse) justFinished = true;
            s.playing = false;
        }

        float t = (s.anim->duration > 0.0f)
            ? (s.elapsed / s.anim->duration) : 1.0f;
        for (auto& track : s.anim->tracks)
            track->apply(t, &node);

        node.pendingTransition = true;

        if (justFinished) {
            auto cb = s.anim->onFinished;
            if (cb) cb();
        }
    }

    return localBlocks;
}
```

Called from `Layout::update`, before the state machine. Returns whether
any active animation has `blocksInput`.

Step by step for each animation:

1. Skip if not playing.
2. If `blocksInput` is set, mark `localBlocks = true` (this persists
   even if the animation is in its delay phase).
3. If in the delay phase, decrement `delayElapsed` and skip the rest.
4. Advance `elapsed` by `dt`, in the direction of `reverse`.
5. Clamp at both ends. Set `playing = false` when reaching an end.
6. Set `justFinished = true` only if finishing forward (reverse
   completion doesn't fire `onFinished`).
7. Compute `t = elapsed / duration`.
8. Apply each track with `t`.
9. Set `node.pendingTransition = true`, so the style system re-resolves.
10. If `justFinished`, copy the callback and call it.

The callback is copied **before** being invoked, because the callback
might modify the `imperative` map (e.g. by calling `removeFromParent`
on the node, which doesn't affect the map, or by calling
`addAnimation` which does). Copying the `std::function` ensures the
call is safe even if the map is mutated during the callback.

### 3.3 `hasActive` and `hasActiveLocal`

```cpp
bool AnimationPlayer::hasActive(const Layout& node) const
{
    for (const auto& [n, s] : imperative)
        if (s.playing) return true;
    if (!css.empty()) return true;
    if (hasActivePartAnims()) return true;

    for (const auto& c : node.children)
        if (c->hasActiveAnimations()) return true;
    return false;
}
```

`hasActive` is **recursive**: it checks whether the node or any
descendant has active animations.

```cpp
bool hasActiveLocal() const {
    for (const auto& [n, s] : imperative)
        if (s.playing) return true;
    if (!css.empty()) return true;
    for (const auto& [pn, v] : parts)
        if (!v.empty()) return true;
    return false;
}
```

`hasActiveLocal` is **non-recursive**: only checks whether this node
has active animations.

`hasActiveLocal` is used in `Layout::update` to fire
`onAnimationsFinished` when the node goes from "active" to "inactive".
`hasActive` is used by the public `Layout::hasActiveAnimations()`.

Note that `css` is checked with `!css.empty()`, not "has any not-finished".
A finished animation with `fillForwards = true` is still in the list
(see §4.4), so `hasActiveLocal` returns true for it. This is correct:
`fillForwards` animations are visually active (they keep the final
keyframe applied), so the node can't be considered "done".

---

## 4. Node CSS animations

### 4.1 `syncCss`

```cpp
void AnimationPlayer::syncCss(Layout& node)
{
    const auto& refs = node.style_.targetStyle.animations;
    Anim::syncRefs(css, refs, UIContext::get().time);
}
```

Called from `Layout::update` after any style resolution (state change or
pending transition). It syncs the active CSS animation list against the
`animations` field of the node's **target** style (not current).

Why target style? Because `targetStyle` is what the node "should look
like" now; `currentStyle` lags behind during a transition. Syncing
against the target means a new animation starts as soon as the target
style includes it, without waiting for the transition to complete.

### 4.2 `tickCss`

```cpp
void AnimationPlayer::tickCss(Layout&, float dt)
{
    Anim::tickIncremental(css, dt);
}
```

Called from `Layout::update`, after `style_.tick`. Node CSS animations
use **incremental** ticking — `elapsed += dt`.

### 4.3 When animations start and stop

Start:

1. A style is resolved with a new `animation:` shorthand.
2. `syncRefs` adds a new `ActiveCssAnimation` with `startTime = now`,
   `elapsed = 0`.
3. On the next `tickCss`, it starts advancing.

Stop (natural):

1. `tickIncremental` samples the animation and finds it finished.
2. If `fillForwards == false`, the animation is removed.
3. If `fillForwards == true`, it stays in the list with `finished = true`.

Stop (forced):

1. A style is re-resolved **without** the `animation:` shorthand.
2. `syncRefs` sees the ref is gone and removes the active instance.
3. The animation stops immediately, no fade.

There's no `animation-play-state: paused` equivalent. To pause an
animation, remove the `animation` property (or the class that carries
it) and re-add it. Restarting requires the same trick, plus the
`syncRefs` behavior of not resetting `startTime` for an existing name —
but if the instance was removed and re-added, `startTime` is fresh.

### 4.4 `fillForwards`

If `fillForwards = true`, a finished animation **stays in the list**
with `finished = true`. During the overlay, `sampleActive` returns
`localT = 1.0` (the last keyframe), so the final animated values are
applied to the render style.

This means a `forwards` animation keeps the node looking like the last
keyframe until the `animation:` property is removed. Useful for
one-shot animations that should end on a specific state (fade-in
ending at opacity 1).

If `fillForwards = false` (the default), the animation is removed when
it finishes, and the node reverts to its non-animated style. This is
the correct behavior for infinite animations (which never finish) and
for animations that should "snap back" (like a pulse).

### 4.5 Where the overlay happens

Node CSS animations are applied **at draw time**, not at style
resolution time:

```cpp
void Layout::draw(float parentOpacity) {
    // ...
    ComputedStyle renderStyle = style_.currentStyle;
    Anim::overlayCssComputed(renderStyle, anim_.css);
    // ...
}
```

This means:

- `getStyle()` returns the un-animated style.
- Hit-testing uses the un-animated rect.
- Layout (`measure` / `arrange`) uses the un-animated size.
- Only the visual output reflects the animation.

This is a deliberate simplification. The alternative would be to apply
the animation to `currentStyle` in `update`, which would make
`getStyle()` reflect the animation but would force a re-layout every
frame for every animated node. For most animations (opacity, scale,
translate) that's not necessary. For animations of `width` / `height`
it would matter — but those are rare.

The consequence is documented in [Animations §2.6](../user/07-animations.md):
don't rely on `getStyle()` to observe an animation's effect.

---

## 5. Part CSS animations

### 5.1 `applyCssToPart`

```cpp
void AnimationPlayer::applyCssToPart(Layout& node, const std::string& partName,
                                     Style& out)
{
    const auto& refs = out.animations.is_set
        ? out.animations.value
        : std::vector<AnimationRef>{};

    auto& anims = parts[partName];
    const double now = UIContext::get().time;

    Anim::syncRefs(anims, refs, now);
    Anim::tickAbsolute(anims, now);
    Anim::overlayCssStyle(out, anims);
}
```

Called from `StyleResolver::partFor`, which is itself called from
`partStyle(partName)` inside a widget's `renderContent`.

Three steps:

1. **Sync**: align the active list with the part's declared
   `animations` (from the resolved part style).
2. **Tick**: advance elapsed by recomputing from `now - startTime`.
3. **Overlay**: apply the current keyframe values to the part's
   `Style` (`out`), which is then returned by `partFor`.

Note that `tickAbsolute` is used here, not `tickIncremental`. The part
has no `dt` — it only knows the current absolute time and when it
started. So elapsed is recomputed each call.

### 5.2 The `parts` map

```cpp
std::unordered_map<std::string, std::vector<ActiveCssAnimation>> parts;
```

Keyed by part name. Each entry is the active list for that part.

Entries are added lazily: the first `applyCssToPart(node, "track")` call
for a given node creates `parts["track"]`. Entries are never removed —
even if a part is no longer styled, its (empty) `anims` vector stays in
the map. This is a minor memory overhead, not a leak.

### 5.3 When parts tick

A part only ticks when `applyCssToPart` is called, which is when
`partStyle(name)` is called, which is when a widget's `renderContent`
(or `renderChrome`) runs. If a widget skips rendering a part (e.g.
`Checkbox` skips `::mark` when unchecked), the part's animations don't
advance. When the part is drawn again, its elapsed is recomputed from
`now - startTime`, so it "catches up" instantly.

This is why absolute ticking is the right choice for parts: it doesn't
matter when the calls happen, only the wall-clock time matters.

### 5.4 Part animation examples

```css
@keyframes knobGlow {
    0%   { color: #d6ff32; }
    50%  { color: #ffffff; }
    100% { color: #d6ff32; }
}

Slider:hover::knob {
    animation: knobGlow 0.6s ease-in-out infinite;
}
```

`Slider::renderContent` calls `partStyle("knob")` every frame, which
calls `applyCssToPart(node, "knob", knobStyle)`. On the first frame
where `:hover` is active, `syncRefs` sees a new `knobGlow` ref and
creates an active instance. On subsequent frames, `tickAbsolute`
advances it. The overlay applies the interpolated color to `knobStyle`,
which `renderContent` uses to draw the knob.

When `:hover` goes away, the rule stops matching, `syncRefs` sees the
ref disappear, and the active instance is removed. The part reverts to
its base style.

---

## 6. Interaction with the dirty system

Animations are one of the two reasons a node can't be cached (the other
is a pending state transition). The relevant logic is in
`Layout::recomputeDirty`:

```cpp
bool inTransition = (style_.transitionTimer < 1.0f);

subtreeDirty_ = wasPending || pendingTransition || anyChildDirty ||
                styleChanged || inTransition;
```

- `pendingTransition` is set by imperative animation tracks (they write
  to `inlineBase` via `getInlineBase()`).
- `inTransition` is true while a CSS state transition is in progress.
- CSS **keyframe** animations do not affect `subtreeDirty_`.

Wait — that's important. A node with only a CSS keyframe animation (no
transition, no pending transition) doesn't set `subtreeDirty_` from the
animation. So its subtree can be cached. The keyframe animation still
renders, because the overlay is applied at draw time on `renderStyle`.

But the node's **own** style is applied to a local `renderStyle` in
`draw`, not to `currentStyle`. Since `measure` and `arrange` use
`currentStyle`, the animation's effect on layout is zero (as
documented). So a CSS keyframe animation that only changes `opacity`
or `scale` doesn't need a re-layout, and the cache is valid.

A CSS keyframe animation that changes `width` or `height` **would** be
wrong here — the node would be drawn with the animated size but laid
out with the un-animated size. That's the documented limitation.

### 6.1 `onAnimationsFinished`

```cpp
bool anyLocal = anim_.hasActiveLocal();
if (hadLocalAnimationsLast_ && !anyLocal && onAnimationsFinished)
    onAnimationsFinished();
hadLocalAnimationsLast_ = anyLocal;
```

`hadLocalAnimationsLast_` is a per-node bool. It's `true` if the node
had any active local animation last frame, and `false` otherwise. The
callback fires on the falling edge: `hadLocalAnimationsLast_ == true`
and `anyLocal == false`.

The check is **local**, not recursive — child animations don't count.
This means a container whose children are still animating won't fire
its own `onAnimationsFinished` until its children finish too. Correct
behavior for a "the subtree is done" callback, and it's documented.

---

## 7. Interaction with the style overlay

The interaction between CSS animations and styles is the subtle part.
Let's trace a node with a `:hover { animation: pulse 1s infinite; }`
rule:

### 7.1 Frame N: pointer enters

1. `updateFlags` sets `isHovered = true`.
2. `computeNextState` returns `Hover`, state changed.
3. `beginStateTransition` resolves the new target style. This style
   includes `animations = [{name: "pulse", duration: 1, ...}]`.
4. `anim_.syncCss(*this)` is called. It sees `animations` now has
   `"pulse"`, creates an `ActiveCssAnimation` with `startTime = now`,
   `elapsed = 0`.
5. `style_.tick` runs the state transition (interpolating from non-hover
   to hover style over the declared transition duration, if any).
6. `anim_.tickCss` advances `elapsed` for "pulse" by `dt`.
7. Draw: `renderStyle = currentStyle; overlayCssComputed(renderStyle, css)`.
   The overlay evaluates "pulse" at `localT = elapsed / duration` and
   applies the resulting `Style` on top of `currentStyle`.
8. The node is rendered with the pulsing values.

### 7.2 Frame N+1..M: hovering

Same as above, but `elapsed` advances. The animation runs to completion
(if finite) or continues forever (if infinite).

### 7.3 Frame M+1: pointer leaves

1. `updateFlags` sets `isHovered = false`.
2. State changes back to `Idle`.
3. `beginStateTransition` resolves the non-hover target style. This
   style does **not** have `animations = [{name: "pulse", ...}]`.
4. `anim_.syncCss` sees `animations` is now empty. `syncRefs` removes
   the "pulse" active instance.
5. Draw: `renderStyle = currentStyle` (mid-transition). The overlay has
   nothing to apply.
6. The node reverts to its non-hover style, transitioning if a
   `transition:` is declared.

The pulse animation stops **immediately** — no graceful finish. This is
documented in [Animations §7.3](../user/07-animations.md).

### 7.4 Overlapping animations

If the rule declared two animations (`animation: pulse 1s, glow 2s`),
both are in the `animations` vector. `syncRefs` creates two
`ActiveCssAnimation`. `overlayCssComputed` iterates them in order and
applies each. The **last** one wins on conflicts, which is what CSS
does.

There's no way to control the priority order other than the order in
the shorthand.

### 7.5 Interaction with transitions

If a property has both a `transition:` and is in an animated keyframe:

- The transition runs on `currentStyle`, from the old state to the new
  state.
- The animation overlays the transition result at draw time.

So the property ends up with the **animated** value, not the transition
value. The transition still runs (visually invisible), and its timer
still reaches 1.0 (so `inTransition` eventually becomes false).

This wastes CPU on the invisible transition. To avoid it, don't
transition a property you're also animating. In practice, this is rare.

---

## 8. Removing and re-adding animations

### 8.1 Removing an imperative animation

```cpp
node->addAnimation("intro", anim);
node->playAnimation("intro");
// ...
// To remove:
// (no public API — you'd need to modify node->anim_.imperative)
```

There's no public `removeAnimation` on `Layout`. The `imperative` map
is a private member of `AnimationPlayer`, accessible only via
`friend`. To remove an imperative animation:

- Overwrite the same name with a new one (the old one is dropped).
- Remove the node itself.
- Add a subclass that accesses `anim_` via the friend relationship.

In practice, imperative animations are rarely removed — they're small
and inactive ones don't tick.

### 8.2 Removing a CSS animation

Remove the class that carries the `animation:` rule:

```cpp
// No public removeClass() — see below
```

There's **no `removeClass`** in the current API. To stop a CSS
animation:

- Rebuild the node without the class.
- Add an inline style that overrides `animation:` to an empty list —
  except `applyStyleAttr` doesn't handle `animation`, so this doesn't
  work either.
- Define the `animation` under a rule that can be deactivated (e.g.
  `.node.animating { animation: pulse 1s; }`), and toggle
  `.animating` — except there's no `removeClass` to remove it.

The realistic options are:

1. **Rebuild the node** — replace it with a fresh one that doesn't have
   the animating class.
2. **Use an imperative animation** — they're easier to stop (though
   there's no `stopAnimation` API either, but at least you can
   overwrite).
3. **Use a state-driven animation** — `:hover`, `:checked`, etc. The
   animation runs only while the state is active.

The demo uses approach 3 for the slider knob glow. For most other cases,
imperative is the way.

### 8.3 Re-adding an animation

Same as removing: rebuild, or use a state. If you want a "play again"
behavior, an imperative animation with `playAnimation("name")` resets
`elapsed` if it's at the end:

```cpp
// Assuming "intro" already finished forward
node->playAnimation("intro");   // restarts from 0
```

This works because `play` checks:

```cpp
if (!state.reverse && state.elapsed >= state.anim->duration)
    state.elapsed = 0.0f;
```

So calling `play` on a finished-forward animation restarts it.

For CSS animations, the equivalent would require re-adding the class,
which isn't possible without a `removeClass`. So imperative wins for
repeatable one-shots.

---

## 9. Extending the system

### 9.1 Adding a new easing function

1. Add the enum value in `Easing.hpp`:

   ```cpp
   enum class TransitionFunction {
       // ...
       EaseInOutSine,
   };
   ```

2. Add the case in `getRatio`:

   ```cpp
   case TransitionFunction::EaseInOutSine:
       return -(std::cos(pi * t) - 1.0f) / 2.0f;
   ```

3. Add a name in `parseEasing`'s table:

   ```cpp
   {"easeinoutsine", TransitionFunction::EaseInOutSine},
   ```

4. Add a name in `Debug::toString(TransitionFunction)` for dumps.

All three places use a switch or a table. Adding a function means
adding one entry to each.

### 9.2 Adding a new animation pattern

If you want a custom animation behavior (e.g. a "shake" that's not
expressible as keyframes), you'd write an imperative `UIAnimation` with
tracks that use `std::sin` in the setter:

```cpp
auto shake = std::make_shared<UIAnimation>(0.5f);
shake->addTrack<float>(0.0f, 1.0f,
    [](Layout* l, float t) {
        float x = std::sin(t * 20.0f) * (1.0f - t) * 10.0f;
        l->getInlineBase().translateX = Px(x);
    },
    TransitionFunction::Linear);
```

The track's `apply` computes `t` (normalized progress), and the setter
can use any math. This is the escape hatch for animations that don't
fit the keyframe model.

### 9.3 Adding a new track type

`UIAnimation::addTrack<T>` is generic. It works for any `T` that has a
`lerpProp` overload. Available:

- `float`
- `Color`
- `Value`
- `Spacing`
- Any enum (snap)
- `ZIndex` (snap)
- `std::string` (snap)

To add a new type, add a `lerpProp` overload for it in `Style.hpp`. If
the type doesn't have a natural linear interpolation (like
`TextureRef`), you can snap (`return t > 0.0f ? b : a;`) or provide a
custom interpolation.

### 9.4 Interpolating non-Style data

A track's setter receives the node and the interpolated value. It can
do anything with them:

```cpp
anim->addTrack<Vec2>({0, 0}, {100, 0},
    [](Layout* l, Vec2 v) {
        // Update a custom field, call a function, whatever
    },
    TransitionFunction::Linear);
```

`Vec2` doesn't have a `lerpProp` overload, so you'd need to add one.
Or use a `float` track and construct the `Vec2` in the setter.

---

## 10. Performance

The three channels have very different costs.

### 10.1 Transitions

Cheap. They only run when a state changes, and the tick is a set of
`lerpProp` calls on the properties declared in `transition:`. The
number of properties is the cost per frame, not the number of nodes.

### 10.2 Imperative animations

Medium. Each playing animation runs a `for` loop over its tracks and
calls each track's setter. The setter typically writes to
`getInlineBase()`, which sets `pendingTransition = true`, which forces
a style re-resolve on the same frame. So an imperative animation costs:

- The tick loop itself (`apply` per track).
- One `resolveFor` per frame for the node.
- One `renderStyle` overlay per frame at draw time (no animation
  overlay for imperative, but the style re-resolve does happen).

For a few animations, this is fine. For dozens on the same frame, the
style resolution becomes the bottleneck.

### 10.3 Node CSS animations

Medium. Each active animation runs `sampleActive` +
`evaluateKeyframes` per frame. `evaluateKeyframes` iterates all ~45
properties via the macro. That's the cost.

`sampleActive` and `evaluateKeyframes` are called in two places:

- `tickIncremental` — to detect finish.
- `overlayCssComputed` — to apply at draw time.

So each active CSS animation does two full evaluations per frame. This
is a known inefficiency. If you have many CSS animations, you'd want
to cache the evaluation between `tickCss` and `draw`. The current
implementation doesn't.

### 10.4 Part CSS animations

Cheap-to-medium. Same as node CSS, but only when the widget calls
`partStyle(partName)`, which is once per frame per drawn part.
Same double-evaluation issue applies.

### 10.5 Memory

- `imperative` map: one `AnimState` per registered animation. `shared_ptr`
  to the `UIAnimation`, which owns its tracks (which own their setters,
  which capture whatever the user captured).
- `css` vector: one `ActiveCssAnimation` per running animation. Small
  (a string + a few floats).
- `parts` map: one entry per part name per node that has part
  animations. Even empty entries take a slot.

There's no per-frame allocation in the tick path. `tracks` is a vector
of `unique_ptr`, iterated by reference. `evaluateKeyframes` returns a
`Style` by value — that's a stack allocation (a `std::vector` and a
`std::unordered_map` inside, but usually empty), so the cost is
minimal.

---

## 11. Pitfalls

**`onFinished` fires only forward.**
Reverse completion (via `playAnimation(name, true)`) doesn't fire the
callback. If you need reverse-completion notification, use
`onAnimationsFinished` on the node (which fires when **all** animations
stop, either direction).

**`onFinished` is copied before being invoked.**
If the callback replaces itself (sets `anim->onFinished = ...`), the
new callback takes effect on the next play, not the current one. This
is by design — it prevents mutation during iteration.

**`blocksInput` persists through the delay.**
If an animation has `delay = 1s` and `blocksInput = true`, the node is
blocked for the full second of delay. To let input through during the
delay, set `blocksInput = false` explicitly.

**CSS animations don't re-start on state re-entry.**
If a node has `:hover { animation: pulse 1s; }` and the pointer leaves
and re-enters within a few frames, the animation resumes from where it
was — because `syncRefs` for an existing name doesn't reset `startTime`.
To force a restart on each hover, the state must change to remove the
animation (which it does, since the state changes to `Idle` and the
animation is removed), and then re-add it (creating a fresh instance
with `startTime = now`). So in practice it does restart, but only if
the state fully changes. If you re-enter in the same frame, no.

**`hasActiveAnimations()` is recursive.**
It returns true if the node or any descendant has active animations.
Use it for "is this subtree done?", not "is this node done?". For the
latter, `hasActiveLocal` is the right check, but it's a private method
of `AnimationPlayer` (accessed via `friend`).

**Node CSS animation value is not in `getStyle()`.**
As documented: only `renderStyle` at draw time sees the animated value.
Hit-testing, layout, and `getStyle()` see the un-animated value.

**CSS animations tick twice per frame.**
`tickIncremental` calls `sampleActive` and `evaluateKeyframes` to detect
finish; `overlayCssComputed` calls both again to apply. So each active
node CSS animation does ~90 property comparisons and up to ~90
`lerpProp` calls per frame. For an infinite animation, that's ~90
lerps × 60 fps per animation. Not catastrophic, but not free.

**Imperative animation setters that write to non-style state.**
```cpp
anim->addTrack<float>(0, 1, [](Layout* l, float t) {
    // Suppose you have a custom widget with a `progress` field
    static_cast<MyWidget*>(l)->progress = t;   // ⚠️ static_cast is unsafe
});
```

The `Track` template doesn't know the actual type of the layout — it's
`Layout*`. If you `static_cast` to your widget, you're trusting that the
animation is only ever played on a `MyWidget` instance. If you play it
on a plain `Layout`, you get undefined behavior. Use `dynamic_cast` for
safety (with the associated cost), or only use layout properties in the
setter.

**`getInlineBase()` in a setter triggers a re-resolve.**
```cpp
l->getInlineBase().opacity = v;
```
`getInlineBase()` returns a mutable reference and sets
`pendingTransition = true`. Even reading it (without writing) sets the
flag. So if your track setter reads the base to compute something, then
writes back, you pay for two re-resolves. Use `getStyle()` for reading,
`getInlineBase()` only for writing.

Actually, `getInlineBase()` does NOT read — it just returns a reference
and sets the flag. So calling it once per frame is one flag set. But
if you call it twice per frame (once to read, once to write), you set
the flag twice (idempotent, but the second call returns the same
reference, so it's really just the flag). It's not a real cost, but
worth knowing that merely calling `getInlineBase()` has a side effect.

**The `animate()` shorthand in `.zstyle` doesn't exist.**
It's `animation:` (with `-i-`), not `animate:`. A typo silently does
nothing (the property is "unknown" and logged as a warning). Check the
logs if animations don't run.

**Keyframe names must be unique.**
`Theme::addKeyframes` stores by name, so a second `@keyframes pulse`
overwrites the first. If you load two stylesheets that both define
`pulse`, the second wins. There's no namespacing.

**`animation-delay` requires `animation` first.**
If `animation-delay: 0.5s` appears without a preceding `animation:`,
the parser logs a warning and ignores it. The parser uses
`st.animations.is_set` to decide whether there's a target for the
longhand. Same for all the other longhands.