# ZenitUI — Roadmap

This document tracks the path from the current **alpha** state toward a
**beta** release and, eventually, **1.0**. It's both a plan and a
snapshot of the current state: each section lists what's done, what's
pending, and what's blocking.

It's derived from a full code + documentation review. The **blocking**
items (bugs, missing API, critical test gaps) are prioritized. The
**nice-to-have** items are collected at the end.

Last updated: review pass after the initial documentation.

---

## Status

**Current:** 🗓️ beta — the core API is frozen, no known critical bugs,
test coverage on every widget, documentation complete. Breaking
changes are still possible for post-beta work (Phases 4–8) but the
surface described in Phases 1–3 is stable.

**Next milestone:** 🚀 1.0 — feature-complete for a typical game /
tool use case, stable API, tooling available. Requires Phases 4 and 5.

**What "beta" means in this project:**

- Phase 1 (bug fixes) — complete.
- Phase 2 (missing core API) — complete.
- Phase 3 (test coverage) — complete.
- Documentation — audited and updated.

Phases 4–8 are post-beta work. They are additive and don't require
breaking the API.

---

## Phase 1 — Bug fixes

Before anything else. These are known bugs or behaviors that don't
match the documented intent. They should be fixed before adding new
features.

### 1.1 `onHoverExit` fires on `Hover → Pressed`

**Severity:** high — affects every widget that uses hover callbacks
(tooltips, highlights).

**Location:** `src/Input.cpp`, `InputController::fireCallbacks`.

**Problem:** when a node is hovered and then pressed, the state
transition is `Hover → Pressed`, and the current logic fires
`onHoverExit` because `Pressed != Hover`.

**Expected:** `onHoverExit` should only fire when the pointer actually
leaves the node. `Pressed` is a geometric subset of `Hover`.

**Fix:** treat both `Hover` and `Pressed` as "hovered" in the callback
condition:

```cpp
bool wasHovered = (prevState == UIState::Hover || prevState == UIState::Pressed);
bool isHovered  = (nextState  == UIState::Hover || nextState  == UIState::Pressed);
if (isHovered && !wasHovered && node.onHoverEnter) node.onHoverEnter();
else if (!isHovered && wasHovered && node.onHoverExit) node.onHoverExit();
```

**Test:** add a test in `test_interaction.cpp` that verifies
`onHoverEnter` fires once and `onHoverExit` doesn't fire on press.

- [x] Fix the condition in `InputController::fireCallbacks`
- [x] Add regression test
- [x] Update documentation (remove the "known issue" note in
      `internals/07-input-system.md`)

---

### 1.2 `resetIfOverflowChanged` uses a stale snapshot

**Severity:** medium — causes spurious scroll resets in some sequences.

**Location:** `src/ScrollController.cpp`,
`ScrollController::resetIfOverflowChanged`; `src/StyleResolver.cpp`,
`StyleResolver::propagateInheritance`.

**Problem:** `resetIfOverflowChanged` compares the current `overflow`
against `lastInherited.overflowX/Y`. But `propagateInheritance` only
updates `lastInherited` when *some other* property changed — overflow
is not in the `changed` check. So the snapshot is updated
inconsistently, and `resetIfOverflowChanged` can see spurious changes.

**Fix:** either (a) add `overflowX` / `overflowY` to the `changed`
check in `propagateInheritance`, or (b) give the scroll controller its
own overflow snapshot, updated after `resetIfOverflowChanged` runs.

Option (b) is cleaner — it decouples the scroll system from the
inheritance snapshot, which has a different purpose.

**Test:** add a test that changes `overflow` without changing anything
else and verifies that `reset()` is called exactly once.

- [x] Move the overflow snapshot into `ScrollController`
- [x] Update `resetIfOverflowChanged` to use the local snapshot
- [x] Add regression test in `test_interaction.cpp`
- [x] Remove the "known issue" note in `internals/08-scroll-system.md`

---

### 1.3 `translateSubtree` translates portals

**Severity:** medium — affects portals inside scroll containers.

**Location:** `src/Measure.cpp`, `Layout::translateSubtree`.

**Problem:** the `isPortal()` check is commented out. Portal nodes and
their descendants get translated by the scroll offset. The portal's own
`rect` is usually overwritten in its `onUpdate`, but its **children**
are translated, which can move them unexpectedly.

**Expected:** portals should be opaque to `translateSubtree` — their
position is independent of the scroll container.

**Fix:** re-enable the check:

```cpp
void Layout::translateSubtree(float dx, float dy) {
    for (auto& c : children) {
        if (c->isPortal())
            continue;
        c->rect.x += dx;
        c->rect.y += dy;
        c->translateSubtree(dx, dy);
    }
}
```

**Test:** create a `ScrollView` with a `Popup` child, scroll it, verify
the popup's rect doesn't move.

- [x] Re-enable the `isPortal()` check
- [x] Add regression test
- [x] Verify `Dropdown` still works inside a `ScrollView`
- [x] Update `internals/08-scroll-system.md`

