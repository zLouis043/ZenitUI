# Portals

A **portal** is a node that renders and hit-tests as if it lived at the
root of the tree, but is still declared as a child of its logical parent.
This is how the framework solves the "dropdown cut off by its container"
problem, and how popups, tooltips, and context menus work.

The concept is the same as React's portals, or CSS's `position: fixed`
with `z-index: 9999`, but built into the layout engine instead of hacked
on top.

---

## 1. What a portal does

When you call `node->setPortal(true)`:

- **Drawing**: the node is not drawn in the normal child order. It's
  queued and drawn **after** the whole tree is done, on top of everything
  else.
- **Hit-testing**: the node is tested **before** any other node, in
  reverse declaration order (last portal wins).
- **Clipping**: the node is not affected by ancestor `overflow: hidden`
  or `overflow: scroll`. It draws in screen space.
- **Z-order**: the node draws above all non-portal content,
  regardless of `z-index`.

The node still participates in `measure` / `arrange` normally. It still
has a `rect`. It still receives input events and produces callbacks.
Only the *rendering* and *hit-testing* order changes.

```cpp
auto menu = Popup::create();
menu->setContent(VStack()->with(Label("Item")));

someNode->addChild(menu);   // logical parent (for lifetime)
menu->setPortal(true);      // now it draws above everything
```

