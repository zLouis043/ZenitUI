#pragma once

#include "Common.hpp"
#include "Logger.hpp"
#include "CoreTypes.hpp"
#include "Easing.hpp"
#include "Unit.hpp"
#include "Style.hpp"

#include <charconv>
#include <optional>
#include <sstream>

namespace ZenitUI {

// =========================================================================
//  Parsers CSS-style: valore, colore, spacing, align, justify.
//  Estratti da ZMarkup per essere riusabili da StyleParser e StyleResolver.
// =========================================================================

namespace detail {

struct CalcToken {
    enum class Kind { Number, Ident, Percent, Plus, Minus, Star, Slash, LParen, RParen, End };
    Kind kind{ Kind::End };
    float num{ 0.0f };
    std::string ident;
};

inline std::vector<CalcToken> tokenizeCalc(std::string_view s) {
    std::vector<CalcToken> out;
    size_t i = 0;
    while (i < s.size()) {
        char c = s[i];
        if (std::isspace((unsigned char)c)) { ++i; continue; }
        if (c == '+') { out.push_back({CalcToken::Kind::Plus});   ++i; continue; }
        if (c == '-') { out.push_back({CalcToken::Kind::Minus});  ++i; continue; }
        if (c == '*') { out.push_back({CalcToken::Kind::Star});   ++i; continue; }
        if (c == '/') { out.push_back({CalcToken::Kind::Slash});  ++i; continue; }
        if (c == '(') { out.push_back({CalcToken::Kind::LParen}); ++i; continue; }
        if (c == ')') { out.push_back({CalcToken::Kind::RParen}); ++i; continue; }
        if (c == '%') { out.push_back({CalcToken::Kind::Percent});++i; continue; }

        if (std::isdigit((unsigned char)c) || c == '.') {
            size_t start = i;
            while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '.')) ++i;
            float v = 0.0f;
            try { v = std::stof(std::string(s.substr(start, i - start))); } catch (...) {}
            CalcToken t; t.kind = CalcToken::Kind::Number; t.num = v;
            out.push_back(t);
            continue;
        }
        if (std::isalpha((unsigned char)c) || c == '_') {
            size_t start = i;
            while (i < s.size() && (std::isalnum((unsigned char)s[i]) || s[i] == '_')) ++i;
            CalcToken t; t.kind = CalcToken::Kind::Ident;
            t.ident = std::string(s.substr(start, i - start));
            out.push_back(t);
            continue;
        }
        ++i;
    }
    out.push_back({CalcToken::Kind::End});
    return out;
}

class CalcParser {
public:
    explicit CalcParser(const std::vector<CalcToken>& toks) : toks(toks) {}

    bool parse(Value& out) {
        out = parseExpr();
        return !errored && pos < toks.size() && toks[pos].kind == CalcToken::Kind::End;
    }

private:
    const std::vector<CalcToken>& toks;
    size_t pos{ 0 };
    bool errored{ false };

    const CalcToken& peek() const { return toks[pos]; }
    void advance() { if (pos + 1 < toks.size()) ++pos; }
    void error()   { errored = true; }

    Value parseExpr() {
        Value left = parseTerm();
        while (!errored && (peek().kind == CalcToken::Kind::Plus ||
                            peek().kind == CalcToken::Kind::Minus)) {
            auto op = peek().kind;
            advance();
            Value right = parseTerm();
            left = (op == CalcToken::Kind::Plus) ? (left + right) : (left - right);
        }
        return left;
    }

    Value parseTerm() {
        Value left = parseFactor();
        while (!errored && (peek().kind == CalcToken::Kind::Star ||
                            peek().kind == CalcToken::Kind::Slash)) {
            auto op = peek().kind;
            advance();
            Value right = parseFactor();
            left = (op == CalcToken::Kind::Star) ? left.mulWith(right)
                                                 : left.divWith(right);
        }
        return left;
    }

