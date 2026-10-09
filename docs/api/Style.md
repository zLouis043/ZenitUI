# `Style` — API Reference

The style data model. Three structs (`Style`, `ComputedStyle`, and the
helpers), plus the supporting types used inside them.

- **Header**: `Style.hpp`
- **Namespace**: `ZenitUI`

For the cascade and transitions, see
[Style System](../internals/03-style-system.md). For the `.zstyle`
grammar, see [ZStyle](../user/05-zstyle.md).

---

## 1. `Opt<T>` — optional property wrapper

```cpp
template <typename T>
struct Opt {
    T    value{};
    bool is_set{false};

    Opt() = default;
    Opt(const T& v);
    Opt& operator=(const T& v);
    void reset();
    T get_or(const T& fallback) const;
};
```

Every style property is an `Opt<T>`. `is_set` distinguishes "not
declared" from "declared to the default value". This is what makes the
cascade work: an unset property doesn't override a lower-priority one.

### Methods

| Method | Description |
|--------|-------------|
| `Opt(const T&)` | Constructs a set `Opt`. |
| `operator=(const T&)` | Assigns and sets `is_set = true`. |
| `reset()` | Sets `is_set = false`. The value is unchanged but ignored. |
| `get_or(fallback)` | Returns `value` if `is_set`, else `fallback`. |

**Note:** `Opt<T>` has no `operator bool`. Use `is_set` directly.

---

## 2. `BUBBLE_STYLE_PROPS` — the property macro

All properties of `Style` and `ComputedStyle` are defined by one macro:

```cpp
#define BUBBLE_STYLE_PROPS(X)                               \
    X(Value, width, Value::autoSize())                      \
    X(Value, height, Value::autoSize())                     \
    X(Value, minWidth, Value::autoSize())                   \
    X(Value, minHeight, Value::autoSize())                  \
    X(Value, maxWidth, Value::autoSize())                   \
    X(Value, maxHeight, Value::autoSize())                  \
    X(float, grow, 0.0f)                                    \
    X(float, shrink, 1.0f)                                  \
    X(Value, gap, Value(0.0f))                              \
    X(Spacing, margin, Spacing())                           \
    X(Spacing, padding, Spacing())                          \
    X(Align, alignH, Align::Auto)                           \
    X(Align, alignV, Align::Auto)                           \
    X(Align, itemsH, Align::Start)                          \
    X(Align, itemsV, Align::Start)                          \
    X(Justify, justify, Justify::Start)                     \
    X(Align, textAlign, Align::Auto)                        \
    X(Color, background, Colors::Blank)                     \
    X(Color, color, Colors::White)                          \
    X(Color, tint, Colors::White)                           \
    X(TextureRef, backgroundTexture, TextureRef{})          \
    X(BoxShadow, boxShadow, BoxShadow{})                    \
    X(std::string, effect, "")                              \
    X(std::vector<FilterRef>, filters, {})                  \
    X(Color, borderColor, Colors::Blank)                    \
    X(Value, borderWidth, Value(0.0f))                      \
    X(Value, radius, Value(0.0f))                           \
    X(float, opacity, 1.0f)                                 \
    X(float, scale, 1.0f)                                   \
    X(float, rotation, 0.0f)                                \
    X(Value, translateX, Value(0.0f))                       \
    X(Value, translateY, Value(0.0f))                       \
    X(Value, fontSize, Value(20.0f))                        \
    X(Value, letterSpacing, Value(2.0f))                    \
    X(float, transitionTime, 0.15f)                         \
    X(TransitionFunction, ease, TransitionFunction::Linear) \
    X(Overflow, overflowX, Overflow::Visible)               \
    X(Overflow, overflowY, Overflow::Visible)               \
    X(Position, position, Position::Static)                 \
    X(ZIndex, zIndex, ZIndex::Auto())                       \
    X(Value, top, Value::autoSize())                        \
    X(Value, left, Value::autoSize())                       \
    X(Value, right, Value::autoSize())                      \
    X(Value, bottom, Value::autoSize())                     \
    X(std::string, font, "")
```

Every operation on `Style` and `ComputedStyle` (`overlay`, `operator==`,
`lerp`, `stylesDiffer`, `forEachSetStyleProp`, `propTable`, etc.) uses
this macro. Adding a property means editing this macro and the parser
(`applyStyleAttr` in `StyleAttr.hpp`).

