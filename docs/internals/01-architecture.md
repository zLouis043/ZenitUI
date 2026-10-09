# Architecture

This document is the bird's-eye view of how ZenitUI is put together:
which pieces exist, what each one owns, and how data flows between them
during a frame. It's the map you should read before diving into the
individual subsystem documents.

If you only read one internals file, read this one.

---

## 1. What kind of framework is this?

ZenitUI is a **retained-mode** UI framework:

- Every widget is a persistent object (`Layout` and subclasses) that
  lives across frames.
- The tree is built once and mutated incrementally (add/remove children,
  change styles, change state).
- There's no diffing, no virtual tree, no reconciler. What you build is
  what runs.

This is in contrast to **immediate-mode** frameworks (like Dear ImGui),
where the whole UI is re-declared every frame and thrown away. Retained
mode is the right choice when:

- You have many widgets and want to avoid re-declaring them.
- You want real node identity (each widget has a stable `shared_ptr`).
- You want CSS-like cascade and state transitions.
- You want to build declaratively (ZMarkup) or imperatively (C++) with
  the same underlying model.

The trade-off is more bookkeeping: a `Layout` node carries more state
than an immediate-mode "draw call".

---

## 2. Layers

ZenitUI is organized in six conceptual layers. Each one depends only on
the ones below it, never above.

```
┌──────────────────────────────────────────────────────────────┐
│  Authoring                                                    │
│    C++ widget API  ·  ZMarkup DSL  ·  ZStyle stylesheets      │
├──────────────────────────────────────────────────────────────┤
│  Node model                                                   │
│    Layout  (tree, rect, state flags, callbacks)               │
│    TLayout<Derived, Base>  (CRTP factory + fluent helpers)    │
├──────────────────────────────────────────────────────────────┤
│  Subsystems                                                   │
│    StyleResolver  ·  AnimationPlayer  ·  InputController      │
│    ScrollController  ·  FilterRegistry                        │
├──────────────────────────────────────────────────────────────┤
│  Runtime                                                      │
│    UIContext (singleton)  ·  Metrics  ·  Theme                │
├──────────────────────────────────────────────────────────────┤
│  Backend interfaces                                           │
│    IRenderer  ·  IPlatform  ·  IAssetProvider                 │
├──────────────────────────────────────────────────────────────┤
│  Backend implementation                                       │
│    RaylibBackend  ·  any other renderer/platform you write    │
└──────────────────────────────────────────────────────────────┘
```

### Authoring

Two ways to build a tree, both producing the same `Layout` objects:

- **C++**: `auto b = Btn("OK", cb);`
- **ZMarkup**: `ZMarkup::build("Button \"OK\"")`

Plus a `.zstyle` stylesheet language parsed into `ThemeRule`s, loaded
into the global `Theme`.

### Node model

`Layout` is the single node type. It's not pure virtual — it's concrete
and usable directly. Subclasses (widgets) override specific hooks. The
node owns:

- Its type (`Stack` / `Vertical` / `Horizontal`).
- Its `rect` (computed during `arrange`).
- Its `measuredSize` (computed during `measure`).
- Its flags (`isHovered`, `isPressed`, `isFocused`, `isEnabled`,
  `isChecked_`, `isPortal_`, `passThrough_`, ...).
- Its children (`std::vector<std::shared_ptr<Layout>>`).
- Its four subsystems (`style_`, `anim_`, `input_`, `scroll_`).
- Its callbacks (`onClick`, `onHoverEnter`, ...).

`TLayout<Derived, Base>` is the CRTP helper that adds:

- A `static create(...)` factory that calls `onBuild()` after
  construction.
- Fluent helpers (`cls`, `id`, `size`, `with`, `passThrough`) that
  return `shared_ptr<Derived>`.

The base `Base` parameter lets you build on top of an existing widget:
`Popup : TLayout<Popup, Panel>`, `Tooltip : TLayout<Tooltip, Panel>`.

### Subsystems

Each subsystem is a small struct, `friend`-accessible from `Layout`.
They don't own a `Layout` reference; they're called by `Layout`'s
lifecycle methods with `*this` as a parameter.