---

### 1.4 `Vec3` / `Vec4` filter params scale to `[0, 255]`

**Severity:** medium — affects every filter that uses those types.

**Location:** `src/FilterRegistry.cpp`, `setParams`.

**Problem:** `Vec3` and `Vec4` interpret their float arguments as
`[0, 1]`, clamp, and multiply by 255. The shader receives a `vec4` in
`[0, 255]`. This is not what a shader author expects for a `vec3` or
`vec4` uniform.

**Fix (preferred):** make `Vec3` / `Vec4` pass their values as-is (in
`[0, 1]`) via a new `setEffectVec3` / updated `setEffectVec4`. Keep
`Color` as the `uint8_t`-based type.

**Fix (alternative):** document the current behavior explicitly and
require shaders to normalize. Worse, but cheaper.

**Test:** add a filter test that sets a `Vec4` param and verifies the
uniform value. Requires a way to read back uniforms from the mock
renderer.

- [x] Decide between preferred and alternative
- [x] Implement
- [x] Add test
- [x] Update `user/09-effects.md` and `api/FilterRegistry.md`

---

### 1.5 `hitTest` ignores the current clip

**Severity:** low-medium — causes clicks on invisible parts of clipped
nodes.

**Location:** `src/Measure.cpp`, `Layout::hitTest`.

**Problem:** the final `rect.contains(p)` check doesn't account for the
active clip rect. A node clipped by an ancestor's `overflow: hidden` is
still hit-testable on its full rect.

**Fix:** intersect `rect` with `renderer->getClipRect()` before
`contains(p)`. The clip is in screen coordinates, so this requires
careful transform handling.

**Test:** create a clipped node, hit-test outside the clip but inside
the rect, verify no hit.

- [x] Implement clip-aware hit-testing
- [x] Add regression test
- [x] Consider performance (a `getClipRect()` call per node per
      hit-test)

---

### 1.6 `notifyFocusAncestors` not called on click-focus

**Severity:** low — asymmetry between Tab and click focus.

**Location:** `src/Layout.cpp`, `Layout::updateTree`.

**Problem:** when focus is granted via click, `notifyFocusAncestors` is
not called. So a container's `onDescendantFocused` doesn't fire on
click, only on Tab.

**Fix:** call `notifyFocusAncestors` after the `requestFocus` in the
pointer-pressed branch.

**Test:** create a focusable node inside a container with
`onDescendantFocused`, click it, verify the callback fired.

- [x] Add the call
- [x] Add test
- [x] Update `internals/07-input-system.md`

### 1.7 `Dropdown` hardcoda `height: VH(3.0f)` in `inlineBase`

**Severity:** medium — affects every dropdown whose trigger needs a
different height than the default.

**Location:** `src/ui/components/Dropdown.hpp`, `Dropdown::onBuild`.

**Problem:** the trigger button's `height` (and most of its visual
properties) are set with `getInlineBase()`, which wins over any
`.zstyle` rule. A user can't change the trigger's height via CSS, and
if they add padding larger than the baked-in height, the text gets
clipped to invisibility.

**Expected:** visual defaults should live in `inlineDefaults` (the
"user agent" layer), so a `.zstyle` rule can override them. Only
structural properties — like `width: 100%`, which is what makes the
trigger fill its dropdown — should stay in `inlineBase`.

**Fix:** in `onBuild`, move `height`, `background`, `radius`,
`borderColor`, `borderWidth`, `itemsH`, `itemsV` from `inlineBase` to
`inlineDefaults` for both the trigger button and the list container.

**Test:** add a test that sets `.dropdown-trigger { height: 60px; }`
in a stylesheet and verifies the trigger's actual rect height.

- [x] Move visual properties to `inlineDefaults`
- [x] Add regression test
- [x] Update `user/03-widgets.md` (Dropdown section)

### 1.8 Imperative animations don't progress

**Severity:** high — breaks every imperative animation that writes
into `inlineBase`.

**Location:** `src/ui/layout/StyleResolver.cpp`,
`StyleResolver::resolvePendingTransition`.

**Problem:** an imperative animation writes its interpolated value
into `inlineBase` on every frame. This sets `pendingTransition = true`,
which triggers `resolvePendingTransition` on the next frame. When the
new target differs from the previous one, the method resets
`transitionTimer = 0.0f` and `transitionStartStyle = currentStyle`.
Since the target changes every frame, the timer is reset every frame
and `currentStyle` never reaches it. As a result, imperative
animations produce no visible movement.

**Expected:** while an imperative animation is running, the CSS
transition system must step aside. `currentStyle` should follow the
target directly (snap), with no interpolation — the animation itself
provides the interpolation, and the resolver must not layer a second
transition on top.

**Fix:** added `AnimationPlayer::hasActiveImperative()`. In
`resolvePendingTransition`, when this returns true, `currentStyle` is
aligned immediately to the target instead of starting a transition.

**Test:** added `Anim_imperative.drives_current_style_without_transition`
in `tests/test_animations.cpp`.

