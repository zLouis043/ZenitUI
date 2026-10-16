# `Layout` — API Reference

The base node type. Every widget derives from it, directly or through
`TLayout<Derived, Base>`.

```cpp
namespace ZenitUI;

class Layout : public std::enable_shared_from_this<Layout> { /* ... */ };
```

- **Header**: `Layout.hpp`
- **Inherits**: `std::enable_shared_from_this<Layout>`
- **Friends**: `StyleResolver`, `AnimationPlayer`, `InputController`,
  `ScrollController`

For the full lifecycle and internals, see
[Lifecycle](../internals/02-lifecycle.md).

---

## 1. Construction

### `Layout(LayoutType type = LayoutType::Stack)`

Creates a node of the given type.

```cpp
auto node = std::make_shared<Layout>(LayoutType::Vertical);
```

**Note:** a bare `Layout` has no `onBuild` — it's fully initialized
after construction. For subclasses with children, use the `::create()`
factory instead (see §2).

### `~Layout()`

Destroys the node. If the renderer is set, it destroys any cached
render targets (`layerTarget_`, `layerScratch_`).

**Note:** children are destroyed with the parent, in reverse order
(unordered across siblings).

---

## 2. CRTP factory — `TLayout<Derived, Base>`

The recommended way to define a widget. `TLayout` adds a `create()`
factory that calls `onBuild()` after construction (when
`shared_from_this()` is valid).

```cpp
template <typename Derived, typename Base = Layout>
class TLayout : public Base {
public:
    template <typename... Args>
    TLayout(Args&&... args);

    template <typename... Args>
    static std::shared_ptr<Derived> create(Args&&... args);

    // Fluent helpers (return shared_ptr<Derived>)
    std::shared_ptr<Derived> cls(const std::string& name);
    std::shared_ptr<Derived> id(const std::string& node_id);
    std::shared_ptr<Derived> size(Value w, Value h);
    std::shared_ptr<Derived> passThrough(bool p = true);
    std::shared_ptr<Derived> with(std::shared_ptr<Layout> c);
    std::shared_ptr<Derived> self();

protected:
    virtual void onBuild() {}
};
```

**Example:**

```cpp
class MyWidget : public TLayout<MyWidget> {
public:
    MyWidget(int arg) : TLayout<MyWidget>(LayoutType::Vertical), arg_(arg) {}
protected:
    void onBuild() override {
        addChild(Label("hello"));
    }
    int arg_;
};

auto w = MyWidget::create(42);   // calls onBuild
```

**Note:** `Base` allows building on top of an existing widget:

```cpp
class Popup : public TLayout<Popup, Panel> { /* ... */ };
```

---

## 3. Style — classes, id, tag

### `void addClass(const std::string& className)`

Adds a CSS class. Sets `internal_.pendingTransition = 1`. No-op if the class
is already present, so calling it twice does not create duplicates.

```cpp
node->addClass("btn-primary");
node->addClass("btn-primary");   // ignored
```

### `void removeClass(const std::string& className)`

Removes every occurrence of the class. Sets `internal_.pendingTransition = 1`
if something was actually removed.

```cpp
node->removeClass("btn-primary");
node->removeClass("missing");     // no-op
```

Removing a class is what makes CSS animations declared under it stop
without having to rebuild the node:

```cpp
// .pulse { animation: pulse 1s infinite; }
btn->addClass("pulse");   // starts the animation
// ...
btn->removeClass("pulse"); // stops it, immediately
```

### `bool hasClass(const std::string& className) const`

True if the class is currently set on this node.

```cpp
if (node->hasClass("highlight")) { /* ... */ }
```

### `void clearStyleClasses()`

Removes every class in one shot. No-op if there were none.

```cpp
node->clearStyleClasses();
```

### `std::shared_ptr<Derived> cls(const std::string& name)` (CRTP only)

Fluent wrapper for `addClass`. Returns `self()`.

