#pragma once

#include "UI.hpp"
#include "ZMarkup.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace ZenitUI::ZMarkup {

// =========================================================================
//  Utility
// =========================================================================
inline std::string trim(std::string s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) a++;
    while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

inline std::string stripComments(std::string_view src) {
    std::string out; out.reserve(src.size());
    size_t i = 0;
    bool inBlock = false, inLine = false, inStr = false;
    while (i < src.size()) {
        char c = src[i], n = (i + 1 < src.size()) ? src[i + 1] : '\0';
        if (inLine)  { if (c == '\n') { inLine = false; out.push_back('\n'); } i++; continue; }
        if (inBlock) { if (c == '*' && n == '/') { inBlock = false; i += 2; continue; } i++; continue; }
        if (inStr)   { out.push_back(c); if (c == '\\' && i + 1 < src.size()) { out.push_back(src[i + 1]); i += 2; continue; } if (c == '"') inStr = false; i++; continue; }
        if (c == '/' && n == '/') { inLine = true; i += 2; continue; }
        if (c == '/' && n == '*') { inBlock = true; i += 2; continue; }
        if (c == '"') inStr = true;
        out.push_back(c); i++;
    }
    return out;
}

inline std::vector<std::string> splitBy(std::string_view s, char sep) {
    std::vector<std::string> out; std::string cur;
    for (char c : s) { if (c == sep) { out.push_back(cur); cur.clear(); } else cur.push_back(c); }
    out.push_back(cur);
    return out;
}