- [x] Add `hasActiveImperative` to `AnimationPlayer`
- [x] Update `resolvePendingTransition`
- [x] Add the regression test
- [x] Update `internals/03-style-system.md`
- [x] Update `internals/04-animation-system.md`
- [x] Update `user/07-animations.md`

### 1.9 `CanvasLayout` draws with wrong coordinates inside the target

**Severity:** high — the canvas renders empty or clipped.

**Location:** `src/ui/components/Canvas.hpp`, `CanvasLayout::draw`.

**Problem:** `pushTarget()` resets the coordinate system to `(0, 0)` at
the target's top-left, but the node's `rect` (and its children's rects,
and `renderChrome`'s use of `rect`) are still in **screen coordinates**.
Drawing at screen coordinates inside a target of a smaller size produces
nothing visible: everything is drawn outside the target bounds.

**Expected:** the subtree must be offset by `-rect.x, -rect.y` while
drawing inside the target, so it starts at the target's `(0, 0)`.

**Fix:** push an offset transform around the subtree draw, exactly like
`Layout::drawLayer` does:

```cpp
Transform2D offsetTr;
offsetTr.pivot = {0.0f, 0.0f};
offsetTr.translate = {-rect.x, -rect.y};

r->pushTarget(target);
r->pushTransform(offsetTr);
// ... renderChrome, renderContent, children
r->popTransform();
r->popTarget();
```

**Test:** render a canvas with a child that has a known color, then
verify the target's pixels contain that color. Requires a framebuffer
readback, so this test is easiest with the real Raylib backend rather
than the mock.

- [x] Add the offset transform
- [x] Add a regression test (needs framebuffer readback)
- [x] Update `user/10-custom-widgets.md` if the canvas is mentioned
---

## Phase 2 — Missing core API

Small, high-impact additions that unblock common use cases. No new
features — just filling gaps.

### 2.1 `Slider::setValue(float)`

Add a public setter that clamps to `[0, 1]` and optionally fires
`onValueChanged`. A `bool notify` parameter (default `true`) lets
callers suppress the callback for programmatic updates.

- [x] Add method to `Slider`
- [x] Add test
- [x] Update `user/03-widgets.md` and `api/Widgets.md`

### 2.2 `Layout::removeClass` / `clearStyleClasses`

Without these, CSS animations declared under a class can't be stopped
without rebuilding the node. Also blocks dynamic styling use cases.

- [x] Add `removeClass(const std::string&)`
- [x] Add `clearStyleClasses()`
- [x] Add `hasClass(const std::string&) const`
- [x] Add tests
- [x] Update `api/Layout.md`

### 2.3 `TextInput` cursor positioning

- [x] `setCursorPos(int)` with clamping
- [x] `getCursorPos() const`
- [x] Click-to-position: on click, compute the cursor index from the
      pointer's x position
- [x] Add tests
- [x] Update documentation

### 2.4 `Layout::scrollIntoView`

Scroll the nearest scroll-container ancestor so the node is visible.
Common pattern after `requestFocus`.

- [x] Implement `scrollIntoView()` walking up to find the scroll
      ancestor
- [x] Implement `scrollIntoView(const Rect& subRect)` for partial
      visibility
- [x] Add tests
- [x] Update documentation

### 2.5 `Button` text accessor

`Btn(text, cb)` creates a `Text` child but there's no accessor. Add:

- [x] `Button::text() const` returning the current label
- [x] `Button::setText(std::string)` updating the child

### 2.6 `Text::getText()`

`setText` exists, `getText` doesn't.

- [x] Add `const std::string& getText() const`

### 2.7 `Layout::removeAllChildren()`

A safe bulk-clear for the `children` vector. Useful for rebuilding
lists.

- [x] Add method
- [x] Consider whether it should also clear `parent` pointers of the
      removed children

### 2.8 Compound selectors in ZStyle

**Severity:** high — ZStyle doesn't support `Tag.class`,
`.class1.class2`, `Tag#id`, or `Tag.class:pseudo`. The parser reads
the whole token as a single name, so `Button.btn-primary` never
matches any node.

**Location:** `src/ui/StyleParser.hpp`, the selector-splitting step
inside `parseRule`; `Style.hpp`, `SimpleSelector`; `Layout.hpp`,
`nodeMatchesSimple` and `ruleMatches`.

**Problem:** a token like `Button.btn-primary` is parsed as
`SimpleSelector{kind=Tag, name="Button.btn-primary"}`, which matches
nothing. The parser only handles a single component per token:
`Tag`, `.class`, `#id`, and their `:pseudo` variants. It does not
distinguish a *compound* selector (`Tag.class`, one node) from a
*descendant* selector (`Tag .class`, two nodes).

**Expected:** the parser must recognise compound selectors and the
matcher must require all their components to match the same node.

**Design options:**

- **A.** Extend `SimpleSelector` with optional extra components
  (`Opt<TagName>`, `std::vector<ClassName>`, `Opt<IdName>`) and
  update `computeSpecificity`, `nodeMatchesSimple`,
  `Debug::selectorToString`.
- **B.** Introduce a `CompoundSelector` struct and change
  `ThemeRule::chain` to `std::vector<CompoundSelector>`.
- **C.** Pre-process the selector string at parse time: emit one
  `SimpleSelector` per component and add a `bool compound` flag
  on the `SimpleSelector` to indicate "same node as next".

**Recommendation:** option C is the smallest change and keeps the
existing chain semantics for descendant matching.

**Test:** a stylesheet with `Button.btn-primary { background: red; }`
applied to a `Button` with class `btn-primary` must produce a red
button.

- [x] Extend the parser to split compound selectors
- [x] Update `nodeMatchesSimple` / `ruleMatches`
- [x] Update `computeSpecificity`
- [x] Add tests for `Tag.class`, `.class1.class2`, `Tag#id`, `Tag.class:hover`
- [x] Update `user/05-zstyle.md` (add a section on compound selectors)
- [x] Update `api/ZStyle.md`

### 2.9 `Modal` intercepts clicks while hidden

**Severity:** high — a hidden `Modal` blocks input to any node under it.

**Location:** `src/ui/components/Modal.hpp`, `show` / `hide` / `onBuild`.

**Problem:** the `Modal` is full-screen (`width: 100%; height: 100%`) and
`onBuild` only sets `setInteractive(false)`. Its content VStack and the
content's descendants remain enabled and interactive. The `hitTest`
doesn't check opacity, so the invisible content captures clicks on any
node that happens to be under its rect — including buttons in the page
behind it.

**Expected:** while the modal is hidden, it must not participate in
hit-testing at all. While visible, it must capture input (that's the
point of a modal).

**Fix:** `setEnabled(false)` in `onBuild` (and after `hide()`), and
`setEnabled(true)` in `show()`. Since `hitTest` returns early when
`!isEnabled`, disabling the modal prevents the recursion into its
content — the click falls through to the page.

**Test:** add a test that adds a `Modal` on the same tree as a
clickable button, positioned so that the button's rect overlaps the
modal's full-screen rect. Verify that clicking the button works while
the modal is hidden, and does not work while the modal is shown.

- [x] Disable the modal in `onBuild`
- [x] Enable it in `show()`, disable it in `hide()`
- [x] Add the regression test
- [x] Update `user/03-widgets.md` (Modal section)
- [x] Update `api/Widgets.md`

---

## Phase 3 — Test coverage

The current test suite covers the foundations well (parsing, layout
basics, basic interaction) but has gaps on composite widgets and the
scroll system.

### 3.1 Scroll system

No tests currently exist for the scroll system. Add:

- [x] `scrollTo`, `scrollToX/Y`, `scrollToTop/Bottom/Left/Right`
- [x] `getScrollX/Y`, `getMaxScrollX/Y` after arrange with overflow
- [x] Wheel event increments velocity
- [x] Inertia decays velocity over time
- [x] `maxScroll` is zero when content fits
- [x] Scrollbar thumb hit-test and drag
- [x] `resetIfOverflowChanged` (once 1.2 is fixed)
- [x] Hover freeze during scroll

### 3.2 `TextInput`

- [x] Character insertion
- [x] `Backspace` / `Delete` with cursor position
- [x] `Left` / `Right` cursor movement
- [x] `Home` / `End`
- [x] Key repeat (initial delay + interval)
- [x] `onTextChanged` fires on edit
- [x] `onSubmit` fires on Enter
- [x] `Escape` releases focus without submitting
- [x] Horizontal scroll of the cursor
- [x] Click focuses the input

### 3.3 `Text` wrapping

- [x] `setWrap(true)` with a bounded parent width
- [x] Multiple lines
- [x] Re-wrap on width change (`onLayout`)
- [x] No wrap when parent width is unbounded

### 3.4 `Dropdown`

- [x] `setSelected` fires `onChange`
- [x] Opening the list shows options
- [x] Clicking an option sets the selection and closes
- [x] Clicking outside closes
- [x] Flip above when no space below
- [x] List scrolls when options exceed the visible area

### 3.5 State transitions end-to-end

- [x] `:hover` with `transition:` interpolates over N frames
- [x] `:checked` with transition (Toggle knob)
- [x] `:focus` styling applies on focus
- [x] `:disabled` styling applies on disable
- [x] Transition completes and snaps to target

### 3.6 CSS animations end-to-end

- [x] `@keyframes` with `animation:` changes the render style
- [x] `infinite` animation keeps ticking
- [x] `alternate` reverses every other iteration
- [x] `forwards` keeps the last keyframe
- [x] `::part` animation ticks
- [x] `onAnimationsFinished` fires

### 3.7 Focus scopes

- [x] Tab stays within a scope
- [x] Nested scopes: inner scope wins
- [x] Focus release on click outside
- [x] Focus release on disable

### 3.8 Portal hit-test delay

- [x] Verify that a newly-opened portal becomes hit-testable on the
      **next** frame, not the same one
- [x] Verify that a destroyed portal is skipped

### 3.9 Media queries on viewport change

- [x] Setting `Metrics::viewport` triggers a style re-resolve
- [x] `@media (min-width: 600px)` matches / doesn't match based on the
      viewport

---

## Phase 4 — Performance

These are measurable wins. None are blocking, but they compound.

### 4.1 `Value` without heap allocation

`Value` uses `std::vector<Term>`, which allocates for any non-empty
value. Since a `ComputedStyle` has ~30 `Value` fields and is copied
every frame per node, this is the single largest allocation source.

**Options:**

- **A:** `SmallVector<Term, 2>` — covers 1 and 2 terms without
  allocation. Requires a small custom container.
- **B:** `std::variant<Term, std::pair<Term, Term>, std::vector<Term>>`
  — variant covers 1 and 2 terms, vector covers 3+. More complex API.
- **C:** `std::array<Term, N>` with a size, and a fallback to vector
  when N is exceeded. Fixed-size array is small but wastes space for
  the common 1-term case.

Recommendation: **A** — it's the least invasive and gives the biggest
win for the common case.

- [x] Implement `SmallVector<Term, 2>`
- [x] Replace `std::vector<Term>` in `Value`
- [x] Benchmark before/after
- [x] Verify no behavioral change

### 4.2 Cache `renderStyle` in `draw`

Currently every node copies its `ComputedStyle` in `draw`:

```cpp
ComputedStyle renderStyle = style_.currentStyle;
Anim::overlayCssComputed(renderStyle, anim_.css);
```

The copy involves `std::string`, `std::vector`, and
`std::unordered_map` fields.

**Options:**

- **A:** A per-node cached `ComputedStyle renderStyle_` reused each
  frame. Apply the overlay in place.
- **B:** Skip the copy when there are no active animations (the common
  case), and use `style_.currentStyle` directly.

**Recommendation:** **B** first (simple), then **A** if the cache needs
to persist.

- [x] Implement option B (fast path with no copy)
- [x] Benchmark

The copy of `ComputedStyle` is only performed when the node has
active CSS keyframe animations (`anim_.css` non-empty). When there are
none — which is the case for the vast majority of nodes, since state
transitions are handled by `StyleResolver` and not by
`AnimationPlayer::css` — `draw` skips the copy and calls
`drawWithStyle` directly with `style_.currentStyle`.

This is a pure performance optimization: the two paths behave
identically. `ComputedStyle` is ~720 bytes with the current `Value`
layout, so skipping the copy saves ~0.15 us per drawn node per frame.
On a scene with 20 visible nodes, that's ~3 us per frame — a
measurable fraction of the frame budget.

### 4.3 Rule indexing by tag

`StyleResolver::resolveFor` scans all `Theme::rules` linearly. For
stylesheets with hundreds of rules and nodes that change state
frequently, this is a hotspot.

**Fix:** `Theme` now maintains two indices, updated incrementally by
`addRule`:

- `rulesByTag[tag]` — rules whose rightmost selector is that tag.
- `rulesNoTag` — rules whose rightmost selector is a class or an id
  (they can match any node and must always be tested).

`resolveFor` scans only the bucket for the node's tag plus
`rulesNoTag`, instead of the full rule list. `::part` rules are not
indexed; they are resolved by `partFor`, not by `resolveFor`.

**Benchmark:** `system.frame.200rules` — 100 nodes, 200 rules, style
re-resolve on every node per frame.

| Before 4.3 (estimated) | After 4.3 | 
|------------------------|-----------|
| ~2.3 ms per frame      | ~1.63 ms per frame |

The estimated "before" is based on a local run with the index
disabled; see the analysis below. Because the cost of `resolveFor`
is dominated by `ComputedStyle::from` (cascade, var resolution,
inheritance) rather than by `ruleMatches`, the index helps but does
not eliminate the cost. The visible win is proportional to the rule
count: with 2 rules the index changes nothing, with 200 it halves
the matching cost.

- [x] Add the index to `Theme`
- [x] Update `resolveFor` to use it
- [x] Benchmark with a large stylesheet
- [x] Handle invalidation correctly (`clear` resets both indices)

### 4.4 Cache animation evaluation

**Severity:** low — measurable improvement, not blocking.

**Location:** `src/ui/AnimPrimitives.hpp`, `src/ui/layout/AnimationPlayer.cpp`.

**Problem:** `sampleActive` is called twice per animation per frame:
once in the tick phase (`tickIncremental` / `tickAbsolute`) to detect
whether the animation just finished, and once in the overlay phase
(`overlayCssComputed` / `overlayCssStyle`) to compute `localT` for the
keyframe evaluation. Between the two calls, `elapsed` doesn't change,
so the second call is guaranteed to return the same value as the first.

**Expected:** compute `sampleActive` once per animation per frame and
reuse the result in the overlay phase.

**Fix (implemented):** added `sampleActiveCached`, a thin wrapper around
`sampleActive` that caches `localT` and `finished` keyed on `elapsed`.
The cache fields (`cachedSampleElapsed`, `cachedLocalT`, `cachedFinished`)
are `mutable` on `ActiveCssAnimation` because `sampleActive` is logically
a read-only query. There's no frame counter: the elapsed time itself is
the cache key, so the cache is invalidated automatically whenever the
animation advances, and it survives across frames if `dt == 0`.

**Benchmark:** `micro.tickCss.100` improved by 9.1% (10.06 → 9.14 µs).
`system.frame.animated100` improved by 1.8% (275 → 270 µs).

- [x] Add `cachedSampleElapsed` / `cachedLocalT` / `cachedFinished` to `ActiveCssAnimation`
- [x] Add `sampleActiveCached` wrapper in `AnimPrimitives.hpp`
- [x] Use `sampleActiveCached` in `tickIncremental`, `tickAbsolute`,
      `overlayCssStyle`, `overlayCssComputed`
- [x] Add `micro.tickCssAndOverlay.100` benchmark
- [x] Add regression tests in `test_animations.cpp`
- [x] Update `internals/04-animation-system.md`

### 4.5 Optimize `isInheritedProp`

**Severity:** low — measurable on style resolution, not blocking.

**Location:** `src/ui/Style.hpp`, `isInheritedProp`.

**Problem:** `isInheritedProp` is called once per non-set property per
call to `ComputedStyle::from` when a parent is present. For a typical
style resolution, that's ~40 calls per node per frame. Each call is a
chain of up to 5 string comparisons:

```cpp
return name == "font" || name == "fontSize" || name == "color"
    || name == "letterSpacing" || name == "textAlign";
```

For the ~40 non-inherited properties, which is the vast majority, this
walks the entire chain and returns false.

**Expected:** short-circuit on a property's string length, since the
five inherited names have unique lengths (4, 5, 8, 9, 13).

**Fix (implemented):** dispatch on `name.size()` with a switch, then
do a single string comparison inside each case:

```cpp
inline bool isInheritedProp(std::string_view name)
{
    switch (name.size()) {
        case 4:  return name == "font";
        case 5:  return name == "color";
        case 8:  return name == "fontSize";
        case 9:  return name == "textAlign";
        case 13: return name == "letterSpacing";
        default: return false;
    }
}
```

For a property with a length that doesn't match any of the five (e.g.
`width` → 5, `padding` → 7, `background` → 10), the switch returns false
after a single integer comparison. For a property with a matching
length (e.g. `width` → 5, `minWidth` → 8, `minHeight` → 9), the single
string comparison discriminates correctly.