```cpp
node->cls("card")->cls("elevated");
```

There is no fluent counterpart for `removeClass` — removal is a runtime
operation, not part of the construction chain.

### `void setId(const std::string& node_id)`

Sets the node id, used by `#id` selectors in `.zstyle` and by ZMarkup's
`UINode::find()`.

### `std::shared_ptr<Derived> id(const std::string& node_id)` (CRTP only)

Fluent wrapper for `setId`.

### `bool hasId(const std::string& id) const`

True if the node's id matches the given string. Returns false if the
node has no id, or if the input is empty. Useful for conditional
logic that doesn't want to read the public `nodeId` field directly.

```cpp
if (node->hasId("save-button")) {
    // ...
}

### `void setStyleTag(const std::string& t)`

Sets the style tag. Used by `TagName { ... }` selectors. Widgets set
this in `onBuild` (e.g. `"Button"`, `"Toggle"`).

```cpp
node->setStyleTag("MyWidget");
```

### `const std::string& getStyleTag() const`

Returns the current style tag.

### `const std::vector<std::string>& getStyleClasses() const`

Returns the list of classes.

---

## 4. Size

### `void setSize(Value w, Value h)`

Sets `width` and `height` on the node's inline base style.

```cpp
node->setSize(Px(100), Px(50));
node->setSize(Percent(100), Auto());
```

### `std::shared_ptr<Derived> size(Value w, Value h)` (CRTP only)

Fluent wrapper for `setSize`.

### `Rect getRect() const`

Returns the node's final rect (set by `arrange`).

```cpp
Rect r = node->getRect();
// r.x, r.y, r.width, r.height
```

### `Vec2 getMeasuredSize() const`

Returns the intrinsic size computed by `measure`.

**Note:** `measuredSize` and `rect.size()` differ when the parent
stretches or constrains the node.

---

## 5. Tree

### `void addChild(std::shared_ptr<Layout> child)`

Adds a child. Sets the child's parent pointer and `internal_.pendingTransition`.

**Warning:** if `weak_from_this()` is expired (i.e. the node isn't
managed by `shared_ptr`), the call logs a warning and returns without
adding. Use `X::create()` instead of `std::make_shared<X>()`.

### `virtual Layout* contentTarget()`

Returns the node to which ZMarkup attaches children. Default: `this`.

Composite widgets that build an internal content wrapper override this
to return the wrapper instead. Currently only `Modal` does — its
`onBuild` creates a `content` VStack with the "Intro" animation, and
`contentTarget()` returns that VStack. Children written in markup
under a `Modal` element end up inside the wrapper, and therefore get
animated by it automatically.

```cpp
// Default behavior (plain Layout, Panel, VStack, HStack, ...):
// contentTarget() returns the node itself.
auto stack = std::make_shared<Layout>(LayoutType::Vertical);
stack->contentTarget() == stack.get();   // true

// Modal: contentTarget() returns the internal content wrapper.
auto modal = Modal::create();
modal->contentTarget() != modal.get();   // true
modal->contentTarget() == modal->children[0].get();  // true
```

*When to override*: if you write a custom widget that builds an
internal wrapper and wants ZMarkup children to participate in the
wrapper's layout/animation, override `contentTarget()`. Don't override
it just to "protect" the widget from children — that's not what it's
for.

*When NOT to use it directly*: regular C++ code rarely needs to
call `contentTarget()`. Use `addChild()` on the widget and the
framework routes to the right place, the same way ZMarkup does.
`contentTarget()` exists so that `ZMarkup::create` (which doesn't know
the widget type) can attach children correctly.

### `void removeFromParent()`

Marks the node for removal. The actual removal happens in the parent's
next `cullRemovedChildren` pass (end of `update`).

### `void removeAllChildren()`

Clears every entry in the `children` vector in one shot, and resets
each removed child's `parent` pointer to null.

```cpp
parent->removeAllChildren();
// parent->children is now empty
// each removed child: hasParent() == false
```

Unlike `removeFromParent()` on each child, this is **immediate**: the
`children` vector is cleared during the call, not deferred to the next
`cullRemovedChildren` pass.

The grandchildren are not touched. Removing a child only breaks the
link between that child and its parent; the child keeps its own
`children` intact. If you still hold a `shared_ptr` to a removed child,
it stays alive — only the tree relationship is broken.

Use it to rebuild the content of a container (a list, a settings panel)
without allocating a new node:

```cpp
list->removeAllChildren();
for (const auto& item : newItems)
    list->addChild(Label(item));
