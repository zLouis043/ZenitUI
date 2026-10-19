# Data binding

`Bindable<T>` is a small, header-only class that holds a value of type
`T` and notifies its observers when the value changes. It's not a
widget, not a `Layout`, and not involved in the frame loop — it's a
plain value with observer semantics.

Use it to **eliminate the "callback glue" that accumulates in every
non-trivial UI controller**. If you've ever written this:

~~~cpp
slider->onValueChanged = [progress, label, audioSystem](float v) {
    progress->setValue(v);
    label->setText(std::to_string((int)(v * 100)) + "%");
    audioSystem.setMasterVolume(v);
};
~~~

…and then had to edit the lambda every time a new widget or system
needed to react to `v`, `Bindable<T>` is for you.

---

## 1. The problem

In a traditional retained-mode UI, the model lives in one place
(`float volume` inside a settings controller) and the widgets that
display it are scattered across the screen. Without a binding system,
every change to the model has to be manually propagated to every
widget that displays it, and every widget that edits it has to
manually push back to the model.

The result is boilerplate that:

- **Grows with every widget.** Adding a second progress bar means
  editing the slider's callback.
- **Breaks on destruction.** A callback that captures a widget by
  `shared_ptr` keeps it alive forever. A callback that captures it by
  raw pointer dangles when the widget dies.
- **Mixes concerns.** The slider callback shouldn't know that
  `audioSystem` exists.

---

## 2. The solution

`Bindable<T>` is a **value with a list of observers**:

~~~cpp
Bindable<float> volume{0.65f};

// Write side: slider pushes into the model.
slider->onValueChanged = [&volume](float v) { volume.set(v); };

// Read side: each consumer subscribes.
auto sub1 = volume.onChanged([progress](const float& v) {
    progress->setValue(v);
});
auto sub2 = volume.onChanged([label](const float& v) {
    label->setText(std::to_string((int)(v * 100)) + "%");
});
~~~

When `volume.set(0.8f)` is called:

1. `volume.get()` returns `0.8f` from then on.
2. `sub1`'s callback is called with `0.8f`.
3. `sub2`'s callback is called with `0.8f`.

If the value doesn't change (`v == current`), no callback fires.

If `sub1` is destroyed (its `Subscription` goes out of scope), it's
automatically unsubscribed. If `sub2` outlives its `progress` widget,
it becomes a no-op.

---

## 3. API

### 3.1 `Bindable<T>`

~~~cpp
template <typename T>
class Bindable {
public:
    Bindable();                          // default-constructed T
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
~~~

`Bindable<T>` is **non-copyable and non-movable**. This guarantees
that each instance has exactly one "identity" for its subscriptions.
Move-only would have been possible but complicates the lifetime
semantics for no real benefit.

`T` must satisfy:

- `operator==` — used by `set()` to detect changes.
- Default-constructible — only if you use the default constructor.
  `Bindable<T>{initial}` works for any `T`.
- Copy/move-constructible — as `get()` and `set()` require.

### 3.2 `Subscription`

~~~cpp
class Subscription {
public:
    Subscription() = default;
    ~Subscription();

    void unsubscribe();
    bool active() const;

