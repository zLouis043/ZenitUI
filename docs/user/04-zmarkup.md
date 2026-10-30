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
| `Canvas` | `CanvasLayout` | Renders subtree into an offscreen target. |
| `Modal` | `Modal` | Full-screen overlay with intro animation. Content goes into `contentTarget()` (see below). |
| `Popup` | `Popup` | Floating panel; requires C++ to call `openBelow`/`openAt`. |

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

### 2.3 Id (style identity)

A `#name` sets the node **id**. It is used:

- by `#name` selectors in `.zstyle` — with id specificity `(1, 0, 0)`,
  beating any class selector;
- by `UINode::find<T>(id)` to look up the node from C++;
- by `onClick(id, ...)` and friends on the `UINode` handle.

Because `#id` participates in CSS specificity, it's meant for **unique**
elements you want to style specifically. It's the wrong tool for list
items: 100 delete buttons each with `#delete_1`, `#delete_2`, ... would
pollute the cascade and the `byId` map. For that case, use `@ref` — see
§2.4.

There is no requirement that ids are unique. If two nodes share the
same `#id`, `find` returns the **last** one registered, and a warning
is logged (once per handle). Avoid duplicate ids unless you know what
you're doing.

### 2.4 Ref (logic identity)

A `@name` sets the node **ref**. It is used:

- by `UINode::ref<T>(name)` and `UINode::refAll<T>(name)` to look up
  the node from C++;
- **not** by `.zstyle` — `@ref` has no CSS specificity, no selectors.

The point of `@ref` is to **decouple logic lookup from style identity**.
A list of N delete buttons needs N unique logic handles but only one
shared style. With `@ref`:

```
VStack#list {
    Button.btn-danger@delete "X"
    Button.btn-danger@delete "X"
    Button.btn-danger@delete "X"
}
```

All three buttons share the `btn-danger` class (one style rule) and the
`delete` ref (one C++ lookup). From C++:

```cpp
// Look up by ref — returns the first match, warns if multiple.
auto first = ui.ref<Button>("delete");

// Look up all matches — returns every node with the given ref.
auto all = ui.refAll<Button>("delete");
for (auto& btn : all) {
    btn->onClick = [&]{ remove_item(btn); };
}
```

### 2.4.1 Semantics

| Method | Source | Cardinality | Empty result | Multiple results |
|--------|--------|-------------|--------------|------------------|
| `find<T>(id)` | `#id` | 0 or 1 | `nullptr` + warning | last wins |
| `ref<T>(name)` | `@ref` | 0..N | `nullptr` + warning | first + warning |
| `refAll<T>(name)` | `@ref` | 0..N | `{}` (debug log) | all |

`ref` and `find` are **two different concepts**, not aliases. A node
can have `#save@main_save` and both `find("save")` and
`ref("main_save")` return the same node. Or only `@delete` — the node
has no id, and can't be found by `find`.

Warnings are deduplicated per `UINode` handle: a typo'd id doesn't
spam the log every frame.

### 2.4.2 When to use which

- **`#id`** — unique elements you need to style specifically, and
  whose identity is stable: a save button, a title, a header.
- **`@ref`** — repeated elements generated from a list, or any node
  whose C++ handle is more natural than its style handle. The
  canonical case is a list of items: all share the style, each needs
  a handle.
- **Both** — rarely useful, but allowed. A save button that is also
  looked up in a loop would have `#save@save`.
- **Neither** — most nodes have no id and no ref. They're pure layout
  containers or static content.

### 2.5 Attributes

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

### 2.6 Text content

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

### 2.7 Children

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

Children are added to the parent's **content target**. For most widgets
the content target is the widget itself, so `{ ... }` children end up
directly inside the widget, exactly as `addChild()` would do.

For composite widgets that have an internal content wrapper — currently
only `Modal` — the content target is the wrapper. This means:

```zmarkup
Modal#dialog {
VStack.modal-box {
Label "Are you sure?"
}
}
```

The `VStack.modal-box` ends up **inside** the modal's content wrapper,
not as a sibling of it. Since the wrapper owns the "Intro" animation
that slides the modal content into view, markup children participate
in that animation automatically. No C++ injection needed:

```cpp
auto ui = ZMarkup::loadMarkupFile("assets/ui/menu.zmarkup");
auto modal = ui.find<Modal>("dialog");
modal->show();   // the modal-box slides in, animated
```

For other composite widgets (`Toggle`, `Dropdown`), the content target
is the widget itself — same as a plain `Layout`. Adding children to a
`Toggle` via markup stacks them on top of the knob in the same `Stack`
layer, which is usually not what you want. Use `Toggle` without markup
children, and configure its look through `.zstyle` instead.

For the full contract, see Layout::contentTarget() in the [Layout API](../api/Layout.md)

### 2.8 Comments

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
| `context()` | The `BuildContext` used to build the tree. Escape hatch for looking up nodes by id outside of `find()`. See §6. |
| `find(id)` | Find a node by `#id`, returns `shared_ptr<Layout>`. Warns (once) if the id isn't found. |
| `find<T>(id)` | Typed lookup — `ui.find<Slider>("vol")`. |
| `ref(name)` | Find a node by `@ref`, returns `shared_ptr<Layout>`. Warns if not found or if multiple match (returns the first). |
| `ref<T>(name)` | Typed lookup for `@ref` — `ui.ref<Button>("delete")`. |
| `refAll(name)` | Find all nodes by `@ref`, returns `vector<shared_ptr<Layout>>`. Empty vector + debug log if not found. |
| `refAll<T>(name)` | Typed `refAll` — `ui.refAll<Button>("delete")`. |
| `onClick(id, cb)` | Bind `onClick` (by `#id`). |
| `onPress(id, cb)` | Bind `onPress`. |
| `onRelease(id, cb)` | Bind `onRelease`. |
| `onHoverEnter(id, cb)` | Bind `onHoverEnter`. |
| `onHoverExit(id, cb)` | Bind `onHoverExit`. |
| `onValueChanged(id, cb)` | Bind `Slider::onValueChanged`. |
| `onToggle(id, cb)` | Bind `Toggle::onToggle` or `Checkbox::onToggle`. |

**`find` vs `ref` in the callbacks.** The `on*` helpers currently take
an id, not a ref: `ui.onClick("save", cb)` binds to the node with
`#save`. Binding to a ref is done explicitly:

```cpp
auto btn = ui.ref<Button>("delete");
if (btn) btn->onClick = []{ /* ... */ };
```

A future extension may add `ui.onClick("@delete", cb)` where the
leading `@` signals "look up by ref". This is deferred — see
`docs/architecture/PROPOSALS.md` §1.11.

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

### Loading from a file

```cpp
auto ui = ZMarkup::loadMarkupFile("assets/ui/main_menu.zmarkup");
if (!ui.root()) {
    // file not found or empty; see the log for details
    return;
}
root->addChild(ui.root());
```

Symmetric to `ZMarkup::loadStyleFile` for `.zstyle` sources.

Missing file → null root. The error is logged via `Logger`, so a
custom logger will see it. The `UINode` is always usable: `context()`
returns a valid (empty) context even when the root is null.

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

### 4.5 List of repeated actions with `@ref`

The canonical use case for `@ref`: a list of items, each with a delete
button, all sharing one style but each needing its own C++ handle.

```
VStack#list {
    HStack.item { Label "Apple";  Button.btn-danger@delete "X" }
    HStack.item { Label "Banana"; Button.btn-danger@delete "X" }
    HStack.item { Label "Cherry"; Button.btn-danger@delete "X" }
}
```

```cpp
// Wire all delete buttons at once.
for (auto& btn : ui.refAll<Button>("delete")) {
    btn->onClick = [&]{
        // The callback runs for whichever button fired it; use the
        // button's parent (the HStack.item) to identify the row.
        if (auto row = btn->getParent())
            row->removeFromParent();
    };
}
```

Compare with the id-based approach: without `@ref`, each button needs
its own `#delete_apple`, `#delete_banana`, `#delete_cherry`, plus three
separate `onClick` bindings. The ref-based approach scales to any
number of rows.

**Why not use `#id` here.** Ids carry CSS specificity `(1, 0, 0)`. If
each delete button has `#delete_apple`, no class rule can override
`Button { background: ... }` for that button. With `@ref`, the button
keeps its class-based styling and gains a C++ handle at zero CSS cost.

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