**Benchmark:** `micro.ComputedStyle.from_with_parent` added to
isolate the cost of the parent branch (the pre-existing
`micro.ComputedStyle.from` passes `nullptr` as parent, so it never
exercised `isInheritedProp`).

- [x] Add `micro.ComputedStyle.from_with_parent` benchmark
- [x] Rewrite `isInheritedProp` with a length-based switch
- [x] Add unit tests for the length-collision cases
- [x] Benchmark (measure improvement)
- [x] Update `api/Style.md` §"Inheritance"

### 4.6 Vector resize audit

`Layout::drawChildren` uses three member vectors (`drawNegZ_`,
`drawNormal_`, `drawPosZ_`). They grow to the max sibling count and
then stabilize. Verify with a benchmark that no per-frame allocation
happens after warmup.

- [ ] Benchmark `drawChildren` allocation behavior
- [ ] If allocations persist, pre-reserve based on `children.size()`

### 4.7 Build system: `-O2` by default

**Severity:** critical — all previous benchmark numbers were measured
against an unoptimized library.

**Location:** `Makefile` (repository root).

**Problem:** the library was compiled with `-ggdb` only, no `-O2`. The
benchmarks linked the debug library, so every measurement was inflated
by 3–10x. This was discovered when `cf17e3dc` (the commit that added
the 4.4 cache) showed every single benchmark jumping by ~2x compared
to the previous commit — a pattern too uniform to be a code
regression, and specifically impossible for benchmarks like
`micro.tree.addChild.1000` that touch no animation code.

