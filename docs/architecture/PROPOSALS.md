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
  the user-facing API. The "Open questions" section below is kept
  for historical context.
- ✅ **Proposal 2 — `Bindable<T>`**: shipped as roadmap §5.17. See
  `docs/user/11-bindable.md` and `docs/api/Bindable.md` for the
  user-facing API.

---

## Index

- [Proposal 1 — `ref`: separation of style identity and logic lookup](#proposal-1--ref-separation-of-style-identity-and-logic-lookup)
- [Proposal 2 — `Bindable<T>`: lightweight data binding](#proposal-2--bindablet-lightweight-data-binding)
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
  `<memory>`, `<unordered_map>` (no — vector, see §2.3.1),
  `<cstdint>`, `<utility>` — all already included by `Common.hpp`.
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

## Out of scope (future rounds)

The following were discussed in the same review but are **not** part
of this proposal. They are listed here for context and to avoid
re-discussing them.

| Feature | Why deferred |
|---------|--------------|
| `UIPrefab` | High value but depends on `ref` landing first. Separate proposal. |
| `<Slot>` | Completes `UIPrefab`. Separate proposal. |
| Drag & Drop | Touches 6 files (UIContext, InputController, Layout, StyleResolver, StyleParser, hitTest). Must be split into 3 PRs. Separate proposal. |
| `wrap` (HStack wrap) | Medium value, ~100 lines in `Measure.cpp`. Separate proposal. |
| `GridLayout` | Low priority. `Stack`/`Vertical`/`Horizontal` cover 90% of use cases. Later. |
| `VirtualList` | Depends on `UIPrefab`. After `UIPrefab`. |
| Memory management (arena + pool + `UIHandle`/`UIProxy`) | Post-1.0. API-breaking. Performance numbers don't justify it today (500 µs for 1000 nodes = 3% of a 60 fps frame budget). |
| `EventBus` (zero-RTTI) | Out of scope for ZenitUI. Belongs to the game engine roadmap. |
| Hot reload | Out of scope for ZenitUI. Belongs to the consumer. See roadmap §5.10. |

---

## Implementation sequence

1. **Proposal 1 — `ref`** (1 day). Ships in roadmap as §5.16.
2. **Proposal 2 — `Bindable<T>`** (half a day). Ships in roadmap as §5.17.
3. **`UIPrefab`** (1–2 days). Ships in roadmap as §5.18. Depends on `ref`.
4. **`<Slot>`** (half a day). Ships as §5.19. Depends on `UIPrefab`.
5. **StyleParser diagnostics** (half a day). Ships as §5.20. Covers
   *ZStyle-side* warnings only: unknown properties (already logged),
   unparseable values (already logged), unknown pseudo-classes
   (currently silent), duplicate `@keyframes` names, unused `:root`
   custom properties. The `find` / `ref` warnings are **not** part of
   §5.20 — they ship with Proposal 1 as §5.16.
6. **`wrap`** (1 day). Ships as §5.21.
7. **Drag & Drop** (2–3 days, split in 3 PRs). Ships as §5.22a/b/c.
8. **`VirtualList`** (1–2 weeks). Ships as §5.23. Depends on `UIPrefab`.

Post-1.0:

- Memory management (§8 in the roadmap).
- `GridLayout`.
- Out-of-scope items as decided by the user.

---

## File impact summary

| File | Proposal 1 | Proposal 2 |
|------|------------|------------|
| `src/ui/Layout.hpp` | +field, +2 methods | — |
| `src/ui/ZMarkup.hpp` | +parser, +2 UINode methods, +logging | — |
| `src/ui/Bindable.hpp` | — | new file |
| `src/ui/UI.hpp` | — | +include |
| `src/ui/Debug.hpp` | +ref in dump | — |
| `tests/test_zmarkup_build.cpp` | +12 tests | — |
| `tests/test_layout.cpp` | +2 tests | — |
| `tests/test_bindable.cpp` | — | +16 tests |
| `docs/user/04-zmarkup.md` | edit | — |
| `docs/user/11-bindable.md` | — | new file |
| `docs/api/ZMarkup.md` | edit | — |
| `docs/api/Layout.md` | edit | — |
| `docs/api/Bindable.md` | — | new file |
| `docs/internals/01-architecture.md` | edit | — |
| `docs/CHEATSHEET.md` | edit | edit |
| `docs/GLOSSARY.md` | edit | edit |
| `docs/ROADMAP.md` | +§5.16 | +§5.17 |

Zero deletions in either proposal. Both are purely additive.

---

## Shipped proposals

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

**Follow-up (not in this proposal):** `examples/12_bindable` as a
live demonstration of the API, tracked as an unchecked item in
roadmap §5.17. Will land as a separate commit.