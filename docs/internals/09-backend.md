# Backend

ZenitUI doesn't draw anything itself. It produces a stream of abstract
draw calls, input queries, and asset lookups that are interpreted by a
**backend**. The framework ships a reference backend for
[Raylib](https://www.raylib.com/), but the core only depends on three
abstract interfaces:

- **`IRenderer`** — drawing primitives, text measurement, clip,
  transform, render targets, shader push/pop.
- **`IPlatform`** — window size, pointer, keyboard, timers, DPI, safe
  area.
- **`IAssetProvider`** — fonts, textures, shaders by string name.

These are declared in `UIContext.hpp`. `UIContext` holds pointers to
one of each, and the rest of the framework uses them exclusively.

This document covers what each interface must provide, the contract
each method obeys, and how to write a new backend.

---

## 1. The three interfaces at a glance

| Interface       | Direction | Called from                          | Owns                  |
|-----------------|-----------|--------------------------------------|-----------------------|
| `IPlatform`     | inbound   | `UIContext::beginFrame`              | window, input, time   |
| `IRenderer`     | outbound  | `Layout::draw`, filters              | framebuffer, state    |
| `IAssetProvider`| lookup    | style resolution, effect resolution  | fonts, textures, shaders |

**Platform** is called **once per frame**, at the start of `beginFrame`.
It returns the current input snapshot and viewport. It's the only source
of time, pointer, and keyboard state.

**Renderer** is called **during draw**, once per node, and inside
filters. Every method is either a primitive draw or a stack operation
(clip, transform, effect, target). The renderer maintains its own state
stacks and must keep them balanced.

**Asset provider** is called from a few well-defined places:

- `Layout::resolveFont` — when a node's style has a `font` property.
- `Layout::resolveBgTexture` — when a node has a `background-texture`.
- `Layout::resolveEffectIfNeeded` — when a node has an `effect`.
- `FilterRegistry::applyFilter` — when a filter's shader is needed.

It's a lookup by string name, returning an opaque handle.

---

## 2. `IPlatform`

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

Eight methods, all called from `UIContext::beginFrame`. None of them
draw anything, and none of them mutate state inside the framework.

### 2.1 `viewportSize()`

Returns the **logical** viewport size, in UI pixels. On a HiDPI display
or a mobile device with a device pixel ratio, this is the size the UI
should treat as "the screen".

The framework uses this value for:

- `Metrics::viewport`, used to resolve `vw` / `vh` units.
- The root's arrange rect (`{0, 0, viewport.x, viewport.y}`).
- Media query evaluation.

The returned value should be stable across a single frame — calling it
twice in the same `beginFrame` should return the same value.

### 2.2 `pointer()`

Returns the current pointer state:

```cpp
struct PointerState {
    Vec2 pos;                // logical coordinates
    bool down;               // button currently held
    bool pressed;            // rising edge this frame
    bool released;           // falling edge this frame
    bool rightDown;
    bool rightPressed;
    bool rightReleased;
    float wheelY;            // wheel delta this frame
};
```

The `pos` should be in the same logical coordinate space as
`viewportSize`. If the platform reports physical pixels, divide by
`dpiScale()` before returning.

The `pressed` / `released` flags are **edge events** — they should be
true only on the frame the transition happens. The framework reads them
once per frame and relies on this one-frame semantics.

`wheelY` is the wheel delta for this frame. A positive value
conventionally means "scroll up" (or "away from user"), matching the
Raylib convention. The scroll system negates it internally.

### 2.3 `time()`

Returns a monotonically increasing time in seconds, as a `double`.

This is used by:

- `UIContext::time`, which is the timeline for absolute-tick animations
  (parts) and the `startTime` of CSS animations.
- `StyleResolver::partFor` for part transition timing.

The value doesn't need to start at zero — it just needs to be monotonic
and to advance at wall-clock rate. Raylib returns `GetTime()` directly.

### 2.4 `shiftHeld()`

Returns whether `Shift` is currently down. Used by:

- Tab navigation (reverse direction).
- Scroll (horizontal wheel).
- `Slider` (fine step).

Only `Shift` is exposed. Other modifiers (Ctrl, Alt, Meta) aren't part
of the interface. Adding one means extending the interface and updating
every backend.

### 2.5 `pollInputEvents()`

Returns the keyboard events for this frame:

```cpp
struct InputEvents {
    std::vector<int> chars;   // Unicode codepoints typed this frame
    std::vector<int> keys;    // special keys pressed this frame (rising edge)
    std::vector<int> held;    // special keys currently down (level)
};
```

Three channels:

- **`chars`** — printable characters typed. From the platform's text
  input (e.g. `GetCharPressed` in Raylib). These are the actual
  codepoints, not the physical keys pressed.
- **`keys`** — special key codes that were **pressed this frame**
  (rising edge). The codes are `Key::Backspace`, `Key::Delete`,
  `Key::Enter`, `Key::Escape`, `Key::Left`, `Key::Right`,
  `Key::Home`, `Key::End`, `Key::Tab`, `Key::Space` — see
  `CoreTypes.hpp` for the full list.
- **`held`** — special key codes that are **currently down** (level).
  Used for key repeat.

The distinction between `keys` and `held` matters: `keys` fires once
per press, `held` stays true while the key is down. A `TextInput`
uses `keys` for `Escape` (single action) and `held` for `Backspace`
(repeat).

### 2.6 `dpiScale()`

Returns the DPI scale factor, e.g. `1.0` on standard displays, `2.0` on
Retina.

Used by the framework to:

- Convert logical coordinates to physical pixels for the renderer
  (Raylib's scissor operates in physical pixels).
- Adjust text measurement (`RaylibRenderer::measureText` multiplies by
  the scale, then divides the result back).

The framework handles most of the DPI logic in the renderer, not in the
platform. The platform just reports the value.

### 2.7 `safeArea()`

Returns the safe area insets (notch, rounded corners, etc.). Currently
only reported; the framework doesn't apply them automatically. Future
extensions might use them in layout (e.g. `safe-area-top: 1` as a
padding value).

For desktop, return `{}` (all zeros).

### 2.8 What `IPlatform` should **not** do

- Don't draw anything. `IPlatform` is pure input.
- Don't own the main loop. The user's `main()` drives the loop.
- Don't create the window. The user creates it before initializing
  ZenitUI (in the Raylib case, `InitWindow` is called by the user).
- Don't poll more than once per frame. The framework calls
  `pointer()` and `pollInputEvents()` exactly once per `beginFrame`.

---

## 3. `IRenderer`

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

    // State stacks
    virtual void pushTransform(const Transform2D& t) = 0;
    virtual void popTransform() = 0;
    virtual void pushEffect(EffectHandle e) = 0;
    virtual void popEffect() = 0;
    virtual void pushClip(Rect r) = 0;
    virtual void popClip() = 0;
    virtual Rect getClipRect() const = 0;

    // Render targets
    virtual TargetHandle createTarget(int w, int h) = 0;
    virtual void destroyTarget(TargetHandle t) = 0;
    virtual void pushTarget(TargetHandle t) = 0;
    virtual void popTarget() = 0;
    virtual void drawTarget(TargetHandle t, Rect dst, Color tint) = 0;
    virtual void clearTarget(TargetHandle t, Color c) = 0;
    virtual bool inTarget() const = 0;

    // Effect uniforms
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

This is the largest interface, but each method is small.

### 3.1 Primitives

**`fillRect(r, c)`** — draw a filled rectangle. `c` already has the
node's opacity applied by the caller (via `.withAlpha(op)`).

**`fillRoundedRect(r, radiusPx, c)`** — draw a filled rectangle with
rounded corners. `radiusPx` is in logical pixels; if larger than
`min(r.width, r.height) * 0.5`, it should be clamped (or the caller
clamps, which it does — see `renderChrome`).

**`fillCircle(center, radius, c)`** — draw a filled circle.

**`strokeRect(r, thickness, c)`** — draw a rectangle outline of the
given thickness. The stroke should be centered on the rect's edge (or
drawn inside — the exact convention is up to the backend, but it must
be consistent).

**`strokeRoundedRect(r, radiusPx, thickness, c)`** — same, rounded.

These five are what `renderChrome` and `renderContent` use. Every
widget draws with them.

### 3.2 Textures

**`drawTexture(t, src, dst, tint)`** — draw a texture from the `src`
sub-rectangle to the `dst` rectangle, multiplied by `tint`.

**`drawNineSlice(t, s, dst, tint)`** — draw a nine-slice texture
(corners preserved, edges stretched, center stretched). `s` contains
the four inset values.

`t` is an opaque `TextureHandle`. The handle is valid only if
`t.valid()` returns true (`id != 0`).

### 3.3 Text

**`drawText(f, s, pos, size, spacing, c)`** — draw a text string at
`pos` with the given font, size, and letter spacing.

**`measureText(f, s, size, spacing)`** — return the size the text would
occupy if drawn.

Both take a `FontHandle`. If `f.id == 0`, the backend should use its
default font (Raylib's `GetFontDefault`).

Text measurement is **critical**: it drives the `Text` widget's
intrinsic size, which drives layout. The measurement must match what
`drawText` actually produces, or layouts will be wrong.

### 3.4 Transforms

**`pushTransform(t)`** / **`popTransform()`** — push and pop a
`Transform2D` on the renderer's transform stack.

```cpp
struct Transform2D {
    Vec2 pivot;
    Vec2 translate;
    float rotationDeg;
    float scale;
};
```

The transform is applied as: translate to `pivot + translate`, rotate,
scale, translate back by `-pivot`. This is the standard
pivot-centered transform.

Calls must be balanced. The framework always pairs them, but a backend
should handle unbalanced calls gracefully (e.g. by returning early on
`popTransform` if the stack is empty).

### 3.5 Effects (shaders)

**`pushEffect(e)`** / **`popEffect()`** — push and pop a shader on the
effect stack.

An effect is a shader that affects subsequent draw calls until popped.
Nested pushes are allowed — the top of the stack is the active shader.

**`setEffectFloat`** / **`setEffectVec2`** / **`setEffectVec4`** — set
a uniform on the currently active effect. The `name` is the uniform
name in the shader source. If the uniform doesn't exist, the call
should be a no-op (not an error).

### 3.6 Clipping

**`pushClip(r)`** / **`popClip()`** — push and pop a clip rect on the
clip stack.

The clip should intersect with the current clip (nested clips compose
by intersection). The rect is in the **current transform's** coordinate
space — the backend applies any active transforms to convert it to
screen space.

**`getClipRect()`** — return the current effective clip rect in
**logical** coordinates. If no clip is active, return the full viewport
(`{0, 0, Metrics::viewport.x, Metrics::viewport.y}`).

The clip is used for frustum culling in `draw`. A node outside the
current clip is skipped entirely.

### 3.7 Render targets

**`createTarget(w, h)`** — allocate a render target of the given size.
Returns a `TargetHandle`. If allocation fails, return an invalid handle
(`id == 0`).

**`destroyTarget(t)`** — free a target. Called in `~Layout` and when a
layer's region size changes.

**`pushTarget(t)`** — redirect subsequent draws into the target. The
coordinate system resets to `(0, 0)` at the top-left of the target.
The previous transform stack, clip stack, and effect stack should be
preserved but not applied until the target is popped.

**`popTarget()`** — return drawing to the framebuffer.

**`drawTarget(t, dst, tint)`** — draw a target's content into the
current framebuffer at `dst`, multiplied by `tint`.

**`clearTarget(t, c)`** — fill a target with a solid color.

**`inTarget()`** — return true if currently inside a `pushTarget`.

The render target API is the most backend-specific. Raylib's
`RenderTexture2D` has a flipped Y-axis relative to the framebuffer, so
`drawTarget` uses a negative source height to flip it back. Other
backends might not need this.

**Important**: the framework doesn't support nested targets. The check
in `Layout::draw`:

```cpp
const bool isLayer =
    !renderStyle.filters.empty() &&
    renderer->supports(Feature::Effects) &&
    rect.width > 0.0f && rect.height > 0.0f &&
    !renderer->inTarget();
```

A backend that supports nested targets (via a target stack) could lift
this restriction, but the framework doesn't currently use it. For now,
`inTarget()` must accurately report whether a target is active.

### 3.8 Frame

**`beginFrame()`** / **`endFrame()`** — frame-level setup. Called once
per frame, around the whole draw pass.

In the Raylib backend, `beginFrame` pushes a DPI matrix if
`dpiScale_ != 1.0`, and `endFrame` pops it. A different backend might
use these for clearing, present-swap, or other per-frame work.

The two must be paired. The framework always calls them around
`root->draw()`.

**`setDpiScale(scale)`** — set the current DPI scale factor. Called
from `beginFrame` after reading `platform->dpiScale()`.

The renderer uses the scale to convert logical coordinates to physical
pixels for operations that require them (e.g. scissor). A backend that
works entirely in logical coordinates can ignore this.

### 3.9 Feature detection

**`supports(f)`** — return whether the backend supports a feature.

```cpp
enum class Feature {
    Effects,        // pushEffect / popEffect / setEffect*
    NestedTargets,  // pushTarget while already in a target
};
```

Currently only `Effects` is checked by the framework. The Raylib backend
returns `true` for it. A software renderer might return `false`, which
would make all `filter:` and `effect:` properties no-ops.

`NestedTargets` is declared but not used. It's a placeholder for a
future extension.

---

## 4. `IAssetProvider`

```cpp
class IAssetProvider {
public:
    virtual ~IAssetProvider() = default;
    virtual FontHandle    getFont(std::string_view name) = 0;
    virtual TextureHandle getTexture(std::string_view name) = 0;
    virtual EffectHandle  getEffect(std::string_view name) = 0;
};
```

Three methods, all lookups by string name. Return an invalid handle
(`id == 0`) if the name isn't found.

The provider is populated out-of-band (by the user, before the first
frame) with load methods. The interface itself only exposes the getters.

### 4.1 Raylib's implementation

`RaylibAssetProvider` adds `loadFont` / `loadTexture` / `loadEffect`:

```cpp
bool loadFont(std::string name, const char* path, int baseSize = 96) {
    ::Font f = LoadFontEx(path, baseSize, nullptr, 0);
    if (f.texture.id == 0) return false;
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    fonts[std::move(name)] = renderer.registerFont(f);
    return true;
}
```

Each load method calls the corresponding Raylib loader, registers the
result with the renderer (which assigns an ID and stores the resource
in a static map), and stores the handle under the name.

The renderer's `registerFont` / `registerTexture` / `registerEffect`
methods return the handle that the provider exposes.

### 4.2 Naming

Names are **case-sensitive** strings. `"calibri"` and `"Calibri"` are
different.

The convention is lowercase, no spaces. See `main.cpp` in the demo:

```cpp
backendAssets.loadFont("calibri", "assets/calibrib.ttf", 64);
backendAssets.loadFont("mont", "assets/mont.otf", 64);
backendAssets.loadTexture("bubble", "assets/bubble.png");
backendAssets.loadEffect("blur", "assets/blur.fs");
```

### 4.3 Load order

Assets must be loaded **before the first frame**. The framework doesn't
lazy-load — if a `.zstyle` rule references `font: mont` and `mont`
isn't registered, the resolution falls back to the default font
silently.

Loading mid-run is possible but not recommended. If you do, be aware
that style resolution caches handles — a font that's added after a node
resolved its style won't be picked up until the node re-resolves (e.g.
via a state change).

### 4.4 Lifetime

The provider owns the resources. It's the user's responsibility to
destroy them (or to let the provider's destructor do it). The Raylib
backend's provider doesn't currently unload its resources — a small
leak at shutdown, harmless for a typical game.

---

## 5. The Raylib backend

`RaylibBackend.hpp` / `.cpp` implement the three interfaces for Raylib.

### 5.1 Structure

```cpp
class RaylibPlatform : public IPlatform { /* ... */ };
class RaylibRenderer : public IRenderer { /* ... */ };
class RaylibAssetProvider : public IAssetProvider { /* ... */ };
```

No inheritance between them. They're independent classes that the user
constructs and wires into `UIContext`.

### 5.2 Resource management

`RaylibRenderer` uses a static (file-local) `RaylibResources` struct to
store the actual resources:

```cpp
struct RaylibResources {
    std::unordered_map<uint32_t, ::RenderTexture2D> targets;
    std::unordered_map<uint32_t, ::Texture2D> textures;
    std::unordered_map<uint32_t, ::Font> fonts;
    std::unordered_map<uint32_t, ::Shader> shaders;
    uint32_t nextId{1};
};
static RaylibResources res;
```

Handles are `{id, ...}` structs. The renderer maps IDs to resources.
This indirection keeps the framework's handles opaque and small.

The static `res` means there's effectively one renderer per process.
This is fine for the framework's single-window use case, but would be
a problem if you wanted multiple windows with multiple renderers.

### 5.3 Transform stack

Maintained as a `std::vector<Transform2D> transformStack`. Used by
`transformClipToScreen` to compute the screen-space AABB of a clip
rect. The actual rendering uses Raylib's `rlPushMatrix` / `rlPopMatrix`.

### 5.4 Clip stack

Maintained as a `std::vector<Rect> clipStack` and a `bool clipActive`
with the current `currentClip`. `pushClip` calls `BeginScissorMode`
with the transformed and DPI-scaled rect. `popClip` calls it again
with the restored rect (or `EndScissorMode` if the stack is empty).

`pushClip` is a no-op inside a target (`insideTarget_ == true`). This
is because Raylib's render targets don't support scissor. The target's
bounds act as the clip instead.

### 5.5 Effect stack

Maintained as a `std::vector<EffectHandle> effectStack_`. `pushEffect`
calls `BeginShaderMode` with the shader. `popEffect` calls
`EndShaderMode`, then re-pushes the previous shader if there's one on
the stack.

The re-push is because Raylib's shader mode is not stackable by default
— it's a "current state" not a "stack". The renderer implements the
stack manually.

### 5.6 Target management

`pushTarget` calls `rlPushMatrix`, `rlLoadIdentity` (so the target's
coordinate system starts at `(0, 0)`), sets `insideTarget_ = true`,
ends any active scissor, and calls `BeginTextureMode`.

`popTarget` calls `EndTextureMode`, restores the matrix with
`rlPopMatrix`, sets `insideTarget_ = false`, and re-issues the scissor
if one was active.

The `rlLoadIdentity` is important: it isolates the target from the
parent's transform. A draw call inside the target uses only the
transforms pushed within the target.

### 5.7 DPI

`beginFrame` pushes a scale matrix if `dpiScale_ != 1.0f`:

```cpp
void RaylibRenderer::beginFrame() {
    if (dpiScale_ != 1.0f) {
        rlPushMatrix();
        rlScalef(dpiScale_, dpiScale_, 1.0f);
    }
}
```

`endFrame` pops it. The scale is applied to all coordinates passed to
the backend, so the backend's logical coordinates are automatically
converted to physical pixels.

`pushClip` applies the scale manually to the scissor:

```cpp
int x = (int)std::floor(currentClip.x * dpiScale_);
int y = (int)std::floor(currentClip.y * dpiScale_);
int w = (int)std::ceil(currentClip.width * dpiScale_);
int h = (int)std::ceil(currentClip.height * dpiScale_);
```

Because scissor operates in physical pixels (Raylib's API), not in the
transformed coordinate space.

`measureText` divides the result by `dpiScale_` to return logical
pixels:

```cpp
Vec2 RaylibRenderer::measureText(FontHandle f, std::string_view s,
                                  float size, float spacing)
{
    ::Font font = res.fonts.count(f.id) ? res.fonts[f.id] : GetFontDefault();
    std::string str(s);
    auto v = MeasureTextEx(font, str.c_str(),
                           size * dpiScale_, spacing * dpiScale_);
    return {v.x / dpiScale_, v.y / dpiScale_};
}
```

This is because Raylib's `MeasureTextEx` works in physical pixels, but
the framework expects logical pixels.

### 5.8 Text rendering

`drawText` doesn't apply DPI manually — the `beginFrame` scale matrix
takes care of it. So the text is drawn at `size` (logical), and the
matrix scales it to `size * dpiScale_` (physical).

But `measureText` uses the physical size for the measurement, then
divides back. The result is consistent: `drawText` at logical size `s`
has the same physical size as `measureText` at logical size `s`.

### 5.9 `drawTarget` and the Y-flip

```cpp
void RaylibRenderer::drawTarget(TargetHandle t, Rect dst, Color tint) {
    if (res.targets.count(t.id)) {
        ::Texture2D tex = res.targets[t.id].texture;
        ::Rectangle src = {0.0f, 0.0f, (float)tex.width, -(float)tex.height};
        DrawTexturePro(tex, src, rl(dst), {0, 0}, 0.0f, rl(tint));
    }
}
```

Raylib's render textures are stored **upside down** relative to the
screen. The `-tex.height` in the source rect flips the texture during
the draw, restoring the correct orientation.

This is a Raylib-specific quirk. Other backends (e.g. OpenGL FBOs
rendered in the correct orientation) wouldn't need it.

---

## 6. Writing a new backend

The process for porting ZenitUI to a new graphics library:

### 6.1 Implement `IPlatform`

This is usually the easiest. Map your library's input and time APIs to
the eight methods. The main gotchas:

- Convert pointer position to logical pixels if your library reports
  physical.
- Ensure `pressed` / `released` are one-frame edges.
- Map your key codes to the `Key::*` constants.
- Return the correct DPI scale.

### 6.2 Implement `IRenderer`

This is the largest. Structure it like the Raylib backend:

1. A resources struct (maps, next ID).
2. A transform stack (for `transformClipToScreen`).
3. A clip stack (for nested clips).
4. An effect stack (for nested shaders).
5. A target state (for `inTarget()`).

The methods are individually simple, but the interactions between them
(clipping inside transforms, effects inside targets, etc.) require care.

Common pitfalls:

- **Forgetting to intersect clips.** Nested `pushClip` calls must
  produce the intersection, not the last clip.
- **Forgetting to apply transforms to clips.** A clip rect is in the
  current transform's space; converting it to screen space is what
  makes the clip follow the transform.
- **Not resetting state on `pushTarget`.** Drawing inside a target
  should start from a clean coordinate system.
- **Not restoring state on `popTarget`.** The clip that was active
  before the target must be re-applied.
- **Not handling `dpiScale`.** If your backend works in physical
  pixels, you need to scale the clip and divide the text measurement.

### 6.3 Implement `IAssetProvider`

Simple. Your library's font / texture / shader loading methods map
directly to `loadFont` / `loadTexture` / `loadEffect`, plus a registry
to store the handles by name.

### 6.4 Wire into `UIContext`

```cpp
MyRenderer   renderer;
MyPlatform   platform;
MyAssets     assets(renderer);

UIContext::get().renderer = &renderer;
UIContext::get().platform = &platform;
UIContext::get().assets   = &assets;
```

All three must be set before the first frame. The framework doesn't
validate this — if any is null, the first frame will crash or
misbehave silently.

### 6.5 Handling unsupported features

If your backend can't implement `pushEffect` / `popEffect` (e.g. a
software renderer without shaders), return `false` from
`supports(Feature::Effects)`. The framework will skip all shader code
paths, and `filter:` / `effect:` properties become no-ops.

Similarly, `NestedTargets` should return `false` unless you actually
support nested render targets.

The `supports` method is the **only** feature-detection mechanism. The
framework doesn't try-catch or probe — it just asks and trusts the
answer.

### 6.6 DPI strategy

If your backend works entirely in logical coordinates (e.g. a vector
renderer that doesn't care about physical pixels), you can ignore
`dpiScale` — always return `1.0f` from `setDpiScale`, ignore it in
`pushClip`, and return the measured text size directly.

If your backend uses physical pixels (OpenGL, Vulkan, Metal, Raylib),
you need to handle the scale, following the Raylib pattern.

### 6.7 The Y-flip question

Raylib's render textures are Y-flipped relative to the framebuffer. If
your backend has the same quirk, you need the `-tex.height` trick in
`drawTarget`. If not, just use `{0, 0, tex.width, tex.height}`.

The framework doesn't care — it just calls `drawTarget` and expects the
target's content to appear at `dst` with the same orientation as when
it was drawn.

---

## 7. Testing a backend

ZenitUI is designed to be testable without a real backend. The
`tests/Mocks.hpp` file provides:

- **`MockRenderer`** — records calls, provides deterministic text
  measurement, tracks clip intersection.
- **`MockPlatform`** — scriptable pointer, input events, viewport.
- **`MockAssetProvider`** — returns empty handles.
- **`Env`** — fixture that installs the mocks, clears `Theme`, and
  provides `frame()` / `click()` helpers.

You can write a test suite for a new backend by:

1. Implementing the three interfaces.
2. Running the framework's existing tests against them.
3. Adding backend-specific tests for the tricky parts (clip
   intersection, transform composition, text measurement consistency).

The existing tests use `MockRenderer` and pass without a window. A new
backend should be able to pass the same tests.

### 7.1 Verifying text measurement

The most important consistency check: `measureText` and `drawText` must
agree on the size. If `measureText(s)` returns `100x20` but `drawText`
actually draws `110x22`, the layout will be wrong.

The test for this is to draw the text to a target, read back the pixels,
and measure the actual bounding box. The Raylib backend doesn't have
this test — it trusts the library.

### 7.2 Verifying clip and transform

The clip and transform interactions are the trickiest parts of a
backend. A common failure mode: clipping to a rect that's been
transformed by a scale, but the clip doesn't follow the scale.

The test: push a `scale: 2` transform, push a clip, draw a full-rect
fill, and read back the pixels. The filled area should be the clip
scaled by 2.

---

## 8. Common tasks

### 8.1 Adding a new feature to the framework

If a new feature needs backend support:

1. Add a value to the `Feature` enum.
2. Update the framework to check `supports(Feature::YourFeature)`.
3. Implement the feature in the Raylib backend.
4. Update other backends to return `false` (or implement it).

### 8.2 Adding a new primitive

1. Add a method to `IRenderer`.
2. Implement in `RaylibRenderer`.
3. Use it in the framework (in `renderChrome`, `renderContent`, or a
   filter).

### 8.3 Adding a new platform event

1. Add the field to `PointerState` or `InputEvents` (in
   `CoreTypes.hpp`).
2. Populate it in the platform's `pointer()` or `pollInputEvents()`.
3. Handle it in the framework (`InputController` or `Layout`).
4. Map it in the Raylib backend (e.g. a new `IsKeyDown` check).

### 8.4 Supporting a second window

The framework assumes a single window. `UIContext::get()` is a
singleton, and the Raylib backend uses a static resources struct. To
support multiple windows, you'd need:

1. A per-window `UIContext` (or a context passed explicitly).
2. A per-window renderer (no static resources).
3. A window manager on top.

This is a significant refactor. The current design is intentionally
single-window.

---

## 9. Pitfalls

**Forgetting to call `setDpiScale`.**

The user's `main()` must call `UIContext::get().renderer->setDpiScale(...)`
if they want DPI scaling. The framework's `beginFrame` calls it
automatically from `platform->dpiScale()`, but only if the renderer is
set before the first frame. If you set the renderer late, the scale
might be wrong.

**Handles are `uint32_t` IDs, not pointers.**

A `FontHandle` is `{id}`. The backend maps IDs to actual resources. This
keeps the framework's types small and copyable, but it means you can't
compare handles directly to resources — you need the backend's lookup.

The `valid()` method on handles checks `id != 0`. So `id = 0` is the
"invalid" / "default" handle. For fonts, `id = 0` means "use the
backend's default font". For textures and effects, `id = 0` means
"no resource".

**Assets are never unloaded.**

The `RaylibAssetProvider` doesn't have an unload method. Resources are
leaked at shutdown. For a game, this is harmless (the OS reclaims at
process exit). For a long-running app with dynamic loading, you'd need
to add explicit unloading.

**The Raylib renderer uses global state.**

`static RaylibResources res;` is a file-local static. This means:

- Only one `RaylibRenderer` per process can have valid resources.
- Creating a second renderer doesn't create a second set of resources.
- Destroying the first renderer doesn't clean up (the static persists).

This is a simplification for the single-window use case.

**`pushClip` inside a target is a no-op.**

A filtered node inside a scroll container: the filter pushes a target,
then the scroll container's clip is skipped inside the target. The
target's size acts as the clip. If the target is larger than the
visible region (because the transform expanded it), the content might
be visible outside the container's bounds.

This is a known limitation. In practice, the target size is the region,
which is the intersection of the transformed rect and the outer clip,
so the target is exactly the visible area. The no-op clip is fine.

**Effects don't apply to text unless the backend supports it.**

`drawText` calls `DrawTextEx` in Raylib, which respects the current
shader mode. So a node with `effect: hueShift` and text content will
apply the shader to the text too. But the shader must expect text
coordinates — many shaders are written for full-screen quads and won't
work on text.

**The DPI scale is global, not per-node.**

A backend can't have different DPI scales for different nodes. If you
need per-node scaling, use the `scale` transform property, not the DPI
scale.

**Text rendering with `spacing = 0` is backend-dependent.**

The framework passes the `letterSpacing` value from the style. If it's
zero, `DrawTextEx` uses its default spacing (which might not be zero).
The framework's `letterSpacing` default is `2.0`, not `0`, so this is
usually not an issue.

**Measure text with a font that failed to load.**

If `loadFont` returns `false` (file not found), the name isn't
registered. A style that references it falls back to the default font
(`f.id == 0`). `drawText` with `f.id == 0` uses `GetFontDefault()`. The
text renders in Raylib's built-in font. No error is logged (by default).

**Clip rects are not transformed for the current target.**

Inside a render target, `pushClip` is a no-op. So the clip rect isn't
transformed. This is fine because the target is clipped by its bounds.
But if you were to lift the "no nested targets" restriction, you'd need
to handle clips inside targets differently.

**`drawTarget` with a target that was destroyed.**

If `destroyTarget` is called and then `drawTarget` with the same
handle, the map lookup fails and nothing is drawn. No crash — but the
content is missing. The framework avoids this by only destroying targets
in `~Layout` or right before recreating them.

**The `beginFrame` / `endFrame` pair is not a transaction.**

The framework calls `renderer->beginFrame()` before `root->draw()` and
`renderer->endFrame()` after. If `draw` throws or returns early, the
`endFrame` is still called (in `renderFrame`). This is correct — the
backend's frame-level state must be reset regardless.

If a backend's `beginFrame` allocates something, it must be freed in
`endFrame`, even if `draw` was empty.

**Shaders are loaded from file, not from source.**

`RaylibAssetProvider::loadEffect(name, fsPath)` calls
`LoadShader(nullptr, fsPath)`. The vertex shader defaults to Raylib's
standard one. To use a custom vertex shader, pass a `vsPath`.

The framework doesn't support loading shaders from source strings.
You'd need to write them to disk first, or extend the provider.

**The framework doesn't handle window resize explicitly.**

A window resize changes `viewportSize()`, which changes
`Metrics::viewport`, which triggers `viewportGeneration++`, which
triggers a style re-resolution. The render targets allocated by the
framework (for filters, canvas) keep their old size until the next
`drawLayer` sees a size mismatch and reallocates. This usually works,
but there can be a frame of incorrect rendering during the transition.

The Raylib backend doesn't handle window resize in `beginFrame` — it
just reports the new size. The framework's logic handles the rest.

**The backend doesn't have a "clear screen" method.**

The user's `main()` calls `ClearBackground` (Raylib) before
`root->renderFrame()`. The framework doesn't clear the framebuffer
itself. If you forget to clear, you'll see the previous frame's content
bleeding through.

For a new backend, the user is responsible for clearing. The framework
assumes a cleared framebuffer at the start of `draw`.

**The framework doesn't handle window close.**

`WindowShouldClose()` is checked in the user's loop. When the window
closes, the loop exits, and the user's `main()` cleans up (via
destructors). The framework doesn't have a "shutdown" method — cleanup
is RAII-based.

The `UIContext::renderer` pointer becomes dangling after the renderer
is destroyed, but the framework isn't called anymore (the main loop is
over), so this is harmless.

**Multithreading is not supported.**

The framework assumes all backend calls happen on the main thread.
`UIContext` is a singleton without locking. Backends that do async work
(e.g. loading assets in a background thread) must synchronize their
own state.

**The `supports(Feature::Effects)` check is done in multiple places.**

In `Layout::draw`, `drawInline`, `drawLayer`. If a backend returns
`false`, all effect-related calls are skipped. This is the correct
behavior but means the check is repeated. A future optimization could
cache the result, but the `supports` call is cheap (a virtual call
returning a bool).

**The `inTarget()` check prevents filtering inside a filter.**

A node with `filter:` inside another node with `filter:` falls back to
inline rendering (no filter on the inner node). This is a Raylib
limitation, exposed as a general framework rule. If a new backend
supports nested targets, it can override `supports(Feature::NestedTargets)`
— but the framework doesn't currently check that feature, so it wouldn't
help.

To lift the restriction, the check would need to be:

```cpp
const bool isLayer =
    !renderStyle.filters.empty() &&
    renderer->supports(Feature::Effects) &&
    rect.width > 0.0f && rect.height > 0.0f &&
    (!renderer->inTarget() || renderer->supports(Feature::NestedTargets));
```

Not currently implemented, but a clear extension point.