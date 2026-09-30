#pragma once

#include "UI.hpp"

#include <charconv>
#include <optional>
#include <sstream>

namespace ZenitUI::ZMarkup {

// =========================================================================
//  AST
// =========================================================================
struct Element {
    std::string tag;
    std::vector<std::string> classes;
    std::string id;
    std::string text;
    std::vector<std::pair<std::string, std::string>> attrs;
    std::vector<Element> children;

    std::string attr(std::string_view k, std::string_view d = "") const {
        for (auto& [key, val] : attrs) if (key == k) return val;
        return std::string(d);
    }
    bool has(std::string_view k) const {
        for (auto& [key, _] : attrs) if (key == k) return true;
        return false;
    }
    float attrFloat(std::string_view k, float d = 0.0f) const {
        auto v = attr(k);
        if (v.empty()) return d;
        try { return std::stof(v); } catch (...) { return d; }
    }
    bool attrBool(std::string_view k, bool d = false) const {
        auto v = attr(k);
        if (v.empty()) return d;
        return v == "true" || v == "1" || v == "yes";
    }
};

// =========================================================================
//  Parser
// =========================================================================
class Parser {
public:
    explicit Parser(std::string_view src) : s(src) {}

    Element parseRoot() {
        skipWs();
        return parseElement();
    }

private:
    std::string_view s;
    size_t p{ 0 };

    bool eof() const { return p >= s.size(); }
    char peek() const { return eof() ? '\0' : s[p]; }
    char get() { return eof() ? '\0' : s[p++]; }

    static bool isIdentStart(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    static bool isIdentChar(char c) {
        return isIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
    }

    void skipWs() {
        while (!eof()) {
            char c = s[p];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { ++p; continue; }
            if (c == '/' && p + 1 < s.size() && s[p + 1] == '/') {
                while (p < s.size() && s[p] != '\n') ++p;
                continue;
            }
            break;
        }
    }
    void skipInlineWs() {
        while (!eof() && (s[p] == ' ' || s[p] == '\t')) ++p;
    }

    std::string readIdent() {
        std::string out;
        while (!eof() && isIdentChar(s[p])) out.push_back(s[p++]);
        return out;
    }

    std::string readQuoted() {
        std::string out;
        if (peek() != '"') return out;
        ++p;
        while (!eof() && s[p] != '"') {
            if (s[p] == '\\' && p + 1 < s.size()) {
                ++p;
                char c = s[p++];
                if      (c == 'n') out.push_back('\n');
                else if (c == 't') out.push_back('\t');
                else               out.push_back(c);
            } else {
                out.push_back(s[p++]);
            }
        }
        if (!eof()) ++p;
        return out;
    }

    std::string readBareToken() {
        std::string out;
        while (!eof()) {
            char c = s[p];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n'
             || c == '{' || c == '}' || c == '"') break;
            out.push_back(c);
            ++p;
        }
        return out;
    }

