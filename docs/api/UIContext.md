# `UIContext` — API Reference

The global context: backend pointers, input snapshot, focus / capture
state, portal registry, frame clock, and the three backend interfaces.

- **Header**: `UIContext.hpp`
- **Namespace**: `ZenitUI`

For the backend interfaces, see [Backend](../internals/09-backend.md).
For the frame lifecycle, see [Lifecycle](../internals/02-lifecycle.md).

---

## 1. Overview

`UIContext` is a singleton that holds everything the framework needs
that isn't per-node:

- **Backend pointers** — `IRenderer*`, `IPlatform*`, `IAssetProvider*`.
- **Input snapshot** — the current pointer state, input events,
  modifier keys, DPI, safe area.
- **Frame clock** — `dt`, `time`, `viewportGeneration`.
- **Focus** — the currently focused node.
- **Pointer capture** — the node that's currently capturing pointer
  events.
- **Targets** — the hit-test / press / release targets for the frame.
- **Portals** — two lists (active from last frame, pending for this
  frame).
- **Per-frame flags** — `clickConsumed`, `rightClickConsumed`,
  `wheelConsumedThisFrame`.

It's a singleton because the framework assumes a single UI tree on a
single window. For multi-window, you'd need a significant refactor.

---

## 2. The three interfaces

`UIContext.hpp` also declares the three backend interfaces. They're
documented fully in [Backend](../internals/09-backend.md). Summary:

### `IRenderer`

Drawing primitives, text measurement, clip / transform / effect stacks,
render targets, frame setup, feature detection.

```cpp
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Primitives
    virtual void fillRect(Rect r, Color c) = 0;
    virtual void fillRoundedRect(Rect r, float radiusPx, Color c) = 0;
    virtual void fillCircle(Vec2 center, float radius, Color c) = 0;
    virtual void strokeRect(Rect r, float thickness, Color c) = 0;
    virtual void strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c) = 0;
    virtual void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) = 0;
    virtual void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) = 0;
    virtual void drawText(FontHandle f, std::string_view s, Vec2 pos,
                          float size, float spacing, Color c) = 0;
    virtual Vec2 measureText(FontHandle f, std::string_view s,
                             float size, float spacing) = 0;

    // Stacks
    virtual void pushTransform(const Transform2D& t) = 0;
    virtual void popTransform() = 0;
    virtual void pushEffect(EffectHandle e) = 0;
    virtual void popEffect() = 0;
    virtual void pushClip(Rect r) = 0;
    virtual void popClip() = 0;
    virtual Rect getClipRect() const = 0;

    // Targets
    virtual TargetHandle createTarget(int w, int h) = 0;
    virtual void destroyTarget(TargetHandle t) = 0;
    virtual void pushTarget(TargetHandle t) = 0;
    virtual void popTarget() = 0;
    virtual void drawTarget(TargetHandle t, Rect dst, Color tint) = 0;
    virtual void clearTarget(TargetHandle t, Color c) = 0;
    virtual bool inTarget() const = 0;

    // Uniforms
    virtual void setEffectFloat(EffectHandle e, const char* name, float value) = 0;
    virtual void setEffectVec2(EffectHandle e, const char* name, Vec2 value) = 0;
    virtual void setEffectVec4(EffectHandle e, const char* name, Color value) = 0;

    // Frame
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void setDpiScale(float scale) = 0;

    // Features
    virtual bool supports(Feature f) const = 0;
};
```

### `IPlatform`

Window, input, time, DPI, safe area.

```cpp
class IPlatform {
public:
    virtual ~IPlatform() = default;
    virtual Vec2 viewportSize() = 0;
    virtual PointerState pointer() = 0;
    virtual double time() = 0;
    virtual bool shiftHeld() = 0;
    virtual InputEvents pollInputEvents() = 0;
    virtual float dpiScale() = 0;
    virtual EdgeInsets safeArea() = 0;
};
```

### `IAssetProvider`

Named resource lookup.

```cpp
class IAssetProvider {
public:
    virtual ~IAssetProvider() = default;
    virtual FontHandle    getFont(std::string_view name) = 0;
    virtual TextureHandle getTexture(std::string_view name) = 0;
    virtual EffectHandle  getEffect(std::string_view name) = 0;
};
```