**Fix (implemented):** the default `BUILD` profile is now `release`
(`-O2 -ggdb`). `make BUILD=debug` restores the old behavior for
interactive debugging. The fix was verified by rebuilding locally
and observing 50–90% improvements across every benchmark dominated by
framework code.

**Benchmark deltas after the fix (local):**

| Benchmark                    | Before | After | Δ |
|------------------------------|--------|-------|---|
| `micro.tickCss.100`          | 10.06  | 0.91  | -91% |
| `micro.tickImperative.100`   | 198.8  | 41.5  | -79% |
| `system.frame.static100`     | 164.4  | 42.96 | -74% |
| `system.frame.200rules`      | 1632.8 | 295.6 | -82% |
| `system.frame.animated100`   | 275.0  | 114.9 | -58% |
| `system.frame.large1000`     | 1653.1 | 500.8 | -70% |

- [x] Change `Makefile` default to `-O2 -ggdb`
- [x] Add `BUILD=debug` opt-out
- [x] Regenerate `benchmarks/baseline.json`
- [x] Update `internals/11-benchmarks.md` with a note on build profiles

---

## Phase 5 — Feature completion

Not blocking for beta, but needed for 1.0.

### 5.1 Text input (advanced)

- [ ] Selection (shift+arrow, shift+home/end)
- [ ] Cut / copy / paste (`Ctrl+C`, `Ctrl+V`, `Ctrl+X`)
- [ ] Select all (`Ctrl+A`)
- [ ] Undo / redo (`Ctrl+Z`, `Ctrl+Y`)
- [ ] Click-to-deselect
- [ ] Double-click to select word
- [ ] Triple-click to select all