| Subsystem | Owns | Depends on |
|-----------|------|------------|
| `StyleResolver` | Cascade inputs (inline defaults/base/states), computed style, transitions, part states | `Theme`, `Layout` state flags |
| `AnimationPlayer` | Imperative animations, CSS node animations, `::part` animations | `Layout`, `Theme::keyframes` |
| `InputController` | (Nothing) — stateless dispatcher over `Layout` flags and `UIContext` | `UIContext`, `Layout` |
| `ScrollController` | `ScrollState` (offset, velocity, drag start), scrollbar geometry | `Layout` style, `UIContext` pointer/wheel |
| `FilterRegistry` | Named filter definitions (built-ins + custom) | `IRenderer`, `IAssetProvider` |

The subsystems are not independently ticked from outside. They're called
by `Layout::update`, `Layout::measure`, `Layout::arrange`, and
`Layout::draw`. This keeps the frame order under one roof.

### Runtime

`UIContext` is a singleton that holds:

- The three backend pointers.
- The current input snapshot (`pointer`, `inputEvents`, `shiftHeld`).
- Focus state (`focusedNode`).
- Pointer capture (`pointerCapture`).
- Portal registries (`activePortals`, `framePortals`).
- Frame clock (`dt`, `time`).
- Viewport generation counter.
- A RNG (used by `TextInput` for nothing important, but available).

`Metrics::viewport` holds the current logical viewport size, updated
each `beginFrame`. It's used by `Value::resolveH/V` for `vw`/`vh`.

`Theme` is the global stylesheet: a list of `ThemeRule`s, a `root`
style, and a map of `KeyframeAnimation`s. It's populated by the
`.zstyle` parser and read by `StyleResolver`.

### Backend interfaces

Three abstract classes define everything the framework needs from the
outside world:

- **`IRenderer`** — drawing primitives, text measurement, clip,
  transform, render target, shader push/pop, `supports(Feature)`.
- **`IPlatform`** — window size, pointer state, input events, time,
  DPI, safe area.
- **`IAssetProvider`** — get font/texture/shader by string name.

Nothing in the framework above these interfaces knows about Raylib,
SDL, OpenGL, Metal, or any concrete graphics API.

### Backend implementation

`RaylibBackend.hpp/cpp` provides `RaylibRenderer`, `RaylibPlatform`,
and `RaylibAssetProvider`. This is the only place where the codebase
includes `<raylib.h>`. To port ZenitUI to another backend, you write
three new classes implementing the same interfaces — no core changes.

---

## 3. The frame pipeline

Every frame has two phases, called separately by the user's main loop.

```cpp
// Phase 1: logic (no GPU calls)
root->runFrame(dt);

// Phase 2: draw (no input reads)
BeginDrawing();
root->renderFrame();
EndDrawing();
```

### 3.1 `runFrame(dt)`

```cpp
void Layout::runFrame(float dt) {
    UIContext::get().beginFrame(dt);
    updateTree(dt);
    measure(Metrics::viewport.x, Metrics::viewport.y);
    arrange({0, 0, Metrics::viewport.x, Metrics::viewport.y});
}
```

Four sub-phases:

#### `beginFrame(dt)`

Polls the platform:

- Updates `Metrics::viewport`, `safeArea`, `pointer`, `inputEvents`,
  `shiftHeld`, `dpiScale`.
- Increments `viewportGeneration` if the viewport or safe area changed.
- Clears per-frame flags: `clickConsumed`, `rightClickConsumed`,
  `wheelConsumedThisFrame`, `topmostConsumer`, `hoverTarget`,
  `releaseTarget`.
- Swaps `activePortals ← framePortals` (portals from last frame become
  the hit-test set for this frame).

It does **not** read the pointer or hit-test. That's `updateTree`'s job.

#### `updateTree(dt)`

Runs the input pass and then the per-node update:

1. Hit-test the pointer to find `topmostConsumer`.
2. Set `hoverTarget = topmostConsumer`.
3. If pointer pressed, set `pressTarget = hoverTarget`; if released,
   `releaseTarget = hoverTarget`.
4. Focus handling:
   - On press: focus `topmostConsumer` if focusable, else release.
   - On `Tab`: cycle through focusables within the current scope.
5. Call `update(dt, false, false)` on the root — which recursively
   walks the tree, running the per-node update (see §3.2).
6. Cleanup: if the pointer was released, `pressTarget = nullptr`; if
   the button is up and capture is set, release capture.

#### `measure(w, h)`

Recursive, top-down-then-bottom-up. Each node:

1. Resolves its style if needed.
2. Computes its available inner size (parent size minus padding).
3. Calls `computeIntrinsicSize(availW, availH)`.
4. Recurses into children to get their measured sizes.
5. Combines children sizes according to `LayoutType` (max for `Stack`,
   sum for `Vertical`/`Horizontal`).