### `Feature`

```cpp
enum class Feature {
    Effects,
    NestedTargets
};
```

Feature flags for `IRenderer::supports`. Currently only `Effects` is
checked by the framework.

---

## 3. `UIContext` — the singleton

```cpp
struct UIContext {
    static UIContext& get();

    // ... fields and methods ...
};
```

### `get()`

```cpp
static UIContext& get();
```

Returns the global instance. Thread-safe initialization via C++11
magic statics.

There's no `set` — you modify fields directly on the reference. This is
intentional: the framework assumes one instance, and the fields are
mutable public members.

---

## 4. Backend pointers

```cpp
IRenderer*     renderer{nullptr};
IPlatform*     platform{nullptr};
IAssetProvider* assets{nullptr};
```

The user sets these before the first frame:

```cpp
UIContext::get().renderer = &backendRenderer;
UIContext::get().platform = &backendPlatform;
UIContext::get().assets   = &backendAssets;
```

All three are raw pointers. The user owns the objects and is
responsible for their lifetime — they must outlive the `UIContext`
usage.

If any is null at frame time, the framework's behavior is undefined
(it might crash, or silently do nothing depending on the code path).
Always set all three.

---

## 5. Input snapshot

These fields are overwritten every `beginFrame` from `platform`:

```cpp
PointerState pointer;
InputEvents  inputEvents;
bool         shiftHeld{false};
float        dt{0.0f};
double       time{0.0};
float        dpiScale{1.0f};
EdgeInsets   safeArea;
```

### `pointer`

The current pointer state. See `CoreTypes.hpp`:

```cpp
struct PointerState {
    Vec2  pos;
    bool  down, pressed, released;
    bool  rightDown, rightPressed, rightReleased;
    float wheelY;
};
```

`pressed` / `released` (and their right-click counterparts) are
one-frame edges. `down` is level.

### `inputEvents`

The keyboard events for the frame:

```cpp
struct InputEvents {
    std::vector<int> chars;   // Unicode codepoints typed this frame
    std::vector<int> keys;    // special keys pressed (rising edge)
    std::vector<int> held;    // special keys currently down (level)
};
```

`chars` are text input, `keys` / `held` are physical keys mapped to
`Key::*` codes.

### `shiftHeld`

Whether `Shift` is down. The only modifier tracked. Used by Tab
(reverse direction), scroll (horizontal wheel), and Slider (fine step).

### `dt` / `time`

- `dt` — the frame delta, in seconds. Passed in by the user to
  `runFrame(dt)`.
- `time` — cumulative time, advanced by `dt` each frame. Used as the
  timeline for absolute-tick animations.

### `dpiScale`

The current DPI scale, from `platform->dpiScale()`. Passed through to
the renderer in `beginFrame`.

### `safeArea`

Notch / rounded-corner insets. Stored but not used by the framework.
Available for user code.

---

## 6. Per-frame flags

Cleared at the start of each frame:

```cpp
bool wheelConsumedThisFrame{false};
bool clickConsumed{false};
bool rightClickConsumed{false};

void consumeClick() { clickConsumed = true; }
void consumeRightClick() { rightClickConsumed = true; }
```

- **`wheelConsumedThisFrame`** — set by the first scroll container that
  handles a wheel event. Prevents nested scroll containers from both
  scrolling.
- **`clickConsumed`** — set when a node's `onClick` (or `onPress`) fires
  without `passThrough`. Prevents ancestors from also firing `onClick`.
- **`rightClickConsumed`** — same, for `onRightClick`.

The `consumeClick` / `consumeRightClick` methods are the public API to
set the flags. User code rarely needs them (the framework sets them
automatically).

---

## 7. Focus

```cpp
std::weak_ptr<Layout> focusedNode;

void requestFocus(std::shared_ptr<Layout> n);
void releaseFocus() { focusedNode.reset(); }
bool hasFocus(const Layout* n) const;
```

### `requestFocus(n)`

Sets `focusedNode = n` if `n` is focusable. If `n` is null or not
focusable, clears focus (`focusedNode.reset()`).

Focusable means `isFocusable_ && isEnabled`. A disabled node can't be
focused.

