# ZMarkup

ZMarkup is a small declarative DSL for building UI trees without writing
C++ boilerplate. It's parsed at runtime, compiled to `Layout` nodes, and
returns a `UINode` handle you can use to bind callbacks and query
widgets by id.

It is **not** a replacement for C++ — it's a convenience for describing
structure. Logic, callbacks, and integration with the rest of your code
still happen in C++.

---

## 1. Why two languages?

You could write everything in ZMarkup:

```
VStack.card {
    Text.card-title "Settings"
    HStack.setting-row {
        Text.setting-label "Volume"
        Slider#vol value=0.8
    }
}
```

Or everything in C++:

```cpp
auto card = VStack()->cls("card");
card->addChild(Label("Settings")->cls("card-title"));

auto row = HStack()->cls("setting-row");
row->addChild(Label("Volume")->cls("setting-label"));
row->addChild(Slider::create(0.8f)->id("vol"));
card->addChild(row);
```

Both produce the same tree. ZMarkup shines when:

- The structure is deep but the logic is shallow.
- You want designers / non-programmers to edit layout.
- You want to prototype quickly.
- You want to separate "what the UI looks like" from "what the UI does".

ZMarkup is weaker when:

- You need computed attributes (build the string dynamically).
- You need loops / conditionals (build in C++ instead).
- You need tight control over every node.

**You can mix the two freely** — a ZMarkup tree can be added as a child
of a C++ tree, and vice versa.

---

## 2. Syntax

```
Tag.class1.class2#id attr1="value1" attr2=value2 flag "text content" {
    Child
    Child
}
```

Everything except the tag name is optional. The parts are described in
the sections below.

### 2.1 Tag

The first identifier is the tag. It maps to a factory registered with the
`ZMarkup::Registry`. Built-in tags:

| Tag | Widget | Notes |
|-----|--------|-------|
| `Stack` | `Layout(Stack)` | Style tag `Stack` |
| `VStack` | `Layout(Vertical)` | Style tag `VStack` |
| `HStack` | `Layout(Horizontal)` | Style tag `HStack` |
| `Spacer` | `Layout(Stack)` | Style tag `Spacer`, typically `grow: 1` |
| `Text` / `Label` | `Text` | Text widget |
| `Button` | `Button` | Uses `Btn(text)` factory |
| `Panel` | `Panel` | Plain box |
| `Toggle` | `Toggle` | |
| `Checkbox` | `Checkbox` | |
| `Slider` | `Slider` | |
| `ProgressBar` | `ProgressBar` | |
| `TextInput` | `TextInput` | |
| `Dropdown` | `Dropdown` | |
| `ScrollView` | `ScrollView` | |

Tags are **case-sensitive**. `Button` and `button` are different — only
`Button` is registered by default.

### 2.2 Classes

A `.name` adds a CSS class. You can chain as many as you want, in any
order relative to `#id`:

```
Button.btn.btn-primary "OK"
Button#submit.btn-primary "OK"
```

The classes are added via `Layout::cls()` and are visible to `.zstyle`
selectors and to `getStyleClasses()` at runtime.

### 2.3 Id

A `#name` sets the node id. It is used:

- by `UINode::find<T>(id)` to look up the node,
- by `#name` selectors in `.zstyle`,
- by `onClick(id, ...)` and friends on the `UINode` handle.

There is no requirement that ids are unique, but `find` returns the
**first** match in the order nodes were created, so use unique ids if
you plan to query them.

### 2.4 Attributes

`key=value` pairs, separated by whitespace. Values can be:

- **Quoted strings**: `value="Player 1"` (supports `\n`, `\t`, `\"`).
- **Bare tokens**: `value=0.65` — anything up to the next whitespace,
  `{`, `}`, or `"`.
- **Flags**: `checked` with no `=` is shorthand for `checked="true"`.

Attributes are applied **after** the widget is created, so they can
override defaults set by the factory.

The following attributes have special meaning:

| Attribute | Effect |
|-----------|--------|
| `passthrough` | Sets `Layout::setPassThrough(...)`. |
| `value` | Passed to `TextInput`, `Slider`, `ProgressBar` at construction. |
| `checked` | Passed to `Toggle`, `Checkbox` at construction. |
| `options` | Comma-separated list for `Dropdown`. |
| `wrap` | Enables wrapping on `Text`. |

**Everything else** is applied as an inline style via
`applyStyleAttr(getInlineBase(), key, value)`. So any property you
could set in `.zstyle` can also be set inline here — see the full list
in [ZStyle](05-zstyle.md) §"Inline attributes".

```
Button "OK" width="120px" height="40px" background="#0079F1" radius="8px"
```

### 2.5 Text content

A quoted string after the attributes is the widget's text. It is passed
to the tag's factory, which decides what to do with it:

- `Text` / `Label` → `setText(text)`.
- `Button` → `Btn(text)`, adds a `Text` child.
- `TextInput` → initial value.
- Others → ignored.

```
Text "Hello"
Button "Click me"
```

### 2.6 Children

A `{ ... }` block contains child elements, each on its own line (or
separated by whitespace). Nesting is unlimited.

```
VStack {
    Text "First"
    Text "Second"
    HStack {
        Button "A"
        Button "B"
    }
}
```

Children are added to the parent via `addChild()`. For composite widgets
(`Toggle`, `Dropdown`, `Modal`), the children of the ZMarkup element are
**appended after** the ones created by `onBuild()`. This is usually not
what you want for `Toggle` — its knob is an internal child, and adding
your own children will stack them in the same `Stack` layer.

### 2.7 Comments

`//` starts a line comment. Everything from `//` to the end of the line
is ignored.

```
// This is a comment
VStack {
    Text "Hello"   // trailing comment
}
```

Block comments `/* ... */` are **not** supported in ZMarkup (they are
supported in `.zstyle`).

---

## 3. Building a UI

`ZMarkup::build(source)` parses the source, builds the tree, and returns
a `UINode` handle.

```cpp
#include "ZMarkup.hpp"
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

auto root = ui.root();
parent->addChild(root);
```

### The `UINode` handle

| Method | Description |
|--------|-------------|
| `root()` | The root `Layout` of the built tree. |
| `find(id)` | Find a node by id, returns `shared_ptr<Layout>`. |
| `find<T>(id)` | Typed lookup — `ui.find<Slider>("vol")`. |
| `onClick(id, cb)` | Bind `onClick`. |
| `onPress(id, cb)` | Bind `onPress`. |
| `onRelease(id, cb)` | Bind `onRelease`. |
| `onHoverEnter(id, cb)` | Bind `onHoverEnter`. |
| `onHoverExit(id, cb)` | Bind `onHoverExit`. |
| `onValueChanged(id, cb)` | Bind `Slider::onValueChanged`. |
| `onToggle(id, cb)` | Bind `Toggle::onToggle` or `Checkbox::onToggle`. |

All the `on*` methods return `UINode&` for chaining:

```cpp
ui.onClick("save",   []{ save(); })
  .onClick("cancel", []{ cancel(); })
  .onToggle("mute",  [](bool v){ setMuted(v); });
```

If the id is not found, the binding is silently ignored (no exception,
no log). This is intentional — it makes partial UIs easier to iterate on.

### Manual lookup with type

```cpp
if (auto slider = ui.find<Slider>("vol")) {
    slider->onValueChanged = [](float v){
        std::printf("volume: %.2f\n", v);
    };
}

if (auto toggle = ui.find<Toggle>("mute")) {
    toggle->onToggle = [](bool v){ /* ... */ };
}
```

`find<T>` uses `std::static_pointer_cast`, so you must be sure of the
type — otherwise you'll get undefined behavior when the cast is used.
Prefer the typed `on*` helpers when possible.

---

## 4. Common patterns

### 4.1 Settings row

```
VStack.settings-section {
    Text.settings-section-title "Audio"

    HStack.settings-row {
        Text.setting-label "Music"
        Slider#music value=0.55 width="55%"
        Text.setting-value#music-value "55%"
    }
}
```

