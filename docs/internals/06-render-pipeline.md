# Render Pipeline

The render pipeline turns the tree of `Layout` nodes into draw calls on
`IRenderer`. It lives in `Render.cpp` and is driven by `Layout::draw`,
which recurses through the tree.

This document covers:

- The two render paths: **inline** and **layer**.
- The transform stack.
- The clip stack.
- Z-ordered child drawing.
- The portal drain at the root.
- The effect (shader) resolution and push/pop.
- The filter chain and its `IRenderer` contract.

For the backend interface itself, see [Backend](09-backend.md).

---

## 1. The frame-level structure

Rendering happens in the second phase of the frame:

```cpp
BeginDrawing();
ClearBackground(BLACK);
root->renderFrame();
EndDrawing();
```

`renderFrame` on the root:

```cpp
void Layout::renderFrame() {
    auto* r = UIContext::get().renderer;
    if (!r) return;
    r->beginFrame();
    draw();
    r->endFrame();
}
```

`beginFrame` / `endFrame` are the backend's opportunity to do
per-frame setup. In `RaylibRenderer`, `beginFrame` pushes a DPI scale
matrix if `dpiScale_ != 1.0`, and `endFrame` pops it. The two are
symmetric and must be paired.

`draw()` is called with the default opacity of `1.0f`. Recursion passes
a cumulative opacity down to children.

---

## 2. `draw(parentOpacity)`

The entry point for each node. Let's trace it in order.

### 2.1 Portal registration

```cpp
if (isPortal_) {
    UIContext::get().framePortals.push_back(weak_from_this());
    return;
}
```

A portal doesn't draw itself where it is in the tree. It registers for
later drawing at the root. See [Portals](../user/08-portals.md) for
the full lifecycle.

The `weak_from_this()` is what makes the portal safe to destroy: if the
portal is removed between registration and the root's drain, the
`weak_ptr::lock()` at drain time returns null and the portal is
skipped.

### 2.2 Frustum culling

```cpp
if (rect.width > 0.0f && rect.height > 0.0f) {
    Rect clip = renderer->getClipRect();
    float m = 40.0f;
    if (rect.x > clip.x + clip.width + m ||
        rect.x + rect.width < clip.x - m ||
        rect.y > clip.y + clip.height + m ||
        rect.y + rect.height < clip.y - m)
        return;
}
```

If the node is entirely outside the current clip rect, skip it. The
`40.0f` margin is a safety buffer: it accounts for effects like blur,
drop-shadow, box-shadow, and rotation that can draw outside the `rect`.

The clip rect comes from `IRenderer::getClipRect()`, which returns the
current clip if any, or the full viewport otherwise. So a node inside
a scrolled container that's off-screen gets culled.

This is the primary optimization for large lists: a `ScrollView` with
1000 children only draws the ~15 that are visible.

### 2.3 Render style

```cpp
ComputedStyle renderStyle = style_.currentStyle;
Anim::overlayCssComputed(renderStyle, anim_.css);
resolveEffectIfNeeded(renderStyle);

float globalOp = renderStyle.opacity * parentOpacity;
if (globalOp <= 0.001f) return;
```

Three steps:

1. **Copy** the current style.
2. **Overlay** CSS keyframe animations on it. The overlay is local:
   `style_.currentStyle` is not modified. Only `renderStyle` sees the
   animated values.
3. **Resolve the effect** if the style has an `effect` property and the
   resolved handle changed.

Then compute the effective opacity: this node's `opacity` times the
accumulated `parentOpacity`. If it's effectively zero, skip the whole
subtree.

### 2.4 Path selection

```cpp
const bool isLayer =
    !renderStyle.filters.empty() &&
    renderer->supports(Feature::Effects) &&
    rect.width > 0.0f && rect.height > 0.0f &&
    !renderer->inTarget();

if (isLayer) drawLayer(renderStyle, globalOp);
else         drawInline(renderStyle, globalOp);
```

The layer path is used only when:

- The style declares at least one `filter:`.
- The backend supports effects.
- The node has non-zero size.
- We're not already inside a render target.

The last condition is the Raylib limitation: the backend can't nest
targets. A filtered node inside another layer falls back to inline,
which means the inner filter is ignored.

### 2.5 Scrollbar

```cpp
scroll_.drawScrollbar(*this, parentOpacity);
```

Drawn after the content and children. The scrollbar is on top of
everything the container draws. It uses `parentOpacity`, not `globalOp`
— the scrollbar's own opacity comes from the container's style, applied
inside `drawScrollbar`.

The scrollbar is not drawn in `drawInline` / `drawLayer` — it's a
separate call after them. So a filtered scroll container draws its
content into the layer and the scrollbar directly, on top.

### 2.6 Portal drain

```cpp
if (!hasParent()) {
    auto portals = UIContext::get().framePortals;
    for (auto &wp : portals) {
        auto sp = wp.lock();
        if (!sp) continue;
        bool wasPortal = sp->isPortal();
        sp->setPortal(false);
        sp->draw(1.0f);
        sp->setPortal(wasPortal);
    }
}
```

Only the root runs this, and only after the normal tree is done. Steps:

1. Copy `framePortals` into a local. This is important: `sp->draw(1.0f)`
   can register **new** portals (a portal's child might be a portal),
   and iterating while the vector is being modified would be UB.
2. For each alive portal (weak_ptr locks), temporarily clear the
   `isPortal_` flag, draw it normally, then restore the flag.
3. Portals are drawn at full opacity (`1.0f`). The portal's own
   `opacity` is applied inside its `draw` via `renderStyle.opacity`
   (times 1.0).

The order is declaration order within `framePortals` — the order in
which portals registered themselves during the tree walk. This is
typically bottom-to-top in the tree, but a portal registered by a
later sibling is drawn after one registered by an earlier sibling.

**Note:** because `framePortals` was already drained in `beginFrame`
(the previous frame's content became `activePortals`), this drain only
sees the portals registered during **this** draw. And the root is drawn
**before** any portal is drained, so any portal that registers during
the drain is also drawn (in the local copy).

---

## 3. `drawInline`

The common path: draw chrome, content, and children directly to the
current framebuffer.

### 3.1 Transform

```cpp
Transform2D tr = currentTransform(renderStyle);
const bool identity =
    tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
    tr.rotationDeg == 0.0f && tr.scale == 1.0f;

if (!identity)
    renderer->pushTransform(tr);
```

`currentTransform` builds a `Transform2D` from the node's resolved
style:

```cpp
Transform2D Layout::currentTransform(const ComputedStyle& style) const {
    Transform2D tr;
    tr.pivot = rect.center();
    tr.translate = {
        style.translateX.resolveSelfH(rect.width, rect.height),
        style.translateY.resolveSelfV(rect.width, rect.height)
    };
    tr.rotationDeg = style.rotation;
    tr.scale = style.scale;
    return tr;
}
```

Pivot is always the node's center. `translate-x` / `translate-y`
resolve against the node's own width / height (`resolveSelfH` /
`resolveSelfV`), not the parent's.

If the transform is identity (no translate, no rotation, no scale),
skip the push/pop. This is a common case and avoids the overhead of
`rlPushMatrix` / `rlPopMatrix`.

### 3.2 Effects

```cpp
if (hasShader && renderer->supports(Feature::Effects))
    renderer->pushEffect(customEffect);
if (currentEffect_.valid() && renderer->supports(Feature::Effects))
    renderer->pushEffect(currentEffect_);
```

Two effects can be active:

- `customEffect` — set via `setShader(EffectHandle)`. This is the
  old API, still used by `CanvasLayout`. It's a "hardcoded" shader
  handle stored on the node.
- `currentEffect_` — resolved from the style's `effect` property via
  `resolveEffectIfNeeded`.

Both can be active at once, but this is unusual. `setShader` is
deprecated in favor of `effect:` in `.zstyle`, but not removed.

The effects are pushed in order: `customEffect` first, then
`currentEffect_`. Both are popped in reverse order at the end.

A backend that doesn't support effects ignores these calls (the
`supports(Feature::Effects)` check).

### 3.3 Clip

```cpp
bool needsClip = renderStyle.overflowX != Overflow::Visible ||
                 renderStyle.overflowY != Overflow::Visible;
if (needsClip)
    renderer->pushClip(rect);
```

If either axis has `overflow != visible`, clip to the node's `rect`.
The clip is pushed **after** the transform, so the clip rect is
transformed by the backend before being applied.

The actual clipping mechanism is backend-specific:

- In `RaylibRenderer`, `pushClip` calls `transformClipToScreen` to
  compute the screen-space AABB of the clip rect (applying all active
  transforms), intersects it with the current clip, and calls
  `BeginScissorMode`.
- Inside a render target, `pushClip` is a no-op (the target is
  rendered without a scissor).

### 3.4 Chrome, content, children

```cpp
renderChrome(globalOp, renderStyle);
renderContent(globalOp, renderStyle);
drawChildren(globalOp);
```

Three virtual calls in order:

1. **`renderChrome`** — background, `box-shadow`, border. Default
   implementation handles solid backgrounds, textures (with nine-slice),
   and the `box-shadow` approximation.
2. **`renderContent`** — widget-specific: text, tracks, marks, cursors.
   Empty by default.
3. **`drawChildren`** — the children, in z-order.

### 3.5 Teardown

```cpp
if (needsClip)
    renderer->popClip();
if (currentEffect_.valid() && renderer->supports(Feature::Effects))
    renderer->popEffect();
if (hasShader && renderer->supports(Feature::Effects))
    renderer->popEffect();
if (!identity)
    renderer->popTransform();
```

In reverse order of push: clip, effects, transform. The symmetric
push/pop is essential — the backend maintains stacks, and an unbalanced
push would corrupt all subsequent draws.

### 3.6 Nested state

The push/pop calls are strictly nested: `drawChildren` recurses, and
each child does its own push/pop within the parent's pushed state. So
by the time a child draws, the transform, clip, and effects of its
ancestors are active.

A child that wants to **not** inherit an ancestor's effect can push
another effect on top — but it can't remove the ancestor's. In
practice, effects are rare enough that this isn't a problem.

---

## 4. `drawLayer`

Used when the node has filters. The subtree is rendered into an
offscreen target, then the filter chain is applied.

### 4.1 Transformed bounding box

```cpp
const float scale = renderStyle.scale;
const float tx = renderStyle.translateX.resolveSelfH(rect.width, rect.height);
const float ty = renderStyle.translateY.resolveSelfV(rect.width, rect.height);
const Vec2 pivot = rect.center();

const float tW = rect.width * scale;
const float tH = rect.height * scale;

Rect transformedRect = {
    pivot.x - tW * 0.5f + tx,
    pivot.y - tH * 0.5f + ty,
    tW, tH
};
```

Computes the AABB of the node after applying **scale** and
**translate**. Rotation is ignored — a rotated filtered node will have
its region computed as if it weren't rotated. This is a documented
limitation; the code has a comment about it.