6. Adds padding, applies `min-*` / `max-*`, produces `measuredSize`.

The result is cached. `measure` short-circuits if neither the node's
subtree is dirty nor the parent size changed.

#### `arrange(rect)`

Recursive, top-down. Each node:

1. Sets its `rect` from the incoming space.
2. Splits the space into `padding` inner box.
3. Arranges absolute children and portals into the inner box.
4. Arranges flow children (`Vertical` / `Horizontal` / `Stack`) with
   flex (`grow` / `shrink` / `gap` / `justify` / `items-*`).
5. Calls `onLayout()` for widgets that need to react to final size.

If the node is a scroll container, `arrange` delegates to
`ScrollController`, which may expand the arrange space to fit content
and applies the scroll offset via `translateSubtree`.

### 3.2 Per-node `update(dt)`

Called for every node in the tree, depth-first. It's the heart of the
framework. In order:

1. `initStyleIfNeeded` — on the first frame, resolve the initial style.
2. `scroll_.resetIfOverflowChanged` — reset scroll if `overflow`
   changed.
3. Viewport generation check — if the viewport changed, force a
   `pendingTransition`.
4. `anim_.tickImperative(*this, dt)` — tick imperative animations.
   Returns whether they block input.
5. Compute `blockSubtree` and `selfBlocked`.
6. `input_.updateFlags(*this, selfBlocked, scrolling)` — update
   `isHovered`, `isPressed`, `isFocused`. Skipped if `scrolling` is
   true (hover freeze).
7. Compute the new `UIState` (`Idle` / `Hover` / `Pressed` / `Disabled`).
8. If the state changed, `style_.beginStateTransition(...)`. Else if
   `pendingTransition`, `style_.resolvePendingTransition(...)`.
9. `style_.tick(*this, dt)` — tick the CSS transition.
10. `anim_.tickCss(*this, dt)` — tick CSS keyframe animations.
11. `style_.propagateInheritance(*this)` — notify children if inherited
    props changed.
12. `onAnimationsFinished` if the node just became animation-free.
13. Recurse into children with `update(dt, blockSubtree, scrolling)`.
14. `scroll_.tickInput(*this)` — handle wheel, drag, inertia.
15. `input_.handleKeyInput(*this)` — Enter/Space activation.
16. `input_.fireCallbacks(*this, prev, next, changed)` — hover, press,
    release, click, right-click.
17. `cullRemovedChildren()` — remove children with `wantsRemoval`.
18. `recomputeDirty(...)` — update `subtreeDirty_`.
19. `onUpdate(dt)` if `isEnabled || updateWhenDisabled_`.

The order matters. Read it carefully before changing anything.

### 3.3 `renderFrame()`

```cpp
void Layout::renderFrame() {
    auto* r = UIContext::get().renderer;
    r->beginFrame();       // e.g. push DPI matrix
    draw();
    r->endFrame();
}
```

`draw(opacity)` on the root recursively draws the tree. Each node:

1. If `isPortal_`, register itself in `framePortals` and return.
2. Frustum cull: if the node's `rect` is far outside the current clip,
   skip.
3. Compute `renderStyle = currentStyle + CSS animation overlay`.
4. Resolve the effect (shader) name to a handle if needed.
5. Compute `globalOp = renderStyle.opacity * parentOpacity`.
6. Choose `drawInline` or `drawLayer` based on whether the node has
   filters and whether the renderer supports them.
7. Draw children in z-order.
8. Draw the scrollbar if this is a scroll container.
9. If this is the root, drain `framePortals` and draw each one.

`drawInline`:

```
push transform (if identity is false)
push effect (if node has effect)
push filter effect (if currentEffect_ is valid)
push clip (if overflow is not visible)
renderChrome (background, box-shadow, border)
renderContent (widget-specific)
drawChildren (z-ordered)
pop clip
pop filter effect
pop effect
pop transform
```

`drawLayer`:

1. Compute the transformed bounding box.
2. Intersect with the current clip → `region`.
3. Ensure two render targets are sized to `region`.
4. `pushTarget(layerTarget_)`.
5. Apply the offset transform so the subtree draws at (0, 0) of the
   target.
6. Draw chrome + content + children.
7. `popTarget()`.
8. Run the filter chain on the target.
9. `drawTarget(layerTarget_, region, ...)`.

### 3.4 `hitTest(p, ancestorBlocked)`

