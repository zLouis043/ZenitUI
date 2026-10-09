# `ZMarkup` — API Reference

The declarative DSL: parser, registry, builder, and the `UINode` handle
returned by `build`.

- **Header**: `ZMarkup.hpp`
- **Namespace**: `ZenitUI::ZMarkup`

For usage examples and the syntax guide, see
[ZMarkup (guide)](../user/04-zmarkup.md).

---

## 1. Overview

`ZMarkup` is split into four parts:

- **`Element`** — the AST node from the parser.
- **`Parser`** / **`parse`** — parses a markup string into an `Element` tree.
- **`Registry`** — maps tag names to factory functions.
- **`UINode`** — the handle returned by `build`, providing `find` and
  typed callback bindings.

The typical flow:

```cpp
auto ui = ZMarkup::build(R"(
    VStack#root {
        Text "Hello"
        Button.btn-primary#ok "OK"
    }
)");
ui.onClick("ok", []{ /* ... */ });
root->addChild(ui.root());
```

Or manually, with a custom registry:

```cpp
auto el = ZMarkup::parse(source);
auto ctx = std::make_shared<ZMarkup::BuildContext>();
ZMarkup::Registry reg;
ZMarkup::registerBuiltins(reg);
reg.reg("MyWidget", ...);
auto root = reg.create(el, *ctx);
ZMarkup::UINode ui(root, ctx);
```

---

## 2. `Element`

The AST node.

```cpp
struct Element {
    std::string tag;
    std::vector<std::string> classes;
    std::string id;
    std::string text;
    std::vector<std::pair<std::string, std::string>> attrs;
    std::vector<Element> children;

    std::string attr(std::string_view k, std::string_view d = "") const;
    bool        has(std::string_view k) const;
    float       attrFloat(std::string_view k, float d = 0.0f) const;
    bool        attrBool(std::string_view k, bool d = false) const;
};
```

### Fields

| Field | Description |
|-------|-------------|
| `tag` | The tag name (`"Button"`, `"Text"`, ...). |
| `classes` | Classes parsed from `.name` modifiers. |
| `id` | Id parsed from `#name`. Empty if none. |
| `text` | The quoted string after the attributes. |
| `attrs` | Key-value pairs. Flags become `(key, "true")`. |
| `children` | Nested elements from the `{ ... }` block. |

### Methods

#### `attr(k, default = "")`

Returns the value of the attribute `k`, or `default` if not present.

```cpp
auto v = el.attr("value");        // "" if missing
auto v = el.attr("value", "0");   // "0" if missing
```

#### `has(k)`

True if the attribute is present (even if its value is empty).

```cpp
if (el.has("checked")) { /* ... */ }
```

#### `attrFloat(k, default = 0.0f)`

Parses the attribute as a float. Returns `default` on parse failure.

```cpp
float val = el.attrFloat("value", 0.5f);
```

#### `attrBool(k, default = false)`

Parses the attribute as a boolean. Accepts `"true"`, `"1"`, `"yes"` as
true; anything else as false. Flags (no `=`) are stored as `"true"`,
so `el.attrBool("checked")` returns true for `Toggle checked`.

```cpp
bool chk = el.attrBool("checked", false);
```

**Note:** if the attribute is **missing**, the method returns `default`,
not false. To distinguish "missing" from "false", use `has(k)` first.

---

## 3. `parse` and `Parser`

### `parse(source)`

```cpp
inline Element parse(std::string_view source);
```

Parses the source string and returns the root `Element`. On empty input,
returns an `Element` with an empty tag.

The parser is a single-pass recursive descent. It handles:

- Tag names (identifier: `[a-zA-Z_][a-zA-Z0-9_-]*`).
- Modifiers `.class` and `#id`, concatenable in any order.
- Attributes: `key="value"`, `key=bare`, `key` (flag).
- Text: `"..."` with escape sequences (`\n`, `\t`, `\"`).
- Children: `{ ... }` block.
- Line comments: `// ...` (to end of line).

**No block comments.** `/* ... */` is not supported.

### `Parser`

```cpp
class Parser {
public:
    explicit Parser(std::string_view src);
    Element parseRoot();
};
```

The class form, if you need fine-grained control. `parse` is a
convenience wrapper.

### Example

```cpp
auto el = ZMarkup::parse(R"(
    Button.btn-primary#ok width="100px" "Click me"
)");

el.tag;         // "Button"
el.classes;     // ["btn-primary"]
el.id;          // "ok"
el.text;        // "Click me"
el.attr("width");  // "100px"
```

---

## 4. `BuildContext`

```cpp
struct BuildContext {
    std::unordered_map<std::string, std::shared_ptr<Layout>> byId;
};
```

A map from id → node, populated by `Registry::create` as it builds the
tree. Used by `UINode::find` for lookups.