The pivot is the node's center (matching the transform convention in
`pushTransform`), so the bounding box is centered at `pivot + translate`.

### 4.2 Region

```cpp
Rect clip = renderer->getClipRect();

Rect region;
region.x = std::max(transformedRect.x, clip.x);
region.y = std::max(transformedRect.y, clip.y);
float right = std::min(transformedRect.x + transformedRect.width,
                       clip.x + clip.width);
float bottom = std::min(transformedRect.y + transformedRect.height,
                        clip.y + clip.height);
region.width = std::max(0.0f, right - region.x);
region.height = std::max(0.0f, bottom - region.y);

if (region.width < 1.0f || region.height < 1.0f) return;
```

The region is the intersection of the transformed rect and the current
clip. It's the area of the framebuffer that the filter will end up
writing to. Everything outside is clipped away.

If the intersection is degenerate, skip the layer entirely — nothing
would be visible.

### 4.3 Target allocation

```cpp
const int tw = (int)std::ceil(region.width);
const int th = (int)std::ceil(region.height);
if (tw <= 0 || th <= 0) return;

constexpr int MAX_TARGET_DIM = 4096;
if (tw > MAX_TARGET_DIM || th > MAX_TARGET_DIM) {
    drawInline(renderStyle, globalOp);
    return;
}

if (!layerTarget_.valid() || layerTarget_.width != tw ||
    layerTarget_.height != th)
{
    if (layerTarget_.valid()) renderer->destroyTarget(layerTarget_);
    layerTarget_ = renderer->createTarget(tw, th);
    if (!layerTarget_.valid()) return;
}
if (!layerScratch_.valid() || layerScratch_.width != tw ||
    layerScratch_.height != th)
{
    if (layerScratch_.valid()) renderer->destroyTarget(layerScratch_);
    layerScratch_ = renderer->createTarget(tw, th);
    if (!layerScratch_.valid()) return;
}
```

Two targets are ensured:

- `layerTarget_` — the subtree is rendered here, then filtered.
- `layerScratch_` — for separable filters (blur), the intermediate
  pass.

Both are cached on the node. They're only reallocated when the region
size changes. Targets are destroyed in `~Layout`.

The `MAX_TARGET_DIM = 4096` hard limit falls back to inline rendering.
This is a defensive check to avoid allocating pathological targets.

### 4.4 Render the subtree into the target

```cpp
renderer->pushTarget(layerTarget_);

Transform2D offsetTr;
offsetTr.pivot = {0.0f, 0.0f};
offsetTr.translate = {-region.x, -region.y};

Transform2D tr = currentTransform(renderStyle);
const bool identity =
    tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
    tr.rotationDeg == 0.0f && tr.scale == 1.0f;
if (!identity) renderer->pushTransform(tr);
renderer->pushTransform(offsetTr);

if (hasShader && renderer->supports(Feature::Effects))
    renderer->pushEffect(customEffect);
if (currentEffect_.valid() && renderer->supports(Feature::Effects))
    renderer->pushEffect(currentEffect_);

renderChrome(globalOp, renderStyle);
renderContent(globalOp, renderStyle);
drawChildren(globalOp);

if (currentEffect_.valid() && renderer->supports(Feature::Effects))
    renderer->popEffect();
if (hasShader && renderer->supports(Feature::Effects))
    renderer->popEffect();
renderer->popTransform();
if (!identity) renderer->popTransform();

renderer->popTarget();
```

Steps:

