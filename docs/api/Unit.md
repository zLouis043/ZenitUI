# `Unit` and `Value` — API Reference

The `Value` struct is ZenitUI's length type. It represents a
**sum of terms**, where each term is a coefficient and a unit. This
is what makes `calc()` work and what allows a single value to mix
units (e.g. `calc(100% - 20px)`).

- **Header**: `Unit.hpp`
- **Namespace**: `ZenitUI`

---

## 1. The `Unit` enum

```cpp
enum class Unit {
    Auto    = -1,
    Number  = 0,
    Pixel   = 1,
    Percent = 2,
    VW      = 3,
    VH      = 4,
    PW      = 5,   // % of the parent's WIDTH
    PH      = 6    // % of the parent's HEIGHT
};
```

| Value | Meaning |
|-------|---------|
| `Auto` | The value is unset. Resolution returns the `autoValue` argument (or 0). |
| `Number` | A bare number. Useful for arithmetic inside `calc()` and for properties like `grow` / `shrink`. |
| `Pixel` | Logical pixels. Same as `Number` at resolution time but semantically distinct. |
| `Percent` | Percentage of the **relevant axis** of the parent. Width for horizontal props, height for vertical props. |
| `VW` | 1% of the viewport width (`Metrics::viewport.x`). |
| `VH` | 1% of the viewport height (`Metrics::viewport.y`). |
| `PW` | Percentage of the parent's **width**, regardless of axis. |
| `PH` | Percentage of the parent's **height**, regardless of axis. |

**`Percent` vs `PW` / `PH`**: `Percent` is axis-aware — `width: 50%`
resolves against the parent's width, `height: 50%` against the parent's
height. `PW` is always the parent's width and `PH` always its height,
regardless of which property they're used in.

---

## 2. The `Value` struct

```cpp
struct Value {
    struct Term {
        float coeff{0.0f};
        Unit  unit{Unit::Auto};

        bool operator==(const Term& o) const;
    };

    std::vector<Term> terms;

    Value() = default;
    Value(float pixels);
    Value(float amount, Unit u);

    // ... see below ...
};
```

A `Value` is a vector of `Term`s. Empty means `Auto`. A single term
is the common case (`Px(10)` has one term). Multiple terms come from
`calc()` (e.g. `calc(100% - 20px)` has two terms).

### 2.1 Construction

```cpp
Value v1;                      // Auto
Value v2 = Px(10.0f);          // one Pixel term
Value v3 = Percent(50.0f);     // one Percent term
Value v4 = Px(10.0f) + Percent(50.0f);  // two terms
```

The two direct constructors:

| Constructor | Effect |
|-------------|--------|
| `Value(float pixels)` | Creates a single `Pixel` term. |
| `Value(float amount, Unit u)` | Creates a single term with the given unit. If `u == Unit::Auto`, the result is empty. |

### 2.2 Factory functions

```cpp
static Value autoSize();
static Value number(float n);
static Value px(float p);
static Value percent(float p);
static Value vw(float p);
static Value vh(float p);
static Value pw(float p);
static Value ph(float p);
```

Each returns a `Value` with a single term of the corresponding unit.

### 2.3 Free helpers

Shorter names, defined in `namespace ZenitUI`:

```cpp
inline Value Auto();
inline Value Px(float p);
inline Value Percent(float p);
inline Value VW(float p);
inline Value VH(float p);
inline Value PW(float p);
inline Value PH(float p);
inline Value Num(float p);
```

These are what you use in most code:

```cpp
node->size(Px(100), Percent(50));
node->getInlineBase().gap = VH(2.0f);
```

---

## 3. Queries

### `bool isAuto() const`

True if the value is `Auto` (empty terms vector).

### `bool isSimple() const`

True if the value has exactly one term.

### `bool isNumber() const`

True if the value has exactly one term and it's a `Number`.

### `float asNumber() const`

Returns the coefficient if `isNumber()`, else 0.

---

## 4. Resolution

Four resolution methods, differing in which axis each unit resolves
against.

### `float resolveH(float parentW, float parentH, float autoValue = 0) const`