**Note:** ids are **not required to be unique**. The map stores the
last node registered for a given id. If two elements have the same id,
`find` returns the second one (the last to be built).

In practice, use unique ids.

---

## 5. `Registry`

```cpp
class Registry {
public:
    using Factory = std::function<std::shared_ptr<Layout>(const Element&)>;

    void reg(const std::string& tag, Factory f);

    std::shared_ptr<Layout> create(const Element& el, BuildContext& ctx);

private:
    std::unordered_map<std::string, Factory> factories;
};
```

Maps tag names to factory functions.

### `reg(tag, factory)`

Registers a factory for a tag. If the tag is already registered, the
old factory is overwritten.

```cpp
reg.reg("MyWidget", [](const ZMarkup::Element& e) {
    return MyWidget::create(e.attrFloat("value", 0.5f));
});
```

The factory:

- Receives the parsed `Element`.
- Returns a `shared_ptr<Layout>` (or `nullptr` to skip the element).

### `create(el, ctx)`

Builds a node from an `Element`:

1. Look up the factory by `el.tag`. If not found, return `nullptr`.
2. Call the factory with `el`. If it returns `nullptr`, return `nullptr`.
3. Apply the `passthrough` attribute if present:
   `node->setPassThrough(el.attrBool("passthrough"))`.
4. Add each class from `el.classes`.
5. If `el.id` is non-empty, set it and register in `ctx.byId`.
6. Apply each attribute as an inline style via
   `applyStyleAttr(node->getInlineBase(), k, v)`.
7. Recursively create and add each child.

**Attributes reserved by the factory:** the factory is expected to read
its own special attributes (`value`, `checked`, `options`, `wrap`)
before returning. The `create` method then applies **all** attributes
(including those) as inline styles. So `value="0.5"` on a `Slider` is
both used by the factory (to set the initial value) and applied as an
inline style (where it's a no-op, since `value` isn't a real style
property).

There's no harm in this double application — `applyStyleAttr` ignores
unknown keys.

### `registerBuiltins(reg)`

```cpp
inline void registerBuiltins(Registry& r);
```

Registers the built-in tag factories. Called automatically by
`ZMarkup::build` on first use.

The built-in tags:

| Tag | Factory |
|-----|---------|
| `Stack` | `Layout(Stack)` with tag `"Stack"` |
| `Spacer` | `Layout(Stack)` with tag `"Spacer"` |
| `VStack` | `VStack()` |
| `HStack` | `HStack()` |
| `Text` / `Label` | `Text::create(e.text)`, honors `wrap` |
| `TextInput` | `TextInput::create(e.attr("value", e.text))` |
| `Checkbox` | `Checkbox::create(e.attrBool("checked"))` |
| `ProgressBar` | `ProgressBar::create(e.attrFloat("value"))` |
| `Dropdown` | `Dropdown::create(options)` from comma-split |
| `Button` | `Btn(e.text)` |
| `Panel` | `Pan()` |
| `Slider` | `Slider::create(e.attrFloat("value", 0.5f))` |
| `Toggle` | `Toggle::create(e.attrBool("checked"))` |
| `ScrollView` | `ScrollView::create()` |

### Extending with custom tags

```cpp
ZMarkup::Registry reg;
ZMarkup::registerBuiltins(reg);   // start with defaults
reg.reg("Rating", [](const Element& e) {
    return Rating::create(
        (int)e.attrFloat("max", 5.0f),
        (int)e.attrFloat("value", 0.0f)
    );
});

auto el = ZMarkup::parse(source);
auto ctx = std::make_shared<BuildContext>();
auto root = reg.create(el, *ctx);
UINode ui(root, ctx);
```

**Note:** there's no way to add tags to the **default** registry used
by `ZMarkup::build`. That registry is a private static. To use custom
tags, build the tree manually with your own registry.

---

## 6. `UINode`

The handle returned by `build`. Provides lookups and typed bindings.

```cpp
class UINode {
public:
    UINode(std::shared_ptr<Layout> root,
           std::shared_ptr<BuildContext> ctx);

    std::shared_ptr<Layout> root() const;
    std::shared_ptr<Layout> find(const std::string& id) const;

    template <typename T = Layout>
    std::shared_ptr<T> find(const std::string& id) const;

    UINode& onClick(const std::string& id, std::function<void()> cb);
    UINode& onPress(const std::string& id, std::function<void()> cb);
    UINode& onRelease(const std::string& id, std::function<void()> cb);
    UINode& onHoverEnter(const std::string& id, std::function<void()> cb);
    UINode& onHoverExit(const std::string& id, std::function<void()> cb);
    UINode& onValueChanged(const std::string& id, std::function<void(float)> cb);
    UINode& onToggle(const std::string& id, std::function<void(bool)> cb);
};
```

### `root()`

Returns the root node of the built tree.

