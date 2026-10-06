#pragma once

#include "Common.hpp"
#include "Logger.hpp"
#include "CoreTypes.hpp"
#include "Easing.hpp"
#include "Unit.hpp"
#include "Style.hpp"
#include "Layout.hpp"
#include "UIComponents.hpp"
#include "StyleAttr.hpp"

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

        if (el.has("passthrough")) node->setPassThrough(el.attrBool("passthrough"));

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

    r.reg("Stack", [](const Element&) {
        auto l = std::make_shared<Layout>(LayoutType::Stack);
        l->setStyleTag("Stack");
        return l;
    });
    r.reg("Spacer", [](const Element&) {
        auto l = std::make_shared<Layout>(LayoutType::Stack);
        l->setStyleTag("Spacer");
        return l;
    });
    r.reg("VStack",  [](const Element&)   { return VStack(); });
    r.reg("HStack",  [](const Element&)   { return HStack(); });

    r.reg("Text", [](const Element& e) {
        auto t = Text::create(e.text);
        if (e.has("wrap")) t->setWrap(e.attrBool("wrap", true));
        return t;
    });
    r.reg("Label", [](const Element& e) {
        auto t = Text::create(e.text);
        if (e.has("wrap")) t->setWrap(e.attrBool("wrap", true));
        return t;
    });

    r.reg("TextInput", [](const Element& e) {
        return TextInput::create(e.attr("value", e.text));
    });
    r.reg("Checkbox", [](const Element& e) {
        return Checkbox::create(e.attrBool("checked", false));
    });
    r.reg("ProgressBar", [](const Element& e) {
        return ProgressBar::create(e.attrFloat("value", 0.0f));
    });
    r.reg("Dropdown", [](const Element& e) {
        std::vector<std::string> opts;
        std::string cur;
        auto flush = [&]() {
            size_t a = cur.find_first_not_of(" \t");
            size_t b = cur.find_last_not_of(" \t");
            if (a != std::string::npos) opts.push_back(cur.substr(a, b - a + 1));
            cur.clear();
        };
        for (char c : e.attr("options", "")) {
            if (c == ',') flush(); else cur.push_back(c);
        }
        flush();
        if (opts.empty()) opts.push_back("Default");
        return Dropdown::create(std::move(opts), 0);
    });
    r.reg("Button",  [](const Element& e) { return Btn(e.text); });
    r.reg("Panel",   [](const Element&)   { return Pan(); });
    r.reg("Slider",  [](const Element& e) { return Slider::create(e.attrFloat("value", 0.5f)); });
    r.reg("Toggle",  [](const Element& e) { return Toggle::create(e.attrBool("checked", false)); });
    r.reg("ScrollView", [](const Element&) { return ScrollView::create(); });
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