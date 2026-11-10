# Proposals

Design documents for features that are planned but not yet
implemented. Each proposal is a contract: it fixes the API shape, the
semantics, and the files that will be touched, so that the
implementation phase is mechanical rather than exploratory.

This file is **not** a roadmap. The roadmap (see
[ROADMAP](../ROADMAP.md)) tracks *what* is planned and *when*. This
file tracks *how* the planned features will be shaped.

**Status:** proposals are not implemented. When a proposal ships, it
is moved to the roadmap as a §5.x item with the `[x]` checklist
filled in, and this document is updated to point at the shipped
implementation.

**Shipped:**

- ✅ **Proposal 1 — `ref`**: shipped as roadmap §5.16. See
  `docs/user/04-zmarkup.md` §2.4 and `docs/api/ZMarkup.md` §6 for
  the user-facing API. The "Open questions" sections below are kept for historical context;
  where the implementation diverged from the proposal, the diff is
  recorded under "Shipped — implementation notes" at the end of the file.
- ✅ **Proposal 2 — `Bindable<T>`**: shipped as roadmap §5.17. See
  `docs/user/11-bindable.md` and `docs/api/Bindable.md` for the
  user-facing API.
- ✅ **Proposal 3 — UIPrefab and Slot**: shipped as roadmap §5.18 and §5.19.
- ✅ **Proposal 4 — StyleParser Diagnostics**: shipped as roadmap §5.20.
- ✅ **Proposal 5 — Wrap for HStack**: shipped as roadmap §5.21.
- ✅ **Proposal 8 — VirtualView**: shipped as roadmap §5.24. See
  `docs/user/12-virtual_view.md` and `docs/api/Widgets.md` for the
  user-facing API.

---

## Index

