# VirtualView

VirtualView is a ScrollView subclass that materializes only the items in (or
near) the visible viewport. It is designed for large collections where every
item must exist as data (bound to a model with thousands of entries) but
constructing one Layout per item is too expensive.

## When to use it

Use VirtualView when:

- The collection has many items (typically hundreds to tens of thousands).
- Items are homogeneous or near-homogeneous in height.
- Losing per-item state when scrolling out of view is acceptable (VirtualView
  destroys and recreates out-of-window items).

Do not use VirtualView when:

- The collection is small (under ~100 items) — a plain Vertical layout is
  simpler.
- Items have highly variable heights AND depend on inherited style (font-size
  in em, inherited custom properties). See the auto-mode caveats below.
- You need per-item focus rings or hover state that survive scrolling out of
  view. VirtualView destroys out-of-window nodes; their focus/hover state is
  lost.

## Quick start

    auto list = VirtualView::create(
        [](int i) {
            return Text::create("Item #" + std::to_string(i));
        },
        10000);

    list->setUniformHeight(24.0f);
    list->size(Percent(100), Percent(100));
    root->addChild(list);

On the first frame(), VirtualView:

1. Computes the total content height (24 * 10000 = 240000px).
2. Finds the first and last visible item indices.
3. Materializes indices [first-2, last+2] (the buffer).
4. Arranges them as absolutely positioned children.

As you scroll, the window slides: out-of-window items are dropped, in-window
items are created.

## API

### create(factory, count)

Two overloads:

    static std::shared_ptr<VirtualView> create(
        std::function<std::shared_ptr<Layout>(int index)> factory,
        int count);

    static std::shared_ptr<VirtualView> create(
        std::function<std::shared_ptr<Layout>()> factory,
        int count);

The first is the standard form. The second wraps a zero-argument factory. The
factory may throw; exceptions propagate to the caller of frame().

### setItemCount(int) / getItemCount()

Total number of items. Negative values clamp to zero. Setting a new count
invalidates the height cache and re-triggers materialization on the next frame.

### setUniformHeight(float) / getUniformHeight()

Controls the height model:

- px > 0: uniform mode. Every item is treated as px tall. measure is never
  called. Recommended for lists of identical rows, or whenever the height is
  known.
- px == 0: auto mode. Every item's height is measured once per width change.
  See caveats below.

### getFirstVisibleItem() / getLastVisibleItem()

Return the indices of the first and last visible items (not the buffered
range). Return -1 when the list is empty or when heights have not yet been
computed (i.e. before the first measure).

### scrollToItem(int)

Scrolls so that the given item is at the top of the viewport. Clamps index to
[0, count-1].

### onItemCreated

    std::function<void(Layout* node, int index)> onItemCreated;

Called every time a node is materialized (added to children as part of the
window). Not called when the same node is re-shown after being dropped —
VirtualView recreates it. Use this hook to set up per-item bindings, hover
handlers, lazy-loaded data, etc.

### removeAllChildren()

Overridden from Layout. Clears children and the internal materialized tracker.
After this call, the next frame() re-materializes the current window from
scratch.

Note: if you only need to clear items, prefer setItemCount(0). It is the
semantically correct way and reuses the same code path.

## The two height modes

### Uniform mode (setUniformHeight(px > 0))

- measure: skipped entirely. cumulativeHeights[i] = i * px.
- Cost: O(N) to fill the cumulative array once, then O(log N) per query.
- Use this whenever possible.

### Auto mode (setUniformHeight(0))

- measure: called once per item on the first frame, and again on every width
  change.
- Cost: O(N) measure() calls at startup and on every resize. For 10000 items,
  expect roughly 10–30 ms on first frame and on each window resize.

#### Caveats

1. No parent inheritance. rebuildCumulative calls node->measure(availW, 1e9f)
   on an unattached node. If your item's height depends on inherited style
   (font-size in em, font-family, custom properties via var()), the measured
   height will differ from the in-tree height. Use setUniformHeight if that
   matters.

2. O(N) on resize. If the app window is frequently resized, auto mode
   re-measures everything. Prefer uniform mode.

3. Lazy factories. The factory is called once per item in auto mode (during
   rebuildCumulative), plus once per materialization. If your factory is
   expensive, cache inside it.

## The materialization window

For visible range [first, last], VirtualView materializes [first - 2, last + 2]:

- 2 items of buffer on each side produce smooth scrolling without popping.
- Out-of-window items are dropped on the next arrange.
- In-window items are kept (not recreated). Their state (hover, focus, scroll)
  persists.

onItemCreated fires at materialization, not at "becomes visible". With a
buffer of 2, the two usually coincide, but conceptually they are different.

## Performance

    Operation   Uniform                     Auto
    ---------   -------------------------   -----------------------------------
    measure     O(N) once                   O(N) once + O(N) per resize
    update      skipped for out-of-window   skipped for out-of-window
    arrange     O(window)                   O(window)
    draw        skipped by viewport clip    skipped by viewport clip

The arrange cost is the remaining bottleneck: every materialized node must be
positioned, even if it is off-screen. This is why VirtualView does not help
with an eager VStack of 10000 children — those are all in children and all
need arrange. VirtualView avoids the problem by not having 10000 children.

## Relationship with content-visibility (Proposal 8)

They solve different problems:

- VirtualView reduces the number of children (only visible + buffer are
  materialized). Ideal for lists where items are data-bound and lazily
  constructed.
- content-visibility: auto (Proposal 8) keeps all children in the tree but
  skips measure, update, and draw for those outside the clip rect. arrange is
  still O(N). Ideal for deep static subtrees that happen to be off-screen
  (e.g. a hidden Modal with 200 children).

They compose: a Modal opened on top of a VirtualView can benefit from
content-visibility on the modal content, while the underlying list uses
VirtualView.

## Common pitfalls

- Calling removeAllChildren() directly: safe in this version, but
  setItemCount(0) is the recommended way.
- Accessing getFirstVisibleItem() before the first frame(): returns -1.
- Calling arrange() directly: it is protected. Use Env::frame(root) in tests,
  or drive the app's normal frame loop.
- Relying on onItemCreated for cleanup: there is no onItemDestroyed. If you
  need it, wrap the item in a custom widget that holds the resource in its
  constructor/destructor.