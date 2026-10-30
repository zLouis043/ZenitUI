# ZenitUI — Roadmap

This document tracks the path from the current **alpha** state toward a
**beta** release and, eventually, **1.0**. It's both a plan and a
snapshot of the current state: each section lists what's done, what's
pending, and what's blocking.

It's derived from a full code + documentation review. The **blocking**
items (bugs, missing API, critical test gaps) are prioritized. The
**nice-to-have** items are collected at the end.

Last updated: after the per-button input refactor (§5.15).

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

### 1.10 `pressTarget` dangling after `cullRemovedChildren`

**Severity:** high — crashes on the common pattern "onClick removes
its own item from a list".

**Location:** `src/ui/Layout.hpp`, `Layout::~Layout()`.

**Problem:** during `updateTree`, `UIContext::pressTarget` (and
`releaseTarget`, `topmostConsumer`, `hoverTarget`) are set at the
start of the frame and reset only at the end of `updateTree`. During
the recursive `update`, a node can be removed from the tree (via
`cullRemovedChildren`, after `removeFromParent()`), which frees it.
If the freed node was in one of those four pointers, any later
ancestor's `fireCallbacks` — which calls
`node.isAncestorOf(ctx.pressTarget)` — dereferences a dangling
pointer.

The pattern triggers whenever `onClick` calls `removeFromParent()`
on the clicked node (or on one of its ancestors), and a higher
ancestor of the removed node still runs `fireCallbacks` in the same
frame.

**Fix:** in `~Layout()`, reset any of the four context pointers
that equal `this`. A destroyed node leaves the context in a
consistent state, so the ancestor's check becomes a no-op.

- [x] Clear `pressTarget` / `releaseTarget` / `topmostConsumer` / `hoverTarget` in `~Layout` if they equal `this`
- [x] Add regression test in `test_interaction.cpp`
- [x] Update `internals/07-input-system.md` §10 (new pitfall: "the context pointers are cleared in `~Layout`")

### 1.11 Container doesn't shrink after a child is removed

**Severity:** medium — visible stale layout in scroll containers whose
content shrinks.

**Location:** `src/ui/Layout.hpp`, `Layout::cullRemovedChildren` and
`Layout::recomputeDirty`; `NodeInternal` bitfield.

**Problem:** when a child is removed (`removeFromParent` followed by
`cullRemovedChildren`), no dirty flag is set on the parent. So
`recomputeDirty` sees an unchanged subtree and `measure` short-circuits
on the next frame. The parent's `contentSize` and any scroll
container's `maxScroll` stay at their pre-removal values. Visible as
"the scrollbar won't disappear until the last child is removed" —
the container stays at its maximum height and keeps a scrollbar even
after the content shrinks well below it.

**Fix:** added a per-node `internal_.hadRemovedChildren` bit. Set by
`cullRemovedChildren` when at least one child is actually erased.
Consumed (OR'd and cleared) by `recomputeDirty`, which forces
`subtreeDirty = 1` on the frame of the removal. The flag propagates
upward through the normal `anyChildDirty` check.

- [x] Add `hadRemovedChildren` to `NodeInternal`
- [x] Set the flag in `cullRemovedChildren` when a child is erased
- [x] OR the flag into `recomputeDirty` and clear it
- [x] Add regression test in `test_interaction.cpp`
- [ ] Update `internals/05-layout-engine.md` §7 (dirty tracking) to
      mention the "removed children" bit

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
`drawNormal_`, `drawPosZ_`) to partition children by stacking context
before drawing. They're reused across frames (cleared with `.clear()`,
which preserves capacity) and should reach a steady state after the
first frame with the maximum child count.

**Verification:** added `micro.tree.drawChildren.500` — a root with
500 children, all with explicit z-index, so they land in the `posZ` /
`negZ` buckets. The benchmark measures pure `draw()` cost.

**Result:** 37.1 µs with sd 1.2 µs (3%). Stable timing indicates no
per-frame allocation. `clear()` + `push_back` reaches capacity within
the first run and stays there. No code change required.

- [x] Add `micro.tree.drawChildren.500` benchmark
- [x] Verify no per-frame allocation
- [x] Document the "grow-once, then reuse" pattern in
      `internals/06-render-pipeline.md`

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

✅ **Done.** `TextInput` now supports selection, text replacement, and integration with the system clipboard via `IPlatform`. Mouse clipping restricts overflow. The Undo/Redo history stack and double/triple click selection have been implemented.

- [x] Selection (shift+arrow, shift+home/end, mouse drag)
- [x] Cut / copy / paste (`Ctrl+C`, `Ctrl+V`, `Ctrl+X`)
- [x] Select all (`Ctrl+A`)
- [x] Click-to-deselect / replace selection on type
- [x] Undo / redo (`Ctrl+Z`, `Ctrl+Y`) 
- [x] Double-click to select word 
- [x] Triple-click to select all