Resolves for a **horizontal** property (e.g. `width`, `padding-left`).

| Unit | Multiplied by |
|------|---------------|
| `Number` / `Pixel` | 1 |
| `Percent` | `parentW` |
| `PW` | `parentW` |
| `PH` | `parentH` |
| `VW` | `Metrics::viewport.x` |
| `VH` | `Metrics::viewport.y` |

### `float resolveV(float parentW, float parentH, float autoValue = 0) const`

Resolves for a **vertical** property (e.g. `height`, `padding-top`).

Same as `resolveH` except `Percent` uses `parentH` instead of
`parentW`. `PW` and `PH` are unchanged (always width and height
respectively).

### `float resolveSelfH(float selfW, float selfH) const`

Resolves against the **node's own** width for `Percent` / `PW`, and
its own height for `PH`. Used for `translate-x` / `translate-y`, which
resolve against the node's own size, not the parent's.

No `autoValue` parameter — an unset value returns 0.

### `float resolveSelfV(float selfW, float selfH) const`

Same as `resolveSelfH` but for vertical self-resolution.

### Resolution table

| Method | `%` uses | `PW` uses | `PH` uses | `VW` / `VH` |
|--------|----------|-----------|-----------|-------------|
| `resolveH` | parentW | parentW | parentH | viewport |
| `resolveV` | parentH | parentW | parentH | viewport |
| `resolveSelfH` | selfW | selfW | selfH | viewport |
| `resolveSelfV` | selfH | selfW | selfH | viewport |

---

## 5. Arithmetic

### `operator+` — sum of terms

```cpp
Value c = Px(10) + Percent(50);   // two terms
```

Terms are concatenated and normalized (same-unit coefficients are
combined).

### `operator-` — difference

```cpp
Value c = PW(100) - PH(80);
```

Same as `+` with negated coefficients on the right.

### Unary `operator-`

```cpp
Value neg = -Px(10);
```

### `operator*` and `operator/` — scalar

```cpp
Value v = Percent(50) * 2.0f;   // Percent(100)
Value w = Px(10) / 2.0f;        // Px(5)
```

Scalar multiplication scales every coefficient. Scalar division by a
non-zero value is equivalent to multiplying by the reciprocal.

### `mulWith(const Value& o)`

Multiplication by another value. Returns `o * this.coeff` if `this` is
a single number, or `this * o.coeff` if `o` is a single number,
otherwise returns `this` (multiplication by a non-number is a no-op).
Used inside `calc()` for `*`.

### `divWith(const Value& o)`

Division by another value. Only defined when `o` is a single number
(division by a unit is a no-op, returning `this`). Used inside
`calc()` for `/`.

### `normalize()`

Combines terms with the same unit into a single term, then removes
zero-coefficient terms. Called automatically by `operator+` and
`operator-`. Idempotent.

Example:

```cpp
Value a = Px(5) + Px(10) + Px(3);
a.normalize();
// a is now Px(18) (one term)
```

---

## 6. Comparison

### `bool operator==(const Value& o) const`

True if the two values have the same terms in the same order. Assumes
both are normalized.

```cpp
Px(10) == Px(10)                            // true
Px(10) == Px(11)                            // false
(Px(5) + Px(5)) == Px(10)                   // true (after normalize)
(PW(100) - PH(80)) == (PW(100) - PH(80))    // true
```

### `bool operator!=(const Value& o) const`

Negation of `==`.

---

## 7. Common patterns

### Setting an explicit size

```cpp
node->size(Px(100), Px(50));
node->size(Percent(100), Auto());
node->size(VW(30), VH(6));
```

### Reading a resolved value

```cpp
const ComputedStyle& style = node->getStyle();
float w = style.width.resolveH(parentW, parentH);
```

### Combining units in `calc()`

```cpp
auto v = parseValueToken("calc(100% - 20px)").value();
float w = v.resolveH(parentW, parentH);
```

### `%` vs `pw` / `ph`

```css
/* width: 50% of parent's width */
width: 50%;

/* height: 50% of parent's width (regardless of property axis) */
height: 50pw;

/* top: 50% of parent's height */
top: 50%;

/* left: 50% of parent's height (unusual but valid) */
left: 50ph;
```