    // Move-only
    Subscription(Subscription&&) noexcept;
    Subscription& operator=(Subscription&&) noexcept;
};
~~~

A `Subscription` is an **RAII token**. Its destructor unsubscribes.
This is the only way to subscribe — `onChanged` returns one, and
`bindTo` returns one.

`unsubscribe()` can be called explicitly to end the subscription early.
It's idempotent (safe to call multiple times).

`active()` returns `true` if the subscription is live (i.e. the
`Bindable` exists and the subscription hasn't been cancelled).

### 3.3 `onChanged(cb, fireImmediately)`

~~~cpp
auto sub = volume.onChanged([](const float& v){ /* ... */ });
~~~

The callback fires on every change of the `Bindable`. The argument is
`const T&` (no copy for large `T`).

If `fireImmediately = true`, the callback is invoked once with the
current value at subscribe time, before `onChanged` returns. This is
useful for wiring a progress bar to a volume that already has a
non-zero value:

~~~cpp
auto sub = volume.onChanged([progress](const float& v) {
    progress->setValue(v);
}, /*fireImmediately=*/true);
// progress now reflects the current volume, not the default 0.
~~~

Callbacks fire in **registration order**. If a callback unsubscribes
itself or another subscription during notification, the removal takes
effect immediately: the cancelled subscription doesn't receive the
current value.

### 3.4 `bindTo(widget, method, fireImmediately)`

Convenience wrapper for the common case "when the value changes, call
this method on this widget":

~~~cpp
auto sub = volume.bindTo(progress, &ProgressBar::setValue);
~~~

is equivalent to:

~~~cpp
auto sub = volume.onChanged([progress](const float& v) {
    progress->setValue(v);
});
~~~

but shorter and **lifetime-safe** (uses `weak_ptr` internally).

`bindTo` has two overloads: one taking `weak_ptr<W>` and one taking
`shared_ptr<W>`. Both return the same `Subscription`.

The `Arg` template parameter is deduced from the method signature.
This means `bindTo` works transparently with both methods that take
the argument by value and by const-ref:

~~~cpp
volume.bindTo(progress, &ProgressBar::setValue);   // void(float)
name.bindTo(label,      &Text::setText);           // void(std::string)
enabled.bindTo(widget,  &Layout::setEnabled);      // void(bool)
~~~

No wrapper lambda needed.

---

## 4. Lifetime safety

The critical design decision of `Bindable<T>` is that **a
`Subscription` can outlive its `Bindable<T>` without crashing**.

~~~cpp
Subscription sub;
{
    Bindable<float> temp{0.5f};
    sub = temp.onChanged([](const float&){});
}   // temp destroyed
sub.unsubscribe();   // no-op, no crash
~~~

This is implemented via a shared `ControlBlock` between the
`Bindable` and every `Subscription` it hands out:

- `Bindable` owns `shared_ptr<ControlBlock>`.
- Every `Subscription` holds a copy of the same `shared_ptr`.
- `~Bindable` sets `ctrl_->alive_ = false` **before** destruction
  completes.
- `~Subscription` checks `ctrl_->alive_` before dereferencing
  `ctrl_->owner_`.

The cost is one `shared_ptr` copy per subscription (16 bytes) and one
refcount bump at subscribe time.

**What this buys you:** the pattern "subscribe from a widget's
constructor, forget to unsubscribe because the widget is destroyed
first" is safe by construction.

---

## 5. Thread safety

**`Bindable<T>` is single-thread only.** It's not thread-safe and
doesn't claim to be. The framework is single-thread; `Bindable` lives
in the same constraint.

If a game runs on multiple threads, the producer thread must post to
the main thread (via a queue, a `std::atomic`, or an engine-specific
mechanism) **before** calling `set()`. `Bindable` doesn't do this for
you and doesn't try to.

Calling `set()` from a background thread while the main thread reads
`get()` or triggers a callback is undefined behavior.

---

## 6. Patterns

### 6.1 Two-way binding (widget ↔ model)

The most common pattern: a widget edits a value, the value updates the
model, the model updates other widgets.

~~~cpp
// Model
Bindable<float> volume{0.5f};

// Widget that edits the model
slider->onValueChanged = [&volume](float v) { volume.set(v); };

// Widgets that display the model
auto sub1 = volume.onChanged([progress](const float& v) { progress->setValue(v); });
auto sub2 = volume.onChanged([label](const float& v) {
    label->setText(std::to_string((int)(v * 100)) + "%");
});
~~~

No flags, no "am I echoing my own update?" checks. `Bindable::set`
short-circuits when `v == current`, so a slider that fires `onValueChanged`
back with the same value is a no-op.

### 6.2 `bindTo` for widget methods

When the "reaction" is a single widget method, `bindTo` removes the
wrapper lambda:

~~~cpp
// Instead of this:
auto sub = volume.onChanged([progress](const float& v) {
    progress->setValue(v);
});

// Write this:
auto sub = volume.bindTo(progress, &ProgressBar::setValue);
~~~

`bindTo` also handles lifetime: if `progress` is destroyed, the
subscription becomes a no-op (no dangling pointer, no crash).

### 6.3 Fire immediately on subscribe

When a new widget is created **after** the value has already been set,
you usually want it to reflect the current value right away:

~~~cpp
volume.set(0.8f);   // happens first
// ...
auto newProgress = ProgressBar::create(0.0f);
volume.bindTo(newProgress, &ProgressBar::setValue, /*fireImmediately=*/true);
// newProgress now shows 0.8, not the default 0.
~~~

### 6.4 Manual unsubscribe

A `Subscription` can be cancelled manually:

~~~cpp
auto sub = volume.onChanged([](const float&){});
// ... some time later
sub.unsubscribe();
~~~

After `unsubscribe()`, the callback never fires again, even if `set()`
is called on the `Bindable`. `sub.active()` returns `false`.

### 6.5 Subscription outliving the widget

`bindTo` already handles this via `weak_ptr`. But if you use
`onChanged` directly with a raw capture:

~~~cpp
// ⚠️ DANGEROUS: captures progress by shared_ptr
auto sub = volume.onChanged([progress](const float& v) {
    progress->setValue(v);
});
// progress is now kept alive by the subscription, even
// after the UI removes it.
~~~

The correct pattern is to let `bindTo` handle it, or capture a
`weak_ptr` manually:

~~~cpp
std::weak_ptr<ProgressBar> weakProgress = progress;
auto sub = volume.onChanged([weakProgress](const float& v) {
    if (auto p = weakProgress.lock()) p->setValue(v);
});
~~~

But `bindTo` is cleaner. Prefer it.

---

## 7. Comparison with manual callbacks

Without `Bindable`, wiring three widgets to one value looks like this:

~~~cpp
float volume = 0.5f;
slider->onValueChanged = [progress, label, audioSystem, &volume](float v) {
    volume = v;
    progress->setValue(v);
    label->setText(std::to_string((int)(v * 100)) + "%");
    audioSystem.setMasterVolume(v);
};
~~~

Adding a fourth widget means editing this lambda. If `progress` is
destroyed, `progress->setValue(v)` crashes. If `label` is destroyed
first, `label->setText(...)` crashes. The lambda becomes the single
point of failure for the entire UI.

With `Bindable`:

~~~cpp
Bindable<float> volume{0.5f};
slider->onValueChanged = [&volume](float v) { volume.set(v); };

auto sub1 = volume.bindTo(progress, &ProgressBar::setValue);
auto sub2 = volume.onChanged([label](const float& v) {
    label->setText(std::to_string((int)(v * 100)) + "%");
});
auto sub3 = volume.onChanged([&audioSystem](const float& v) {
    audioSystem.setMasterVolume(v);
});
~~~

Each subscription is independent. Adding a fourth widget is one line.
Removing any widget is safe — its `Subscription` (if you kept it)
becomes a no-op, or its RAII token unsubscribes automatically.

The model is the single source of truth. No controller glue.

---

## 8. Pitfalls

**`T` must have `operator==`.**
`Bindable::set(v)` calls `v == current`. If `T` doesn't support
equality comparison, `set` fails to compile. This is intentional: the
whole point is that "set to the same value" is a no-op.

**`Bindable` is non-movable.**
This is deliberate. A movable `Bindable` would have to rewire all its
active subscriptions or share the `ControlBlock` — the first is
expensive, the second is confusing. If you need to move a `Bindable`
around, wrap it in a `std::unique_ptr`.

**Callback order is guaranteed but not stable.**
Subscribers fire in registration order, but if a callback unsubscribes
itself, the order of the *remaining* callbacks is unchanged. If a
callback **subscribes** a new callback during notification, the new
one doesn't fire for the current `set()` — it fires starting from the
next one. This is a common source of "why didn't my callback fire?".

**Recursive `set()` is allowed but discouraged.**
If a callback calls `set()` on the same `Bindable`, a new notification
pass begins. If the new value equals the current value, it's a no-op.
Infinite loops are the caller's responsibility to avoid.

**Two `Bindable`s notifying each other is a caller bug.**
`A.onChanged` calling `B.set`, and `B.onChanged` calling `A.set`, is
an infinite loop. `Bindable` doesn't detect it and doesn't try to.

**`Subscription` keeps no reference to `Bindable`.**
It holds a `weak_ptr` to the `ControlBlock`, not the `Bindable`
itself. This is why a `Subscription` outliving its `Bindable` is safe
— the `ControlBlock` is kept alive by the `Subscription`'s own
`shared_ptr`, and its `alive_` flag is set to `false` by `~Bindable`.

**Threads: post to the main thread before `set()`.**
`Bindable<T>` is not thread-safe. A background producer must post the
new value to the main thread (queue, `std::atomic`, whatever the
engine provides) and call `set()` from there.

**`fireImmediately` runs the callback inside `onChanged`.**
If the callback throws, the `Subscription` hasn't been returned yet.
For typical widget-setter callbacks this isn't an issue; for callbacks
that might throw, wrap them in a try/catch.

**Subscriptions stored in `std::vector<Subscription>`.**
If you store many subscriptions (e.g. all the bindings of a screen),
remember that `Subscription` is **move-only**. `std::vector` handles
this correctly, but `std::vector<Subscription>::push_back` uses moves,
so the vector reallocates fine. Just don't try to copy them.

---

## 9. See also

- [API — Bindable](../api/Bindable.md) — full method reference.
- [Widgets](03-widgets.md) — every built-in widget and its callbacks.
- [Events](06-events.md) — the manual `on*` callback model that
  `Bindable` complements (not replaces).
- [Roadmap §5.17](../ROADMAP.md) — the design rationale behind the
  API.