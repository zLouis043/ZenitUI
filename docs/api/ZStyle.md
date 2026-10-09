# `ZStyle` — API Reference

The `.zstyle` parser and its supporting utilities. This document is the
API reference for the parsing layer; for the grammar itself, see
[ZStyle (guide)](../user/05-zstyle.md).

- **Headers**: `StyleParser.hpp`, `StyleAttr.hpp`
- **Namespace**: `ZenitUI::ZMarkup` (parser) and `ZenitUI` (attribute parsers)

---

## 1. Overview

The `.zstyle` pipeline has three stages:

1. **Tokenize and parse** — read the source, extract rules, keyframes,
   and `:root` declarations into an intermediate `StyleSheet` AST.
2. **Convert** — walk the AST and produce `ThemeRule`, `Style`, and
   `KeyframeAnimation` objects.
3. **Apply** — append the results to a `Theme`.

The public entry points wrap all three:

```cpp
ZMarkup::loadStyleString(source, theme, fileName);
ZMarkup::loadStyleFile(path, theme);
```

Everything else is exposed for advanced use (custom parsing flows,
unit tests, tooling).

---

## 2. Public entry points

### `loadStyleString`

```cpp
inline bool loadStyleString(std::string_view src,
                            Theme& theme = Theme::get(),
                            const std::string& fileName = "<string>");
```

Parses `src` and applies it to `theme`. Returns `true` on success.

Parse errors are logged (via `Logger`) but don't abort the whole
parse — the offending rule is skipped, and the rest continues.

```cpp
ZMarkup::loadStyleString(R"(
    Button { background: red; }
    .btn-primary { background: blue; }
)");
```

### `loadStyleFile`

```cpp
inline bool loadStyleFile(const std::filesystem::path& path,
                          Theme& theme = Theme::get());
```

Reads the file at `path` and calls `loadStyleString` with the file's
content and name. Returns `false` if the file can't be opened.

```cpp
if (!ZMarkup::loadStyleFile("assets/game.zstyle")) {
    std::fprintf(stderr, "Failed to load game.zstyle\n");
}
```

### `applyStyleSheet`

```cpp
inline void applyStyleSheet(const StyleSheet& sheet,
                            const std::string& fileName,
                            Theme& theme);
```

Applies an already-parsed `StyleSheet` to a `Theme`. Used internally by
`loadStyleString`. Public for advanced flows where you parse once and
apply to multiple themes.

---

## 3. AST types

### `StyleSheet`

```cpp
struct StyleSheet {
    struct Rule {
        std::vector<SimpleSelector> chain;
        std::string part;                    // non-empty for ::part
        std::vector<Declaration> decls;
        int line{0};
        int col{0};
        std::optional<MediaQuery> media;
    };
    std::vector<Rule> rules;

    struct KfFrame {
        std::string ts;                      // "0%", "from", "100%", etc.
        std::vector<Declaration> decls;
        int line{0};
        int col{0};
    };
    struct KfRule {
        std::string name;
        std::vector<KfFrame> frames;
        int line{0};
        int col{0};
    };
    std::vector<KfRule> keyframes;

    std::unordered_map<std::string, std::string> vars;  // --name → value
    std::vector<Declaration> rootDecls;                 // :root { ... }
};
```

The AST output of `StyleParser::parse()`. Three top-level sections:

- **`rules`** — one entry per selector (a comma-separated selector list
  produces multiple rules). Each has a chain, an optional `::part`
  name, declarations, and an optional media query.
- **`keyframes`** — one entry per `@keyframes` block. Frames carry a
  timestamp string (`"0%"`, `"from"`) and declarations.
- **`rootDecls`** — the `:root { ... }` block's declarations.
- **`vars`** — custom properties extracted from `:root` (a subset of
  `rootDecls`, kept for backward compatibility).

### `Declaration`

```cpp
struct Declaration {
    std::string prop;
    std::string value;
    int line{0};
    int col{0};
};
```

A single `property: value;` declaration. The `line` / `col` are the
source position, used for logging.

### `ParseLoc`

