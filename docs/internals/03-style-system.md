# Style System

The style system is `StyleResolver` plus the data structures it operates
on: `Style`, `ComputedStyle`, `Theme`, `ThemeRule`, `SimpleSelector`,
and the `Theme::keyframes` map. This document describes how a node's
final style is computed every frame, how transitions are ticked, how
inheritance propagates, and how `::part` styles are resolved
independently.

---

## 1. The data model

### 1.1 `Style` — the "declared" style

`Style` is a bag of optional properties. Every field is an
`Opt<T>` — a `T` plus a `bool is_set`. The property list is generated
by the macro `BUBBLE_STYLE_PROPS(X)`:

```cpp
#define BUBBLE_STYLE_PROPS(X)                                \
    X(Value, width, Value::autoSize())                       \
    X(Value, height, Value::autoSize())                      \
    X(float, grow, 0.0f)                                     \
    X(Color, background, Colors::Blank)                      \
    X(float, opacity, 1.0f)                                  \
    /* ... ~45 properties total ... */                       \
    X(std::string, font, "")
```

`Style` also has:

- `Opt<std::vector<TransitionSpec>> transitions` — the parsed
  `transition:` shorthand.
- `Opt<std::vector<AnimationRef>> animations` — the parsed `animation:`
  shorthand.
- `std::unordered_map<std::string, std::string> customProps` — custom
  properties (`--name`).
- `std::unordered_map<std::string, std::string> unresolvedProps` — raw
  properties that contain `var()` and couldn't be resolved at parse time.

The macro is used by `Style::overlay`, `stylesDiffer`, `forEachSetStyleProp`,
`copyStyleProp`, `hasStyleProp`, `propTable`, `clearStyleProp`, and
`Debug::styleToString`. Adding a property means adding one line to the
macro and everything else picks it up.

`overlay(o)` copies every property from `o` that is set:

```cpp
Style& overlay(const Style& o) {
    #define X(T, name, def) if (o.name.is_set) name = o.name;
    BUBBLE_STYLE_PROPS(X)
    #undef X
    if (o.transitions.is_set) transitions = o.transitions;
    if (o.animations.is_set)  animations  = o.animations;
    // merge customProps, unresolvedProps
    return *this;
}
```

The macro-based overlay is why the entire cascade is a sequence of
`overlay` calls — see §3.

### 1.2 `ComputedStyle` — the "resolved" style

Same property list, but each field is a concrete `T`, not `Opt<T>`. No
`is_set` flag — everything has a value (the default from
`BUBBLE_STYLE_PROPS`).

`ComputedStyle::from(s, parent, root)` converts a `Style` to a
`ComputedStyle` using the priority:

1. If `s.name.is_set`, use it.
2. Else if the property is inherited and `parent` exists, use
   `parent->name`.
3. Else if `root` has the property set, use `root->name`.
4. Else use the default.

Plus the overflow computed-value rule:

```cpp
if (c.overflowX == Overflow::Visible && c.overflowY != Overflow::Visible)
    c.overflowX = Overflow::Auto;
else if (c.overflowY == Overflow::Visible && c.overflowX != Overflow::Visible)
    c.overflowY = Overflow::Auto;
```

And the merge of `transitions`, `animations`, and `customProps` (custom
props inherit from parent by default).

`ComputedStyle` has `operator==` and `operator!=`, both generated from
the macro. They compare every field, which is what drives the
`styleChanged` check in `recomputeDirty`.

### 1.3 `Theme` — the global stylesheet

`Theme` is a singleton:

```cpp
class Theme {
public:
    std::vector<ThemeRule> rules;
    Style root;
    std::unordered_map<std::string, KeyframeAnimation> keyframes;

    static Theme& get();
    void addRule(ThemeRule r);        // computes specificity + order
    void addKeyframes(const KeyframeAnimation& anim);
    void clear();
};
```

`rules` is an ordered list — order of declaration. `root` is the
`:root` block, used as a fallback for non-inherited properties and as
the base for custom properties.

### 1.4 `ThemeRule` — a single rule

```cpp
struct ThemeRule {
    std::vector<SimpleSelector> chain;   // descendant chain
    std::string part;                    // "track", "knob", or empty
    Style style;
    Specificity specificity;
    int order;                           // declaration order
    std::optional<MediaQuery> media;     // @media condition
};
```

`chain` is right-to-left: `chain.back()` matches the node itself, and
`chain[i]` for `i < back` matches ancestors.

