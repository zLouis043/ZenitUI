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

**Current:** 🧪 alpha — API still evolving, breaking changes expected.

**Next milestone:** 🅱️ beta — API frozen for the core, no known critical
bugs, test coverage on every widget, documentation complete.

**Target after that:** 🚀 1.0 — feature-complete for a typical game /
tool use case, stable API, tooling available.

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

---

## Phase 2 — Missing core API

Small, high-impact additions that unblock common use cases. No new
features — just filling gaps.

### 2.1 `Slider::setValue(float)`

Add a public setter that clamps to `[0, 1]` and optionally fires
`onValueChanged`. A `bool notify` parameter (default `true`) lets
callers suppress the callback for programmatic updates.

- [ ] Add method to `Slider`
- [ ] Add test
- [ ] Update `user/03-widgets.md` and `api/Widgets.md`

### 2.2 `Layout::removeClass` / `clearStyleClasses`

Without these, CSS animations declared under a class can't be stopped
without rebuilding the node. Also blocks dynamic styling use cases.

- [ ] Add `removeClass(const std::string&)`
- [ ] Add `clearStyleClasses()`
- [ ] Add `hasClass(const std::string&) const`
- [ ] Add tests
- [ ] Update `api/Layout.md`

### 2.3 `TextInput` cursor positioning

- [ ] `setCursorPos(int)` with clamping
- [ ] `getCursorPos() const`
- [ ] Click-to-position: on click, compute the cursor index from the
      pointer's x position
- [ ] Add tests
- [ ] Update documentation

### 2.4 `Layout::scrollIntoView`

Scroll the nearest scroll-container ancestor so the node is visible.
Common pattern after `requestFocus`.

- [ ] Implement `scrollIntoView()` walking up to find the scroll
      ancestor
- [ ] Implement `scrollIntoView(const Rect& subRect)` for partial
      visibility
- [ ] Add tests
- [ ] Update documentation

### 2.5 `Button` text accessor

`Btn(text, cb)` creates a `Text` child but there's no accessor. Add:

- [ ] `Button::text() const` returning the current label
- [ ] `Button::setText(std::string)` updating the child

### 2.6 `Text::getText()`

`setText` exists, `getText` doesn't.

- [ ] Add `const std::string& getText() const`

### 2.7 `Layout::removeAllChildren()`

A safe bulk-clear for the `children` vector. Useful for rebuilding
lists.

- [ ] Add method
- [ ] Consider whether it should also clear `parent` pointers of the
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

---

## Phase 3 — Test coverage

The current test suite covers the foundations well (parsing, layout
basics, basic interaction) but has gaps on composite widgets and the
scroll system.

### 3.1 Scroll system

No tests currently exist for the scroll system. Add:

- [ ] `scrollTo`, `scrollToX/Y`, `scrollToTop/Bottom/Left/Right`
- [ ] `getScrollX/Y`, `getMaxScrollX/Y` after arrange with overflow
- [ ] Wheel event increments velocity
- [ ] Inertia decays velocity over time
- [ ] `maxScroll` is zero when content fits
- [ ] Scrollbar thumb hit-test and drag
- [ ] `resetIfOverflowChanged` (once 1.2 is fixed)
- [ ] Hover freeze during scroll

### 3.2 `TextInput`

- [ ] Character insertion
- [ ] `Backspace` / `Delete` with cursor position
- [ ] `Left` / `Right` cursor movement
- [ ] `Home` / `End`
- [ ] Key repeat (initial delay + interval)
- [ ] `onTextChanged` fires on edit
- [ ] `onSubmit` fires on Enter
- [ ] `Escape` releases focus without submitting
- [ ] Horizontal scroll of the cursor
- [ ] Click focuses the input

### 3.3 `Text` wrapping

- [ ] `setWrap(true)` with a bounded parent width
- [ ] Multiple lines
- [ ] Re-wrap on width change (`onLayout`)
- [ ] No wrap when parent width is unbounded

### 3.4 `Dropdown`

- [ ] `setSelected` fires `onChange`
- [ ] Opening the list shows options
- [ ] Clicking an option sets the selection and closes
- [ ] Clicking outside closes
- [ ] Flip above when no space below
- [ ] List scrolls when options exceed the visible area

