#pragma once

/**
 * \file StyleParser.hpp
 * \brief CSS-like stylesheet parser and applier.
 *
 * Parses CSS-style source into a `StyleSheet`, then merges it into a `Theme`:
 *
 * \code{.css}
 * :root {
 *     --accent: #4a90e2;
 *     font: "Inter", sans-serif;
 * }
 *
 * Button {
 *     background: var(--accent);
 *     padding: 8px 16px;
 *     transition: background 0.2s ease;
 * }
 *
 * Button:hover { background: #5aa0f2; }
 * Button:disabled { opacity: 0.5; }
 *
 * @keyframes pulse {
 *     from { opacity: 1; }
 *     to   { opacity: 0.5; }
 * }
 *
 * @media (min-width: 800px) {
 *     Button { padding: 12px 24px; }
 * }
 * \endcode
 *
 * Supported features:
 *  - Type / class / id selectors, descendant combinator (space) only.
 *  - Pseudo-classes `:hover`, `:pressed`, `:focus`, `:disabled`, `:checked`.
 *  - Pseudo-elements `::part` (e.g. `Slider::knob`).
 *  - `:root` for the global inherited style.
 *  - `@keyframes` with `from`, `to`, `%` frames.
 *  - `@media (min-width: N) and (orientation: landscape)` etc.
 *  - `--custom-props` and `var(--name)` resolution.
 *  - Property-level transitions and animations (see `Style.hpp`).
 *
 * Not supported: child (`>`) and sibling (`+`, `~`) combinators, `@import`,
 * `:not()`, `:nth-child()`, `@supports`.
 *
 * Error handling: the parser produces diagnostics through an optional
 * `DiagnosticSink`. Without a sink, warnings are logged via `logWarn`.
 * `loadStyleString` / `loadStyleFile` return `false` if any error was
 * reported (errors only — warnings do not fail the load).
 */

#include "ZenitUI.hpp"
#include "ZMarkup.hpp"
#include "Logger.hpp"
#include "StyleAttr.hpp"
#include "Diagnostics.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>
#include <charconv>
#include <cctype>
#include <algorithm>

namespace ZenitUI::ZMarkup
{