    Element parseElement() {
        Element e;
        e.tag = readIdent();

        // Modifiers: .class, #id (concatenabili, nell'ordine che vuoi)
        while (peek() == '.' || peek() == '#') {
            char m = get();
            std::string name = readIdent();
            if (m == '.') e.classes.push_back(std::move(name));
            else          e.id = std::move(name);
        }

        // Attributes
        while (true) {
            skipInlineWs();
            char c = peek();
            if (c == '"' || c == '{' || c == '}' || c == '\0' || c == '\n' || c == '\r') break;
            if (!isIdentStart(c)) break;

            std::string key = readIdent();
            skipInlineWs();
            if (peek() == '=') {
                ++p;
                skipInlineWs();
                std::string val = (peek() == '"') ? readQuoted() : readBareToken();
                e.attrs.emplace_back(std::move(key), std::move(val));
            } else {
                e.attrs.emplace_back(std::move(key), "true"); // flag
            }
        }

        // Testo (opzionale)
        skipInlineWs();
        if (peek() == '"') e.text = readQuoted();

        // Blocco figli (opzionale)
        skipInlineWs();
        if (peek() == '{') {
            ++p;
            while (true) {
                skipWs();
                if (eof() || peek() == '}') break;
                e.children.push_back(parseElement());
            }
            if (peek() == '}') ++p;
        }
        return e;
    }
};

inline Element parse(std::string_view src) {
    Parser p(src);
    return p.parseRoot();
}

// =========================================================================
//  Value parsers (li riuserai anche per il CSS degli stili)
// =========================================================================
inline std::optional<Value> parseValueToken(std::string_view s) {
    if (s.empty()) return std::nullopt;
    if (s == "auto") return Value::autoSize();

    size_t n = 0;
    while (n < s.size() && (std::isdigit((unsigned char)s[n]) || s[n] == '.' || s[n] == '-' || s[n] == '+')) ++n;

    float amount = 0.0f;
    try { amount = std::stof(std::string(s.substr(0, n))); } catch (...) { return std::nullopt; }

    std::string_view unit = s.substr(n);
    if (unit.empty() || unit == "px") return Px(amount);
    if (unit == "%")  return Percent(amount);
    if (unit == "vw") return VW(amount);
    if (unit == "vh") return VH(amount);
    return std::nullopt;
}

inline std::optional<Color> parseColorToken(std::string_view s) {
    if (s == "white")       return Colors::White;
    if (s == "black")       return Colors::Black;
    if (s == "transparent") return Colors::Blank;
    if (s == "blank")       return Colors::Blank;
    if (s == "red")         return Colors::Red;
    if (s == "maroon")      return Colors::Maroon;
    if (s == "green")       return Colors::Green;
    if (s == "blue")        return Colors::Blue;
    if (s == "yellow")      return Colors::Yellow;
    if (s == "gray")        return Colors::Gray;
    if (s == "darkgray")    return Colors::DarkGray;
    if (s == "lightgray")   return Colors::LightGray;

    if (!s.empty() && s[0] == '#') {
        std::string h(s.substr(1));
        auto hex = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        if (h.size() == 3)
            return Color{ (uint8_t)(hex(h[0]) * 17), (uint8_t)(hex(h[1]) * 17), (uint8_t)(hex(h[2]) * 17), 255 };
        if (h.size() == 4)
            return Color{ (uint8_t)(hex(h[0]) * 17), (uint8_t)(hex(h[1]) * 17), (uint8_t)(hex(h[2]) * 17), (uint8_t)(hex(h[3]) * 17) };
        if (h.size() == 6)
            return Color{ (uint8_t)(hex(h[0]) * 16 + hex(h[1])), (uint8_t)(hex(h[2]) * 16 + hex(h[3])), (uint8_t)(hex(h[4]) * 16 + hex(h[5])), 255 };
        if (h.size() == 8)
            return Color{ (uint8_t)(hex(h[0]) * 16 + hex(h[1])), (uint8_t)(hex(h[2]) * 16 + hex(h[3])), (uint8_t)(hex(h[4]) * 16 + hex(h[5])), (uint8_t)(hex(h[6]) * 16 + hex(h[7])) };
    }
    return std::nullopt;
}

inline std::optional<Spacing> parseSpacingToken(std::string_view s) {
    std::vector<Value> vals;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && s[i] == ' ') ++i;
        size_t start = i;
        while (i < s.size() && s[i] != ' ') ++i;
        if (start == i) break;
        auto v = parseValueToken(s.substr(start, i - start));
        if (!v) return std::nullopt;
        vals.push_back(*v);
    }
    if (vals.size() == 1) return Spacing(vals[0]);
    if (vals.size() == 2) return Spacing(vals[0], vals[1]);
    if (vals.size() == 3) return Spacing(vals[0], vals[1], vals[2]);
    if (vals.size() == 4) return Spacing(vals[0], vals[1], vals[2], vals[3]);
    return std::nullopt;
}

inline std::optional<Align> parseAlignToken(std::string_view s) {
    if (s == "auto")    return Align::Auto;
    if (s == "start")   return Align::Start;
    if (s == "center")  return Align::Center;
    if (s == "end")     return Align::End;
    if (s == "stretch") return Align::Stretch;
    return std::nullopt;
}