### Accessing the BuildContext

The default `ZMarkup::build` and `loadMarkupFile` create a
`BuildContext` internally and store it in the returned `UINode`.
`context()` exposes it:

```cpp
auto ui = ZMarkup::build(source);
auto ctx = ui.context();

// ctx->byId["save"] is the node with id "save", or absent
auto it = ctx->byId.find("save");
if (it != ctx->byId.end()) {
    auto save = it->second;
    // ...
}
```

Most of the time `find()` is enough and more ergonomic. `context()`
is the escape hatch for two cases:

- Bulk inspection. Iterating every id in the tree, checking for
  duplicates, or building a map from id → node for external use.
  `find()` is per-id; `context()` gives the whole map.

- Extending a tree at runtime. If you built the tree with a
  custom Registry (see above), you can use the same registry
  and the same BuildContext to add more nodes later, and they'll
  be registered in the same id map:

```cpp
auto ui = /* built with custom registry + context */;
auto ctx = ui.context();

auto el = ZMarkup::parse(R"(Button#new "Added later")");
auto node = myRegistry.create(el, *ctx);
ui.root()->addChild(node);

// ctx->byId now contains "new" as well
```

The `BuildContext` is a plain struct holding a single
`unordered_map<string, shared_ptr<Layout>>`. It's safe to keep the
shared pointer around as long as you need it. Trees built by
`ZMarkup::build` share nothing with each other — each call produces
its own context.

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

## 6. Prefabs and Slots

For complex, repeatable UI containers (like a custom card, a list item, or a settings row), you can define a `Prefab`. A Prefab is a ZMarkup AST parsed once and registered under a custom tag name.

When you instantiate a Prefab, you can pass custom styling attributes which are automatically merged into the Prefab's root node. You can also inject child nodes into a designated `<Slot>` within the Prefab.

### 6.1 Registering a Prefab

Register a prefab in C++ before calling `ZMarkup::build`. The `<Slot>` tag marks where children will be injected:

```cpp
ZenitUI::ZMarkup::registerPrefab("SettingsRow", R"(
    HStack width="100%" justify="space-between" items-v="center" background="#2A2A32" padding="10px" radius="6px" {
        Slot {
            // Default children if none are provided
            Text "Empty Row"
        }
    }
)");
```

### 6.2 Instantiating a Prefab

Use the registered tag name in your main markup. The children you provide will replace the `<Slot>`.

```zmarkup
VStack {
    // Injecting two elements. Because the prefab is an HStack with space-between,
    // the Text goes to the left and the Toggle to the right.
    SettingsRow {
        Text color="#A0A0A0" "Fullscreen"
        Toggle#fs_toggle checked=true
    }
}
```

### 6.3 Root Merging

Attributes applied to the prefab instance tag (`.class`, `#id`, `@ref`, inline styles) are merged into the prefab's root node.

```zmarkup
SettingsRow.highlighted-row#row1@main_row width="50%" { ... }
```

Here, `.highlighted-row`, `#row1`, `@main_row`, and `width="50%"` are applied directly to the root `HStack` defined in the `SettingsRow` prefab.

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

**`@ref` doesn't match anything in `.zstyle`.**

```
Button@delete "X"
```

is **not** styleable by `@delete { ... }` or `[ref="delete"]`. There
is no CSS hook for `@ref`. Use a class (`.btn-danger`) for styling,
`@ref` only for C++ lookup.

**`ref` and `find` return `nullptr` on a miss, and log a warning.**

Both `ui.find<Layout>("typo")` and `ui.ref<Layout>("typo")` return
`nullptr` if no node matches. A warning is logged (once per handle,
not per frame), so if the UI is still behaving as if nothing happened,
check the log — the id or ref is likely wrong. The `refAll` variant
returns an empty vector and logs at `Debug` level, since "no items in
the list" is legitimate.

**`ref` on a single node vs `refAll` on a group.**

```cpp
// If "delete" matches 3 nodes, this returns the first and logs a warning.
auto btn = ui.ref<Button>("delete");

// This returns all 3, no warning.
auto all = ui.refAll<Button>("delete");
```

Use `ref` when you expect a unique match (a specific element), and
`refAll` when you're iterating (a list of items). Mixing them is a
common source of "the wrong button got the callback".