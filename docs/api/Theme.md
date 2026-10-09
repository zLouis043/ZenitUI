# `Theme` — API Reference

The global stylesheet: a list of rules, a root style, and a map of
keyframes. Populated by the `.zstyle` parser, read by `StyleResolver`
during style resolution.

- **Header**: `Theme.hpp`
- **Namespace**: `ZenitUI`

For the cascade and specificity, see
[Style System](../internals/03-style-system.md). For the `.zstyle`
grammar, see [ZStyle](../user/05-zstyle.md).

---

## 1. `Specificity`

```cpp
struct Specificity {
    int ids{0};
    int classes{0};
    int tags{0};

    bool operator<(const Specificity& o) const;
    bool operator==(const Specificity& o) const;
    bool operator!=(const Specificity& o) const;
};
```

A triple of counters, compared **lexicographically**: `(ids, classes,
tags)`. So one id beats any number of classes, one class beats any
number of tags.

### `computeSpecificity(chain)`

```cpp
inline Specificity computeSpecificity(
    const std::vector<SimpleSelector>& chain);
```

Walks a selector chain and accumulates:

- `Id` → `ids++`.
- `Class` → `classes++`.
- `Tag` → `tags++`.
- Any state flag on the selector (`:hover`, `:pressed`, `:focus`,
  `:disabled`, `:checked`) → `classes++`.

**Example:**

```cpp
auto s1 = computeSpecificity({ tagSel("Toggle") });
// (0, 0, 1)

auto s2 = computeSpecificity({ classSel("btn-primary") });
// (0, 1, 0)

auto s3 = computeSpecificity({ idSel("save") });
// (1, 0, 0)

auto s4 = computeSpecificity({ tagSel("Toggle", {.requireChecked = true}) });
// (0, 1, 1)
```

### Comparison

```cpp
Specificity a{0, 1, 0};  // one class
Specificity b{0, 0, 5};  // five tags

a < b  // false (0,1,0) > (0,0,5) because classes beat tags
```

The comparison is:

```cpp
if (ids != o.ids) return ids < o.ids;
if (classes != o.classes) return classes < o.classes;
return tags < o.tags;
```

So higher specificity is "greater" — the sort in `resolveFor` uses
ascending order, so the last (highest) wins.

---

## 2. `ThemeRule`

```cpp
struct ThemeRule {
    std::vector<SimpleSelector> chain;
    std::string part;           // "track", "knob", etc. Empty for regular rules.
    Style style;
    Specificity specificity;
    int order{0};
    std::optional<MediaQuery> media;
};
```

One rule from a `.zstyle` file.

### Fields

| Field | Description |
|-------|-------------|
| `chain` | Descendant selector, right-to-left. `chain.back()` matches the node itself. |
| `part` | Non-empty for `::part` rules. Empty for regular rules. |
| `style` | The declarations from the rule's `{ ... }` block. |
| `specificity` | Computed from `chain` when the rule is added to the `Theme`. |
| `order` | Declaration order (0-based index in the `Theme`). Used as tie-break. |
| `media` | Optional media query. If set, the rule only matches when the query is satisfied. |

### Chain semantics

A selector like `.card Button.btn-primary` produces a chain:

```cpp
[
    { kind: Class, name: "card" },
    { kind: Tag,   name: "Button" },
    { kind: Class, name: "btn-primary" }
]
```

The rightmost entry (`btn-primary`) matches the node itself. The
earlier entries match ancestors.

A `::part` rule's chain is the selector **before** the `::`:

```css
Slider:hover::knob { ... }
```

produces:

```cpp
chain = [ { Tag, "Slider", requireHover = true } ]
part  = "knob"
```

The part name is stored separately, and `chain` refers to the widget
node (not the part).

---

## 3. `Theme`

```cpp
class Theme {
public:
    std::vector<ThemeRule> rules;
    Style root;
    std::unordered_map<std::string, KeyframeAnimation> keyframes;

    static Theme& get();

    void addRule(ThemeRule r);
    void addKeyframes(const KeyframeAnimation& anim);
    void clear();
};
```

