# `FilterRegistry` — API Reference

The registry that maps filter names (used in `.zstyle`'s `filter:`) to
filter definitions and their rendering pipelines.

- **Header**: `FilterRegistry.hpp`
- **Namespace**: `ZenitUI`

For usage examples, see [Effects](../user/09-effects.md). For the
render pipeline that invokes filters, see
[Render Pipeline](../internals/06-render-pipeline.md).

---

## 1. Overview

A filter is a post-processing pass applied to a subtree. The subtree is
rendered into a render target (a "layer"), then the filter's pipeline
processes that target and composites the result onto the framebuffer.

The registry holds **filter definitions** — named descriptors that tell
the framework:

- Which shader to use.
- What pattern (single-pass, separable, silhouette, custom) to run.
- How to map CSS arguments to shader uniforms.

A `.zstyle` rule references a filter by name:

```css
.card {
    filter: blur(4px);
}
```

The `FilterRegistry` looks up `"blur"`, gets its `FilterDef`, and
runs the pipeline.

---

## 2. `FilterPattern`

```cpp
enum class FilterPattern {
    SinglePass,   // push shader, draw src → region
    Separable,    // 2 pass (H, V) with a direction uniform
    Silhouette,   // shadow color, two draws (shadow then src)
    Custom        // arbitrary lambda
};
```

| Pattern | Passes | Description |
|---------|--------|-------------|
| `SinglePass` | 1 | A single draw with the shader. The framework sets `texSize`. |
| `Separable` | 2 | Horizontal then vertical pass, with `direction = {1,0}` and `{0,1}`. Uses `layerScratch_` for the intermediate. |
| `Silhouette` | 2 draws | Draws the subtree offset (colored by the shader), then the original on top. |
| `Custom` | N | The registered lambda runs with full control. |

---

## 3. `FilterParamType`

```cpp
enum class FilterParamType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Color
};
```

The type of a shader uniform, and how many CSS arguments it consumes.

| Type | Consumes | Set with |
|------|----------|----------|
| `Float` | 1 arg | `setEffectFloat` |
| `Vec2` | 2 args | `setEffectVec2` |
| `Vec3` | 3 args | `setEffectVec4` (values scaled to `[0,1]` then to `uint8_t`) |
| `Vec4` | 4 args | `setEffectVec4` (same) |
| `Color` | 1 arg (`#RRGGBB[AA]`) | `setEffectVec4` |

**Note:** `Vec3` and `Vec4` interpret their float arguments as
normalized `[0, 1]` values and convert them to `uint8_t` by
multiplying by 255. The shader receives `vec4` in `[0, 255]` — it
should normalize back if needed. This is a known quirk.

---

## 4. `FilterParam`

```cpp
struct FilterParam {
    std::string     uniform;
    FilterParamType type{FilterParamType::Float};
    int             argIndex{0};
    float           fdef{0.0f};      // default for Float
    Vec2            vdef{};          // default for Vec2
    Color           cdef{Colors::White}; // default for Color
};
```

Describes one uniform to set from the CSS arguments.

| Field | Description |
|-------|-------------|
| `uniform` | The uniform name in the shader. |
| `type` | The type (see §3). |
| `argIndex` | Which CSS argument to read (0-based). |
| `fdef` / `vdef` / `cdef` | Default values per type. |

### Constructors

```cpp
// Float
FilterParam(std::string u, FilterParamType t, int ai, float fd);

// Color
FilterParam(std::string u, FilterParamType t, int ai, Color cd);
```

Only these two convenience constructors exist. For `Vec2` / `Vec3` /
`Vec4`, use aggregate initialization:

```cpp
FilterParam{.uniform = "dir", .type = FilterParamType::Vec2,
            .argIndex = 0, .vdef = {1.0f, 0.0f}}
```

### Example

For a `blur(radius)` filter:

```cpp
FilterParam{"radius", FilterParamType::Float, 0, 4.0f}
```

- `radius` — the shader uniform.
- `Float` — a single float.
- `0` — read the CSS arg at index 0 (`4px` in `blur(4px)`).
- `4.0f` — the default if no arg is given.

---

## 5. `FilterDef`

```cpp
struct FilterDef {
    std::string              name;
    std::string              shader;
    FilterPattern            pattern{FilterPattern::SinglePass};
    std::vector<FilterParam> params;
    FilterCustomFn           custom;   // used if pattern == Custom
};
```

A registered filter descriptor.

| Field | Description |
|-------|-------------|
| `name` | The name used in `.zstyle` (`"blur"`). |
| `shader` | The asset name registered with `IAssetProvider` (`"blur"`). |
| `pattern` | The pipeline pattern. |
| `params` | The uniform bindings. |
| `custom` | The lambda for `Custom` patterns. |

The `name` and `shader` are often the same string, but they're
independent — you can have a filter named `"soft-shadow"` that uses a
shader named `"dropShadow"`.

### Example

```cpp
FilterDef{
    .name    = "sepia",
    .shader  = "sepia",
    .pattern = FilterPattern::SinglePass,
    .params  = {
        {"amount", FilterParamType::Float, 0, 0.7f}
    }
}
```

---

## 6. `FilterContext`

```cpp
struct FilterContext {
    IRenderer*       renderer{nullptr};
    Layout*          node{nullptr};
    TargetHandle     src{};
    TargetHandle     scratch{};
    Rect             region{};    // where to draw on the framebuffer
    Rect             dst{};       // {0, 0, tw, th}: target coordinates
    float            opacity{1.0f};
    const FilterRef* ref{nullptr};
};
```

The context passed to a filter's pipeline. Contains everything the
filter needs to process and composite the layer.

| Field | Description |
|-------|-------------|
| `renderer` | The active `IRenderer`. |
| `node` | The filtered `Layout` node. |
| `src` | The render target holding the subtree. |
| `scratch` | A second target of the same size, for intermediate passes. |
| `region` | The destination rect on the framebuffer (in logical pixels). |
| `dst` | `{0, 0, tw, th}` — the target's coordinate space. |
| `opacity` | The node's computed opacity (parent × own). |
| `ref` | The `FilterRef` from the CSS `filter:` list (name + args). |

### Coordinate systems

`region` is where the filter should draw **on the screen**. It's the
intersection of the node's transformed bounding box and the current
clip, in logical pixels.

`dst` is the **target's** coordinate space, starting at `(0, 0)`. It's
used to set the `texSize` uniform (which should be the target's
dimensions) and as the destination rect when rendering into a target.

### Usage

Every `applyFilter` variant uses `ctx.src`, `ctx.region`, and
`ctx.opacity`. The `scratch` target is used only by `Separable`
patterns. The `node` and `ref` are available for filters that need
context-specific behavior.

---

## 7. `FilterCustomFn`

```cpp
using FilterCustomFn = std::function<void(const FilterContext&)>;
```

The signature of a custom filter. Called with a `FilterContext` and
expected to do the whole pipeline.

The custom function has full control — it can push targets, effects,
set uniforms, and draw. It's the escape hatch for filters that don't
fit the standard patterns.

---

## 8. `FilterRegistry`

```cpp
class FilterRegistry {
public:
    static FilterRegistry& get();

    void add(FilterDef def);
    void addCustom(std::string name, FilterCustomFn fn);
    const FilterDef* find(const std::string& name) const;

private:
    std::unordered_map<std::string, FilterDef> defs_;
};
```

A singleton registry of filter definitions.

### `get()`

```cpp
static FilterRegistry& get();
```

Returns the global instance. Thread-safe initialization.

### `add(def)`

```cpp
void add(FilterDef def);
```

Registers a filter. If the name already exists, it's **silently
overwritten**. Useful for overriding built-ins.

```cpp
FilterRegistry::get().add(FilterDef{
    .name    = "sepia",
    .shader  = "sepia",
    .pattern = FilterPattern::SinglePass,
    .params  = {{"amount", FilterParamType::Float, 0, 0.7f}}
});
```

### `addCustom(name, fn)`

```cpp
void addCustom(std::string name, FilterCustomFn fn);
```

Convenience wrapper for a `Custom`-pattern filter.

```cpp
FilterRegistry::get().addCustom("invert", [](const FilterContext& ctx) {
    // custom pipeline
});
```

Equivalent to:

```cpp
add(FilterDef{
    .name = name,
    .pattern = FilterPattern::Custom,
    .custom = std::move(fn)
});
```

### `find(name)`

```cpp
const FilterDef* find(const std::string& name) const;
```

Looks up a filter by name. Returns `nullptr` if not found.

```cpp
if (const auto* def = FilterRegistry::get().find("blur")) {
    // def is valid
}
```

The returned pointer is stable as long as the registry isn't modified
(no `add` / `addCustom` calls). In practice, filters are registered at
startup and then read-only.

---

## 9. `registerBuiltinFilters`

```cpp
void registerBuiltinFilters();
```

Registers the built-in filters (`blur`, `drop-shadow`). Idempotent —
calling multiple times has no effect after the first.

Called automatically by `Layout::drawLayer` before applying a filter
chain. So the built-ins are always available, even if the user never
calls `registerBuiltinFilters` explicitly.

### `blur`

```cpp
reg.add({
    .name    = "blur",
    .shader  = "blur",
    .pattern = FilterPattern::Separable,
    .params  = {
        FilterParam{"radius", FilterParamType::Float, 0, 4.0f}
    }
});
```

| Arg | Type | Default | Meaning |
|-----|------|---------|---------|
| `radius` | float (px) | 4.0 | Blur radius. |

Requires a `blur` shader registered via `IAssetProvider`:

```cpp
assets.loadEffect("blur", "assets/blur.fs");
```

### `drop-shadow`

```cpp
reg.add({
    .name    = "drop-shadow",
    .shader  = "dropShadow",
    .pattern = FilterPattern::Silhouette,
    .params  = {
        FilterParam{"dx", FilterParamType::Float, 0, 2.0f},
        FilterParam{"dy", FilterParamType::Float, 1, 2.0f},
        FilterParam{"shadowColor", FilterParamType::Color, 2,
                    Color{0, 0, 0, 180}}
    }
});
```

| Arg | Type | Default | Meaning |
|-----|------|---------|---------|
| `dx` | float (px) | 2.0 | Horizontal offset. |
| `dy` | float (px) | 2.0 | Vertical offset. |
| `shadowColor` | color | `#000000B4` | Shadow color. |

Requires a `dropShadow` shader:

```cpp
assets.loadEffect("dropShadow", "assets/drop_shadow.fs");
```

---

## 10. `applyFilter`

```cpp
void applyFilter(const FilterDef& def, const FilterContext& ctx);
```

Runs the pipeline for a given filter definition. Called by
`Layout::drawLayer` for each filter in the node's `filters` list.

Handles:

- Setting uniforms via `setParams`.
- Dispatching to the pattern-specific pass function.
- Falling back to a plain `drawTarget` if the shader isn't valid.

### Dispatch

```cpp
switch (def.pattern) {
case FilterPattern::SinglePass: applySinglePass(ctx, fx);   break;
case FilterPattern::Separable:  applySeparable(ctx, fx, def); break;
case FilterPattern::Silhouette: applySilhouette(ctx, fx, def); break;
case FilterPattern::Custom:     def.custom(ctx);            break;
}
```

The `Custom` pattern doesn't require a shader — the lambda receives the
context directly.

### Fallback

If the shader name resolves to an invalid handle (shader not loaded),
the function logs a warning and draws the target unmodified:

```cpp
if (!fx.valid()) {
    ctx.renderer->drawTarget(ctx.src, ctx.region,
                             Colors::White.withAlpha(ctx.opacity));
    return;
}
```

This means a missing shader is a silent failure (visually, the subtree
renders without the filter). Check that shaders are loaded before
relying on a filter.

---

## 11. Complete examples

### Registering a single-pass filter

```cpp
assets.loadEffect("sepia", "assets/sepia.fs");

FilterRegistry::get().add(FilterDef{
    .name    = "sepia",
    .shader  = "sepia",
    .pattern = FilterPattern::SinglePass,
    .params  = {
        {"amount", FilterParamType::Float, 0, 0.7f}
    }
});
```

```css
.photo {
    filter: sepia(0.8);
}
```

### Registering a custom filter

```cpp
FilterRegistry::get().addCustom("invert-blur",
    [](const FilterContext& ctx) {
        auto r = ctx.renderer;

        // Blur the src into scratch
        r->pushTarget(ctx.scratch);
        r->pushEffect(assets.getEffect("blur"));
        r->setEffectVec2(assets.getEffect("blur"), "texSize",
                         {ctx.dst.width, ctx.dst.height});
        r->setEffectVec2(assets.getEffect("blur"), "direction", {1, 0});
        r->drawTarget(ctx.src, ctx.dst, Colors::White);
        r->popEffect();
        r->popTarget();

        // Draw scratch back, inverted (example)
        r->pushEffect(assets.getEffect("invert"));
        r->drawTarget(ctx.scratch, ctx.region,
                      Colors::White.withAlpha(ctx.opacity));
        r->popEffect();
    });
```

### Overriding a built-in

```cpp
// Increase the default blur radius
FilterRegistry::get().add(FilterDef{
    .name    = "blur",
    .shader  = "blur",
    .pattern = FilterPattern::Separable,
    .params  = {
        FilterParam{"radius", FilterParamType::Float, 0, 8.0f}
    }
});
```

Now `filter: blur` (without args) uses radius 8 instead of 4.

### Using multiple filters

```css
.card {
    filter: blur(2px), sepia(0.5);
}
```

Produces two `FilterRef` in the node's style. `drawLayer` runs
`applyFilter` for each, in order. **Each filter reads the original
`layerTarget_`** — they don't chain. See
[Effects §1.5](../user/09-effects.md) for the caveat.

---

## 12. Shader contract

A filter shader is a fragment shader. It receives:

- **The source texture** — as `texture0` (Raylib convention).
- **`texSize`** — a `vec2` with the target's dimensions.
- **`direction`** — a `vec2` (only for `Separable` patterns).
- **Any uniforms declared in the `FilterDef::params`**.

The framework sets `texSize` automatically before calling the shader.
For `Separable`, it sets `direction` as well.

### Minimal `sepia.fs`

```glsl
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 texSize;
uniform float amount;

void main() {
    vec4 c = texture(texture0, fragTexCoord);
    float r = c.r * 0.393 + c.g * 0.769 + c.b * 0.189;
    float g = c.r * 0.349 + c.g * 0.686 + c.b * 0.168;
    float b = c.r * 0.272 + c.g * 0.534 + c.b * 0.131;
    vec3 sepia = vec3(r, g, b);
    finalColor = vec4(mix(c.rgb, sepia, amount), c.a) * fragColor;
}
```

### Minimal `blur.fs` (separable)

```glsl
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 texSize;
uniform vec2 direction;
uniform float radius;

void main() {
    vec2 texel = 1.0 / texSize;
    vec4 sum = vec4(0.0);
    float total = 0.0;
    for (float i = -radius; i <= radius; i += 1.0) {
        float w = 1.0 - abs(i) / (radius + 1.0);
        sum += texture(texture0, fragTexCoord + direction * texel * i) * w;
        total += w;
    }
    finalColor = (sum / total) * fragColor;
}
```

The shader is loaded with `assets.loadEffect(name, path)`, which uses
Raylib's `LoadShader`. The vertex shader defaults to Raylib's standard
one unless you pass an explicit `vsPath`.

---

## 13. Common tasks

### Registering a new filter at startup

Add the `loadEffect` call (to load the shader) and the `add` call (to
register the filter) in `main`, before the first frame:

```cpp
assets.loadEffect("sepia", "assets/sepia.fs");
FilterRegistry::get().add(FilterDef{
    .name    = "sepia",
    .shader  = "sepia",
    .pattern = FilterPattern::SinglePass,
    .params  = {{"amount", FilterParamType::Float, 0, 0.7f}}
});
```

### Adding a filter dynamically

You can register filters at any time, but only effects applied to
nodes that are drawn **after** the registration will pick them up. A
node that's already mid-layer (in `drawLayer`) uses the registry's
state at the time of the lookup.

In practice, register filters at startup. The registry isn't designed
for dynamic mid-frame changes.

### Listing registered filters

The registry has no public iteration API. To inspect the registered
filters, you'd need to add a method or hold your own list of names.

### Unregistering a filter

There's no `remove` method. To "remove" a filter, add a new one with
the same name and a no-op shader or a fallback pattern.

In practice, the registry is populated once and not modified.

---

## 14. Pitfalls

**Missing shader → silent fallback.**

If the shader name isn't registered with `IAssetProvider`, `applyFilter`
draws the target unmodified. No error is visible to the user (only a
warning in the log). If a filter "does nothing", check the log and
verify the shader was loaded.

**Missing filter name → silent fallback.**

If a `.zstyle` rule has `filter: unknown`, `drawLayer` calls
`registry.find("unknown")`, gets `nullptr`, and draws the target
unmodified. Same silent behavior.

**The registry is a singleton.**

`FilterRegistry::get()` returns the same instance from every call site.
Filters registered in one test can affect subsequent tests. Reset
between tests isn't provided — you'd need to `clear()` the registry
manually (there's no public API for it, so this requires modifying the
class or using a fresh process).

**`Vec3` / `Vec4` param scaling.**

Arguments are parsed as floats, clamped to `[0, 1]`, multiplied by 255,
and stored as `uint8_t` in a `Color`. The shader receives `vec4` in
`[0, 255]`. This is documented in §3 but easy to forget.

For shaders that expect normalized values, either normalize in the
shader (`vec4 / 255.0`) or use a `Custom` filter and set uniforms
directly.

**Multiple filters don't chain.**

Each filter in a `filters` list reads the **original** target. See
[Effects §1.5](../user/09-effects.md). To compose, use a `Custom`
filter that runs the whole pipeline.

**Rotation is ignored in region computation.**

`drawLayer` doesn't account for rotation when computing the region. A
rotated filtered node has its layer computed as if it weren't rotated.

**The render target has a size limit.**

`MAX_TARGET_DIM = 4096`. A filtered node larger than this falls back to
inline rendering (no filter). If you see a large node lose its filter,
this is why.

**Nested filters are unsupported on the Raylib backend.**

`!renderer->inTarget()` is part of the `isLayer` check. A filtered
node inside another filtered node falls back to inline. The inner
filter is silently ignored.

**`FilterDef::custom` is only used for `Custom` pattern.**

If you set `custom` on a `SinglePass` filter, it's ignored. The pattern
determines which code path runs.

**Shader uniforms that don't exist are silent no-ops.**

`setEffectFloat` etc. check the shader's uniform location. If the
uniform doesn't exist in the shader, the location is `-1` and the call
does nothing. No error, no warning. A typo'd uniform name silently does
nothing.

**`FilterContext::ref` can be null.**

It's set by `drawLayer` before calling `applyFilter`, but a `Custom`
filter that's invoked outside the standard flow (e.g. from a test)
might not have it. Check before dereferencing.

**`FilterRegistry::find` returns a pointer into the internal map.**

The pointer is valid until the next `add` / `addCustom` call (which
can rehash the map). Don't hold the pointer across registration calls.

In practice, you look up and use the pointer immediately, which is safe.

**The built-in filters are registered lazily.**

`registerBuiltinFilters` is called by `drawLayer` the first time a
filter is applied. If you inspect the registry before the first frame,
`blur` and `drop-shadow` might not be there yet. Call
`registerBuiltinFilters()` explicitly if you need them available before
the first frame.

**No filter parameters from C++.**

A filter's parameters are all defined in the `FilterDef` and read from
the CSS arguments. There's no way to set a uniform from C++ for a
specific node — the node's `.zstyle` determines the filter's arguments.

To control a filter parameter from C++, use an imperative animation
that updates a uniform via `setEffectFloat`, or define the parameter as
a CSS custom property.

**The filter runs on every frame.**

A filtered node's layer is re-rendered every frame. There's no caching
of the target between frames (except for the size). This means a
filtered node is more expensive than an unfiltered one, even if the
content hasn't changed.

For static content, consider baking the filter into a texture offline,
or caching the result manually.

**Layout::drawLayer doesn't call `FilterRegistry::get()` every frame.**

Wait — actually it does:

```cpp
auto& registry = FilterRegistry::get();
```

This is a lookup, but it returns a reference to a static instance, so
it's cheap. The cost is in `find`, which is a `unordered_map` lookup
per filter per frame.

For a node with 2 filters drawn every frame, that's 2 hash lookups per
frame. Negligible.

**Adding a filter with the same name as a built-in overrides it.**

If you register a filter named `"blur"` after `registerBuiltinFilters`
has run, your definition replaces the built-in. This is useful for
customizing the default, but could also silently break code that
expects the built-in behavior.

**The registry is not thread-safe.**

`add` / `find` / `addCustom` don't lock. The registry is meant to be
populated at startup and then read-only. Concurrent registration from
multiple threads isn't supported.

---

## 15. See also

- [Effects](../user/09-effects.md) — user-facing guide to filters and
  effects.
- [Render Pipeline](../internals/06-render-pipeline.md) — where
  `drawLayer` and `applyFilter` fit in the frame.
- [Backend](UIContext.md) — `IRenderer`, `IAssetProvider`, and the
  shader loading interface.
- [ZStyle](../user/05-zstyle.md) — the `filter:` property grammar.