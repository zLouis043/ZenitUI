#pragma once

#include "UI.hpp"
#include "ZMarkup.hpp"
#include "Logger.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>

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
//  Posizione sorgente (per logging)
// =========================================================================
struct ParseLoc {
    std::string file;
    int line{ 0 };
    int col{ 0 };
};

// =========================================================================
//  Applica una singola dichiarazione (gestisce transition/animation longhand)
// =========================================================================
inline void applyStyleDeclaration(Style& st, const std::string& keyRaw, const std::string& valRaw, const ParseLoc& loc) {
    std::string k = trim(keyRaw);
    std::string v = trim(valRaw);

    // ---------- TRANSITION ----------
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
    if (k == "transition-delay") {
        if (!st.transitions.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'transition-delay' usato senza un blocco 'transition:' precedente. Ignorato.");
            return;
        }
        float d = 0.0f;
        try { d = parseDurationSec(v); } catch (...) { return; }
        for (auto& s : st.transitions.value) s.delay = d;
        return;
    }
    if (k == "transition-duration") {
        if (!st.transitions.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'transition-duration' usato senza un blocco 'transition:' precedente. Ignorato.");
            return;
        }
        float d = 0.0f;
        try { d = parseDurationSec(v); } catch (...) { return; }
        for (auto& s : st.transitions.value) s.duration = d;
        return;
    }
    if (k == "transition-timing-function") {
        if (!st.transitions.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'transition-timing-function' usato senza un blocco 'transition:' precedente. Ignorato.");
            return;
        }
        TransitionFunction tf;
        if (parseEasing(v, tf)) for (auto& s : st.transitions.value) s.ease = tf;
        return;
    }

    // ---------- ANIMATION ----------
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
    if (k == "animation-delay") {
        if (!st.animations.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'animation-delay' usato senza un blocco 'animation:' precedente. Ignorato.");
            return;
        }
        float d = 0.0f;
        try { d = parseDurationSec(v); } catch (...) { return; }
        for (auto& a : st.animations.value) a.delay = d;
        return;
    }
    if (k == "animation-duration") {
        if (!st.animations.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'animation-duration' usato senza un blocco 'animation:' precedente. Ignorato.");
            return;
        }
        float d = 0.0f;
        try { d = parseDurationSec(v); } catch (...) { return; }
        for (auto& a : st.animations.value) a.duration = d;
        return;
    }
    if (k == "animation-timing-function") {
        if (!st.animations.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'animation-timing-function' usato senza un blocco 'animation:' precedente. Ignorato.");
            return;
        }
        TransitionFunction tf;
        if (parseEasing(v, tf)) for (auto& a : st.animations.value) a.ease = tf;
        return;
    }
    if (k == "animation-iteration-count") {
        if (!st.animations.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'animation-iteration-count' usato senza un blocco 'animation:' precedente. Ignorato.");
            return;
        }
        int n = 1;
        if (v == "infinite") n = -1;
        else { try { n = std::stoi(v); } catch (...) { return; } }
        for (auto& a : st.animations.value) a.iterations = n;
        return;
    }
    if (k == "animation-direction") {
        if (!st.animations.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'animation-direction' usato senza un blocco 'animation:' precedente. Ignorato.");
            return;
        }
        bool alt = (v == "alternate");
        for (auto& a : st.animations.value) a.alternate = alt;
        return;
    }
    if (k == "animation-fill-mode") {
        if (!st.animations.is_set) {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "'animation-fill-mode' usato senza un blocco 'animation:' precedente. Ignorato.");
            return;
        }
        bool fwd = (v == "forwards" || v == "both");
        for (auto& a : st.animations.value) a.fillForwards = fwd;
        return;
    }

        // ---------- Overflow ----------
    if (k == "overflow") {
        if      (v == "visible") st.overflow = Overflow::Visible;
        else if (v == "hidden")  st.overflow = Overflow::Hidden;
        else if (v == "scroll")  st.overflow = Overflow::Scroll;
        else {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                "valore overflow non riconosciuto: '" + v + "'. Uso 'visible'.");
            st.overflow = Overflow::Visible;
        }
        return;
    }

    // ---------- Proprietà base (bubbled) ----------
    if (!applyStyleAttr(st, k, v)) {
        logWarn("StyleParser", loc.file, loc.line, loc.col,
            "unknown property '" + k + "'. Ignored.");
    }
}