```cpp
struct ParseLoc {
    std::string file;
    int line{0};
    int col{0};
};
```

A location used in log messages. Constructed from the parser's current
position and the source file name.

---

## 4. `StyleParser`

The parser class.

```cpp
class StyleParser {
public:
    StyleParser(std::string_view src, std::string fileName);
    StyleSheet parse();
};
```

### Construction

`StyleParser(src, fileName)` strips comments from `src`, stores it, and
remembers the file name for diagnostics.

**Note:** the constructor stores a **copy** of the stripped source. The
original `std::string_view` doesn't need to outlive the parser, but the
parser itself holds the whole source in memory.

### `parse()`

```cpp
StyleSheet parse();
```

Parses the source and returns the AST. Idempotent — calling it twice
on the same parser produces the same result (the parser's internal
position is reset).

The parser handles:

- `:root { ... }` blocks.
- `@keyframes name { ... }` blocks.
- `@media (cond) { ... }` blocks (containing rules).
- Regular `selector { declarations }` rules.
- Comments: `//` (line) and `/* ... */` (block).
- Multiple selectors via comma.

### Example

```cpp
ZenitUI::ZMarkup::StyleParser parser(R"(
    Button { background: red; }
    Button:hover { background: blue; }
)", "test.zstyle");

auto sheet = parser.parse();
// sheet.rules.size() == 2
// sheet.rules[0].chain[0].name == "Button"
// sheet.rules[1].chain[0].requireHover == true
```

### Line / column tracking

The parser maintains a `line` and `col` that advance as it consumes
characters. Each `Declaration` in the AST carries these, and error
messages use them via `ParseLoc`.

For a multi-line source, error messages point to the correct line:

```
[WARN ] [StyleParser] assets/game.zstyle(45:10): unknown property 'foo'. Ignored.
```

---

## 5. Attribute parsers (`StyleAttr.hpp`)

Low-level parsers for individual values. Used by `applyStyleAttr`, and
available for custom parsing flows.

### `parseValueToken`

```cpp
inline std::optional<Value> parseValueToken(std::string_view s);
```

Parses a value string into a `Value`. Handles:

- `auto` → `Value::autoSize()`.
- `calc(...)` → evaluated via `CalcParser`.
- `10px` / `10` / `10%` / `10vw` / `10vh` / `10pw` / `10ph`.
- Bare numbers (treated as pixels).

Returns `std::nullopt` on parse failure.

```cpp
auto v = parseValueToken("calc(100% - 20px)");
// v.value() resolves to 100% - 20px
```

### `parseColorToken`

```cpp
inline std::optional<Color> parseColorToken(std::string_view s);
```

Parses a color string. Supports:

- Named colors: `white`, `black`, `transparent`, `blank`, `red`,
  `maroon`, `green`, `blue`, `yellow`, `gray`, `darkgray`, `lightgray`.
- Hex: `#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA`.

Returns `std::nullopt` for unknown colors.

```cpp
auto c = parseColorToken("#FF000080");
// c->r == 0xFF, c->g == 0x00, c->b == 0x00, c->a == 0x80
```

### `parseSpacingToken`

```cpp
inline std::optional<Spacing> parseSpacingToken(std::string_view s);
```

Parses 1, 2, 3, or 4 whitespace-separated values into a `Spacing`.

```cpp
parseSpacingToken("10px");              // all sides
parseSpacingToken("10px 20px");         // vertical | horizontal
parseSpacingToken("1px 2px 3px");       // top | horizontal | bottom
parseSpacingToken("1px 2px 3px 4px");   // top | right | bottom | left
```

### `parseAlignToken`

```cpp
inline std::optional<Align> parseAlignToken(std::string_view s);
```

Parses one of: `auto`, `start`, `center`, `end`, `stretch`.

### `parseJustifyToken`

```cpp
inline std::optional<Justify> parseJustifyToken(std::string_view s);
```

Parses one of: `start`, `center`, `end`, `space-between`.

### `applyStyleAttr`

```cpp
inline bool applyStyleAttr(Style& st,
                           const std::string& key,
                           const std::string& val);
```

