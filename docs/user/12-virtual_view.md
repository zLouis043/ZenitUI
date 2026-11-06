# VirtualView

VirtualView is a virtualized vertical list: it materializes only the
rows that are actually visible (plus a small buffer), so a list with
10.000 items costs roughly the same as a list with 12.

It is a ScrollView subclass. All scrolling behaviour — wheel, inertia,
scrollbar, clipping, scrollToY / getScrollY — is inherited from
ScrollView. VirtualView only adds row virtualization on top.

---

## 1. When to use it

Use VirtualView when:

- the list has more than a few hundred rows;
- the row count is known up front (or can be updated with
  setItemCount);
- rows can be built by a factory function
  std::shared_ptr<Layout>(int index).

Do **not** use it when:

- the list has a small, fixed number of children — a plain `Vertical`
  or `VStack` is simpler;
- rows need to *recycle* the same node instances across indices (that
  is the job of the planned `VirtualList`, see `PROPOSALS.md`
  Proposal 6 / `ROADMAP.md` §5.23). VirtualView creates and destroys
  nodes; it does not recycle.

---

## 2. Quick start

```cpp
auto v = VirtualView::create(
    [](int i) {
        auto row = std::make_shared<Layout>(LayoutType::Stack);
        row->size(Percent(100), Px(24));
        return row;
    },
    10000);
```

That is a 10.000-row list that materializes 12 nodes.

The overload without the index is for the case where the factory does
not depend on the row index:

```cpp
auto v = VirtualView::create(
    [] { return std::make_shared<Layout>(LayoutType::Stack); },
    10000);
```

Both overloads are static and return `std::shared_ptr<VirtualView>`.

---

## 3. API

### 3.1 Creation

```cpp
using ItemFactory = std::function<std::shared_ptr<Layout>(int index)>;

static std::shared_ptr<VirtualView> create(ItemFactory factory, int count);
static std::shared_ptr<VirtualView> create(
    std::function<std::shared_ptr<Layout>()> factory, int count);
```

`count` is clamped to `>= 0`. `factory` may return `nullptr` for an
index — in that case the row is skipped (no node is materialized for
it, and the cumulative height for that row falls back to 1 px).

### 3.2 Item count

```cpp
int  getItemCount() const noexcept;
void setItemCount(int count);
```

`setItemCount` clamps to `>= 0` and invalidates the cached cumulative
heights and the current materialization on the next frame. It does
**not** synchronously create or destroy children; that happens on the
next `arrange`.

### 3.3 Uniform height

```cpp
float getUniformHeight() const noexcept;
void  setUniformHeight(float px);
```

When `uniformHeight > 0`, VirtualView assumes every row is exactly
that tall and computes `cumulativeHeights` in O(N) arithmetic — the
factory is **not** called during measurement, only during
materialization. This is the fast path and should be preferred
whenever rows have a fixed height.

When `uniformHeight == 0` (the default), VirtualView measures every
row by calling the factory with each index and invoking `measure` on
the returned node. This is O(N) factory calls + N `measure` calls on
the frame where the cumulative heights are (re)built. See §5 for the
performance implications.

`setUniformHeight` invalidates the same caches as `setItemCount`.

### 3.4 Visibility queries

```cpp
int getFirstVisibleItem() const;
int getLastVisibleItem()  const;
```

Both are computed against the **current** scroll offset and the
current viewport height:

- `getFirstVisibleItem() == itemAtY(getScrollY())`
- `getLastVisibleItem()  == itemAtY(getScrollY() + rect.height - eps)`

`itemAtY(y)` is a binary search over the cumulative heights; it
returns the index whose span `[cumulative[i], cumulative[i+1])`
contains `y`, clamped to `[0, itemCount - 1]`.

If `itemCount == 0`, or if the cumulative heights have not been built
yet (before the first `measure`), both return `-1`. This is the
documented "not laid out yet" sentinel — see the test
`first_visible_before_measure_is_minus_one`.

### 3.5 Scrolling

```cpp
void scrollToItem(int index);
```

Clamps `index` to `[0, itemCount - 1]` and calls
`scrollToY(cumulativeHeights[index])`. In other words it aligns the
top of the row with the top of the viewport.

The usual `ScrollView` API is also available:
`scrollToY`, `getScrollY`, `getMaxScrollY`, `scrollToTop`,
`scrollToBottom`, and so on.

### 3.6 Callbacks