### 5.2 Font handling

- [ ] Font fallback chain (e.g. `font: mont, calibri, sans`)
- [ ] Runtime font loading / registration
- [ ] Font metrics caching

### 5.3 `calc()` extensions

- [x] `min(a, b)` / `max(a, b)` / `clamp(min, val, max)`
- [x] `round()`, `floor()`, `ceil()`
- [x] `mod()`

### 5.4 Filter and effect chaining

Two separate limitations:

**5.4a — Filter-to-filter chaining.** ✅ Done.

Each filter in a `filter:` list now reads the previous filter's output
instead of the original `layerTarget_`. `filter: blur(2px), sepia(0.6)`
composes as `sepia(blur(original))`, matching CSS's order semantics.

The chain uses a ping-pong between `layerTarget_` and `layerScratch_`:
each filter reads from one target and writes to the other, alternating
per step. The last filter writes to the framebuffer with the node's
opacity applied; intermediate writes use full opacity to avoid
compounding alpha.

A third target (`layerSeparable_`) was added to `Layout` for `Separable`
patterns (blur): their H-pass intermediate would otherwise overwrite
one of the two ping-pong targets mid-computation. It's allocated
lazily, with the same dimensions as the other two.

`FilterContext` gained a `write_target` field. A filter that receives
a valid `write_target` renders into it; a filter that receives an
invalid one is the last in the chain and writes to `ctx.region` with
`ctx.opacity` applied. Custom filters must honor this contract.

- [x] Add `write_target` to `FilterContext`
- [x] Update `applySinglePass` / `applySeparable` / `applySilhouette`
      to write to `write_target` when valid
- [x] Add `layerSeparable_` to `Layout` (allocated lazily in `drawLayer`)
- [x] Rewrite the chain loop in `drawLayer` to ping-pong between the
      two main targets
- [x] Extend `MockRenderer` to support effects and count `pushTarget`
- [x] Add `Filter_chaining.two_filters_ping_pong_through_targets` test
- [x] Verify visually in `examples/07_effects` (three new cards:
      blur → sepia, blur → sepia → drop-shadow, drop-shadow → blur)
- [x] Update `user/09-effects.md` §1.5, §3.3, §7
- [x] Update `internals/06-render-pipeline.md` §4.5 and Pitfalls
- [x] Update `api/FilterRegistry.md` §6 and §7

**5.4b — `effect:` and `filter:` on the same node.** ✅ Done (documentation).

The two properties operate at different pipeline levels:

- `effect:` pushes a shader around the node's own draw calls. Each
  primitive is processed individually — the shader does not see the
  composited subtree.
- `filter:` renders the subtree into a target and processes the
  composited image.

When both are set, the effect runs first (during the subtree render
into the layer target, once per draw call), then the filter chain
runs on the composite. This is intentional: the two serve different
purposes and combining them is valid but rare.

Decision: document rather than unify. Unifying would collapse the
two properties into one at the cost of a breaking change to the
`.zstyle` surface, and would lose the ability to express per-primitive
shaders. Not worth it for a rare use case.

- [x] Document the behavior in `user/09-effects.md` §7.5
- [x] Explain the pipeline order, when each is appropriate, and how
      to get "one shader on the composite" semantics via a `Custom`
      filter
- [x] Note the cost when both are used together
- [ ] (Deferred) If per-primitive shader needs grow, revisit and
      consider making `effect:` a shorthand for a single-pass
      `Custom` filter that operates on the composite

A future revisit could unify `effect:` as a shorthand for a
single-pass `Custom` filter, but the current split is preserved
because it covers the per-primitive use case cleanly.

### 5.5 Nested render targets

✅ **Done.** `RaylibRenderer` now maintains a stack of active render targets and OpenGL matrices, allowing filtered nodes to be nested inside other filtered nodes seamlessly.

- [x] Implement a target stack in the backend
- [x] Update `isLayer` in `Layout::draw` to check `supports(Feature::NestedTargets)`
- [x] Verify in tests
- [x] Update documentation

### 5.6 Rotation in `drawLayer`