```cpp
auto ui = ZMarkup::build(markup);

auto slider = ui.find<Slider>("music");
auto label  = ui.find<Text>("music-value");

slider->onValueChanged = [label](float v){
    label->setText(std::to_string((int)(v * 100)) + "%");
};
```

### 4.2 Button row with mixed actions

```
HStack.button-row {
    Button.btn-primary#save "Save"
    Button.btn-secondary#cancel "Cancel"
    Button.btn-danger#delete "Delete"
}
```

```cpp
ui.onClick("save",   []{ save(); })
  .onClick("cancel", []{ cancel(); })
  .onClick("delete", []{ confirmDelete(); });
```

### 4.3 Nested layouts

```
VStack.card {
    Text.card-title "Two columns"

    HStack gap="16px" {
        VStack.side-panel {
            Text "Left"
            Button "L1"
        }
        VStack.side-panel {
            Text "Right"
            Button "R1"
        }
    }
}
```

### 4.4 Form with a dropdown and a progress bar

```
VStack.card {
    Text.card-title "Download"

    HStack.setting-row {
        Text.setting-label "Quality"
        Dropdown#quality options="Low,Medium,High" width="40%"
    }

    HStack.setting-row {
        Text.setting-label "Progress"
        ProgressBar#progress value=0.3 width="55%"
    }
}
```

```cpp
auto dd = ui.find<Dropdown>("quality");
auto pb = ui.find<ProgressBar>("progress");

dd->onChange = [](int idx, const std::string& name){
    std::printf("quality: %s\n", name.c_str());
};

ui.onClick("start", [pb]{
    pb->setValue(0.0f);
    // kick off your download...
});
```

---

## 5. Building fragments

You can build a fragment (a tree with a non-`Stack` root, or a single
widget) and use it as part of a C++ tree:

```cpp
auto ui = ZMarkup::build(R"(Button.btn-primary "Click me")");
auto button = ui.root();
parent->addChild(button);
```

Or add a ZMarkup subtree to an existing widget:

```cpp
auto card = VStack()->cls("card");

auto ui = ZMarkup::build(R"(
    HStack.setting-row {
        Text.setting-label "Volume"
        Slider#vol value=0.5
    }
)");
card->addChild(ui.root());

// Bind after adding
ui.find<Slider>("vol")->onValueChanged = [](float v){ /* ... */ };
```

---

## 6. Extending the registry

ZMarkup resolves tags through a registry. You can register your own
factories to expose custom widgets to the DSL.

```cpp
ZMarkup::Registry& reg = /* ... */;
```

The built-in registry is a static instance inside `ZMarkup::build`, so
you can't easily get a reference to it. If you need custom tags, build
your own registry:

```cpp
ZMarkup::Registry reg;
ZMarkup::registerBuiltins(reg);   // start from the defaults

reg.reg("MyWidget", [](const ZMarkup::Element& e){
    auto w = MyWidget::create(e.attrFloat("value", 0.f));
    return std::static_pointer_cast<Layout>(w);
});

auto el = ZMarkup::parse(source);
auto ctx = std::make_shared<ZMarkup::BuildContext>();
auto root = reg.create(el, *ctx);
ZMarkup::UINode ui(root, ctx);
```

A factory receives the `Element` and returns a `shared_ptr<Layout>` (or
`nullptr` to skip). The registry then:

1. Applies `passthrough` if present.
2. Adds the classes.
3. Sets the id.
4. Applies inline style attributes.
5. Recursively creates and adds children.

This means a factory only needs to construct the widget — everything
else (classes, id, attrs, children) is handled by the registry.

### The `Element` struct

```cpp
struct Element {
    std::string tag;
    std::vector<std::string> classes;
    std::string id;
    std::string text;
    std::vector<std::pair<std::string, std::string>> attrs;
    std::vector<Element> children;

    std::string attr(std::string_view k, std::string_view d = "") const;
    bool has(std::string_view k) const;
    float attrFloat(std::string_view k, float d = 0.f) const;
    bool attrBool(std::string_view k, bool d = false) const;
};
```