### 3.5 State transitions end-to-end

- [ ] `:hover` with `transition:` interpolates over N frames
- [ ] `:checked` with transition (Toggle knob)
- [ ] `:focus` styling applies on focus
- [ ] `:disabled` styling applies on disable
- [ ] Transition completes and snaps to target

### 3.6 CSS animations end-to-end

- [ ] `@keyframes` with `animation:` changes the render style
- [ ] `infinite` animation keeps ticking
- [ ] `alternate` reverses every other iteration
- [ ] `forwards` keeps the last keyframe
- [ ] `::part` animation ticks
- [ ] `onAnimationsFinished` fires

### 3.7 Focus scopes

- [ ] Tab stays within a scope
- [ ] Nested scopes: inner scope wins
- [ ] Focus release on click outside
- [ ] Focus release on disable

### 3.8 Portal hit-test delay

- [ ] Verify that a newly-opened portal becomes hit-testable on the
      **next** frame, not the same one
- [ ] Verify that a destroyed portal is skipped

### 3.9 Media queries on viewport change

- [ ] Setting `Metrics::viewport` triggers a style re-resolve
- [ ] `@media (min-width: 600px)` matches / doesn't match based on the
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

- [ ] Implement `SmallVector<Term, 2>`
- [ ] Replace `std::vector<Term>` in `Value`
- [ ] Benchmark before/after
- [ ] Verify no behavioral change

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

- [ ] Implement option B (fast path with no copy)
- [ ] Benchmark

### 4.3 Rule indexing by tag

`StyleResolver::resolveFor` scans all `Theme::rules` linearly. For
stylesheets with hundreds of rules and nodes that change state
frequently, this is a hotspot.

**Fix:** build an index `unordered_map<string, vector<size_t>>` mapping
tag names to rule indices. A rule with a tag in its `chain.back()` is
indexed; rules with only a class / id in `chain.back()` go into a
"no-tag" bucket that's always scanned.

The index is built lazily and invalidated when `Theme::addRule` is
called.

- [ ] Add the index to `Theme`
- [ ] Update `resolveFor` to use it
- [ ] Benchmark with a large stylesheet
- [ ] Handle invalidation correctly

### 4.4 Cache animation evaluation

`tickIncremental` evaluates the keyframe to detect finish;
`overlayCssComputed` evaluates it again to apply. Each evaluation
iterates ~45 properties.

**Fix:** store the last-evaluated `Style` in the `ActiveCssAnimation`
and reuse it if the elapsed time hasn't changed between the two calls.

- [ ] Add `Style cachedFrame` to `ActiveCssAnimation`
- [ ] Add a `uint32_t cacheKey` (frame number or elapsed timestamp)
- [ ] Set / check the cache in the two call sites
- [ ] Benchmark

### 4.5 Avoid double `resolveFor` per frame

When a node has a `pendingTransition`, `update` calls `resolveFor` and
then `measure` calls it again. A "resolved this frame" flag would avoid
the second call.

- [ ] Add `bool resolvedThisFrame` to `StyleResolver`
- [ ] Check it in `measure`
- [ ] Reset at the start of each `update`
- [ ] Benchmark

### 4.6 Vector resize audit

`Layout::drawChildren` uses three member vectors (`drawNegZ_`,
`drawNormal_`, `drawPosZ_`). They grow to the max sibling count and
then stabilize. Verify with a benchmark that no per-frame allocation
happens after warmup.

- [ ] Benchmark `drawChildren` allocation behavior
- [ ] If allocations persist, pre-reserve based on `children.size()`

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
| alpha | — | Current state. API unstable. |
| beta | TBD | Phase 1-3 complete. API frozen for core. |
| 1.0 | TBD | Phase 4-5 complete. Feature-complete for typical use. |

---

## See also

- [README](README.md) — overview and quick start.
- [Getting Started](user/01-getting-started.md) — first steps.
- [Architecture](internals/01-architecture.md) — how the framework is
  built.
- [Debugging](internals/10-debug.md) — how to inspect runtime state.