```cpp
auto r = ui.root();
parent->addChild(r);
```

### `find(id)` / `find<T>(id)`

Finds a node by id.

```cpp
auto node = ui.find("save");                 // shared_ptr<Layout>
auto slider = ui.find<Slider>("volume");     // shared_ptr<Slider>
```

Returns `nullptr` if the id isn't found.

The typed version uses `std::static_pointer_cast`. If the actual type
doesn't match `T`, using the returned pointer is undefined behavior.
Use the typed `on*` helpers when possible (they do the cast safely and
no-op if the id doesn't exist or the type doesn't match).

### `onClick` / `onPress` / `onRelease` / `onHoverEnter` / `onHoverExit`

Bind a `void()` callback to the corresponding `Layout` field.

```cpp
ui.onClick("ok", []{ std::printf("clicked\n"); });
```

If the id doesn't exist, the binding is silently ignored (no error,
no log). This makes partial UIs easier to iterate on.

All return `UINode&` for chaining:

```cpp
ui.onClick("ok",     []{ save(); })
  .onClick("cancel", []{ cancel(); })
  .onHoverEnter("ok", []{ highlight(); });
```

### `onValueChanged`

Binds a `std::function<void(float)>` to a `Slider`'s `onValueChanged`.

```cpp
ui.onValueChanged("volume", [](float v){ setVolume(v); });
```

If the id doesn't resolve to a `Slider`, the binding is silently
ignored.

### `onToggle`

Binds a `std::function<void(bool)>` to a `Toggle`'s or `Checkbox`'s
`onToggle`.

```cpp
ui.onToggle("mute", [](bool v){ setMuted(v); });
```

The lookup is: try `find<Toggle>`, then `find<Checkbox>`. First match
wins.

---

## 7. `build`

```cpp
inline UINode build(std::string_view source);
```

The high-level entry point. Parses the source, builds the tree with
the built-in registry, and returns a `UINode` handle.

```cpp
auto ui = ZMarkup::build(R"(
    VStack#root {
        Text "Hello"
        Button.btn-primary#ok "OK"
    }
)");
```

**Note:** `build` uses a private static registry, initialized lazily on
first call. The default tags are registered once and reused for every
subsequent call. Custom tags can't be added to this registry — use
`parse` + a custom `Registry` + `UINode` for that.

If the source is empty or the root tag isn't registered, the returned
`UINode`'s `root()` is `nullptr`. Check before using:

```cpp
auto ui = ZMarkup::build(source);
if (!ui.root()) {
    std::printf("build failed\n");
    return;
}
```

---

## 8. Style loading

ZMarkup also handles `.zstyle` parsing. Two entry points, both defined
in `StyleParser.hpp`:

### `loadStyleString(src, theme = Theme::get(), fileName = "<string>")`

```cpp
inline bool loadStyleString(std::string_view src,
                            Theme& theme = Theme::get(),
                            const std::string& fileName = "<string>");
```

Parses a style source and applies it to the given theme.

```cpp
ZMarkup::loadStyleString(R"(
    Button { background: red; }
    Button:hover { background: blue; }
)");
```