Applies a single `key: value` declaration to a `Style`. This is the
**shared** routine used by:

- The `.zstyle` parser's `applyStyleDeclaration`.
- ZMarkup's inline attribute application.
- The `var()` resolution path.

Returns `true` if the key was recognized, `false` otherwise. Unknown
keys are not an error — the caller decides whether to log.

The handled keys are the **kebab-case** versions of the
`BUBBLE_STYLE_PROPS` fields, plus a few special cases:

| Key | Effect |
|-----|--------|
| `width`, `height`, `min-width`, `max-width`, `min-height`, `max-height` | Sets the corresponding `Value` field. |
| `gap`, `padding`, `margin` | `gap` is `Value`, the others are `Spacing`. |
| `grow`, `shrink` | Parses a float. |
| `background`, `color`, `tint`, `border-color` | Parses a color. |
| `border-width`, `radius` | Parses a `Value`. |
| `opacity`, `scale`, `rotation` | Parses a float. |
| `translate-x`, `translate-y`, `top`, `left`, `right`, `bottom` | Parses a `Value`. |
| `font-size`, `letter-spacing` | Parses a `Value`. |
| `items-h`, `items-v`, `align-h`, `align-v`, `text-align` | Parses an `Align`. |
| `justify` | Parses a `Justify`. |
| `transition-time` | Parses a float. |
| `ease` | Parses an easing name. |
| `position` | `static` / `relative` / `absolute`. |
| `z-index` | `auto` or integer. |
| `font` | Sets the string. |
| `background-texture` | Parses `name [l t r b]`. |
| `effect` | Sets the string. |
| `wrap`, `checked`, `value`, `options`, `passthrough` | Recognized but ignored (widget-specific, handled by factories). |

Everything else returns `false`.

---

## 6. `var()` substitution

Two functions, both in `StyleAttr.hpp`.

### `substituteVarRefs`

```cpp
inline std::string substituteVarRefs(
    std::string v,
    const std::unordered_map<std::string, std::string>& customProps);
```

Replaces `var(--name)` references in `v` with the values from
`customProps`. Iterates up to 10 times to handle chained references
(`--a: var(--b)`).

If a `var()` reference doesn't resolve, it's left as-is (the caller
decides what to do — usually nothing, and the resulting string fails to
parse).

```cpp
std::unordered_map<std::string, std::string> props{
    {"--primary", "#FF0000"},
    {"--size", "100px"}
};
substituteVarRefs("var(--size) - 20px", props);  // "100px - 20px"
substituteVarRefs("var(--missing)", props);      // "var(--missing)"
```

The 10-iteration limit prevents infinite loops on circular references
(`--a: var(--b); --b: var(--a);`). After 10 passes, the remaining
`var()` calls are left unresolved.

### `resolveUnresolvedProps`

```cpp
inline Style resolveUnresolvedProps(
    const std::unordered_map<std::string, std::string>& unresolved,
    const std::unordered_map<std::string, std::string>& customProps);
```

Resolves a map of `property → raw value` (where the value contains
`var()`) into a typed `Style`. Each property is substituted and then
parsed via `applyStyleAttr`.

Returns a `Style` with only the properties that successfully parsed.

```cpp
std::unordered_map<std::string, std::string> unresolved{
    {"background", "var(--primary)"}
};
std::unordered_map<std::string, std::string> props{
    {"--primary", "#FF0000"}
};

auto s = resolveUnresolvedProps(unresolved, props);
// s.background.is_set == true, value == red
```

---

## 7. Parsing utilities

Free functions used by the parser and exposed for reuse.

### `trim`

```cpp
inline std::string trim(std::string s);
```

Removes leading and trailing whitespace.

```cpp
trim("  hello  ");  // "hello"
```

### `stripComments`

```cpp
inline std::string stripComments(std::string_view src);
```

Removes `//` line comments and `/* ... */` block comments. Preserves
comments inside string literals (respects `"..."` boundaries).

```cpp
stripComments("a // comment\nb");  // "a \nb"
stripComments("a /* block */ b");  // "a  b"
stripComments(R"("a // b")");      // "\"a // b\"" — comment preserved
```