    Value parseFactor() {
        if (errored) return Value::px(0.0f);
        if (peek().kind == CalcToken::Kind::Minus) { advance(); return -parseFactor(); }
        if (peek().kind == CalcToken::Kind::Plus)  { advance(); return  parseFactor(); }
        if (peek().kind == CalcToken::Kind::LParen) {
            advance();
            Value v = parseExpr();
            if (peek().kind != CalcToken::Kind::RParen) { error(); return v; }
            advance();
            return v;
        }
        if (peek().kind == CalcToken::Kind::Number) {
            float num = peek().num;
            advance();
            if (peek().kind == CalcToken::Kind::Percent) {
                advance();
                return Value::percent(num);
            }
            if (peek().kind == CalcToken::Kind::Ident) {
                const std::string& u = peek().ident;
                advance();
                if (u == "px") return Value::px(num);
                if (u == "vw") return Value::vw(num);
                if (u == "vh") return Value::vh(num);
                if (u == "pw") return Value::pw(num);
                if (u == "ph") return Value::ph(num);
                return Value::px(num);
            }
            return Value::number(num);
        }
        error();
        return Value::px(0.0f);
    }
};

} // namespace detail

inline std::optional<Value> parseValueToken(std::string_view s) {
    if (s.empty()) return std::nullopt;
    if (s == "auto") return Value::autoSize();

    if (s.size() >= 6 && s.substr(0, 5) == "calc(" && s.back() == ')') {
        std::string inner(s.substr(5, s.size() - 6));
        auto toks = detail::tokenizeCalc(inner);
        detail::CalcParser p(toks);
        Value out;
        if (!p.parse(out)) {
            logWarn("StyleParser", "", 0, 0,
                    "calc: espressione non valida '" + std::string(s) + "'");
            return std::nullopt;
        }
        return out;
    }

    size_t n = 0;
    while (n < s.size() && (std::isdigit((unsigned char)s[n]) ||
                            s[n] == '.' || s[n] == '-' || s[n] == '+')) ++n;

    float amount = 0.0f;
    try { amount = std::stof(std::string(s.substr(0, n))); } catch (...) { return std::nullopt; }

    std::string_view unit = s.substr(n);
    if (unit.empty() || unit == "px") return Px(amount);
    if (unit == "%")  return Percent(amount);
    if (unit == "vw") return VW(amount);
    if (unit == "vh") return VH(amount);
    if (unit == "pw") return PW(amount);
    if (unit == "ph") return PH(amount);
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

inline bool applyStyleAttr(Style& st, const std::string& key, const std::string& val) {
    if      (key == "width")          { if (auto v = parseValueToken(val))   st.width = *v; return true; }
    else if (key == "height")         { if (auto v = parseValueToken(val))   st.height = *v; return true; }
    else if (key == "min-width")      { if (auto v = parseValueToken(val))   st.minWidth = *v; return true; }
    else if (key == "max-width")      { if (auto v = parseValueToken(val))   st.maxWidth = *v; return true; }
    else if (key == "min-height")     { if (auto v = parseValueToken(val))   st.minHeight = *v; return true; }
    else if (key == "max-height")     { if (auto v = parseValueToken(val))   st.maxHeight = *v; return true; }
    else if (key == "gap")            { if (auto v = parseValueToken(val))   st.gap = *v; return true; }
    else if (key == "grow")           { try { st.grow = std::stof(val); } catch (...) {} return true; }
    else if (key == "shrink")         { try { st.shrink = std::stof(val); } catch (...) {} return true; }
    else if (key == "padding")        { if (auto v = parseSpacingToken(val)) st.padding = *v; return true; }
    else if (key == "margin")         { if (auto v = parseSpacingToken(val)) st.margin = *v; return true; }
    else if (key == "background")     { if (auto v = parseColorToken(val))   st.background = *v; return true; }
    else if (key == "color")          { if (auto v = parseColorToken(val))   st.color = *v; return true; }
    else if (key == "tint")           { if (auto v = parseColorToken(val))   st.tint = *v; return true; }
    else if (key == "border-color")   { if (auto v = parseColorToken(val))   st.borderColor = *v; return true; }
    else if (key == "border-width")   { if (auto v = parseValueToken(val))   st.borderWidth = *v; return true; }
    else if (key == "radius")         { if (auto v = parseValueToken(val))   st.radius = *v; return true; }
    else if (key == "opacity")        { try { st.opacity = std::stof(val); } catch (...) {} return true; }
    else if (key == "scale")          { try { st.scale = std::stof(val); } catch (...) {} return true; }
    else if (key == "rotation")       { try { st.rotation = std::stof(val); } catch (...) {} return true; }
    else if (key == "translate-x")    { if (auto v = parseValueToken(val))   st.translateX = *v; return true; }
    else if (key == "translate-y")    { if (auto v = parseValueToken(val))   st.translateY = *v; return true; }
    else if (key == "top")            { if (auto v = parseValueToken(val))   st.top = *v; return true; }
    else if (key == "left")           { if (auto v = parseValueToken(val))   st.left = *v; return true; }
    else if (key == "right")          { if (auto v = parseValueToken(val))   st.right = *v; return true; }
    else if (key == "bottom")         { if (auto v = parseValueToken(val))   st.bottom = *v; return true; }
    else if (key == "font-size")      { if (auto v = parseValueToken(val))   st.fontSize = *v; return true; }
    else if (key == "letter-spacing") { if (auto v = parseValueToken(val))   st.letterSpacing = *v; return true; }
    else if (key == "items-h")        { if (auto v = parseAlignToken(val))   st.itemsH = *v; return true; }
    else if (key == "items-v")        { if (auto v = parseAlignToken(val))   st.itemsV = *v; return true; }
    else if (key == "align-h")        { if (auto v = parseAlignToken(val))   st.alignH = *v; return true; }
    else if (key == "align-v")        { if (auto v = parseAlignToken(val))   st.alignV = *v; return true; }
    else if (key == "justify")        { if (auto v = parseJustifyToken(val)) st.justify = *v; return true; }
    else if (key == "text-align")     { if (auto v = parseAlignToken(val))   st.textAlign = *v; return true; }
    else if (key == "transition-time"){ try { st.transitionTime = std::stof(val); } catch (...) {} return true; }
    else if (key == "ease")           { TransitionFunction tf; if (parseEasing(val, tf)) st.ease = tf; return true; }
    else if (key == "position") {
        if (val == "absolute") st.position = Position::Absolute;
        else if (val == "relative") st.position = Position::Relative;
        else st.position = Position::Static;
        return true;
    }
    else if (key == "z-index") {
        if (val == "auto") st.zIndex = ZIndex::Auto();
        else { try { st.zIndex = ZIndex(std::stoi(val)); } catch(...) {} }
        return true;
    }
    else if (key == "wrap" || key == "checked" || key == "value" ||
             key == "options" || key == "passthrough") {
        return true;
    }
    else if (key == "font") { st.font = val; return true; }
    else if (key == "background-texture") {
        std::istringstream iss(val);
        std::string name;
        if (!(iss >> name)) return true;
        TextureRef ref;
        ref.name = name;
        int l, t, r, b;
        if (iss >> l >> t >> r >> b) { ref.left = l; ref.top = t; ref.right = r; ref.bottom = b; }
        st.backgroundTexture = ref;
        return true;
    }
    else if (key == "effect") { st.effect = val; return true; }
    return false;
}

// Sostituzione ricorsiva di var(--x) con il valore in customProps.
inline std::string substituteVarRefs(
    std::string v,
    const std::unordered_map<std::string, std::string>& customProps)
{
    auto trimLocal = [](std::string s) {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace((unsigned char)s[a])) a++;
        while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
        return s.substr(a, b - a);
    };

    bool changed = true;
    int safety = 0;
    while (changed && safety++ < 10) {
        changed = false;
        size_t pos = 0;
        while ((pos = v.find("var(", pos)) != std::string::npos) {
            size_t close = v.find(')', pos);
            if (close == std::string::npos) break;
            std::string name = trimLocal(v.substr(pos + 4, close - pos - 4));
            auto it = customProps.find(name);
            if (it == customProps.end()) { pos = close + 1; continue; }
            v.replace(pos, close - pos + 1, it->second);
            changed = true;
        }
    }
    return v;
}

// Applica le prop "unresolved" (con var) a uno Style, tipizzandole.
inline Style resolveUnresolvedProps(
    const std::unordered_map<std::string, std::string>& unresolved,
    const std::unordered_map<std::string, std::string>& customProps)
{
    Style out;
    for (auto& [k, raw] : unresolved) {
        std::string resolved = substituteVarRefs(raw, customProps);
        applyStyleAttr(out, k, resolved);
    }
    return out;
}

} // namespace ZenitUI