A singleton holding the entire stylesheet.

### `get()`

```cpp
static Theme& get();
```

Returns the global instance. Thread-safe initialization via C++11
magic statics.

### `rules`

Ordered list of all rules. The index in this vector is the rule's
`order`. When two rules have the same specificity, the one with the
larger `order` wins.

Rules are stored in the order they were declared, across all
`loadStyleFile` / `loadStyleString` calls. Loading a new stylesheet
appends to the end.

### `root`

The `:root { ... }` block. Serves three purposes:

1. **Base style for non-inherited properties.** In `ComputedStyle::from`,
   if a property isn't set in the node's style or inherited from the
   parent, the root's value is used (if set).
2. **Base custom properties.** A node's `customProps` start from the
   parent's (or the root's, if no parent) and merge with the node's own.
3. **Typography defaults.** Since `font`, `color`, `letterSpacing`, and
   `fontSize` are inherited, setting them on `:root` propagates to every
   node unless overridden.

Example:

```css
:root {
    --primary: #0079F1;
    font: calibri;
    font-size: 20px;
    color: #FFFFFF;
}
```

### `keyframes`

Map of keyframe name → `KeyframeAnimation`. Populated by `@keyframes`
blocks. The `animation:` shorthand references names in this map.

Duplicate names: the later definition overwrites the earlier one. If
two stylesheets both define `pulse`, the second wins.

### `addRule(ThemeRule r)`

```cpp
void Theme::addRule(ThemeRule r) {
    r.specificity = computeSpecificity(r.chain);
    r.order = (int)rules.size();
    rules.push_back(std::move(r));
}
```

Fills in `specificity` and `order`, then appends. The caller doesn't
need to compute these.

### `addKeyframes(anim)`

```cpp
void Theme::addKeyframes(const KeyframeAnimation& anim) {
    keyframes[anim.name] = anim;
}
```

Stores (or overwrites) a keyframe animation by name.

### `clear()`

```cpp
void Theme::clear() {
    rules.clear();
    keyframes.clear();
    root = Style{};
}
```

Removes all rules, keyframes, and resets the root. The `Theme` is back
to empty.

Used primarily in tests (`Env::reset`) and for full reloading of the
stylesheet.

---

## 4. `MediaQuery` and `MediaCondition`

Defined in `Media.hpp`, but used by `ThemeRule`.

### `MediaCondition`

```cpp
struct MediaCondition {
    enum class Kind {
        MinWidth,
        MaxWidth,
        MinHeight,
        MaxHeight,
        OrientationLandscape,
        OrientationPortrait,
        MinAspectRatio,
        MaxAspectRatio
    };
    Kind  kind;
    float value{0.0f};   // not used for orientation
};
```

### `MediaQuery`

```cpp
struct MediaQuery {
    std::vector<MediaCondition> conditions;
    bool empty() const { return conditions.empty(); }
};
```

A list of conditions joined by `and`. An empty query is always true.

### `evaluateMedia(q, viewport)`

```cpp
inline bool evaluateMedia(const MediaQuery& q, Vec2 viewport);
```

Evaluates all conditions against the given viewport. Returns true if
all pass.

| Condition | Passes when |
|-----------|-------------|
| `MinWidth` | `viewport.x >= value` |
| `MaxWidth` | `viewport.x <= value` |
| `MinHeight` | `viewport.y >= value` |
| `MaxHeight` | `viewport.y <= value` |
| `OrientationLandscape` | `viewport.x >= viewport.y` |
| `OrientationPortrait` | `viewport.x < viewport.y` |
| `MinAspectRatio` | `viewport.x / viewport.y >= value` |
| `MaxAspectRatio` | `viewport.x / viewport.y <= value` |

The viewport is `Metrics::viewport` (logical pixels), not the physical
window size.

### Example

```css
@media (min-width: 900px) and (orientation: landscape) {
    .card { padding: 2vh 2vw; }
}
```

Produces:

```cpp
MediaQuery {
    conditions: [
        { MinWidth, 900.0 },
        { OrientationLandscape, 0.0 }
    ]
}
```

