#pragma once

/**
 * \file Diagnostics.hpp
 * \brief Structured diagnostics for the CSS parser and style resolver.
 *
 * The style subsystem can produce errors, warnings and informational
 * messages with source spans, severities, machine-readable codes, optional
 * "did you mean" suggestions, and cross-references to related spans.
 *
 * The `DiagnosticSink` interface decouples reporting from consumption: the
 * parser writes to any sink, tests can use `CollectingDiagnosticSink`, and
 * production code can use `NullDiagnosticSink` to discard everything.
 *
 * `loadStyleString` / `loadStyleFile` (in `StyleParser.hpp`) accept an
 * optional sink and return `false` if any error was reported.
 */

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace ZenitUI {

    /**
     * \brief Source location for a diagnostic.
     *
     * `line` and `column` are 1-based. `length` is 0 when unknown (e.g. for
     * diagnostics that point at a whole rule).
     */
    struct SourceSpan {
        std::string file;
        std::uint32_t line   = 0;
        std::uint32_t column = 0;
        std::uint32_t length = 0;

        [[nodiscard]] bool valid() const noexcept { return line != 0 && column != 0; }
    };

    /** \brief Diagnostic severity level. */
    enum class DiagnosticSeverity { Info, Warning, Error };

    /**
     * \brief Machine-readable diagnostic identifier.
     *
     * Grouped by subsystem:
     *  - Lexical / syntactic: `UnterminatedString`, `UnbalancedBrace`, `UnexpectedToken`
     *  - Declaration-level: `UnknownProperty`, `InvalidValue`, `UnknownPseudo`,
     *    `UnknownPart`, `UnresolvedVariable`, `UnusedVariable`
     *  - Selector-level: `UnknownSelectorTag`, `SelectorMatchesNothing`, `DuplicateRule`
     *
     * Tools can switch on `code` without matching localized message strings.
     */
    enum class DiagnosticCode {
        UnterminatedString,
        UnbalancedBrace,
        UnexpectedToken,

        UnknownProperty,
        InvalidValue,
        UnknownPseudo,
        UnknownPart,
        UnresolvedVariable,
        UnusedVariable,

        UnknownSelectorTag,
        SelectorMatchesNothing,
        DuplicateRule,
    };

    [[nodiscard]] const char* toString(DiagnosticSeverity s) noexcept;
    [[nodiscard]] const char* toString(DiagnosticCode c) noexcept;

    /** \brief A fully-formed diagnostic record. */
    struct ZStyleDiagnostic {
        DiagnosticCode     code{};
        DiagnosticSeverity severity{DiagnosticSeverity::Error};
        SourceSpan         span{};
        std::string        message;
        std::string        hint;
        std::optional<SourceSpan> related;
    };

    /**
     * \brief Build a diagnostic record.
     * \param code      Machine-readable identifier.
     * \param severity  Severity level.
     * \param span      Primary source location.
     * \param message   Human-readable message.
     * \param hint      Optional suggestion text.
     */
    [[nodiscard]] ZStyleDiagnostic
    makeDiagnostic(DiagnosticCode code,
                   DiagnosticSeverity severity,
                   SourceSpan span,
                   std::string message,
                   std::string hint = {});

    /**
     * \class DiagnosticSink
     * \brief Abstract destination for diagnostics.
     *
     * Implementations must be safe to call from any point in parsing; the
     * `CollectingDiagnosticSink` is the canonical in-memory implementation.
     */
    class DiagnosticSink {
    public:
        virtual ~DiagnosticSink() = default;
        virtual void report(ZStyleDiagnostic d) = 0;
        [[nodiscard]] virtual bool hasErrors() const = 0;
        [[nodiscard]] virtual std::vector<ZStyleDiagnostic> sorted() const = 0;
    };

    /** \brief Sink that discards every diagnostic. */
    class NullDiagnosticSink final : public DiagnosticSink {
    public:
        void report(ZStyleDiagnostic) override {}
        [[nodiscard]] bool hasErrors() const override { return false; }
        [[nodiscard]] std::vector<ZStyleDiagnostic> sorted() const override { return {}; }
    };

    /**
     * \brief Sink that accumulates diagnostics in insertion order.
     *
     * `sorted()` returns a copy sorted by (file, line, column), useful for
     * stable test output. `hasErrors()` checks for any `Error` severity.
     */
    class CollectingDiagnosticSink final : public DiagnosticSink {
    public:
        void report(ZStyleDiagnostic d) override {
            diagnostics_.push_back(std::move(d));
        }
        [[nodiscard]] bool hasErrors() const override {
            for (const auto& d : diagnostics_)
                if (d.severity == DiagnosticSeverity::Error) return true;
            return false;
        }
        [[nodiscard]] std::vector<ZStyleDiagnostic> sorted() const override;
        [[nodiscard]] const std::vector<ZStyleDiagnostic>& diagnostics() const noexcept {
            return diagnostics_;
        }
        void clear() { diagnostics_.clear(); }

    private:
        std::vector<ZStyleDiagnostic> diagnostics_;
    };

    /**
     * \brief Levenshtein edit distance between \p a and \p b.
     *
     * O(n*m) time, O(min(n,m)) space. Used by `suggestSimilar` for "did you
     * mean" hints; strings of length > 100 are rare in this codebase.
     */
    [[nodiscard]] std::size_t editDistance(std::string_view a, std::string_view b);

    /**
     * \brief Suggest candidates within \p maxDistance edits of \p unknown.
     * \param unknown        The misspelled identifier.
     * \param candidates     List of valid identifiers.
     * \param maxDistance    Maximum edit distance to consider.
     * \param maxSuggestions Maximum number of suggestions to return.
     * \return Suggestions sorted by (distance, alphabetical).
     */
    [[nodiscard]] std::vector<std::string>
    suggestSimilar(std::string_view unknown,
                   const std::vector<std::string_view>& candidates,
                   std::size_t maxDistance   = 2,
                   std::size_t maxSuggestions = 3);

    /**
     * \brief Write a diagnostic to \p os in
     *        `file:line:col: severity: message [code]` format.
     *
     * Appends the `hint:` line when non-empty, and the `note: see …` line
     * when `related` is set.
     */
    void formatDiagnostic(const ZStyleDiagnostic& d, std::ostream& os);

    /**
     * \brief Return the list of style property names known to the parser.
     *
     * Used by `suggestSimilar` when producing `UnknownProperty` hints.
     */
    [[nodiscard]] const std::vector<std::string_view>& getKnownStyleProperties();

} // namespace ZenitUI