inline std::optional<Justify> parseJustifyToken(std::string_view s) {
    if (s == "start")         return Justify::Start;
    if (s == "center")        return Justify::Center;
    if (s == "end")           return Justify::End;
    if (s == "space-between") return Justify::SpaceBetween;
    return std::nullopt;
}

// =========================================================================
//  Applicazione attributi → inlineBase dello Style
// =========================================================================
inline void applyStyleAttr(Style& st, const std::string& key, const std::string& val) {
    if      (key == "width")          { if (auto v = parseValueToken(val))   st.width = *v; }
    else if (key == "height")         { if (auto v = parseValueToken(val))   st.height = *v; }
    else if (key == "min-width")      { if (auto v = parseValueToken(val))   st.minWidth = *v; }
    else if (key == "max-width")      { if (auto v = parseValueToken(val))   st.maxWidth = *v; }
    else if (key == "min-height")     { if (auto v = parseValueToken(val))   st.minHeight = *v; }
    else if (key == "max-height")     { if (auto v = parseValueToken(val))   st.maxHeight = *v; }
    else if (key == "gap")            { if (auto v = parseValueToken(val))   st.gap = *v; }
    else if (key == "grow")           { try { st.grow = std::stof(val); } catch (...) {} }
    else if (key == "padding")        { if (auto v = parseSpacingToken(val)) st.padding = *v; }
    else if (key == "margin")         { if (auto v = parseSpacingToken(val)) st.margin = *v; }
    else if (key == "background")     { if (auto v = parseColorToken(val))   st.background = *v; }
    else if (key == "color")          { if (auto v = parseColorToken(val))   st.color = *v; }
    else if (key == "tint")           { if (auto v = parseColorToken(val))   st.tint = *v; }
    else if (key == "border-color")   { if (auto v = parseColorToken(val))   st.borderColor = *v; }
    else if (key == "border-width")   { if (auto v = parseValueToken(val))   st.borderWidth = *v; }
    else if (key == "radius")         { if (auto v = parseValueToken(val))   st.radius = *v; }
    else if (key == "opacity")        { try { st.opacity = std::stof(val); } catch (...) {} }
    else if (key == "scale")          { try { st.scale = std::stof(val); } catch (...) {} }
    else if (key == "rotation")       { try { st.rotation = std::stof(val); } catch (...) {} }
    else if (key == "translate-x")    { if (auto v = parseValueToken(val))   st.translateX = *v; }
    else if (key == "translate-y")    { if (auto v = parseValueToken(val))   st.translateY = *v; }
    else if (key == "font-size")      { if (auto v = parseValueToken(val))   st.fontSize = *v; }
    else if (key == "letter-spacing") { if (auto v = parseValueToken(val))   st.letterSpacing = *v; }
    else if (key == "items-h")        { if (auto v = parseAlignToken(val))   st.itemsH = *v; }
    else if (key == "items-v")        { if (auto v = parseAlignToken(val))   st.itemsV = *v; }
    else if (key == "align-h")        { if (auto v = parseAlignToken(val))   st.alignH = *v; }
    else if (key == "align-v")        { if (auto v = parseAlignToken(val))   st.alignV = *v; }
    else if (key == "justify")        { if (auto v = parseJustifyToken(val)) st.justify = *v; }
    else if (key == "transition-time"){ try { st.transitionTime = std::stof(val); } catch (...) {} }
    else if (key == "ease")           { TransitionFunction tf; if (parseEasing(val, tf)) st.ease = tf; }
    // altri attributi (class, id, value, checked, ...) sono gestiti altrove
}

// =========================================================================
//  BuildContext — tiene traccia di nodi per id (per gli hook)
// =========================================================================
struct BuildContext {
    std::unordered_map<std::string, std::shared_ptr<Layout>> byId;
};

// =========================================================================
//  Registry — mappa tag → factory
// =========================================================================
class Registry {
public:
    using Factory = std::function<std::shared_ptr<Layout>(const Element&)>;

    void reg(const std::string& tag, Factory f) { factories[tag] = std::move(f); }