---

## 5. How `Theme` is populated

The theme is populated by the parser in `StyleParser.hpp`. The
parser reads a `.zstyle` file (or string) and calls
`applyStyleSheet`, which:

1. For each `:root` declaration, `applyStyleDeclaration(theme.root, ...)`.
2. For each rule, build a `ThemeRule`, apply its declarations, and call
   `theme.addRule(...)`.
3. For each `@keyframes`, build a `KeyframeAnimation` (parsing frames,
   normalizing), and call `theme.addKeyframes(...)`.

The public entry points are:

```cpp
// Parse a string
ZMarkup::loadStyleString(src, theme = Theme::get(), fileName = "<string>");

// Parse a file
ZMarkup::loadStyleFile(path, theme = Theme::get());
```

Both return `bool` (true on success). Errors (missing file, parse
failures on individual rules) are logged via `Logger` but don't abort
the whole parse.

### Example

```cpp
ZMarkup::loadStyleFile("assets/game.zstyle");
```

Reads the file, parses it, and appends every rule, keyframe, and root
declaration to `Theme::get()`.

To load into a custom `Theme` (for tests):

```cpp
Theme t;
ZMarkup::loadStyleString(R"(
    Button { background: red; }
)", t);
```

---

## 6. Rule ordering and specificity

The cascade uses three criteria, in order:

1. **Inline styles** (from `getInlineBase()`, ZMarkup attributes).
   These win unconditionally, regardless of specificity.
2. **Specificity** (higher wins).
3. **Declaration order** (later wins) — only used as a tie-break when
   specificity is equal.

So a rule with lower specificity but later declaration **does not**
override a rule with higher specificity. Only same-specificity rules
tie-break by order.

### Example

```css
Button { color: red; }              /* (0,0,1), order 0 */
.btn-primary { color: blue; }       /* (0,1,0), order 1 */
Button.btn-primary { color: green; }/* (0,1,1), order 2 */
#save { color: yellow; }            /* (1,0,0), order 3 */
```

Applied to `<Button class="btn-primary" id="save">`:

- All four match.
- Sorted by specificity: `(0,0,1)`, `(0,1,0)`, `(0,1,1)`, `(1,0,0)`.
- The highest `(1,0,0)` wins → `yellow`.

If `#save` weren't there, `(0,1,1)` wins → `green`.

If only the first two were there, `(0,1,0)` wins → `blue`, even though
it's declared after `Button`.

### Same specificity, different order

```css
Button { color: red; }
Button { color: blue; }
```

Both are `(0,0,1)`. The second has higher `order`, so it wins → `blue`.

### `::part` rules

`::part` rules use the same specificity computation on the `chain`
(which excludes the part name). But they're stored in `Theme::rules`
alongside normal rules, distinguished by `part != ""`.

The `resolveFor` method skips them (`if (!r.part.empty()) continue;`).
The `partFor` method filters for `r.part == partName`.

This means a `::part` rule and a normal rule with the same chain and
specificity don't compete — they're applied to different targets.

---

## 7. Keyframe storage

`Theme::keyframes` is a map from name to `KeyframeAnimation`. The map
is populated by the parser's `applyStyleSheet`:

```cpp
for (auto& kf : sheet.keyframes) {
    KeyframeAnimation anim;
    anim.name = kf.name;
    for (auto& frame : kf.frames) {
        float t = 0.0f;
        if (frame.ts == "from") t = 0.0f;
        else if (frame.ts == "to") t = 1.0f;
        else if (!frame.ts.empty() && frame.ts.back() == '%')
            t = std::stof(frame.ts.substr(0, frame.ts.size() - 1)) / 100.0f;

        Keyframe k;
        k.t = t;
        for (auto& d : frame.decls)
            applyStyleDeclaration(k.delta, d.prop, d.value, loc);
        anim.keyframes.push_back(std::move(k));
    }
    std::sort(anim.keyframes.begin(), anim.keyframes.end(),
              [](const Keyframe& a, const Keyframe& b) { return a.t < b.t; });
    normalizeKeyframes(anim, kfLoc);
    theme.addKeyframes(anim);
}
```