The string awareness is what makes it safe to have `//` or `/*` inside
text content.

### `splitBy`

```cpp
inline std::vector<std::string> splitBy(std::string_view s, char sep);
```

Splits on a character, no whitespace trimming.

```cpp
splitBy("a,b,c", ',');   // ["a", "b", "c"]
splitBy("a,b,", ',');    // ["a", "b", ""]
splitBy("", ',');        // [""]
```

Trailing separators produce an empty final element.

### `splitWs`

```cpp
inline std::vector<std::string> splitWs(std::string_view s);
```

Splits on whitespace, collapsing consecutive spaces.

```cpp
splitWs("a    b   c");  // ["a", "b", "c"]
splitWs("   a b   ");   // ["a", "b"]
splitWs("");            // []
```

### `parseDurationSec`

```cpp
inline float parseDurationSec(std::string_view s);
```

Parses a duration string into seconds. Handles `"1s"`, `"500ms"`, and
bare numbers (interpreted as seconds).

```cpp
parseDurationSec("1s");     // 1.0
parseDurationSec("500ms");  // 0.5
parseDurationSec("2");      // 2.0
```

Throws `std::invalid_argument` / `std::out_of_range` on invalid input
(uses `std::stof` internally). The caller is expected to catch.

### `parseAspectRatio`

```cpp
static float parseAspectRatio(const std::string& s);
```

Parses `"16/9"` or a decimal number into a float. Used by the media
query parser.

```cpp
parseAspectRatio("16/9");   // 1.777...
parseAspectRatio("1.77");   // 1.77
```

### `normalizeKeyframes`

```cpp
inline void normalizeKeyframes(KeyframeAnimation& anim,
                                const ParseLoc& loc);
```

Fills in missing properties in a keyframe animation. For each property
that appears in some frames but not others, the missing values are
copied from the **nearest neighbor** (previous frame first, then next).

Logs a warning for each auto-filled property, with the source location.

```cpp
@keyframes example {
    0%   { scale: 1.0; }
    50%  { scale: 1.2; opacity: 0.5; }
    100% { scale: 1.0; }
}
```

Here `opacity` is set only at 50%. `normalizeKeyframes` copies `0.5`
into the 0% and 100% frames, logging two warnings.

The name "normalize" is a bit misleading — the animation would work
without this step (the evaluator's per-property fallback handles
missing endpoints). Normalization just makes the behavior explicit
and consistent.

### `applyStyleDeclaration`

```cpp
inline void applyStyleDeclaration(Style& st,
                                   const std::string& keyRaw,
                                   const std::string& valRaw,
                                   const ParseLoc& loc);
```

The high-level declaration applier. Handles:

- Custom properties (`--name`) — stored in `st.customProps`.
- Properties with `var()` — stored in `st.unresolvedProps`.
- `transition`, `transition-*` shorthands and longhands.
- `animation`, `animation-*` shorthands and longhands.
- `filter` — parsed into a `FilterRef` list.
- `box-shadow` — parsed into a `BoxShadow`.
- `overflow`, `overflow-x`, `overflow-y` — parsed into the enum.
- Fallback to `applyStyleAttr` for the standard properties.

**Logs a warning** for unknown properties and invalid values.

This is the function called for each declaration in a rule, a
keyframe, or `:root`. It's public because it's used by the parser and
by some test files, but for most use cases you'd just call
`loadStyleFile` / `loadStyleString`.

---

## 8. `calc()` parsing

The `calc()` support is implemented via an internal tokenizer and
recursive descent parser. Both are in `namespace ZenitUI::detail` and
not part of the public API, but documented here for completeness.

### `CalcToken`

```cpp
struct CalcToken {
    enum class Kind {
        Number, Ident, Percent,
        Plus, Minus, Star, Slash,
        LParen, RParen, End
    };
    Kind kind{Kind::End};
    float num{0.0f};
    std::string ident;
};
```

A single token in a `calc()` expression.

### `tokenizeCalc`

```cpp
inline std::vector<CalcToken> tokenizeCalc(std::string_view s);
```