The transformed bounding box used for the filter region now accounts
for rotation, not just scale and translate. The four corners of `rect`
are transformed by `applyTransform` (translate, scale, rotate around
the node's center), then the AABB is taken from the four results.

This matches the same transform used by `transformClipToScreen` (in
the backend) and by `hitTest` in its inverse form, so "the node's
transformed area" is computed consistently everywhere.

- [x] Apply `Transform2D::rotationDeg` to the four corners
- [x] Compute the AABB
- [x] Use it for the region
- [x] Add test (`Filter_rotation.region_covers_rotated_bbox`)
- [x] Remove the "rotation ignored" pitfall from internals/06-render-pipeline.md

### 5.7 `Popup` / `Dropdown` open/close callbacks

- [x] `Popup::onOpen`
- [x] `Dropdown::onOpenChanged`
- [x] `Modal::onShow` / `onHide`

### 5.8 `Layout::hasClass`, `Layout::hasId`

- [x] Simple accessors

### 5.9 Vector accessor for callbacks

Currently callbacks are individual `std::function` fields. A future
version could expose a vector for multi-listener patterns. Not critical
but sometimes needed.

- [ ] Decide if needed
- [ ] If yes, add `addOnClickListener` etc.

### 5.10 Live reload for `.zstyle` — delegated to the consumer

**Moved out of the library scope.**

Hot reload is a consumer concern, not a library concern. ZenitUI
provides all the primitives a project needs to implement it externally:

- `Theme::get().clear()` resets the loaded stylesheets.
- `ZMarkup::loadStyleFile(path)` reloads them.
- `UIContext::get().invalidateStyles()` (added in 5.10a, see below)
  forces every node to re-resolve on the next frame.

The decision of *when* to reload (file mtime polling, native file
watchers, editor integration) and *what* to reload (all stylesheets,
just the one that changed) belongs to the project. The project for example
implements its own in `src/core/StyleHotReload.hpp`.

This is the pattern for future proposals: if the feature only makes
sense for one kind of consumer, it stays out of the library.

### 5.10a `UIContext::invalidateStyles()` — small helper

Currently the only way to force a full style re-resolve is to bump
`UIContext::viewportGeneration` manually:

```cpp
UIContext::get().viewportGeneration++;
```

This works, but it's using a media-query counter for a purpose it
wasn't designed for. The intent isn't obvious from the call site.

Add a `void invalidateStyles()` method that bumps the counter (and
does nothing else for now, but is the natural place for future
invalidation logic):

```cpp
void invalidateStyles() { viewportGeneration++; }
```

Callers become self-documenting:

```cpp
Theme::get().clear();
ZMarkup::loadStyleFile("assets/game.zstyle");
UIContext::get().invalidateStyles();
```

- [x] Add `UIContext::invalidateStyles()`
- [x] Update `api/UIContext.md`
- [x] Update the live-reload example in `user/05-zstyle.md` (if it exists)

### 5.11 `Layout.hpp` restructuring

`Layout.hpp` is the largest single header in the project. It declares
~50 public methods, ~20 protected boolean flags, ~20 protected fields,
four `friend struct` declarations, and the entire CRTP helper
`TLayout<Derived, Base>`. Every widget includes it, so it's on the
critical path for compile time.

Four strategies have been identified, in increasing order of scope and
risk. They're listed here for context — not all of them should be
adopted.

#### 5.11a — Group state flags into a bitfield struct

Today, the ~20 boolean flags on `Layout` are declared individually:

```cpp
bool isInteractive{true}, blocksRaycast{false}, isHovered{false},
     wantsRemoval{false}, pendingTransition{true}, isEnabled{true};
bool isFocused{false};
bool isFocusable_{false};
// ... etc
```

Grouping them into a single bitfield struct would:

- Cut ~20 bytes per node (marginal but real on large trees).
- Remove ~20 lines from the header.
- Make the state of a node readable at a glance.

The caveat: bitfield layout is implementation-defined by the ABI, and
you can't take the address of an individual bitfield member. The
current code reads flags by value (`node.isHovered`, `node.getEnabled()`)
and only ever takes the address of the whole `Layout` (e.g.
`ctx.hasFocus(&node)`), so the conversion is technically compatible.
But it needs careful testing, especially for anything that relies on
`memcmp` / `operator==` on a `Layout` (unlikely, but worth checking).

Priority: medium. Not blocking, low risk if tested.

#### 5.11b — Event listener array (5.9)

See 5.9. Already planned.

#### 5.11c — Remove `friend` declarations

The four subsystems (`StyleResolver`, `AnimationPlayer`,
`InputController`, `ScrollController`) are `friend`s of `Layout` so
they can write directly into its private state during the frame.

Removing the `friend`s would force the subsystems to operate on an
explicit state struct, with `Layout` applying the result:

```cpp
auto newState = input_.computeState(rect, ctx.pointer);
if (newState.isHovered != flags.isHovered) {
    flags.isHovered = newState.isHovered;
    flags.pendingTransition = true;
}
```

This is cleaner on paper. But it costs: one struct construction +
return + copy per subsystem per node per frame. On `system.frame.large1000`
(1000 nodes), that's measurable. The `friend` approach is what keeps
the per-node update path tight.

**Decision: not recommended.** The `friend` declarations are a
deliberate design choice, not a smell. If future profiling shows the
subsystems are hot spots, optimize them within the current structure
instead of paying the composition cost.

#### 5.11d — Split into `DOMNode` / `StyledNode` / `Layout`

The "fat node" pattern is what every retained-mode UI framework ends
up with (HTMLElement in the browser, Node/Node2D in Godot, etc.). A
three-level hierarchy would separate:

1. `DOMNode` — parent, children, `addChild`, `removeFromParent`,
   `nodeId`, event listeners.
2. `StyledNode : DOMNode` — classes, tag, `StyleResolver`,
   `AnimationPlayer`.
3. `Layout : StyledNode` — `rect`, `measure`, `arrange`, `draw`,
   `ScrollController`.

This is the "correct" architecture for a large framework. But:

- It breaks the `TLayout<Derived, Base = Layout>` CRTP pattern. Every
  widget would need to pick its base explicitly.
- It breaks the public API. Code that does `node->size(...)` would
  need to know whether `size` lives on `StyledNode` or `Layout`.
- It changes `friend` targets and every existing include.

**Decision: deferred to post-1.0.** Not worth the churn while the
framework is still in beta. Revisit when:
- Compile time becomes a real problem (>30s incremental builds on a
  typical machine).
- Multiple stakeholders need to work on style / layout / DOM in
  parallel without stepping on each other.
- A plugin system needs to expose "just the tree" without the drawing
  half.

- [ ] 5.11a — Group state flags into a bitfield struct (low risk)
- [ ] 5.11b — Listener array (tracked in 5.9)
- [x] 5.11c — Remove `friend` — declined, deliberate design choice
- [ ] 5.11d — Split hierarchy — deferred to post-1.0

### 5.12 ZMarkup / ZStyle ergonomics

Four small gaps that made the DSL awkward for anything beyond the
simplest markup. All resolved.

**5.12a — Canvas / Modal / Popup in `registerBuiltins`.** ✅

Previously, using these three widgets from ZMarkup required building a
custom `Registry` in user code, for no reason. They're regular widgets
with no constructor arguments, so they belong in the default registry.

`Tooltip` (needs an owner) and `ImageContainer` (needs a texture) stay
unregistered — those genuinely can't be expressed by the grammar.

**5.12b — `UINode::context()`.** ✅

`UINode` held a `BuildContext` internally but didn't expose it. Any
user who wanted to look up nodes by id from outside of `find()`, or
who wanted to extend a tree at runtime with the same registry, had to
re-run `parse` + `Registry` + `create` manually. `context()` returns
the shared pointer directly.

**5.12c — `rgb()` / `rgba()` in `.zstyle`.** ✅

The color parser handled named colors and hex only. `rgb(r, g, b)`
and `rgba(r, g, b, a)` are now recognized, with values clamped to
their valid ranges. Components follow the CSS convention: 0–255 for
the color channels, 0.0–1.0 for alpha.

**5.12d — `ZMarkup::loadMarkupFile`.** ✅

Symmetric to `ZMarkup::loadStyleFile`. Reads a `.zmarkup` source from
a file and returns a `UINode`. Missing file → null root + error log.

- [x] 5.12a — Register Canvas / Modal / Popup
- [x] 5.12b — Expose `UINode::context()`
- [x] 5.12c — Support `rgb()` / `rgba()`
- [x] 5.12d — Add `loadMarkupFile`

### 5.13 Content slot for composite widgets

**Done for `Modal`. Others left as default.**

Composite widgets build internal structure in `onBuild()`. When ZMarkup
adds children to such a widget, those children used to become siblings
of the internal content wrapper instead of children of it. This broke
the wrapper's animations — the most visible case was `Modal`, whose
"Intro" slide didn't apply to markup children.

**Design (implemented):** a virtual `Layout::contentTarget()` returning
the node children should be attached to. Default: `this`. `Modal`
overrides it to return its internal `content` VStack. `ZMarkup::create`
now attaches children via `contentTarget()` instead of calling
`addChild()` directly.

| Widget | Overrides `contentTarget()`? | Why |
|--------|------------------------------|-----|
| `Layout`, `Panel`, `VStack`, `HStack`, `Stack`, `Text`, `Button`, `Checkbox`, `Slider`, `ProgressBar`, `TextInput`, `ScrollView`, `Canvas` | No (default `return this`) | No internal wrapper |
| `Modal` | Yes → `content` VStack | Children need to live inside the animated wrapper |
| `Popup` | No | Content is the direct children; no wrapper |
| `Dropdown` | No | Internal structure (trigger + list) isn't a content slot |
| `Toggle` | No | Knob is decorative; children are unusual and default behavior is documented |

- [x] Add `Layout::contentTarget()` virtual, default returns `this`
- [x] Override in `Modal`
- [x] Update `ZMarkup::create` to use `contentTarget()`
- [x] Add tests (3: Modal-with-children, Stack-with-children, nested layout)
- [x] Update `user/04-zmarkup.md` §2.6
- [x] Update `api/Layout.md` §5
- [x] Update `api/Widgets.md` (`Modal` section)

If a future widget needs its own content slot, the override is a
one-liner. The pattern is settled.

### 5.14 CanvasLayout respects `effect:`

**Done.**

`CanvasLayout` is the only widget that overrides `Layout::draw()`
completely, because it renders its subtree into an offscreen target
and composites the result. The consequence was that `effect:` in
`.zstyle` never reached the canvas — the effect was resolved during
`Layout::draw()`, which the canvas bypasses. Binding a shader required
`setShader()` from C++, even though the docs mark that API as a
legacy path.

Fix: `Layout` gains a protected helper `resolveActiveEffect()` that
returns the effect to apply, with a documented priority:

1. `effect:` from the current style (if resolved to a valid handle).
2. `setShader()` handle (if set).
3. Invalid handle (no effect).

`CanvasLayout::draw()` calls `resolveActiveEffect()` and pushes the
returned handle around the final `drawTarget`. Other widgets are
unaffected — they go through `Layout::draw()`, which already calls
`resolveEffectIfNeeded` and pushes `currentEffect_` in the same
priority order.

- [x] Add `Layout::resolveActiveEffect()` (protected)
- [x] Use it in `CanvasLayout::draw()`
- [x] Add `pushEffectCount` to `MockRenderer`
- [x] Add test (`Canvas.effect_property_pushes_shader`)
- [x] Update `api/Widgets.md` CanvasLayout section
- [x] Update `api/Layout.md` §14 with the new hook

### 5.15 Unified per-button input

**Done.** The input system used to have per-button hardcoded fields on
`PointerState` (`down`, `pressed`, `released`, `rightDown`,
`rightPressed`, `rightReleased`), separate consumption flags on
`UIContext` (`clickConsumed`, `rightClickConsumed`), and a separate
dispatch path for Left (with `onClick`/`onPress`/`onRelease`) and
Right (with `onRightClick`). Middle was not supported at all.
Extending to a new button required touching 5+ files with no
safety net.

The refactor replaces all of that with a single `MouseButton` enum
as the canonical index:

- `PointerState` exposes `std::array<ButtonState, (size_t)MouseButton::Count> buttons`
  with an `operator[]`. Each `ButtonState` carries `down` /
  `pressed` / `released`.
- `UIContext` exposes
  `std::array<bool, (size_t)MouseButton::Count> buttonConsumed`
  with `consumeClick(MouseButton)` / `wasConsumed(MouseButton)`
  helpers.
- `InputController::fireCallbacks` unifies Left and Middle through a
  `fireClickButton(MouseButton, isLeftLegacy)` lambda. Right stays on
  its own block because its semantic (one-shot on press) is different.
- `Layout` gains `onButtonPress`, `onButtonRelease`, `onButtonClick`,
  each `std::function<void(MouseButton)>`. They fire for Left and
  Middle. The legacy `onClick` / `onPress` / `onRelease` still fire
  for Left, so existing code is unaffected.

**Design decisions:**

- **Right is deliberately not unified.** Its historical semantic is
  "fire on press, one-shot" (context menus). Unifying it under the
  press+release click model would lose that behavior. If a future
  design wants the unified model for Right, it's a new decision, not
  a regression of this refactor.
- **Left fires both legacy and unified.** If a user sets both
  `onClick` and `onButtonClick`, both fire. This is the "both were
  explicitly requested" case, not a bug.
- **Middle fires only unified.** There is no legacy Middle callback,
  so `onButtonClick(MouseButton::Middle)` is the only way to react
  to it.

**Backward compatibility.** User code that uses only `onClick`,
`onPress`, `onRelease`, `onRightClick`, `pointer.pos`, `wheelY`,
`modifiers`, and `inputEvents` continues to work unchanged. The
breaking changes are limited to:

- `pointer.pressed` / `pointer.down` / `pointer.released` →
  `pointer[MouseButton::X].pressed` / `.down` / `.released`
- `pointer.rightPressed` / `rightDown` / `rightReleased` → same
  pattern with `MouseButton::Right`
- `ctx.clickConsumed` / `ctx.rightClickConsumed` →
  `ctx.wasConsumed(MouseButton::X)`
- `ctx.consumeClick()` / `ctx.consumeRightClick()` →
  `ctx.consumeClick(MouseButton::X)`

**Extending to a new button is now a 2-step change:** add an enum
value before `MouseButton::Count`, then populate it in the backend's
`IPlatform::pointer()`. The framework propagates it automatically
(array sizes, `anyPressed`/`anyReleased`/`anyDown` loops, per-button
consumption flags). See `internals/07-input-system.md` §9.2.

**Cleanup during the refactor:**

- `examples/09_custom_widget/Rating.hpp` had **two** bugs:
  `pointer.pressed` (introduced by the refactor) and
  `pendingTransition = true` (pre-existing — the field moved to
  `internal_.pendingTransition` at an earlier flag refactor and the
  example was never updated). Both fixed.
- `examples/00_demo/SettingsScreen.hpp` had `pointer.pressed` in
  `onUpdate`. Fixed.

**Known limitation introduced:** `pressTarget` is a single pointer,
not per-button. A press on Middle overwrites `pressTarget` from a
still-held Left. Multi-button simultaneous interactions aren't
supported. In practice, mouse users don't hold two buttons at once,
and the limitation is documented in
`internals/07-input-system.md` §10.

- [x] Add `MouseButton` enum (`Left`, `Right`, `Middle`, `Count`) in `CoreTypes.hpp`
- [x] Rewrite `PointerState` with `std::array<ButtonState, N>` + `operator[]`
- [x] Add `buttonConsumed` array + `consumeClick` / `wasConsumed` to `UIContext`
- [x] Rewrite `InputController::fireCallbacks` with `fireClickButton` lambda
- [x] Add `onButtonPress` / `onButtonRelease` / `onButtonClick` to `Layout`
- [x] Rewrite `Layout::updateTree` with `anyPressed` / `anyReleased` / `anyDown`
- [x] Update `RaylibPlatform::pointer()` to populate all three buttons
- [x] Verify `tests/test_interaction.cpp` covers the new callbacks (5 tests: Middle press, Middle click, Left fires both legacy+unified, Right doesn't fire unified, Middle on empty area)
- [x] Verify `tests/Mocks.hpp` has `pressMiddle` / `releaseMiddle` / `holdMiddle` helpers
- [x] Update `docs/internals/07-input-system.md` (§1.1, §1.2, §2.1, §2.4, §2.5, §3.2, §3.3, §3.6, §5.3, §5.4, §7.3, §9.2, §10)
- [x] Update `docs/internals/02-lifecycle.md` (§3.2, §3.3, §3.6)
- [x] Update `docs/user/06-events.md` (§2, §3, §3.1, §4, §5, §6.1, §10.2, §14, §15)
- [x] Update `docs/user/10-custom-widgets.md` (§5.4, §8.1)
- [x] Verify `docs/api/Layout.md` already documents `onButton*` correctly (no change needed)
- [x] Verify `docs/api/UIContext.md` already documents `buttonConsumed` array correctly (no change needed)
- [x] Fix `examples/09_custom_widget/Rating.hpp` (2 bugs)
- [x] Fix `examples/00_demo/SettingsScreen.hpp` (1 bug)

### 5.16 `ref`: separation of style identity and logic lookup

**Done.**

Today `Layout::nodeId` (set by `#id` in ZMarkup) is the only way to
look up a node from C++. This conflates two distinct concepts:

1. **Style identity** — `#id` participates in CSS specificity as an
   id selector `(1, 0, 0)`, beating any class. It's meant for unique
   elements.
2. **Logic identity** — the string C++ uses to look up the node.

For a list of 100 delete buttons, each button needs a unique logic
handle but **not** a unique style id — they all want the same style.
The old pattern forced users to set `#delete_1`, `#delete_2`, ... on
each one, polluting the cascade and the `byId` map.

The fix introduces a separate **`ref`**:

- `Layout::ref` — public `std::string` field, populated from `@name`
  in ZMarkup.
- `Layout::setRef(const std::string&)` — setter.
- `Layout::withRef(const std::string&)` — fluent, on `TLayout`.
- `Element::ref` — populated by the ZMarkup parser from `@name`.
- `BuildContext::byRef` — `unordered_map<string, vector<shared_ptr<Layout>>>`,
  populated by `Registry::create`.
- `UINode::ref<T>(name)` — returns the first match, warns if not
  found or ambiguous.
- `UINode::refAll<T>(name)` — returns all matches, `{}` + debug log
  if not found.

`find` (by `#id`) is unchanged in behavior, except that it now logs a
warning (once per `UINode` handle, not per frame) when the id is not
found. Silent `nullptr` returns are gone for the common typo case.

**Design decisions:**

- **`ref` and `id` are orthogonal.** A node can have both
  (`Button#save@main_save`), only one, or neither. They map to
  different fields (`nodeId` and `ref`) and different maps (`byId`
  and `byRef`).
- **`@ref` is not a CSS hook.** `.zstyle` has no `@` selector, no
  `[ref="..."]` attribute selector. Styling a node by ref is not
  supported and not planned. Use classes for styling, refs for
  lookup.
- **`withRef` over `ref()`.** The fluent method would shadow the
  inherited public `ref` field if it were named `ref()`. `withRef`
  matches the `with(child)` pattern and is unambiguous.
- **Per-handle warning dedup.** `find`/`ref` warnings are deduplicated
  via a per-`UINode` `unordered_set<string>`, so a typo'd id in a
  loop doesn't spam the log. `refAll` on an empty list is `Debug`,
  not `Warning`, because empty lists are legitimate.
- **`@ref` in `on*` helpers deferred.** `ui.onClick("@delete", cb)`
  would attach the callback to every node with the ref. Useful but
  not required — deferred to a follow-up, since `@` in an id is
  currently undefined behavior, so the addition would be
  backwards-compatible.

**Backward compatibility.** `#id`, `find`, `byId`, and every CSS
selector involving `#id` are unchanged. The only behavior change is
the new warning on `find` misses. Code that ignored the `nullptr`
return and kept going still works — the warning is informational.

**Known limitation.** `ref` mutation after `Registry::create` does
not update `byRef`. A future `UINode::setRef` method would fix this;
not in the current scope.

- [x] Add `Layout::ref` public field
- [x] Add `Layout::setRef` / `TLayout::withRef`
- [x] Add `Element::ref` + parser `@name`
- [x] Add `BuildContext::byRef` + `Registry::create` population
- [x] Add `UINode::ref<T>` / `refAll<T>` + warning dedup
- [x] Add warning on `UINode::find<T>` miss
- [x] Update `Debug::dumpTree` / `dumpStyle` to show `@ref`
- [x] Add tests (14 new in `test_zmarkup_build.cpp`, 4 in `test_layout.cpp`)
- [x] Update `docs/user/04-zmarkup.md` (§2.3/§2.4 split, §3 table, §4.5, §10 pitfalls)
- [x] Update `docs/api/ZMarkup.md` (§2, §4, §5, §6, §10)
- [x] Update `docs/api/Layout.md` (§3, §12)
- [x] Update `docs/CHEATSHEET.md` (Id vs Ref table)
- [x] Update `docs/GLOSSARY.md` (Ref entry)

### 5.17 `Bindable<T>`: lightweight data binding

**Done.**

Wiring a model value to one or more widgets required a `std::function`
callback on each widget and manual capture of every widget that
displayed the value. For a settings screen with a volume slider
driving a progress bar and a percentage label, the boilerplate added
up fast, and a `shared_ptr` capture in the callback created a
reference cycle that kept widgets alive after the screen was closed.

`Bindable<T>` is a header-only observable value:

- Holds a `T` and notifies subscribers when the value changes.
- `set(v)` short-circuits when `v == current` — no notification, no
  assignment.
- `onChanged(cb, fireImmediately)` returns a `Subscription`.
- `bindTo(widget, &W::method, fireImmediately)` binds a widget method
  with lifetime safety (uses `weak_ptr` internally).
- `Subscription` is a move-only RAII token. Destroying it
  unsubscribes. It's safe for a `Subscription` to outlive its
  `Bindable` (via a shared `ControlBlock` whose `alive_` flag is
  cleared by `~Bindable`).

**Design decisions:**

- **`Subscription` instead of just `bindTo`.** `bindTo` handles the
  lifetime correctly, but the caller has to remember to use it. Making
  `onChanged` the only way to subscribe — and having it return a
  `Subscription` — prevents the "dangling capture" bug by design.
- **`bindTo` deduces `Arg` from the method pointer type.** ZenitUI
  widget setters take their argument either by value
  (`ProgressBar::setValue(float)`) or by const-ref
  (`Text::setText(std::string)`). C++ doesn't convert a method pointer
  `void(W::*)(float)` to `void(W::*)(const float&)`. Deducing `Arg`
  separately lets both forms work at the call site without a wrapper
  lambda.
- **`std::vector<Entry>` for subscribers, not `std::unordered_map`.**
  Preserves registration order (the standard guarantees it for
  vector, not for unordered_map) and the snapshot cost is
  proportional to N, which is small (1–5 in practice).
- **`Bindable` is non-copyable and non-movable.** A movable
  `Bindable` would have to rewire its subscriptions on every move, or
  share the `ControlBlock` — the first is expensive, the second is
  confusing. `unique_ptr<Bindable>` covers the rare "needs to move"
  case.
- **`ControlBlock` for lifetime safety.** A `Subscription` holds a
  `shared_ptr<ControlBlock>`; `~Bindable` sets `alive_ = false`. This
  makes "`Subscription` outlives `Bindable`" a no-op instead of a
  use-after-free. Cost: 16 bytes per subscription.
- **Single-thread only.** `Bindable<T>` is not thread-safe. The
  framework is single-thread; producers on background threads must
  post to the main thread before `set()`.

**Backward compatibility.** `Bindable<T>` is opt-in. No existing code
uses it. No existing code breaks. The `on*` callbacks on widgets
(`onValueChanged`, `onToggle`, …) are untouched — `Bindable` sits on
top of them, it doesn't replace them.

**Known limitation.** `T` must have `operator==`. This is used by
`set()` to detect real changes; a `T` without equality comparison
won't compile. For `float`, the usual floating-point equality caveat
applies, but UI use is fine.

- [x] Add `src/ui/Bindable.hpp` (header-only)
- [x] Add `#include "Bindable.hpp"` to `src/ui/UI.hpp`
- [x] Add `tests/test_bindable.cpp` (24 tests: construct, set, subscription RAII, recursion, bindTo, lifetime, type constraints)
- [x] Add `docs/user/11-bindable.md` (motivation, patterns, lifetime, pitfalls)
- [x] Add `docs/api/Bindable.md` (full method reference)
- [x] Update `docs/CHEATSHEET.md` (new "Data binding" section)
- [x] Update `docs/GLOSSARY.md` ("Bindable" entry)
- [x] Update `docs/README.md` and root `README.md` (link the new docs)
- [x] Update `docs/architecture/PROPOSALS.md` (mark Proposal 2 shipped)
- [x] Add `examples/12_bindable` — model → 3 widgets, `bindTo` lifetime safety demonstrated live (follow-up commit)

### 5.18 UIPrefab
✅ **Done.** `ZMarkup::registerPrefab` parses a ZMarkup string once. Instantiation clones the AST, merges root attributes (`.class`, `#id`, `@ref`, inline styles), and provides blazing fast reusable components.

### 5.19 `<Slot>`
✅ **Done.** The `Slot` tag inside a Prefab acts as an injection point for children passed during instantiation. Supports default fallback content.

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

### 7.6 — CI: build examples

**Severity:** medium — catches API drift in user-facing code that the
test suite doesn't exercise.

**Location:** `.github/workflows/ci.yml`.

**Problem:** the examples live in `examples/` and were not compiled by
CI. They exercise the public API in ways the tests don't:
`RaylibBackend.cpp` (the tests use mocks, so the real backend was
never compiled by CI), `Dropdown` / `Popup` / `Tooltip` / `Modal` /
`CanvasLayout`, the fluent API (`cls` / `id` / `size` / `with`), and
the ZMarkup → `UINode` → callback integration. A breaking change in
any of these passes CI silently — exactly what happened with the
per-button input refactor (§5.15), where two examples were left with
`pointer.pressed` and `pointer.transition` and nobody noticed until
a manual `make examples` was run.

**Fix:** add a `build-examples` job to `ci.yml`. It compiles every
example (`make -j4 examples`) without running them. It runs on
**one** row — `windows-latest` + MSYS2 MinGW — not the full test
matrix, because the errors it catches are API-drift errors which are
platform-independent.

- [x] Add `build-examples` job to `ci.yml` (windows-latest + MSYS2)
- [x] Verify it catches the `pointer.pressed` regression (revert one
      example locally, push, confirm CI fails)

**Long-term (separate item):** the `examples/Makefile` currently
hardcodes Windows link flags (`-lraylibdll -lopengl32 -lgdi32 -lwinmm`),
which is why the CI job runs on Windows. Making the Makefile
cross-platform (or replacing it with CMake, as already planned for
Phase 7) would let the job run on `ubuntu-latest` — faster, cheaper,
and more representative of the CI environment used for benchmarks.

- [ ] Make `examples/Makefile` cross-platform (OS detection)
- [ ] Move `build-examples` to `ubuntu-latest`
- [ ] Precompile `RaylibBackend.cpp` into an object file (currently
      recompiled once per example)

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
| `Text` is `char`-based, not Unicode | `Text.hpp` | Text input and rendering are ASCII-only. |
| Font fallback chain missing | `IAssetProvider` | Missing glyphs use the default font. |
| `var()` in `transition:` / `animation:` shorthand | `StyleParser.hpp` | `unresolvedProps` doesn't apply to those. |
| Custom props not in `ComputedStyle::operator==` | `Style.hpp` | Two styles differing only in custom props compare equal. |
| `pointerCapture` and `focusedNode` are weak, but `topmostConsumer` etc. are raw | `UIContext.hpp` | Raw pointers can dangle in theory. |
| `Style` macro-driven is verbose | `Style.hpp` | Adding a property touches ~1 line but generates cryptic errors. |
| `renderFrame` doesn't clear the framebuffer | `Layout.cpp` | User is responsible. |
| `Popup` `closeOnEscape` doesn't reach the popup if focus is elsewhere | `Popup.hpp` | Known limitation. |
| `pressTarget` shared across mouse buttons | `Layout.cpp`, `Input.cpp` | A press on Middle overwrites `pressTarget` from a still-held Left. Multi-button simultaneous drags not supported. |

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