inline std::vector<std::string> splitWs(std::string_view s) {
    std::vector<std::string> out; std::string cur;
    for (char c : s) {
        if (std::isspace((unsigned char)c)) { if (!cur.empty()) { out.push_back(cur); cur.clear(); } }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

inline float parseDurationSec(std::string_view s) {
    if (s.size() >= 2 && s.substr(s.size() - 2) == "ms")
        return std::stof(std::string(s.substr(0, s.size() - 2))) / 1000.0f;
    if (!s.empty() && s.back() == 's')
        return std::stof(std::string(s.substr(0, s.size() - 1)));
    return std::stof(std::string(s));
}

// =========================================================================
//  Applica una singola dichiarazione in un Style (gestisce transition/animation)
// =========================================================================
inline void applyStyleDeclaration(Style& st, const std::string& keyRaw, const std::string& valRaw) {
    std::string k = trim(keyRaw);
    std::string v = trim(valRaw);

    if (k == "transition") {
        std::vector<TransitionSpec> specs;
        for (auto& part : splitBy(v, ',')) {
            auto tokens = splitWs(part);
            if (tokens.empty()) continue;
            TransitionSpec spec;
            spec.prop = tokens[0];
            for (size_t i = 1; i < tokens.size(); ++i) {
                const auto& t = tokens[i];
                TransitionFunction tf;
                if (parseEasing(t, tf)) { spec.ease = tf; continue; }
                if (!t.empty() && (t.back() == 's')) {
                    try { spec.duration = parseDurationSec(t); } catch (...) {}
                }
            }
            specs.push_back(std::move(spec));
        }
        st.transitions = specs;
        return;
    }

    if (k == "animation") {
        std::vector<AnimationRef> anims;
        for (auto& part : splitBy(v, ',')) {
            auto tokens = splitWs(part);
            if (tokens.empty()) continue;
            AnimationRef a;
            a.name = tokens[0];
            bool durSet = false, delaySet = false;
            for (size_t i = 1; i < tokens.size(); ++i) {
                const auto& t = tokens[i];
                if (t == "infinite")  { a.iterations = -1; continue; }
                if (t == "alternate") { a.alternate = true; continue; }
                if (t == "forwards")  { a.fillForwards = true; continue; }
                if (t == "none" || t == "running") continue;
                TransitionFunction tf;
                if (parseEasing(t, tf)) { a.ease = tf; continue; }
                if (!t.empty() && (t.back() == 's')) {
                    try {
                        float d = parseDurationSec(t);
                        if (!durSet)        { a.duration = d; durSet = true; }
                        else if (!delaySet) { a.delay    = d; delaySet = true; }
                    } catch (...) {}
                    continue;
                }
                try { a.iterations = std::stoi(t); continue; } catch (...) {}
            }
            anims.push_back(std::move(a));
        }
        st.animations = anims;
        return;
    }

    // Tutte le altre: delega al applyStyleAttr esistente in ZMarkup.hpp
    applyStyleAttr(st, k, v);
}

// =========================================================================
//  Parser minimale di .zstyle
// =========================================================================
struct StyleSheet {
    // name → state → decls
    struct Rule {
        std::string name;
        UIState state{ UIState::Idle };
        std::vector<std::pair<std::string, std::string>> decls;
    };
    std::vector<Rule> rules;

    struct KfFrame { std::string ts; std::vector<std::pair<std::string, std::string>> decls; };
    struct KfRule  { std::string name; std::vector<KfFrame> frames; };
    std::vector<KfRule> keyframes;

    std::unordered_map<std::string, std::string> vars;
};

class StyleParser {
public:
    explicit StyleParser(std::string_view src) : src(stripComments(src)) {}

    StyleSheet parse() {
        while (!atEnd()) {
            skipWs();
            if (atEnd()) break;

            if (matchStr(":root"))            parseRoot();
            else if (matchStr("@keyframes"))  parseKeyframes();
            else                              parseRule();
        }
        return sheet;
    }

private:
    std::string src;
    size_t p{ 0 };
    StyleSheet sheet;

    bool atEnd() const { return p >= src.size(); }
    char peek() const { return atEnd() ? '\0' : src[p]; }
    char get() { return atEnd() ? '\0' : src[p++]; }
    bool match(char c) { if (peek() == c) { p++; return true; } return false; }
    bool matchStr(std::string_view s) {
        if (src.compare(p, s.size(), s) == 0) { p += s.size(); return true; }
        return false;
    }
    void skipWs() { while (!atEnd() && std::isspace((unsigned char)peek())) p++; }

    std::string readIdent() {
        std::string out;
        while (!atEnd() && (std::isalnum((unsigned char)peek()) || peek() == '_' || peek() == '-'))
            out.push_back(get());
        return out;
    }

    void parseRoot() {
        skipWs();
        if (!match('{')) return;
        while (!atEnd() && peek() != '}') {
            skipWs();
            if (peek() != '-') {
                while (!atEnd() && peek() != ';' && peek() != '}') get();
                match(';');
                continue;
            }
            if (!matchStr("--")) { get(); continue; }
            std::string name;
            while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}') name.push_back(get());
            if (!match(':')) continue;
            std::string val;
            while (!atEnd() && peek() != ';' && peek() != '}') val.push_back(get());
            match(';');
            sheet.vars["--" + trim(name)] = trim(val);
        }
        match('}');
    }

    void parseKeyframes() {
        skipWs();
        std::string name = readIdent();
        skipWs();
        if (!match('{')) return;

        StyleSheet::KfRule kf; kf.name = name;
        while (!atEnd() && peek() != '}') {
            skipWs();
            if (peek() == '}') break;

            std::string ts;
            while (!atEnd() && peek() != '{') ts.push_back(get());
            ts = trim(ts);
            skipWs();
            if (!match('{')) break;

            StyleSheet::KfFrame frame; frame.ts = ts;
            while (!atEnd() && peek() != '}') {
                skipWs();
                if (peek() == '}') break;
                std::string prop, val;
                while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}') prop.push_back(get());
                if (!match(':')) {
                    while (!atEnd() && peek() != ';' && peek() != '}') get();
                    match(';');
                    continue;
                }
                while (!atEnd() && peek() != ';' && peek() != '}') val.push_back(get());
                match(';');
                frame.decls.push_back({ trim(prop), trim(val) });
            }
            match('}');
            kf.frames.push_back(std::move(frame));
        }
        match('}');
        sheet.keyframes.push_back(std::move(kf));
    }

    void parseRule() {
        std::string selector;
        while (!atEnd() && peek() != '{') selector.push_back(get());
        selector = trim(selector);
        skipWs();
        if (!match('{')) return;

        // Rimuovi spazi interni
        std::string s;
        for (char c : selector) if (!std::isspace((unsigned char)c)) s.push_back(c);

        StyleSheet::Rule rule;
        size_t colon = s.find(':');
        std::string base  = (colon == std::string::npos) ? s : s.substr(0, colon);
        std::string state = (colon == std::string::npos) ? "" : s.substr(colon + 1);

        if (!base.empty() && base[0] == '.')      rule.name = base.substr(1);
        else if (!base.empty() && base[0] == '#') rule.name = base.substr(1);
        else                                      rule.name = base;

        if      (state == "hover")    rule.state = UIState::Hover;
        else if (state == "pressed")  rule.state = UIState::Pressed;
        else if (state == "disabled") rule.state = UIState::Disabled;
        else                          rule.state = UIState::Idle;

        while (!atEnd() && peek() != '}') {
            skipWs();
            if (peek() == '}') break;
            std::string prop, val;
            while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}') prop.push_back(get());
            if (!match(':')) {
                while (!atEnd() && peek() != ';' && peek() != '}') get();
                match(';');
                continue;
            }
            while (!atEnd() && peek() != ';' && peek() != '}') val.push_back(get());
            match(';');
            rule.decls.push_back({ trim(prop), trim(val) });
        }
        match('}');
        sheet.rules.push_back(std::move(rule));
    }
};