    std::shared_ptr<Layout> create(const Element& el, BuildContext& ctx) {
        auto it = factories.find(el.tag);
        if (it == factories.end()) return nullptr;

        auto node = it->second(el);
        if (!node) return nullptr;

        for (auto& c : el.classes) node->cls(c);
        if (!el.id.empty()) { node->id(el.id); ctx.byId[el.id] = node; }

        for (auto& [k, v] : el.attrs) applyStyleAttr(node->getInlineBase(), k, v);

        for (auto& child : el.children) {
            auto c = create(child, ctx);
            if (c) node->addChild(c);
        }
        return node;
    }

private:
    std::unordered_map<std::string, Factory> factories;
};

// =========================================================================
//  Componenti built-in
// =========================================================================
inline void registerBuiltins(Registry& r) {
    using namespace ZenitUI::UI;

    r.reg("Stack",   [](const Element&)   { return std::make_shared<Layout>(LayoutType::Stack); });
    r.reg("VStack",  [](const Element&)   { return VStack(); });
    r.reg("HStack",  [](const Element&)   { return HStack(); });
    r.reg("Spacer",  [](const Element&)   { return std::make_shared<Layout>(LayoutType::Stack); });
    r.reg("Text",    [](const Element& e) { return Label(e.text); });
    r.reg("Label",   [](const Element& e) { return Label(e.text); });
    r.reg("Button",  [](const Element& e) { return Btn(e.text); });
    r.reg("Panel",   [](const Element&)   { return Pan(); });
    r.reg("Slider",  [](const Element& e) { return std::make_shared<Slider>(e.attrFloat("value", 0.5f)); });
    r.reg("Toggle",  [](const Element& e) { return std::make_shared<Toggle>(e.attrBool("checked", false)); });
}

// =========================================================================
//  UINode — handle di ritorno: permette find + hook tipizzati
// =========================================================================
class UINode {
public:
    UINode(std::shared_ptr<Layout> root, std::shared_ptr<BuildContext> ctx)
        : root_(std::move(root)), ctx_(std::move(ctx)) {}

    std::shared_ptr<Layout> root() const { return root_; }

    std::shared_ptr<Layout> find(const std::string& id) const {
        auto it = ctx_->byId.find(id);
        return it != ctx_->byId.end() ? it->second : nullptr;
    }

    template <typename T = Layout>
    std::shared_ptr<T> find(const std::string& id) const {
        auto n = find(id);
        return n ? std::static_pointer_cast<T>(n) : nullptr;
    }

    UINode& onClick(const std::string& id, std::function<void()> cb) {
        if (auto n = find(id)) n->onClick = std::move(cb);
        return *this;
    }
    UINode& onPress(const std::string& id, std::function<void()> cb) {
        if (auto n = find(id)) n->onPress = std::move(cb);
        return *this;
    }
    UINode& onRelease(const std::string& id, std::function<void()> cb) {
        if (auto n = find(id)) n->onRelease = std::move(cb);
        return *this;
    }
    UINode& onHoverEnter(const std::string& id, std::function<void()> cb) {
        if (auto n = find(id)) n->onHoverEnter = std::move(cb);
        return *this;
    }
    UINode& onHoverExit(const std::string& id, std::function<void()> cb) {
        if (auto n = find(id)) n->onHoverExit = std::move(cb);
        return *this;
    }
    UINode& onValueChanged(const std::string& id, std::function<void(float)> cb) {
        if (auto s = find<UI::Slider>(id)) s->onValueChanged = std::move(cb);
        return *this;
    }
    UINode& onToggle(const std::string& id, std::function<void(bool)> cb) {
        if (auto t = find<UI::Toggle>(id)) t->onToggle = std::move(cb);
        return *this;
    }

private:
    std::shared_ptr<Layout> root_;
    std::shared_ptr<BuildContext> ctx_;
};

// =========================================================================
//  Entry point
// =========================================================================
inline UINode build(std::string_view source) {
    static Registry registry;
    static bool initialized = false;
    if (!initialized) { registerBuiltins(registry); initialized = true; }

    auto ctx = std::make_shared<BuildContext>();
    auto el = parse(source);
    auto root = registry.create(el, *ctx);
    return UINode(root, ctx);
}

} // namespace ZenitUI::ZMarkup