1. `pushTarget(layerTarget_)` — subsequent draws go to the target,
   starting from `(0, 0)`. The backend may reset the matrix
   (Raylib's `RaylibRenderer` does `rlLoadIdentity`).
2. `offsetTr` — translate by `-region.x, -region.y` so the subtree
   draws at `(0, 0)` of the target instead of at the screen position.
3. `tr` — the node's own transform (scale, translate, rotation) is
   applied **inside** the target. So the target contains the
   transformed subtree. The offset is applied **after** the transform,
   so the net effect is: transform → offset → target.
4. Effects are pushed inside the target. They apply to every draw
   call inside the subtree.
5. `renderChrome`, `renderContent`, `drawChildren` — the normal inline
   sequence.
6. Pop everything in reverse.
7. `popTarget()` — returns drawing to the framebuffer.

Note the offset is applied after the transform. The order in the code
is `pushTransform(tr); pushTransform(offsetTr);`, and the backend stacks
them so `offsetTr` is the innermost (applied last in the vertex
transform). So a point `p` in the node's local space becomes
`tr * p` (its visual position), then `+ offsetTr.translate`
(shifted to target space).

### 4.5 Apply the filter chain

```cpp
registerBuiltinFilters();

FilterContext ctx;
ctx.renderer = renderer;
ctx.node = this;
ctx.src = layerTarget_;
ctx.scratch = layerScratch_;
ctx.region = region;
ctx.dst = {0, 0, (float)tw, (float)th};
ctx.opacity = globalOp;

auto& registry = FilterRegistry::get();

if (renderStyle.filters.empty()) {
    renderer->drawTarget(layerTarget_, region,
                         Colors::White.withAlpha(globalOp));
    return;
}

for (const auto& f : renderStyle.filters) {
    ctx.ref = &f;
    if (const auto* def = registry.find(f.name)) {
        applyFilter(*def, ctx);
    } else {
        renderer->drawTarget(layerTarget_, region,
                             Colors::White.withAlpha(globalOp));
    }
}
```

`registerBuiltinFilters` is idempotent — it ensures `blur` and
`drop-shadow` are registered before the chain runs.

For each filter in `renderStyle.filters`:

- If the filter name is registered, `applyFilter(def, ctx)` runs the
  filter's pattern-specific pipeline.
- If not registered (missing shader or typo), fallback: draw the target
  directly.

The fallback is silent — no error, no log. The node just renders
unfiltered.

**Note:** in a `filters` list with two filters, both `applyFilter`
calls read from `layerTarget_` (the original). They don't chain — the
second doesn't see the first's output. This is documented in
[Effects §1.5](../user/09-effects.md).

### 4.6 `applyFilter` patterns

`applyFilter` dispatches by `def.pattern`. The four patterns:

**SinglePass**:
```cpp
c.renderer->pushEffect(fx);
c.renderer->setEffectVec2(fx, "texSize", {c.dst.width, c.dst.height});
c.renderer->drawTarget(c.src, c.region, Colors::White.withAlpha(c.opacity));
c.renderer->popEffect();
```

One draw with the shader. `texSize` is set to the target's dimensions.
The result goes to `region` on the framebuffer.

**Separable**:
```cpp
// H pass: src → scratch
c.renderer->pushTarget(c.scratch);
c.renderer->pushEffect(fx);
c.renderer->setEffectVec2(fx, "texSize", {c.dst.width, c.dst.height});
c.renderer->setEffectVec2(fx, "direction", {1.0f, 0.0f});
c.renderer->drawTarget(c.src, c.dst, Colors::White);
c.renderer->popEffect();
c.renderer->popTarget();

// V pass: scratch → framebuffer
c.renderer->pushEffect(fx);
c.renderer->setEffectVec2(fx, "texSize", {c.dst.width, c.dst.height});
c.renderer->setEffectVec2(fx, "direction", {0.0f, 1.0f});
c.renderer->drawTarget(c.scratch, c.region, Colors::White.withAlpha(c.opacity));
c.renderer->popEffect();
```

Two passes. The `direction` uniform tells the shader which axis to
sample. The scratch target holds the H-pass result. The final V-pass
writes to `region` with the node's opacity.

The intermediate H-pass draws at full opacity to `c.dst` — it's
compositing the same subtree image, so opacity would be applied twice
otherwise.

**Silhouette**:
```cpp
float dx = 2.0f, dy = 2.0f;
for (const auto& p : def.params) {
    if (p.uniform == "dx" && p.argIndex < (int)c.ref->args.size())
        dx = parseLengthArg(c.ref->args[p.argIndex]);
    if (p.uniform == "dy" && p.argIndex < (int)c.ref->args.size())
        dy = parseLengthArg(c.ref->args[p.argIndex]);
}
Rect shadowDst = {c.region.x + dx, c.region.y + dy,
                  c.region.width, c.region.height};

c.renderer->pushEffect(fx);
c.renderer->setEffectVec2(fx, "texSize", {c.dst.width, c.dst.height});
c.renderer->drawTarget(c.src, shadowDst, Colors::White.withAlpha(c.opacity));
c.renderer->popEffect();

c.renderer->drawTarget(c.src, c.region, Colors::White.withAlpha(c.opacity));
```

Two draws: the first offsets the destination by `(dx, dy)` and uses the
shader to color the silhouette; the second draws the original at the
original position with no shader (so the original renders normally on
top of the shadow).

The `dx` / `dy` args are read from `c.ref->args` and re-parsed to
compute the offset destination. This duplicates the parsing done in
`setParams`, which sets the same uniforms. A bit redundant but simple.

**Custom**:
```cpp
if (def.custom) def.custom(ctx);
```

The user-supplied lambda runs with full access to the context. It can
push targets, effects, draw any number of passes — whatever it needs.

### 4.7 `setParams`

Before each pattern runs, `applyFilter` calls `setParams`, which sets
the shader uniforms from the filter def's `params` and the CSS args:

```cpp
static void setParams(IRenderer* r, EffectHandle fx,
                      const FilterDef& def, const FilterRef& ref)
{
    for (const auto& p : def.params) {
        const int n = (int)ref.args.size();
        switch (p.type) {
        case FilterParamType::Float:
            float v = p.fdef;
            if (p.argIndex < n) v = parseLengthArg(ref.args[p.argIndex]);
            r->setEffectFloat(fx, p.uniform.c_str(), v);
            break;
        case FilterParamType::Vec2:
            // reads argIndex and argIndex + 1
            // ...
        case FilterParamType::Color:
            Color c = p.cdef;
            if (p.argIndex < n) {
                if (auto col = parseColorToken(ref.args[p.argIndex]))
                    c = *col;
            }
            r->setEffectVec4(fx, p.uniform.c_str(), c);
            break;
        }
    }
}
```

Each param has:

- `uniform` — the shader uniform name.
- `type` — how to set it (`Float` / `Vec2` / `Vec3` / `Vec4` / `Color`).
- `argIndex` — which CSS argument to read.
- A default value.

`parseLengthArg` strips `px` suffixes and parses the float.
`parseColorToken` parses a color (hex or named).

**Note:** `Vec3` and `Vec4` params go through `setEffectVec4` with
their float args clamped to `[0, 1]` and scaled to `[0, 255]` (as
`uint8_t`). This is a quirk of the current implementation — the shader
receives values that it should normalize back to `[0, 1]`. See
[Effects §4.2](../user/09-effects.md).

---

## 5. `drawChildren(globalOp)`

```cpp
void Layout::drawChildren(float globalOp) {
    drawNegZ_.clear();
    drawNormal_.clear();
    drawPosZ_.clear();

    for (auto& child : children) {
        if (child->isStackingContext()) {
            if (child->getZIndex() < 0) drawNegZ_.push_back(child.get());
            else                        drawPosZ_.push_back(child.get());
        } else {
            drawNormal_.push_back(child.get());
        }
    }

    auto sortByZ = [](Layout* a, Layout* b) {
        return a->getZIndex() < b->getZIndex();
    };
    std::stable_sort(drawNegZ_.begin(), drawNegZ_.end(), sortByZ);
    std::stable_sort(drawPosZ_.begin(), drawPosZ_.end(), sortByZ);

    for (auto* child : drawNegZ_) child->draw(globalOp);
    for (auto* child : drawNormal_) child->draw(globalOp);
    for (auto* child : drawPosZ_) child->draw(globalOp);
}
```

The classic z-order partition:

1. Partition children into three buckets by stacking context.
2. Stable-sort each bucket by z-index. Stable so declaration order is
   preserved within the same z-index.
3. Draw `negZ` (ascending z), then `normal` (declaration order), then
   `posZ` (ascending z).

The three vectors are members of `Layout` (`drawNegZ_`, `drawNormal_`,
`drawPosZ_`), reused across frames to avoid per-frame allocation. They're
cleared at the start of each `drawChildren` call.

### 5.1 Stacking context

```cpp
bool isStackingContext() const {
    return style_.currentStyle.position != Position::Static &&
           !style_.currentStyle.zIndex.isAuto;
}
```

Only nodes with **both** non-static position and explicit `z-index` are
stacking contexts. Nodes with `z-index` but `position: static` fall
into `normalFlow` and draw in declaration order.

Nodes without `z-index` but with `position: absolute` / `relative` also
fall into `normalFlow` — they're positioned out of flow but don't have
a z-ordering priority.

This matches the CSS 2.1 rules for stacking contexts, simplified (no
`opacity < 1` or `transform` as stacking context triggers).

### 5.2 The `draw` recursion

Each `child->draw(globalOp)` recurses into the child. The child does
its own push/pop, its own culling, its own path selection. The parent's
`globalOp` is the accumulated opacity so far.

A child that is a portal registers itself and returns. The parent's
loop continues, but the portal isn't drawn here — it'll be drained at
the root.

### 5.3 Auto-removal during draw

If a child's `draw` calls `removeFromParent` (e.g. via a callback that
fires during draw — unusual but possible), the child is still in
`children` at this point. The `wantsRemoval` flag is set, but the
child is drawn one more time. It'll be culled at the end of the next
`update`.

