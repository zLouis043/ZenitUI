#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace ZenitUI {

// ---------------------------------------------------------------- SourceSpan

struct SourceSpan {
    std::string file;
    std::uint32_t line   = 0;  // 1-based
    std::uint32_t column = 0;  // 1-based
    std::uint32_t length = 0;  // 0 = unknown

    [[nodiscard]] bool valid() const noexcept { return line != 0 && column != 0; }
};

// ---------------------------------------------------------- Codes / severity

enum class DiagnosticSeverity { Info, Warning, Error };

enum class DiagnosticCode {
    // parser
    UnterminatedString,
    UnbalancedBrace,
    UnexpectedToken,

    // resolver / dichiarazioni
    UnknownProperty,
    InvalidValue,
    UnknownPseudo,
    UnknownPart,
    UnresolvedVariable,
    UnusedVariable,

    // selettori (post-build)
    UnknownSelectorTag,
    SelectorMatchesNothing,
    DuplicateRule,
};

[[nodiscard]] const char* toString(DiagnosticSeverity s) noexcept;
[[nodiscard]] const char* toString(DiagnosticCode c) noexcept;

// ----------------------------------------------------------- Diagnostic

struct ZStyleDiagnostic {
    DiagnosticCode     code{};
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    SourceSpan         span{};
    std::string        message;
    std::string        hint;                    // "did you mean 'color'?"
    std::optional<SourceSpan> related;          // es. definizione di --x
};

[[nodiscard]] ZStyleDiagnostic
makeDiagnostic(DiagnosticCode code,
               DiagnosticSeverity severity,
               SourceSpan span,
               std::string message,
               std::string hint = {});

// ---------------------------------------------------------------- Sinks

class DiagnosticSink {
public:
    virtual ~DiagnosticSink() = default;
    virtual void report(ZStyleDiagnostic d) = 0;
    [[nodiscard]] virtual bool hasErrors() const = 0;
    [[nodiscard]] virtual std::vector<ZStyleDiagnostic> sorted() const = 0;
};

class NullDiagnosticSink final : public DiagnosticSink {
public:
    void report(ZStyleDiagnostic) override {}
    [[nodiscard]] bool hasErrors() const override { return false; }
    [[nodiscard]] std::vector<ZStyleDiagnostic> sorted() const override { return {}; }
};

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

// ---------------------------------------------------------------- Helpers

[[nodiscard]] std::size_t editDistance(std::string_view a, std::string_view b);

[[nodiscard]] std::vector<std::string>
suggestSimilar(std::string_view unknown,
               const std::vector<std::string_view>& candidates,
               std::size_t maxDistance   = 2,
               std::size_t maxSuggestions = 3);

void formatDiagnostic(const ZStyleDiagnostic& d, std::ostream& os);

[[nodiscard]] const std::vector<std::string_view>& getKnownStyleProperties();

} // namespace ZenitUI