```

### `std::shared_ptr<Layout> getParent() const`

Returns the parent, or null if this is the root.

### `bool hasParent() const`

True if the node has a parent.

### `bool isAncestorOf(const Layout* other) const`

True if `this` is an ancestor of `other` (walks up `other`'s parent
chain).

### `std::vector<std::shared_ptr<Layout>> children`

Public vector of children. Direct manipulation is possible but
discouraged — prefer `addChild` / `removeFromParent`.

### `std::string nodeId`

Public field. The node id. Set by `setId` or directly.

---

## 6. State flags

### `void setInteractive(bool interactive)`

Whether the node participates in hit-testing and receives events.
Default: `true` for most nodes, `false` for `Panel` and `Text`.

### `void setBlocksRaycast(bool blocks)`

Whether the node stops hit-testing at its rect (blocking nodes behind
it) without firing its own callbacks. Default: `false`.

### `void setEnabled(bool e)`

Enables or disables the node. Disabling clears focus and pointer
capture if this node holds them, and calls `onEnabledChanged(false)`.

### `bool getEnabled() const`

True if enabled.

### `void setChecked(bool c)`

Sets the checked state. Triggers a style re-resolve, so `:checked`
rules apply.

### `bool getChecked() const`

True if checked.

### State queries

```cpp
bool isHoveredState()   const;   // pointer over the node's rect
bool isPressedState()   const;   // pointer down, node is press target
bool isFocusedState()   const;   // UIContext::focusedNode == this
bool isDisabledState()  const;   // !config.enabled
bool isCheckedState()   const;   // same as getChecked()
```

### `void setUpdateWhenDisabled(bool v)` / `bool getUpdateWhenDisabled() const`

If `true`, `onUpdate(dt)` runs even when the node is disabled.
Default: `false`.

### `void setFocusable(bool f)` / `bool isFocusable() const`

Whether the node can be focused. `isFocusable()` returns
`config.focusable != 0 && config.enabled != 0`.

### `void setFocusScope(bool s)` / `bool isFocusScope() const`

Whether the node is a focus scope (a boundary for Tab navigation).

### `void setKeyboardActivates(bool v)` / `bool getKeyboardActivates() const`

Whether `Enter` / `Space` trigger `onClick` when the node is focused.

### `void setPassThrough(bool p)` / `bool getPassThrough() const`

If `true`, click / press / right-click events bubble up to ancestors
after firing on this node.

---

## 7. Style access

### `Style& getInlineBase()`

Returns a mutable reference to the node's inline base style. Any
modification triggers `internal_.pendingTransition = 1`.

**Note:** the call itself sets the flag, even if you don't modify the
style.

```cpp
node->getInlineBase().background = Colors::Blue;
node->getInlineBase().radius = Px(8);
```

### `Style& getInlineDefaults()`

Returns a mutable reference to the node's inline default style. Lower
priority than CSS rules — used for widget defaults the user can
override.

### `void setInlineBase(const Style& s)`

Merges `s` into `inlineBase` via `overlay`.

### `const ComputedStyle& getStyle() const`

Returns the node's current resolved style (`style_.currentStyle`).
Does **not** include CSS animation values (which are overlaid only at
draw time).

### `void beginTransition()`

Sets `internal_.pendingTransition = 1`, resets the transition timer, and
snapshots the current style as the transition start.

---

## 8. Animations

### `void addAnimation(const std::string& name, std::shared_ptr<UIAnimation> anim)`

Registers an imperative animation under `name`.

### `void playAnimation(const std::string& name, bool playReverse = false)`

Plays the imperative animation with the given name. If the animation is
finished forward and `playReverse == false`, restarts from 0. If
finished reverse and `playReverse == true`, restarts from the end.

### `bool hasActiveAnimations() const`

True if the node or any descendant has active animations (recursive).

### `void addCssAnimation(const std::string& name)`

Adds a CSS animation by keyframe name to the node's active list.

### `void stopCssAnimation(const std::string& name)`

Marks a CSS animation as finished.

### `std::function<void()> onAnimationsFinished`

Public callback. Fires once when the node's **local** animations
(imperative + CSS node + CSS parts) all stop.

---

## 9. Scroll

### `bool isScrollContainer() const`

True if either `overflow-x` or `overflow-y` is not `visible`.

### Scroll position

```cpp
void  scrollTo(float x, float y);
void  scrollToX(float x);
void  scrollToY(float y);
void  scrollToTop();
void  scrollToBottom();
void  scrollToLeft();
void  scrollToRight();
```

### Scroll queries

```cpp
float getScrollX() const;
float getScrollY() const;
float getMaxScrollX() const;
float getMaxScrollY() const;
```

All values in logical pixels.

Ottimo, 843 check. Chiudiamo la 2.4 con la documentazione.

## Blocco 4 — Documentazione (2 file, li mando separati)

### File 1 di 2 — `docs/api/Layout.md`

Cerca la sezione **`## 9. Scroll`**. Attualmente contiene:

```markdown
### Scroll position

```cpp
void  scrollTo(float x, float y);
void  scrollToX(float x);
void  scrollToY(float y);
void  scrollToTop();
void  scrollToBottom();
void  scrollToLeft();
void  scrollToRight();
```

### Scroll queries

```cpp
float getScrollX() const;
float getScrollY() const;
float getMaxScrollX() const;
float getMaxScrollY() const;
```

All values in logical pixels.

### `bool scrollIntoView(float margin = 0.0f)`

Scrolls the nearest scrollable ancestor so this node becomes visible.
Returns `true` if the scroll offset was changed, `false` if the node
has no scrollable ancestor or was already fully visible.

```cpp
void onFocus() {
    // Make sure the focused node is visible after Tab navigation.
    scrollIntoView(/*margin=*/8.0f);
}
```

`margin` reserves that many pixels of breathing room between the node
and the container's edge. Without it, the node ends up flush against
the border.

### `bool scrollIntoView(const Rect& subRect, float margin = 0.0f)`

Same as above, but scrolls to make `subRect` (in the node's
coordinates) visible instead of the node's full `rect`. Useful when
the "interesting" part of a node is smaller than its bounding box —
a specific line of a `Text`, a specific cell of a grid.

```cpp
// Scroll so that only the first line of a Text is visible.
Rect firstLine = text->getRect();
firstLine.height = lineHeight;
text->scrollIntoView(firstLine);
```

The two overloads return the same thing. The no-argument version is
equivalent to `scrollIntoView(getRect(), 0.0f)`.

---

## 10. Portal

### `void setPortal(bool p)` / `bool isPortal() const`

If `true`, the node is drawn at the root's z-top and hit-tested first.
See [Portals](../user/08-portals.md).

### `void capturePointer()` / `void releasePointer()` / `bool hasPointerCapture() const`

Pointer capture. While captured, all pointer events are routed to the
node regardless of position. Capture is released automatically when the
button is released.

---

## 11. Rendering hooks

### `void setBackgroundTexture(TextureHandle tex, NineSlice np = {0,0,0,0})`

Sets a fallback background texture for `renderChrome`.

### `void setShader(EffectHandle shader)`

Sets a hardcoded shader applied to the node's draw calls. Deprecated in
favor of the `effect` style property.

### `FontHandle resolveFont(const ComputedStyle& style, FontHandle fallback = {})`

Resolves the `font` property via `IAssetProvider::getFont`, with a
fallback.

### `ResolvedBgTexture resolveBgTexture(const ComputedStyle& style, TextureHandle fallbackTex = {}, NineSlice fallbackSlice = {0,0,0,0})`

Resolves the `background-texture` property via
`IAssetProvider::getTexture`.

---

## 12. Callbacks

Public `std::function<void()>` fields:

| Field | Fires on |
|-------|----------|
| `onClick` | Press + release on the same node. |
| `onPress` | Pointer down on the node. |
| `onRelease` | Pointer up while the press target was inside the node. |
| `onHoverEnter` | Pointer enters the node. |
| `onHoverExit` | Pointer leaves the node. |
| `onRightClick` | Right-click on the node. |
| `onAnimationsFinished` | All local animations stop. |

All are `void()`. See [Events](../user/06-events.md) for the exact
firing conditions.

---

## 13. Runtime — lifecycle

These are called by the runtime, not by user code. They're virtual so
they can be overridden by the engine, not by widgets.

### `virtual Vec2 measure(float parent_w, float parent_h)`

Bottom-up intrinsic size. Cached; short-circuits if the subtree is
clean and the parent size is unchanged.

### `virtual void arrange(Rect space)`

Top-down position. Sets `rect` and arranges children. If the node is a
scroll container, uses the scroll path.

### `virtual void update(float dt, bool ancestorBlocked = false, bool scrolling = false)`

Per-node update. Called recursively from `updateTree`. Handles state
flags, style resolution, animation ticks, children update, callbacks,
and `onUpdate`.

### `virtual void draw(float parentOpacity = 1.0f)`

Recursive draw. Chooses inline or layer path, draws chrome + content +
children, then the scrollbar. If the node is the root, drains
`framePortals`.

### `Layout* hitTest(Vec2 p, bool ancestorBlocked = false)`

Point query with z-ordered traversal. Returns the topmost node under
`p`, or null.

### `void updateTree(float dt)`

Entry point for the logic phase. Hit-tests the pointer, updates focus,
runs `update` on the root.

### `void runFrame(float dt)`

Convenience: `beginFrame` + `updateTree` + `measure` + `arrange`. Called
once per frame by the user, before `BeginDrawing`.

### `void renderFrame()`

Convenience: `renderer->beginFrame` + `draw` + `renderer->endFrame`.
Called between `BeginDrawing` and `EndDrawing`.

---

## 14. Widget extension hooks

Virtual methods for subclasses. Override only what you need.

### `virtual void onBuild()`

Called once after construction, when the node is managed by a
`shared_ptr`. Set up the widget's style tag, flags, children.

### `virtual Vec2 computeIntrinsicSize(float availW, float availH)`

Return the widget's natural size. Default: `{0, 0}`.

### `virtual void onLayout()`

Called after `arrange` sets the final rect. Do **not** modify
`measuredSize`.

### `virtual void onUpdate(float dt)`

Per-frame logic. Skipped if `config.enabled == 0 && config.updateWhenDisabled == 0`.

### `virtual void onEnabledChanged(bool nowEnabled)`

Called when `setEnabled` changes the state.

### `virtual void onDescendantFocused(Layout* descendant)`

Called when a descendant gains focus (via Tab navigation).

### `virtual void renderContent(float op, const ComputedStyle& style)`

Draw the widget's own content. Called after `renderChrome`, before
children.

### `virtual void renderChrome(float op, const ComputedStyle& style)`

Draw the background, texture, and border. Default implementation
handles all standard cases. Override only for custom backgrounds.

### `Style partStyle(const std::string& partName)`

Resolves a `::part` style with its own transitions and animations.
Called from `renderContent`.

### `EffectHandle resolveActiveEffect()`

Protected helper that resolves the effect shader to apply to this
node, considering both sources. Returns an `EffectHandle` — possibly
invalid if neither source is set.

Priority:

1. The `effect:` property from `currentStyle`, resolved via
   `IAssetProvider::getEffect(name)`.
2. The `setShader()` handle set from C++.
3. Invalid handle (no effect).

```cpp
EffectHandle fx = resolveActiveEffect();
if (fx.valid() && renderer->supports(Feature::Effects))
    renderer->pushEffect(fx);