`attrFloat` / `attrBool` do a best-effort parse and return the default
on failure. `attr` returns an empty string (or the given default) if the
key isn't present.

---

## 7. Interaction with `.zstyle`

Attributes in ZMarkup are **inline styles**. They have the **highest**
priority in the cascade — they beat any `.zstyle` rule, including
state rules like `:hover`.

```cpp
// Inline: wins over any .zstyle rule
Button "OK" background="#FF0000"
```

```css
/* Ignored — the inline style above always wins */
Button { background: #0000FF; }
```

Best practice: use ZMarkup attributes only for structural things
(`id`, `class`, `value`, `checked`, `options`, `wrap`), and put visual
properties in `.zstyle`.

```
Button.btn-primary#save "Save"
```

```css
.btn-primary {
    background: #0079F1;
    radius: 8px;
    padding: 1.5vh 2vw;
    transition: background 0.25s ease-out;
}
.btn-primary:hover { background: #3296FF; }
```

This keeps your markup readable and your styles centralized.

---

## 8. Loading styles separately

ZMarkup also handles the `.zstyle` language. Two entry points:

```cpp
ZMarkup::loadStyleString(R"(
    Button { radius: 8px; }
    Button:hover { background: #3296FF; }
)");
```

```cpp
ZMarkup::loadStyleFile("assets/game.zstyle");
```

Both append rules to `Theme::get()`. Rules accumulate — you can call
`loadStyleFile` multiple times and they'll all apply.

To reset:

```cpp
Theme::get().clear();
```

See [ZStyle](05-zstyle.md) for the full language reference.

---

## 9. Limitations

- **No conditionals, no loops.** ZMarkup is a static description. Use
  C++ to generate strings or to build the tree directly.
- **No expressions in attributes.** `width=calc(...)` works because
  `calc()` is parsed by the value parser, but you can't reference
  runtime variables. Build the string in C++ if you need dynamic values.
- **No `text` attribute.** Use the quoted string form
  (`Text "Hello"`) rather than `Text text="Hello"` — the latter is not
  handled by the factories.
- **No escaping of `"` inside attributes.** You can use `\"` in the
  quoted form (the parser handles it), but it's easier to avoid.
- **Comments cannot be nested.** `//` is line-based; there is no block
  comment.
- **Tags must be registered.** Unknown tags cause `nullptr` to be
  returned by the factory, and the registry skips the element silently
  (including its children).

---

## 10. Pitfalls

**Forgetting `.create()`.**

Inside ZMarkup, this is not an issue — the factories call `::create()`
correctly. But if you write a custom factory and call
`std::make_shared<MyWidget>()`, `onBuild()` won't run. Always use
`MyWidget::create(...)`.

**Missing id on bind.**

`ui.onClick("save", ...)` silently does nothing if `#save` isn't
present. Double-check the id string.

**Duplicate ids.**

`find(id)` returns the first match in **creation** order, which is
depth-first. If you have two `#save`, the first one built wins.

**Setting `width` on a `VStack` in ZMarkup, then also in `.zstyle`.**

The inline attribute wins. Move it to `.zstyle` if you want state rules
to be able to override it.

**`wrap` on `Text` needs a bounded width.**

```css
/* Fine — Text has width, so wrap works */
.card Text { width: 100%; }
```

```
/* Broken — Text gets infinite width, wrap never kicks in */
VStack {
    Text wrap "long text..."
}
```

Wrap the `Text` in a fixed-width parent, or set an inline `width`.

**Number vs string attributes.**

`value=0.65` is a bare token — the parser stores `"0.65"` and the
factory uses `attrFloat` to convert. `value="0.65"` is equivalent. But
`value="Hello, world"` with a comma is parsed as-is; the `Dropdown`
factory splits on commas, so `options="A,B,C"` creates three options.

**`options` with quotes inside options.**

Not supported. If you need options containing commas or quotes, build
the `Dropdown` in C++ and add it as a child.