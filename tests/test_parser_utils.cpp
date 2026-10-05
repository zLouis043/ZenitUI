#include "TestFramework.hpp"
#include "StyleParser.hpp"

using namespace ZenitUI::ZMarkup;

TEST(ParserUtils_trim, empty) {
    CHECK(trim("") == "");
}

TEST(ParserUtils_trim, no_whitespace) {
    CHECK(trim("hello") == "hello");
}

TEST(ParserUtils_trim, leading_and_trailing) {
    CHECK(trim("  hello  ") == "hello");
    CHECK(trim("\t\nhello\r\n") == "hello");
}

TEST(ParserUtils_trim, only_whitespace) {
    CHECK(trim("   \t\n\r  ") == "");
}

TEST(ParserUtils_trim, preserves_internal) {
    CHECK(trim("  a b c  ") == "a b c");
}

TEST(ParserUtils_stripComments, empty) {
    CHECK(stripComments("") == "");
}

TEST(ParserUtils_stripComments, no_comments) {
    CHECK(stripComments("Toggle { color: red; }") == "Toggle { color: red; }");
}

TEST(ParserUtils_stripComments, line_comment_removed) {
    auto out = stripComments("a // comment\nb");
    CHECK(out.find("//") == std::string::npos);
    CHECK(out.find("a") != std::string::npos);
    CHECK(out.find("b") != std::string::npos);
}

TEST(ParserUtils_stripComments, block_comment_removed) {
    auto out = stripComments("a /* comment */ b");
    CHECK(out.find("/*") == std::string::npos);
    CHECK(out.find("comment") == std::string::npos);
    CHECK(out.find("a") != std::string::npos);
    CHECK(out.find("b") != std::string::npos);
}

TEST(ParserUtils_stripComments, comment_marker_inside_string_preserved) {
    auto out = stripComments(R"("a // b")");
    CHECK(out.find("//") != std::string::npos);
}

TEST(ParserUtils_splitBy, empty) {
    auto v = splitBy("", ',');
    CHECK(v.size() == 1);
    CHECK(v[0] == "");
}

TEST(ParserUtils_splitBy, single) {
    auto v = splitBy("a", ',');
    CHECK(v.size() == 1);
    CHECK(v[0] == "a");
}

TEST(ParserUtils_splitBy, two) {
    auto v = splitBy("a,b", ',');
    CHECK(v.size() == 2);
    CHECK(v[0] == "a");
    CHECK(v[1] == "b");
}

TEST(ParserUtils_splitBy, trailing_sep) {
    auto v = splitBy("a,b,", ',');
    CHECK(v.size() == 3);
    CHECK(v[2] == "");
}

TEST(ParserUtils_splitWs, empty) {
    auto v = splitWs("");
    CHECK(v.empty());
}

TEST(ParserUtils_splitWs, single_word) {
    auto v = splitWs("hello");
    CHECK(v.size() == 1);
    CHECK(v[0] == "hello");
}

TEST(ParserUtils_splitWs, multiple_spaces_collapsed) {
    auto v = splitWs("a    b   c");
    CHECK(v.size() == 3);
    CHECK(v[0] == "a");
    CHECK(v[1] == "b");
    CHECK(v[2] == "c");
}

TEST(ParserUtils_splitWs, leading_trailing) {
    auto v = splitWs("   a b   ");
    CHECK(v.size() == 2);
}

TEST(ParserUtils_parseDuration, seconds) {
    CHECK_NEAR(parseDurationSec("1s"), 1.0f, 1e-6);
    CHECK_NEAR(parseDurationSec("0.5s"), 0.5f, 1e-6);
}

TEST(ParserUtils_parseDuration, milliseconds) {
    CHECK_NEAR(parseDurationSec("500ms"), 0.5f, 1e-6);
    CHECK_NEAR(parseDurationSec("100ms"), 0.1f, 1e-6);
}

TEST(ParserUtils_parseDuration, bare_number) {
    CHECK_NEAR(parseDurationSec("2"), 2.0f, 1e-6);
}