In practice, callbacks don't fire during `draw` — they fire during
`update`. So this is a non-issue.

---

## 6. `resolveEffectIfNeeded`

```cpp
void Layout::resolveEffectIfNeeded(const ComputedStyle& style) {
    if (style.effect.empty()) {
        effectName_.clear();
        currentEffect_ = {};
        return;
    }
    if (style.effect == effectName_) return;

    effectName_ = style.effect;
    if (auto* assets = UIContext::get().assets)
        currentEffect_ = assets->getEffect(style.effect);
    else
        currentEffect_ = {};
}
```

Resolves the string `effect` name to an `EffectHandle`. Caches the last
resolved name (`effectName_`) and the handle (`currentEffect_`), so
the lookup only happens when the name changes.

Three states:

- Empty `effect` → clear the cache, no effect.
- Same name as last resolved → reuse the handle.
- New name → resolve via `IAssetProvider::getEffect`.

If `assets` is null (headless or misconfigured), the handle is invalid
and no effect is pushed.

This is called from `draw`, **after** the CSS animation overlay, so a
CSS animation that changes the `effect` property would re-resolve
correctly. (Though the `effect` property snaps — no interpolation —
so the change is instantaneous.)

---

## 7. The `renderChrome` default

`renderChrome` is defined inline in `Layout.hpp` as the default
background+border renderer. Its four phases:

### 7.1 `box-shadow`

```cpp
if (style.boxShadow.enabled) {
    const auto& sh = style.boxShadow;
    float sx = sh.x.resolveSelfH(rect.width, rect.height);
    float sy = sh.y.resolveSelfV(rect.width, rect.height);
    float blur = sh.blur.resolveSelfH(rect.width, rect.height);

    Rect shadowRect = {rect.x + sx, rect.y + sy, rect.width, rect.height};

    if (blur <= 0.5f) {
        r->fillRoundedRect(shadowRect, rPx, sh.color.withAlpha(op));
    } else {
        constexpr int LAYERS = 5;
        for (int i = 0; i < LAYERS; ++i) {
            float expand = blur * (float)(i + 1) / (float)LAYERS;
            float alpha = (1.0f - (float)i / (float)LAYERS) * 0.35f;
            Rect r2 = {
                shadowRect.x - expand,
                shadowRect.y - expand,
                shadowRect.width + expand * 2.0f,
                shadowRect.height + expand * 2.0f
            };
            float rPx2 = std::clamp(rPx + expand, 0.0f, expand + maxRadius);
            r->fillRoundedRect(r2, rPx2, sh.color.withAlpha(op * alpha));
        }
    }
}
```

The box-shadow is **not** a filter or a shader. It's drawn as a
sequence of 5 concentric rounded rects with decreasing alpha for the
blur approximation. Each layer is larger and less opaque. The result
is a soft-edged shadow that's much cheaper than a Gaussian blur but
also much less accurate.