// =========================================================================
//  AST
// =========================================================================

struct Declaration {
    std::string prop;
    std::string value;
    int line{ 0 };
    int col{ 0 };
};

struct StyleSheet {
    enum class SelectorKind { Tag, Class, Id };

    struct Rule {
        std::string name;
        std::string part;
        SelectorKind kind{ SelectorKind::Class };
        UIState state{ UIState::Idle };
        bool isFocus{ false };
        std::vector<Declaration> decls;
        int line{ 0 };
        int col{ 0 };
    };
    std::vector<Rule> rules;

    struct KfFrame { std::string ts; std::vector<Declaration> decls; int line{ 0 }; int col{ 0 }; };
    struct KfRule  { std::string name; std::vector<KfFrame> frames; int line{ 0 }; int col{ 0 }; };
    std::vector<KfRule> keyframes;

    std::unordered_map<std::string, std::string> vars;
    std::vector<Declaration> rootDecls;
};

// =========================================================================
//  Parser con tracking riga/colonna
// =========================================================================
class StyleParser {
public:
    StyleParser(std::string_view src, std::string fileName)
        : src(stripComments(src)), fileName(std::move(fileName)) {}

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
    std::string fileName;
    size_t p{ 0 };
    int line{ 1 };
    int col{ 1 };
    StyleSheet sheet;

    ParseLoc loc() const { return { fileName, line, col }; }

    bool atEnd() const { return p >= src.size(); }
    char peek() const { return atEnd() ? '\0' : src[p]; }

    char get() {
        if (atEnd()) return '\0';
        char c = src[p++];
        if (c == '\n') { line++; col = 1; } else { col++; }
        return c;
    }

    bool match(char c) { if (peek() == c) { (void)get(); return true; } return false; }

    bool matchStr(std::string_view s) {
        if (src.compare(p, s.size(), s) == 0) {
            for (size_t i = 0; i < s.size(); ++i) (void)get();
            return true;
        }
        return false;
    }

    void skipWs() {
        while (!atEnd()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { (void)get(); continue; }
            break;
        }
    }

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
            if (peek() == '}') break;