### Property list (quick reference)

| Group | Properties |
|-------|-----------|
| Sizing | `width`, `height`, `minWidth`, `minHeight`, `maxWidth`, `maxHeight` |
| Flex | `grow`, `shrink`, `gap` |
| Spacing | `margin`, `padding` |
| Alignment | `alignH`, `alignV`, `itemsH`, `itemsV`, `justify`, `textAlign` |
| Visual | `background`, `color`, `tint`, `backgroundTexture`, `boxShadow`, `borderColor`, `borderWidth`, `radius`, `opacity` |
| Effects | `effect`, `filters` |
| Transform | `scale`, `rotation`, `translateX`, `translateY` |
| Typography | `fontSize`, `letterSpacing`, `font` |
| Transition | `transitionTime`, `ease` |
| Overflow / position | `overflowX`, `overflowY`, `position`, `zIndex`, `top`, `left`, `right`, `bottom` |

All property names use **camelCase** in C++ and **kebab-case** in
`.zstyle` (`borderWidth` ↔ `border-width`).

---

## 3. `Style` — the declared style

```cpp
struct Style {
    #define X(T, name, def) Opt<T> name;
    BUBBLE_STYLE_PROPS(X)
    #undef X

    Opt<std::vector<TransitionSpec>> transitions;
    Opt<std::vector<AnimationRef>>   animations;

    std::unordered_map<std::string, std::string> customProps;
    std::unordered_map<std::string, std::string> unresolvedProps;

    Style& overlay(const Style& o);
};
```

Every property is an `Opt<T>`. Plus:

- **`transitions`** — the parsed `transition:` shorthand.
- **`animations`** — the parsed `animation:` shorthand.
- **`customProps`** — custom properties (`--name`) stored as raw
  strings.
- **`unresolvedProps`** — properties whose values contained `var()`
  and couldn't be typed at parse time.

### `overlay(const Style& o)`

Copies every **set** property from `o` into `*this`. Unset properties
in `o` don't change `*this`. This is the core cascade operation.

```cpp
Style a;
a.background = Colors::Red;
a.color = Colors::White;

Style b;
b.background = Colors::Blue;   // only background

a.overlay(b);
// a.background == Blue, a.color == White
```

For `customProps` and `unresolvedProps`, the maps are merged
(overlay wins on conflicts). For `transitions` and `animations`, the
whole vector is replaced if `o`'s is set.

**Special behavior for `unresolvedProps`:** when an overlay brings an
unresolved prop, `clearStyleProp` is called on the same name in `*this`
to clear any typed value. This ensures a typed value from a lower-
priority source doesn't win over a `var()`-containing value from a
higher-priority one.

---

## 4. `ComputedStyle` — the resolved style

```cpp
struct ComputedStyle {
    #define X(T, name, def) T name = def;
    BUBBLE_STYLE_PROPS(X)
    #undef X

    std::vector<TransitionSpec> transitions;
    std::vector<AnimationRef>   animations;
    std::unordered_map<std::string, std::string> customProps;

    static ComputedStyle from(const Style& s,
                              const ComputedStyle* parent = nullptr,
                              const Style* root = nullptr);
};
```

Same property list, but each field is a concrete value (no `Opt`). Every
field has a default, so a `ComputedStyle` is always fully populated.

### `from(s, parent, root)`

Resolves a `Style` into a `ComputedStyle`. Priority:

1. If `s.name.is_set`, use it.
2. Else if the property is inherited and `parent` exists, use
   `parent->name`.
3. Else if `root` has the property set, use `root->name.value`.
4. Else use the default.

Plus the **overflow computed-value rule**:

```cpp
if (c.overflowX == Overflow::Visible && c.overflowY != Overflow::Visible)
    c.overflowX = Overflow::Auto;
else if (c.overflowY == Overflow::Visible && c.overflowX != Overflow::Visible)
    c.overflowY = Overflow::Auto;
```

If one axis is `Visible` and the other isn't, the `Visible` axis is
promoted to `Auto`. This makes clipping behavior predictable.

`customProps` inherit from the parent, then merge with the node's own
(winning on conflict).

### Inheritance

