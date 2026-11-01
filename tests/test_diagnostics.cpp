#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "StyleParser.hpp"
#include "Diagnostics.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;
using namespace ZenitUI::Test;


TEST(Diagnostics, valid_style_no_diagnostics) {
    CollectingDiagnosticSink sink;
    Theme t;
    loadStyleString("Button { color: #fff; padding: 4px; }", t, "<string>", &sink, false);
    CHECK(sink.diagnostics().empty());
    CHECK(!sink.hasErrors());
}

TEST(Diagnostics, unknown_property_emits_warning) {
    CollectingDiagnosticSink sink;
    Theme t;
    loadStyleString("Button { colr: #fff; }", t, "test.zstyle", &sink, false);
    CHECK(sink.diagnostics().size() == 1);
    
    const auto& d = sink.diagnostics().front();
    CHECK(d.code == DiagnosticCode::UnknownProperty);
    CHECK(d.severity == DiagnosticSeverity::Warning);
    CHECK(d.hint.find("color") != std::string::npos);
    CHECK(!sink.hasErrors());
}

TEST(Diagnostics, unknown_property_strict_emits_error) {
    CollectingDiagnosticSink sink;
    Theme t;
    loadStyleString("Button { colr: #fff; }", t, "test.zstyle", &sink, true);
    CHECK(sink.diagnostics().size() == 1);
    CHECK(sink.diagnostics().front().severity == DiagnosticSeverity::Error);
    CHECK(sink.hasErrors());
}

TEST(Diagnostics, unknown_pseudo_class) {
    CollectingDiagnosticSink sink;
    Theme t;
    loadStyleString("Button:hovver { color: red; }", t, "test.zstyle", &sink, false);
    CHECK(sink.diagnostics().size() == 1);
    CHECK(sink.diagnostics().front().code == DiagnosticCode::UnknownPseudo);
}

TEST(Diagnostics, duplicate_keyframes) {
    CollectingDiagnosticSink sink;
    Theme t;
    loadStyleString(R"(
        @keyframes pulse { 0% { opacity: 0; } 100% { opacity: 1; } }
        @keyframes pulse { 0% { scale: 1; } 100% { scale: 2; } }
    )", t, "test.zstyle", &sink, false);
    
    CHECK(sink.diagnostics().size() == 1);
    CHECK(sink.diagnostics().front().code == DiagnosticCode::DuplicateRule);
}


TEST(Diagnostics, edit_distance_basics) {
    CHECK(editDistance("color", "color") == 0);
    CHECK(editDistance("colr", "color") == 1);
    CHECK(editDistance("colour", "color") == 1);
    CHECK(editDistance("red", "blue") == 4);
}