The node's `isFocused` flag isn't updated here — it's updated on the
next `update` in `updateFlags`.

### `releaseFocus()`

Clears the focused node.

### `hasFocus(n)`

True if `n` is the currently focused node.

```cpp
if (UIContext::get().hasFocus(myInput)) {
    // handle keyboard input
}
```

### Focus and `setEnabled(false)`

When a node is disabled, `Layout::setEnabled` clears `focusedNode` if it
points to that node. This is automatic; no user action needed.

---

## 8. Pointer capture

```cpp
std::weak_ptr<Layout> pointerCapture;
```

The node that's currently capturing pointer events. Set by
`Layout::capturePointer()`, cleared by `Layout::releasePointer()` or
automatically when the pointer button is released.

While capture is active:

- `updateTree` uses the captured node as `topmostConsumer`.
- All pointer events are routed to the captured node.

The `weak_ptr` means a destroyed node automatically releases capture.

---

## 9. Hit-test / event routing

```cpp
Layout* topmostConsumer{nullptr};
Layout* hoverTarget{nullptr};
Layout* pressTarget{nullptr};
Layout* releaseTarget{nullptr};
```

Raw pointers, updated during `updateTree`:

- **`topmostConsumer`** — the result of `hitTest` (or the captured
  node). Cleared at the start of each frame, set during `updateTree`.
- **`hoverTarget`** — same as `topmostConsumer` for the current frame.
- **`pressTarget`** — fixed at the frame of a press. Persists across
  frames until the pointer is released.
- **`releaseTarget`** — fixed at the frame of a release.

### Query helpers

```cpp
bool isPressTarget(const Layout* n) const;
bool isValidClick() const;
```

`isPressTarget(n)` — true if `n == pressTarget`.

`isValidClick()` — true if both `pressTarget` and `releaseTarget` are
non-null and equal.

---

## 10. Portals

```cpp
std::vector<std::weak_ptr<Layout>> activePortals;
std::vector<std::weak_ptr<Layout>> framePortals;
```

Two lists of portals:

- **`activePortals`** — the previous frame's `framePortals`. Used by
  `hitTest` for input routing.
- **`framePortals`** — filled during the current frame's draw pass by
  each portal node. Drained at the end of the root's `draw`.

At the start of each frame, `beginFrame` swaps them:

```cpp
activePortals = std::move(framePortals);
framePortals.clear();
```

The one-frame delay in `hitTest` is because `activePortals` is from the
previous frame. See [Portals](../user/08-portals.md) for the
implications.

Both are `weak_ptr` — destroyed portals are automatically cleaned up.

---

## 11. Viewport generation

```cpp
uint32_t viewportGeneration{0};
```

Incremented in `beginFrame` whenever the viewport size or safe area
changes. Each `Layout` caches the last seen generation and forces a
style re-resolve if it changed.

This is how media queries react to viewport changes.

---

## 12. RNG

```cpp
std::mt19937 rng;
```

A random number generator, seeded at construction (default seed). Not
used by the framework. Available for user code that wants randomness
without creating its own RNG.

In practice, you'd use your own RNG. This field is a convenience.

---

## 13. `beginFrame`

```cpp
void beginFrame(float delta_time);
```

The per-frame entry point. Called from `Layout::runFrame`. Handles:

1. Update `dt`, advance `time`.
2. Clear per-frame flags (`wheelConsumedThisFrame`, `clickConsumed`,
   `rightClickConsumed`, `topmostConsumer`, `hoverTarget`,
   `releaseTarget`).
3. Rotate portal buffers (`activePortals ← framePortals`).
4. Poll the platform:
   - `viewportSize`, `safeArea` → update `Metrics::viewport`, check
     for changes → increment `viewportGeneration`.
   - `pointer`, `shiftHeld`, `pollInputEvents`, `dpiScale` → update
     the corresponding fields.
   - Call `renderer->setDpiScale(dpiScale)`.

Does **not** hit-test, set focus, or run updates. Those happen in
`updateTree` (called after `beginFrame`).

### Example

```cpp
float dt = GetFrameTime();
UIContext::get().beginFrame(dt);
```

But in normal use, you call `root->runFrame(dt)` which calls
`beginFrame` for you.

---

## 14. Singleton initialization