Only these properties are inherited:

| Property | Reason |
|----------|--------|
| `font` | Text typography cascades. |
| `fontSize` | Same. |
| `color` | Same. |
| `letterSpacing` | Same. |
| `textAlign` | Same. |

The check is `isInheritedProp(name)` in `Style.hpp`:

```cpp
inline bool isInheritedProp(std::string_view name) {
    return name == "font" || name == "fontSize" || name == "color"
        || name == "letterSpacing" || name == "textAlign";
}
```

Everything else uses the local value or the default — no inheritance.

### `operator==` / `operator!=`

Compares every field. Used by `Layout::recomputeDirty` to detect style
changes.

```cpp
inline bool operator==(const ComputedStyle& a, const ComputedStyle& b);
inline bool operator!=(const ComputedStyle& a, const ComputedStyle& b);
```

---

## 5. `Spacing`

```cpp
struct Spacing {
    Value top, right, bottom, left;

    Spacing();
    Spacing(float all);
    Spacing(Value all);
    Spacing(Value v, Value h);
    Spacing(Value v, Value h, Value b);
    Spacing(Value t, Value r, Value b, Value l);

    bool operator==(const Spacing& o) const;
    bool operator!=(const Spacing& o) const;
};
```

The four-side value used by `padding` and `margin`. Follows CSS syntax:

| Constructor | Meaning |
|-------------|---------|
| `Spacing(v)` | All sides = `v`. |
| `Spacing(v, h)` | Vertical = `v`, horizontal = `h`. |
| `Spacing(v, h, b)` | Top = `v`, horizontal = `h`, bottom = `b`. |
| `Spacing(t, r, b, l)` | Top, right, bottom, left. |

**Note:** these are constructed programmatically. In `.zstyle`, spacing
is parsed by `parseSpacingToken`, which produces the same result from
1 / 2 / 3 / 4 whitespace-separated values.

---

## 6. `TransitionSpec`

```cpp
struct TransitionSpec {
    std::string        prop;           // property name, or "all"
    float              duration{0.15f}; // seconds
    TransitionFunction ease{TransitionFunction::Linear};
    float              delay{0.0f};    // seconds
};
```

One entry in `Style::transitions`. A `transition:` shorthand produces
one `TransitionSpec` per comma-separated item.

See [Animations](../user/07-animations.md) for the CSS semantics.

---

## 7. `AnimationRef`

```cpp
struct AnimationRef {
    std::string        name;           // keyframe name
    float              duration{1.0f}; // seconds
    TransitionFunction ease{TransitionFunction::Linear};
    float              delay{0.0f};    // seconds
    int                iterations{1};  // -1 for infinite
    bool               alternate{false};
    bool               fillForwards{false};
    bool               blocksInput{false};
};
```

One entry in `Style::animations`. An `animation:` shorthand produces
one `AnimationRef` per comma-separated item.

---

## 8. `Position`, `ZIndex`, `Overflow`

### `Position`

```cpp
enum class Position {
    Static,     // in flow, no z-index stacking
    Relative,   // in flow, can have z-index
    Absolute    // out of flow, positioned by top/left/right/bottom
};
```

