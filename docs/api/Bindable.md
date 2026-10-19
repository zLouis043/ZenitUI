# `Bindable` — API Reference

A small, header-only observable value type. Holds a `T`, notifies
subscribers when the value changes, and provides RAII subscription
tokens with lifetime safety.

- **Header**: `Bindable.hpp`
- **Namespace**: `ZenitUI`

For usage examples and patterns, see
[Data binding](../user/11-bindable.md). This document is the method
reference.

---

## 1. Overview

```cpp
template <typename T>
class Bindable {
public:
    class Subscription { /* ... */ };

    Bindable();
    explicit Bindable(T initial);

    const T& get() const;
    void     set(T v);

    Subscription onChanged(std::function<void(const T&)> cb,
                           bool fireImmediately = false);

    template <typename W, typename Arg>
    Subscription bindTo(std::weak_ptr<W> widget,
                        void (W::*method)(Arg),
                        bool fireImmediately = false);

    template <typename W, typename Arg>
    Subscription bindTo(std::shared_ptr<W> widget,
                        void (W::*method)(Arg),
                        bool fireImmediately = false);
};
```

`Bindable<T>` is **non-copyable and non-movable**. Copy is deleted to
preserve the one-to-one relationship between a `Bindable` and its
`ControlBlock`. Move is deleted for the same reason.

`T` requirements:

| Requirement | When | Why |
|-------------|------|-----|
| `operator==` | Always | `set()` uses it to detect real changes. |
| Default-constructible | Only if you use `Bindable()` (no initial) | Otherwise `Bindable<T>{initial}` is enough. |
| Copy/move-constructible | Always | `get()`, `set()`, and notification pass values around. |

---

## 2. Construction

### `Bindable()`

Default constructor. `get()` returns a value-initialized `T`
(`0`, `""`, `false`, …). Equivalent to `Bindable<T>{T{}}`.

### `explicit Bindable(T initial)`

Constructs with an initial value. `get()` returns `initial` until the
first `set()`. No notification is fired during construction.

```cpp
Bindable<float>       volume{0.65f};
Bindable<std::string> name{std::string("Track A")};
Bindable<bool>        enabled{true};
```

### Deleted copy and move

```cpp
Bindable<int> a{1};
Bindable<int> b = a;              // ❌ does not compile
Bindable<int> c = std::move(a);   // ❌ does not compile
```

If you need to move a `Bindable` between scopes, wrap it in
`std::unique_ptr`:

```cpp
auto b = std::make_unique<Bindable<float>>(0.5f);
// later
some_owner.take(std::move(b));
```

---

## 3. Value access

### `const T& get() const`

Returns the current value. No side effects, no notifications. The
returned reference is valid until the next `set()` on the same
`Bindable`.

### `void set(T v)`

Sets the value and notifies every active subscriber.

- If `v == get()`, **nothing happens**: no assignment, no notification.
  This is the "same value is a no-op" rule.
- Otherwise, `value_ = std::move(v)` is performed first, then each
  subscriber is called with `const T&` to the new value, in
  registration order.

`set` takes its argument **by value**. This allows the caller to
`std::move` for expensive `T`:

```cpp
volume.set(0.5f);                  // float, cheap
name.set(std::string("Track B"));  // one move, not one copy
```

A callback that unsubscribes itself (or another subscription) during
notification is honored immediately: the cancelled subscription
doesn't receive the current value on the same pass.

A callback that calls `set()` recursively on the **same** `Bindable`
triggers a new notification pass. If the value doesn't change, it's
a no-op (no infinite loop for the "echo the current value" case).

---

## 4. `Subscription`

```cpp
class Subscription {
public:
    Subscription() = default;
    ~Subscription();

    void unsubscribe();
    bool active() const;

    Subscription(Subscription&&) noexcept;
    Subscription& operator=(Subscription&&) noexcept;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
};
```

A move-only RAII token representing one subscription. Destroying it
unsubscribes. Moving it transfers ownership of the subscription.

### `~Subscription()`

Calls `unsubscribe()` if the subscription is still active. Safe to
call after the `Bindable` has been destroyed (no crash, no
dereference of freed memory).

### `void unsubscribe()`

Cancels the subscription. After this call:

- The callback never fires again.
- `active()` returns `false`.
- Calling `unsubscribe()` again is a no-op.

`unsubscribe()` is idempotent and thread-safe in the sense that
calling it from the same thread multiple times has no adverse effect
— but `Bindable` itself isn't thread-safe, so don't call it
concurrently with `set()`.

### `bool active() const`

Returns `true` if the subscription is live. A subscription is
inactive after:

- `unsubscribe()` was called on it.
- The `Bindable` it subscribed to has been destroyed.

```cpp
Bindable<float> b{0.5f};
auto sub = b.onChanged([](const float&){});
CHECK(sub.active());     // true

b.~Bindable();           // (or let it go out of scope)
CHECK(!sub.active());    // false — safe to check, safe to destroy
```

### Move semantics