In practice you rarely set the portal flag yourself — the widgets that
need it (`Popup`, `Tooltip`, `Dropdown`'s list) do it internally. But
it's a public API, and useful to understand if you build your own
floating widget.

---

## 2. How it works internally

This section is a summary. For the full picture see
[Render Pipeline](../internals/06-render-pipeline.md) and
[Layout Engine](../internals/05-layout-engine.md).

### 2.1 Registration

When a portal node is drawn, instead of painting itself, it pushes a
`weak_ptr` to itself into `UIContext::framePortals`:

```cpp
void Layout::draw(float parentOpacity) {
    if (isPortal_) {
        UIContext::get().framePortals.push_back(weak_from_this());
        return;
    }
    // ... normal drawing ...
}
```

At the end of the root's `draw()`, it iterates the collected portals and
draws them (temporarily clearing the flag so they render normally):

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

### 2.2 Double buffering

Portals use two lists to avoid the "adding while iterating" problem:

- `framePortals` — filled during the current frame's draw.
- `activePortals` — the snapshot from the *previous* frame.

At the start of each frame, `beginFrame` swaps them:

```cpp
activePortals = std::move(framePortals);
framePortals.clear();
```

The hit-test uses `activePortals` (from last frame), the draw uses
`framePortals` (this frame). This means there's a one-frame delay
between when a portal appears on screen and when it becomes clickable —
imperceptible in practice, but worth knowing when writing tests.

### 2.3 Hit-testing order

In `Layout::hitTest`, the root node checks its `activePortals` **first**,
in reverse order:

```cpp
if (!hasParent()) {
    for (auto it = UIContext::get().activePortals.rbegin();
         it != UIContext::get().activePortals.rend(); ++it)
    {
        auto sp = it->lock();
        if (!sp) continue;
        bool wasPortal = sp->isPortal();
        sp->setPortal(false);
        Layout *hit = sp->hitTest(p, false);
        sp->setPortal(wasPortal);
        if (hit) return hit;
    }
}
```

If any portal contains the point, the hit-test returns that node
immediately. Only if no portal matches does the normal tree get tested.

This is why clicking a dropdown item works even if the item overlaps a
button in the underlying screen — the portal wins.

### 2.4 Positioning

A portal's `arrange` is called by its parent normally. In
`arrangeAbsoluteAndPortals`:

```cpp
if (c->isPortal()) {
    c->arrange({inner.x, inner.y, c->measuredSize.x, c->measuredSize.y});
    continue;
}
```

The portal is arranged at the parent's inner origin with its own
measured size — **not stretched** by alignment. This gives the portal a
default `rect`, but most floating widgets override `arrange` entirely
and compute their own position:

```cpp
// Popup::arrange
void arrange(Rect) override { /* no-op */ }

// Popup::onUpdate
rect = {x, y, size.x, size.y};
arrangeInto(rect);
```

`Tooltip` does the same, positioning itself above the anchor. `Dropdown`
positions its list below or above the trigger.

So in practice, the base `arrange` for portals is just a fallback. If
your portal wants a specific position, override `arrange` and set
`rect` manually.

---

## 3. Built-in portal widgets

### 3.1 Popup

The generic floating panel. It has three open methods:

```cpp
popup->openBelow(anchor);   // below a node, auto-flip if no room
popup->openAbove(anchor);   // above a node
popup->openAt({100, 200});  // at an absolute screen position
```

`Popup::onUpdate` recomputes the rect every frame:

- If it has an `anchor`, it reads `anchor_->getRect()` and positions
  relative to it.
- If it has a `pos_`, it uses that directly.
- It clamps to the viewport edges.
- It auto-flips above if positioning below would overflow the bottom.

It also handles:

- `closeOnClickOutside` (default `true`) — closes on left or right click
  outside its rect.
- `closeOnEscape` (default `true`) — closes on `Escape`.
- Opacity transition via `setInlineBase`.

See the [Popup section in Widgets](03-widgets.md#17-popup--contextmenu)
for the full API.

### 3.2 ContextMenu

A `Popup` with a vertical list of `Btn` items, each with class
`popup-item`. Clicking an item fires its callback and closes the menu.

```cpp
auto menu = ContextMenu({
    { "Copy",   []{ /* ... */ } },
    { "Paste",  []{ /* ... */ } },
    { "Delete", []{ /* ... */ } },
});

someNode->addChild(menu);
someNode->onRightClick = [someNode, menu]{
    menu->openAt(UIContext::get().pointer.pos);
};
```

The `weak_ptr` capture in `ContextMenu`'s item click handlers prevents a
reference cycle: the menu owns the buttons, and the buttons capture the
menu. Using `weak_ptr` breaks the loop.

### 3.3 Tooltip

Attached to an owner, shown on hover after a delay.

```cpp
Tooltip::attach(btn, "Save your changes", 0.4f);
```

`Tooltip::onUpdate` positions the tooltip centered above the anchor,
offset by `6px` vertically:

```cpp
rect = {
    a.x + a.width * 0.5f - ms.x * 0.5f,
    a.y - ms.y - 6.0f,
    ms.x,
    ms.y
};
arrangeInto(rect);
```

Tooltips don't capture input (`setBlocksRaycast(false)`) and are
non-interactive.

### 3.4 Dropdown's option list

`Dropdown::onBuild` creates a `ScrollView` with `setPortal(true)` and
`position: absolute`. Its `arrange` override is more complex than the
generic case — it looks for the nearest ancestor with `overflow != visible`,
converts the clip rect to base coordinates, and decides whether to open
below or above based on available space.

This is the reason the Dropdown works inside a `ScrollView` without
being cut off. Without a portal, the list would be clipped by the
ScrollView's `overflow: hidden`.

### 3.5 Modal — not a portal

`Modal` is **not** a portal. It's a regular child of its parent, added
on top with `setFocusScope(true)`. Because it fills the viewport with a
semi-transparent background and consumes clicks (via `setInteractive(true)`
in `show()`), it behaves like a modal without needing the portal
machinery.

If you want to add a `Modal` inside a scrollable container, you may want
to make it a portal — otherwise it will be clipped. In practice, modals
are usually added to the root.

---

## 4. Writing a custom portal widget

The pattern is:

1. In `onBuild`, call `setPortal(true)`.
2. Optionally override `arrange` to be a no-op (if you position manually).
3. In `onUpdate`, compute the desired rect and call `arrangeInto(rect)`.
4. Handle open/close state, focus, and click-outside yourself.

A minimal "tooltip-like" floating widget:

```cpp
class FloatingLabel : public TLayout<FloatingLabel> {
public:
    FloatingLabel(std::string text)
        : TLayout<FloatingLabel>(LayoutType::Stack), text_(std::move(text)) {}

    void showAt(Vec2 pos) {
        isOpen_ = true;
        pos_ = pos;
        style_.inlineBase.opacity = 1.0f;
        setEnabled(true);
    }

    void hide() {
        isOpen_ = false;
        style_.inlineBase.opacity = 0.0f;
        setEnabled(false);
    }

    void arrange(Rect) override { /* no-op, we position manually */ }

protected:
    void onBuild() override {
        setStyleTag("FloatingLabel");
        setPortal(true);
        setInteractive(false);
        setBlocksRaycast(false);
        style_.inlineBase.position = Position::Absolute;
        style_.inlineBase.opacity = 0.0f;
        isOpen_ = false;
        setEnabled(false);

        addChild(Label(text_));
    }

    void onUpdate(float) override {
        if (!isOpen_) return;

        Vec2 s = getMeasuredSize();
        rect = {pos_.x, pos_.y, s.x, s.y};
        arrangeInto(rect);
    }

private:
    std::string text_;
    Vec2 pos_{0, 0};
    bool isOpen_{false};
};
```

Things to keep in mind:

- **Set `position: absolute`.** Portals are positioned by their own
  `rect`, but they still participate in the layout of their parent.
  Making them absolute removes them from the parent's flow, so they
  don't consume space. `Popup` does this; so should you.
- **Start disabled.** A portal that's not open shouldn't be hit-tested.
  Use `setEnabled(false)` until `show()` is called. `Tooltip` does this
  by keeping `visible_ = false` until the delay elapses.
- **Override `arrange` to no-op if you position manually.** The
  default `arrange` will place the portal at the parent's inner origin
  with its own measured size, which is usually not what you want. If
  you're going to compute the rect in `onUpdate`, ignore the `arrange`
  argument.
- **`arrangeInto` is called after setting `rect`.** This arranges the
  portal's *children* into the portal's rect. It's the equivalent of
  what the parent would do, but for the portal's own children.

---

## 5. Lifetime

A portal is still a child of its logical parent. This means:

- It's kept alive by the parent's `children` vector.
- It's removed when the parent is removed (`removeFromParent` cascades
  via `wantsRemoval`).
- It's drawn in the parent's `draw()` pass — but only registered, not
  painted. The painting happens at the root.

The typical pattern for a menu on a button:

```cpp
auto menu = ContextMenu({ ... });
button->addChild(menu);   // lifetime
button->onClick = [button, menu]{
    menu->openBelow(button);
};
```

The `shared_ptr` captured by the lambda keeps the menu alive too, but
it's the `addChild` that makes it part of the tree. If you skip
`addChild`, the menu will work but won't be cleaned up when the button
is destroyed.

### 5.1 Removing a portal

`removeFromParent()` works normally. The portal is removed from its
parent's children during the next `cullRemovedChildren` pass. Any
pending registration in `framePortals` or `activePortals` uses
`weak_ptr`, so it's automatically cleaned up when the portal is
destroyed.

### 5.2 Focus

A portal can be focusable. `Popup` sets `setFocusable(true)` and
`setBlocksRaycast(true)`. When it opens, it calls
`UIContext::requestFocus(shared_from_this())`:

```cpp
void openInternal() {
    isOpen_ = true;
    setEnabled(true);
    style_.inlineBase.opacity = 1.0f;
    beginTransition();
    UIContext::get().requestFocus(shared_from_this());
}
```

This makes `Escape` reach the popup (which closes it), and lets Tab
cycle through the popup's focusable children without needing a focus
scope. (A focus scope would be more robust, but `Popup` uses focus
directly.)

---

## 6. Interaction with filters

A portal with a `filter:` goes through the layer render path — its
subtree is rendered to a render target, filtered, then composited on
top of the framebuffer. This works, but the render target is sized to
the portal's `rect` (intersected with the current clip), which for a
portal is the viewport. So a filtered portal creates a full-screen
render target. Don't do this unless you need it.

---

## 7. Common patterns

### 7.1 Right-click context menu

```cpp
auto box = VStack()->cls("bubbling-box");
box->addChild(Label("Right-click me"));

auto menu = ContextMenu({
    { "Action A", []{ std::printf("A\n"); } },
    { "Action B", []{ std::printf("B\n"); } },
});
box->addChild(menu);
box->onRightClick = [box, menu]{
    menu->openAt(UIContext::get().pointer.pos);
};
```

### 7.2 Tooltip on hover

```cpp
auto btn = Btn("Save", []{});
Tooltip::attach(btn, "Save your progress", 0.3f);
```

### 7.3 Dropdown inside a scrollable list

The dropdown handles this automatically — its list is a portal, so it
escapes the ScrollView's clipping.

```cpp
auto sv = ScrollView::create(LayoutType::Vertical);
for (int i = 0; i < 20; ++i) {
    auto row = HStack();
    row->addChild(Label("Row " + std::to_string(i)));
    row->addChild(Dropdown::create({"A", "B", "C"}, 0));
    sv->addChild(row);
}
```

Opening any dropdown shows its options above the ScrollView's clipping.

### 7.4 Modal (not a portal, but same "on top" effect)

```cpp
auto modal = Modal::create();
modal->children[0]->addChild(Label("Confirm?"));
modal->children[0]->addChild(Btn("OK", [modal]{ modal->hide(); }));
root->addChild(modal);
modal->show();
```

`Modal` covers the viewport with a semi-transparent black background
and consumes input. It's not a portal because it's usually added to the
root anyway.

---

## 8. Pitfalls

**Portals always draw on top.**

There's no way to put a portal *behind* other content. If you need a
node that floats in the middle of the z-order, use `position: absolute`
with `z-index` inside a stacking context — not a portal.

**Portals escape `overflow: hidden`. This is a feature, not a bug.**

But it means you can't use a portal to create a "clipped floating box".
If you want the box to be clipped, don't make it a portal.

**One-frame delay in hit-testing.**

Portals become clickable the frame **after** they first render. This is
because `activePortals` (used for hit-testing) is populated from the
previous frame's `framePortals`. In a real app, this is invisible. In
tests, it means you need to run one extra `frame()` before clicking on
a freshly-created portal.

```cpp
popup->openAt({100, 100});
env.frame(root);   // popup registers itself in framePortals
env.frame(root);   // framePortals → activePortals; now hit-testable
env.click(root, {100, 100});   // hits the popup
```

**`Popup::arrange` is a no-op.** Don't rely on the parent laying it out.
The popup positions itself in `onUpdate` every frame.

**Clicking a popup item that overlaps the popup trigger.**

If the trigger's `onClick` toggles the popup open, and the popup draws
on top of the trigger, clicking a popup item might also be interpreted
as clicking the trigger. This is why `Popup` calls
`UIContext::get().requestFocus(shared_from_this())` on open — focus
routing prevents this. But if you build a custom portal, be aware.

**A portal that isn't `position: absolute` consumes space in its
parent.**

By default, `arrangeAbsoluteAndPortals` arranges a portal at the
parent's inner origin with the portal's measured size. If the parent is
a `Vertical` layout, the portal **does not** consume vertical space —
the code path checks `c->isPortal()` before accumulating sizes. But if
you're using `Stack`, the parent's `items-h` / `items-v` might affect
the portal's position. In practice, always set
`position: absolute` on a portal.

**Portals in a `ScrollView` don't scroll with the content.**

The portal is drawn at the root's z-top, in screen space. Its rect is
computed from the anchor's rect, which *does* include the scroll
offset — so a tooltip anchored to a scrolled item will follow it. But
a portal that positions itself at a fixed screen coordinate will not
move with the scroll.

**Weak references in `activePortals` mean destroyed portals are skipped.**

If a portal is removed between frames, it won't be hit-tested next
frame. No leak, no dangling pointer. But it also means you can't rely
on `activePortals` size as a count of currently-open portals —
use the widget's own state (`popup->isOpen()`, `dropdown->getOpen()`).

**Debugging portals is awkward.**

`Debug::dumpTree` traverses the logical tree, so a portal appears as a
child of its logical parent — not where it's actually drawn. Keep this
in mind when reading dumps. `Debug::dumpStyle` reports the portal's
`rect` correctly (it's been arranged), but the visual position may
differ if the widget overrides `arrange`.