Splits a `calc()` body into tokens. Numbers are read as floats, units
as identifiers (`px`, `vw`, `%`), operators as single characters.

### `CalcParser`

```cpp
class CalcParser {
public:
    explicit CalcParser(const std::vector<CalcToken>& toks);
    bool parse(Value& out);
};
```

Recursive descent parser for `calc()` expressions. Grammar:

```
expr   := term (('+' | '-') term)*
term   := factor (('*' | '/') factor)*
factor := '-' factor
        | '+' factor
        | '(' expr ')'
        | number ('%' | ident)?
```

Units are recognized: `px`, `vw`, `vh`, `pw`, `ph`, and `%`. An
unknown unit falls back to pixels.

The result is a `Value` — possibly with multiple terms for mixed-unit
expressions like `100% - 20px`.

**Note:** `*` and `/` require one side to be a bare number. `calc(50% * 2)`
works (`50% * 2.0`), but `calc(50% * 100%)` doesn't (multiplication of
two units is not meaningful and falls back to the left operand).

---

## 9. Complete example

Parsing a stylesheet manually and inspecting the AST:

```cpp
#include "StyleParser.hpp"
using namespace ZenitUI;

const char* src = R"(
    :root {
        --primary: #0079F1;
    }

    Button {
        background: var(--primary);
        padding: 8px 16px;
        transition: background 0.25s ease-out;
    }

    Button:hover {
        background: #3296FF;
    }

    @keyframes pulse {
        0%   { scale: 1.0; }
        50%  { scale: 1.2; }
        100% { scale: 1.0; }
    }
)";

ZMarkup::StyleParser parser(src, "example.zstyle");
auto sheet = parser.parse();

// sheet.rules.size() == 2
// sheet.rules[0].chain[0].kind == SimpleSelector::Kind::Tag
// sheet.rules[0].chain[0].name == "Button"
// sheet.rules[0].decls.size() == 3
// sheet.rules[1].chain[0].requireHover == true

// sheet.keyframes.size() == 1
// sheet.keyframes[0].name == "pulse"
// sheet.keyframes[0].frames.size() == 3

// sheet.rootDecls.size() == 1
// sheet.rootDecls[0].prop == "--primary"
// sheet.rootDecls[0].value == "#0079F1"

// Apply to a theme
Theme t;
ZMarkup::applyStyleSheet(sheet, "example.zstyle", t);
```

For most use cases, you'd skip the manual parsing and call
`loadStyleString` / `loadStyleFile` directly.

---

## 10. Pitfalls

**`StyleParser` copies the source.**

The constructor takes a `std::string_view`, but stores a stripped
`std::string` copy. This is safe (no lifetime issue with the view), but
it means the full source is in memory for the parser's lifetime. For
very large stylesheets, this is fine — parsing is a one-time cost.

**`parse()` can be called multiple times.**

It resets the internal position and re-parses. Useful for tests, but
in practice you'd call it once.

**Unknown properties are logged but not errors.**

`applyStyleDeclaration` logs a warning and continues. The rest of the
rule is still applied. This is intentional — a typo doesn't break the
whole stylesheet.

**`calc()` with malformed input returns `nullopt` from the outer
`parseValueToken`.**

The inner `CalcParser` sets an error flag and returns a best-effort
value. The outer check `!p.parse(out)` decides whether to accept the
result. A malformed `calc()` is treated as a parse failure, and the
property is left unset.

**`var()` substitution happens at cascade time, not parse time.**

The parser stores `unresolvedProps` with the raw `var()` expression.
`StyleResolver::resolveFor` substitutes at resolution time, when the
cascade has been resolved and the final `customProps` are known. This
is what makes `--x` inheritable and overridable.

**`substituteVarRefs` has a 10-iteration cap.**

Circular references (`--a: var(--b); --b: var(--a);`) will leave the
`var()` unresolved after 10 passes. The resulting string fails to
parse, and the property is left unset. No warning is logged for the
cycle itself — the failure manifests as a parse warning.

