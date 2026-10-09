#pragma once

#include "UI.hpp"
#include "ZMarkup.hpp"
#include "Logger.hpp"
#include "StyleAttr.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>

namespace ZenitUI::ZMarkup
{

    // =========================================================================
    //  Utility
    // =========================================================================
    inline std::string trim(std::string s)
    {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace((unsigned char)s[a]))
            a++;
        while (b > a && std::isspace((unsigned char)s[b - 1]))
            b--;
        return s.substr(a, b - a);
    }

    inline std::string stripComments(std::string_view src)
    {
        std::string out;
        out.reserve(src.size());
        size_t i = 0;
        bool inBlock = false, inLine = false, inStr = false;
        while (i < src.size())
        {
            char c = src[i], n = (i + 1 < src.size()) ? src[i + 1] : '\0';
            if (inLine)
            {
                if (c == '\n')
                {
                    inLine = false;
                    out.push_back('\n');
                }
                i++;
                continue;
            }
            if (inBlock)
            {
                if (c == '*' && n == '/')
                {
                    inBlock = false;
                    i += 2;
                    continue;
                }
                i++;
                continue;
            }
            if (inStr)
            {
                out.push_back(c);
                if (c == '\\' && i + 1 < src.size())
                {
                    out.push_back(src[i + 1]);
                    i += 2;
                    continue;
                }
                if (c == '"')
                    inStr = false;
                i++;
                continue;
            }
            if (c == '/' && n == '/')
            {
                inLine = true;
                i += 2;
                continue;
            }
            if (c == '/' && n == '*')
            {
                inBlock = true;
                i += 2;
                continue;
            }
            if (c == '"')
                inStr = true;
            out.push_back(c);
            i++;
        }
        return out;
    }

    inline std::vector<std::string> splitBy(std::string_view s, char sep)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s)
        {
            if (c == sep)
            {
                out.push_back(cur);
                cur.clear();
            }
            else
                cur.push_back(c);
        }
        out.push_back(cur);
        return out;
    }

    inline std::vector<std::string> splitWs(std::string_view s)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s)
        {
            if (std::isspace((unsigned char)c))
            {
                if (!cur.empty())
                {
                    out.push_back(cur);
                    cur.clear();
                }
            }
            else
                cur.push_back(c);
        }
        if (!cur.empty())
            out.push_back(cur);
        return out;
    }

    inline float parseDurationSec(std::string_view s)
    {
        if (s.size() >= 2 && s.substr(s.size() - 2) == "ms")
            return std::stof(std::string(s.substr(0, s.size() - 2))) / 1000.0f;
        if (!s.empty() && s.back() == 's')
            return std::stof(std::string(s.substr(0, s.size() - 1)));
        return std::stof(std::string(s));
    }

    // =========================================================================
    //  Posizione sorgente (per logging)
    // =========================================================================
    struct ParseLoc
    {
        std::string file;
        int line{0};
        int col{0};
    };

    // =========================================================================
    //  Applica una singola dichiarazione (gestisce transition/animation longhand)
    // =========================================================================
    inline void applyStyleDeclaration(Style &st, const std::string &keyRaw, const std::string &valRaw, const ParseLoc &loc)
    {
        std::string k = trim(keyRaw);
        std::string v = trim(valRaw);

        // ---------- CUSTOM PROPERTY ----------
        // Una prop che inizia con -- è una custom property: la salviamo raw.
        if (k.rfind("--", 0) == 0)
        {
            st.customProps[k] = v;
            return;
        }

        // ---------- VALORI CON var() ----------
        // Non risolviamo ora: il valore dipende dal cascade finale.
        // Le salviamo raw e le risolviamo in StyleResolver::resolveFor.
        if (v.find("var(") != std::string::npos)
        {
            st.unresolvedProps[k] = v;
            clearStyleProp(st, k);
            return;
        }

        // ---------- TRANSITION ----------
        if (k == "transition")
        {
            std::vector<TransitionSpec> specs;
            for (auto &part : splitBy(v, ','))
            {
                auto tokens = splitWs(part);
                if (tokens.empty())
                    continue;
                TransitionSpec spec;
                spec.prop = tokens[0];
                for (size_t i = 1; i < tokens.size(); ++i)
                {
                    const auto &t = tokens[i];
                    TransitionFunction tf;
                    if (parseEasing(t, tf))
                    {
                        spec.ease = tf;
                        continue;
                    }
                    if (!t.empty() && (t.back() == 's'))
                    {
                        try
                        {
                            spec.duration = parseDurationSec(t);
                        }
                        catch (...)
                        {
                        }
                    }
                }
                specs.push_back(std::move(spec));
            }
            st.transitions = specs;
            return;
        }
        if (k == "transition-delay")
        {
            if (!st.transitions.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'transition-delay' usato senza un blocco 'transition:' precedente. Ignorato.");
                return;
            }
            float d = 0.0f;
            try
            {
                d = parseDurationSec(v);
            }
            catch (...)
            {
                return;
            }
            for (auto &s : st.transitions.value)
                s.delay = d;
            return;
        }
        if (k == "transition-duration")
        {
            if (!st.transitions.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'transition-duration' usato senza un blocco 'transition:' precedente. Ignorato.");
                return;
            }
            float d = 0.0f;
            try
            {
                d = parseDurationSec(v);
            }
            catch (...)
            {
                return;
            }
            for (auto &s : st.transitions.value)
                s.duration = d;
            return;
        }
        if (k == "transition-timing-function")
        {
            if (!st.transitions.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'transition-timing-function' usato senza un blocco 'transition:' precedente. Ignorato.");
                return;
            }
            TransitionFunction tf;
            if (parseEasing(v, tf))
                for (auto &s : st.transitions.value)
                    s.ease = tf;
            return;
        }

        // ---------- ANIMATION ----------
        if (k == "animation")
        {
            std::vector<AnimationRef> anims;
            for (auto &part : splitBy(v, ','))
            {
                auto tokens = splitWs(part);
                if (tokens.empty())
                    continue;
                AnimationRef a;
                a.name = tokens[0];
                bool durSet = false, delaySet = false;
                for (size_t i = 1; i < tokens.size(); ++i)
                {
                    const auto &t = tokens[i];
                    if (t == "infinite")
                    {
                        a.iterations = -1;
                        continue;
                    }
                    if (t == "alternate")
                    {
                        a.alternate = true;
                        continue;
                    }
                    if (t == "forwards")
                    {
                        a.fillForwards = true;
                        continue;
                    }
                    if (t == "none" || t == "running")
                        continue;
                    TransitionFunction tf;
                    if (parseEasing(t, tf))
                    {
                        a.ease = tf;
                        continue;
                    }
                    if (!t.empty() && (t.back() == 's'))
                    {
                        try
                        {
                            float d = parseDurationSec(t);
                            if (!durSet)
                            {
                                a.duration = d;
                                durSet = true;
                            }
                            else if (!delaySet)
                            {
                                a.delay = d;
                                delaySet = true;
                            }
                        }
                        catch (...)
                        {
                        }
                        continue;
                    }
                    try
                    {
                        a.iterations = std::stoi(t);
                        continue;
                    }
                    catch (...)
                    {
                    }
                }
                anims.push_back(std::move(a));
            }
            st.animations = anims;
            return;
        }
        if (k == "animation-delay")
        {
            if (!st.animations.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'animation-delay' usato senza un blocco 'animation:' precedente. Ignorato.");
                return;
            }
            float d = 0.0f;
            try
            {
                d = parseDurationSec(v);
            }
            catch (...)
            {
                return;
            }
            for (auto &a : st.animations.value)
                a.delay = d;
            return;
        }
        if (k == "animation-duration")
        {
            if (!st.animations.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'animation-duration' usato senza un blocco 'animation:' precedente. Ignorato.");
                return;
            }
            float d = 0.0f;
            try
            {
                d = parseDurationSec(v);
            }
            catch (...)
            {
                return;
            }
            for (auto &a : st.animations.value)
                a.duration = d;
            return;
        }
        if (k == "animation-timing-function")
        {
            if (!st.animations.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'animation-timing-function' usato senza un blocco 'animation:' precedente. Ignorato.");
                return;
            }
            TransitionFunction tf;
            if (parseEasing(v, tf))
                for (auto &a : st.animations.value)
                    a.ease = tf;
            return;
        }
        if (k == "animation-iteration-count")
        {
            if (!st.animations.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'animation-iteration-count' usato senza un blocco 'animation:' precedente. Ignorato.");
                return;
            }
            int n = 1;
            if (v == "infinite")
                n = -1;
            else
            {
                try
                {
                    n = std::stoi(v);
                }
                catch (...)
                {
                    return;
                }
            }
            for (auto &a : st.animations.value)
                a.iterations = n;
            return;
        }
        if (k == "animation-direction")
        {
            if (!st.animations.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'animation-direction' usato senza un blocco 'animation:' precedente. Ignorato.");
                return;
            }
            bool alt = (v == "alternate");
            for (auto &a : st.animations.value)
                a.alternate = alt;
            return;
        }
        if (k == "animation-fill-mode")
        {
            if (!st.animations.is_set)
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "'animation-fill-mode' usato senza un blocco 'animation:' precedente. Ignorato.");
                return;
            }
            bool fwd = (v == "forwards" || v == "both");
            for (auto &a : st.animations.value)
                a.fillForwards = fwd;
            return;
        }

        // ---------- FILTER ----------
        if (k == "filter")
        {
            std::vector<FilterRef> refs;
            std::string cur = v;
            size_t pos = 0;
            while (pos < cur.size())
            {
                // Trova la prossima virgola top-level
                size_t comma = pos;
                int depth = 0;
                while (comma < cur.size())
                {
                    char c = cur[comma];
                    if (c == '(')
                        depth++;
                    else if (c == ')')
                        depth--;
                    else if (c == ',' && depth == 0)
                        break;
                    comma++;
                }
                std::string part = trim(cur.substr(pos, comma - pos));
                pos = (comma < cur.size()) ? comma + 1 : cur.size();
                if (part.empty())
                    continue;

                FilterRef ref;
                size_t lparen = part.find('(');
                if (lparen == std::string::npos)
                {
                    ref.name = trim(part);
                }
                else
                {
                    size_t rparen = part.find(')', lparen);
                    ref.name = trim(part.substr(0, lparen));
                    std::string args = (rparen == std::string::npos)
                                           ? part.substr(lparen + 1)
                                           : part.substr(lparen + 1, rparen - lparen - 1);

                    std::string curArg;
                    int argDepth = 0;
                    for (char c : args)
                    {
                        if (c == '(')
                            argDepth++;
                        else if (c == ')')
                            argDepth--;
                        if (c == ',' && argDepth == 0)
                        {
                            ref.args.push_back(trim(curArg));
                            curArg.clear();
                        }
                        else
                        {
                            curArg.push_back(c);
                        }
                    }
                    if (!curArg.empty())
                        ref.args.push_back(trim(curArg));
                }
                refs.push_back(std::move(ref));
            }
            st.filters = refs;
            return;
        }

        // ---------- BOX-SHADOW ----------
        if (k == "box-shadow")
        {
            auto tokens = splitWs(v);
            if (tokens.size() >= 3)
            {
                BoxShadow s;
                s.enabled = true;
                if (auto x = parseValueToken(tokens[0]))
                    s.x = *x;
                if (auto y = parseValueToken(tokens[1]))
                    s.y = *y;
                if (auto b = parseValueToken(tokens[2]))
                    s.blur = *b;
                if (tokens.size() >= 4)
                {
                    if (auto c = parseColorToken(tokens[3]))
                        s.color = *c;
                    else
                        s.color = Color{0, 0, 0, 128};
                }
                else
                {
                    s.color = Color{0, 0, 0, 128};
                }
                st.boxShadow = s;
            }
            return;
        }

        // ---------- Overflow ----------
        auto parseOverflowValue = [&](const std::string &val, Overflow &out) -> bool
        {
            if (val == "visible")
            {
                out = Overflow::Visible;
                return true;
            }
            if (val == "hidden")
            {
                out = Overflow::Hidden;
                return true;
            }
            if (val == "scroll")
            {
                out = Overflow::Scroll;
                return true;
            }
            if (val == "auto")
            {
                out = Overflow::Auto;
                return true;
            }
            return false;
        };

        if (k == "overflow")
        {
            Overflow o;
            if (parseOverflowValue(v, o))
            {
                st.overflowX = o;
                st.overflowY = o;
            }
            else
            {
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "valore overflow non riconosciuto: '" + v + "'. Ignorato.");
            }
            return;
        }
        if (k == "overflow-x")
        {
            Overflow o;
            if (parseOverflowValue(v, o))
                st.overflowX = o;
            else
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "valore overflow-x non riconosciuto: '" + v + "'. Ignorato.");
            return;
        }
        if (k == "overflow-y")
        {
            Overflow o;
            if (parseOverflowValue(v, o))
                st.overflowY = o;
            else
                logWarn("StyleParser", loc.file, loc.line, loc.col,
                        "valore overflow-y non riconosciuto: '" + v + "'. Ignorato.");
            return;
        }

        // ---------- Proprietà base (bubbled) ----------
        if (!applyStyleAttr(st, k, v))
        {
            logWarn("StyleParser", loc.file, loc.line, loc.col,
                    "unknown property '" + k + "'. Ignored.");
        }
    }

    // =========================================================================
    //  AST
    // =========================================================================

    struct Declaration
    {
        std::string prop;
        std::string value;
        int line{0};
        int col{0};
    };

    struct StyleSheet
    {
        struct Rule
        {
            std::vector<SimpleSelector> chain; // es. [Tag Toggle {checked}, Class knob]
            std::string part;                  // vuoto se non è ::part
            std::vector<Declaration> decls;
            int line{0};
            int col{0};
            std::optional<MediaQuery> media;
        };
        std::vector<Rule> rules;

        struct KfFrame
        {
            std::string ts;
            std::vector<Declaration> decls;
            int line{0};
            int col{0};
        };
        struct KfRule
        {
            std::string name;
            std::vector<KfFrame> frames;
            int line{0};
            int col{0};
        };
        std::vector<KfRule> keyframes;

        std::unordered_map<std::string, std::string> vars;
        std::vector<Declaration> rootDecls;
    };

    // =========================================================================
    //  Parser con tracking riga/colonna
    // =========================================================================
    class StyleParser
    {
    public:
        StyleParser(std::string_view src, std::string fileName)
            : src(stripComments(src)), fileName(std::move(fileName)) {}

        StyleSheet parse()
        {
            while (!atEnd())
            {
                skipWs();
                if (atEnd())
                    break;

                if (matchStr(":root"))
                    parseRoot();
                else if (matchStr("@keyframes"))
                    parseKeyframes();
                else if (matchStr("@media"))
                    parseMedia();
                else
                    parseRule();
            }
            return sheet;
        }

    private:
        std::string src;
        std::string fileName;
        size_t p{0};
        int line{1};
        int col{1};
        StyleSheet sheet;

        ParseLoc loc() const { return {fileName, line, col}; }

        bool atEnd() const { return p >= src.size(); }
        char peek() const { return atEnd() ? '\0' : src[p]; }

        char get()
        {
            if (atEnd())
                return '\0';
            char c = src[p++];
            if (c == '\n')
            {
                line++;
                col = 1;
            }
            else
            {
                col++;
            }
            return c;
        }

        bool match(char c)
        {
            if (peek() == c)
            {
                (void)get();
                return true;
            }
            return false;
        }

        bool matchStr(std::string_view s)
        {
            if (src.compare(p, s.size(), s) == 0)
            {
                for (size_t i = 0; i < s.size(); ++i)
                    (void)get();
                return true;
            }
            return false;
        }

        void skipWs()
        {
            while (!atEnd())
            {
                char c = peek();
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                {
                    (void)get();
                    continue;
                }
                break;
            }
        }

        std::string readIdent()
        {
            std::string out;
            while (!atEnd() && (std::isalnum((unsigned char)peek()) || peek() == '_' || peek() == '-'))
                out.push_back(get());
            return out;
        }

        void parseRoot()
        {
            skipWs();
            if (!match('{'))
                return;
            while (!atEnd() && peek() != '}')
            {
                skipWs();
                if (peek() == '}')
                    break;

                int declLine = line, declCol = col;
                std::string prop, val;
                while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}')
                    prop.push_back(get());
                if (!match(':'))
                {
                    while (!atEnd() && peek() != ';' && peek() != '}')
                        (void)get();
                    match(';');
                    continue;
                }
                while (!atEnd() && peek() != ';' && peek() != '}')
                    val.push_back(get());
                match(';');

                std::string p = trim(prop);
                std::string v = trim(val);

                if (p.rfind("--", 0) == 0)
                {
                    // Variabile CSS: la teniamo anche in sheet.vars (retrocompat),
                    // ma va comunque registrata come declaration per finire
                    // in theme.root.customProps tramite applyStyleDeclaration.
                    sheet.vars[p] = v;
                }

                // Sempre: push come declaration. Se è una var, applyStyleDeclaration
                // la instrada in theme.root.customProps; se è una prop normale,
                // la instrada nella root Style.
                Declaration d;
                d.prop = p;
                d.value = v;
                d.line = declLine;
                d.col = declCol;
                sheet.rootDecls.push_back(std::move(d));
            }
            match('}');
        }
        void parseKeyframes()
        {
            skipWs();
            std::string kfName = readIdent();
            int kfLine = line, kfCol = col;
            skipWs();
            if (!match('{'))
                return;

            StyleSheet::KfRule kf;
            kf.name = kfName;
            kf.line = kfLine;
            kf.col = kfCol;

            while (!atEnd() && peek() != '}')
            {
                skipWs();
                if (peek() == '}')
                    break;

                int frameLine = line, frameCol = col;
                std::string ts;
                while (!atEnd() && peek() != '{')
                    ts.push_back(get());
                ts = trim(ts);
                skipWs();
                if (!match('{'))
                    break;

                StyleSheet::KfFrame frame;
                frame.ts = ts;
                frame.line = frameLine;
                frame.col = frameCol;

                while (!atEnd() && peek() != '}')
                {
                    skipWs();
                    if (peek() == '}')
                        break;

                    int declLine = line, declCol = col;
                    std::string prop, val;
                    while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}')
                        prop.push_back(get());
                    if (!match(':'))
                    {
                        while (!atEnd() && peek() != ';' && peek() != '}')
                            (void)get();
                        match(';');
                        continue;
                    }
                    while (!atEnd() && peek() != ';' && peek() != '}')
                        val.push_back(get());
                    match(';');

                    Declaration d;
                    d.prop = trim(prop);
                    d.value = trim(val);
                    d.line = declLine;
                    d.col = declCol;
                    frame.decls.push_back(std::move(d));
                }
                match('}');
                kf.frames.push_back(std::move(frame));
            }
            match('}');
            sheet.keyframes.push_back(std::move(kf));
        }

        void parseMedia()
        {
            skipWs();

            // Parse della condizione fino a '{'.
            std::string cond;
            while (!atEnd() && peek() != '{')
                cond.push_back(get());
            cond = trim(cond);
            if (!match('{'))
                return;

            MediaQuery query = parseMediaQuery(cond);

            // Le regole dentro ereditano questa query.
            while (!atEnd() && peek() != '}')
            {
                skipWs();
                if (peek() == '}')
                    break;
                parseRuleWithMedia(query);
            }
            match('}');
        }

        MediaQuery parseMediaQuery(const std::string &s)
        {
            MediaQuery q;
            // Formato: "(min-width: 600px) and (orientation: landscape)"
            // Facciamo uno split per " and " e parse di ogni condizione.
            std::vector<std::string> parts;
            std::string cur;
            size_t depth = 0;
            for (size_t i = 0; i < s.size(); ++i)
            {
                char c = s[i];
                if (c == '(')
                {
                    depth++;
                    cur.push_back(c);
                }
                else if (c == ')')
                {
                    depth--;
                    cur.push_back(c);
                }
                else if (depth == 0 && i + 5 <= s.size() &&
                         s.compare(i, 5, " and ") == 0)
                {
                    parts.push_back(trim(cur));
                    cur.clear();
                    i += 4;
                }
                else
                    cur.push_back(c);
            }
            if (!cur.empty())
                parts.push_back(trim(cur));

            for (auto &p : parts)
            {
                // p = "(min-width: 600px)" → estrai "min-width" e "600px"
                std::string inner = p;
                if (inner.size() >= 2 && inner.front() == '(' && inner.back() == ')')
                    inner = inner.substr(1, inner.size() - 2);

                size_t colon = inner.find(':');
                std::string key = trim(colon == std::string::npos ? inner : inner.substr(0, colon));
                std::string val = colon == std::string::npos ? "" : trim(inner.substr(colon + 1));

                MediaCondition c;
                if (key == "min-width")
                {
                    c.kind = MediaCondition::Kind::MinWidth;
                    c.value = std::stof(val);
                }
                else if (key == "max-width")
                {
                    c.kind = MediaCondition::Kind::MaxWidth;
                    c.value = std::stof(val);
                }
                else if (key == "min-height")
                {
                    c.kind = MediaCondition::Kind::MinHeight;
                    c.value = std::stof(val);
                }
                else if (key == "max-height")
                {
                    c.kind = MediaCondition::Kind::MaxHeight;
                    c.value = std::stof(val);
                }
                else if (key == "orientation")
                {
                    if (val == "landscape")
                        c.kind = MediaCondition::Kind::OrientationLandscape;
                    else if (val == "portrait")
                        c.kind = MediaCondition::Kind::OrientationPortrait;
                    else
                        continue;
                }
                else if (key == "min-aspect-ratio")
                {
                    // "16/9" o "1.77"
                    c.kind = MediaCondition::Kind::MinAspectRatio;
                    c.value = parseAspectRatio(val);
                }
                else if (key == "max-aspect-ratio")
                {
                    c.kind = MediaCondition::Kind::MaxAspectRatio;
                    c.value = parseAspectRatio(val);
                }
                else
                    continue;

                q.conditions.push_back(c);
            }
            return q;
        }

        static float parseAspectRatio(const std::string &s)
        {
            size_t slash = s.find('/');
            if (slash == std::string::npos)
                return std::stof(s);
            float num = std::stof(s.substr(0, slash));
            float den = std::stof(s.substr(slash + 1));
            return den > 0.0f ? num / den : 1.0f;
        }

        void parseRuleWithMedia(const MediaQuery &media)
        {
            // Identico a parseRule(), ma prima di push nella sheet imposta tr.media.
            // Per non duplicare 100 righe, si rifattorizza parseRule per accettare
            // un Opt<MediaQuery>. Vedi sotto.
            parseRule(&media);
        }

        void parseRule(const MediaQuery *media = nullptr)
        {
            int ruleLine = line, ruleCol = col;
            std::string selector;
            while (!atEnd() && peek() != '{')
                selector.push_back(get());
            selector = trim(selector);
            skipWs();
            if (!match('{'))
                return;

            // Raccogli declarations
            std::vector<Declaration> decls;
            while (!atEnd() && peek() != '}')
            {
                skipWs();
                if (peek() == '}')
                    break;

                int declLine = line, declCol = col;
                std::string prop, val;
                while (!atEnd() && peek() != ':' && peek() != ';' && peek() != '}')
                    prop.push_back(get());
                if (!match(':'))
                {
                    while (!atEnd() && peek() != ';' && peek() != '}')
                        (void)get();
                    match(';');
                    continue;
                }
                while (!atEnd() && peek() != ';' && peek() != '}')
                    val.push_back(get());
                match(';');

                Declaration d;
                d.prop = trim(prop);
                d.value = trim(val);
                d.line = declLine;
                d.col = declCol;
                decls.push_back(std::move(d));
            }
            match('}');

            // Split per virgole (selettori multipli)
            std::vector<std::string> selectors;
            {
                std::string cur;
                for (char c : selector)
                {
                    if (c == ',')
                    {
                        selectors.push_back(trim(cur));
                        cur.clear();
                    }
                    else
                        cur.push_back(c);
                }
                selectors.push_back(trim(cur));
            }

            for (auto &rawSel : selectors)
            {
                std::string sel = trim(rawSel);
                if (sel.empty())
                    continue;

                StyleSheet::Rule rule;
                rule.line = ruleLine;
                rule.col = ruleCol;
                if (media)
                    rule.media = *media;

                // 1) Separa "::part" (doppia colon), che sta sempre in fondo.
                std::string chainPart = sel;
                size_t dcolon = sel.rfind("::");
                if (dcolon != std::string::npos)
                {
                    chainPart = sel.substr(0, dcolon);
                    rule.part = trim(sel.substr(dcolon + 2));
                }

                // 2) Split della chain per whitespace (discendente).
                std::vector<std::string> tokens;
                {
                    std::string cur;
                    for (char c : chainPart)
                    {
                        if (std::isspace((unsigned char)c))
                        {
                            if (!cur.empty())
                            {
                                tokens.push_back(cur);
                                cur.clear();
                            }
                        }
                        else
                            cur.push_back(c);
                    }
                    if (!cur.empty())
                        tokens.push_back(cur);
                }
                if (tokens.empty())
                    continue;

                // 3) Per ogni token, estrai base (tag/.class/#id) e stato (dopo ':').
                // 3) Per ogni token, estrai base (tag/.class/#id) e stato (dopo ':').
                //    Il base può essere un selettore composto: `Button.btn-primary`,
                //    `.card.elevated`, `Button#save`.
                for (auto &tok : tokens)
                {
                    SimpleSelector ss;

                    std::string base = tok;
                    std::string state;
                    size_t colon = tok.find(':');
                    if (colon != std::string::npos)
                    {
                        base = tok.substr(0, colon);
                        state = tok.substr(colon + 1);
                    }

                    // Split di `base` in componenti: ogni '.' o '#' inizia un
                    // nuovo componente. Il testo prima di qualsiasi marker è
                    // il tag (se presente).
                    std::vector<std::pair<SimpleSelector::Kind, std::string>> components;
                    {
                        size_t i = 0;
                        // Tag iniziale (fino al primo '.' o '#')
                        if (i < base.size() && base[i] != '.' && base[i] != '#')
                        {
                            size_t start = i;
                            while (i < base.size() && base[i] != '.' && base[i] != '#')
                                ++i;
                            components.push_back({SimpleSelector::Kind::Tag,
                                                  base.substr(start, i - start)});
                        }
                        // Componenti successivi
                        while (i < base.size())
                        {
                            char marker = base[i];
                            ++i;
                            size_t start = i;
                            while (i < base.size() && base[i] != '.' && base[i] != '#')
                                ++i;
                            std::string name = base.substr(start, i - start);
                            if (marker == '.')
                                components.push_back({SimpleSelector::Kind::Class, name});
                            else
                                components.push_back({SimpleSelector::Kind::Id, name});
                        }
                    }

                    // Il primo componente va in ss.kind / ss.name.
                    // Gli altri vanno in ss.extras.
                    if (components.empty())
                    {
                        // Token vuoto o malformato: salta.
                        continue;
                    }

                    ss.kind = components[0].first;
                    ss.name = components[0].second;
                    for (size_t ci = 1; ci < components.size(); ++ci)
                        ss.extras.push_back(components[ci]);

                    // Split per ':' per gestire stati composti tipo "checked:hover".
                    size_t pos = 0;
                    while (pos < state.size())
                    {
                        size_t next = state.find(':', pos);
                        std::string one = (next == std::string::npos)
                                              ? state.substr(pos)
                                              : state.substr(pos, next - pos);

                        if (one == "hover")
                            ss.requireHover = true;
                        else if (one == "pressed")
                            ss.requirePressed = true;
                        else if (one == "focus")
                            ss.requireFocus = true;
                        else if (one == "disabled")
                            ss.requireDisabled = true;
                        else if (one == "checked")
                            ss.requireChecked = true;
                        // stati sconosciuti: ignorati silenziosamente

                        if (next == std::string::npos)
                            break;
                        pos = next + 1;
                    }

                    rule.chain.push_back(std::move(ss));
                }

                rule.decls = decls;
                sheet.rules.push_back(std::move(rule));
            }
        }
    };

    // =========================================================================
    //  Auto-fill dei keyframe non uniformi + warning
    // =========================================================================
    inline void normalizeKeyframes(KeyframeAnimation &anim, const ParseLoc &loc)
    {
        if (anim.keyframes.size() < 2)
            return;

        // 1) Raccogli l'unione di tutte le prop
        std::set<std::string> allProps;
        for (const auto &k : anim.keyframes)
            forEachSetStyleProp(k.delta, [&](const char *n)
                                { allProps.insert(n); });

        if (allProps.empty())
            return;

        // 2) Per ogni keyframe, controlla le prop mancanti e fai auto-fill
        for (size_t i = 0; i < anim.keyframes.size(); ++i)
        {
            for (const auto &prop : allProps)
            {
                if (!hasStyleProp(anim.keyframes[i].delta, prop))
                {
                    // Cerca sorgente: precedente se esiste, altrimenti successivo
                    bool filled = false;
                    if (i > 0)
                        filled = copyStyleProp(anim.keyframes[i].delta, anim.keyframes[i - 1].delta, prop);
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
    inline void applyStyleSheet(const StyleSheet &sheet, const std::string &fileName, Theme &theme)
    {
        // :root → Style base
        for (auto &d : sheet.rootDecls)
        {
            ParseLoc loc{fileName, d.line, d.col};
            applyStyleDeclaration(theme.root, d.prop, d.value, loc);
        }

        // Regole normali → ThemeRule
        for (auto &r : sheet.rules)
        {
            ThemeRule tr;
            tr.chain = r.chain;
            tr.part = r.part;
            tr.media = r.media;

            for (auto &d : r.decls)
            {
                ParseLoc loc{fileName, d.line, d.col};
                applyStyleDeclaration(tr.style, d.prop, d.value, loc);
            }

            theme.addRule(std::move(tr));
        }

        // Keyframes (invariato)
        for (auto &kf : sheet.keyframes)
        {
            KeyframeAnimation anim;
            anim.name = kf.name;
            for (auto &frame : kf.frames)
            {
                float t = 0.0f;
                if (frame.ts == "from")
                    t = 0.0f;
                else if (frame.ts == "to")
                    t = 1.0f;
                else if (!frame.ts.empty() && frame.ts.back() == '%')
                    t = std::stof(frame.ts.substr(0, frame.ts.size() - 1)) / 100.0f;

                Keyframe k;
                k.t = t;
                for (auto &d : frame.decls)
                {
                    ParseLoc loc{fileName, d.line, d.col};
                    applyStyleDeclaration(k.delta, d.prop, d.value, loc);
                }
                anim.keyframes.push_back(std::move(k));
            }
            std::sort(anim.keyframes.begin(), anim.keyframes.end(),
                      [](const Keyframe &a, const Keyframe &b)
                      { return a.t < b.t; });

            ParseLoc kfLoc{fileName, kf.line, kf.col};
            normalizeKeyframes(anim, kfLoc);
            theme.addKeyframes(anim);
        }
    }

    // =========================================================================
    //  Entry point
    // =========================================================================
    inline bool loadStyleString(std::string_view src, Theme &theme = Theme::get(), const std::string &fileName = "<string>")
    {
        StyleParser parser(src, fileName);
        applyStyleSheet(parser.parse(), fileName, theme);
        return true;
    }

    inline bool loadStyleFile(const std::filesystem::path &path, Theme &theme = Theme::get())
    {
        std::ifstream f(path, std::ios::binary);
        if (!f)
        {
            logError("StyleParser", path.string(), 0, 0, "impossibile aprire il file");
            return false;
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return loadStyleString(ss.str(), theme, path.string());
    }

} // namespace ZenitUI::ZMarkup