`part` is non-empty only for `::part` rules. Those rules are stored in
the same `rules` vector, but skipped by the normal cascade and picked
up by `partFor`.

### 1.5 `SimpleSelector` — one component of a selector

```cpp
struct SimpleSelector {
    enum class Kind { Tag, Class, Id };
    std::string name;
    Kind kind;

    bool requireHover{false};
    bool requirePressed{false};
    bool requireFocus{false};
    bool requireDisabled{false};
    bool requireChecked{false};
};
```

A compound selector like `Button.btn-primary:hover:pressed` becomes one
`SimpleSelector` with `kind = Tag`, `name = "Button"`, `requireHover =
true`, `requirePressed = true`.

The class check is actually a "contains class" — the node's
`styleClasses` vector is searched for `name`.

### 1.6 `Specificity`

```cpp
struct Specificity {
    int ids{0}, classes{0}, tags{0};
    bool operator<(const Specificity& o) const;   // lexicographic
};
```

Computed from the chain:

```cpp
Specificity computeSpecificity(const std::vector<SimpleSelector>& chain) {
    Specificity s;
    for (const auto& sel : chain) {
        if (sel.kind == Id)          s.ids++;
        else if (sel.kind == Class)  s.classes++;
        else                          s.tags++;
        if (sel.requireHover || sel.requirePressed || sel.requireFocus ||
            sel.requireDisabled || sel.requireChecked)
            s.classes++;
    }
    return s;
}
```

States count as classes. A rule like `Toggle:checked .toggle-knob` has
specificity `(0, 2, 1)` — one class (`.toggle-knob`), one state
(`:checked`), one tag (`Toggle`).

---

## 2. Selector matching

### 2.1 `nodeMatchesSimple(node, sel)`

```cpp
inline bool nodeMatchesSimple(const Layout* node, const SimpleSelector& ss) {
    if (!node) return false;

    if (ss.requireHover    && !node->isHoveredState())   return false;
    if (ss.requirePressed  && !node->isPressedState())   return false;
    if (ss.requireFocus    && !node->isFocusedState())   return false;
    if (ss.requireDisabled && !node->isDisabledState())  return false;
    if (ss.requireChecked  && !node->isCheckedState())   return false;

    if (ss.kind == Tag)   return node->getStyleTag() == ss.name;
    if (ss.kind == Id)    return node->nodeId == ss.name;

    for (const auto& c : node->getStyleClasses())
        if (c == ss.name) return true;
    return false;
}
```

Order matters: state checks come first (cheap), then tag/id/class.

### 2.2 `ruleMatches(rule, node)`

```cpp
inline bool ruleMatches(const ThemeRule& r, const Layout* node) {
    if (r.chain.empty() || !node) return false;

    if (r.media.has_value() && !evaluateMedia(*r.media, Metrics::viewport))
        return false;

    if (!nodeMatchesSimple(node, r.chain.back()))
        return false;

    int i = (int)r.chain.size() - 2;
    const Layout* cur = node->getParent().get();
    while (i >= 0 && cur) {
        if (nodeMatchesSimple(cur, r.chain[i]))
            --i;
        cur = cur->getParent().get();
    }
    return i < 0;
}
```

The algorithm:

1. Evaluate the media query, if any. If false, no match.
2. Check the rightmost selector against the node.
3. Walk up the ancestor chain, trying to match each remaining selector
   in reverse order. If all are matched, the rule matches.

The ancestor walk is **greedy but not backtracking**: if a candidate
ancestor matches selector `i`, we advance `i`. If it doesn't, we keep
going up. This is the standard CSS descendant combinator semantics (a
descendant is "some ancestor", not "the immediate parent").

There's no backtracking, so a selector like `.a .a .b` will match
`.a .b` even with only one `.a` ancestor, because the first candidate
`.a` matches the second `.a` and there's no way to distinguish. This
is the same as CSS: `.a .a` matches a single `.a` ancestor.

### 2.3 Media queries

```cpp
if (r.media.has_value() && !evaluateMedia(*r.media, Metrics::viewport))
    return false;
```

`Metrics::viewport` is updated every `beginFrame`. So a resize changes
the viewport, which changes media evaluation, which changes which rules
match. The `viewportGeneration` counter ensures nodes re-resolve their
style on viewport change.

---

## 3. `StyleResolver::resolveFor(node)`