`Relative` currently behaves like `Static` for layout purposes (only
affects stacking). See [Layout §7](../user/02-layout.md#7-absolute-positioning).

### `ZIndex`

```cpp
struct ZIndex {
    bool isAuto{true};
    int  value{0};

    ZIndex() = default;
    ZIndex(int v);
    static ZIndex Auto();

    bool operator==(const ZIndex& o) const;
    bool operator!=(const ZIndex& o) const;
};
```

`isAuto = true` means "no explicit z-index" — the node participates in
its parent's normal flow ordering. `isAuto = false` with `value` set
means the node is a stacking context (if `position != Static`).

### `Overflow`

```cpp
enum class Overflow {
    Visible,   // no clipping, no scrolling
    Hidden,    // clipping, no scrolling
    Scroll,    // clipping, scrollbar always visible
    Auto       // clipping, scrollbar only when needed
};
```

See [Layout §8](../user/02-layout.md#8-overflow).

---

## 9. `TextureRef`

```cpp
struct TextureRef {
    std::string name;
    int left{0}, top{0}, right{0}, bottom{0};

    bool isNineSlice() const;
    bool operator==(const TextureRef& o) const;
    bool operator!=(const TextureRef& o) const;
};
```

A background texture reference. `name` is the asset name. The four
integers are nine-slice insets; if all zero, `isNineSlice()` returns
false and the texture is stretched.

Set in `.zstyle`:

```css
background-texture: bubble;               /* stretch */
background-texture: npatches 16 16 16 16; /* nine-slice */
```

---

## 10. `SimpleSelector`

```cpp
struct SimpleSelector {
    enum class Kind { Tag, Class, Id };
    std::string name;
    Kind kind{Kind::Tag};

    // Extra components of a compound selector (`Button.btn-primary`,
    // `.card.elevated`, `Button#save`). Each pair is (kind, name).
    // All components must match the same node.
    std::vector<std::pair<Kind, std::string>> extras;

    bool requireHover{false};
    bool requirePressed{false};
    bool requireFocus{false};
    bool requireDisabled{false};
    bool requireChecked{false};
};
```

One token of a selector chain. A `ThemeRule` holds a `chain` of these:
the rightmost entry matches the node itself, the earlier ones match
ancestors (descendant selectors).

A token can be a **compound selector**: a tag, any number of classes,
and an id, all on the same node. In that case `kind`/`name` hold the
first component and `extras` holds the rest. Examples:

| Source              | `kind` / `name`         | `extras`                                    |
|---------------------|-------------------------|---------------------------------------------|
| `Button`            | `Tag` / `"Button"`      | (empty)                                     |
| `.btn-primary`      | `Class` / `"btn-primary"` | (empty)                                   |
| `Button.btn-primary`| `Tag` / `"Button"`      | `[(Class, "btn-primary")]`                  |
| `.card.elevated`    | `Class` / `"card"`      | `[(Class, "elevated")]`                     |
| `Button.btn-primary#save` | `Tag` / `"Button"` | `[(Class, "btn-primary"), (Id, "save")]` |

States (`:hover`, `:checked`, …) always apply to the whole compound,
regardless of where they appear in the source.

---

## 11. `FilterRef`

```cpp
struct FilterRef {
    std::string name;
    std::vector<std::string> args;

    bool operator==(const FilterRef& o) const;
    bool operator!=(const FilterRef& o) const;
};

inline bool operator==(const std::vector<FilterRef>& a,
                       const std::vector<FilterRef>& b);
inline bool operator!=(const std::vector<FilterRef>& a,
                       const std::vector<FilterRef>& b);
```

One filter in a `filter:` list. `args` is the raw argument strings.

```css
filter: blur(4px), drop-shadow(2px, 2px, #000);
```

produces two `FilterRef`: `{name="blur", args={"4px"}}` and
`{name="drop-shadow", args={"2px", "2px", "#000"}}`.

---

## 12. `BoxShadow`

```cpp
struct BoxShadow {
    Value x{0.0f}, y{0.0f}, blur{0.0f};
    Color color{Colors::Blank};
    bool  enabled{false};

    bool operator==(const BoxShadow& o) const;
    bool operator!=(const BoxShadow& o) const;
};
```

A box-shadow definition. `enabled` is true if the property was declared.

```css
box-shadow: 4px 4px 8px #00000080;
```

is `{x=4px, y=4px, blur=8px, color={0,0,0,128}, enabled=true}`.

---

## 13. Property table

```cpp
using PropValue = std::variant<float, Value, Color, Spacing, Align, Justify,
                                TransitionFunction, Overflow, Position, ZIndex,
                                std::string, TextureRef, std::vector<FilterRef>,
                                BoxShadow>;

struct PropDesc {
    const char* name;
    PropValue (*get)(const ComputedStyle&);
    void (*set)(ComputedStyle&, const PropValue&);
};

inline const std::vector<PropDesc>& propTable();
inline const PropDesc* findProp(std::string_view name);
```

A runtime-discoverable table of all properties. Used by `lerpValue`,
and available for reflection / tooling.

`findProp("width")` returns a `PropDesc*` or null. The `get` / `set`
function pointers let you read or write a property by name.

---

## 14. Lerp functions

### Scalar and unit types

```cpp
float  lerpProp(float a, float b, float t);
Color  lerpProp(const Color& a, const Color& b, float t);
Value  lerpProp(const Value& a, const Value& b, float t);
Spacing lerpProp(const Spacing& a, const Spacing& b, float t);
```

Numeric interpolation with the easing applied upstream.

For `Value`, `auto` on either side causes a **snap** (returns `b` if
`t > 0`, else `a`). Mixed units are interpolated term-by-term,
including units present in only one side.

### Enum-like types (snap)

```cpp
template <typename E>
E lerpProp(E a, E b, float t);              // enums snap

ZIndex      lerpProp(const ZIndex& a,      const ZIndex& b,      float t);
std::string lerpProp(const std::string& a, const std::string& b, float t);
TextureRef  lerpProp(const TextureRef& a,  const TextureRef& b,  float t);
BoxShadow   lerpProp(const BoxShadow& a,   const BoxShadow& b,   float t);
std::vector<FilterRef> lerpProp(const std::vector<FilterRef>& a,
                                 const std::vector<FilterRef>& b,
                                 float t);
```

All return `t > 0 ? b : a` (snap at the first non-zero `t`).

### `lerpValue`

```cpp
PropValue lerpValue(const PropValue& a, const PropValue& b, float t);
```

Variant-dispatched lerp. If the two variants hold the same type, lerps;
otherwise returns `a`.

### `lerpStyle`, `lerpStyleTimed`, `lerpStyleParts`

```cpp
ComputedStyle lerpStyle(const ComputedStyle& a,
                        const ComputedStyle& b, float t);

ComputedStyle lerpStyleTimed(const ComputedStyle& a,
                             const ComputedStyle& b,
                             float realElapsed);

Style lerpStyleParts(const Style& a, const Style& b, float t);
```

Three variants:

- **`lerpStyle`** — global lerp with a pre-normalized `t`. All properties
  use the same `t`.
- **`lerpStyleTimed`** — per-property timing. Each property uses its own
  duration/ease from `b.transitions`, with `b.transitionTime` as
  fallback. `realElapsed` is the elapsed time in seconds.
- **`lerpStyleParts`** — for `Style` (not `ComputedStyle`). Uses
  `lerpOpt`, which **snaps** if either side is unset. Used by
  `StyleResolver::partFor`.

---

## 15. Style manipulation helpers

### `overlayComputed(dst, src)`

```cpp
inline void overlayComputed(ComputedStyle& dst, const Style& src);
```

Applies a `Style` on top of a `ComputedStyle`. Every set property in
`src` overwrites the corresponding field in `dst`.

Used by `Anim::overlayCssComputed` to apply CSS keyframe values to the
node's render style.

### `forEachSetStyleProp(s, fn)`

```cpp
template <typename F>
inline void forEachSetStyleProp(const Style& s, F&& fn);
```

Calls `fn(name)` for each property that's set. `name` is a `const char*`.

### `hasStyleProp(s, name)`

```cpp
inline bool hasStyleProp(const Style& s, std::string_view name);
```

True if the named property is set.

### `copyStyleProp(dst, src, name)`

```cpp
inline bool copyStyleProp(Style& dst, const Style& src, std::string_view name);
```

If `src.name` is set, copies it to `dst.name` and returns true. Else
returns false without touching `dst`.

### `clearStyleProp(s, name)`

```cpp
inline bool clearStyleProp(Style& s, std::string_view name);
```

Resets the named property (`is_set = false`). Returns true if the
property was recognized.

### `stylesDiffer(a, b)`

```cpp
inline bool stylesDiffer(const Style& a, const Style& b);
```

Compares two `Style`s, distinguishing "unset" from "set to default".
Returns true if any property is set in exactly one, or set to different
values in both. Also compares `animations`.

Used by `StyleResolver::partFor` to decide whether to start a part
transition.

---

## 16. Common patterns

### Reading a resolved property

```cpp
const ComputedStyle& style = node->getStyle();
float w = style.width.resolveH(parentW, parentH);
float rPx = style.radius.resolveH(rect.width, rect.height);
```

### Overlaying a style

```cpp
Style base;
base.background = Colors::Red;

Style override;
override.background = Colors::Blue;
override.radius = Px(8);

base.overlay(override);
// base.background == Blue, base.radius == 8px
```

### Applying a keyframe overlay

```cpp
ComputedStyle renderStyle = node->getStyle();
Anim::overlayCssComputed(renderStyle, node->anim_.css);
```

### Building a custom `Style` in a widget

```cpp
Style s;
s.background = Colors::DarkGray;
s.radius = Px(6);
s.padding = Spacing(Px(8));
node->setInlineBase(s);
```

### Iterating set properties (debugging)

```cpp
forEachSetStyleProp(style, [](const char* name) {
    std::printf("%s\n", name);
});
```

---

## 17. Pitfalls

**`overlay` doesn't clear unset properties.**

```cpp
Style a; a.background = Colors::Red;
Style b; // empty
a.overlay(b);
// a.background is still Red
```

To clear, use `clearStyleProp(a, "background")` or `reset()` on the
specific `Opt`.

**`ComputedStyle::from` uses the parent's value for inherited props
even if the parent has no explicit value.**

Because `ComputedStyle` is always fully populated, an inherited
property always comes from the parent (or root, or default) — never
from a "no value" state. This is correct for CSS but can surprise
you if you expected inheritance to be optional.

**`Value` lerp with `auto` snaps.**

```cpp
Value a = Auto();
Value b = Px(100);
lerpProp(a, b, 0.5f) == b   // snap
```

There's no interpolation between "no value" and a concrete value.

**`stylesDiffer` and `ComputedStyle::operator==` are not the same.**

- `stylesDiffer(a, b)` compares two `Style`s, considering "setness".
- `operator==(ComputedStyle, ComputedStyle)` compares two resolved
  styles, where every field has a value.

`stylesDiffer` is for part transitions (where unset means "fall back"),
`operator==` is for dirty tracking.

**`unresolvedProps` shadows typed values.**

When a `Style` has both `background` (typed) and
`unresolvedProps["background"]` (a `var()` expression), the overlay
rule clears the typed value. This is intentional: the `var()` is
higher priority. Don't set both manually unless you know what you're
doing.

**`TransitionSpec::prop` is a raw string.**

There's no enum. The comparison in `lerpStyleTimed` is a string compare
against the property name (`"opacity"`, `"all"`, etc.). Typos silently
fall through to the default timing.

**`propTable` is built once, on first call.**

The static table is initialized on first use. Thread-safety of the
initialization is guaranteed by C++11's magic statics, but the returned
reference is `const`, and the table is read-only after init.

**`BoxShadow` snaps under interpolation.**

`lerpProp(BoxShadow, BoxShadow, t)` returns `b` if `t > 0`, else `a`.
So a transition on `box-shadow` jumps to the target at the first
non-zero `t`. It's visually a hard cut, not an interpolation.

The same applies to `TextureRef`, `FilterRef` lists, `ZIndex`, enums,
and strings.

**`lerpProp(Value, Value, t)` can produce multi-term results.**

Interpolating `Px(0)` to `Percent(100)` at `t = 0.5` produces
`{0.5 Pixel, 50 Percent}` — a two-term value. This resolves fine but
has more terms than either input. For repeatedly-recomputed values
(e.g. in a transition tick), the term count stays bounded because the
lerp is between fixed endpoints, not accumulated.

**`customProps` are not part of `operator==`.**

`ComputedStyle::operator==` compares the macro-generated properties
only. Two styles that differ only in `customProps` compare equal. This
is a known limitation — custom properties are treated as metadata, not
as part of the style identity.

**`Style::overlay` merges `customProps` but not other maps.**

`customProps` and `unresolvedProps` are merged key-by-key. Everything
else (including `transitions` and `animations`) is replaced wholesale
when the source is set.

**`forEachSetStyleProp` doesn't iterate `transitions` / `animations`.**

The macro only covers the ~45 typed properties. The two vectors and the
custom props maps are skipped.

**`propTable` doesn't include `transitions` / `animations` / `customProps`.**

Same reason. The variant doesn't have a case for them.

---

## 18. See also

- [Style System](../internals/03-style-system.md) — the cascade in
  detail.
- [ZStyle](../user/05-zstyle.md) — the `.zstyle` grammar.
- [Unit](Unit.md) — the `Value` type used by many properties.
- [Theme](Theme.md) — `ThemeRule` and specificity.
- [Animations](../user/07-animations.md) — transitions and keyframes.