```cpp
std::function<void(Layout* node, int index)> onItemCreated;
```

Fires **every time a row is materialized**, i.e. every time a node is
created by the factory and inserted as a child. This is not
"once per row": if a row scrolls out of the buffer and later scrolls
back in, a *new* node is created and `onItemCreated` fires again with
the same index.

### 3.7 Lifecycle

```cpp
void removeAllChildren() override;
```

Clears the internal `materialized` vector **and** the base class
children. On the next `arrange`, the visible range is recomputed and
the rows are recreated via the factory. Unlike `setItemCount(0)`,
`removeAllChildren()` does **not** reset `itemCount`.

---

## 4. How materialization works

On each `arrange`:

1. `updateMaterialization()` runs *before* the parent `ScrollView`
   arranges.
2. It reads `getFirstVisibleItem()` and `getLastVisibleItem()` from
   the current scroll offset and viewport height.
3. It expands the range by 2 rows above and 2 rows below:
   `start = max(0, first - 2)`, `end = min(itemCount - 1, last + 2)`.
4. Every entry in `materialized` whose index is outside `[start, end]`
   is detached (`node->removeFromParent()`) and erased from the
   vector.
5. Every index in `[start, end]` that is not already materialized is
   built by calling the factory, positioned absolutely with
   `top = cumulative[i]`, `left = 0`, `width = 100%`,
   `height = cumulative[i+1] - cumulative[i]`, and appended as a
   child.
6. Finally the base `ScrollView::arrange` runs.

The children are positioned in **content coordinates**, not viewport
coordinates. The `ScrollView` scroll offset translates (or clips) the
subtree as usual, so the rows move with the scroll without
VirtualView having to reposition them on every frame.

The buffer of 2 rows above and 2 below means that scrolling by a
small amount does not immediately cause the boundary row to be
destroyed and a new one to be created. It also gives the row a chance
to be already materialized when the scroll is fast.

The number of materialized children is therefore bounded by
`ceil(viewportHeight / rowHeight) + 4` (2 above + 2 below) when
`uniformHeight > 0`. For mixed heights the bound is the same
expression with `rowHeight` replaced by the minimum row height — in
practice it is a small constant.

---

## 4. How materialization works

On each `arrange`:

1. `updateMaterialization()` runs *before* the parent `ScrollView`
   arranges.
2. It reads `getFirstVisibleItem()` and `getLastVisibleItem()` from
   the current scroll offset and viewport height.
3. It expands the range by 2 rows above and 2 rows below:
   `start = max(0, first - 2)`, `end = min(itemCount - 1, last + 2)`.
4. Every entry in `materialized` whose index is outside `[start, end]`
   is detached (`node->removeFromParent()`) and erased from the
   vector.
5. Every index in `[start, end]` that is not already materialized is
   built by calling the factory, positioned absolutely with
   `top = cumulative[i]`, `left = 0`, `width = 100%`,
   `height = cumulative[i+1] - cumulative[i]`, and appended as a
   child.
6. Finally the base `ScrollView::arrange` runs.

The children are positioned in **content coordinates**, not viewport
coordinates. The `ScrollView` scroll offset translates (or clips) the
subtree as usual, so the rows move with the scroll without
VirtualView having to reposition them on every frame.

The buffer of 2 rows above and 2 below means that scrolling by a
small amount does not immediately cause the boundary row to be
destroyed and a new one to be created. It also gives the row a chance
to be already materialized when the scroll is fast.

The number of materialized children is therefore bounded by
`ceil(viewportHeight / rowHeight) + 4` (2 above + 2 below) when
`uniformHeight > 0`. For mixed heights the bound is the same
expression with `rowHeight` replaced by the minimum row height — in
practice it is a small constant.

With auto height, the factory is called **once per item** the first
time the cumulative heights are built, and the returned node is only
used for `measure` — it is discarded and a fresh node is created
during materialization. **The factory must be cheap.** If it loads
assets or allocates heavily, either switch to `setUniformHeight` or
cache the row's data inside the factory closure and only build the
`Layout` on materialization.

`setItemCount` and `setUniformHeight` both clear `cumulativeHeights`,
so the next `measure` rebuilds them from scratch. Avoid calling them
every frame — they are model-change setters, not per-frame setters.

The `lastMeasuredWidth` cache means that, with auto height, the
cumulative heights are only rebuilt when the available width changes
(from a resize) or when `setItemCount` / `setUniformHeight` is called.

---