- [Proposal 1 — `ref`: separation of style identity and logic lookup](#proposal-1--ref-separation-of-style-identity-and-logic-lookup)
- [Proposal 2 — `Bindable<T>`: lightweight data binding](#proposal-2--bindablet-lightweight-data-binding)
- [Proposal 3 — UIPrefab and Slot](#proposal-3--uiprefab-and-slot)
- [Proposal 4 — StyleParser Diagnostics](#proposal-4--styleparser-diagnostics)
- [Proposal 5 — Wrap for HStack](#proposal-5--wrap-for-hstack)
- [Proposal 6 — VirtualList](#proposal-6--virtuallist)
- [Proposal 7 — Drag & Drop](#proposal-7--drag--drop)
- [Proposal 8 — VirtualView](#proposal-8--virtualview)
- [Proposal 9 — Filter layer pipeline performance](#proposal-9--filter-layer-pipeline-performance)
- [Out of scope (future rounds)](#out-of-scope-future-rounds)

---

## Proposal 1 — `ref`: separation of style identity and logic lookup

**Priority:** high — unblocks `UIPrefab`, `VirtualList`, and clean
list rendering.

**Complexity:** low (~1 day).

**Depends on:** nothing.

**Status:** 🟡 proposed.

### 1.1 Problem

Today, `Layout::nodeId` is the only way to look up a node from C++:

~~~cpp
auto btn = ui.find<Button>("save");
~~~

And the id is set via `#save` in ZMarkup:

~~~zmarkup
Button#save "Save"
~~~

This works, but it conflates two distinct concepts:

1. **Style identity** — `#save` participates in CSS specificity as an
   id selector, with weight `(1, 0, 0)`. It beats any class selector.
2. **Logic identity** — the string that C++ code uses to look up the
   node.

The two concepts don't always want the same lifetime or cardinality:

- In a list of 100 delete buttons, each needs a unique logic handle
  so C++ can wire its callback. But they *don't* need a unique style
  id — they all want the same style, which is what a class is for.
- Today the only way to give them unique handles is to set
  `#delete_1`, `#delete_2`, ... on each one. This:
  - pollutes the CSS cascade with 100 id-specific rules (or, worse,
    forces the stylesheet to use `[id^="delete_"]` hacks);
  - bloats `BuildContext::byId` with 100 entries;
  - makes the ZMarkup source unreadable.

The correct model is the one every retained-mode UI framework
converges on (Qt's `objectName`, React's `ref`, Godot's `NodePath`):
**style identity and logic identity are two different strings**.

### 1.2 Solution

Introduce a **`ref`** — a string identifier that:
- is stored on `Layout::ref` (public field, alongside `nodeId`);
- does **not** participate in CSS matching;
- is used by `UINode::ref<T>(name)` and `UINode::refAll<T>(name)`;
- can be shared by multiple nodes.

The `#id` continues to exist, unchanged, and continues to be used by
`UINode::find<T>(id)`. The two are orthogonal.

### 1.3 C++ API

New public field on `Layout`:

~~~cpp
// Layout.hpp — public, next to nodeId
std::string nodeId;   // style id, matches #id in ZMarkup, participates in CSS specificity
std::string ref;      // logic ref, matches @ref in ZMarkup, does NOT participate in CSS
~~~

New fluent helper on `Layout` and `TLayout<Derived>`:

~~~cpp
void setRef(const std::string& r);
std::shared_ptr<Derived> withRef(const std::string& r);   // fluent, TLayout only
~~~

`withRef` is chosen over `ref()` because the fluent method would
otherwise shadow the inherited public `ref` field, which reads badly
at the call site. The existing `id` field / `id()` method share a
name and work because `id()` is on `TLayout` while the field is on
`Layout`; replicating that pattern for `ref` is technically possible
but confusing.

New methods on `UINode`:

~~~cpp
template <typename T = Layout>
std::shared_ptr<T> ref(const std::string& name) const;

template <typename T = Layout>
std::vector<std::shared_ptr<T>> refAll(const std::string& name) const;
~~~

New methods on `BuildContext`:

~~~cpp
std::unordered_map<std::string, std::vector<std::shared_ptr<Layout>>> byRef;
~~~

Populated by `ZMarkup::create` when `Element::ref` is non-empty.

### 1.4 ZMarkup API

New prefix `@` in the modifier section of a tag:

~~~zmarkup
Button.btn-danger@delete "X"
Button.btn-danger@delete "X"
Button.btn-danger@delete "X"
~~~

Multiple `@ref` on the same node is **not** allowed (only the last
one wins, but a warning is logged at parse time — see §1.6).

Grammar extension: after `tag`, `.class`, `#id`, a new token `@name`
is accepted. Order is free among `.`, `#`, `@`, matching the existing
free order of `.` and `#`.

Examples:

~~~zmarkup
Button.btn-danger@delete "X"          // class + ref
Button#save@main_save "Save"          // id + ref
Button@action "OK"                    // ref only
Button#save "Save"                    // id only (existing, unchanged)
Button@action.btn-danger "OK"         // ref + class
~~~

`Element` gains:

~~~cpp
std::string ref;   // populated by the parser from @name
~~~

### 1.5 Semantics: `find` vs `ref` vs `refAll`

| Method | Source | Cardinality | Empty result | Multiple results |
|--------|--------|-------------|--------------|------------------|
| `find<T>(id)` | `byId` | 0 or 1 | `nullptr` + Warn | N/A (last wins by map overwrite) |
| `ref<T>(name)` | `byRef` | 0, 1, or N | `nullptr` + Warn | first + Warn |
| `refAll<T>(name)` | `byRef` | 0..N | `{}` + Debug | all |

`find` is unchanged in behavior, except that it now logs a warning
when the id is not found. This is the "critical" addition mentioned in
the review — it turns a silent `nullptr` into a diagnostic.

The distinction between `Warn` and `Debug` matters:
- `find("typo")` and `ref("typo")` are almost always bugs → `Warn`.
- `refAll("list-items")` on an empty list is legitimate → `Debug`.

**Warning deduplication.** All warnings are deduplicated per-session:
a `std::unordered_set<std::string>` keyed on `"find:" + id` /
`"ref:" + name` / `"refAll:" + name` is checked before logging.
This prevents a warning from spamming every frame if the user's
`onUpdate` calls `find` in a loop.

### 1.6 Logging

New log calls in `UINode`:

~~~cpp
// find: warning if not found
if (!node) logWarn("ZMarkup", "", 0, 0,
    "UINode::find('" + id + "'): no node with this id");

// ref: warning if not found, warning if ambiguous
if (matches.empty()) logWarn("ZMarkup", "", 0, 0,
    "UINode::ref('" + name + "'): no node with this ref");
else if (matches.size() > 1) logWarn("ZMarkup", "", 0, 0,
    "UINode::ref('" + name + "'): " + std::to_string(matches.size()) +
    " nodes match, returning the first. Use refAll for the full list.");

// refAll: debug if empty (legitimate for optional lists)
if (matches.empty()) logDebug("ZMarkup", "", 0, 0,
    "UINode::refAll('" + name + "'): empty");
~~~

**Dedup.** A static `std::unordered_set<std::string>` in `UINode.cpp`
(or a per-`UINode` set, see §1.11) tracks the last-logged keys. Each
key is `"find:" + id` etc. The set is never cleared (once-per-process
diagnosis, not once-per-frame).

**Warning on duplicate `@ref` in markup.** If the parser sees
`@a @b` on the same tag, it logs a warning and keeps the last one:

~~~cpp
if (!e.ref.empty())
    logWarn("ZMarkup", "", 0, 0,
        "duplicate @ref on tag, keeping the last one ('" + name + "')");
e.ref = std::move(name);
~~~

### 1.7 File-by-file changes

| File | Change |
|------|--------|
| `src/ui/Layout.hpp` | Add `std::string ref;` public field, `setRef()`, `withRef()` fluent |
| `src/ui/ZMarkup.hpp` | Add `Element::ref`, extend parser for `@name`, populate `Layout::ref` in `Registry::create`, populate `ctx.byRef` |
| `src/ui/ZMarkup.hpp` (UINode) | Add `ref<T>()`, `refAll<T>()`, add warning logic to `find<T>()` |
| `src/ui/Debug.hpp` | `dumpTree` and `dumpStyle` show `@ref` next to `#id` |

Estimated: ~80 lines of additions, zero deletions.

### 1.8 Tests

New tests in `tests/test_zmarkup_build.cpp`:

- Parse `Button@foo "X"` → `Element::ref == "foo"`.
- Parse `Button#id@foo "X"` → `nodeId == "id"`, `ref == "foo"`.
- Parse `Button@foo.btn "X"` → `ref == "foo"`, class `btn`.
- `Registry::create` populates `Layout::ref` from `Element::ref`.
- `BuildContext::byRef` has one entry after one `@foo`.
- `BuildContext::byRef["foo"].size() == 3` after three `@foo` nodes.
- `ui.ref<Layout>("foo")` returns the first of three.
- `ui.refAll<Layout>("foo")` returns all three.
- `ui.ref<Layout>("nonexistent")` returns `nullptr`.
- `ui.refAll<Layout>("nonexistent")` returns `{}`.
- `ui.find<Layout>("id")` still works for `#id` (regression).
- `ui.find<Layout>("nonexistent")` returns `nullptr` (regression).

New tests in `tests/test_layout.cpp`:

- `node->setRef("x")` updates `node->ref`.
- `node->withRef("x")` returns `shared_ptr<Derived>` (fluent).

Total: ~12 new tests.

### 1.9 Documentation

| File | Change |
|------|--------|
| `docs/user/04-zmarkup.md` | §2.3 "Id" → split into §2.3 "Id (style)" and §2.4 "Ref (logic)". New example with list of `@delete`. §3 `UINode` → add `ref`/`refAll`. |
| `docs/api/ZMarkup.md` | §2 `Element` — add `ref` field. §6 `UINode` — add `ref<T>`, `refAll<T>`, note `find` warning. |
| `docs/api/Layout.md` | §3 "Style — classes, id, tag" → add `ref` subsection. |
| `docs/internals/01-architecture.md` | §2 — mention `ref` as separate from `id`. |
| `docs/CHEATSHEET.md` | Add a row in the "ZMarkup syntax" table. |
| `docs/GLOSSARY.md` | Add "Ref" entry. |
| `docs/ROADMAP.md` | Add §5.16 as shipped, with checklist. |

### 1.10 What does NOT change

- `Layout::nodeId` exists and works exactly as before.
- `Layout::setId()` works exactly as before.
- `Layout::hasId()` works exactly as before.
- `UINode::find<T>()` works exactly as before, **plus** the warning.
- `BuildContext::byId` exists and works exactly as before.
- Parser accepts `#id` and `.class` exactly as before.
- CSS specificity for `#id` is unchanged.
- `StyleParser` is untouched (ZStyle has no `@ref` concept).

### 1.11 Open questions

1. **Per-`UINode` dedup vs global dedup.** A per-`UINode` set means
   two different UI trees can log the same missing id twice. A global
   set means once. Decision: **per-`UINode`**, because it's cheap
   (one `unordered_set` per handle) and semantically correct (each
   tree is its own diagnostic context). Global would require a
   mutex or thread-local, which is overkill.

2. **`ref` mutability.** Should `Layout::ref` be `const` after
   `Registry::create`? Decision: **no, it's mutable**. Users might
   want to change a ref at runtime. If they do, they must re-register
   in `byRef` manually (this is documented). A future extension could
   add `UINode::setRef` for the common case, but it's not in this
   proposal.

3. **Fluent method name.** `withRef` chosen over `ref()` or `ref_()`.
   The existing `id` field / `id()` method share a name and work
   because the field is on `Layout` and the fluent method is on
   `TLayout` — the parser disambiguates at the call site. Replicating
   that pattern for `ref` would work syntactically, but `ref()` on
   `TLayout` shadows the inherited public `ref` field, which reads
   badly. `withRef` is unambiguous, matches the `with(child)` helper
   pattern, and clearly signals "fluent setter". Final: field `ref`,
   fluent `withRef`.

4. **`@ref` syntax in `UINode::onX` helpers.** Today
   `ui.onClick("save", cb)` looks up the node by `#id`. A natural
   extension is to accept `ui.onClick("@delete", cb)` — the leading
   `@` disambiguates "look up by ref" from "look up by id". For
   `refAll` semantics the callback would be attached to *every* node
   with that ref. This is ergonomic (no
   `ui.ref<Button>("delete")->onClick = ...` boilerplate) but it's an
   extension, not a requirement for the proposal.

   Decision: **deferred to a follow-up**. Proposal 1 ships `ref` and
   `refAll` only. If `@ref` in `onX` helpers proves useful in real
   usage (the game has 100 delete buttons), it can be added in §5.16a
   without breaking anything. The `@` prefix is reserved for this
   purpose: `onX("...")` where the string starts with `@` is
   currently undefined behavior (no node has an id starting with `@`),
   so adding it later is backwards-compatible.

---

## Proposal 2 — `Bindable<T>`: lightweight data binding

**Priority:** medium — halves boilerplate in UI controllers.

**Complexity:** low (~half a day).

**Depends on:** nothing.

**Status:** 🟡 proposed.

### 2.1 Problem

Today, wiring a model value to a widget requires a `std::function`
callback on the widget:

~~~cpp
auto slider   = Slider::create(0.5f);
auto progress = ProgressBar::create(0.5f);

slider->onValueChanged = [progress](float v) {
    progress->setValue(v);
};
~~~

This works, but it's boilerplate that repeats in every screen:

- The controller has to hold references to both widgets.
- The callback captures `progress` by `shared_ptr`, keeping it alive
  as long as the slider.
- A single change of model value has to be propagated manually.

For a game with 10 settings screens, this is ~200 lines of glue.

### 2.2 Solution

A tiny template, header-only, that holds a value and notifies
subscribers when the value changes:

~~~cpp
template <typename T>
class Bindable { /* ... */ };
~~~

Usage:

~~~cpp
Bindable<float> volume{0.5f};

auto slider   = Slider::create(volume.get());
auto progress = ProgressBar::create(volume.get());

slider->onValueChanged = [&volume](float v){ volume.set(v); };

auto sub = volume.onChanged([progress](const float& v){
    progress->setValue(v);
});
~~~

`Bindable` is **not** a widget, **not** a `Layout`, and **not**
involved in the frame loop. It's a plain value with observer
semantics. The framework doesn't tick it, doesn't route input to it,
doesn't draw it.

### 2.3 API

~~~cpp
namespace ZenitUI {

template <typename T>
class Bindable {
public:
    class Subscription {
    public:
        Subscription() = default;
        ~Subscription();
        Subscription(Subscription&&) noexcept;
        Subscription& operator=(Subscription&&) noexcept;
        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        void unsubscribe();
        bool active() const;

    private:
        friend class Bindable;

        struct ControlBlock {
            Bindable* owner_{nullptr};
            bool      alive_{true};
        };

        Subscription(std::shared_ptr<ControlBlock> ctrl, uint64_t token);
        std::shared_ptr<ControlBlock> ctrl_;
        uint64_t token_{0};
    };

    Bindable() = default;
    explicit Bindable(T initial);
    ~Bindable();

    Bindable(const Bindable&)            = delete;
    Bindable& operator=(const Bindable&) = delete;
    Bindable(Bindable&&)                 = delete;
    Bindable& operator=(Bindable&&)      = delete;

    const T& get() const;
    void     set(T v);

    Subscription onChanged(std::function<void(const T&)> cb,
                           bool fireImmediately = false);

    // Convenience: bind to a widget method, with lifetime safety.
    // `Arg` is deduced, so the method can take the parameter by value
    // (void(ProgressBar::*)(float)) or by const-ref
    // (void(Label::*)(const std::string&)) — both work without a
    // wrapper lambda.
    template <typename W, typename Arg>
    Subscription bindTo(std::weak_ptr<W> widget,
                        void (W::*method)(Arg),
                        bool fireImmediately = false);

private:
    struct Entry {
        uint64_t token;
        std::function<void(const T&)> cb;
    };
    std::vector<Entry> subscribers_;
    std::shared_ptr<typename Subscription::ControlBlock> ctrl_;
    uint64_t nextToken_{1};
    T value_{};

    void unsubscribe_(uint64_t token);
    bool isStillSubscribed(uint64_t token) const;
};

} // namespace ZenitUI
~~~

#### 2.3.1 Internal storage

Subscribers are stored in a `std::vector<Entry>`, where
`Entry = { uint64_t token; std::function<void(const T&)> cb; }`.
Insertion appends to the vector; the token is monotonic across
unsubscribes. `unsubscribe_()` erases the entry with
`std::erase_if`, which is O(N) but N is typically 1–5.

A `std::vector` is chosen over `std::unordered_map` for three
reasons:

- **Ordering**: the standard guarantees insertion order for a
  `std::vector`, but not for a `std::unordered_map` (hash iteration
  order depends on buckets and rehash). The notification promise in
  §2.5 ("subscribers are notified in registration order") requires
  the vector.
- **Cache**: iteration over a contiguous vector is faster than
  iteration over a hash map, especially for small N.
- **Snapshot cost**: `auto snapshot = subscribers_` copies a vector
  of N small entries, which is cheap; copying a map is more
  expensive.

#### 2.3.2 `bindTo` signature

`Arg` is a separate template parameter because the widget setters in
ZenitUI take their argument either by value
(`ProgressBar::setValue(float)`, `Slider::setValue(float)`,
`Layout::setEnabled(bool)`) or by const reference
(`Label::setText(const std::string&)`). A method pointer
`void (ProgressBar::*)(float)` does **not** convert to
`void (ProgressBar::*)(const float&)` in C++. Deducing `Arg`
separately makes the call site work for both:

~~~cpp
volume.bindTo(progress, &ProgressBar::setValue);   // Arg = float
name.bindTo(label,      &Label::setText);          // Arg = const std::string&
~~~

Without the deduction, every call site would need a wrapper lambda,
which defeats the purpose of `bindTo`.

### 2.4 Why `Subscription` and not just `bindTo`

`bindTo(weak_ptr, method)` handles lifetime correctly, but it has a
subtle failure mode: **the caller has to remember to use it**. If the
caller writes `onChanged([ptr](...){...})` directly, the bug (dangling
`shared_ptr` or `weak_ptr` capture) comes right back.

`Subscription` prevents the bug **by design**: the only way to
subscribe is `onChanged`, which returns a `Subscription`. The
`Subscription` is RAII — when it's destroyed, the callback is
unregistered. If the widget that owns the `Subscription` is destroyed,
the callback is gone with it. No leak, no dangling.

`bindTo` is still provided as a convenience for users who don't want
to manage a `Subscription` member. It uses `weak_ptr` internally and
returns a `Subscription` too, so both APIs are consistent.

### 2.5 Semantics

**`get()`** — returns `const T&`. No side effects.

**`set(T v)`** — if `v == value_`, no-op (no notification).
Otherwise, `value_ = std::move(v)`, then notify every active
subscriber.

**Notification order.** Subscribers are notified in registration
order (guaranteed by the `std::vector` storage — see §2.3.1). If a
subscriber unsubscribes during the notification, it's skipped in the
current pass.

**Recursion.** If a subscriber calls `set()` on the **same**
`Bindable` during notification, the new value is set and a new
notification pass begins. This is allowed but discouraged. If the
new value equals the current one, no-op. This prevents infinite loops
for the common "callback echoes the current value" case.

**Two `Bindable`s notifying each other.** If `A.onChanged` calls
`B.set` and `B.onChanged` calls `A.set`, the user has created an
infinite loop. `Bindable` does not detect this; it's the user's
responsibility.

#### 2.5.1 Notification and unsubscription during iteration

The internal storage is a `std::vector<Entry>` (see §2.3.1). During
`set()`:

~~~cpp
// Pseudocode
auto snapshot = subscribers_;   // copy the vector
for (const auto& entry : snapshot) {
    if (isStillSubscribed(entry.token))
        entry.cb(value_);
}
~~~

`isStillSubscribed(token)` is a linear scan of `subscribers_` (or a
`std::find_if` on the vector, which for N ≤ 5 is faster than any
hash lookup). This is safe against `unsubscribe` during the loop:
the entry is removed from `subscribers_`, and the check skips it on
the next iteration. The cost is one vector copy per notification —
negligible for small N.

#### 2.5.2 Lifetime safety: `Subscription` outliving `Bindable`

A `Subscription` can outlive its `Bindable<T>`. Example:

~~~cpp
Bindable<float> volume;
auto sub = volume.onChanged([...]);
// volume goes out of scope before sub:
~~~

Without protection, `~Subscription` would dereference a dangling
`Bindable*`. The `ControlBlock` prevents this:

- `Bindable` owns a `std::shared_ptr<ControlBlock>`. Every
  `Subscription` holds a copy of the same `shared_ptr`.
- `~Bindable` sets `ctrl_->alive_ = false` **before** its own
  destruction completes. The `ControlBlock` itself stays alive
  because `Subscription` still holds a reference.
- `~Subscription` checks `ctrl_->alive_` before dereferencing
  `ctrl_->owner_`. If the `Bindable` is already dead, it's a no-op.

This costs one `shared_ptr` copy per subscription (16 bytes) and one
`shared_ptr` refcount bump at subscribe time. In exchange, the
following are all safe:

~~~cpp
// Case 1: Subscription outlives Bindable
Subscription s;
{
    Bindable<float> b;
    s = b.onChanged([...]);
}
// b is destroyed; s is still valid but inactive
s.unsubscribe();   // no-op, no crash

// Case 2: Bindable outlives Subscription (the normal case)
Bindable<float> b;
{
    auto s = b.onChanged([...]);
    // ...
}
// s is destroyed; callback is unregistered; b continues

// Case 3: Both destroyed together, any order
{
    Bindable<float> b;
    auto s = b.onChanged([...]);
}
~~~

Note: `Bindable` itself is non-movable and non-copyable (see §2.3),
so the `ControlBlock` can't be shared across instances. Each
`Bindable` has exactly one `ControlBlock`.

### 2.6 Thread safety

**Single-thread only.** `Bindable<T>` is not thread-safe. It is
documented as such. The framework is single-thread; `Bindable` lives
in the same constraint.

If a game has multiple threads that produce values for the UI, the
producer thread must post to the main thread (via a queue, a
`std::atomic`, etc.) before calling `set()`. This is not `Bindable`'s
job.

### 2.7 File-by-file changes

| File | Change |
|------|--------|
| `src/ui/Bindable.hpp` | New file, header-only, ~200 lines |
| `src/ui/UI.hpp` | `#include "Bindable.hpp"` |

Estimated: 1 new file, 1 include.

### 2.8 Tests

New file `tests/test_bindable.cpp`:

- `Bindable<int> b{42}` → `b.get() == 42`.
- `b.set(42)` → no callback.
- `b.set(43)` → callback fires with 43.
- Two subscribers both fire in registration order.
- Subscription RAII: destroy subscription → no callback.
- `unsubscribe()` explicit → no callback.
- `bindTo(weak_ptr, &Widget::setValue)` with `setValue(float)` →
  method called with new value (Arg deduced as `float`).
- `bindTo(weak_ptr, &Widget::setText)` with
  `setText(const std::string&)` → method called (Arg deduced as
  `const std::string&`).
- `bindTo` with expired `weak_ptr` → no crash, no call.
- `bindTo(..., fireImmediately = true)` → method called once with
  current value at subscribe time.
- Subscriber that calls `unsubscribe` in its own callback → no
  crash, current value still delivered to other subscribers.
- Subscriber that calls `set()` on same `Bindable` with same value →
  no recursion.
- Move-construction of `Bindable` → deleted (compile-time test via
  `static_assert(!std::is_move_constructible_v<...>)`).
- `Subscription` outlives `Bindable`: `~Subscription` does not crash.
- `Bindable` destroyed, then `Subscription::unsubscribe()` called →
  no-op, no crash.
- `Bindable` destroyed, then `~Subscription` → no crash.

Total: ~16 new tests.

### 2.9 Documentation

| File | Change |
|------|--------|
| `docs/user/11-bindable.md` | New file — motivation, API, examples, thread-safety note |
| `docs/api/Bindable.md` | New file — full API reference |
| `docs/GLOSSARY.md` | Add "Bindable" entry |
| `docs/CHEATSHEET.md` | Add "Data binding" section with 5-line example |
| `docs/ROADMAP.md` | Add §5.17 as shipped, with checklist |

### 2.10 What does NOT change

- All `on*` callbacks on widgets (`onValueChanged`, `onToggle`, etc.)
  are untouched.
- `Bindable<T>` is opt-in. No existing code uses it. No existing code
  breaks.
- No new dependency: `Bindable<T>` uses only `<functional>`,
  `<memory>`, `<vector>`, `<cstdint>`, `<utility>` — all already
  included by `Common.hpp`.
- No `std::atomic` is used. The `ControlBlock::alive_` flag is a
  plain `bool`, safe because `Bindable` is single-thread (see §2.6).

### 2.11 Open questions

1. **`const T&` vs `T` in callback signature.** `void(const T&)`
   avoids copies for large `T`. Decision: **`const T&`**. If the
   user wants a copy, they can copy inside the callback.

2. **`set(T v)` by value or by `const T&`?** By value + move:
   allows the caller to `std::move` for large `T`, and the internal
   `value_ = std::move(v)` is one move, not one copy. Decision:
   **`set(T v)`**.

3. **`fireImmediately` — when?** `onChanged(cb, /*fireImmediately=*/true)`
   calls `cb` with the current value at subscribe time, before
   returning the `Subscription`. Useful for "attach a progress bar and
   it updates to the current value at once". Default `false` to keep
   the common case zero-cost. Decision: **include it**, as a
   default-false second parameter.

4. **Multiple `Bindable<T>` types — is `T` required to be
   equality-comparable?** Yes, `set()` uses `v == value_`. This is
   documented. If `T` is not equality-comparable, use a wrapper or
   make the set explicit. Decision: **requires `operator==`**.

---

## Proposal 3 — UIPrefab and Slot

**Priority:** high — unblocks `VirtualList` and reusable UI components.
**Complexity:** medium (~1-2 days).
**Depends on:** `ref` (shipped in §5.16).
**Status:** 🟡 proposed.

### 3.1 Problem

Currently, to reuse a UI component like a `Card` or a `SettingsRow`, you either have to:
1. Copy and paste the ZMarkup definition everywhere.
2. Write a custom C++ widget that inherits from `Layout` and builds its children imperatively in `onBuild()`.

This defeats the purpose of having a declarative ZMarkup language. We need a way to define a composite widget once in ZMarkup (a "Prefab") and instantiate it multiple times, injecting custom children into specific areas (Slots).

### 3.2 Solution

Introduce **`UIPrefab`** and **`Slot`**:
- **`UIPrefab`**: A parsed ZMarkup AST (`Element`) stored in the `Registry` under a custom tag name (e.g., `SettingsRow`).
- **`Slot`**: A special ZMarkup tag (`Slot`) used inside a prefab's definition. When the prefab is instantiated, any children passed to it are injected into the `<Slot>`.

Because we recently shipped `@ref` (Proposal 1), the C++ code can easily query the instantiated prefab for specific internal nodes to update text, values, or attach callbacks, completely removing the need for complex template strings in the markup.

### 3.3 ZMarkup API (Usage)

**1. Defining the Prefab (e.g., `assets/card.zmarkup`):**
```zmarkup
VStack.card-container {
    Text.card-title@title "Default Title"
    Slot  // Children injected here
}

```

**2. Instantiating the Prefab (in `main.zmarkup`):**

```zmarkup
HStack {
    Card@card1 {
        Text "This goes into the slot!"
        Button "Action"
    }
    Card.special-card@card2 {
        Text "Another card content"
    }
}

```

### 3.4 C++ API

New methods on `ZMarkup::Registry`:

```cpp
void registerPrefab(const std::string& tagName, const std::string& markupSource);
bool registerPrefabFile(const std::string& tagName, const std::string& filepath);

```

Internally, `registerPrefab` parses the markup into an `Element` AST and registers a custom factory for `tagName`. When the factory is invoked, it:

1. Clones the prefab's `Element` AST.
2. Merges the instance's classes, id, and ref into the root of the cloned AST.
3. Passes the instance's children down the build context.
4. When the builder encounters a `Slot` tag, it iterates and builds the passed children instead of the `Slot`.

### 3.5 Semantics and Edge Cases

* **Root merging**: The attributes of the instance (e.g., `.my-class`, `#id`, `@ref`) are applied to the *root* node of the prefab. E.g., `<Card.extra-padding>` adds `.extra-padding` to the prefab's root `VStack`.
* **Multiple Slots**: For MVP (Minimum Viable Product), a single anonymous `Slot` tag is supported. If a prefab has no `Slot`, any children passed to it at instantiation are ignored (with a debug warning).
* **Default Slot Content**: If `Slot { Text "Empty" }` has children in the prefab definition, they are rendered *only* if the instantiating tag provides no children.
* **Performance**: The prefab markup is parsed *once* at registration. Instantiation just traverses and builds from the in-memory AST (`Element`), which is extremely fast and allocates no extra strings.

### 3.6 File-by-file changes

| File | Change |
| --- | --- |
| `src/ui/ZMarkup.hpp` | Add `Registry::registerPrefab` and `registerPrefabFile`. Add `Slot` to built-in tags. |
| `src/ui/ZMarkup.cpp` | Implement AST cloning/merging logic. Update `BuildContext` to forward children to `Slot`. |
| `tests/test_zmarkup_build.cpp` | Add tests for prefab registration, root attribute merging, and slot injection. |
| `docs/user/04-zmarkup.md` | New section §6 "Prefabs and Slots". |
| `docs/api/ZMarkup.md` | Document `registerPrefab`. |
| `docs/ROADMAP.md` | Track §5.18 and §5.19. |

Estimated: ~100-150 lines of additions.

### 3.7 Tests

* Register a prefab and instantiate it.
* Instantiate a prefab passing a child -> child appears inside the `Slot`.
* Instantiate a prefab without children -> `Slot` default children are rendered.
* Prefab instance with `.class`, `#id`, `@ref` -> applied to the prefab root.
* Instantiating a prefab that has no `Slot` but passing children -> warning logged, children ignored.

## Proposal 4 — StyleParser Diagnostics

**Priority:** high — without this, a typo in a `.zstyle` file is
silently ignored and the developer has to bisect the stylesheet to
find it.

**Complexity:** medium (~1 day).

**Depends on:** nothing.

**Status:** ✅ shipped as roadmap §5.20.

### 4.1 Problem

The `.zstyle` parser was defensive — it ignored unknown properties
and unknown pseudo-classes — but it reported nothing. A typo like
`colr: #fff` would simply not apply, with no diagnostic. A misspelled
pseudo-class (`:hovver`) would silently fail to match. A duplicated
`@keyframes` would silently overwrite the previous one. Debugging
these required bisecting the stylesheet by hand.

Worse, the parser had no notion of source location. Even when a
diagnostic could have been produced, there was no way to say *where*
in the file the problem was.

### 4.2 Solution

Introduce a structured diagnostic system:

- **`SourceSpan`** — file, line, column, token length. Every AST node
  (declarations, rules, keyframe frames) carries one.
- **`DiagnosticCode`** and **`DiagnosticSeverity`** — enumerated
  categories and severities (`Info`, `Warning`, `Error`).
- **`DiagnosticSink`** — an abstract interface that receives
  diagnostics. Three implementations: `NullDiagnosticSink` (default,
  discards), `CollectingDiagnosticSink` (stores for later inspection),
  `ConsoleDiagnosticSink` (prints via `Logger`).
- **`suggestSimilar`** — Levenshtein-based "did you mean?" suggestions
  for unknown properties and pseudo-classes.
- **`strict` mode** — a flag on `loadStyleString` / `loadStyleFile`
  that promotes warnings to errors and makes the load return `false`
  if any diagnostic is an error. Useful in CI and debug builds.

The parser is unchanged in its error tolerance: it still ignores
bad declarations and continues. Only the reporting is new.

### 4.3 API

~~~cpp
namespace ZenitUI
{

struct SourceSpan
{
    std::string   file;
    std::uint32_t line   = 0;
    std::uint32_t column = 0;
    std::uint32_t length = 0;

    [[nodiscard]] bool valid() const noexcept;
};

enum class DiagnosticSeverity { Info, Warning, Error };

enum class DiagnosticCode
{
    UnterminatedString, UnbalancedBrace, UnexpectedToken,
    UnknownProperty, InvalidValue, UnknownPseudo, UnknownPart,
    UnresolvedVariable, UnusedVariable,
    UnknownSelectorTag, SelectorMatchesNothing, DuplicateRule,
};

struct ZStyleDiagnostic
{
    DiagnosticCode     code;
    DiagnosticSeverity severity;
    SourceSpan         span;
    std::string        message;
    std::string        hint;
    std::optional<SourceSpan> related;
};

class DiagnosticSink
{
public:
    virtual ~DiagnosticSink() = default;
    virtual void report(ZStyleDiagnostic d) = 0;
    virtual bool hasErrors() const = 0;
    virtual std::vector<ZStyleDiagnostic> sorted() const = 0;
};

class NullDiagnosticSink final : public DiagnosticSink { /* ... */ };
class CollectingDiagnosticSink final : public DiagnosticSink { /* ... */ };

std::size_t editDistance(std::string_view a, std::string_view b);

std::vector<std::string> suggestSimilar(
    std::string_view unknown,
    const std::vector<std::string_view>& candidates,
    std::size_t maxDistance    = 2,
    std::size_t maxSuggestions = 3);

const std::vector<std::string_view>& getKnownStyleProperties();

}
~~~

The load functions gain two optional parameters:

~~~cpp
bool loadStyleString(
    std::string_view src,
    Theme& theme = Theme::get(),
    const std::string& fileName = "<string>",
    DiagnosticSink* sink = nullptr,
    bool strict = false);

bool loadStyleFile(
    const std::filesystem::path& path,
    Theme& theme = Theme::get(),
    DiagnosticSink* sink = nullptr,
    bool strict = false);
~~~

Both default `sink` to `nullptr` (no diagnostics collected) and
`strict` to `false` (warnings don't fail the load). Fully backward
compatible.

### 4.4 Semantics

**Unknown properties.** `applyStyleDeclaration` checks
`applyStyleAttr`; if it returns `false` (property not recognized), a
diagnostic is emitted with `DiagnosticCode::UnknownProperty` and a
hint built from `suggestSimilar`.

**Unknown pseudo-classes.** The parser recognizes `:hover`,
`:pressed`, `:focus`, `:disabled`, `:checked`. Any other `:name`
emits `DiagnosticCode::UnknownPseudo`.

**Duplicate `@keyframes`.** `applyStyleSheet` tracks keyframe names
in a `std::set`. A second `@keyframes` with the same name emits
`DiagnosticCode::DuplicateRule`.

**Strict mode.** Every warning is emitted as an error if
`strict == true`. The load function returns `false` if
`sink->hasErrors()`.

**Default sink.** When `sink == nullptr`, an internal
`NullDiagnosticSink` is used and diagnostics go to `Logger` (the
existing behavior). This preserves backward compatibility.

**Location accuracy.** Every declaration, rule, and frame carries
the line/column of the **start of the token**, not of the enclosing
block. This matches common LSP conventions.

### 4.5 File-by-file changes

| File | Change |
|------|--------|
| `include/Diagnostics.hpp` | New file. `SourceSpan`, `DiagnosticCode`, `DiagnosticSeverity`, `ZStyleDiagnostic`, `DiagnosticSink` and its implementations, `editDistance`, `suggestSimilar`, `getKnownStyleProperties`. |
| `src/internal/Diagnostics.cpp` | New file. `toString` overloads, `formatDiagnostic`, `editDistance`, `suggestSimilar`, the static `getKnownStyleProperties` table. |
| `include/StyleParser.hpp` | `applyStyleDeclaration` gains `DiagnosticSink*` and `bool strict`. `StyleSheet::Declaration` (and `Rule`, `KfRule`, `KfFrame`) gain a `SourceSpan loc`. `loadStyleString` / `loadStyleFile` gain `sink` and `strict`. |
| `include/StyleAttr.hpp` | (No signature change; the diagnostic path calls `applyStyleAttr` and uses its return value.) |
| `tests/test_diagnostics.cpp` | New file. 6 tests. |
| `docs/ROADMAP.md` | §5.20 closed. |

No source files other than the two new ones and `StyleParser.hpp`.

### 4.6 Tests

`tests/test_diagnostics.cpp`:

- `valid_style_no_diagnostics` — a clean stylesheet produces no
  diagnostics.
- `unknown_property_emits_warning` — `colr: #fff` produces a
  `Warning` with `UnknownProperty` and a hint containing "color".
- `unknown_property_strict_emits_error` — the same stylesheet with
  `strict = true` produces an `Error` and `hasErrors()` is true.
- `unknown_pseudo_class` — `Button:hovver` produces `UnknownPseudo`.
- `duplicate_keyframes` — two `@keyframes pulse` produce
  `DuplicateRule`.
- `edit_distance_basics` — `editDistance` sanity checks (equal,
  insert, delete, totally different).

All 6 pass on the current implementation.

### 4.7 Documentation

- `docs/user/05-zstyle.md` §11 "Diagnostics and Strict Mode" — how to
  use a `DiagnosticSink`, what `strict` mode does, the format of the
  hint.
- `docs/api/Style.md` — no change (the diagnostic API is documented
  in the new `Diagnostics.hpp`; a `docs/api/Diagnostics.md` would be
  redundant for a header-only utility).
- `docs/ROADMAP.md` §5.20 — mark shipped.

### 4.8 What does NOT change

- The `.zstyle` grammar: unchanged. Diagnostics only *report* on
  existing syntax.
- The default behavior: still tolerant. Bad declarations are still
  ignored by default (`strict = false`).
- The `Logger` path: preserved as the fallback when no sink is
  provided.
- `applyStyleAttr`: unchanged. It still returns `bool` (whether it
  recognized the property), and the caller (the diagnostic path) uses
  that return value.

### 4.9 Open questions (resolved at ship time)

1. **Should the sink be required?** No — a required sink would break
   every existing call. Default to `nullptr` and fall back to
   `Logger`.
2. **Should `strict` default to `true` in debug builds?** Tempting,
   but it would change behavior based on build flags, which is
   surprising. Keep `strict = false` by default; CI and debug
   environments can opt in.
3. **Should diagnostics include a related span?** The struct has a
   `related` field for future use (e.g. "this rule shadows the
   previous one at line X"), but nothing populates it yet. Reserved.

---

## Proposal 5 — Wrap for HStack

**Priority:** medium — HStacks with many children overflow today; a
responsive toolbar or a tag cloud is awkward to build without wrap.

**Complexity:** medium (~1 day).

**Depends on:** nothing.

**Status:** ✅ shipped as roadmap §5.21.

### 5.1 Problem

An `HStack` lays out its children in a single horizontal line. If the
sum of the children's widths exceeds the container's inner width, the
children overflow: they are placed beyond the right edge and (if the
container has `overflow: visible`) draw outside their bounds. There is
no way to make them flow onto the next line.

This is the single biggest limitation of the current layout engine
for UI patterns like:

- Toolbars with an unknown number of buttons.
- Tag clouds.
- Image grids.
- Any responsive row of items.

CSS solves this with `flex-wrap: wrap`. We need the equivalent.

### 5.2 Solution

Add a `wrap` boolean property to `Style` / `ComputedStyle`, parsed
from `.zstyle` (`wrap: true`, `wrap: 1`, `wrap: wrap`) and settable
in C++ (`s.wrap = true`).

When `type == Horizontal` and `wrap == true`, `arrange` uses a
multi-line algorithm:

- Iterate the children, accumulating widths.
- Before placing a child, if `currentLineWidth + childWidth > innerW`
  and the line is non-empty, start a new line.
- Track the height of each line (max of its children's heights).
- After line breaking, distribute `justify` **per line** (each line
  is centered / spaced independently, not the whole row).
- `align-v` (`items-v`) is resolved against the line's height, not
  the container's height.

`computeIntrinsicSize` is updated to compute the total height as the
sum of line heights + gaps, and the width as the max line width.

`Vertical` and `Stack` layouts ignore the `wrap` flag (it has no
meaning for them).

### 5.3 API

`.zstyle`:

~~~css
.toolbar { wrap: true; }
.toolbar Button { /* each button is a child */ }
~~~

C++:

~~~cpp
s.wrap = true;
~~~

No new methods, no new types. Only the `Opt<bool> wrap` field in
`Style` and `bool wrap` in `ComputedStyle`, both already generated by
`BUBBLE_STYLE_PROPS`.

### 5.4 Semantics

**Line breaking trigger.** A new line is started when the current
line already has at least one child and the next child's outer width
(child width + margins) would exceed `innerW`. A child alone on a
line is always placed, even if it exceeds `innerW` (matching CSS:
overflow rather than a zero-width column).

**Gap.** `gap` (X axis) is inserted **between children on the same
line**, not between lines. `gap` (Y axis) is inserted **between
lines**, not between rows within a line. This matches CSS
`row-gap` / `column-gap` semantics with a single `gap` property.

**Justify per line.** `justify: center`, `end`, `space-between` are
applied per line independently. A line with a single child and
`justify: space-between` behaves like `justify: start` (no extra
space to distribute). This matches CSS.

**Cross-axis align per line.** `items-v` (`start`, `center`, `end`,
`stretch`) is resolved against the **line height**, not the
container height. A child that is `items-v: center` is centered
vertically within its own line.

**Wrap does not affect Vertical or Stack.** On a `Vertical` or
`Stack` container, `wrap: true` is silently ignored. The property is
documented as "HStack only".

**Interaction with `grow` / `shrink`.** Both are applied per line, as
in the non-wrap case, after line breaking. A line whose children are
all `grow: 0` keeps its natural width; a line with a `grow: 1` child
expands that child to fill the line.

### 5.5 File-by-file changes

| File | Change |
|------|--------|
| `include/Style.hpp` | `X(bool, wrap, false)` added to `BUBBLE_STYLE_PROPS`. `isInheritedProp` unchanged (wrap is not inherited). |
| `include/StyleAttr.hpp` | `else if (key == "wrap") { st.wrap = (v == "true" || v == "1" || v == "wrap"); return true; }` |
| `include/StyleParser.hpp` | No change — `applyStyleAttr` handles the key. |
| `src/ui/layout/Measure.cpp` | New `arrangeWrapFlow` (was `arrangeHorizontalFlow` with a wrap branch added; kept as a separate function for clarity). `arrangeInto` dispatches to it when `type == Horizontal && style.wrap`. `computeIntrinsicSize` updated to compute multi-line height. |
| `tests/test_layout.cpp` | 6 tests added (see §5.6). |
| `docs/ROADMAP.md` | §5.21 closed. |

No source files other than `Measure.cpp`. `Style.hpp` and
`StyleAttr.hpp` get one line each.

### 5.6 Tests

`tests/test_layout.cpp`, group `Layout_wrap`:

- `no_wrap_overflows` — without `wrap`, three 100-wide children in
  a 250-wide container are all at the same `y` and overflow.
- `wrap_moves_to_next_line` — with `wrap: true`, the third child is
  on a new line.
- `wrap_measures_correct_height` — the container's measured height
  is the sum of line heights + gap.
- `wrap_multiple_lines_with_gap` — four children in a 250-wide
  container produce two lines, with `gap` between children on each
  line and between lines.
- `wrap_justify_center` — a line with `justify: center` centers its
  children within the line.
- `wrap_cross_axis_alignment` — a child with `items-v: center` is
  centered within its line's height.

All 6 pass on the current implementation.

### 5.7 Documentation

- `docs/user/02-layout.md` §HStack: add a "Wrap" subsection with an
  example, the `gap` semantics, and the "HStack only" caveat.
- `docs/CHEATSHEET.md` §3 Layout types: add a `wrap` row.
- `docs/api/Style.md` §2 (property list): already lists `wrap`
  because it's in `BUBBLE_STYLE_PROPS`; no change.
- `docs/ROADMAP.md` §5.21: mark shipped.

### 5.8 What does NOT change

- Vertical and Stack layout: unchanged. `wrap` is ignored.
- Non-wrap Horizontal layout: unchanged. `arrangeHorizontalFlow` is
  untouched.
- `computeIntrinsicSize` for non-wrap Horizontal: unchanged.
- Alignment semantics for non-wrap: unchanged.
- ZMarkup: no new tag or attribute. `wrap` is an inline style
  attribute that already existed in the parser's `applyStyleAttr`
  (it was a no-op, now it's wired to the layout).

### 5.9 Open questions (resolved at ship time)

1. **Should `wrap` also apply to `Vertical`?** No — vertical already
   has multiple lines by construction (the Y axis is the line axis).
   `wrap` would be meaningless.
2. **Should `wrap` accept a direction or alignment?** No, matching
   CSS `flex-wrap: wrap` (not `wrap-reverse`). A future
   `flex-wrap: wrap-reverse` would be a separate property.
3. **Should `justify` distribute space across lines or within
   lines?** Within lines. This matches CSS and is what users expect
   for toolbars.

## Proposal 6 — VirtualList

**Priority:** high — unblocks large lists (10k+ items) and is the most
requested feature after UIPrefab.

**Complexity:** medium-high (~1-2 weeks for the fixed-height version,
+1 week for variable-height rows).

**Depends on:** §5.18 (UIPrefab), §5.19 (Slot), `ScrollController`.

**Status:** 🟡 proposed.

**Roadmap mapping:** ships as §5.23.

### 6.1 Problem

Today, rendering a list of N items means materializing N `Layout`
nodes. At N = 1000 this is already measurable (`large1000` allocates
1000 nodes, measures them, and keeps them alive); at N = 10,000 it's
unusable: memory grows linearly, `measure`/`arrange` walk the whole
tree every frame, and `draw` culls but still iterates.

The standard solution is **virtualization**: materialize only the rows
currently visible in the scroll viewport, plus a small buffer, and
recycle the materialized nodes when they scroll out and new ones
scroll in. This is what Qt's `QListView`, React's `react-window`, and
every mobile list widget do.

### 6.2 Solution

Introduce a `VirtualList` class that:

- holds a **data model** (item count + a per-item binding callback),
- holds a **row template** (a `UIPrefab` name),
- materializes only the rows intersecting the scroll viewport,
- recycles materialized rows when they scroll out,
- re-binds recycled rows to a new index (the "recycled row" pattern).

It is a sibling of `ScrollView`, not a subclass: `ScrollView` is a
generic container, `VirtualList` is a specialized one with a mandatory
row model. Sharing code would force `ScrollView` to grow hooks it
doesn't need.

### 6.3 API

~~~cpp
namespace ZenitUI::UI
{

class VirtualList : public TLayout<VirtualList, ScrollView>
{
public:
    static std::shared_ptr<VirtualList> create(
        std::string prefabName,
        int         itemCount);

    void  setItemCount(int count);
    int   getItemCount() const;

    void  setRowHeight(float px);
    float getRowHeight() const;

    // Called when a row is (re)bound to an index. `row` is the root
    // of the prefab instance. Look up nodes inside it with
    // `find` / `ref` / `refAll` on that root.
    std::function<void(int index, Layout* row)> onBindRow;

    // Called once per row when the row is first created. Use for
    // one-time setup (e.g. attaching a click handler whose closure
    // captures a weak_ptr to the row).
    std::function<void(Layout* row)> onRowCreated;

    void scrollToRow(int index);
    int  getFirstVisibleRow() const;
    int  getLastVisibleRow() const;
};

} // namespace ZenitUI::UI
~~~

`.zstyle` / ZMarkup:

~~~zmarkup
VirtualList#log-view prefab="LogRow" items="10000" row-height="24"
~~~

`prefab`, `items`, and `row-height` are consumed by
`VirtualList::onBuild`; other attributes are applied to the internal
scroll container as usual.

### 6.4 Semantics

**Row recycling.** The list maintains a pool of `Layout` instances
materialized from the prefab. When the viewport scrolls, a row that
scrolled out is detached from its index and re-bound to a new one.
The number of `Layout` instances is bounded by
`ceil(viewportHeight / rowHeight) + 2 * buffer`. The buffer defaults
to 2 rows on each side.

**Binding order.** When a row is recycled, `onBindRow(newIndex, row)`
fires **after** the row's children have been rearranged but **before**
the next `measure`/`arrange` pass. This lets the binding callback
mutate the row (set text, change classes) without causing a second
layout pass.

**Row identity.** A row is identified by its index in the model, not
by a stable id. Supporting stable ids would require the user to
provide an equality function and the list to maintain an id→index
map. If stable ids are needed, encode the id in the row itself via
`ref` in the binding callback.

**Empty state.** When `itemCount == 0`, the list renders nothing
beyond the scrollbar. A future extension could add an
`onEmpty`/`emptyPrefab` hook; not in this proposal.

**Scrollbar.** The scrollbar reflects the full model size
(`itemCount * rowHeight`), not the visible rows. The thumb is
proportional to the viewport / content ratio.

**Interaction.** Rows are normal `Layout` instances. Clicks, hover,
focus, and keyboard navigation work as in a normal `ScrollView`. A
hovered row that is recycled under the pointer triggers
`onHoverExit` on the old index and `onHoverEnter` on the new index.

**Vertical only, LTR only.** Horizontal lists and RTL are §5.23b.

### 6.5 File-by-file changes

| File | Change |
|------|--------|
| `include/components/VirtualList.hpp` | New file. Class declaration + implementation (header-only, like the other widgets). |
| `src/ui/UI.hpp` | `#include "components/VirtualList.hpp"` |
| `src/ui/ZMarkup.hpp` | Register `VirtualList` in the default registry; parse `prefab`, `items`, `row-height`. |
| `tests/test_virtual_list.cpp` | New file. ~15 tests. |
| `examples/14_virtual_list/main.cpp` | New example. 10,000 rows, ~30 materialized. |
| `docs/user/03-widgets.md` | New §VirtualList section. |
| `docs/api/Widgets.md` | New `VirtualList` entry. |
| `docs/ROADMAP.md` | §5.23 checklist and closure. |

Estimated: ~400 lines of new code, ~300 lines of test, ~150 lines of
docs, ~80 lines of example.

### 6.6 Tests

Model semantics:
- `setItemCount` updates the model.
- `setRowHeight` updates the scroll content size.
- `getItemCount` returns what was set.
- Zero-item list renders no rows.
- One-item list renders exactly one row.

Recycling behavior:
- After scrolling by `N * rowHeight`, `getFirstVisibleRow` returns `N`.
- After scrolling by `N * rowHeight`, `getLastVisibleRow` returns
  `N + visibleCount - 1`.
- A row that scrolls out and back in is rebound (`onBindRow` fires
  again with the same index).
- `onRowCreated` fires exactly `ceil(viewportH / rowH) + 2 * buffer`
  times, no more.
- After scrolling past all items, `getFirstVisibleRow` is
  `max(0, itemCount - visibleCount)`.
- Hover during recycling: a hovered row that is recycled under the
  pointer triggers `onHoverExit` on the old index and `onHoverEnter`
  on the new index.

Integration:
- A `VirtualList` inside a `ScrollView` scrolls correctly.
- Clicking a row fires its `onClick` with the closure captured in
  `onBindRow` for the current index.

Estimated: ~15 tests.

### 6.7 Documentation

- `docs/user/03-widgets.md` §VirtualList: motivation, API, the
  fixed-height limitation, the recycling contract for `onBindRow`,
  the "closures capture `index` by value" pattern.
- `docs/api/Widgets.md` `VirtualList`: full method reference.
- `docs/CHEATSHEET.md`: a one-liner and a five-line example.
- `examples/14_virtual_list`: 10k rows, shows only the visible ones.

### 6.8 What does NOT change

- `ScrollView` behavior: unchanged. `VirtualList` uses a private
  `ScrollView` instance internally, not a subclass.
- `UIPrefab` behavior: unchanged.
- `Layout` behavior: unchanged.
- `Theme` behavior: unchanged.
- ZMarkup grammar: one new reserved tag (`VirtualList`), three new
  attributes (`prefab`, `items`, `row-height`). All existing markup
  is unaffected.

### 6.9 Open questions

1. **Fixed vs variable row height.** Fixed height is much simpler
   and covers ~80% of use cases (logs, tables, chat messages).
   Variable height requires per-row `measure` and a two-pass layout;
   it's a substantial API surface. Decision: **fixed height only**;
   variable height is §5.23a, deferred.

2. **Recycling policy.** FIFO vs LRU. FIFO is simpler and gives a
   stable memory pattern; LRU keeps recently-visible rows hot, but
   the pool is small enough that it doesn't matter. Decision:
   **FIFO**.

3. **Selection state.** Selection is a property of the model, not of
   the row. The proposal doesn't ship a selection API; the user
   implements it in `onBindRow`. If a selection API is ever needed,
   it's §5.23c.

4. **Scroll momentum during row reuse.** The `ScrollController`'s
   inertia is based on velocity, not on content identity. Recycling
   rows mid-fling is safe. No special handling required.

5. **Row height changes.** `setRowHeight` after the first frame
   forces a full rebuild of the row pool and a `measure`
   invalidation. Allowed but expensive; documented as "call before
   the first `runFrame` in practice".

6. **Nested lists.** A `VirtualList` inside a `VirtualList` row is
   technically possible but not tested. The outer recycling destroys
   the inner list when the outer row scrolls out; the inner list's
   scroll position is lost. Documented as a limitation.

7. **Prefab mutation.** `onBindRow` can freely mutate the row (set
   text, change classes). It must not add or remove children from
   the row — the row's child structure is owned by the prefab and
   is assumed stable across recycles. Documented.

## Proposal 7 — Drag & Drop

**Priority:** medium — unblocks reorderable lists, file pickers, and
palette-based UIs.

**Complexity:** medium-high (~2-3 weeks, split into 3 PRs).

**Depends on:** nothing strictly; benefits from §5.23 (VirtualList).

**Status:** 🟡 proposed.

**Roadmap mapping:** ships as §5.22 (a/b/c).

### 7.1 Problem

Today, reordering items or dragging a node onto another requires
per-widget ad-hoc code: manual pointer capture, manual hit-test,
manual visual feedback, manual drop detection. There's no shared
abstraction, so every widget that needs drag reimplements it with
subtly different behavior (does the drag ghost clip? does the drop
target get hover state? what happens on Esc?).

The standard solution is a framework-level drag & drop protocol with
three roles: **draggable source**, **drop target**, **drag ghost**.

### 7.2 Solution

Three cooperating pieces:

1. **Drag state in `UIContext`.** A single optional
   `ActiveDrag { source, payload, ghost, offset }` set when a drag
   starts and cleared when it ends. The payload is an opaque
   `std::any` so the framework doesn't need to know what's being
   dragged.

2. **`draggable` and `droppable` flags on `Layout`.** Set via C++
   (`setDraggable(true)`, `setDroppable(true)`) or `.zstyle`
   (`draggable: true`). A `draggable` node initiates a drag on press
   + move beyond a threshold. A `droppable` node accepts a drop if
   the payload satisfies a user-provided predicate.

3. **Drag ghost.** When a drag starts, a user-provided ghost factory
   materializes a `Layout` that follows the pointer. The ghost is
   added to a dedicated overlay layer (a portal-like mechanism) so
   it isn't clipped by the source's ancestors.

The protocol:

- `onPress` on a `draggable` node starts a **pending drag**.
- If the pointer moves more than `dragThreshold` pixels while
  pressed, the pending drag becomes an **active drag**.
- While active: `UIContext::activeDrag` is set, `onDragStart` fires
  on the source, the ghost is created and follows the pointer.
- Each frame, `hitTest` on `droppable` nodes (excluding the source
  and the ghost) determines `dragHoverTarget`.
  `onDragEnter` / `onDragLeave` fire on hover transitions.
- On release, `onDrop` fires on `dragHoverTarget` (if any),
  `onDragEnd` fires on the source, drag state is cleared.
- On Esc during a drag, `onDragCancel` fires and the drag is
  cleared without a drop.

### 7.3 API

~~~cpp
namespace ZenitUI
{

struct DragPayload
{
    std::any    data;
    std::string typeTag;   // e.g. "list-item", "file", "color"
};

struct ActiveDrag
{
    std::weak_ptr<Layout>   source;
    std::weak_ptr<Layout>   ghost;
    DragPayload             payload;
    Vec2                    startPos;
    Vec2                    currentPos;
    Layout*                 hoverTarget{nullptr};
    bool                    active{false};
};

}
~~~

In `Layout`:

~~~cpp
void setDraggable(bool v);
void setDroppable(bool v);
bool isDraggable() const;
bool isDroppable() const;

std::function<void(DragPayload&)>        onDragStart;
std::function<void()>                    onDragEnd;
std::function<void()>                    onDragCancel;

std::function<bool(const DragPayload&)>  canAcceptDrop;
std::function<void(const DragPayload&)>  onDrop;
std::function<void()>                    onDragEnter;
std::function<void()>                    onDragLeave;
~~~

In `UIContext`:

~~~cpp
ActiveDrag activeDrag;
float      dragThreshold{5.0f};
std::function<std::shared_ptr<Layout>(const DragPayload&)> dragGhostFactory;
~~~

`.zstyle` / ZMarkup:

~~~css
.list-item { draggable: true; }
.drop-zone { droppable: true; }
~~~

`onDragStart`, `onDrop`, etc. are C++-only. The DSL expresses flags
but not callbacks.

### 7.4 Semantics

**Threshold.** A drag doesn't start until the pointer has moved
`dragThreshold` pixels from the press position. This lets clicks on
a draggable element fire normally. Matching CSS / OS behavior.

**Hit-testing during drag.** `hitTest` is called with the current
pointer position, but the source and ghost are excluded from the
walk. Only `isDroppable()` nodes are considered; non-droppable
nodes are skipped entirely (not just "not accepted"). A non-droppable
parent of a droppable child does not block the child.

**`canAcceptDrop` predicate.** If provided, called once per hover
target change with the payload. Returning false excludes the node;
the walk continues to its parent. If not provided, every droppable
accepts every payload.

**Visual feedback.** The framework does not apply any style change to
hover targets automatically. The user reacts via
`onDragEnter`/`onDragLeave` (e.g. add a `.drop-hover` class).

**Ghost layer.** The ghost is added to a per-frame overlay layer
drawn after the main tree, on top of everything. Not clipped by any
ancestor. Follows the pointer with the offset captured at drag start.

**Drop.** On release, if `dragHoverTarget` is non-null and
`canAcceptDrop` returns true (or isn't set), `onDrop(payload)` fires.
Then `onDragEnd()` fires on the source. Then `activeDrag` is cleared.

**Cancel.** On Esc or on window-leave, `onDragCancel()` fires on the
source, the ghost is destroyed, `activeDrag` is cleared, and
`onDrop` does not fire.

**Source and ghost lifetime.** Both are `weak_ptr` in `ActiveDrag`.
Destroying either mid-drag auto-cancels on the next frame.

**Reentrancy.** Starting a drag from within `onDrop` of another drag
is undefined. Documented as "don't".

**Payload lifetime.** `DragPayload::data` is copied once at drag
start. The user is responsible for making the data copyable (or use
`std::shared_ptr<T>` inside the `std::any`). Documented.

### 7.5 File-by-file changes

Split into three PRs, matching the a/b/c split in the roadmap.

**PR a — input capture and drag state** (~150 lines):

| File | Change |
|------|--------|
| `include/UIContext.hpp` | `ActiveDrag`, `dragThreshold`, `dragGhostFactory`. |
| `include/Layout.hpp` | `setDraggable` / `setDroppable` / flags in `NodeConfig`. |
| `src/ui/input/InputController.cpp` | Threshold detection, state machine. |
| `tests/test_drag.cpp` | ~8 tests: threshold, Esc cancel, source-death cancel, no-drag-on-click. |
| `docs/ROADMAP.md` | §5.22a checklist. |

**PR b — droppable protocol and hit-test** (~200 lines):

| File | Change |
|------|--------|
| `include/Layout.hpp` | Drag callbacks. |
| `src/ui/layout/Layout.cpp` | Drag-aware `hitTest` (skip source and ghost). `updateTree` updates `dragHoverTarget` after regular hover. |
| `tests/test_drag.cpp` | ~10 tests: drop, enter/leave, `canAcceptDrop` filter, nested droppables. |
| `docs/ROADMAP.md` | §5.22b checklist. |

**PR c — visual feedback and ghost** (~150 lines):

| File | Change |
|------|--------|
| `include/Layout.hpp` | Ghost layer registration. |
| `src/ui/layout/Layout.cpp` | Draw ghost after the main tree. |
| `include/UIContext.hpp` | Per-frame overlay list. |
| `tests/test_drag.cpp` | ~5 tests: ghost created / destroyed / not clipped. |
| `examples/15_drag_drop/main.cpp` | New example. Reorderable 10-card list. |
| `docs/user/06-events.md` | New §Drag and Drop. |
| `docs/api/Layout.md` | Document drag callbacks. |
| `docs/CHEATSHEET.md` | Five-line drag example. |

### 7.6 Tests

PR a:
- No drag start on a simple click.
- Drag start on press + move > threshold.
- Esc cancels a drag.
- Source destruction cancels a drag.
- `onDragCancel` fires before `activeDrag` is cleared.
- Drag doesn't start if the source is disabled mid-press.
- Release without move doesn't start a drag.
- Press, move, back to origin, release: `onDragEnd` fires
  (drag started, no drop).

PR b:
- `onDrop` fires on the droppable under the pointer.
- `onDrop` doesn't fire on a non-droppable parent.
- `onDragEnter` / `onDragLeave` fire on hover transitions.
- `canAcceptDrop` returning false excludes the node.
- Nested droppables: innermost wins.
- Droppable above the source's ancestor doesn't block.

PR c:
- Ghost is created at drag start.
- Ghost follows the pointer.
- Ghost is destroyed on drop and on cancel.
- Ghost isn't clipped by the source's `overflow: hidden` ancestor.

Total: ~23 tests.

### 7.7 Documentation

- `docs/user/06-events.md` §Drag and Drop: motivation, three roles,
  protocol, "use `shared_ptr` for payload" pattern, "don't start a
  drag from `onDrop`" warning.
- `docs/api/Layout.md`: `setDraggable`, `setDroppable`, callbacks.
- `docs/api/UIContext.md`: `activeDrag`, `dragThreshold`,
  `dragGhostFactory`.
- `docs/CHEATSHEET.md`: reorderable list in ~10 lines.
- `examples/15_drag_drop`: reorderable 10-card list with ghost.

### 7.8 What does NOT change

- Click behavior: unchanged. Threshold preserves clicks.
- Hover behavior: unchanged for non-draggable nodes.
- Focus behavior: unchanged. A drag doesn't steal focus.
- Keyboard behavior: unchanged. Esc is intercepted only during a
  drag.
- `Layout` public API: additive only.
- `UIContext` public API: additive only.
- ZMarkup grammar: two new attributes (`draggable`, `droppable`).

### 7.9 Open questions

1. **Payload type — `std::any` vs template.** `std::any` is simple but
   requires copyable payloads. A `DragPayload<T>` template would
   force `Layout` to be templated on the payload type — absurd.
   Decision: **`std::any`** with a documented "use `shared_ptr<T>`
   inside the any if T is expensive to copy".

2. **Ghost factory — global or per-source?** Global is simple but
   uniform; per-source gives per-widget control but adds a field.
   Decision: **per-source first, global fallback**. `Layout::onGhostFactory`
   is the per-source hook; `UIContext::dragGhostFactory` is the fallback.

3. **Drop on Esc.** `onDrop` does not fire on Esc. Matches OS
   behavior.

4. **Multi-pointer drag.** Two simultaneous drags (left + right
   mouse) are not supported. `activeDrag` is a single struct.
   Documented.

5. **Cross-process drag.** Dragging from another application into
   the window is not supported. Deferred to post-1.0 (would require
   `IPlatform` extensions).

6. **Auto-scroll during drag.** Dragging near the edge of a
   `ScrollView` should auto-scroll. Not in this proposal; the user
   can implement it in `onDragEnter`/`onUpdate`. Deferred to §5.22d.

7. **Drop between two lists.** The `typeTag` field is designed to
   support this (a droppable can check the tag), but the proposal
   doesn't ship a built-in mechanism for moving items between lists.
   The user implements it in `onDrop`.

8. **Visual feedback during drag.** No automatic style change on
   hover targets. Deliberate. A built-in `.drop-hover` class could be
   added as a follow-up.

## Proposal 8 — VirtualView

**Priority:** high — unblocks large lists without requiring UIPrefab
or Slot, so it ships before Proposal 6 (VirtualList).

**Complexity:** medium (~2-3 days).

**Depends on:** `ScrollController` (shipped).

**Status:** ✅ shipped as roadmap §5.24.

**Roadmap mapping:** ships as §5.24.

### 8.1 Problem

Rendering a list of N items with a plain `Vertical` / `VStack` means
materializing N `Layout` nodes, measuring all of them, and arranging
all of them. At N = 10,000 this is unusable: memory grows linearly,
`measure`/`arrange` walk the whole tree every frame, and `draw`
culls but still iterates.

The standard solution is **virtualization**: materialize only the
rows currently visible in the scroll viewport, plus a small buffer.
Unlike `VirtualList` (Proposal 6), VirtualView does **not** recycle
nodes: each row entering the viewport is built by a user-supplied
factory, each row leaving is destroyed. That makes it usable without
UIPrefab and Slot, at the cost of losing node reuse.

### 8.2 Solution

Introduce a `VirtualView` class that:

- extends `ScrollView` (vertical), inheriting wheel, inertia,
  scrollbar, clipping and `scrollToY` unchanged;
- holds a **factory** that builds a row for a given index;
- holds an **item count**;
- optionally holds a **uniform row height** for the O(N) fast path;
- computes **cumulative heights** once per width / count change and
  reuses them for binary-search visibility queries;
- materializes only the rows in `[first - 2, last + 2]`, where
  `first` / `last` come from `getFirstVisibleItem` /
  `getLastVisibleItem`;
- destroys the rows that fall outside that window.

Because VirtualView is a `ScrollView` subclass, "where the rows are"
and "how the scrollbar behaves" are inherited. VirtualView only owns
the materialization window and the row positioning.

### 8.3 API

~~~cpp
namespace ZenitUI::UI
{

class VirtualView : public TLayout<VirtualView, ScrollView>
{
public:
    using ItemFactory =
        std::function<std::shared_ptr<Layout>(int index)>;

    static std::shared_ptr<VirtualView> create(
        ItemFactory factory, int count);

    static std::shared_ptr<VirtualView> create(
        std::function<std::shared_ptr<Layout>()> factory,
        int count);

    int   getItemCount() const noexcept;
    void  setItemCount(int count);

    float getUniformHeight() const noexcept;
    void  setUniformHeight(float px);

    int   getFirstVisibleItem() const;
    int   getLastVisibleItem()  const;
    void  scrollToItem(int index);

    void  removeAllChildren() override;

    std::function<void(Layout*, int)> onItemCreated;
};

}
~~~

No `.zstyle` / ZMarkup attributes: the factory is a C++ callable and
cannot be expressed in markup. A future follow-up could register a
tag that takes a prefab name, but that requires UIPrefab and Slot
(Proposal 3) and is out of scope for this proposal.

### 8.4 Semantics

**Two measurement modes.** When `uniformHeight > 0`, the cumulative
heights are `i * uniformHeight` for `i` in `[0, itemCount]` and the
factory is **not** called during measurement. When `uniformHeight == 0`
(the default), the factory is called once per index and `measure` is
invoked on each returned node to build the cumulative heights. The
auto path is O(N) factory calls on the frame the cumulative heights
are (re)built; the uniform path is O(N) arithmetic.

**Materialization window.** `first` and `last` are computed from the
current scroll offset and viewport height. The window is
`[max(0, first - 2), min(itemCount - 1, last + 2)]`. Rows inside the
window are materialized (if not already), rows outside are detached
and erased from the internal vector. The 2-row buffer above and below
avoids churn when scrolling by small amounts.

**Row positioning.** Materialized rows are positioned absolutely at
`left = 0`, `width = 100%`, `top = cumulative[i]`,
`height = cumulative[i+1] - cumulative[i]`, in **content
coordinates**. The inherited `ScrollView` translation and clip then
move them with the scroll — VirtualView does not reposition rows on
every frame.

**Visibility queries.** `getFirstVisibleItem()` is
`itemAtY(getScrollY())`, `getLastVisibleItem()` is
`itemAtY(getScrollY() + rect.height - eps)`, where `itemAtY` is a
binary search over the cumulative heights. Both return `-1` before
the first measure and for empty lists.

**Scrolling.** `scrollToItem(i)` clamps `i` to `[0, itemCount - 1]`
and calls `scrollToY(cumulative[i])`. The rest of the `ScrollView`
API is inherited unchanged.

**Callbacks.** `onItemCreated(Layout*, int)` fires every time a node
is materialized, including re-materialization after a row scrolled
out of the window and back in. It is not "once per index, ever".

**Lifecycle.** `removeAllChildren()` clears the materialized set and
the base children; the visible rows come back on the next `arrange`.
Unlike `setItemCount(0)`, it does not reset `itemCount`.

**Caching.** `setItemCount` and `setUniformHeight` both clear
`cumulativeHeights` and `lastMeasuredWidth` and set `subtreeDirty` +
`pendingTransition`. With auto height, the cumulative heights are
also rebuilt when the available width changes (`lastMeasuredWidth`
cache).

### 8.5 File-by-file changes

| File | Change |
|------|--------|
| `include/components/VirtualView.hpp` | New file. Class declaration + implementation (header-only, like the other widgets). |
| `src/ui/UI.hpp` | `#include "components/VirtualView.hpp"` |
| `tests/test_virtual_view.cpp` | New file. 14 tests. |
| `docs/user/12-virtual_view.md` | New user-facing page. |
| `docs/api/Widgets.md` | New `VirtualView` entry. |
| `docs/user/03-widgets.md` | New `VirtualView` section. |
| `docs/CHEATSHEET.md`, `docs/GLOSSARY.md`, `docs/README.md` | Cross-references. |
| `docs/ROADMAP.md` | §5.24 checklist and closure. |

Estimated: ~350 lines of new code, ~200 lines of test, ~250 lines of
docs.

### 8.6 Tests

`tests/test_virtual_view.cpp`:

- `item_count_get_set` — `setItemCount` clamps to `>= 0`.
- `uniform_height_get_set` — `setUniformHeight` clamps to `>= 0`.
- `first_visible_uniform_at_top` — 100 items, viewport 240, height 24
  → first = 0, last = 9.
- `first_visible_uniform_after_scroll` — after `scrollToY(240)` →
  first = 10, last = 19.
- `zero_items` — `getFirstVisibleItem` / `getLastVisibleItem` return
  `-1`.
- `content_size_uniform` — `getMaxScrollY` = `100 * 24 - 240`.
- `scroll_to_item_uniform` — `scrollToItem(500)` makes 500 visible.
- `auto_height_measure_variable` — alternating 24 / 48 → total 3600.
- `auto_first_visible_at_top` — cumulative = `[0, 24, 72, 96, 144,
  168, 216, 240, ...]` → last = 6 at the top.
- `materialized_count_bounded` — 12 children for 10 visible with a
  2-row buffer top and bottom.
- `materialized_dropped_when_out_of_range` — after scrolling by
  `1000 * 24`, the created indices are all in `[998, 1011]`; the
  first visible is in `[998, 1000]`.
- `on_item_created_fires_for_materialized` — 12 calls on the first
  frame, indices 0–9 included.
- `removeAllChildren_clears_materialized` — clears then re-fills on
  the next frame.
- `first_visible_before_measure_is_minus_one` — before the first
  `arrange`, both queries return `-1`.

Total: 14 tests, all passing.

### 8.7 What does NOT change

- `ScrollView` behavior: unchanged. `VirtualView` is a `ScrollView`
  subclass and inherits everything the base class does.
- `Layout` behavior: unchanged.
- `Theme` / `ZStyle`: unchanged.
- `ZMarkup` grammar: unchanged. VirtualView is C++-only because the
  factory is a `std::function`.
- No new style properties.

### 8.8 Open questions

1. **Recycling.** Proposal 6 (`VirtualList`) recycles nodes across
   indices; VirtualView does not. VirtualView is the simpler ship; a
   future VirtualView could adopt recycling without changing the
   public API, but that would effectively merge it with VirtualList.
   Decision: **no recycling in VirtualView**. VirtualList remains the
   recycling variant.

2. **Variable height with heavy factory.** With auto height, the
   factory runs once per index during the first measure. For
   heavy-per-row factories, users must use `setUniformHeight` or
   cache. Documented in the user page §5 and §7.

3. **Infinite scroll.** No built-in hook. Users react to
   `getLastVisibleItem()` and call `setItemCount` with a larger value.
   Documented in the user page §8.3.

4. **ZMarkup tag.** VirtualView cannot be expressed in markup today
   because the row content is a C++ factory. A future follow-up could
   register a tag that takes a prefab name (requires UIPrefab and
   Slot, both shipped), but it is out of scope here. Tracked as the
   open checkbox at the end of `ROADMAP.md` §5.24.

### 8.9 Documentation

- `docs/user/12-virtual_view.md` — user-facing page: when to use,
  quick start, full API, materialization model, uniform-vs-auto
  comparison, interaction with scroll, pitfalls, examples.
- `docs/api/Widgets.md` — `VirtualView` entry with the full method
  reference.
- `docs/user/03-widgets.md` — `VirtualView` section, cross-linked to
  the user page.
- `docs/CHEATSHEET.md` — one-liner and short example.
- `docs/GLOSSARY.md` — "VirtualView" entry.
- `docs/README.md` — link to the new user page.
- `docs/ROADMAP.md` — §5.24 with the shipped checklist.

---

## Proposal 9 — Filter layer pipeline performance

**Priority:** high — severe FPS drops on animated or scrolling nodes
with `filter:`, and the cost grows quickly with the number of filtered
nodes.

**Complexity:** medium. Steps 0–c are ~2–3 days. Step g (layer cache)
is a separate ~1 week task with real regression risk and is only
tackled if the measurements justify it.

**Depends on:** §5.4 (filter chaining) and §5.5 (nested targets), both
shipped. Independent of every other proposal.

**Status:** 🟡 proposed.

**Roadmap mapping:** ships as §4.9 (steps 0, a–g).

### 9.1 Problem

Using `filter:` (blur, drop-shadow, custom filters) makes a scene
disproportionately slow, well beyond the cost of the extra shader
passes themselves. A review of `Layout::drawLayer`
(`src/layout/Render.cpp`), `FilterRegistry.cpp` and
`RaylibRenderer` found several independent causes.

**Evidence — measured.** An instrumented copy of the mock backend
(counters for `createTarget` / `destroyTarget` / `pushTarget`; no GPU
involved) was driven against the real library sources, 60 frames per
scenario, with a 200×200 filtered node. Measured on `master` at
`b2c1046`; the numbers were identical on `212b9ec`, since the filter
pipeline (`drawLayer`, `FilterRegistry`, the Raylib backend) did not
change between the two:

| Scenario | `createTarget` calls | `destroyTarget` calls |
|----------|---------------------:|----------------------:|
| Static node, `blur(4px)` (after warm-up) | 0 | 0 |
| `scale` 1.0 → 1.15, `blur + drop-shadow` | 45 | 45 |
| `rotation` +3°/frame, `blur` | 165 | 165 |
| Node sliding across the viewport edge (clip shrinks the region), `blur` | 177 | 177 |

Each resize recreates **three** targets (`layerTarget_`,
`layerScratch_`, `layerSeparable_`), so the scale case is ~15 resizes
and the rotation / clipping cases are ~55–59 resizes in 60 frames,
i.e. almost every frame.

Two more measured facts:

- A chain with a single `drop-shadow` allocates 3 targets on its first
  frame; it only needs 1.
- 50 static filtered cards (`blur + drop-shadow`) keep **150 render
  targets alive** and issue **150 `pushTarget` calls per frame**
  (3 per card).
- The CPU side of the filter path is **not** the bottleneck: 50 cards
  with `blur + drop-shadow` cost ~0.5–0.7 µs per card more than the same 50
  cards without filters (two runs; mock backend, `-O2`). The per-frame
  `std::stof` parsing in `setParams` is real but negligible. It must
  not be the first thing optimised.

**Evidence — from reading the code, not yet measured on a real GPU.**

1. **Resize churn is expensive in Raylib.** `RaylibRenderer::createTarget`
   calls `LoadRenderTexture`, which creates a framebuffer, a colour
   texture *and* a depth renderbuffer (useless for a 2D UI), and
   `destroyTarget` frees them again.
2. **FBO switches and batch flushes.** Every `pushTarget` /
   `popTarget` calls `rlDrawRenderBatchActive()` (flushing the current
   draw batch for the whole UI), switches framebuffer, and clears the
   whole target. A `blur + drop-shadow` node does 3 pushes per frame.
   `pushTarget` also copies `clipStack` and `transformStack` by value
   into `targetStates_`.
3. **Per-node targets for the whole lifetime of the node.** Targets are
   only needed during `drawLayer`, but each node keeps up to three
   alive. VRAM grows as N × 3 × (width × height × 4 bytes colour, plus
   depth in Raylib).
4. **No result caching.** A filtered subtree that did not change is
   still re-rendered and re-filtered every frame. There is no paint
   dirty flag in the framework (only `subtreeDirty` for layout).
5. **Fill rate of the blur.** The built-in blur runs 9 taps per pass at
   full resolution. Cost scales with pixels; a half-resolution blur is
   4× cheaper and doubles the effective radius.
6. **Minor:** `GetShaderLocation` (a driver call) on every uniform set
   every frame; `IAssetProvider::getEffect` builds a `std::string` per
   filter per frame.

**Limits of this evidence.** Items 1–3 and 5 are inferences from the
code. The first step of this proposal is therefore to measure, not to
optimise.

### 9.2 Solution

Seven independent steps, in this order. Each one is shippable on its
own and each has a test.

**Step 0 — Instrumentation.** A backend-agnostic `RenderStats` struct
on `UIContext`, incremented by the core (`drawLayer`,
`FilterRegistry`), so both the mock and the Raylib backends report the
same numbers: targets created, targets destroyed, live targets,
`pushTarget` calls, filter passes. Exposed through `Debug` (a one-line
dump) and usable from an on-screen overlay in the examples. This is
what lets us confirm items 1–3 on real hardware before spending time
on later steps.

**Step a — Lazy, per-kind target allocation.** `drawLayer` computes
from the filter chain which targets it really needs:

| Target | Needed when |
|--------|-------------|
| main layer target | always |
| scratch (ping-pong write target) | the chain has 2 or more filters |
| separable intermediate | any filter in the chain is `Separable` |

A single `drop-shadow` allocates 1 target (was 3); a single `blur`
allocates 2; `blur + drop-shadow` allocates 3.

**Step b — Bucketed target size.** A target's size is rounded **up**
to a multiple of a bucket (proposed: 64 px) and reused for any region
that fits. The region is drawn into the top-left corner of the target
and read back through a **source rectangle**. This requires a way to
draw a sub-rectangle of a target (see 9.3). Targets are then only
recreated when the region outgrows the bucket, which removes almost
all of the churn in the scale / rotation / clipping scenarios above.
Backends that do not support sub-rectangles keep the current
exact-size behaviour (capability-gated).

**Step c — Shared target pool.** A `TargetPool` owned by `UIContext`.
`drawLayer` *acquires* the targets it needs and *releases* them when
the node is done; targets are no longer members of `Layout`. Nested
layers (§5.5) acquire distinct targets, so the live set is bounded by
(nesting depth × 3) instead of (filtered nodes × 3). Unused targets are
destroyed by `endFrame` after a grace period.

**Step d — Backend hygiene (Raylib).** Colour-only framebuffers (no
depth renderbuffer); avoid the by-value copies in `pushTarget`; cache
uniform locations per shader; avoid the `std::string` allocation in
`getEffect` on the hot path. Low risk, modest gain; done after the
measurements say it is worth it.

**Step e — Optional downscaled separable filters.** `FilterDef` gains
`downscale` (1, 2 or 4; default 1). The intermediate passes run on
targets of `size / downscale` and the final draw upscales with bilinear
filtering. Opt-in because it changes the visual output slightly; the
built-in `blur` keeps `downscale = 1` until a decision is taken at ship
time.

**Step f — Benchmarks and regression tests.** See 9.6. The mock
backend cannot measure GPU time, but it can count targets and passes,
which is what steps a–c change. A manual stress scene in `examples/`
covers real GPU timing.

**Step g — Layer cache (optional, higher risk).** Cache the *final
filtered image* of a node and reuse it while nothing under the node
changed. This is the largest possible win for static UIs and also the
only step that can produce stale rendering if the invalidation misses a
case, so it is designed conservatively:

- **Opt-in** per node (`Layout::setLayerCache(bool)`, default off). The
  framework cannot know whether a custom widget with its own `draw`
  (a `Canvas`, `examples/09_custom_widget`) is animating internally.
- A new `internal_.paintDirty` bit, set when: the node's or a
  descendant's `currentStyle` changes (including hover / pressed /
  focus / checked / disabled transitions), a transition or animation is
  active in the subtree, a child is added or removed, `rect` changes,
  the scroll offset changes, the transform changes, the viewport
  changes, or a widget calls `invalidatePaint()` (`setText`,
  `setValue`, etc.).
- `invalidatePaint()` propagates upward only to the nearest ancestor
  that has the cache enabled.
- A cached node keeps one target (its result) outside the pool.

Step g is decided **after** steps 0–c have been measured. If a–c are
enough, g is dropped and recorded as declined, like §5.9.

### 9.3 API

Backend interface (`include/UIContext.hpp`):

~~~cpp
enum class Feature
{
    Effects,
    NestedTargets,
    TargetSubRect   // new: drawTarget(t, src, dst, tint) is supported
};

class IRenderer
{
    // ...existing...
    virtual void drawTarget(TargetHandle t, Rect dst, Color tint) = 0;

    // new — non-pure, like `fillBorder` (added with the border fix
    // in 65752c7). Default: ignores `src` and forwards to the
    // existing overload. The core only calls it when
    // supports(Feature::TargetSubRect) is true.
    virtual void drawTarget(TargetHandle t, Rect src, Rect dst, Color tint)
    {
        drawTarget(t, dst, tint);
    }
};
~~~

Filter contract (`include/FilterRegistry.hpp`):

~~~cpp
struct FilterContext
{
    // ...existing...
    Rect dst;      // region-sized rect at (0, 0). Also the source rect
                   // inside `src` when the target is larger than the region.
    Vec2 texSize;  // NEW: actual size of the `src` texture. Shaders that
                   // need texel size must use this, not dst.width/height.
};
~~~

`FilterContext::texSize` is the one contract change for custom
filters: with bucketed targets, `dst` can be smaller than the texture.
The built-in filters are updated by the proposal.

Pool (`include/UIContext.hpp` or a new `include/TargetPool.hpp`):

~~~cpp
class TargetPool
{
public:
    // Returns a target at least w × h (bucketed when supported).
    TargetHandle acquire(IRenderer& r, int w, int h);
    void         release(TargetHandle t);

    // Called once per frame by UIContext::endFrame. Destroys targets
    // not used for `graceFrames` frames.
    void trim(IRenderer& r, int graceFrames = 120);
    void clear(IRenderer& r);   // shutdown

    // For RenderStats and tests.
    size_t liveCount() const;
};
~~~

Style / ZStyle: **no change.** `filter:` syntax and order semantics are
untouched. If step g ships, a `layer-cache` style property can be added
later; it is not part of this proposal.

### 9.4 Semantics

- **Pushing a target still clears it** (the whole target, as today). With
  a bucket of 64 px the slack is small; clearing only the region is a
  later micro-optimisation.
- **Blur edge behaviour changes slightly.** Today the shader samples
  past the region edge and the texture wraps / clamps at the target
  boundary. With bucketed targets the padding is transparent, so a blur
  at the edge of the region fades out instead of clamping. This is
  arguably more correct, but it is a visible difference and is called
  out in 9.9.
- **Pool discipline is LIFO.** `acquire` / `release` follow `drawLayer`
  nesting, so a nested filtered descendant never receives its
  ancestor's target.
- **Pool shutdown.** `UIContext` (or the backend's teardown) calls
  `TargetPool::clear` before the renderer is destroyed. `~Layout` no
  longer touches targets; the existing destructor code that destroys
  `layerTarget_` / `layerScratch_` / `layerSeparable_` is removed.
- **`MAX_TARGET_DIM`** (4096) keeps its meaning and applies to the
  requested size, before bucketing.
- **Backends without `TargetSubRect`** still benefit from steps a and c
  (lazy allocation, pooling) but allocate exact-size targets.

### 9.5 File-by-file changes

- `include/UIContext.hpp` — `Feature::TargetSubRect`, the new
  `drawTarget` overload, `RenderStats`, `TargetPool` ownership.
- `include/TargetPool.hpp` (new, header-only) — the pool.
- `include/FilterRegistry.hpp` — `FilterContext::texSize`;
  `FilterDef::downscale` (step e).
- `src/layout/FilterRegistry.cpp` — passes use `texSize` and the source
  rectangle; `downscale` handling (step e).
- `src/layout/Render.cpp` — `drawLayer`: compute needed targets
  (step a), acquire / release from the pool (steps b, c), update
  `RenderStats`.
- `include/Layout.hpp` — remove `layerTarget_` / `layerScratch_` /
  `layerSeparable_` and their destructor cleanup (step c); `paintDirty`
  and `invalidatePaint()` only if step g ships.
- `backends/raylib/RaylibBackend.cpp` / `.hpp` — sub-rect `drawTarget`
  (including the Y flip of render textures), `Feature::TargetSubRect`,
  colour-only framebuffers, cheaper `pushTarget`, cached uniform
  locations (steps b, d).
- `backends/mock/MockBackend.hpp` — `createTargetCount`,
  `destroyTargetCount`, `liveTargets`, sub-rect support flag, last
  `drawTarget` source rect.
- `assets/blur.fs` — read `texSize` as the real texture size.
- `benchmarks/benches.cpp` — filtered-scene benchmarks.
- `examples/` — a stress scene with on-screen `RenderStats`.
- Docs (9.7).

### 9.6 Tests

All on the mock backend unless noted. The scenarios are the ones
measured in 9.1, turned into regression guards:

- Static filtered node: 0 `createTarget` after the first frame.
- Scale sweep, rotation sweep and clip sweep (60 frames each): at most
  a small constant number of `createTarget` calls (to be fixed at
  implementation time from the measured bucket behaviour), instead of
  45 / 165 / 177.
- Lazy allocation: single `drop-shadow` → 1 target; single `blur` → 2;
  `blur + drop-shadow` → 3; `drop-shadow + blur` → 3.
- Pool: 50 static filtered cards → live targets ≤ 3 (was 150), and
  `trim` returns the pool to 0 after the grace period.
- Pool + nesting: a filtered node inside a filtered node uses distinct
  targets and releases them in LIFO order.
- Backend without `TargetSubRect`: falls back to exact-size targets and
  still passes the lazy-allocation and pool tests.
- `FilterContext::texSize` equals the texture size, and `dst` equals the
  region size, when the target is larger than the region.
- `RenderStats` counters match the mock's own counters.
- Raylib only, **manual** (the mock has no framebuffer readback; same
  situation as ROADMAP §1.12): a bucketed target produces the same
  pixels as an exact-size one for the interior of the region, and the
  Y flip of the sub-rectangle is correct. Checked visually in the
  stress scene; automated only if a readback harness is added.
- If step g ships: cached node is not re-rendered while idle; each
  invalidation trigger listed in 9.2 forces exactly one re-render.

Benchmarks (mock; CPU and counters only): `system.frame.filtered50.static`,
`system.frame.filtered50.scaleAnim`, `system.frame.filtered50.rotAnim`.
GPU time is measured manually with the stress example.

### 9.7 Documentation

- `docs/user/09-effects.md` — §1.1 and §7.4 describe **two** targets
  per filtered node, "allocated lazily"; the code allocates all three,
  every time (the chain section of the same page does mention the
  third). Rewrite for the pool and for the actual cost model; add a
  short "Performance" section (what makes filters expensive, what to avoid: filters on
  large animated nodes, many small filtered nodes, deeply nested
  filtered nodes).
- `docs/internals/06-render-pipeline.md` — the `drawLayer` section and
  §13.2 ("Target caching": the claim that a node only reallocates when
  it resizes is not accurate for animated or clipped nodes).
- `docs/api/FilterRegistry.md` — `FilterContext::texSize` contract and
  `downscale`.
- `docs/internals/09-backend.md` — `Feature::TargetSubRect` and the new
  `drawTarget` overload.
- `docs/internals/11-benchmarks.md` — the new benchmarks and what the
  mock can and cannot measure.
- `docs/internals/10-debug.md` — `RenderStats`.

### 9.8 What does NOT change

- The `filter:` syntax, the order semantics of a filter chain, and the
  `effect:` path (§5.4b).
- The `Layout::draw` public API and the CSS-like style surface.
- The behaviour of existing backends that do not implement
  `Feature::TargetSubRect`.
- `MAX_TARGET_DIM`, the 40 px culling margin and the nested-target
  support from §5.5.

### 9.9 Open questions

1. **Bucket size.** 64 px, or powers of two? A power of two wastes more
   VRAM on large nodes; a fixed 64 px means more distinct sizes in the
   pool.
2. **Trim policy.** 120 frames of grace is a guess. It should be tuned
   with the stress scene.
3. **Blur edge behaviour.** Accept the transparent padding (9.4), or
   emulate clamp-to-edge in the shader? Needs a visual check in
   `examples/07_effects`.
4. **Pool owner.** `UIContext` (backend-agnostic, proposed here as
   `TargetPool`) or the renderer itself (an earlier draft called it
   `FboPool` and put it in `IRenderer`)? Keeping it in the core means the
   mock and every backend get the same pooling for free; putting it in
   `IRenderer` lets a backend implement it natively.
5. **`downscale` for the built-in blur.** Keep at 1, or default to 2
   for radii above some threshold?
6. **Layer cache: opt-in only (proposed), or auto-enabled for
   subtrees with no custom `draw` and no running animation?** Auto is a bigger win and a
   bigger risk. Decide only after steps 0–c are measured.
7. **Full-target clear vs scissored clear** on `pushTarget`, once
   bucketing introduces slack.
8. **Is the real-GPU cost dominated by allocation churn (items 1–3) or
   by fill rate and FBO switches (items 2, 5)?** Step 0 answers this
   and may reorder steps d and e.

---

## Out of scope (future rounds)

The following were discussed in the same review but are **not** part
of this proposal. They are listed here for context and to avoid
re-discussing them.

| Feature | Why deferred |
|---------|--------------|
| `GridLayout` | Low priority. `Stack`/`Vertical`/`Horizontal` cover 90% of use cases. Later. |
| Memory management (arena + pool + `UIHandle`/`UIProxy`) | Post-1.0. API-breaking. Performance numbers don't justify it today. |
| `EventBus` (zero-RTTI) | Out of scope for ZenitUI. Belongs to the game engine roadmap. |
| Hot reload | Out of scope for ZenitUI. Belongs to the consumer. See roadmap §5.10. |

---

## Implementation sequence

**Shipped** (roadmap §5.16 – §5.21):

1. Proposal 1 — `ref` → §5.16
2. Proposal 2 — `Bindable<T>` → §5.17
3. UIPrefab → §5.18 (depends on `ref`)
4. `<Slot>` → §5.19 (depends on UIPrefab)
5. Proposal 4 — StyleParser Diagnostics → §5.20
6. Proposal 5 — Wrap for HStack → §5.21


**Proposed** (next, in this order):

7. Proposal 6 — VirtualList → §5.23. Depends on UIPrefab and Slot.
   Benefits: unblocks 10k+ item lists, the most-requested feature
   after UIPrefab.
8. Proposal 7 — Drag & Drop → §5.22 (split into a/b/c). Benefits from
   VirtualList for the "reorderable list" example, but doesn't
   strictly depend on it.
9. Proposal 9 — Filter layer pipeline performance → §4.9. Independent
   of the other proposals, so it can ship at any point. Steps 0 (instrumentation),
   a (lazy targets), b (bucketed sizes) and c (target pool) first; the
   layer cache (step g) only if the measurements justify it.

**Post-1.0:**

- Memory management (Phase 8 in the roadmap).
- `GridLayout`.
- Out-of-scope items as decided by the user.

Post-1.0:

- Memory management (§8 in the roadmap).
- `GridLayout`.
- Out-of-scope items as decided by the user.

---

## Change log — how the implementation differs from the proposal

### Proposal 1 — `ref`

**Shipped:** roadmap §5.16.

**Changes from the proposal:**

- Fluent method is `withRef` (not `ref_`, as originally drafted).
  The proposal itself was updated to `withRef` before implementation;
  no code changes derived from the old name.
- `@ref` in `on*` helpers (`ui.onClick("@delete", cb)`) is **not**
  shipped. It stays as a follow-up in §1.11 (Open questions, point 4).
  The `@` prefix in an id string is currently undefined behavior, so
  adding it later is backwards-compatible.

**What landed:**

- `Layout::ref`, `Layout::setRef`, `TLayout::withRef`.
- `Element::ref` + parser `@name` in ZMarkup.
- `BuildContext::byRef` + population in `Registry::create`.
- `UINode::ref<T>`, `UINode::refAll<T>`, with per-handle warning dedup.
- Warning on `UINode::find<T>` miss.
- `Debug::dumpTree` / `dumpStyle` show `@ref`.
- 18 new tests (14 in `test_zmarkup_build.cpp`, 4 in `test_layout.cpp`).
- 7 files of documentation updated.

**Known limitation carried forward:** `ref` mutation at runtime does
not update `byRef`. A future `UINode::setRef` would fix it; not in
scope.

### Proposal 2 — `Bindable<T>`

**Shipped:** roadmap §5.17.

**Changes from the proposal:**

- `std::unordered_map<uint64_t, std::function<...>>` was replaced by
  `std::vector<Entry>` for subscriber storage. Reason: `std::vector`
  preserves insertion order (the standard guarantees it), while
  `std::unordered_map` doesn't. Notification order is part of the
  contract (§2.5), so the vector is the correct choice. It's also more
  cache-friendly for the small N typical of UI subscribers.
- `bindTo` gained a **second overload** for `shared_ptr<W>`. The
  proposal only listed the `weak_ptr` version, but the common case at
  the call site is `bindTo(progress, &ProgressBar::setValue)` where
  `progress` is a `shared_ptr`. The overload wraps the `weak_ptr`
  version, so both are equivalent.
- `onChanged` and `bindTo` gained a `fireImmediately` parameter
  (default `false`). The proposal treated this as an open question;
  the implementation settled on "yes, include it" because it's a
  common enough pattern (new widget reflects current value at
  subscribe time) that the alternative would have forced every
  caller to duplicate it.

**What landed:**

- `src/ui/Bindable.hpp` — `Bindable<T>`, `Subscription`,
  `ControlBlock`, `bindTo` (both overloads).
- `src/ui/UI.hpp` — include added.
- `tests/test_bindable.cpp` — 24 tests covering construction, set
  short-circuit, notification order, RAII unsubscribe, recursion,
  unsubscribe-in-own-callback, `bindTo` for `float` / `std::string` /
  `bool`, `bindTo` with `fireImmediately`, expired `weak_ptr`,
  `Subscription` outliving `Bindable`, `Bindable` destroyed then
  `Subscription` destroyed, type constraints.
- `docs/user/11-bindable.md` — motivation, patterns, lifetime,
  pitfalls.
- `docs/api/Bindable.md` — full reference.
- `docs/CHEATSHEET.md`, `docs/GLOSSARY.md`, `docs/ROADMAP.md`,
  `docs/README.md`, `README.md` — all updated.

**Known limitation carried forward:** `T` must have `operator==`.
This is a deliberate constraint, not a defect — `set()` needs to
detect "same value" for the short-circuit. `float` works with the
usual caveat.

**Shipped:** `examples/12_bindable` landed as a follow-up commit
(`251e7dd`), demonstrating model → 3 widgets and the `bindTo`
lifetime safety. Roadmap §5.17 has the corresponding checkbox
marked `[x]`.

### Proposal 3 — UIPrefab and Slot

**Shipped:** roadmap §5.18 and §5.19.

**Changes from the proposal:**

- **No `SlotWrapper` in the final tree.** The original design (and MVP implementation) wrapped injected children in a dummy `Stack` node. This broke Flexbox layouts (e.g., `justify="space-between"`) because the container saw only one child (the wrapper). The shipped implementation completely removes the wrapper: `Registry::create_impl` now returns a `std::vector<std::shared_ptr<Layout>>`, allowing the `<Slot>` tag to seamlessly unwrap and inject its children directly into the parent's flow as flat siblings.
- **Strict CSS syntax for inline properties.** During testing, we enforced that properties passed to a prefab instance (like `justify="space-between"`) must strictly follow the kebab-case CSS specification, ensuring exactly 1:1 parity between `.zstyle` rules and ZMarkup inline attributes.

**What landed:**

- `ZMarkup::registerPrefab` and `ZMarkup::registerPrefabFile` added to `Registry` (and exposed globally).
- `<Slot>` tag fully supported natively by the parser and builder.
- AST caching: Prefabs parse their ZMarkup string only once.
- Zero-allocation instantiation: Building a prefab clones the cached AST (`Element`), merging root attributes (`.class`, `#id`, `@ref`, and inline styles) at zero parser cost.
- Support for default fallback children inside `<Slot> { ... }`.
- Robust anti-loop guards (`isPrefabRoot` flags) to prevent circular dependency crashes when building the tree.
- 3 new unit tests in `test_zmarkup_build.cpp` verifying root merging, slot injection, and default fallbacks.
- New example `13_prefabs` demonstrating a dynamic Settings UI built entirely with reusable components.
- Comprehensive updates to `user/04-zmarkup.md` (§6), `CHEATSHEET.md`, `GLOSSARY.md`, and `ROADMAP.md`.

### Proposal 4 — StyleParser Diagnostics

**Shipped:** roadmap §5.20.

**What landed:**
- `DiagnosticSink`, `CollectingDiagnosticSink`, and `ConsoleDiagnosticSink` introduced in `Diagnostics.hpp`.
- `SourceSpan` added to track file, line, column, and token length for precise error reporting.
- AST nodes (`Declaration`, `Rule`, `KfRule`, `KfFrame`) now carry `SourceSpan` instead of plain line/col.
- Replaced rigid macro-driven property checks with a centralized `getKnownStyleProperties()` array automatically synced with `BUBBLE_STYLE_PROPS`.
- `loadStyleString` and `loadStyleFile` now take an optional `DiagnosticSink*` and a `strict` boolean flag.
- Added 6 unit tests in `test_diagnostics.cpp` covering Levenshtein suggestions, strict mode, pseudo-class validation, and duplicate keyframe detection.

### Proposal 5 — Wrap for HStack

**Shipped:** roadmap §5.21.

**What landed:**
- Added `wrap` boolean property to `Style` (`BUBBLE_STYLE_PROPS`) and `ComputedStyle`.
- `StyleParser` now parses `wrap: true`, `wrap: 1`, and `wrap: wrap`.
- `Measure.cpp` implements `arrangeWrapFlow` to handle multi-line breaking when horizontal children exceed the container's width.
- Supports per-line `justify` (e.g., `Center`, `SpaceBetween`, `End`).
- `contentSize` dynamically computes the exact bounding box encompassing all wrapped lines, accurately updating the parent height.

### Proposal 8 — VirtualView

**Shipped:** roadmap §5.24.

**Changes from the proposal:**

- **Materialization buffer of 2 rows above and 2 below**, not 1.
  The 1-row buffer in the proposal caused visible churn during
  scrollbar drags (the boundary row was destroyed and rebuilt every
  few pixels). 2 rows is the smallest value that hides the churn in
  practice; the cost is +2 nodes in the steady state.
- **`onItemCreated` fires on every materialization, including
  re-materialization.** The proposal did not specify whether the
  callback fires once per index or every time a node is created. The
  implementation picks "every time" because that is the only
  definition that lets the callback configure the new node — the node
  is destroyed on out-of-window, so a new node must be configured
  from scratch on re-entry. Documented in the user page §7 pitfall 3.
- **`getFirstVisibleItem` / `getLastVisibleItem` return `-1` before
  the first measure and for empty lists.** The proposal did not
  specify the empty-state value. `-1` was chosen because it is not a
  valid index in any case and can be checked with a single `< 0`
  condition. Documented in the user page §3.4.
- **No ZMarkup registration.** The proposal listed a possible
  follow-up tag taking a prefab name, but the implementation ships
  C++-only. The `ROADMAP.md` §5.24 checklist keeps the item open as
  `- [ ] (Follow-up) ZMarkup registration`.

**What landed:**

- `include/components/VirtualView.hpp` — `VirtualView` class,
  header-only, as a `TLayout<VirtualView, ScrollView>`.
- `src/ui/UI.hpp` — include added.
- `tests/test_virtual_view.cpp` — 14 tests covering item count,
  uniform height, visibility queries (uniform and auto),
  `getMaxScrollY`, `scrollToItem`, materialization bound, drop on
  scroll, `onItemCreated`, `removeAllChildren`, and the
  before-first-measure sentinel.
- `docs/user/12-virtual_view.md` — full user page: when to use, quick
  start, API, materialization model, uniform-vs-auto comparison,
  scroll interaction, pitfalls, examples.
- `docs/api/Widgets.md` — `VirtualView` entry.
- `docs/user/03-widgets.md` — `VirtualView` section.
- `docs/CHEATSHEET.md`, `docs/GLOSSARY.md`, `docs/README.md` —
  cross-references.
- `docs/ROADMAP.md` — §5.24 with the shipped checklist.

**Known limitation carried forward:** VirtualView does not recycle
nodes. Scrolling a long list for a long time keeps allocating and
freeing row nodes; for most lists the allocator handles this fine,
but a user who scrolls a 100k-row list for minutes will see GC-like
pauses. This is the exact problem VirtualList (Proposal 6 / roadmap
§5.23) solves with node recycling. VirtualView is not deprecated by
VirtualList — it stays the simpler option for lists where allocation
churn is not a concern, and it is the only large-list option until
VirtualList ships.