The `pw` / `ph` units are useful when you want a value to scale with a
specific axis of the parent regardless of which property it's set on.
The canonical example is the toggle knob:

```css
Toggle:checked .toggle-knob {
    left: calc(100pw - 80ph);
}
```

`100pw` is the full parent width (where the knob must move to),
`80ph` is a fixed offset based on the parent's height (the knob's
size + margins). Mixed units, resolved lazily.

---

## 8. Pitfalls

**Empty `Value` is `Auto`.**

```cpp
Value v;                // Auto
v.isAuto()              // true
```

An `Auto` value resolves to the `autoValue` argument of `resolveH` /
`resolveV` (default 0), or 0 for `resolveSelfH` / `resolveSelfV`. This
is what "let the layout engine decide" means.

**`std::clamp` with mixed-unit `Value`.**

`Value` doesn't have `operator<`. To compare two values numerically,
resolve them first:

```cpp
if (a.resolveH(w, h) < b.resolveH(w, h)) { ... }
```

**`Value` equality is structural, not numerical.**

```cpp
Px(10) == Px(10)           // true
Px(10) == Percent(50)      // false, even if the parent makes them equal
```

If you need to compare resolved values, resolve them first.

**`operator*` and `operator/` only accept scalars.**

```cpp
Px(10) * 2.0f              // ✅
Px(10) * Px(2)             // ❌ doesn't compile
```

To multiply two `Value`s, use `mulWith`. In practice, `calc()` handles
this automatically.

**`normalize` doesn't sort terms.**

The order of terms in the vector matters for `operator==`. Two values
built in different orders but with the same terms won't compare equal
unless both are normalized. `operator+` normalizes automatically;
manual construction doesn't.

To be safe, call `normalize()` before comparing manually-built values.

**`resolveH` / `resolveV` vs `resolveSelfH` / `resolveSelfV`.**

The "self" variants resolve `%` against the node's **own** size, which
is what `translate-x` / `translate-y` need. Other properties use the
parent-relative variants. Using the wrong one silently produces wrong
values — no error.

**`VW` / `VH` read `Metrics::viewport` at resolution time.**

The value is not cached. A `vw` value resolves to a different pixel
value before and after a viewport change. The style re-resolution on
viewport change (`viewportGeneration`) handles this for styled
properties, but a manually-resolved `Value` won't update until you
re-resolve it.

**`Unit::Auto` on a single term.**

```cpp
Value v = Value(10, Unit::Auto);   // empty, not a 10-value
```

The constructor treats `Unit::Auto` as "unset" and produces an empty
`Value`. Use `Value::autoSize()` for clarity.

**`calc()` with a trailing unit on the result.**

`calc(100% - 20px)` produces two terms (Percent and Pixel). `calc(100% / 2)`
produces a single `Percent(50)`. `calc(10px * 2)` produces a single
`Px(20)`. The result of `calc()` is always a valid `Value`, but the
number of terms depends on the operators used.

**`Value` in `BoxShadow` vs `BoxShadow::x` / `y` / `blur`.**

`BoxShadow` fields are `Value`, so a shadow offset can use any unit:

```css
box-shadow: 2vw 2vh 4px #00000080;
```

The resolution uses `resolveSelfH` / `resolveSelfV` in
`renderChrome`, meaning the offsets resolve against the node's own
size, not the parent's. This is usually what you want (a shadow scales
with its node), but it's different from other properties.

**`Value` with multiple `Percent` terms.**

After normalization, only one `Percent` term remains, with the sum of
all the original coefficients. So `Percent(30) + Percent(20)` becomes
`Percent(50)`, not two separate terms.

---

## 9. See also

- [Layout](../user/02-layout.md) — how units are used in practice.
- [ZStyle](../user/05-zstyle.md) — the `.zstyle` grammar for values,
  including `calc()`.
- [Layout Engine](../internals/05-layout-engine.md) — where `resolveH`
  and `resolveV` are called.
- [Style](Style.md) — the `Style` and `ComputedStyle` structs that hold
  `Value` fields.