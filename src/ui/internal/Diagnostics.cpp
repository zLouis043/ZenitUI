#include "Diagnostics.hpp"
#include "Style.hpp"
#include <algorithm>
#include <sstream>

namespace ZenitUI {

const char* toString(DiagnosticSeverity s) noexcept {
    switch (s) {
        case DiagnosticSeverity::Info:    return "info";
        case DiagnosticSeverity::Warning: return "warning";
        case DiagnosticSeverity::Error:   return "error";
    }
    return "?";
}

const char* toString(DiagnosticCode c) noexcept {
    switch (c) {
        case DiagnosticCode::UnterminatedString:     return "UnterminatedString";
        case DiagnosticCode::UnbalancedBrace:        return "UnbalancedBrace";
        case DiagnosticCode::UnexpectedToken:        return "UnexpectedToken";
        case DiagnosticCode::UnknownProperty:        return "UnknownProperty";
        case DiagnosticCode::InvalidValue:           return "InvalidValue";
        case DiagnosticCode::UnknownPseudo:          return "UnknownPseudo";
        case DiagnosticCode::UnknownPart:            return "UnknownPart";
        case DiagnosticCode::UnresolvedVariable:     return "UnresolvedVariable";
        case DiagnosticCode::UnusedVariable:         return "UnusedVariable";
        case DiagnosticCode::UnknownSelectorTag:     return "UnknownSelectorTag";
        case DiagnosticCode::SelectorMatchesNothing: return "SelectorMatchesNothing";
        case DiagnosticCode::DuplicateRule:          return "DuplicateRule";
    }
    return "?";
}

ZStyleDiagnostic makeDiagnostic(DiagnosticCode code, DiagnosticSeverity sev, SourceSpan span, std::string message, std::string hint) {
    ZStyleDiagnostic d;
    d.code = code;
    d.severity = sev;
    d.span = std::move(span);
    d.message = std::move(message);
    d.hint = std::move(hint);
    return d;
}

std::vector<ZStyleDiagnostic> CollectingDiagnosticSink::sorted() const {
    auto out = diagnostics_;
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        if (a.span.file != b.span.file) return a.span.file < b.span.file;
        if (a.span.line != b.span.line) return a.span.line < b.span.line;
        return a.span.column < b.span.column;
    });
    return out;
}

std::size_t editDistance(std::string_view a, std::string_view b) {
    const std::size_t n = a.size(), m = b.size();
    if (n == 0) return m;
    if (m == 0) return n;
    std::vector<std::size_t> prev(m + 1), cur(m + 1);
    for (std::size_t j = 0; j <= m; ++j) prev[j] = j;
    for (std::size_t i = 1; i <= n; ++i) {
        cur[0] = i;
        for (std::size_t j = 1; j <= m; ++j) {
            const std::size_t cost = (a[i-1] == b[j-1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j-1] + 1, prev[j-1] + cost});
        }
        std::swap(prev, cur);
    }
    return prev[m];
}

std::vector<std::string> suggestSimilar(std::string_view unknown, const std::vector<std::string_view>& candidates, std::size_t maxDistance, std::size_t maxSuggestions) {
    std::vector<std::pair<std::size_t, std::string>> scored;
    for (auto c : candidates) {
        const auto d = editDistance(unknown, c);
        if (d <= maxDistance) scored.emplace_back(d, std::string(c));
    }
    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first < b.first;
        return a.second < b.second;
    });
    if (scored.size() > maxSuggestions) scored.resize(maxSuggestions);
    std::vector<std::string> out;
    out.reserve(scored.size());
    for (auto& s : scored) out.push_back(std::move(s.second));
    return out;
}

void formatDiagnostic(const ZStyleDiagnostic& d, std::ostream& os) {
    os << d.span.file << ':' << d.span.line << ':' << d.span.column
       << ": " << toString(d.severity) << ": " << d.message
       << " [" << toString(d.code) << "]\n";
    if (!d.hint.empty()) {
        os << "    hint: " << d.hint << '\n';
    }
    if (d.related && d.related->valid()) {
        os << "    note: see " << d.related->file << ':'
           << d.related->line << ':' << d.related->column << '\n';
    }
}

const std::vector<std::string_view>& getKnownStyleProperties() {
    static const std::vector<std::string_view> kNames = {
        "width", "height", "min-width", "min-height", "max-width", "max-height",
        "grow", "shrink", "gap", "padding", "margin", 
        "align-h", "align-v", "items-h", "items-v", "justify", 
        "position", "top", "left", "right", "bottom", "z-index", 
        "background", "background-texture", "tint", 
        "border-color", "border-width", "radius", "opacity",
        "scale", "rotation", "translate-x", "translate-y",
        "font", "font-size", "letter-spacing", "color", "text-align", 
        "transition", "transition-delay", "transition-duration", "transition-timing-function",
        "animation", "animation-delay", "animation-duration", "animation-timing-function", 
        "animation-iteration-count", "animation-direction", "animation-fill-mode",
        "filter", "effect", "box-shadow", "overflow", "overflow-x", "overflow-y"
    };
    return kNames;
}

} // namespace ZenitUI