Steps:

1. Parse each frame's timestamp (`from`, `to`, `0%`…`100%`).
2. Parse each declaration into the frame's `Style delta`.
3. Sort frames by `t`.
4. Normalize (auto-fill missing properties).
5. Store in `theme.keyframes[name]`.

### `KeyframeAnimation`

```cpp
struct KeyframeAnimation {
    std::string name;
    std::vector<Keyframe> keyframes;
};

struct Keyframe {
    float t{0.0f};
    Style delta;
};
```

Each frame has a normalized time `t ∈ [0, 1]` and a `Style` with the
**deltas** (not the full style — only the properties that change at
that keyframe).

The evaluator (`evaluateKeyframes` in `AnimPrimitives.hpp`) interpolates
between the two keyframes surrounding the current time.

### Auto-fill

`normalizeKeyframes` fills missing properties from the nearest
neighbor (previous frame first, then next). A warning is logged for
each auto-filled property per keyframe.

This is a convenience — the animation works without explicit values in
every frame — but the warnings nudge you to be explicit.

---

## 8. Common tasks

### Clearing the theme between tests

```cpp
Theme::get().clear();
```

Resets everything. Any node that has already resolved its style with
the old theme keeps its cached style until the next re-resolve (which
happens when the node's `pendingTransition` is set — e.g. by a state
change or by `beginTransition()`).

In tests, use `Env::reset()` which calls `Theme::clear()` and resets
the `UIContext`.

### Loading multiple stylesheets

```cpp
ZMarkup::loadStyleFile("assets/base.zstyle");
ZMarkup::loadStyleFile("assets/game.zstyle");
ZMarkup::loadStyleFile("assets/settings.zstyle");
```

Rules accumulate. The second file's rules have higher `order` than the
first's, so at equal specificity, later files win.

There's no unload — to reload, `clear()` first.

### Inspecting rules

```cpp
auto& theme = Theme::get();
for (const auto& r : theme.rules) {
    std::printf("%s  spec=(%d,%d,%d)\n",
        selectorToString(r).c_str(),
        r.specificity.ids,
        r.specificity.classes,
        r.specificity.tags);
}
```

Or use `Debug::dumpTheme()` (see [Debugging](../internals/10-debug.md)).

### Adding a rule programmatically

```cpp
ThemeRule rule;
rule.chain = { /* SimpleSelector for "Button" */ };
rule.style.background = Colors::Red;
Theme::get().addRule(std::move(rule));
```

Rarely needed — the parser handles the normal case. Programmatic rules
are useful for generated styles or tests.

### Counting keyframes

```cpp
size_t n = Theme::get().keyframes.size();
```

---

## 9. Pitfalls

**`Theme` is a singleton.**

There's only one `Theme::get()`. Loading a stylesheet always loads into
the global theme (unless you pass a custom one to `loadStyleString`).
This means tests can pollute each other if they don't `clear()`.

**Rules are never removed.**

Once added, a rule stays until `clear()` is called. There's no
`removeRule` or `unloadStylesheet`. To reload, clear and re-parse.

**`order` is not re-assigned after removal.**

There's no removal, so `order` is monotonic: 0, 1, 2, ... as rules are
added.

**Media queries are evaluated at rule-match time, every frame.**

`ruleMatches` calls `evaluateMedia(*r.media, Metrics::viewport)`. This
happens every time a node's style is resolved. For a node with many
matching rules and many media queries, this is a small per-node cost.

The `viewportGeneration` counter ensures the style is re-resolved on
viewport change, but there's no caching of "which rules match given a
viewport" — each resolution recomputes.

**`addRule` overwrites `specificity` and `order`.**

If you set them manually before calling `addRule`, they're overwritten.
The `addRule` function is the authority.

**Specificity counts `:checked` as a class.**

A selector like `Toggle:checked` has specificity `(0, 1, 1)` — one
class-equivalent for the state, one tag for `Toggle`. This is different
from pure CSS, where `:checked` has class specificity. It's the same
effect.

**Keyframe names are global.**