```cpp
auto sub1 = volume.onChanged(cb);
auto sub2 = std::move(sub1);    // sub1 now inactive, sub2 owns
sub1.unsubscribe();             // no-op
```

---

## 5. `onChanged`

```cpp
Subscription onChanged(std::function<void(const T&)> cb,
                       bool fireImmediately = false);
```

Subscribes a callback to changes on this `Bindable`. Returns a
`Subscription` that can be used to cancel.

### Callback signature

`void(const T&)` — the current value is passed by const reference.
For `T` that's cheap to copy (POD, pointers), this is the same as
passing by value at the call site. For expensive `T`
(`std::string`, `std::vector`), this avoids the copy entirely.

If you need to **modify** the value inside the callback, copy it
locally first. The reference is invalidated by the next `set()`.

### `fireImmediately`

If `true`, `cb` is invoked **synchronously** with `get()` before
`onChanged` returns. This is useful for wiring a widget to a value
that's already non-default:

```cpp
volume.set(0.8f);   // happens first
// ...
auto pb = ProgressBar::create(0.0f);
auto sub = volume.onChanged(
    [pb](const float& v){ pb->setValue(v); },
    /*fireImmediately=*/true);
// pb->getValue() == 0.8f, not 0.0f.
```

The `Subscription` is constructed before `cb` fires, so the callback
can call `unsubscribe()` on itself if needed (via a forward-declared
`Subscription`, see the pitfalls in the user guide).

### Notification order

Subscribers fire in **registration order**: the first subscriber
registered is the first to be called.

If a callback subscribes a **new** callback during notification, the
new one doesn't fire for the current `set()` — it fires starting from
the next one.

If a callback calls `unsubscribe()` on itself or on another
subscription, the cancellation takes effect immediately: the
cancelled subscription doesn't receive the current value, even if its
turn in the registration order hasn't come yet.

### Subscribers survive widget destruction (when using `bindTo`)

`onChanged` itself doesn't check the lifetime of the callback's
captures. If the callback captures a widget by raw pointer or
`shared_ptr`, the `Bindable` doesn't know when the widget dies.

For lifetime-safe widget binding, use `bindTo` instead.

---

## 6. `bindTo`

Two overloads, one for `weak_ptr<W>` and one for `shared_ptr<W>`. Both
return the same `Subscription` type.

```cpp
template <typename W, typename Arg>
Subscription bindTo(std::weak_ptr<W> widget,
                    void (W::*method)(Arg),
                    bool fireImmediately = false);

template <typename W, typename Arg>
Subscription bindTo(std::shared_ptr<W> widget,
                    void (W::*method)(Arg),
                    bool fireImmediately = false);
```

Subscribes the given **method** of the given **widget** to changes on
this `Bindable`. Internally captures the widget as a `weak_ptr`, so
the callback is a no-op if the widget has been destroyed.

### Method signature

`Arg` is deduced from the method pointer type. This makes `bindTo`
work transparently with both value and const-ref parameters:

```cpp
// void ProgressBar::setValue(float)
volume.bindTo(progress, &ProgressBar::setValue);     // Arg = float

// void Text::setText(std::string)
name.bindTo(label, &Text::setText);                  // Arg = std::string

// void Layout::setEnabled(bool)
enabled.bindTo(widget, &Layout::setEnabled);         // Arg = bool
```

No wrapper lambda is needed.

### Lifetime behavior

If `widget` is destroyed before the `Bindable`:

- The callback still fires (the `Subscription` is still active).
- The call to `(w.get()->*method)(v)` is skipped because
  `widget.lock()` returns null.
- No crash, no dangling pointer.

If the `Bindable` is destroyed before the widget:

- The `Subscription` becomes inactive (via `ControlBlock::alive_`).
- The widget's method is never called again.
- Destroying the `Subscription` is a no-op.

### `fireImmediately`

Same semantics as `onChanged`. If `true`, the method is called
synchronously with `get()` before `bindTo` returns.

If the widget has already been destroyed at `bindTo` time (rare but
possible), the immediate call is skipped.

---

## 7. Notification semantics

### Same-value short-circuit

`set(v)` compares with `v == get()`. If equal, no assignment, no
notification, no state change.

```cpp
Bindable<int> b{42};
int calls = 0;
auto sub = b.onChanged([&](const int&){ calls++; });

b.set(42);   // no-op
CHECK(calls == 0);

b.set(43);   // notifies
CHECK(calls == 1);
```

### Notification snapshot

`set()` iterates a **copy** of the subscriber list. This is what
makes `unsubscribe()` safe inside a callback (the copy is unaffected
by the removal from the source list). The cost is one vector copy per
`set()` — negligible for typical subscriber counts (1–5).

### Recursion

Calling `set()` on the **same** `Bindable` inside a callback triggers
a new notification pass. If the new value equals the current value,
it's a no-op (no infinite loop). If the new value differs, the new
pass begins immediately, before the outer `set()` returns.

Two `Bindable`s notifying each other is a caller bug. `Bindable`
doesn't detect or prevent infinite loops.

---

## 8. Lifetime: `ControlBlock`