```

**When to call it**: any widget that overrides `draw()` and needs to
apply the node's effect. Currently only `CanvasLayout` does. Regular
widgets that go through `Layout::draw()` don't need to call this — the
framework resolves and pushes the effect for them in drawInline /
`drawLayer`.

**Why it exists**: the "priority" between `effect:` and `setShader()`
needs to be identical everywhere. Putting it in one place means a
subclass can't accidentally implement a different rule.

---

## 15. Helper functions

### `nodeMatchesSimple(const Layout* node, const SimpleSelector& ss)`

Free function. Tests a single compound selector against a node.

### `ruleMatches(const ThemeRule& r, const Layout* node)`

Free function. Tests a full rule (chain + media query) against a node.

### `std::vector<const ThemeRule*> getMatchingRules() const`

Returns all rules that match this node (excluding `::part` rules).

---

## 16. Widgets built on `Layout`

| Widget | Header | Base | Notes |
|--------|--------|------|-------|
| `Panel` | `Layouts.hpp` | `TLayout<Panel>` | Non-interactive box, blocks raycast. |
| `Text` | `Text.hpp` | `TLayout<Text>` | Non-interactive, wraps optionally. |
| `Button` | `Button.hpp` | `TLayout<Button, Panel>` | Interactive, focusable, keyboard. |
| `ImageContainer` | `ImageContainer.hpp` | `TLayout<ImageContainer>` | Non-interactive, draws a texture. |
| `Toggle` | `Toggle.hpp` | `TLayout<Toggle>` | Checked state, knob child. |
| `Checkbox` | `Checkbox.hpp` | `TLayout<Checkbox>` | Checked state, custom render. |
| `Slider` | `Slider.hpp` | `TLayout<Slider>` | Value `[0,1]`, drag + keyboard. |
| `ProgressBar` | `ProgressBar.hpp` | `TLayout<ProgressBar>` | Value `[0,1]`, non-interactive. |
| `TextInput` | `TextInput.hpp` | `TLayout<TextInput>` | Editable text, focus, cursor. |
| `ScrollView` | `ScrollView.hpp` | `TLayout<ScrollView>` | Overflow set to scroll. |
| `Dropdown` | `Dropdown.hpp` | `TLayout<Dropdown>` | Select from options, portal list. |
| `CanvasLayout` | `Canvas.hpp` | `TLayout<CanvasLayout>` | Renders to a render target. |
| `Modal` | `Modal.hpp` | `TLayout<Modal>` | Full-screen overlay with intro animation. |
| `Tooltip` | `Tooltip.hpp` | `TLayout<Tooltip, Panel>` | Shows on hover after delay. |
| `Popup` | `Popup.hpp` | `TLayout<Popup, Panel>` | Floating panel, portal, context menus. |

See [Widgets](../user/03-widgets.md) for usage.

---

## 17. See also

- [Lifecycle](../internals/02-lifecycle.md) — the exact order of
  operations per frame.
- [Style System](../internals/03-style-system.md) — cascade,
  specificity, transitions.
- [Layout Engine](../internals/05-layout-engine.md) — measure, arrange,
  hit-test.
- [Events](../user/06-events.md) — click semantics, focus, capture.
- [Custom Widgets](../user/10-custom-widgets.md) — writing a widget.