### 5.2 Font handling

- [ ] Font fallback chain (e.g. `font: mont, calibri, sans`)
- [ ] Runtime font loading / registration
- [ ] Font metrics caching

### 5.3 `calc()` extensions

- [ ] `min(a, b)` / `max(a, b)` / `clamp(min, val, max)`
- [ ] `round()`, `floor()`, `ceil()`
- [ ] `mod()`

### 5.4 Filter and effect chaining

Two separate limitations:

**5.4a — Filter-to-filter chaining.** Currently each filter in a
`filter:` list reads the **original** `layerTarget_`, not the output
of the previous filter. `filter: blur(2px), sepia(0.6)` produces "sepia
on the original" instead of "sepia of blur". Only the last filter's
output is visible on the framebuffer.

Fix:
- Ping-pong between `layerTarget_` and `layerScratch_` (already both
  exist, both sized to `region`).
- Update `applyFilter` to accept a "current input target" parameter.
- `SinglePass` and `Silhouette` write to the next target instead of
  the framebuffer; the final pass writes to the framebuffer.
- `Separable` already ping-pongs internally; needs to read from the
  current input and leave the result in the next target.
- Add tests.
- Update `user/09-effects.md` §1.5 and `internals/06-render-pipeline.md`.

**5.4b — `effect:` and `filter:` on the same node.** When both are set,
the effect shader is pushed inside the layer target (applies to each
draw call), then the filter chain processes the composited result.
The visual outcome is "effect applied N times (once per draw call),
then filter", not "effect on the composited subtree, then filter".