## 6. Interaction with scrolling

VirtualView is a `ScrollView`, so:

- `getMaxScrollY() == totalContentHeight - viewportHeight`;
- the scrollbar (if enabled on the parent / theme) reflects the full
  `itemCount`, not the materialized rows;
- wheel and inertia behave exactly as in any other `ScrollView`.

There is no built-in infinite scroll. To implement it, react to
`getLastVisibleItem()` in an `onUpdate` and call `setItemCount` with a
larger value when the user is close to the end. Because
`setItemCount` invalidates the cumulative heights, the next frame
rebuilds them; for a large list with auto height this is the O(N)
cost described in §5, so prefer `setUniformHeight` or a chunked
append strategy.

---

## 7. Common pitfalls

**1. Calling the factory for measurement is not free.**
The factory runs once per item during the first measure with auto
height. Do not put heavy work in it. If you need heavy work per row,
either use `uniformHeight` or cache the data in the closure and only
build the `Layout` when the index is actually materialized (you can
distinguish by tracking a `materialized` set in the closure, or by
doing the heavy work inside `onItemCreated`).

**2. The factory must return a node whose height matches the
cumulative height.**
For uniform height, VirtualView overrides the row's height to
`uniformHeight` on materialization, so a mismatch in the factory is
harmless. For auto height, the row's height is taken from the
measured size; if `onItemCreated` later changes the row's content
without changing its measured height, the row will draw outside its
slot. Keep row content stable, or call `setUniformHeight` /
`setItemCount` to force a rebuild.

**3. `onItemCreated` fires on re-materialization.**
If a row scrolls out of the buffer and back in, its node is new and
the callback fires again. Do not assume "one call per index, ever".
For idempotent setup (setting text, attaching a click that captures
the index by value) this is fine; for "register a global listener
once" it is not. Guard with an external `std::set<int>` if needed.

**4. `removeAllChildren` is not `setItemCount(0)`.**
`removeAllChildren` destroys the nodes but keeps `itemCount`; the rows
come back on the next frame. `setItemCount(0)` empties the model and
stays empty until the count is changed again.

**5. Queries before the first frame return -1.**
`getFirstVisibleItem` / `getLastVisibleItem` return `-1` before the
first measure, because the cumulative heights have not been built
yet. Do not treat `-1` as "empty list" — for an empty list they also
return `-1`. Check `getItemCount() == 0` first if you need to
disambiguate.

---

## 8. Examples

### 8.1 Fixed-height log viewer

```cpp
auto log = VirtualView::create(
    [](int) {
        auto row = std::make_shared<Layout>(LayoutType::Stack);
        row->size(Percent(100), Px(20));
        return row;
    },
    100000);

log->size(Percent(100), Px(400));
log->setUniformHeight(20.0f);

log->onItemCreated = [](Layout* row, int i) {
    // populate row from the log buffer at index i
};

log->scrollToItem(99999);   // bottom of the log
```

### 8.2 Variable-height chat

```cpp
auto chat = VirtualView::create(
    [&messages](int i) {
        auto row = std::make_shared<Layout>(LayoutType::Vertical);
        auto label = Text::create(messages[i].text);
        label->setWrap(true);
        row->addChild(label);
        return row;
    },
    (int)messages.size());

chat->size(Percent(100), Percent(100));
chat->onItemCreated = [&messages](Layout* row, int i) {
    // e.g. attach a timestamp tooltip
};
```

The factory here is cheap (the message text is already in memory), so
the one-call-per-item during the first measure is acceptable. If the
text had to be loaded from disk per row, move the load into a cache
populated before creating the VirtualView.

### 8.3 Updating the count on scroll (infinite scroll)

```cpp
// inside the screen's onUpdate
int last = vv->getLastVisibleItem();
if (last >= 0 && last >= vv->getItemCount() - 20) {
    vv->setItemCount(vv->getItemCount() + 50);
}
```

Note the O(N) cost of `setItemCount` with auto height — for very long
lists, prefer `setUniformHeight` or append in chunks with a debounce.

---

## 9. See also

- `docs/user/02-layout.md` — `Vertical` / `VStack` for non-virtualized
  lists.
- `docs/user/03-widgets.md` — `ScrollView`, the base class.
- `docs/architecture/PROPOSALS.md` Proposal 6 and `ROADMAP.md` §5.23
  — `VirtualList`, the planned recycling-based variant.
- `docs/api/Widgets.md` — full API reference.