Returns `true` on success, `false` on file-level errors (which
currently never happen for strings — parse errors are logged
per-rule but don't abort).

### `loadStyleFile(path, theme = Theme::get())`

```cpp
inline bool loadStyleFile(const std::filesystem::path& path,
                          Theme& theme = Theme::get());
```

Reads a file and calls `loadStyleString`.

```cpp
if (!ZMarkup::loadStyleFile("assets/game.zstyle")) {
    std::fprintf(stderr, "Failed to load stylesheet\n");
}
```

Returns `false` if the file can't be opened.

### Notes

- Both functions **append** to the theme. To reload, `Theme::get().clear()`
  first.
- The `fileName` is used in log messages, so a parse error in
  `game.zstyle` reports the correct file name.
- The parser supports `:root`, `@keyframes`, `@media`, `//` and
  `/* */` comments, custom properties, `var()`, `calc()`.
- Rules, keyframes, and root declarations are all applied in the order
  they appear.

See [ZStyle](../user/05-zstyle.md) for the full grammar and
[Theme](Theme.md) for the resulting data structures.

---

## 9. Complete examples

### Basic build and bind

```cpp
#include "UI.hpp"
using namespace ZenitUI;

auto ui = ZMarkup::build(R"(
    VStack.card#card {
        Text.card-title "Settings"
        HStack.setting-row {
            Text.setting-label "Volume"
            Slider#vol value=0.8 width="55%"
        }
        Button.btn-primary#save "Save"
    }
)");

if (!ui.root()) return;

ui.find<Slider>("vol")->onValueChanged = [](float v) {
    std::printf("volume: %.2f\n", v);
};

ui.onClick("save", []{ std::printf("saved\n"); });

parent->addChild(ui.root());
```

### Loading styles

```cpp
ZMarkup::loadStyleFile("assets/game.zstyle");
```

### Custom registry

```cpp
ZMarkup::Registry reg;
ZMarkup::registerBuiltins(reg);

reg.reg("Rating", [](const ZMarkup::Element& e) {
    return Rating::create(
        (int)e.attrFloat("max", 5.0f),
        (int)e.attrFloat("value", 0.0f)
    );
});

auto el = ZMarkup::parse(R"(Rating#stars max=5 value=3)");
auto ctx = std::make_shared<ZMarkup::BuildContext>();
auto root = reg.create(el, *ctx);

ZMarkup::UINode ui(root, ctx);
ui.find<Rating>("stars")->onRatingChanged = [](int r){ /* ... */ };
```

### Fragment building

```cpp
auto ui = ZMarkup::build(R"(
    HStack.setting-row {
        Text.setting-label "Volume"
        Slider#vol value=0.5
    }
)");
card->addChild(ui.root());
```

The root can be any tag. A tree whose root is a `Button` is valid.
The result is added as a child of an existing node.

---

## 10. Pitfalls

**`build` returns a `UINode` with a possibly-null root.**

If the source is empty or the root tag is unregistered, `root()`
returns `nullptr`. Always check before using:

```cpp
auto ui = ZMarkup::build(source);
if (!ui.root()) { /* handle */ }
```

**`find(id)` returns the last node with that id.**

Ids aren't required to be unique. If two elements have `#same`,
`find("same")` returns the second one. Use unique ids.

**`find<T>(id)` doesn't check the type.**

`static_pointer_cast` is used. If the actual node isn't of type `T`,
the returned pointer is invalid. Use the typed `on*` helpers for safety.

**`on*` bindings silently no-op on missing id.**

```cpp
ui.onClick("typo-id", []{ /* never fires */ });
```

No error, no log. Double-check ids.

**`build` uses a private registry.**

Custom tags must be registered via a manually-created `Registry`, and
the tree must be built via `parse` + `create` + `UINode`. There's no
way to add tags to the default registry.

**ZMarkup attributes are inline styles.**

They win over any `.zstyle` rule, including state rules. Use classes
for anything you want to vary by state.

```cpp
// Inline: wins over any rule
Button "OK" background="#FF0000"
```

```css
/* Never applies to the button above */
Button:hover { background: #00FF00; }
```

**`wrap` is a boolean attribute.**

`Text wrap "..."` is equivalent to `Text wrap=true "..."`. Also
`wrap="true"`, `wrap=1`, `wrap=yes`. Anything else is false.

**`options` splits on commas.**

```markup
Dropdown options="A,B,C"
```

produces three options. Whitespace is trimmed. To include a comma in an
option, you can't — use C++.

**Text content and `value` can conflict.**

For `TextInput`, `TextInput#t "initial"` and
`TextInput#t value="initial"` are equivalent (the factory falls back to
`e.text` if `value` isn't set). For other widgets, the meaning of `text`
is widget-specific (`Button` uses it for the label, `Text` for the
content, others ignore it).

**Escape sequences in text.**

Only `\n`, `\t`, and `\"` are supported. `\uXXXX` is not. Unicode
characters must be included literally (assuming the source is UTF-8).

**Comments are line-based.**

```markup
// Fine
VStack {
    Text "a"   // trailing comment
}
```

Block comments aren't supported. `/* ... */` is parsed as literal text.

**Children of composite widgets are appended after `onBuild`'s children.**

For `Toggle`, `onBuild` adds a knob child. If you also add children via
the ZMarkup block, they'll be siblings of the knob, not children of it.
For most widgets, this isn't what you want.

**`loadStyleString` doesn't reset the theme.**

It appends. To reload from scratch, call `Theme::get().clear()` first.

**The `fileName` parameter matters for diagnostics.**

Passing the actual file name (e.g. from `loadStyleFile`) makes log
messages point at the right file. Using the default `"<string>"`
makes them less useful.

**No support for dynamic values in ZMarkup attributes.**

You can't reference C++ variables. Attribute values are static strings
parsed at build time. For dynamic values, build the string in C++ and
pass it to `build`, or use the C++ API directly.

**No support for conditionals or loops.**

ZMarkup is a static description. Use C++ loops to generate a string
before calling `build`, or build the tree directly with the C++ API.

---

## 11. See also

- [ZMarkup (guide)](../user/04-zmarkup.md) — the syntax, patterns, and
  limitations.
- [ZStyle](../user/05-zstyle.md) — the `.zstyle` grammar.
- [StyleParser](ZStyle.md) — internal parsing utilities.
- [Layout](Layout.md) — the `Layout` class returned by factories.
- [Theme](Theme.md) — where rules end up after `loadStyle*`.