This is the main cascade. It builds a `ComputedStyle` for a node from
scratch.

### 3.1 Collect matching rules

```cpp
std::vector<Match> matches;
for (const auto& r : theme.rules) {
    if (!r.part.empty()) continue;         // skip ::part rules
    if (!ruleMatches(r, &node)) continue;
    matches.push_back({&r});
}
```

Skips `::part` rules — those are handled by `partFor`.

### 3.2 Sort by specificity then order

```cpp
std::stable_sort(matches.begin(), matches.end(),
    [](const Match& a, const Match& b) {
        if (a.rule->specificity != b.rule->specificity)
            return a.rule->specificity < b.rule->specificity;
        return a.rule->order < b.rule->order;
    });
```

Ascending specificity, and within the same specificity, ascending
declaration order. This means **later rules win** — the standard CSS
tie-break.

### 3.3 Build the cascade

```cpp
Style finalStyle;
finalStyle.overlay(inlineDefaults);
for (const auto& m : matches) finalStyle.overlay(m.rule->style);
finalStyle.overlay(inlineBase);
```

Four priority levels, each overlaying the previous:

1. `inlineDefaults` — the widget's "user agent" defaults.
2. CSS rules — in specificity order.
3. `inlineBase` — the widget's contract, wins over CSS.
4. State-specific inline styles (see §3.5).

### 3.4 Custom properties

```cpp
std::unordered_map<std::string, std::string> allCustomProps;
if (auto p = node.getParent())
    allCustomProps = p->getStyle().customProps;
else
    allCustomProps = theme.root.customProps;

for (const auto& [k, v] : finalStyle.customProps)
    allCustomProps[k] = v;
```

Custom props inherit from the parent, then merge with the node's own
(winning on conflict). Root has `theme.root.customProps` as its base.

### 3.5 Resolve `var()` references

```cpp
if (!finalStyle.unresolvedProps.empty()) {
    Style resolved = resolveUnresolvedProps(finalStyle.unresolvedProps,
                                             allCustomProps);
    finalStyle.overlay(resolved);
    finalStyle.unresolvedProps.clear();
}
```

`resolveUnresolvedProps` substitutes `var(--x)` with the value from
`allCustomProps`, then applies each property via `applyStyleAttr`. If
the substituted value parses, the property is set; otherwise it's left
unset (with a warning logged by the parser).

The substitution is iterative (up to 10 passes) to handle chained
`var()` references (`--a: var(--b)`).

### 3.6 State inline styles

```cpp
if (!node.getEnabled())
    finalStyle.overlay(inlineDisabled);
else {
    if (node.isHoveredState() || node.isPressedState())
        finalStyle.overlay(inlineHover);
    if (node.isPressedState())
        finalStyle.overlay(inlinePressed);
    if (node.isFocusedState())
        finalStyle.overlay(inlineFocus);
    if (node.isCheckedState())
        finalStyle.overlay(inlineChecked);
}
```

The state-specific inline styles are the widget's way of saying "when
in this state, my base style is X". They win over the CSS rules, but
not over the base `inlineBase`.

Order: `Disabled` is exclusive (a disabled node doesn't get hover or
pressed). Otherwise `Hover` (or `Pressed`, which implies hovered),
then `Pressed`, then `Focus`, then `Checked`.

### 3.7 Produce the `ComputedStyle`

```cpp
const ComputedStyle* parentStyle = nullptr;
if (auto p = node.getParent())
    parentStyle = &p->getStyle();

ComputedStyle result = ComputedStyle::from(finalStyle, parentStyle, &theme.root);
result.customProps = std::move(allCustomProps);
return result;
```

`ComputedStyle::from` applies the inheritance rules, the overflow
computed-value rule, and the defaults. The result is a fully-resolved
style ready to use.

### 3.8 When `resolveFor` runs

Three places:

1. **`initStyleIfNeeded`** — on the first frame, to initialize
   `currentStyle = targetStyle = transitionStartStyle`.
2. **`beginStateTransition`** — when the state machine changes state.
3. **`resolvePendingTransition`** — when `pendingTransition` was set
   (some property was changed via `getInlineBase()` or similar).
4. **`measure`** — if `pendingTransition` is still set, to get the
   target size.

The result is cached in `targetStyle`. `currentStyle` only changes when
a transition completes (or immediately, if there's no transition
duration).

---

## 4. Transitions

### 4.1 `beginStateTransition`