Recursive, top-down, z-ordered. Returns the topmost node that:

- Is not blocked by an ancestor.
- Is enabled.
- Contains the point in its `rect`.
- Is `isInteractive` or `blocksRaycast`.

Order:

1. If this is the root, test portals first (reverse order).
2. Apply the current transform inverse to `p`.
3. Partition children into `negZ`, `normalFlow`, `posZ` (stacking
   contexts with negative / no / positive z-index).
4. Test `posZ` (reversed), then `normalFlow` (reversed), then `negZ`.
5. If no child hit, test `this` (if it qualifies).

This is the same z-order as `drawChildren`, reversed. Topmost pixel
under the cursor is the topmost interactive node.

---

## 4. Data flow between subsystems

The subsystems are not independent. They share state through `Layout`
and `UIContext`. The main flows:

### 4.1 Input → state → style

```
IPlatform → UIContext.pointer, UIContext.inputEvents
  ↓
Layout::updateTree → hitTest → topmostConsumer
  ↓
Layout::update → InputController::updateFlags
  ↓
Layout.isHovered / isPressed / isFocused
  ↓
InputController::computeNextState → UIState
  ↓
StyleResolver::beginStateTransition / resolvePendingTransition
  ↓
StyleResolver resolves matching rules using the flags (via ruleMatches)
  ↓
ComputedStyle.targetStyle
  ↓
StyleResolver::tick → currentStyle
  ↓
Layout::draw → renderChrome / renderContent
```

### 4.2 Layout → scroll

```
measure → measuredSize + contentSize (ScrollController::contentSize)
  ↓
arrange → if isScrollContainer:
    ScrollController::computeArrangeSpace (may expand)
    ScrollController::applyOffsetDelta → translateSubtree on children
    ScrollController::updateMaxScroll
  ↓
ScrollController::tickInput (wheel, drag, inertia)
  ↓
draw → ScrollController::drawScrollbar
```

### 4.3 Animation → style → render

```
AnimationPlayer::tickImperative → writes to getInlineBase() → pendingTransition
  ↓
AnimationPlayer::tickCss → updates css[] active animations
  ↓
Layout::draw → Anim::overlayCssComputed(renderStyle, anim_.css)
  ↓
renderStyle is the effective style used for drawing
```

The important detail: `renderStyle` is computed **at draw time** and
includes the current CSS animation values. The `currentStyle` stored in
`StyleResolver` does **not** include them. This is why `getStyle()`
returns the transition value, not the animated value.

### 4.4 Portal registration

```
Layout::draw (portal node) → UIContext.framePortals.push_back(weak_from_this())
  ↓
Layout::draw (root, end) → iterates framePortals → draws each
  ↓
Next frame: beginFrame → activePortals = move(framePortals)
  ↓
hitTest → iterates activePortals → the portal becomes clickable
```

The one-frame delay is intentional (see [Portals](../user/08-portals.md)
§8).

### 4.5 Filter resolution

```
ComputedStyle.filters (vector<FilterRef>)
  ↓
Layout::drawLayer → for each FilterRef:
    FilterRegistry::find(ref->name) → FilterDef
  ↓
applyFilter(def, FilterContext{...})
  ↓
pattern-specific pass: SinglePass / Separable / Silhouette / Custom
  ↓
IRenderer::pushEffect / setEffect* / drawTarget / popEffect
```

If the shader name isn't registered with `IAssetProvider`, the effect
handle is invalid and `drawTarget` is called without a filter (fallback).

---

## 5. Ownership and lifetime

### 5.1 Nodes

- Children are owned by their parent's `children` vector
  (`shared_ptr<Layout>`).
- A child holds a `weak_ptr<Layout>` to its parent.
- `removeFromParent` sets `wantsRemoval = true`. The parent erases the
  child during the next `cullRemovedChildren` pass.

There are no cycles by default. The `weak_ptr` to parent prevents the
obvious one.

### 5.2 Callbacks that capture nodes

Callbacks stored on a node often capture other nodes. This can create
cycles:

```cpp
node->addAnimation("a", anim);
anim->onFinished = [node] { /* ... */ };   // cycle: node → anim → lambda → node
```

Break it with `weak_ptr`:

```cpp
std::weak_ptr<Layout> weak = node;
anim->onFinished = [weak] {
    if (auto n = weak.lock()) n->playAnimation("b");
};
```

