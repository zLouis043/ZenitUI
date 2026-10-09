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

Adds a CSS class. Sets `pendingTransition = true`.

```cpp
node->addClass("btn-primary");
```

### `std::shared_ptr<Derived> cls(const std::string& name)` (CRTP only)

Fluent wrapper for `addClass`. Returns `self()`.

```cpp
node->cls("card")->cls("elevated");
```

### `void setId(const std::string& node_id)`

Sets the node id, used by `#id` selectors in `.zstyle` and by ZMarkup's
`UINode::find()`.

### `std::shared_ptr<Derived> id(const std::string& node_id)` (CRTP only)

Fluent wrapper for `setId`.

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

Adds a child. Sets the child's parent pointer and `pendingTransition`.

**Warning:** if `weak_from_this()` is expired (i.e. the node isn't
managed by `shared_ptr`), the call logs a warning and returns without
adding. Use `X::create()` instead of `std::make_shared<X>()`.

### `void removeFromParent()`

Marks the node for removal. The actual removal happens in the parent's
next `cullRemovedChildren` pass (end of `update`).

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
bool isDisabledState()  const;   // !isEnabled
bool isCheckedState()   const;   // same as getChecked()
```

### `void setUpdateWhenDisabled(bool v)` / `bool getUpdateWhenDisabled() const`

If `true`, `onUpdate(dt)` runs even when the node is disabled.
Default: `false`.

### `void setFocusable(bool f)` / `bool isFocusable() const`

Whether the node can be focused. `isFocusable()` returns
`isFocusable_ && isEnabled`.

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
modification triggers `pendingTransition = true`.

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

Sets `pendingTransition = true`, resets the transition timer, and
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

Per-frame logic. Skipped if `!isEnabled && !updateWhenDisabled_`.

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