```cpp
void StyleResolver::beginStateTransition(Layout& node, UIState newState) {
    currentState = newState;
    transitionTimer = 0.0f;
    transitionStartStyle = currentStyle;
    targetStyle = resolveFor(node);
    node.pendingTransition = false;
}
```

Snapshot the current style, resolve the new target, reset the timer. The
state change is now in progress; `tick` will interpolate from
`transitionStartStyle` to `targetStyle` over the duration.

### 4.2 `resolvePendingTransition`

```cpp
void StyleResolver::resolvePendingTransition(Layout& node) {
    ComputedStyle newTarget = resolveFor(node);
    node.pendingTransition = false;

    if (newTarget != targetStyle) {
        if (node.anim_.hasActiveImperative()) {
            // An imperative animation rewrites the inline style every
            // frame, so the target changes every frame too. A CSS
            // transition would reset its timer on each of those
            // changes and never make progress. The animation itself
            // is the interpolation, so we snap currentStyle to the
            // target and let the animation drive the visual output.
            currentStyle = newTarget;
            targetStyle = std::move(newTarget);
            transitionStartStyle = currentStyle;
            transitionTimer = 1.0f;
        } else {
            transitionStartStyle = currentStyle;
            transitionTimer = 0.0f;
            targetStyle = std::move(newTarget);
        }
    } else {
        targetStyle = std::move(newTarget);
        if (transitionTimer >= 1.0f) {
            currentStyle = targetStyle;
            transitionStartStyle = currentStyle;
        }
    }
}
```

Similar to `beginStateTransition`, but only restarts the transition if
the new target differs from the current one. If the target is
unchanged, it just refreshes `targetStyle` and, if the transition was
already done, snaps `currentStyle` to it.

When an **imperative** animation is running
(`node.anim_.hasActiveImperative()` is true), the resolver skips the
transition entirely and sets `currentStyle = newTarget` immediately.
This is what allows imperative animations to produce visible motion:
the animation's track setters write into `inlineBase`, the resolver
re-resolves the target every frame, and `currentStyle` follows it
directly. Without this branch, every frame would restart the
transition and `currentStyle` would never reach the target. See
[Animation System](04-animation-system.md) §6.

### 4.3 `tick`

```cpp
void StyleResolver::tick(Layout&, float dt) {
    if (transitionTimer >= 1.0f) return;

    float maxDur = targetStyle.transitionTime;
    for (const auto& spec : targetStyle.transitions)
        maxDur = std::max(maxDur, spec.duration);

    if (maxDur <= 0.001f) {
        transitionTimer = 1.0f;
        currentStyle = targetStyle;
    } else {
        float elapsed = transitionTimer * maxDur;
        transitionTimer += dt / maxDur;
        if (transitionTimer >= 1.0f) {
            transitionTimer = 1.0f;
            currentStyle = targetStyle;
        } else {
            currentStyle = lerpStyleTimed(transitionStartStyle, targetStyle, elapsed);
        }
    }
}
```

The transition runs over the **maximum** duration of all declared
transitions. Each property interpolates with its own duration / easing
via `lerpStyleTimed`:

```cpp
ComputedStyle lerpStyleTimed(const ComputedStyle& a, const ComputedStyle& b,
                             float realElapsed)
{
    ComputedStyle r;

    auto timingFor = [&](const char* propName)
        -> std::pair<float, TransitionFunction>
    {
        for (const auto& spec : b.transitions)
            if (spec.prop == propName || spec.prop == "all")
                return {spec.duration, spec.ease};
        return {b.transitionTime, b.ease};
    };

    #define X(T, name, def)                                                    \
    {                                                                          \
        auto [dur, ease] = timingFor(#name);                                   \
        float tp = (dur > 0.0f) ? clamp(realElapsed / dur, 0.0f, 1.0f) : 1.0f; \
        r.name = lerpProp(a.name, b.name, getRatio(tp, ease));                 \
    }
    BUBBLE_STYLE_PROPS(X)
    #undef X

    r.transitions = b.transitions;
    r.customProps = b.customProps;
    return r;
}
```

Each property is interpolated using its own timing, sampled at
`realElapsed` (the global elapsed since the transition started). So a
property with duration 0.1s finishes at 0.1s, while another with 0.5s
continues until 0.5s. They all complete when `transitionTimer` reaches
1.0, which is when `realElapsed = maxDur`.

### 4.4 `lerpProp` variants

Interpolation depends on type:

- **`float`** — `a + (b - a) * t`.
- **`Color`** — per-channel.
- **`Value`** — term-by-term on the union of units. `auto` on either
  side causes a snap.
- **`Spacing`** — element-wise.
- **Enums, `ZIndex`, strings, `TextureRef`, `FilterRef` list, `BoxShadow`** —
  snap to `t > 0 ? b : a`.

The snap rule means a property that can't be smoothly interpolated
switches at the **midpoint** of the transition (since `t > 0` becomes
true immediately after `realElapsed > 0`). Actually, `t > 0.0f ? b : a`
returns `b` as soon as `t > 0`, which is the first tick after the
transition starts. So a snap happens on the first frame of the
transition. This is intentional — better than a late switch.

### 4.5 Transition and `pendingTransition`

The `pendingTransition` flag is set by:

- `getInlineBase()` (returns a mutable reference, so the caller will
  probably modify it).
- `setInlineBase(style)`.
- `addClass`, `setId`, `setSize`, `setEnabled`, `setChecked`,
  `setPortal`, `addChild`, `beginTransition`, `markInheritanceDirty`.
- `Text::setText`, `TextInput::setText`, `Slider` (via `pendingTransition`
  directly).

When `pendingTransition` is true at the start of `update`, the resolver
re-resolves the target style. If it differs from the current target,
the transition restarts.

The flag is cleared inside `resolvePendingTransition` and
`beginStateTransition`.

---

## 5. Inheritance

### 5.1 Inherited properties

Only five properties are inherited:

```cpp
inline bool isInheritedProp(std::string_view name) {
    return name == "font" ||
           name == "fontSize" ||
           name == "color" ||
           name == "letterSpacing" ||
           name == "textAlign";
}
```

`ComputedStyle::from` consults this function to decide whether to pull
the value from the parent.

### 5.2 `propagateInheritance`

```cpp
void StyleResolver::propagateInheritance(Layout& node) {
    bool changed =
        lastInherited.hovered    != node.isHoveredState() ||
        lastInherited.pressed    != node.isPressedState() ||
        lastInherited.focused    != node.isFocusedState() ||
        lastInherited.enabled    != node.getEnabled() ||
        lastInherited.checked    != node.isCheckedState() ||
        lastInherited.font       != currentStyle.font ||
        !(lastInherited.fontSize == currentStyle.fontSize) ||
        !(lastInherited.color == currentStyle.color) ||
        !(lastInherited.letterSpacing == currentStyle.letterSpacing) ||
        !(lastInherited.textAlign == currentStyle.textAlign);

    if (!changed) return;

    for (auto& c : node.children)
        c->markInheritanceDirty();

    // update lastInherited snapshot
    // ...
}
```

This runs after `style_.tick`. If any inherited property or state flag
changed since the last frame, each child is marked inheritance-dirty
via `markInheritanceDirty`:

```cpp
void markInheritanceDirty() {
    pendingTransition = true;
    subtreeDirty_ = true;
    for (auto& c : children)
        c->markInheritanceDirty();
}
```

The child's `pendingTransition` flag is set, so its own `update` will
re-resolve on the next frame (or the same frame, if it hasn't been
processed yet).

### 5.3 State flag propagation

State flags (`hovered`, `pressed`, `focused`, `enabled`, `checked`) are
part of the snapshot, because a change in a parent's state can affect
which rules match a **descendant**. For example:

```css
.card:hover .card-title { color: green; }
```

The `.card-title` node depends on its ancestor's `:hover` state. When
the card becomes hovered, the title's matching rules change, so its
style must be re-resolved. `propagateInheritance` triggers this by
marking the subtree dirty.

This is why the snapshot includes state flags — they're not really
"inherited" in the CSS sense, but they participate in descendant
selectors, so they need the same propagation mechanism.

### 5.4 Custom properties and inheritance

Custom properties also inherit (they're merged in `ComputedStyle::from`
via `parent->customProps`). But the propagation mechanism doesn't
explicitly track them — the change is detected indirectly because any
style change sets `pendingTransition` on the node, which re-resolves
its style and, if the custom props changed, produces a different
`ComputedStyle`, which then triggers propagation on the next frame.

This can cause a one-frame delay in custom property propagation in some
cases. In practice it's not observable.

---

## 6. `::part` styles

### 6.1 `partFor(node, partName)`