The framework does this internally where needed (`ContextMenu` captures
`weak_ptr<Popup>`, `Tooltip::attach` captures `shared_ptr<Tooltip>` which
is fine because the tooltip is owned by the owner).

### 5.3 Render targets

Each node that uses `drawLayer` owns two `TargetHandle`s
(`layerTarget_`, `layerScratch_`). They're created lazily and destroyed
in `~Layout`. They're not shared between nodes.

### 5.4 Subsystems

All four subsystems are members of `Layout`, by value. They don't hold
`shared_ptr` to the node. They're accessed via `friend`-private methods
and passed `*this` as a parameter.

`AnimationPlayer::imperative` stores `shared_ptr<UIAnimation>`. The
animation is owned by the player. Users create it and pass it via
`addAnimation`.

`StyleResolver::partTransitions` stores `Style` objects and timers, no
pointers.

`ScrollController` stores `ScrollState` (POD).

`InputController` stores nothing.

---

## 6. Why separate files?

The `src/` folder splits the `Layout` implementation into focused files:

| File | Concern |
|------|---------|
| `Layout.cpp` | Orchestration: `update`, `updateTree`, `runFrame`, `renderFrame`, `recomputeDirty`, `UIContext::requestFocus`. |
| `Measure.cpp` | `measure`, `arrange`, `arrangeInto`, `arrangeAbsolute`, `arrangeVerticalFlow`, `arrangeHorizontalFlow`, `arrangeStackFlow`, `hitTest`. |
| `Render.cpp` | `draw`, `drawInline`, `drawLayer`, `drawChildren`, `resolveEffectIfNeeded`. |
| `StyleResolver.cpp` | Cascade resolution, state transitions, part styles, inheritance propagation. |
| `AnimationPlayer.cpp` | Imperative + CSS ticking, keyframe evaluation. |
| `Input.cpp` | `updateFlags`, `computeNextState`, `fireCallbacks`, `handleKeyInput`. |
| `ScrollController.cpp` | Scroll arrange, input, thumb geometry. |
| `FilterRegistry.cpp` | Registry, param binding, built-in filters, `applyFilter`. |

The split is by **phase and subsystem**, not by class. All these files
implement methods declared on `Layout` (or on the free functions in
`Anim` and the `FilterRegistry` class).

The `Layout` class itself is a single header with the full API. It's
large (~700 lines) but the implementation is spread out, so each file
stays readable.

---

## 7. What's `friend` and why

`Layout` declares four friends:

```cpp
friend struct StyleResolver;
friend struct AnimationPlayer;
friend struct InputController;
friend struct ScrollController;
```

This is how the subsystems access private members of `Layout` (flags,
`style_`, `anim_`, `scroll_`, `pendingTransition`, `subtreeDirty_`) and
how they mutate state that's conceptually private but part of the
node's lifecycle.

The alternative — making every field public — would blur the API. The
friend approach keeps the public interface small while letting the
subsystems run the show.

`AnimationPlayer` also accesses `style_.targetStyle.animations` and
`style_.currentStyle` directly. `ScrollController` reads
`node.style_.currentStyle`. `StyleResolver` reads `node.getParent()`,
`node.children`, the flags.

`FilterRegistry` is not a friend — it's a static singleton accessed
through `get()`, and `applyFilter` takes a `FilterContext` with all the
information it needs. It only interacts with `IRenderer` and reads the
`FilterRef` from the context.

---

## 8. Backend interface in one paragraph

The three backend interfaces are the contract with the outside world:

- `IPlatform` gives you **time, input, and viewport**. It's polled once
  per frame in `beginFrame`. It has no drawing responsibility.
- `IRenderer` gives you **drawing primitives, text measurement, and
  compositing state** (clip, transform, effect, target). It's called
  during `draw`. Every draw call is inside a single `beginFrame` /
  `endFrame` pair, matching the host's frame boundary.
- `IAssetProvider` gives you **named resources**: fonts, textures,
  shaders. It's called during style resolution (`resolveFont`,
  `resolveBgTexture`, `resolveEffectIfNeeded`) and during filter
  application (`FilterRegistry::find` returns a def whose `shader` is
  resolved via `assets->getEffect(name)`).

The interfaces are small (a few dozen methods total) and never expose
backend-specific types. The only enum passed across is `Feature`
(currently `Effects` and `NestedTargets`), so the framework can degrade
gracefully when a backend doesn't support a feature.

See [Backend](09-backend.md) for the full list of methods and how to
implement a new one.

---

## 9. Testing strategy

