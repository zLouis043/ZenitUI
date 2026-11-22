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
 *
 * Implementation lives in `src/StyleParser.cpp` (compiled into `libZenitUI`).
 */

#include "Common.hpp"
#include "Style.hpp"
#include "Theme.hpp"
#include "Media.hpp"
#include "Diagnostics.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ZenitUI::ZMarkup
{

    /** \brief Trim leading and trailing ASCII whitespace. */
    std::string_view trim(std::string_view s);

    /**
     * \brief Strip `// line` and `/* block *\/` comments from \p src.
     *
     * Respects string literals: a `//` inside a `"..."` is not treated as a
     * comment. Preserves newlines for accurate line numbering in diagnostics.
     */
    std::string stripComments(std::string_view src);

    /** \brief Split \p s on every occurrence of \p sep. Empty parts are preserved. */
    std::vector<std::string_view> splitBy(std::string_view s, char sep);

    /** \brief Split \p s on any ASCII whitespace. Empty parts are skipped. */
    std::vector<std::string_view> splitWs(std::string_view s);

    /**
     * \brief Parse a CSS duration (`"200ms"` → 0.2, `"1.5s"` → 1.5).
     *
     * Bare numbers are treated as seconds. Used by both `transition` and
     * `animation` shorthands.
     */
    float parseDurationSec(std::string_view s);

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
    void applyStyleDeclaration(Style &st,
                               std::string_view keyRaw,
                               std::string_view valRaw,
                               const SourceSpan &loc,
                               DiagnosticSink *sink = nullptr,
                               bool strict = false);

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
        /** \brief Construct a parser logging warnings through `Logger`. */
        StyleParser(std::string_view src, std::string fileName);

        /** \brief Construct a parser reporting diagnostics into \p sink. */
        StyleParser(std::string_view src, std::string fileName, DiagnosticSink &sink);

        /** \brief Enable strict mode: unknown properties and pseudo-classes become errors. */
        void setStrict(bool s) noexcept { strict_ = s; }

        /** \brief Parse the whole source into a `StyleSheet`. */
        StyleSheet parse();

    private:
        std::string src;
        std::string fileName;
        DiagnosticSink *sink_ = nullptr;
        bool strict_ = false;

        size_t p{0};
        int line{1};
        int col{1};
        StyleSheet sheet;

        SourceSpan spanFrom(int l, int c, std::uint32_t length = 0) const;
        DiagnosticSeverity propSeverity() const noexcept;

        bool atEnd() const { return p >= src.size(); }
        char peek() const { return atEnd() ? '\0' : src[p]; }
        char get();
        bool match(char c);
        bool matchStr(std::string_view s);
        void skipWs();
        std::string readIdent();

        void parseRoot();
        void parseKeyframes();
        void parseMedia();
        MediaQuery parseMediaQuery(const std::string &s);
        static float parseAspectRatio(const std::string &s);
        void parseRuleWithMedia(const MediaQuery &media);
        void parseRule(const MediaQuery *media = nullptr);
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
    void normalizeKeyframes(KeyframeAnimation &anim, const SourceSpan &loc);

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
    void applyStyleSheet(const StyleSheet &sheet,
                         Theme &theme,
                         DiagnosticSink *sink = nullptr,
                         bool strict = false);

    /**
     * \brief Parse \p src and apply it to \p theme.
     * \return `false` if the sink reported any error.
     */
    bool loadStyleString(std::string_view src,
                         Theme &theme = Theme::get(),
                         const std::string &fileName = "<string>",
                         DiagnosticSink *sink = nullptr,
                         bool strict = false);

    /**
     * \brief Load a stylesheet from a file and apply it to \p theme.
     * \return `false` if the file could not be read or a parse error occurred.
     */
    bool loadStyleFile(const std::filesystem::path &path,
                       Theme &theme = Theme::get(),
                       DiagnosticSink *sink = nullptr,
                       bool strict = false);

} // namespace ZenitUI::ZMarkup