Options:
- Leave as is, document the behavior explicitly in `09-effects.md`.
- Route `effect:` through the layer pipeline too (one extra pass in the
  target) so it operates on the composited image like a `filter:`.
- Unify both: make `effect:` a special case of `filter:` with a
  single-pass pipeline.

The unification (option 3) is the cleanest but the largest change.
It would collapse the two properties into one and simplify the mental
model — at the cost of a breaking change to the `.zstyle` surface.

- [ ] Decide between documenting or unifying
- [ ] Implement
- [ ] Add tests
- [ ] Update docs (`09-effects.md`, `06-render-pipeline.md`, `05-zstyle.md`)

### 5.5 Nested render targets

Currently the Raylib backend doesn't support nested targets, so a
filtered node inside a filtered node falls back to inline. To lift
this:

- [ ] Implement a target stack in the backend
- [ ] Update `isLayer` in `Layout::draw` to check
      `supports(Feature::NestedTargets)`
- [ ] Verify in tests
- [ ] Update documentation

### 5.6 Rotation in `drawLayer`

Compute the bounding box of the transformed rect including rotation.

- [ ] Apply `Transform2D::rotationDeg` to the four corners
- [ ] Compute the AABB
- [ ] Use it for the region
- [ ] Add test

### 5.7 `Popup` / `Dropdown` open/close callbacks

- [ ] `Popup::onOpen`
- [ ] `Dropdown::onOpenChanged`
- [ ] `Modal::onShow` / `onHide`

### 5.8 `Layout::hasClass`, `Layout::hasId`

- [ ] Simple accessors

### 5.9 Vector accessor for callbacks

Currently callbacks are individual `std::function` fields. A future
version could expose a vector for multi-listener patterns. Not critical
but sometimes needed.

- [ ] Decide if needed
- [ ] If yes, add `addOnClickListener` etc.

### 5.10 Live reload for development

- [ ] Watch `.zstyle` files, reload on change
- [ ] Watch `.zmarkup` files, reload on change (requires rebuild of
      the tree)
- [ ] Hot-reload integration with Raylib

---

## Phase 6 — Internationalization

Needed for non-English UIs.

### 6.1 Unicode-aware text

Currently `Text` and `TextInput` are `char`-based. Unicode support
requires:

- [ ] `std::u32string` or UTF-8 aware iteration in `Text`
- [ ] Cluster-aware cursor movement (grapheme boundaries)
- [ ] `TextInput` accepting multi-byte characters
- [ ] Text measurement per-glyph via the backend

### 6.2 RTL layout