            int declLine = line, declCol = col;
            std::string prop, val;
            while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}') prop.push_back(get());
            if (!match(':')) {
                while (!atEnd() && peek() != ';' && peek() != '}') (void)get();
                match(';');
                continue;
            }
            while (!atEnd() && peek() != ';' && peek() != '}') val.push_back(get());
            match(';');

            std::string p = trim(prop);
            std::string v = trim(val);

            if (p.rfind("--", 0) == 0) {
                // Variabile CSS: --primary, --danger, ...
                sheet.vars[p] = v;
            } else {
                // Dichiarazione normale → diventa base globale
                Declaration d;
                d.prop  = p;
                d.value = v;
                d.line  = declLine;
                d.col   = declCol;
                sheet.rootDecls.push_back(std::move(d));
            }
        }
        match('}');
    }
    void parseKeyframes() {
        skipWs();
        std::string kfName = readIdent();
        int kfLine = line, kfCol = col;
        skipWs();
        if (!match('{')) return;

        StyleSheet::KfRule kf;
        kf.name = kfName;
        kf.line = kfLine;
        kf.col = kfCol;

        while (!atEnd() && peek() != '}') {
            skipWs();
            if (peek() == '}') break;

            int frameLine = line, frameCol = col;
            std::string ts;
            while (!atEnd() && peek() != '{') ts.push_back(get());
            ts = trim(ts);
            skipWs();
            if (!match('{')) break;

            StyleSheet::KfFrame frame; frame.ts = ts;
            frame.line = frameLine;
            frame.col = frameCol;

            while (!atEnd() && peek() != '}') {
                skipWs();
                if (peek() == '}') break;

                int declLine = line, declCol = col;
                std::string prop, val;
                while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}') prop.push_back(get());
                if (!match(':')) {
                    while (!atEnd() && peek() != ';' && peek() != '}') (void)get();
                    match(';');
                    continue;
                }
                while (!atEnd() && peek() != ';' && peek() != '}') val.push_back(get());
                match(';');

                Declaration d;
                d.prop  = trim(prop);
                d.value = trim(val);
                d.line  = declLine;
                d.col   = declCol;
                frame.decls.push_back(std::move(d));
            }
            match('}');
            kf.frames.push_back(std::move(frame));
        }
        match('}');
        sheet.keyframes.push_back(std::move(kf));
    }

    void parseRule() {
        int ruleLine = line, ruleCol = col;
        std::string selector;
        while (!atEnd() && peek() != '{') selector.push_back(get());
        selector = trim(selector);
        skipWs();
        if (!match('{')) return;

        // Raccogli tutte le declaration del blocco una volta
        std::vector<Declaration> decls;
        while (!atEnd() && peek() != '}') {
            skipWs();
            if (peek() == '}') break;

            int declLine = line, declCol = col;
            std::string prop, val;
            while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}') prop.push_back(get());
            if (!match(':')) {
                while (!atEnd() && peek() != ';' && peek() != '}') (void)get();
                match(';');
                continue;
            }
            while (!atEnd() && peek() != ';' && peek() != '}') val.push_back(get());
            match(';');

            Declaration d;
            d.prop  = trim(prop);
            d.value = trim(val);
            d.line  = declLine;
            d.col   = declCol;
            decls.push_back(std::move(d));
        }
        match('}');

        // Split su virgole: ogni selettore diventa la sua propria regola
        std::vector<std::string> selectors;
        {
            std::string cur;
            for (char c : selector) {
                if (c == ',') { selectors.push_back(trim(cur)); cur.clear(); }
                else cur.push_back(c);
            }
            selectors.push_back(trim(cur));
        }

        for (auto& sel : selectors) {
            std::string s;
            for (char c : sel) if (!std::isspace((unsigned char)c)) s.push_back(c);
            if (s.empty()) continue;

            StyleSheet::Rule rule;
            rule.line = ruleLine;
            rule.col = ruleCol;

            std::string base;
            std::string state;

            // Prima cerco "::" (parts). Solo se non c'è, cerco ":" (state).
            size_t dcolon = s.find("::");
            if (dcolon != std::string::npos) {
                // Sintassi "Tag:state::part"  (state sul widget, come nel CSS reale)
                // o       "Tag::part:state"   (state sul part)
                std::string left  = s.substr(0, dcolon);
                std::string right = s.substr(dcolon + 2);

                // Cerca lo state nella parte sinistra (prima di ::)
                size_t colonL = left.find(':');
                if (colonL != std::string::npos) {
                    base  = left.substr(0, colonL);
                    state = left.substr(colonL + 1);
                } else {
                    base = left;
                }

                // Cerca part e (fallback) state nella parte destra
                size_t colonR = right.find(':');
                if (colonR != std::string::npos) {
                    rule.part = right.substr(0, colonR);
                    if (state.empty()) state = right.substr(colonR + 1);
                } else {
                    rule.part = right;
                }
            } else {
                size_t colon = s.find(':');
                base  = (colon == std::string::npos) ? s : s.substr(0, colon);
                state = (colon == std::string::npos) ? "" : s.substr(colon + 1);
            }

            if (!base.empty() && base[0] == '.') {
                rule.kind = StyleSheet::SelectorKind::Class;
                rule.name = base.substr(1);
            } else if (!base.empty() && base[0] == '#') {
                rule.kind = StyleSheet::SelectorKind::Id;
                rule.name = base.substr(1);
            } else {
                rule.kind = StyleSheet::SelectorKind::Tag;
                rule.name = base;
            }

            if      (state == "hover")    rule.state = UIState::Hover;
            else if (state == "pressed")  rule.state = UIState::Pressed;
            else if (state == "disabled") rule.state = UIState::Disabled;
            else if (state == "focus")    { rule.state = UIState::Idle; rule.isFocus = true; }
            else                          rule.state = UIState::Idle;

            rule.decls = decls;
            sheet.rules.push_back(std::move(rule));
        }
    }
};