The framework is designed to be testable without a window. The
`tests/Mocks.hpp` provides:

- `MockRenderer` — records calls, provides a deterministic
  `measureText` (8px per char + spacing), tracks clip intersection.
- `MockPlatform` — scriptable pointer, input events, viewport.
- `MockAssetProvider` — returns empty handles (so no assets are loaded
  in tests).
- `Env` — a fixture that installs the mocks in `UIContext`, clears
  `Theme`, and provides `frame(root, dt)` and `click(root, pos)`
  helpers.

Tests exercise the real `Layout` code with these mocks. The mock
renderer supports `supports(Feature::Effects) == false`, so filters are
skipped in tests unless explicitly enabled.

This is why the framework doesn't have a `TestingMode` flag or a "headless"
build variant: the backend abstraction is already the seam.

---

## 10. Design trade-offs

A few decisions worth knowing about:

**Single `Layout` class, no virtual hierarchy for node types.**
Widgets are subclasses, but the tree uses `shared_ptr<Layout>`. There's
no `Widget` base, no visitor pattern. Type checks are done by tag name
or by `static_pointer_cast` when a typed handle is needed. This is
simpler than a full class hierarchy but requires discipline (don't
`static_pointer_cast` to the wrong type).

**Subsystems as `friend` structs, not virtual classes.**
`StyleResolver`, `AnimationPlayer`, `InputController`,
`ScrollController` are concrete. There's no polymorphic dispatch between
them. Adding a new subsystem means adding a member to `Layout` and a
new friend declaration. This is faster and simpler than an interface
hierarchy, but it means you can't swap implementations.

**Style as a flat struct with `Opt<T>` fields.**
`Style` has ~40 fields, each wrapped in `Opt<T>`. The macro
`BUBBLE_STYLE_PROPS(X)` expands to all of them, and every operation
(overlay, lerp, comparison, iteration) uses the same macro. This is
verbose at the macro level but keeps the code consistent: adding a
property means adding one line to the macro and everything else updates.

**Custom properties stored as raw strings, resolved lazily.**
`--x: #FF0000` is stored as `"#FF0000"`. `var(--x)` is substituted at
cascade time. This matches CSS and allows custom properties to hold
values that aren't parseable until the cascade is resolved. The cost is
a string map per node per frame — measurable but small.

**Transitions per-property, not per-node.**
Each property in `transition:` has its own duration and easing. This
matches CSS but is more complex than a single "duration + easing" per
node. The implementation uses `TransitionSpec` and `lerpStyleTimed`,
which reads the spec per property.

**`filters` snap, `effects` push/pop.**
Filter chains don't interpolate; they switch at `t > 0`. This is a
known limitation. The `effect:` shader is a push/pop around the node's
draw calls, not a transitioned property. Neither is as flexible as
CSS's `filter:` interpolation, but both are simple and predictable.

**Portals are a flag, not a separate tree.**
A portal is still a child of its logical parent for lifetime purposes.
Only drawing and hit-testing are rerouted. This means the portal's
position is computed by the portal itself (via `onUpdate`), not by the
parent's `arrange`. Simpler than a separate portal tree, but the
positioning responsibility shifts to the widget.

**The frame pipeline is not customizable.**
You can't override the order of `update` → `measure` → `arrange` →
`draw`. Widgets hook in via virtual methods, but the phases are fixed.
This is intentional — it's what makes the framework's behavior
predictable across widgets.

---

## 11. Where to go next

- **[Lifecycle](02-lifecycle.md)** — the exact order of operations per
  frame, with pseudocode.
- **[Style System](03-style-system.md)** — how the cascade resolves, how
  specificity works, how inheritance propagates.
- **[Animation System](04-animation-system.md)** — imperative vs CSS,
  the overlay mechanism, the dirty system.
- **[Layout Engine](05-layout-engine.md)** — intrinsic size, flow
  algorithms, absolute positioning, dirty tracking.
- **[Render Pipeline](06-render-pipeline.md)** — inline vs layer, the
  filter pass, the transform stack.
- **[Input System](07-input-system.md)** — state FSM, focus, capture,
  the click semantics.
- **[Scroll System](08-scroll-system.md)** — `ScrollState`, arrange
  expansion, inertia, scrollbar geometry.
- **[Backend](09-backend.md)** — implementing `IRenderer` / `IPlatform`
  / `IAssetProvider`.
- **[Debugging](10-debug.md)** — `Debug::dump*`, `Logger`, the mock
  fixture.