- [ ] Direction detection (LTR / RTL) per text run
- [ ] Mirroring of `HStack` when direction is RTL
- [ ] `text-align: start` / `end` respecting direction
- [ ] Bidi algorithm for mixed runs (via a library)

### 6.3 IME support

- [ ] `IPlatform` extension for IME composition events
- [ ] `TextInput` rendering the composition string with an underline
- [ ] Backend-specific IME integration (Raylib doesn't have it natively)

---

## Phase 7 — Tooling

Not needed for the framework itself, but for adoption.

### 7.1 Documentation

- [ ] `docs/SUMMARY.md` — a navigable table of contents
- [ ] Review pass to remove redundancy between `user/` and `api/`
- [ ] Standardize examples (consistent output API, consistent style)
- [ ] Add a "cheatsheet" page for `.zstyle` and ZMarkup
- [ ] Tutorial: build a complete app in 30 minutes

### 7.2 Examples

- [ ] `examples/hello-world/` — minimal
- [ ] `examples/settings-screen/` — a realistic settings panel
- [ ] `examples/game-menu/` — a game menu with animations
- [ ] `examples/editor/` — a text-heavy UI (demonstrates wrapping,
      scroll, TextInput)

### 7.3 Inspector

A runtime inspector (like React DevTools):

- [ ] Tree view with live rects and states
- [ ] Style inspector: matching rules, resolved properties
- [ ] Live editing of inline styles
- [ ] FPS / frame time graph
- [ ] Would likely require a separate render target and its own UI

### 7.4 Formatter

- [ ] `.zstyle` formatter (indentation, blank lines, sorted
      properties)
- [ ] ZMarkup formatter (indentation)
- [ ] Integration with common editors (VS Code extension)

### 7.5 Language server

- [ ] Syntax highlighting for `.zstyle` and ZMarkup
- [ ] Autocomplete for property names, tag names, class names
- [ ] Go-to-definition for keyframes, custom properties
- [ ] Diagnostics for unknown properties, typo'd names

---

## Phase 8 — Beyond 1.0

Long-term ideas, not committed.

- [ ] Multi-window support (requires refactoring `UIContext` away from
      singleton)
- [ ] GPU-accelerated transitions (currently CPU-interpolated)
- [ ] Retained-mode scene graph caching
- [ ] Accessibility tree (screen reader support)
- [ ] Native platform integration (macOS menu bar, Windows title bar)
- [ ] Web build (WASM) via an OpenGL ES backend
- [ ] Mobile-specific widgets (safe area layout, on-screen keyboard
      integration)
- [ ] A visual editor for `.zmarkup` + `.zstyle`

---

## Known issues not yet prioritized

These are documented in the code or in the documentation but haven't
been turned into roadmap items. They're either rare, low-impact, or
require a bigger design decision.

| Issue | Location | Notes |
|-------|----------|-------|
| `collectFocusables` doesn't skip disabled subtrees | `Layout.cpp` | Tab reaches disabled containers' children. |
| `Text` is `char`-based, not Unicode | `Text.hpp` | Text input and rendering are ASCII-only. |
| Font fallback chain missing | `IAssetProvider` | Missing glyphs use the default font. |
| `var()` in `transition:` / `animation:` shorthand | `StyleParser.hpp` | `unresolvedProps` doesn't apply to those. |
| Custom props not in `ComputedStyle::operator==` | `Style.hpp` | Two styles differing only in custom props compare equal. |
| `pointerCapture` and `focusedNode` are weak, but `topmostConsumer` etc. are raw | `UIContext.hpp` | Raw pointers can dangle in theory. |
| `Style` macro-driven is verbose | `Style.hpp` | Adding a property touches ~1 line but generates cryptic errors. |
| `renderFrame` doesn't clear the framebuffer | `Layout.cpp` | User is responsible. |
| `Popup` `closeOnEscape` doesn't reach the popup if focus is elsewhere | `Popup.hpp` | Known limitation. |
| `Dropdown` doesn't close on `Escape` | `Dropdown.hpp` | Only closes on outside click. |

---

## How to contribute

See `CONTRIBUTING.md` (to be written). In the meantime:

- **Bug reports** should include a minimal reproduction using the
  `Mocks.hpp` fixture when possible.
- **Feature requests** should reference a concrete use case.
- **Pull requests** should include tests for new behavior.
- **Documentation changes** should follow the existing structure
  (`user/`, `internals/`, `api/`).

---

## Version history

| Version | Date | Notes |
|---------|------|-------|
| alpha | — | Initial state. API unstable. |
| beta | 2026-10-11 | Phase 1–3 complete. API frozen for the core. |
| 1.0 | TBD | Phase 4–5 complete. Feature-complete for typical use. |

---

## See also

- [README](README.md) — overview and quick start.
- [Getting Started](user/01-getting-started.md) — first steps.
- [Architecture](internals/01-architecture.md) — how the framework is
  built.
- [Debugging](internals/10-debug.md) — how to inspect runtime state.