// =========================================================================
//  Applica lo sheet al Theme
// =========================================================================
inline void applyStyleSheet(const StyleSheet& sheet, Theme& theme) {
        auto subst = [&](std::string v) {
        bool changed = true; int safety = 0;
        while (changed && safety++ < 10) {
            changed = false;
            for (auto& [k, val] : sheet.vars) {
                // 1) Prima espandi var(--name) → valore, tutto insieme
                std::string wrapped = "var(" + k + ")";
                size_t pos;
                while ((pos = v.find(wrapped)) != std::string::npos) {
                    v.replace(pos, wrapped.size(), val);
                    changed = true;
                }
                // 2) Poi gestisci eventuali occorrenze nude di --name
                while ((pos = v.find(k)) != std::string::npos) {
                    v.replace(pos, k.size(), val);
                    changed = true;
                }
            }
        }
        return v;
    };

    for (auto& rule : sheet.rules) {
        auto& set = theme.classes[rule.name];
        Style& target =
            (rule.state == UIState::Hover)    ? set.hover :
            (rule.state == UIState::Pressed)  ? set.pressed :
            (rule.state == UIState::Disabled) ? set.disabled :
                                                set.base;
        for (auto& [k, v] : rule.decls) applyStyleDeclaration(target, k, subst(v));
    }

    for (auto& kf : sheet.keyframes) {
        KeyframeAnimation anim;
        anim.name = kf.name;
        for (auto& frame : kf.frames) {
            float t = 0.0f;
            if      (frame.ts == "from") t = 0.0f;
            else if (frame.ts == "to")   t = 1.0f;
            else if (!frame.ts.empty() && frame.ts.back() == '%')
                t = std::stof(frame.ts.substr(0, frame.ts.size() - 1)) / 100.0f;

            Keyframe k; k.t = t;
            for (auto& [k2, v2] : frame.decls) applyStyleDeclaration(k.delta, k2, subst(v2));
            anim.keyframes.push_back(std::move(k));
        }
        std::sort(anim.keyframes.begin(), anim.keyframes.end(),
            [](const Keyframe& a, const Keyframe& b) { return a.t < b.t; });
        theme.addKeyframes(anim);
    }
}

// =========================================================================
//  Entry point
// =========================================================================
inline bool loadStyleString(std::string_view src, Theme& theme = Theme::get()) {
    StyleParser parser(src);
    applyStyleSheet(parser.parse(), theme);
    return true;
}

inline bool loadStyleFile(const std::filesystem::path& path, Theme& theme = Theme::get()) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::stringstream ss; ss << f.rdbuf();
    return loadStyleString(ss.str(), theme);
}

} // namespace ZenitUI::ZMarkup