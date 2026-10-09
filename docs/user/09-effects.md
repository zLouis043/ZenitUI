# Effects

ZenitUI has two systems for visual post-processing:

- **`filter:`** — a *chain* of pixel filters applied to a subtree. The
  subtree is rendered into an offscreen render target, then each filter
  in the list processes that target. This is the general-purpose
  system for blur, drop-shadow, color grading, and anything else that
  needs the full rendered image.
- **`effect:`** — a *single shader* applied to the node's own draw calls.
  No render target, no ping-pong. Cheaper, but limited: the shader sees
  the vertices and fragments of the node's own geometry, not the
  composited result.

They can coexist on the same node. `filter:` wraps the node's subtree
in a layer and processes the whole image; `effect:` applies a shader to
the individual draw calls inside that layer (or directly on screen if
there's no filter).

There's a third, unrelated feature in this area: **`box-shadow`**, which
is a *styling* feature (drawn by `renderChrome`) — not a shader, no
render target, no registry. It's included here because it's visually
adjacent, but it's documented in [ZStyle §4.7](05-zstyle.md#47-visual)
as a styling property.

---

## 1. `filter:` — the layer pipeline

### 1.1 When a node becomes a layer

A node goes through the "layer" render path if **all** of these are true:

- Its `ComputedStyle::filters` is non-empty.
- The active renderer reports `supports(Feature::Effects) == true`.
- Its `rect` has non-zero width and height.
- It's **not** already inside a render target
  (`renderer->inTarget() == false`).

The last condition is important: the Raylib backend can't nest render
targets, so a filtered node inside another layer falls back to inline
rendering (no filter). See §7 for the implications.

When the conditions are met, `Layout::drawLayer` runs:

1. Computes the transformed bounding box of the node
   (`rect` after `scale` and `translate`).
2. Intersects it with the current clip rect to get the visible `region`.
3. Ensures two render targets (`layerTarget_` and `layerScratch_`) are
   sized to the region.
4. Renders the subtree (chrome + content + children) into `layerTarget_`.
5. Runs the filter chain on the target.
6. Draws the result to the framebuffer at `region`, with the node's
   opacity applied.

The render target is cached on the node and only recreated when the
region size changes. This matters for performance — a filtered node
that doesn't resize doesn't reallocate its targets.

### 1.2 Declaring a filter

```css
.card {
    filter: blur(4px);
}
```

Multiple filters, comma-separated:

```css
.card {
    filter: blur(2px), drop-shadow(2px, 2px, #000);
}
```

Syntax: `<name>(<args>)`, where `<args>` is a comma-separated list of
values. Filters with no arguments are written without parentheses:

```css
.image {
    filter: grayscale;
}
```

The parser stores the name and the args as **raw strings**. Resolution
to a `FilterDef` happens at draw time via `FilterRegistry::find(name)`.

If the filter name isn't registered, the layer falls back to drawing
the target unmodified — the subtree renders normally, just without the
effect. A missing shader is not an error.

### 1.3 What a filter does to the node

- **The whole subtree is rasterized.** Chrome, content, and children
  all go into the render target.
- **Transforms are baked in.** The node's own `translate`, `scale`, and
  `rotation` are applied *inside* the target, so the target contains
  the already-transformed image. (Actually the code applies the
  transform before rendering into the target, then draws the target
  at the transformed region.)
- **Rotation is approximated.** `drawLayer` computes the bounding box
  after scale + translate, but ignores rotation for the region
  computation. A rotated node with a filter will have its region
  slightly off. This is a known limitation; rotations on filtered
  nodes are rare.
- **Clipping is intersected, not applied.** The filter's `region` is
  the intersection of the transformed rect and the current clip. The
  parts of the subtree outside that region are still rendered into the
  target but are not drawn back to the framebuffer.

### 1.4 Opacity

The node's `opacity × parentOpacity` is passed to the final
`drawTarget` call as the tint's alpha. So `filter: blur(4px); opacity: 0.5`
results in a blurred, 50%-transparent image.

The opacity is **not** applied inside the target — the subtree is
rendered at full opacity into the target, then the whole image is
composited with the alpha. This is the correct behavior: it avoids
double-blending and matches what you'd expect from CSS.

### 1.5 Multiple filters

The filter chain runs left-to-right:

```css
.card {
    filter: blur(4px), sepia(0.5);
}
```

The subtree is rendered into `layerTarget_`. Then:

1. `blur` reads `layerTarget_` and writes back to the framebuffer
   (or `layerScratch_` for the separable two-pass).
2. `sepia` reads `layerTarget_` again (the original, not the blurred
   version — see §5.2) and writes the final result.

This is a limitation of the current implementation: filters don't chain
in the sense of "the second reads the output of the first". Each filter
reads the **original target**. For most cases (single filter, or filters
that don't depend on previous output) this is fine. For chains that
need true composition, use a `FilterPattern::Custom` filter that runs
the whole pipeline itself.

---

## 2. `effect:` — a single shader

### 2.1 When it's used

If the node has `effect: <name>` set:

- In **inline** mode (no filter, or filter unsupported), the shader is
  pushed before drawing chrome + content + children, and popped after.
- In **layer** mode (with a filter), the shader is pushed inside the
  target, so it applies to the subtree but is then subject to the
  filter chain on the final image.

Either way, the shader sees every draw call the node makes (and its
children make, unless they set their own effect).

### 2.2 Declaring an effect

```css
.btn-primary {
    effect: hueShift;
}
```

The value is a shader name registered with `IAssetProvider`. The name
is resolved once per style change and cached in `effectName_` /
`currentEffect_`.

If the shader isn't registered, `currentEffect_` is invalid and the
shader isn't pushed. No error, no log.

### 2.3 Effect vs filter

Use **`effect:`** when:

- You want a per-draw-call shader (e.g. a shader that manipulates UVs,
  vertex colors, or the fragment color of the node's own geometry).
- You don't need to process the composited image.
- You want the lowest overhead — no render target allocation.

Use **`filter:`** when:

- You need to blur, sample neighbors, or otherwise operate on the
  composited image.
- You want to affect the whole subtree, including children.
- The effect can't be expressed as a per-fragment operation.

The demo uses `effect: hueShift` on `.btn-primary` — but that's
technically misusing it, since `hueShift` is a per-pixel color operation
that would work better as a `filter:`. It works, but at the cost of
applying the shader to each draw call individually (background, border,
text), which is fine for a button but would be wrong for a complex
subtree.

---

## 3. Built-in filters

Two filters are registered by default via `registerBuiltinFilters()`:

### 3.1 `blur`

```css
.card {
    filter: blur(4px);
}
```

| Arg | Type | Default | Meaning |
|-----|------|---------|---------|
| `radius` | float (px) | `4.0` | Blur radius. |

Pattern: **Separable**. Runs two passes (horizontal, then vertical)
using the same shader with a `direction` uniform. Requires the
`blur.fs` shader to be loaded and registered under the name `blur`:

```cpp
assets.loadEffect("blur", "assets/blur.fs");
```

If the shader isn't loaded, the filter falls back to drawing the target
unmodified.

### 3.2 `drop-shadow`

```css
.card {
    filter: drop-shadow(2px, 2px, #000000);
}
```

| Arg | Type | Default | Meaning |
|-----|------|---------|---------|
| `dx` | float (px) | `2.0` | Horizontal offset. |
| `dy` | float (px) | `2.0` | Vertical offset. |
| `shadowColor` | color | `#000000B4` | Shadow color. |

Pattern: **Silhouette**. Draws a colored shadow of the subtree's
silhouette at an offset, then draws the subtree on top. This is not a
Gaussian drop-shadow — it's a solid silhouette offset. The `blur` is
not applied to the shadow; for a soft shadow, chain a `blur` **after**
the `drop-shadow`... except that chaining doesn't compose (see §1.5),
so in practice you need a custom filter for a soft drop-shadow.

Requires `dropShadow.fs` to be loaded and registered under the name
`dropShadow`:

```cpp
assets.loadEffect("dropShadow", "assets/drop_shadow.fs");
```

### 3.3 Chaining `blur` + `drop-shadow`

The intended pattern is:

```css
.card {
    filter: drop-shadow(2px, 2px, #000), blur(2px);
}
```

But because filters read the *original* target rather than chaining,
this doesn't produce a blurred drop-shadow. It produces "a drop shadow
composited with a blurred version of the original". For a true blurred
drop-shadow, you'd write a custom `Silhouette` filter that does the
blur internally.

---

## 4. Custom filters

You can register your own filters with `FilterRegistry::get().add(...)`.

### 4.1 The simple case: a shader with parameters

```cpp
#include "FilterRegistry.hpp"

// Load the shader once
assets.loadEffect("sepia", "assets/sepia.fs");

// Register the filter
ZenitUI::FilterRegistry::get().add(ZenitUI::FilterDef{
    .name    = "sepia",
    .shader  = "sepia",
    .pattern = ZenitUI::FilterPattern::SinglePass,
    .params  = {
        {"amount", ZenitUI::FilterParamType::Float, 0, 0.7f}
    }
});
```

Now you can use it:

```css
.photo {
    filter: sepia(0.8);
}
```

The `FilterDef` tells the registry:

- **name** — the string used in `.zstyle`.
- **shader** — the asset name registered with `IAssetProvider`.
- **pattern** — how to run the passes (see §4.3).
- **params** — how to map CSS arguments to shader uniforms.

At draw time, the framework calls `setParams`, which walks `params` and
sets each uniform from the corresponding CSS argument (or the default if
the argument wasn't provided).

### 4.2 Parameter mapping

Each `FilterParam` describes one shader uniform:

```cpp
struct FilterParam {
    std::string     uniform;    // uniform name in the shader
    FilterParamType type;       // Float, Vec2, Vec3, Vec4, Color
    int             argIndex;   // which CSS argument to read
    float           fdef;       // default (Float)
    Vec2            vdef;       // default (Vec2)
    Color           cdef;       // default (Color)
};
```

The `argIndex` is the position in the CSS argument list (0-based):

```css
filter: sepia(0.8);
/*              ^ arg 0 */
```

For `Vec2` / `Vec3` / `Vec4`, the param consumes that many consecutive
arguments:

```cpp
{"center", FilterParamType::Vec2, 0, /* default */}
```

consumes args 0 and 1:

```css
filter: myfilter(100, 200);
/*                 ^  ^ */
/*               arg0 arg1 */
```

Type mapping:

| FilterParamType | Uniform setter | CSS argument parsing |
|-----------------|----------------|----------------------|
| `Float` | `setEffectFloat` | `parseLengthArg` (strips `px`) |
| `Vec2` | `setEffectVec2` | Two `parseLengthArg` |
| `Vec3` | `setEffectVec4` (alpha=1) | Three `parseLengthArg`, each clamped to `[0,1]` and expanded to `0..255` |
| `Vec4` | `setEffectVec4` | Four `parseLengthArg`, same as Vec3 |
| `Color` | `setEffectVec4` | `parseColorToken` |

`Vec3` and `Vec4` pass their float arguments through to the shader
unchanged, as a `vec3` / `vec4` in the natural float range. A filter
written as `myfilter(0.5, 0.25, 0.75)` receives `vec3(0.5, 0.25, 0.75)`
in the shader. No clamping or scaling is applied.

### 4.3 Patterns

`FilterPattern` decides how many passes run and what uniforms the
framework sets.

#### `SinglePass`

```cpp
.pattern = FilterPattern::SinglePass
```

One pass: push shader, draw src → region, pop shader. The framework
sets one uniform automatically: `texSize` = `{dst.width, dst.height}`.

Your params are set before the pass. Use this for color operations
(sepia, grayscale, brightness, contrast, hue shift, etc.).

```cpp
{"texSize", vec2}   // set automatically
{"amount",  float}  // set from your params
```

#### `Separable`

```cpp
.pattern = FilterPattern::Separable
```

Two passes. The framework:

1. Pass H: push `scratch` target, push shader, set `texSize`, set
   `direction = {1, 0}`, draw src → dst region, pop shader, pop target.
2. Pass V: push shader, set `texSize`, set `direction = {0, 1}`,
   draw scratch → region, pop shader.

Your shader should sample along `direction` using the `radius` (or
whatever your blur axis parameter is called). Both `texSize` and
`direction` are set by the framework; your params define the radius.

The node's `layerScratch_` target is used for the intermediate pass.
It's allocated lazily and cached.

#### `Silhouette`

```cpp
.pattern = FilterPattern::Silhouette
```

Two draws on the same pass. The framework:

1. Sets `texSize`.
2. Draws the subtree at `region + (dx, dy)` — using your shader, which
   should color the silhouette.
3. Draws the subtree at `region` — using your shader, which now draws
   the original.

The `dx` and `dy` uniforms are set from your params **and** used to
compute the shadow's destination rect. This is how `drop-shadow` works:
the shader colors the alpha, the framework offsets the destination.

#### `Custom`

```cpp
.pattern = FilterPattern::Custom,
.custom  = [](const FilterContext& ctx) { /* ... */ }
```

Full control. The framework calls your lambda with the `FilterContext`
and does nothing else. Use this for multi-pass pipelines that need
different shaders at each step, or for filters that need to read the
node's state.

```cpp
ZenitUI::FilterRegistry::get().addCustom("invert-blur",
    [](const ZenitUI::FilterContext& ctx) {
        // ctx.renderer, ctx.node, ctx.src, ctx.scratch, ctx.region, ctx.dst, ctx.opacity, ctx.ref
        ctx.renderer->drawTarget(ctx.src, ctx.region, Colors::White.withAlpha(ctx.opacity));
        // ... your pipeline ...
    });
```

The `FilterContext`:

| Field | Description |
|-------|-------------|
| `renderer` | `IRenderer*` — the active renderer. |
| `node` | `Layout*` — the filtered node. |
| `src` | `TargetHandle` — the render target with the subtree. |
| `scratch` | `TargetHandle` — a second target, same size, for intermediate passes. |
| `region` | `Rect` — where to draw on the framebuffer (screen coords). |
| `dst` | `Rect` — `{0, 0, tw, th}`, the target's coordinate space. |
| `opacity` | `float` — the node's computed opacity. |
| `ref` | `const FilterRef*` — the CSS filter reference (name + args). |

`ctx.ref->args` is the raw CSS argument list, useful if you want to
parse them yourself.

---

## 5. Examples

### 5.1 Blur a card

```cpp
assets.loadEffect("blur", "assets/blur.fs");
```

```css
.card {
    filter: blur(4px);
}
```

Same card, but no shader loaded:

```cpp
// assets.loadEffect("blur", ...);  // missing
```

```css
.card {
    filter: blur(4px);    /* silently does nothing */
}
```

### 5.2 Register and use a sepia filter

```cpp
// main.cpp
assets.loadEffect("sepia", "assets/sepia.fs");

ZenitUI::FilterRegistry::get().add(ZenitUI::FilterDef{
    .name    = "sepia",
    .shader  = "sepia",
    .pattern = ZenitUI::FilterPattern::SinglePass,
    .params  = {
        {"amount", ZenitUI::FilterParamType::Float, 0, 0.7f}
    }
});
```

```css
/* game.zstyle */
.demo-content .card { filter: sepia(0.8); }
```

Every card inside `.demo-content` gets a sepia tone. The `.demo-content`
part is important — a filter on the container itself would be applied
to the *whole* container (all cards at once), which produces a different
visual because the sepia pass runs on the composited image.

### 5.3 Hover to change a filter

Filters can be animated? Not directly — the `filters` list is a
`std::vector<FilterRef>`, and `lerpProp` for vectors **snaps**:

```cpp
inline std::vector<FilterRef> lerpProp(const std::vector<FilterRef>& a,
                                        const std::vector<FilterRef>& b,
                                        float t) {
    return t > 0.0f ? b : a;
}
```

So a `transition: filter 0.3s` has no visible effect — the filter
switches instantly at the midpoint (`t > 0`). To animate a filter
parameter, animate the underlying value that the shader uses, and
re-register the filter with the new value:

```cpp
// Not currently supported via the CSS filter API.
// Workaround: use effect: with a shader uniform you control manually.
```

Or use an imperative animation to drive a uniform via `onUpdate`.

### 5.4 Custom multi-pass filter

```cpp
ZenitUI::FilterRegistry::get().addCustom("soft-shadow",
    [](const ZenitUI::FilterContext& ctx) {
        auto r = ctx.renderer;

        // Pass 1: silhouette into scratch
        r->pushTarget(ctx.scratch);
        r->pushEffect(assets->getEffect("silhouette"));
        r->setEffectVec2(assets->getEffect("silhouette"), "texSize",
                         {ctx.dst.width, ctx.dst.height});
        r->setEffectVec4(assets->getEffect("silhouette"), "shadowColor",
                         Color{0, 0, 0, 180});
        r->drawTarget(ctx.src, ctx.dst, Colors::White);
        r->popEffect();
        r->popTarget();

        // Pass 2: blur the silhouette into the framebuffer
        r->pushEffect(assets->getEffect("blur"));
        r->setEffectVec2(assets->getEffect("blur"), "texSize",
                         {ctx.dst.width, ctx.dst.height});
        r->setEffectVec2(assets->getEffect("blur"), "direction", {1, 0});
        r->drawTarget(ctx.scratch, ctx.region, Colors::White);
        r->popEffect();

        // Pass 3: draw the original on top
        r->drawTarget(ctx.src, ctx.region, Colors::White.withAlpha(ctx.opacity));
    });
```

This is the kind of thing `Custom` exists for. Note that accessing
`UIContext::get().assets` (to get the shader handle) is the way to
retrieve shaders by name inside a custom filter.

---

## 6. Shader requirements

A filter shader is a fragment shader. It receives:

- The source texture (Raylib's convention: `texture0`).
- `texSize` (vec2) — the target dimensions in pixels.
- `direction` (vec2) — for `Separable` patterns.
- Any uniforms declared in your `FilterDef::params`.

Minimal `sepia.fs` (Raylib style):

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

For a `Separable` blur, you'd use `direction` to pick the axis:

```glsl
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
Raylib's `LoadShader`. The vertex shader is Raylib's default unless you
pass an explicit `vsPath`.

---

## 7. Limitations and known issues

### 7.1 No nested render targets

The Raylib backend can't push a target while already inside one. The
check in `drawLayer`:

```cpp
const bool isLayer =
    !renderStyle.filters.empty() &&
    renderer->supports(Feature::Effects) &&
    rect.width > 0.0f && rect.height > 0.0f &&
    !renderer->inTarget();  // <-- this
```

If a filtered node is inside another filtered node, the inner one falls
back to `drawInline` — no filter applied. The subtree renders normally
without the effect.

Workaround: don't nest filters. If you need composition, use a `Custom`
filter on the outer node that does the whole pipeline.

### 7.2 Filters don't chain

As noted in §1.5, each filter in a list reads the **original** target,
not the output of the previous filter. This is by design in the current
implementation. For most single-filter cases it doesn't matter. For
chains, write a `Custom` filter.

### 7.3 Rotation ignored

`drawLayer` computes the transformed bounding box from `scale` and
`translate`, but not `rotation`. A rotated node with a filter will have
its region computed as if it weren't rotated. The filter will still run,
but the region may be slightly too small or too large. In practice,
rotating a filtered node is rare.

### 7.4 No `FilterRef` interpolation

`transition: filter 0.3s` has no visible effect — the `filters` list
snaps to the target at `t > 0`. To animate filter parameters, drive a
uniform via an imperative animation, or make the change instantaneous.

### 7.5 Render target size

The render target is sized to the intersection of the transformed rect
and the current clip. For a filtered node that fills the viewport, this
is a full-viewport target — potentially large. Memory-wise, two targets
(`layerTarget_` + `layerScratch_`) per filtered node, allocated lazily
and cached.

There's a hard limit: `MAX_TARGET_DIM = 4096`. If the region exceeds
this in either dimension, the layer path falls back to `drawInline`:

```cpp
if (tw > MAX_TARGET_DIM || th > MAX_TARGET_DIM) {
    drawInline(renderStyle, globalOp);
    return;
}
```

### 7.6 Effect on the same node as a filter

If a node has both `filter:` and `effect:`:

1. The effect is applied **inside** the render target — every draw call
   in the subtree gets the shader.
2. Then the filter chain processes the composited result.

This means the effect is applied twice-ish: once per draw call inside
the target, and the result is then filtered. Usually not what you want.
Pick one.

---

## 8. Complete example

A filtered card with a custom filter:

```cpp
// 1. Load the shader
assets.loadEffect("sepia", "assets/sepia.fs");

// 2. Register the filter
ZenitUI::FilterRegistry::get().add(ZenitUI::FilterDef{
    .name    = "sepia",
    .shader  = "sepia",
    .pattern = ZenitUI::FilterPattern::SinglePass,
    .params  = {
        {"amount", ZenitUI::FilterParamType::Float, 0, 0.7f}
    }
});

// 3. Register a second filter (blur) as a built-in
//    (or rely on the default registration in registerBuiltinFilters)
assets.loadEffect("blur", "assets/blur.fs");
```

```css
/* assets/game.zstyle */
.card {
    background: #2A2A32;
    radius: 8px;
    padding: 3vh;
    filter: sepia(0.8);
}

.card:hover {
    filter: sepia(0.2);
}
```

Hovering a card switches its filter. Because filter transitions snap,
the switch is instantaneous — but since both filters produce a full
render target, it's still a visible change on each frame.

If you want a smooth transition, animate a uniform instead:

```cpp
auto card = VStack()->cls("card");

auto anim = std::make_shared<UIAnimation>(0.3f);
anim->addTrack<float>(0.8f, 0.2f,
    [](Layout* l, float v) {
        // Update the sepia filter's amount uniform
        // (requires access to the filter registry and the node's effect handle)
    },
    TransitionFunction::EaseOut);
```

The workaround is verbose because the current API doesn't expose a
per-node filter parameter. For 90% of cases, the snap behavior is fine.