Zero blur → a single rounded rect, drawn exactly at the shadow offset.

### 7.2 Background color

```cpp
Color bg = style.background.withAlpha(op);
if (bg.a > 0) {
    if (rPx > 0.0f && maxRadius > 0.0f)
        r->fillRoundedRect(rect, rPx, bg);
    else
        r->fillRect(rect, bg);
}
```

The background is drawn as a rounded rect if the radius > 0, else a
plain rect. The radius is clamped to `min(rect.width, rect.height) * 0.5`
so it can't exceed half the smallest dimension.

### 7.3 Texture

```cpp
auto resolved = resolveBgTexture(style, bgTexture, bgPatchInfo);
if (resolved.valid()) {
    Color tint = style.tint.withAlpha(op);
    if (resolved.slice.left || resolved.slice.top ||
        resolved.slice.right || resolved.slice.bottom)
    {
        r->drawNineSlice(resolved.tex, resolved.slice, rect, tint);
    } else {
        Rect src{0, 0, (float)resolved.tex.width, (float)resolved.tex.height};
        r->drawTexture(resolved.tex, src, rect, tint);
    }
}
```

Two sources:

- `style.backgroundTexture` — a `TextureRef` in the style, resolved by
  name via `IAssetProvider`.
- `bgTexture` — the widget's hardcoded fallback (set via
  `setBackgroundTexture`).

Style wins if both are set. The texture is drawn stretched to `rect`,
or nine-sliced if insets are provided.

The `tint` multiplies the texture. Default is white (no tint).

### 7.4 Border

```cpp
Color bc = style.borderColor.withAlpha(op);
float bw = style.borderWidth.resolveH(maxRadius * 2.0f, maxRadius * 2.0f);
if (bc.a > 0 && bw > 0.0f) {
    if (rPx > 0.0f && maxRadius > 0.0f)
        r->strokeRoundedRect(rect, rPx, bw, bc);
    else
        r->strokeRect(rect, bw, bc);
}
```

Border is drawn as a stroke around `rect`, with the same radius as the
background. The border is drawn **inside** the rect (not offset), so
it overlaps the background edge.

Border width is resolved against `maxRadius * 2.0f` for the percentage
case — matching how `radius` is resolved.

---

## 8. Clipping in detail

Clipping is the most backend-specific part of the pipeline. The
`IRenderer` contract is:

- `pushClip(Rect r)` — clip subsequent draws to `r`.
- `popClip()` — undo the last push.
- `getClipRect()` — the current effective clip.

The Raylib implementation:

```cpp
void RaylibRenderer::pushClip(Rect r) {
    if (insideTarget_) return;

    Rect screenRect = transformClipToScreen(r);

    if (clipActive) {
        clipStack.push_back(currentClip);
        currentClip = intersectRect(currentClip, screenRect);
    } else {
        clipStack.push_back({0, 0, (float)GetScreenWidth() / dpiScale_,
                             (float)GetScreenHeight() / dpiScale_});
        currentClip = screenRect;
        clipActive = true;
    }

    int x = (int)std::floor(currentClip.x * dpiScale_);
    int y = (int)std::floor(currentClip.y * dpiScale_);
    int w = (int)std::ceil(currentClip.width * dpiScale_);
    int h = (int)std::ceil(currentClip.height * dpiScale_);
    if (w <= 0 || h <= 0) {
        BeginScissorMode(0, 0, 0, 0);
        return;
    }
    BeginScissorMode(x, y, w, h);
}
```

Three important details:

1. **Inside a target, clipping is a no-op.** Raylib's render targets
   don't support scissor the same way as the framebuffer. Instead, the
   target itself is the clip — anything outside the target dimensions
   is naturally excluded. So clip pushes/pops are skipped inside a
   layer.

2. **The clip rect is transformed to screen space.** `transformClipToScreen`
   applies all active transforms to the four corners of the clip rect
   and computes the AABB. This makes the clip follow the transform:
   a node with `scale: 2` clips to twice the size.

3. **Nested clips are intersected.** Each `pushClip` intersects the new
   rect with the current one. So overlapping clips correctly accumulate.
   The intersection is an AABB approximation — for rotated clips this
   would be conservative (larger than necessary).

The clip is stored in `currentClip` and pushed/popped in `clipStack`.
`getClipRect()` returns `currentClip` if active, else the full viewport.

### 8.1 Frustum culling uses the clip

`draw`'s frustum culling reads `renderer->getClipRect()` and compares
against `rect`. So a node outside the current clip is skipped. This is
how scroll containers avoid drawing off-screen children.

The margin of `40.0f` is a safety buffer for effects that draw beyond
`rect` (blur, drop-shadow, box-shadow). Without it, those effects would
be culled at the edges.

---

## 9. Interaction with `renderContent` and parts

`renderContent` and `renderChrome` are called **inside** the pushed
transform, clip, and effects. So:

- A draw call in `renderContent` is transformed by the node's transform.
- It's clipped by the node's overflow (if any) and any ancestor's.
- It's colored by the node's effects (if any).

`renderContent` reads `renderStyle` (or a part style via
`partStyle(name)`) and draws primitives. Every color drawn must have
`.withAlpha(op)` applied — otherwise the node's opacity and all
ancestor opacities are ignored.

### 9.1 Parts

`partStyle(name)` is a method of `Layout` that calls
`style_.partFor(*this, name)`. The returned `Style` has the part's
resolved style, with transitions and animations applied.