There's no namespacing. A `@keyframes pulse` in one stylesheet and
another in a second stylesheet: the second overrides the first. If you
need namespacing, prefix names (`"game_pulse"`, `"settings_pulse"`).

**`Theme::root` is not a `ThemeRule`.**

The root style is stored separately. It has no specificity and no
order — it's the lowest-priority source for non-inherited properties,
and the base for custom properties.

Because `:root` rules don't compete with other rules, you can't write
`:root { background: red; }` and expect it to override
`Button { background: blue; }` — the root's background is only used
for a node that has no other background source.

**Media queries don't support `or`, `not`, or comma.**

Only `and` between conditions. To express "wide OR tall", use two rules:

```css
@media (min-width: 900px) {
    .card { ... }
}
@media (min-height: 900px) {
    .card { ... }
}
```

The rules will both match if both conditions are true, but the CSS
cascade handles the duplicate application (last one wins for the same
property).

**`evaluateMedia` uses `Metrics::viewport`, not the window size.**

On HiDPI displays where the logical viewport is smaller than the
physical window, the media queries evaluate against the logical size.
This is usually what you want, but it means a `@media (min-width:
1920px)` won't trigger on a 1920-physical-pixel window if the DPI
scale is 2.0 (logical width would be 960).

**Rules with the same specificity and different states.**

```css
Button:hover { background: red; }
Button:pressed { background: blue; }
```

When a button is pressed, both rules match (since `isHovered` is
usually true during a press). Specificity is equal: `(0, 1, 1)` for
both. The order decides — the later rule wins. So if `:pressed` is
declared after `:hover`, `blue` wins during the press.

This is the standard CSS behavior and matches what users expect.

**Custom properties in `:root` don't override inherited values from a
parent.**

Custom props inherit. If a parent has `--primary: red` (set via a
`.card { --primary: red; }` rule that matched the parent), a child
inside it inherits `--primary: red`, even if `:root` has
`--primary: blue`. Local wins.

To "reset" a custom prop to the root value, you'd need to re-declare
it explicitly:

```css
.reset { --primary: var(--primary-from-root); }
```

Or use a different name.

**`Theme::keyframes` uses `std::unordered_map`, so iteration order is
unspecified.**

`Debug::dumpTheme` iterates the map for the keyframe summary. The
order of keyframes in the dump is not the order of declaration. If you
need stable iteration, sort the names.

**`applyStyleSheet` doesn't sort rules.**

Rules are added in file order (with multiple selectors from a comma
list being added in order). The sorting happens at match time in
`resolveFor`, which sorts the matching subset. So the `Theme::rules`
vector itself is not sorted by specificity.

This means `Debug::dumpTheme` shows rules in declaration order, not
specificity order. If you're debugging specificity, look at the
`spec=` line for each rule.

**Adding rules after the first frame doesn't re-resolve existing nodes.**

A node's style is resolved on demand (when `pendingTransition` is true,
or on state change). Adding a new rule to `Theme` doesn't
retroactively affect nodes. To force a re-resolve, call
`beginTransition()` on the node, or trigger a state change.

The common pattern is to load all stylesheets **before** the first
frame. Dynamic style changes are rare.

**`selectorToString` for `::part` rules shows the part name.**

`Debug::selectorToString(rule)` appends `::part` if `rule.part` is
non-empty. So a `::part` rule is printed with its `::` suffix, making
it easy to spot in `dumpTheme`.

**The `chain` order in `dumpTheme` output is left-to-right.**

A `.card Button` rule is printed as `.card Button`, matching the source.
But the internal `chain` vector stores them in the same left-to-right
order — `chain[0] = .card`, `chain[1] = Button`. The matching algorithm
uses `chain.back()` for the node itself.

---

## 10. See also

- [Style System](../internals/03-style-system.md) — how rules are
  matched and sorted.
- [ZStyle](../user/05-zstyle.md) — the `.zstyle` grammar.
- [Style](Style.md) — the `Style` struct that lives inside rules.
- [Debugging](../internals/10-debug.md) — `dumpTheme` for inspection.