The three backend pointers must be set before the first `runFrame`:

```cpp
UIContext::get().renderer = &backendRenderer;
UIContext::get().platform = &backendPlatform;
UIContext::get().assets   = &backendAssets;
```

There's no "init" method. Assigning the pointers is the initialization.
The framework doesn't validate them — if they're null at frame time,
`beginFrame` skips the platform poll, and subsequent calls that use
`renderer` / `assets` check for null or crash depending on the code
path.

In practice, set them early in `main` and don't change them.

### Teardown

Nulling the pointers before the objects are destroyed:

```cpp
UIContext::get().renderer = nullptr;
UIContext::get().platform = nullptr;
UIContext::get().assets   = nullptr;
// then destroy the backends
```

Useful if the backends are destroyed before `UIContext` (which is a
static singleton, destroyed at program exit). If a `Layout` is still
alive after the backends are destroyed, its destructor (`~Layout`)
checks for `renderer != nullptr` before calling `destroyTarget`. So the
nulling is a safety net.

`Env` in `tests/Mocks.hpp` does exactly this: nulls the pointers in its
destructor.

---

## 15. Common patterns

### Wiring the backends

```cpp
RaylibRenderer      renderer;
RaylibPlatform      platform;
RaylibAssetProvider assets(renderer);

UIContext::get().renderer = &renderer;
UIContext::get().platform = &platform;
UIContext::get().assets   = &assets;
```

### Accessing input in a callback

```cpp
btn->onClick = []{
    auto& ctx = UIContext::get();
    std::printf("clicked at %.0f,%.0f shift=%d\n",
        ctx.pointer.pos.x, ctx.pointer.pos.y, ctx.shiftHeld);
};
```

### Manual focus control

```cpp
if (someCondition) {
    UIContext::get().requestFocus(myInput);
} else {
    UIContext::get().releaseFocus();
}
```

### Checking the current target

```cpp
auto& ctx = UIContext::get();
if (ctx.topmostConsumer == myNode.get()) {
    // myNode is the topmost hit
}
```

### Manual input consumption

```cpp
auto& ctx = UIContext::get();
if (handleSomething()) {
    ctx.consumeClick();
}
```

Rarely needed — the framework consumes automatically when a node fires
`onClick` without `passThrough`.

### Reading `dt` in a widget

```cpp
void onUpdate(float dt) override {
    // dt is passed to onUpdate; also available via
    // UIContext::get().dt
}
```

Both are equivalent. Prefer the parameter — it's explicit and doesn't
require the singleton.

### Detecting viewport changes

```cpp
static uint32_t lastGen = 0;
uint32_t gen = UIContext::get().viewportGeneration;
if (gen != lastGen) {
    lastGen = gen;
    // viewport changed
}
```

Not usually needed — the framework handles re-resolution. Useful for
user code that caches viewport-dependent values.

---

## 16. Testing with mocks

The `Env` fixture in `tests/Mocks.hpp` installs mocks and resets state:

```cpp
struct Env {
    MockRenderer      renderer;
    MockPlatform      platform;
    MockAssetProvider assets;

    Env() {
        auto& ctx = UIContext::get();
        ctx.renderer = &renderer;
        ctx.platform = &platform;
        ctx.assets   = &assets;
        reset();
    }

    ~Env() {
        auto& ctx = UIContext::get();
        ctx.renderer = nullptr;
        ctx.platform = nullptr;
        ctx.assets   = nullptr;
        Metrics::viewport = {1920.0f, 1080.0f};
    }

    void reset();   // clears UIContext, Theme, mock state
    void frame(std::shared_ptr<Layout> root, float dt = 1.0f / 60.0f);
    void click(std::shared_ptr<Layout> root, Vec2 pos);
};
```

### `reset()`

Clears:

- `Theme` (rules, keyframes, root).
- `UIContext` focus, capture, targets, flags, portals.
- `MockPlatform` state (viewport, time, pointer, events).
- `MockRenderer` counters and clip state.
- `Metrics::viewport` to `{1920, 1080}`.

This gives each test a clean slate.

### `frame(root, dt)`

Runs a single frame:

```cpp
void frame(std::shared_ptr<Layout> root, float dt = 1.0f / 60.0f) {
    auto& ctx = UIContext::get();
    ctx.beginFrame(dt);
    root->updateTree(dt);
    root->measure(platform.viewport.x, platform.viewport.y);
    root->arrange({0, 0, platform.viewport.x, platform.viewport.y});
    root->draw();
}
```

### `click(root, pos)`

Simulates a full click (press + release):

```cpp
void click(std::shared_ptr<Layout> root, Vec2 pos) {
    platform.pressLeft(pos);
    frame(root);
    platform.releaseLeft(pos);
    frame(root);
}
```

Two frames — one for the press, one for the release. Callbacks fire on
the appropriate frame.

---

## 17. Pitfalls

**Setting `renderer` after the first frame.**

The renderer's DPI scale is set in `beginFrame` from
`platform->dpiScale()`. If you swap the renderer after the first frame,
the new one might not have the correct scale until the next `beginFrame`.
Set the renderer before the first frame.

**Nulling a backend while nodes are alive.**

A `Layout`'s destructor calls `renderer->destroyTarget` if the node has
render targets. If you null the renderer while a `Layout` still exists
and is destroyed, the destructor checks for null and skips the call.
This is safe.

But if the renderer pointer is dangling (not null) and the renderer
object is destroyed, the destructor calls into freed memory. Always
null the pointer before destroying the object.

**`focusedNode` and `pointerCapture` are weak_ptrs.**

They don't keep the target alive. If the target is destroyed (e.g.
`removeFromParent`), the weak_ptr expires. Code that reads
`focusedNode.lock()` gets null and handles it.

**The RNG is not reseeded.**

`UIContext::rng` is default-constructed with a fixed seed. Every run
produces the same sequence. If you need real randomness, seed it
yourself:

```cpp
UIContext::get().rng.seed(std::random_device{}());
```

**`consumeClick` without `onClick`.**

If you manually call `consumeClick()` in a callback that isn't the
`onClick` handler, the current frame's `clickConsumed` is set, and any
subsequent `onClick` (in the same frame) is skipped. This is the
intended behavior — but be careful about where you call it.

**`time` is a `double`, `dt` is a `float`.**

`time += delta_time` where `delta_time` is a `float`. The cumulative
`time` is `double` to preserve precision over long sessions. `dt` is
`float` for simplicity.

**`activePortals` and `framePortals` swap, not copy.**

`activePortals = std::move(framePortals)` transfers ownership. The
`framePortals` vector is empty after the swap. If you hold a reference
to it, it's now empty.

**`topmostConsumer` is cleared at the start of each frame.**

Between `beginFrame` and `updateTree`, it's null. Don't read it during
that window.

**`pressTarget` persists across frames.**