The part is not a `Layout` — it has no `rect`. The widget's
`renderContent` computes the geometry and reads the style:

```cpp
Style knobStyle = partStyle("knob");
Color knobColor = knobStyle.color.is_set
    ? knobStyle.color.value
    : defaultColor;
r->fillCircle(center, radius, knobColor.withAlpha(op));
```

Each `partStyle` call re-resolves the part, ticks its transition, and
overlays its animations. In practice, once per frame per part.

---

## 10. Interaction with the portal drain

The root's `draw` drains `framePortals` after drawing the tree. Each
portal is drawn with `setPortal(false)` temporarily so it renders
normally.

Important consequences:

- Portals draw on top of the entire tree, regardless of z-index in the
  tree. If you need a floating element **behind** other content, use
  `position: absolute` + `z-index` within a stacking context — not a
  portal.
- Portals draw at `opacity = 1.0f` — their own opacity is applied
  inside their `draw` call.
- The order of portals in `framePortals` determines the draw order. It's
  the order they registered themselves during the tree walk, which is
  roughly declaration order for portals at the same depth, but nested
  portals register before their ancestors are drawn (since drawing is
  depth-first).

---

## 11. What the backend sees

The `IRenderer` interface is the contract. The pipeline uses:

- `fillRect`, `fillRoundedRect`, `fillCircle` — for chrome and content.
- `strokeRect`, `strokeRoundedRect` — for borders.
- `drawTexture`, `drawNineSlice` — for backgrounds.
- `drawText` — for text.
- `pushTransform` / `popTransform` — for the transform stack.
- `pushEffect` / `popEffect` — for shader push/pop.
- `pushClip` / `popClip` / `getClipRect` — for the clip stack.
- `createTarget` / `destroyTarget` / `pushTarget` / `popTarget` /
  `drawTarget` — for render target management.
- `setEffectFloat` / `setEffectVec2` / `setEffectVec4` — for shader
  uniforms.
- `clearTarget` / `inTarget` — for target state queries.
- `beginFrame` / `endFrame` — for frame-level setup.
- `setDpiScale` — for DPI management.
- `supports(Feature)` — for feature detection.

The pipeline never touches the backend's own types. Everything crosses
the interface as framework types (`Rect`, `Color`, `Vec2`, handles).

---

## 12. The Raylib backend specifics

`RaylibRenderer` is the reference implementation. Key decisions:

### 12.1 Clip stack

Maintained as `std::vector<Rect> clipStack` and a `bool clipActive` with
the current `currentClip`. The scissor is set via `BeginScissorMode`,
which must be called after any state change (including `popTarget`).
The renderer handles this by re-issuing the scissor after `popTarget`.

### 12.2 Transform stack

Maintained as `std::vector<Transform2D> transformStack`. `pushTransform`
calls `rlPushMatrix` and applies the transform via `rlTranslatef`,
`rlRotatef`, `rlScalef`. `popTransform` calls `rlPopMatrix`.

The `transformStack` is used by `transformClipToScreen` to compute the
screen-space AABB of a clip rect: it applies all active transforms to
the four corners.

### 12.3 Target stack

`pushTarget` saves the current matrix (`rlPushMatrix`, `rlLoadIdentity`),
sets `insideTarget_ = true`, ends any active scissor, and calls
`BeginTextureMode`. `popTarget` calls `EndTextureMode`, restores the
matrix (`rlPopMatrix`), and re-issues the scissor if one was active.

The "no nested targets" limitation comes from Raylib: `BeginTextureMode`
while already in a texture mode doesn't work correctly. The pipeline
avoids this by the `!renderer->inTarget()` check in `draw`.

### 12.4 Effects

`pushEffect` uses `BeginShaderMode`. `popEffect` calls `EndShaderMode`
and re-pushes the previous effect if there's one on the stack
(`effectStack_`). This is because Raylib's shader mode is not stackable
by default.

### 12.5 DPI

`beginFrame` pushes a scale matrix if `dpiScale_ != 1.0f`. `endFrame`
pops it. `pushClip` multiplies the clip rect by `dpiScale_` before
calling `BeginScissorMode`, because scissor operates in physical pixels
but the clip is in logical.

`measureText` also applies DPI scaling to the text size and then
divides the result by `dpiScale_` to return logical pixels.

---

## 13. Performance notes

### 13.1 Frustum culling

The biggest win for large UIs. A 1000-child scroll view only draws the
~15 visible children. Each `draw` call does the AABB check cheaply.

### 13.2 Target caching

Render targets are cached per node and only reallocated when the size
changes. A filtered node that doesn't resize doesn't allocate in a
frame.

### 13.3 The three z-vectors

`drawNegZ_`, `drawNormal_`, `drawPosZ_` are members, reused across
frames. The only allocations are when the vectors grow beyond their
capacity, which stabilizes after the first few frames.

### 13.4 String comparisons

`resolveEffectIfNeeded` compares `style.effect` to `effectName_`. This
is a `std::string` comparison, done once per draw call per node. For
nodes with no effect, it's `style.effect.empty()` — cheap. For nodes
with an effect that didn't change, it's a `std::string::operator==` on
the last resolved name — also cheap but not free.

### 13.5 The effect and clip stacks

Every `pushTransform` / `pushClip` / `pushEffect` is a stack operation.
For deeply nested trees with many transforms, this accumulates. The
`identity` checks in `drawInline` / `drawLayer` skip push/pop when
there's no transform, which is the common case.

### 13.6 The `renderStyle` copy