```cpp
Style StyleResolver::partFor(Layout& node, const std::string& partName) {
    auto& theme = Theme::get();

    std::vector<const ThemeRule*> matches;
    for (const auto& r : theme.rules) {
        if (r.part != partName) continue;
        if (!ruleMatches(r, &node)) continue;
        matches.push_back(&r);
    }

    std::stable_sort(matches.begin(), matches.end(),
        [](const ThemeRule* a, const ThemeRule* b) {
            if (a->specificity != b->specificity)
                return a->specificity < b->specificity;
            return a->order < b->order;
        });

    Style target;
    for (const auto* r : matches) target.overlay(r->style);

    // transition + animation overlay (see below)

    return result;
}
```

Filters rules by `part` name, matches them against the node (using the
same `ruleMatches` as the main cascade), and overlays in specificity
order. The result is a `Style` (not a `ComputedStyle`) — parts are not
inherited and don't interact with the parent's cascade.

### 6.2 Part transitions

Each part has its own transition state:

```cpp
struct PartTransition {
    bool initialized{false};
    Style target;
    Style current;
    double startTime{0.0};
    float duration{0.0f};
    TransitionFunction ease{TransitionFunction::Linear};
    bool active{false};
};

std::unordered_map<std::string, PartTransition> partTransitions;
```

Keyed by part name, so a widget with `track`, `fill`, and `knob` parts
has three independent transition states.

When `partFor` is called:

1. If the part has no state, initialize it (`target = current = target`).
2. If the target differs from last frame's target, start a transition.
3. Tick the transition based on `now - startTime`.
4. Interpolate `current → target` using the transition timer.

The part uses the **absolute time** from `UIContext::get().time`
(`double`), not the incremental `dt`. This is why parts use `tickAbsolute`
in `Anim` — the animation elapsed is `now - startTime`.

### 6.3 Part animations

After the transition, `Anim::applyCssToPart` overlays any CSS keyframe
animation:

```cpp
node.anim_.applyCssToPart(node, partName, result);
```

That method:

1. Reads `out.animations` (the part's declared animations).
2. Syncs the active animation list against them.
3. Ticks each active animation with the absolute time.
4. Overlays the current keyframe values onto `result`.

So a part style is: static rules + transition interpolation + keyframe
overlay.

### 6.4 When `partFor` is called

Only from `renderContent` (or `renderChrome`) of a widget, via
`partStyle(partName)`:

```cpp
protected:
    Style partStyle(const std::string& partName) {
        return style_.partFor(*this, partName);
    }
```

Each call re-resolves the part's style, ticks its transition, and
overlays its animations. This means **the part's state advances every
time `partStyle` is called**, which for a well-behaved widget is once
per frame per part.

If a widget calls `partStyle("track")` twice in the same frame, the
second call sees the same state as the first (the transition tick is
idempotent within a frame — it reads `now` from `UIContext::time`,
which doesn't advance during a frame).

If a widget **doesn't** call `partStyle` on a frame, its parts don't
tick. A widget that hides a part (e.g. `Checkbox::mark` when unchecked)
skips the call, so the part's transition doesn't advance — but since
it's not drawn either, nobody notices.

### 6.5 Parts are not nodes

A `::part` is not a `Layout`. There's no hit-testing, no layout, no
children. It's a styling surface that a widget paints. `renderContent`
reads the part style and paints accordingly.

This is why `Slider::knob` doesn't have a `rect` — its geometry is
computed by `renderContent` from the node's own `rect` and the current
value. The part only contributes style (color, radius, animations).

---

## 7. `stylesDiffer` and style comparison

`stylesDiffer(a, b)` compares two `Style`s (not `ComputedStyle`s):

```cpp
inline bool stylesDiffer(const Style& a, const Style& b) {
    #define X(T, name, def)                    \
    if (a.name.is_set != b.name.is_set) return true; \
    if (a.name.is_set && !(a.name.value == b.name.value)) return true;
    BUBBLE_STYLE_PROPS(X)
    #undef X

    // animations comparison (size + each field)
    // ...
    return false;
}
```

Used by `partFor` to decide whether to start a part transition. It's
stricter than `ComputedStyle::operator==` because it distinguishes
"unset" from "set to the default" — important for part inheritance,
where "unset" means "fall back to the previous part's value".

`ComputedStyle::operator==` is used by `recomputeDirty` to detect
whether the resolved style changed. It compares all fields (which are
always set in `ComputedStyle`), so it detects value changes but not
"setness" changes — but by that point, setness has been resolved to
concrete values anyway.

---

## 8. The `pendingTransition` flag

`pendingTransition` is a per-node flag that means "something about my
style changed, re-resolve on the next update". It's set by every setter
that could affect the resolved style:

- `getInlineBase()` — returns a mutable reference, so any modification
  will require a re-resolve.
- `getInlineDefaults()` — same.
- `setInlineBase(style)` — merges a `Style` into `inlineBase`.
- `addClass`, `setId`, `setStyleTag` — changes selectors that match.
- `setSize`, `setEnabled`, `setChecked`, `setPortal` — changes state
  or inline values.
- `addChild`, `removeFromParent` (via the child's flag) — affects the
  layout.
- `beginTransition` — explicit "start a transition from the current
  style".
- `markInheritanceDirty` — propagates from a parent's change.
- `Text::setText`, `TextInput::setText`, `Slider::setValue`,
  `ProgressBar::setValue` — content changes.
- `Dropdown::setSelected` — updates the button's text.

The flag is cleared in `beginStateTransition` and
`resolvePendingTransition`, both of which run inside `update`.

The subtlety: `getInlineBase()` is used by animation track setters:

```cpp
anim->addTrack<float>(0.0f, 1.0f,
    [](Layout* l, float v) { l->getInlineBase().opacity = v; });
```

Every frame that the animation ticks, the setter calls
`getInlineBase()`, which sets `pendingTransition = true`. This is
intentional: the animation changes the inline style, so the node needs
to re-resolve. The overhead is small (one re-resolve per frame per
animated node).

`tickImperative` runs **before** the style check in `update`, so within
the same frame the `pendingTransition` set by the animation is picked
up by the `else if (pendingTransition)` branch:

```cpp
if (stateChanged) {
    style_.beginStateTransition(*this, nextState);
    anim_.syncCss(*this);
} else if (pendingTransition) {
    style_.resolvePendingTransition(*this);
    anim_.syncCss(*this);
}
```

`resolvePendingTransition` sees that an imperative animation is
active and snaps `currentStyle` to the new target instead of starting
a transition (see §4.2). The result is that the animated value is
visible in the **same** frame it was written: the animation's setter
runs, `pendingTransition` is set, the target is re-resolved, and
`currentStyle` is updated to match. `renderStyle` at draw time then
contains the interpolated value, since it's a copy of `currentStyle`
with the CSS keyframe overlay applied on top.

### 8.1 Flag lifecycle

Without an imperative animation running:

```
Frame N, update:
├── tickImperative: no playing animation → nothing happens
├── ...
├── pendingTransition might be set by a setter called earlier in update
├── resolvePendingTransition runs, if pendingTransition is true
│   ├── target differs → start a transition (timer = 0)
│   ├── target same → refresh target, snap if transition done
│   └── clears pendingTransition
├── style.tick advances the transition (if any)
└── draw uses currentStyle + css overlay
```

With an imperative animation running:

```
Frame N, update:
├── tickImperative: writes to inlineBase → sets pendingTransition
├── ...
├── resolvePendingTransition runs
│   ├── sees hasActiveImperative() == true
│   ├── sets currentStyle = newTarget (snap, no transition)
│   └── clears pendingTransition
├── style.tick: transitionTimer already 1.0 → no-op
└── draw uses currentStyle (already at the animated value)
```

The animation continuously sets `pendingTransition` and the resolver
continuously clears it, snapping `currentStyle` to the animated value
on each frame. Once the animation finishes (`hasActiveImperative()`
becomes false), the next style change goes through the normal
transition path again.

---

## 9. Common tasks

### 9.1 Adding a new property

1. Add one line to `BUBBLE_STYLE_PROPS(X)`:
   ```cpp
   X(float, myProp, 0.0f)
   ```
2. If the property should be interpolated, add a `lerpProp` overload
   for its type if it doesn't exist. `float`, `Color`, `Value`,
   `Spacing` already have overloads. Enums snap.
3. If the property should be inherited, add it to `isInheritedProp`.
4. Add the parser case in `applyStyleAttr` (`StyleAttr.hpp`).
5. If needed, add a `Debug::toString` overload for the type.

Everything else (overlay, comparison, dump, prop table) updates
automatically.

### 9.2 Adding a new selector feature

1. Add a field to `SimpleSelector` (e.g. `requireActive`).
2. Update `nodeMatchesSimple` to check it.
3. Update `computeSpecificity` if the new feature contributes to
   specificity.
4. Update the parser in `StyleParser.hpp` to recognize it.
5. Update `Debug::selectorToString` to print it.

The matching and sorting code is feature-agnostic, so no changes there.

### 9.3 Adding a new state

1. Add the flag to `Layout` (like `isFocused`).
2. Update `InputController::updateFlags` to set it.
3. Add `requireX` to `SimpleSelector` and `nodeMatchesSimple`.
4. Update the parser to recognize the pseudo-class.
5. Add an `inlineX` to `StyleResolver` if there's a base style for the
   state.
6. Update `resolveFor` to overlay `inlineX` when the state is active.

---

## 10. Pitfalls

**Styles are re-resolved on every `pendingTransition`.**
The resolver does the full cascade (matching all rules, sorting,
overlaying, resolving `var()`) on every `update` where the node's
`pendingTransition` is true. For a typical node, this is once at
initialization and then only when the state changes or a property is
set. For an animated node, it's once per frame. `Theme::rules` is a
linear list — a stylesheet with thousands of rules will make resolution
slow for nodes that change state frequently.

There's no rule indexing by tag or class. If you need to optimize, the
first step is to bucket `Theme::rules` by tag so `resolveFor` only
checks the relevant subset.

**Specificity is a struct, not a single number.**
`(ids, classes, tags)` is compared lexicographically. This means
`(1, 0, 0)` beats `(0, 999, 999)`. Correct, matches CSS.

**`:hover` doesn't propagate to `::part` by default.**
A part has its own state check against the node. So
`Slider:hover::knob` works because the rule's `chain` is `Slider` with
`requireHover`, and the rule's `part` is `"knob"`. But
`Slider:hover .knob` (no `::`) would be a descendant selector — a
different thing entirely. Parts use `::`, not descendant.

**`var()` in a shorthand.**
```css
transition: var(--my-transition);
```
This isn't supported. `transition` and `animation` are parsed as
shorthands directly in the parser, before `var()` substitution happens.
`unresolvedProps` handling only applies to `applyStyleAttr`-style
properties, not the special-cased shorthands. If you need
parameterized transitions, define the whole thing in a rule.

**`ComputedStyle::from` always picks a parent value for inherited props.**
If a parent exists, the parent's computed value wins over the root
style and the default. This means you can't "un-inherit" a property —
once the parent has a value, children inherit it unless they set it
themselves. This is CSS behavior.

**`inlineDefaults` is not the same as "the default".**
The default is what `ComputedStyle::from` uses when nothing else is
set. `inlineDefaults` is an explicit override that the widget applies
via `getInlineDefaults()`, and it wins over nothing — it's the lowest
priority in the cascade. To use it correctly, put "reasonable defaults
the user should be able to override" there. To use it wrong, put
"contract" values there and expect CSS to override them (it won't, if
the CSS rule has lower specificity).

**`propagateInheritance` doesn't compare all properties.**
Only the inherited ones plus the state flags. Custom properties and
`transitions` / `animations` are not compared. A change to a custom
property on a parent will propagate via the normal `pendingTransition`
mechanism (the parent's style changes, and `ComputedStyle::from` for
the child reads the parent's `customProps`), but not via
`propagateInheritance`. In practice, this is fine because the child
re-resolves anyway when its parent does.

**`partFor` matches against the node, not the part.**
A rule like `Slider:hover::knob { ... }` matches against the `Slider`
node with `:hover` required. The part name is just a label. So you
can't have a rule that matches "the knob when the knob is hovered" —
the part has no independent state.

**Snap interpolation has no easing.**
`lerpProp(enum, enum, t)` returns `t > 0 ? b : a`. So an enum
transition switches on the first tick, ignoring the easing. If you want
a "delayed" snap, you'd need a custom `lerpProp` that samples the
easing function.

**`Value` interpolation with different units can produce weird
intermediates.**
`lerpProp(Value, Value, t)` merges units term-by-term. `Px(0)` and
`Percent(100)` interpolated at `t = 0.5` becomes `{0.5*0 px + 0.5*100 %}`,
which resolves to `50%` at draw time. If the parent width is 200, that's
100px — visually halfway between `0px` and `100%` at 200px parent, which
is `100px`. So it works. But if the parent width changes during the
transition, the intermediate value changes too, which can look jittery.

For clean interpolation, keep both endpoints in the same unit..