// =========================================================================
//  Auto-fill dei keyframe non uniformi + warning
// =========================================================================
inline void normalizeKeyframes(KeyframeAnimation& anim, const ParseLoc& loc) {
    if (anim.keyframes.size() < 2) return;

    // 1) Raccogli l'unione di tutte le prop
    std::set<std::string> allProps;
    for (const auto& k : anim.keyframes)
        forEachSetStyleProp(k.delta, [&](const char* n) { allProps.insert(n); });

    if (allProps.empty()) return;

    // 2) Per ogni keyframe, controlla le prop mancanti e fai auto-fill
    for (size_t i = 0; i < anim.keyframes.size(); ++i) {
        for (const auto& prop : allProps) {
            if (!hasStyleProp(anim.keyframes[i].delta, prop)) {
                // Cerca sorgente: precedente se esiste, altrimenti successivo
                bool filled = false;
                if (i > 0) filled = copyStyleProp(anim.keyframes[i].delta, anim.keyframes[i - 1].delta, prop);
                if (!filled && i + 1 < anim.keyframes.size())
                    filled = copyStyleProp(anim.keyframes[i].delta, anim.keyframes[i + 1].delta, prop);

                logWarn("StyleParser", loc.file, loc.line, loc.col,
                    "@keyframes " + anim.name + ": keyframe t=" + std::to_string(anim.keyframes[i].t) +
                    " non dichiara '" + prop + "'. Auto-fill dal keyframe " +
                    (i > 0 ? "precedente" : "successivo") + ".");
            }
        }
    }
}

// =========================================================================
//  Applica lo sheet al Theme
// =========================================================================
inline void applyStyleSheet(const StyleSheet& sheet, const std::string& fileName, Theme& theme) {
    auto subst = [&](std::string v) {
        bool changed = true; int safety = 0;
        while (changed && safety++ < 10) {
            changed = false;
            size_t pos = 0;
            while ((pos = v.find("var(", pos)) != std::string::npos) {
                size_t close = v.find(')', pos);
                if (close == std::string::npos) break;
                std::string name = trim(v.substr(pos + 4, close - pos - 4));
                auto it = sheet.vars.find(name);
                if (it == sheet.vars.end()) { pos = close + 1; continue; }
                v.replace(pos, close - pos + 1, it->second);
                changed = true;
            }
        }
        return v;
    };

    for (auto& d : sheet.rootDecls) {
        ParseLoc loc{ fileName, d.line, d.col };
        applyStyleDeclaration(theme.root.base, d.prop, subst(d.value), loc);
    }

    for (auto& rule : sheet.rules) {
        StyleSet* setPtr = nullptr;
        switch (rule.kind) {
        case StyleSheet::SelectorKind::Tag:
            setPtr = &theme.tags[rule.name];
            break;
        case StyleSheet::SelectorKind::Class:
            setPtr = &theme.classes[rule.name];
            break;
        case StyleSheet::SelectorKind::Id:
            setPtr = &theme.ids[rule.name];
            break;
        }
        
        StyleSet& set = *setPtr;

        // Se la regola è un ::part, il target è dentro set.parts[part]
        StyleSet& effectiveSet = rule.part.empty() ? set : set.parts[rule.part];

        Style& target =
            rule.isFocus                          ? effectiveSet.focus    :
            (rule.state == UIState::Hover)        ? effectiveSet.hover    :
            (rule.state == UIState::Pressed)      ? effectiveSet.pressed  :
            (rule.state == UIState::Disabled)     ? effectiveSet.disabled :
                                                    effectiveSet.base;

        for (auto& d : rule.decls) {
            ParseLoc loc{ fileName, d.line, d.col };
            std::string subbed = subst(d.value);
            applyStyleDeclaration(target, d.prop, subbed, loc);
        }
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
            for (auto& d : frame.decls) {
                ParseLoc loc{ fileName, d.line, d.col };
                applyStyleDeclaration(k.delta, d.prop, subst(d.value), loc);
            }
            anim.keyframes.push_back(std::move(k));
        }
        std::sort(anim.keyframes.begin(), anim.keyframes.end(),
            [](const Keyframe& a, const Keyframe& b) { return a.t < b.t; });

        ParseLoc kfLoc{ fileName, kf.line, kf.col };
        normalizeKeyframes(anim, kfLoc);
        theme.addKeyframes(anim);
    }

}

// =========================================================================
//  Entry point
// =========================================================================
inline bool loadStyleString(std::string_view src, Theme& theme = Theme::get(), const std::string& fileName = "<string>") {
    StyleParser parser(src, fileName);
    applyStyleSheet(parser.parse(), fileName, theme);
    return true;
}

inline bool loadStyleFile(const std::filesystem::path& path, Theme& theme = Theme::get()) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        logError("StyleParser", path.string(), 0, 0, "impossibile aprire il file");
        return false;
    }
    std::stringstream ss; ss << f.rdbuf();
    return loadStyleString(ss.str(), theme, path.string());
}

} // namespace ZenitUI::ZMarkup