`ComputedStyle renderStyle = style_.currentStyle;` copies a struct with
~45 fields (some with `std::string`, `std::vector`, `unordered_map`).
For each node, every frame. This is a nontrivial copy — the `Style`
struct has grown over time.

For most nodes, the copy is cheap because the strings and vectors are
empty (default). For nodes with `font`, `backgroundTexture`, `filters`,
or `animations`, the copy allocates.

This is a known cost. An optimization would be to apply the animation
overlay in place on a reference or to use copy-on-write. For now, it's
acceptable.

### 13.7 The double overlay evaluation

CSS animations are evaluated twice per frame: once in `tickCss` (to
detect finish), once in `draw` (to apply). Each evaluation iterates
~45 properties. For a node with multiple active animations, this is
the dominant cost.

An optimization would be to cache the evaluated `Style` between
`tickCss` and `draw`. Not currently implemented.

---

## 14. Pitfalls

**Inside a render target, clip is a no-op.**
`pushClip` in `RaylibRenderer` returns immediately if `insideTarget_`.
So a filtered scroll container doesn't clip its content to its rect —
it relies on the target's bounds instead. This is mostly invisible but
can cause content to be drawn slightly outside the container if the
target is larger than the rect (which happens when the region is
computed from the transformed rect).

**Rotation is ignored in `drawLayer`'s region computation.**
A rotated filtered node will have its region computed as if it weren't
rotated. The filter runs, but on a slightly wrong region. This is a
known limitation.

**Multiple filters don't chain.**
Each filter reads `layerTarget_`, not the previous filter's output.
For blur + sepia, the sepia operates on the original, not the blurred
version. The result is "sepia blended with blur" rather than "sepia of
blur". See [Effects](../user/09-effects.md) for workarounds.

**`box-shadow` is not a Gaussian blur.**
It's 5 stacked rounded rects. For large blur radii, the approximation
is visible (stepped edges). For small radii, it's indistinguishable.

**Effects apply to each draw call individually.**
A node with `effect: hueShift` and three draw calls (background,
border, text) applies the shader three times. This is different from
`filter:`, which operates on the composited image. For per-fragment
shaders that don't depend on neighbors, this is fine. For shaders that
need the full image, use `filter:`.

**The root's portal drain modifies `framePortals` while copying.**
The drain copies `framePortals` into a local, then iterates the local.
If a portal registers itself during its own `draw`, it's added to
`framePortals` (the member), not the local copy. So it won't be drawn
this frame — it'll be drawn next frame, when the local copy is taken
again.

In practice, portals don't register during draw (they register once,
the first time they're drawn), so this is a non-issue. But it's a
subtle behavior.

**`draw` returns early for portals and for zero-opacity nodes.**
A portal's `draw` is a one-liner (register + return). A zero-opacity
node's `draw` returns before drawing anything. Both skip the
`renderChrome` / `renderContent` / `drawChildren` calls.

**`drawChildren` allocates three vectors on first call per node.**
Not per frame — the vectors are members. The first time a node draws
its children, the three vectors allocate. After that, they're reused
with `clear()` (which doesn't deallocate).

**Z-index only works with non-static position.**
`position: static` (the default) means the node goes into `normalFlow`
regardless of its `z-index`. This is the CSS rule, but it's easy to
forget. To use `z-index`, set `position: relative` (or `absolute`).

**Stacking contexts are not inherited.**
A child of a stacking context is not automatically in the same context
— it depends on its own `position` and `z-index`. Nested stacking
contexts are possible and are handled by the recursive drawing.

**The `region` computed in `drawLayer` uses `renderer->getClipRect()`.**
This returns the current clip in **logical** coordinates. The region
is intersected with the transformed rect (also logical). The target
size `tw`, `th` is the region's logical size, rounded up. The target
is created at that size. When drawing back, `drawTarget(layerTarget_,
region, ...)` draws it at `region`, which is in logical coordinates.
DPI scaling is handled inside the backend.

**The `MAX_TARGET_DIM` fallback can cause visible flicker.**
A node that's sometimes larger than 4096 and sometimes smaller will
alternate between filtered and unfiltered rendering. This is rare but
possible. If you see flicker on a large filtered node, this is why.

**`clearTarget` is not called before `pushTarget`.**
In `pushTarget`, Raylib calls `ClearBackground(::BLANK)` after
`BeginTextureMode`. So the target starts transparent. The pipeline
doesn't call `clearTarget` explicitly — the backend does it as part of
`pushTarget`.

**`drawTarget` flips the Y coordinate.**
Raylib's render textures are stored upside-down relative to the screen.
`drawTarget` uses a negative source height to flip the texture back:

```cpp
::Rectangle src = {0, 0, (float)tex.width, -(float)tex.height};
```

This is backend-specific and hidden behind the interface. A different
backend might not need this.

**Text rendering is not affected by transforms in `measureText`.**
`measureText` measures the text at its natural size, ignoring any
active transform. The drawn text is scaled by the transform, but its
measured size is not. So a node with `scale: 2` will have its text
drawn at 2× size but measured at 1× — the layout will be wrong.
In practice, text nodes are rarely scaled.

**The `insideTarget_` flag disables clipping but not frustum culling.**
Inside a target, `pushClip` is a no-op, but `draw`'s frustum culling
still uses `getClipRect()`. Which returns `currentClip` — the clip
that was active before the target was pushed. So a node inside a
target is culled based on the **outer** clip, not the target bounds.
Usually correct (targets are sized to the clip intersection), but
edge cases exist.