Internally, `Bindable<T>` shares a `ControlBlock` with every
`Subscription` it hands out:

```cpp
struct ControlBlock {
    Bindable* owner_{nullptr};
    bool      alive_{true};
};
```

- `Bindable` owns a `shared_ptr<ControlBlock>`, created lazily on the
  first `onChanged` / `bindTo`.
- Each `Subscription` holds a copy of the same `shared_ptr`.
- `~Bindable()` sets `ctrl_->alive_ = false` before its own
  destruction completes.
- `~Subscription()` and `unsubscribe()` check `ctrl_->alive_` before
  dereferencing `ctrl_->owner_`.

This is what makes the following all safe:

```cpp
// Case 1: Subscription outlives Bindable
Subscription sub;
{
    Bindable<float> b;
    sub = b.onChanged([](const float&){});
}
sub.unsubscribe();   // no-op, no crash

// Case 2: Bindable outlives Subscription (the normal case)
Bindable<float> b;
{
    auto sub = b.onChanged([](const float&){});
}
// Subscription destroyed, unsubscribed, b continues

// Case 3: both destroyed in either order
{
    Bindable<float> b;
    auto sub = b.onChanged([](const float&){});
}
```

The cost is one `shared_ptr` copy per subscription (16 bytes) and one
refcount bump at subscribe time.

`ControlBlock` is an implementation detail, not part of the public API.
It's documented here because its behavior explains the lifetime
guarantees.

---

## 9. Thread safety

**`Bindable<T>` is not thread-safe.**

- `get()`, `set()`, `onChanged`, `bindTo`, `unsubscribe`, `active()`,
  `~Bindable`, `~Subscription` all touch the same state without
  synchronization.
- Calling `set()` from a background thread while the main thread reads
  or notifies is undefined behavior.

If a producer runs on a background thread, post the value to the main
thread (queue, `std::atomic`, engine-specific mechanism) and call
`set()` from there. `Bindable` doesn't do this for you.

`ControlBlock::alive_` is a plain `bool`, not `std::atomic<bool>`,
because the class isn't thread-safe by design.

---

## 10. Pitfalls

**`Bindable<T>` cannot be copied or moved.**
This is intentional. If you need to pass ownership, use
`std::unique_ptr<Bindable<T>>`.

**`onChanged` doesn't track the lifetime of captures.**
If the callback captures a widget by `shared_ptr` or raw pointer, the
`Bindable` doesn't know when the widget dies. Use `bindTo` (which
captures a `weak_ptr` internally) for widget-method binding, or
capture a `weak_ptr` manually and check it in the callback.

**Subscribers fire in registration order, but the order isn't stable
across `set` calls if subscriptions change.**
If you unsubscribe subscriber A and then subscribe a new one, the new
one gets the highest token and fires last. This is usually what you
want, but don't rely on "the same callback always fires before
another" if subscriptions come and go between `set` calls.

**`fireImmediately` runs the callback inside `onChanged`.**
If the callback throws, the `Subscription` hasn't been constructed yet
and the exception propagates out of `onChanged`. For widget setters
(which don't throw), this isn't an issue. For user callbacks that
might throw, wrap them in a try/catch inside the lambda.

**`Subscription` is move-only.**
`std::vector<Subscription>` works (vector uses move on push_back),
but `std::vector<Subscription>::push_back(sub)` needs `sub` to be an
rvalue (`std::move(sub)`) or the return value of `onChanged`/`bindTo`.

**`get()` returns a const reference that becomes stale after `set()`.**
If you hold `const T& ref = b.get()` and then `b.set(newValue)`,
`ref` might dangle (for `T` that reallocates internally, like
`std::string` or `std::vector`). Copy if you need to hold the value
across a `set()`.

**`set()` short-circuits on equal values, but `T::operator==` must
be accurate.**
A `T` with a lossy `operator==` (e.g. floats compared with `==`) will
either miss real changes (rare) or fire too often (also rare). For
`float`, this is the usual `float` equality caveat — most UI uses are
fine with it.

**`ControlBlock` is created lazily.**
If `Bindable` is constructed but never `onChanged`'d or `bindTo`'d,
it has no `ControlBlock`. `~Bindable` checks `ctrl_` before
dereferencing.

**A `Bindable` destroyed while a `Subscription` is being used.**
Safe, but the callback won't fire. If a `Subscription` is created
inside a scope, and the `Bindable` goes out of scope first, the next
`set()` on a different `Bindable` doesn't touch it. This is by design
— but if you expected the callback to fire one last time, it won't.

**Two `Bindable`s notifying each other is an infinite loop.**
`A.onChanged` calling `B.set`, `B.onChanged` calling `A.set`, and so
on. `Bindable` doesn't detect this. It's the caller's responsibility.

---

## 11. See also

- [Data binding](../user/11-bindable.md) — usage, patterns, motivation.
- [Widgets](Widgets.md) — the `on*` callbacks that `Bindable`
  complements.
- [Events](../user/06-events.md) — the manual callback model.
- [Roadmap §5.17](../ROADMAP.md) — the design rationale.