Unlike `topmostConsumer`, `pressTarget` is only updated on the press
frame. It stays set until the release frame (then cleared at the end of
that frame's `updateTree`). So `isPressTarget(n)` returns true
throughout the press-release cycle.

**`viewportGeneration` is monotonic.**

It only increments, never resets. After a long session with many
resizes, it could overflow (after 4 billion frames with a resize every
frame). In practice, this never happens — the counter is compared for
equality, so even if it wraps, the comparison still works (as long as
no node's cached value happens to equal the wrapped value).

**`Metrics::viewport` is updated by `beginFrame`.**

Before the first `beginFrame`, `Metrics::viewport` has whatever value
was last set (default `{1280, 720}`). After the first `beginFrame`,
it's `platform->viewportSize()`. If you read `Metrics::viewport` before
the first frame, you get the default.

**`Metrics::viewport` is a global, not a `UIContext` field.**

It lives in `namespace ZenitUI::Metrics`. It's not part of
`UIContext`, but `beginFrame` writes to it. The separation is
historical — `Value::resolveH` reads `Metrics::viewport` directly, and
it needs to be accessible without the singleton.

**`Env::reset()` resets `Metrics::viewport` to a fixed value.**

This means tests that don't call `beginFrame` see `{1920, 1080}`, not
the mock platform's viewport. Tests that need the platform's viewport
should call `env.frame(root)` at least once.

**Concurrent access is undefined.**

The framework isn't thread-safe. `UIContext` fields are read and
written from the main thread only. If you need to log or query state
from another thread, copy the values or use your own synchronization.

**`IRenderer::supports` is the only feature detection.**

There's no `try { ... } catch` fallback for missing features. If a
backend returns `false` for `Effects`, all effect-related calls are
skipped. If it returns `true` but a shader is missing, the shader
application is a no-op (uniform lookups return `-1`).

**The `pointer` field is overwritten, not merged.**

If you want to accumulate pointer state across frames (e.g. track the
history of positions), copy it somewhere else. `UIContext::pointer` is
always the current frame's state.

**The `inputEvents` vectors are cleared each frame.**

Same reason. If you need to buffer keyboard input across frames, copy
it. The framework doesn't keep a history.

**You can't have two `UIContext` instances.**

The singleton has no "swap" API. Multi-window is not supported. If you
need two independent UIs, use two processes or a different framework.

**Setting `focusedNode` directly bypasses the focusable check.**

`requestFocus` checks `isFocusable()`. If you assign
`ctx.focusedNode = someNode` directly, no check is done. The node's
`isFocused` flag will be set on the next `updateFlags`, even if the
node isn't focusable. Use `requestFocus` for safety.

**`releaseFocus` doesn't notify the node.**

The previously focused node's `isFocused` flag is cleared on the next
`updateFlags` (because `hasFocus` returns false). No callback fires on
focus release. If you need to react, override `onUpdate` and check
`isFocused`.

**`UIContext::get()` is created on first call.**

The first call to `get()` constructs the singleton. If you call it
before setting the backend pointers, the pointers are null. If you call
it after, they're whatever you set. The order matters only for
initialization — once the pointers are set, `get()` returns the same
instance.

**The `activePortals` / `framePortals` swap happens in `beginFrame`.**

If you access them before `beginFrame` (e.g. at startup), they're both
empty. After the first `beginFrame`, `activePortals` has last frame's
portals (empty on frame 1) and `framePortals` is empty.

**`Metrics::viewport` is used for `vw` / `vh`, not `safeArea`.**

The safe area isn't subtracted from the viewport. If you need to
respect notches, do it manually in your layout (e.g. set a padding on
the root). The framework stores `safeArea` but doesn't apply it.

**`pointer.pos` is in logical coordinates.**

If the platform reports physical pixels, it should divide by
`dpiScale()` before returning. The Raylib backend does this:

```cpp
PointerState RaylibPlatform::pointer() {
    auto mp = GetMousePosition();
    float dS = dpiScale();
    PointerState s;
    s.pos = {mp.x / dS, mp.y / dS};
    // ...
}
```

A backend that returns physical coordinates would break hit-testing
against logical-space rects.

---

## 18. See also

- [Backend](../internals/09-backend.md) — how to implement the three
  interfaces.
- [Lifecycle](../internals/02-lifecycle.md) — how `beginFrame` fits
  into the frame.
- [Input System](../internals/07-input-system.md) — how focus, capture,
  and targets are used.
- [Debugging](../internals/10-debug.md) — inspecting `UIContext` state.
- [Getting Started](../user/01-getting-started.md) — wiring the
  backend pointers.

```
FINE FILE
```

---

Questo chiude la sezione `api/`. La documentazione completa è:

```
docs/
├── README.md
├── user/
│   ├── 01-getting-started.md
│   ├── 02-layout.md
│   ├── 03-widgets.md
│   ├── 04-zmarkup.md
│   ├── 05-zstyle.md
│   ├── 06-events.md
│   ├── 07-animations.md
│   ├── 08-portals.md
│   ├── 09-effects.md
│   └── 10-custom-widgets.md
├── internals/
│   ├── 01-architecture.md
│   ├── 02-lifecycle.md
│   ├── 03-style-system.md
│   ├── 04-animation-system.md
│   ├── 05-layout-engine.md
│   ├── 06-render-pipeline.md
│   ├── 07-input-system.md
│   ├── 08-scroll-system.md
│   ├── 09-backend.md
│   └── 10-debug.md
└── api/
    ├── Layout.md
    ├── Widgets.md
    ├── Unit.md
    ├── Style.md
    ├── Theme.md
    ├── ZMarkup.md
    ├── ZStyle.md
    ├── FilterRegistry.md
    └── UIContext.md
```