    /** \brief Trim leading and trailing ASCII whitespace. */
    inline std::string_view trim(std::string_view s)
    {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace((unsigned char)s[a]))
            a++;
        while (b > a && std::isspace((unsigned char)s[b - 1]))
            b--;
        return s.substr(a, b - a);
    }

    /**
     * \brief Strip `// line` and `/* block *\/` comments from \p src.
     *
     * Respects string literals: a `//` inside a `"..."` is not treated as a
     * comment. Preserves newlines for accurate line numbering in diagnostics.
     */
    inline std::string stripComments(std::string_view src)
    {
        if (src.find("//") == std::string_view::npos &&
            src.find("/*") == std::string_view::npos)
        {
            return std::string(src);
        }
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

    /** \brief Split \p s on every occurrence of \p sep. Empty parts are preserved. */
    inline std::vector<std::string_view> splitBy(std::string_view s, char sep)
    {
        std::vector<std::string_view> out;
        size_t start = 0;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == sep)
            {
                out.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        }
        out.push_back(s.substr(start));
        return out;
    }

    /** \brief Split \p s on any ASCII whitespace. Empty parts are skipped. */
    inline std::vector<std::string_view> splitWs(std::string_view s)
    {
        std::vector<std::string_view> out;
        size_t start = 0;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (std::isspace((unsigned char)s[i]))
            {
                if (i > start)
                {
                    out.push_back(s.substr(start, i - start));
                }
                start = i + 1;
            }
        }
        if (start < s.size())
        {
            out.push_back(s.substr(start));
        }
        return out;
    }

    /**
     * \brief Parse a CSS duration (`"200ms"` → 0.2, `"1.5s"` → 1.5).
     *
     * Bare numbers are treated as seconds. Used by both `transition` and
     * `animation` shorthands.
     */
    inline float parseDurationSec(std::string_view s)
    {
        auto parseFloat = [](std::string_view sv) -> float
        {
            float value = 0.0f;
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
            if (ec == std::errc())
                return value;
            std::string tmp(sv);
            return std::strtof(tmp.c_str(), nullptr);
        };
        if (s.size() >= 2 && s.substr(s.size() - 2) == "ms")
            return parseFloat(s.substr(0, s.size() - 2)) / 1000.0f;
        if (!s.empty() && s.back() == 's')
            return parseFloat(s.substr(0, s.size() - 1));
        return parseFloat(s);
    }

    /**
     * \brief Apply one declaration to \p st.
     *
     * Handles:
     *  - `--name: value` (custom properties) → `st.customProps`.
     *  - Values containing `var(...)` → `st.unresolvedProps` for late binding.
     *  - `transition` / `transition-*` shorthands and longhands.
     *  - `animation` / `animation-*` shorthands and longhands.
     *  - `filter` / `box-shadow` / `overflow` / `overflow-x` / `overflow-y`.
     *  - Everything else → `applyStyleAttr`.
     *
     * Reports an `UnknownProperty` warning (or error in strict mode) when
     * `applyStyleAttr` returns false, with "did you mean" suggestions drawn
     * from `getKnownStyleProperties`.
     *
     * \param st      Destination style.
     * \param keyRaw  Property name (trimmed internally).
     * \param valRaw  Value string (trimmed internally).
     * \param loc     Source location for diagnostics.
     * \param sink    Optional diagnostic sink; if null, warnings go to `logWarn`.
     * \param strict  If true, unknown properties become errors instead of warnings.
     */
    inline void applyStyleDeclaration(Style &st, std::string_view keyRaw, std::string_view valRaw, const SourceSpan &loc, DiagnosticSink *sink = nullptr, bool strict = false)
    {
        std::string_view k = trim(keyRaw);
        std::string_view v = trim(valRaw);

        if (k.rfind("--", 0) == 0)
        {
            st.customProps[std::string(k)] = std::string(v);
            return;
        }

        if (v.find("var(") != std::string_view::npos)
        {
            st.unresolvedProps[std::string(k)] = std::string(v);
            clearStyleProp(st, std::string(k));
            return;
        }

        if (k == "wrap")
        {
            st.wrap = (v == "true" || v == "1" || v == "wrap");
            return;
        }

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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'transition-delay' used without a preceding 'transition:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'transition-delay' used without a 'transition:' block previously. Ignored.");
                }
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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'transition-duration' used without a preceding 'transition:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'transition-duration' used without a 'transition:' block previously. Ignored.");
                }
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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'transition-timing-function' used without a preceding 'transition:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'transition-timing-function'used without a 'transition:' block previously. Ignored.");
                }
                return;
            }
            TransitionFunction tf;
            if (parseEasing(v, tf))
                for (auto &s : st.transitions.value)
                    s.ease = tf;
            return;
        }

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
                    int n = 0;
                    auto [ptr, ec] = std::from_chars(t.data(), t.data() + t.size(), n);
                    if (ec == std::errc())
                    {
                        a.iterations = n;
                        continue;
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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'animation-delay' used without a preceding 'animation:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'animation-delay' used without a 'animation:' previously. Ignored.");
                }
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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'animation-duration' used without a preceding 'animation:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'animation-duration' used without a 'animation:' block previously. Ignored.");
                }
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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'animation-timing-function' used without a preceding 'animation:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'animation-timing-function' used without a 'animation:' block previously. Ignored.");
                }
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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'animation-iteration-count' used without a preceding 'animation:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'animation-iteration-count' used without a 'animation:' block previously. Ignored.");
                }
                return;
            }
            int n = 1;
            if (v == "infinite")
            {
                n = -1;
            }
            else
            {
                auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), n);
                if (ec != std::errc())
                    return;
            }
            for (auto &a : st.animations.value)
                a.iterations = n;
            return;
        }
        if (k == "animation-direction")
        {
            if (!st.animations.is_set)
            {
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'animation-direction' used without a preceding 'animation:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'animation-direction' used without a 'animation:' block previously. Ignored.");
                }
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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::UnexpectedToken, DiagnosticSeverity::Warning, loc, "'animation-fill-mode' used without a preceding 'animation:' block. Ignored."));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "'animation-fill-mode' used without a 'animation:' block previously. Ignored.");
                }
                return;
            }
            bool fwd = (v == "forwards" || v == "both");
            for (auto &a : st.animations.value)
                a.fillForwards = fwd;
            return;
        }

        if (k == "filter")
        {
            std::vector<FilterRef> refs;
            std::string cur(v);
            size_t pos = 0;
            while (pos < cur.size())
            {
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
                std::string part = std::string(trim(std::string_view(cur).substr(pos, comma - pos)));
                pos = (comma < cur.size()) ? comma + 1 : cur.size();
                if (part.empty())
                    continue;

                FilterRef ref;
                size_t lparen = part.find('(');
                if (lparen == std::string::npos)
                {
                    ref.name = std::string(trim(part));
                }
                else
                {
                    size_t rparen = part.find(')', lparen);
                    ref.name = std::string(trim(std::string_view(part).substr(0, lparen)));
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
                            ref.args.push_back(std::string(trim(curArg)));
                            curArg.clear();
                        }
                        else
                        {
                            curArg.push_back(c);
                        }
                    }
                    if (!curArg.empty())
                        ref.args.push_back(std::string(trim(curArg)));
                }
                refs.push_back(std::move(ref));
            }
            st.filters = refs;
            return;
        }

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

        auto parseOverflowValue = [](std::string_view val, Overflow &out) -> bool
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
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::InvalidValue, DiagnosticSeverity::Warning, loc, "overflow value not recognized: '" + std::string(v) + "'"));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "overflow value not recognized: '" + std::string(v) + "'. Ignored.");
                }
            }
            return;
        }
        if (k == "overflow-x")
        {
            Overflow o;
            if (parseOverflowValue(v, o))
                st.overflowX = o;
            else
            {
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::InvalidValue, DiagnosticSeverity::Warning, loc, "overflow-x value not recognized: '" + std::string(v) + "'"));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "overflow-x value not recognized: '" + std::string(v) + "'. Ignored.");
                }
            }
            return;
        }
        if (k == "overflow-y")
        {
            Overflow o;
            if (parseOverflowValue(v, o))
                st.overflowY = o;
            else
            {
                if (sink)
                {
                    sink->report(makeDiagnostic(DiagnosticCode::InvalidValue, DiagnosticSeverity::Warning, loc, "overflow-y value not recognized: '" + std::string(v) + "'"));
                }
                else
                {
                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "overflow-y value not recognized: '" + std::string(v) + "'. Ignored.");
                }
            }
            return;
        }

        if (!applyStyleAttr(st, k, v))
        {
            if (sink)
            {
                DiagnosticSeverity sev = strict ? DiagnosticSeverity::Error : DiagnosticSeverity::Warning;
                auto d = makeDiagnostic(
                    DiagnosticCode::UnknownProperty,
                    sev,
                    loc,
                    "unknown property '" + std::string(k) + "'",
                    [&]
                    {
                        auto sugg = suggestSimilar(k, getKnownStyleProperties());
                        if (sugg.empty())
                            return std::string{};
                        if (sugg.size() == 1)
                            return "did you mean '" + sugg[0] + "'?";
                        std::string s = "did you mean one of: ";
                        for (std::size_t i = 0; i < sugg.size(); ++i)
                        {
                            if (i)
                                s += ", ";
                            s += "'" + sugg[i] + "'";
                        }
                        return s + "?";
                    }());
                sink->report(std::move(d));
            }
            else
            {
                logWarn("StyleParser", loc.file, loc.line, loc.column,
                        "unknown property '" + std::string(k) + "'. Ignored.");
            }
        }
    }

    /** \brief One parsed `key: value;` declaration, with source location. */
    struct Declaration
    {
        std::string prop;
        std::string value;
        SourceSpan loc{};
    };

    /**
     * \struct StyleSheet
     * \brief Intermediate representation: parsed rules, keyframes and root vars.
     *
     * Produced by `StyleParser::parse()` and consumed by `applyStyleSheet`.
     * Storing the IR separately from `Theme` allows validating a stylesheet
     * without mutating the live theme, and makes tests easier.
     */
    struct StyleSheet
    {
        /** \brief One selector-plus-declarations entry. */
        struct Rule
        {
            std::vector<SimpleSelector> chain;
            std::string part;
            std::vector<Declaration> decls;
            SourceSpan loc{};
            std::optional<MediaQuery> media;
        };
        std::vector<Rule> rules;

        /** \brief One `from`/`to`/`N%` frame inside `@keyframes`. */
        struct KfFrame
        {
            std::string ts;
            std::vector<Declaration> decls;
            SourceSpan loc{};
        };
        /** \brief One `@keyframes name { ... }` block. */
        struct KfRule
        {
            std::string name;
            std::vector<KfFrame> frames;
            SourceSpan loc{};
        };
        std::vector<KfRule> keyframes;

        std::unordered_map<std::string, std::string> vars;
        std::vector<Declaration> rootDecls;
    };

    /**
     * \class StyleParser
     * \brief Hand-written recursive-descent CSS-like parser.
     *
     * Stateful; create one instance per source. Tracks line / column for
     * accurate `SourceSpan`s in diagnostics.
     */
    class StyleParser
    {
    public:
        StyleParser(std::string_view src, std::string fileName)
            : src(stripComments(src)), fileName(std::move(fileName)) {}

        StyleParser(std::string_view src, std::string fileName, DiagnosticSink &sink)
            : src(stripComments(src)), fileName(std::move(fileName)), sink_(&sink) {}

        /** \brief Enable strict mode: unknown properties become errors. */
        void setStrict(bool s) noexcept { strict_ = s; }

        /** \brief Parse the whole source into a `StyleSheet`. */
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
        DiagnosticSink *sink_ = nullptr;
        bool strict_ = false;

        size_t p{0};
        int line{1};
        int col{1};
        StyleSheet sheet;

        SourceSpan spanFrom(int l, int c, std::uint32_t length = 0) const
        {
            return SourceSpan{fileName, static_cast<std::uint32_t>(l),
                              static_cast<std::uint32_t>(c), length};
        }

        DiagnosticSeverity propSeverity() const noexcept
        {
            return strict_ ? DiagnosticSeverity::Error : DiagnosticSeverity::Warning;
        }

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

                std::string p = std::string(trim(prop));
                std::string v = std::string(trim(val));

                if (p.rfind("--", 0) == 0)
                {
                    sheet.vars[p] = v;
                }

                Declaration d;
                d.prop = p;
                d.value = v;
                d.loc = spanFrom(declLine, declCol, static_cast<std::uint32_t>(prop.size()));
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
            kf.loc = spanFrom(kfLine, kfCol);

            while (!atEnd() && peek() != '}')
            {
                skipWs();
                if (peek() == '}')
                    break;

                int frameLine = line, frameCol = col;
                std::string ts;
                while (!atEnd() && peek() != '{')
                    ts.push_back(get());
                ts = std::string(trim(ts));
                skipWs();
                if (!match('{'))
                    break;

                StyleSheet::KfFrame frame;
                frame.ts = ts;
                frame.loc = spanFrom(frameLine, frameCol);

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
                    d.loc = spanFrom(declLine, declCol, static_cast<std::uint32_t>(prop.size()));
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

            std::string cond;
            while (!atEnd() && peek() != '{')
                cond.push_back(get());
            cond = std::string(trim(cond));
            if (!match('{'))
                return;

            MediaQuery query = parseMediaQuery(cond);

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
                    parts.push_back(std::string(trim(cur)));
                    cur.clear();
                    i += 4;
                }
                else
                    cur.push_back(c);
            }
            if (!cur.empty())
                parts.push_back(std::string(trim(cur)));

            for (auto &p : parts)
            {
                std::string inner = p;
                if (inner.size() >= 2 && inner.front() == '(' && inner.back() == ')')
                    inner = inner.substr(1, inner.size() - 2);

                size_t colon = inner.find(':');
                std::string_view keySrc = (colon == std::string::npos)
                                              ? std::string_view(inner)
                                              : std::string_view(inner).substr(0, colon);
                std::string key(trim(keySrc));
                std::string val;
                if (colon != std::string::npos)
                {
                    val = trim(std::string_view(inner).substr(colon + 1));
                }

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
            parseRule(&media);
        }

        void parseRule(const MediaQuery *media = nullptr)
        {
            int ruleLine = line, ruleCol = col;
            std::string selector;
            while (!atEnd() && peek() != '{')
                selector.push_back(get());
            selector = std::string(trim(selector));
            skipWs();
            if (!match('{'))
                return;

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
                d.loc = spanFrom(declLine, declCol, static_cast<std::uint32_t>(prop.size()));
                decls.push_back(std::move(d));
            }
            match('}');

            std::vector<std::string> selectors;
            {
                std::string cur;
                for (char c : selector)
                {
                    if (c == ',')
                    {
                        selectors.push_back(std::string(trim(cur)));
                        cur.clear();
                    }
                    else
                        cur.push_back(c);
                }
                selectors.push_back(std::string(trim(cur)));
            }

            for (auto &rawSel : selectors)
            {
                std::string sel = std::string(trim(rawSel));
                if (sel.empty())
                    continue;

                StyleSheet::Rule rule;
                rule.loc = spanFrom(ruleLine, ruleCol);
                if (media)
                    rule.media = *media;

                std::string chainPart = sel;
                size_t dcolon = sel.rfind("::");
                if (dcolon != std::string::npos)
                {
                    chainPart = sel.substr(0, dcolon);
                    rule.part = std::string(trim(std::string_view(sel).substr(dcolon + 2)));
                }

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

                    std::vector<std::pair<SimpleSelector::Kind, std::string>> components;
                    {
                        size_t i = 0;
                        if (i < base.size() && base[i] != '.' && base[i] != '#')
                        {
                            size_t start = i;
                            while (i < base.size() && base[i] != '.' && base[i] != '#')
                                ++i;
                            components.push_back({SimpleSelector::Kind::Tag,
                                                  base.substr(start, i - start)});
                        }
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

                    if (components.empty())
                    {
                        continue;
                    }

                    ss.kind = components[0].first;
                    ss.name = components[0].second;
                    for (size_t ci = 1; ci < components.size(); ++ci)
                        ss.extras.push_back(components[ci]);

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
                        else if (!one.empty())
                        {
                            if (sink_)
                            {
                                sink_->report(makeDiagnostic(
                                    DiagnosticCode::UnknownPseudo, propSeverity(),
                                    spanFrom(ruleLine, ruleCol, static_cast<std::uint32_t>(one.size())),
                                    "unknown pseudo-class ':" + one + "'"));
                            }
                            else
                            {
                                logWarn("StyleParser", fileName, ruleLine, ruleCol, "unknown pseudo-class ':" + one + "'. Ignored.");
                            }
                        }

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

    /**
     * \brief Fill missing properties in every keyframe from neighbouring frames.
     *
     * CSS animations that declare a property only at the start and end still
     * need all keyframes to hold a value for that property, so that the
     * evaluator can interpolate uniformly. This function fills gaps by
     * copying from the previous keyframe, or from the next if the gap is at
     * the beginning. Emits a `logWarn` for each auto-filled property to make
     * authoring errors visible.
     */
    inline void normalizeKeyframes(KeyframeAnimation &anim, const SourceSpan &loc)
    {
        if (anim.keyframes.size() < 2)
            return;

        std::set<std::string> allProps;
        for (const auto &k : anim.keyframes)
            forEachSetStyleProp(k.delta, [&](const char *n)
                                { allProps.insert(n); });

        if (allProps.empty())
            return;

        for (size_t i = 0; i < anim.keyframes.size(); ++i)
        {
            for (const auto &prop : allProps)
            {
                if (!hasStyleProp(anim.keyframes[i].delta, prop))
                {
                    bool filled = false;
                    if (i > 0)
                        filled = copyStyleProp(anim.keyframes[i].delta, anim.keyframes[i - 1].delta, prop);
                    if (!filled && i + 1 < anim.keyframes.size())
                        filled = copyStyleProp(anim.keyframes[i].delta, anim.keyframes[i + 1].delta, prop);

                    logWarn("StyleParser", loc.file, loc.line, loc.column,
                            "@keyframes " + anim.name + ": keyframe t=" + std::to_string(anim.keyframes[i].t) +
                                " do not declare '" + prop + "'. Auto-fill from keyframe " +
                                (i > 0 ? "prior" : "next") + ".");
                }
            }
        }
    }

    /**
     * \brief Apply a parsed `StyleSheet` to a `Theme`.
     *
     * Order of operations:
     *  1. Merge `rootDecls` into `theme.root`.
     *  2. For each rule, build a `ThemeRule` (selector chain + part + media)
     *     by applying its declarations to a fresh `Style`, then `addRule`.
     *  3. For each keyframes block, validate uniqueness (warn on duplicate),
     *     sort frames by time, normalize, and `addKeyframes`.
     *
     * \param sheet   Parsed stylesheet.
     * \param theme   Destination theme.
     * \param sink    Optional diagnostic sink.
     * \param strict  When true, unknown properties become errors.
     */
    inline void applyStyleSheet(const StyleSheet &sheet, Theme &theme, DiagnosticSink *sink = nullptr, bool strict = false)
    {
        for (auto &d : sheet.rootDecls)
        {
            applyStyleDeclaration(theme.root, d.prop, d.value, d.loc, sink, strict);
        }

        for (auto &r : sheet.rules)
        {
            ThemeRule tr;
            tr.chain = r.chain;
            tr.part = r.part;
            tr.media = r.media;

            for (auto &d : r.decls)
            {
                applyStyleDeclaration(tr.style, d.prop, d.value, d.loc, sink, strict);
            }

            theme.addRule(std::move(tr));
        }

        std::set<std::string> seenKf;
        for (auto &kf : sheet.keyframes)
        {
            if (seenKf.count(kf.name))
            {
                if (sink)
                {
                    DiagnosticSeverity sev = strict ? DiagnosticSeverity::Error : DiagnosticSeverity::Warning;
                    sink->report(makeDiagnostic(DiagnosticCode::DuplicateRule, sev, kf.loc, "duplicate @keyframes '" + kf.name + "'"));
                }
            }
            seenKf.insert(kf.name);

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
                    applyStyleDeclaration(k.delta, d.prop, d.value, d.loc, sink, strict);
                }
                anim.keyframes.push_back(std::move(k));
            }
            std::sort(anim.keyframes.begin(), anim.keyframes.end(), [](const Keyframe &a, const Keyframe &b)
                      { return a.t < b.t; });

            normalizeKeyframes(anim, kf.loc);
            theme.addKeyframes(anim);
        }
    }

    /**
     * \brief Parse \p src and apply it to \p theme.
     * \return `false` if the sink reported any error.
     */
    inline bool loadStyleString(std::string_view src, Theme &theme = Theme::get(), const std::string &fileName = "<string>", DiagnosticSink *sink = nullptr, bool strict = false)
    {
        NullDiagnosticSink nullSink;
        DiagnosticSink &actualSink = sink ? *sink : nullSink;

        StyleParser parser(src, fileName, actualSink);
        parser.setStrict(strict);
        applyStyleSheet(parser.parse(), theme, &actualSink, strict);
        return !actualSink.hasErrors();
    }

    /**
     * \brief Load a stylesheet from a file and apply it to \p theme.
     * \return `false` if the file could not be read or a parse error occurred.
     */
    inline bool loadStyleFile(const std::filesystem::path &path, Theme &theme = Theme::get(), DiagnosticSink *sink = nullptr, bool strict = false)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f)
        {
            logError("StyleParser", path.string(), 0, 0, "impossibile aprire il file");
            return false;
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return loadStyleString(ss.str(), theme, path.string(), sink, strict);
    }

} // namespace ZenitUI::ZMarkup