**`applyStyleAttr` returns `bool`, but `applyStyleDeclaration` doesn't.**

`applyStyleDeclaration` handles more cases (shorthands, custom props,
etc.) and always logs a warning for unknown keys. If you need the
boolean "was this recognized?" check, use `applyStyleAttr` directly
on standard properties.

**`trim` is by-value.**

`trim(std::string s)` takes by value. It modifies the copy and returns
it. If you already have a `std::string`, this moves efficiently. If
you have a `std::string_view`, it copies.

**`stripComments` preserves newlines for `//`.**

A line comment is replaced by a single newline, so line numbers in the
remaining source stay correct. Block comments are removed entirely,
which can shift subsequent line numbers if the block spans multiple
lines. The parser doesn't track this — line numbers in error messages
might be slightly off after a multi-line block comment.

**The `vars` map in `StyleSheet` is a subset of `rootDecls`.**

Custom properties from `:root` appear in both. The `vars` map is kept
for backward compatibility with code that reads it directly. New code
should use `rootDecls` (which `applyStyleSheet` processes via
`applyStyleDeclaration`, routing custom props into `theme.root.customProps`).

**`normalizeKeyframes` logs per-property per-keyframe.**

A keyframe animation with 5 frames and 3 properties missing in 2
frames produces up to 6 warnings. For large animations, the log can be
noisy. Filter by category or level if needed.

**`parseDurationSec` throws on invalid input.**

Unlike most parsers in the framework, this one uses `std::stof`
directly and lets exceptions propagate. The callers in
`applyStyleDeclaration` wrap it in try/catch, but a direct call site
must handle the exception.

**Media query conditions with unsupported keys are skipped.**

If a media query contains a key the parser doesn't recognize
(e.g. `resolution`), the condition is skipped (not the whole query).
The remaining conditions are still evaluated.

**The parser doesn't support nested rules.**

```css
/* Not supported */
.card {
    Button { color: red; }
}
```

Rules can't be nested inside other rules. Use descendant selectors
(`.card Button { ... }`) instead.

**`@media` blocks are expanded inline.**

Rules inside `@media { ... }` are added to `sheet.rules` with the
`media` field set. They aren't kept as a separate section. This means
the ordering in `sheet.rules` interleaves media rules with regular
ones, which affects the declaration order (and thus the cascade).

**`:root` variables must be declared before use.**

`applyStyleSheet` processes `rootDecls` before rules, so `:root`
custom properties are available when rules are resolved. But the
**parser** stores them in source order — if you manually inspect the
sheet, `rootDecls` might not be first. It doesn't matter for
`applyStyleSheet`, which processes `rootDecls` explicitly first.

**Selectors with `:` in a class name break parsing.**

The parser splits on `:` for state detection, so a class name
containing a colon isn't supported. This is a rare case — CSS also
doesn't allow colons in class names.

**The `part` field is only set for `::part` rules.**

A `::part` suffix on the last selector token moves the name into
`rule.part`. If you have a rule like `Slider::knob:checked`, the parser
might misinterpret `:checked` as part of the part name (since it
appears after `::`). Use the state before `::`:
`Slider:hover::knob` is correct; `Slider::knob:hover` isn't parsed as
a state.

**Unknown states are silently ignored.**

`Button:foo { ... }` parses, but `requireFoo` isn't a field, so the
`:foo` is silently dropped. The rule matches all buttons. No warning.
This is intentional (forward compatibility) but can hide typos.

**Attribute values are not validated at parse time.**

`applyStyleDeclaration` parses each value, but a value that doesn't
parse is logged and ignored — the property is left unset. This means a
typo (`background: blu;`) doesn't break the rule, but silently leaves
the property unchanged.

---

## 11. See also

- [ZStyle (guide)](../user/05-zstyle.md) — the grammar and semantics.
- [Theme](Theme.md) — where parsed rules end up.
- [Style](Style.md) — the `Style` struct modified by the parser.
- [Unit](Unit.md) — the `Value` type produced by `parseValueToken`.
- [ZMarkup](ZMarkup.md) — the DSL that uses `applyStyleAttr` for
  inline attributes.