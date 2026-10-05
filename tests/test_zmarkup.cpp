#include "TestFramework.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI::ZMarkup;

TEST(ZMarkup_parser, empty_input) {
    auto e = parse("");
    CHECK(e.tag.empty());
    CHECK(e.classes.empty());
    CHECK(e.id.empty());
    CHECK(e.attrs.empty());
    CHECK(e.children.empty());
}

TEST(ZMarkup_parser, bare_tag) {
    auto e = parse("Toggle");
    CHECK(e.tag == "Toggle");
    CHECK(e.classes.empty());
    CHECK(e.id.empty());
}

TEST(ZMarkup_parser, tag_with_class) {
    auto e = parse("Toggle.my-class");
    CHECK(e.tag == "Toggle");
    CHECK(e.classes.size() == 1);
    CHECK(e.classes[0] == "my-class");
}

TEST(ZMarkup_parser, tag_with_multiple_classes) {
    auto e = parse("Button.btn.btn-primary");
    CHECK(e.tag == "Button");
    CHECK(e.classes.size() == 2);
    CHECK(e.classes[0] == "btn");
    CHECK(e.classes[1] == "btn-primary");
}

TEST(ZMarkup_parser, tag_with_id) {
    auto e = parse("Text#my-text");
    CHECK(e.tag == "Text");
    CHECK(e.id == "my-text");
}

TEST(ZMarkup_parser, class_and_id_together) {
    auto e = parse("Text.big#title");
    CHECK(e.tag == "Text");
    CHECK(e.classes.size() == 1);
    CHECK(e.id == "title");
}

TEST(ZMarkup_parser, quoted_text) {
    auto e = parse(R"(Text "hello world")");
    CHECK(e.tag == "Text");
    CHECK(e.text == "hello world");
}

TEST(ZMarkup_parser, escape_in_text) {
    auto e = parse(R"(Text "line1\nline2")");
    CHECK(e.text == "line1\nline2");
}

TEST(ZMarkup_parser, attr_string) {
    auto e = parse(R"(TextInput value="Player1")");
    CHECK(e.has("value"));
    CHECK(e.attr("value") == "Player1");
}

TEST(ZMarkup_parser, attr_bare_token) {
    auto e = parse("Slider value=0.65");
    CHECK(e.has("value"));
    CHECK(e.attr("value") == "0.65");
}

TEST(ZMarkup_parser, attr_float) {
    auto e = parse("Slider value=0.65");
    CHECK_NEAR(e.attrFloat("value", 0.0f), 0.65f, 1e-5);
}

TEST(ZMarkup_parser, attr_bool_flag) {
    auto e = parse("Toggle checked");
    CHECK(e.has("checked"));
    CHECK(e.attrBool("checked"));
}

TEST(ZMarkup_parser, attr_bool_explicit) {
    auto e = parse("Toggle checked=false");
    CHECK(e.has("checked"));
    CHECK(!e.attrBool("checked"));
}

TEST(ZMarkup_parser, children_block) {
    auto e = parse(R"(
        VStack {
            Text "a"
            Text "b"
        }
    )");
    CHECK(e.tag == "VStack");
    CHECK(e.children.size() == 2);
    CHECK(e.children[0].tag == "Text");
    CHECK(e.children[0].text == "a");
    CHECK(e.children[1].text == "b");
}

TEST(ZMarkup_parser, nested_children) {
    auto e = parse(R"(
        VStack {
            HStack {
                Text "x"
                Text "y"
            }
            Text "z"
        }
    )");
    CHECK(e.children.size() == 2);
    CHECK(e.children[0].tag == "HStack");
    CHECK(e.children[0].children.size() == 2);
    CHECK(e.children[1].text == "z");
}

TEST(ZMarkup_parser, line_comment_ignored) {
    auto e = parse(R"(
        // this is a comment
        Text "hello"
    )");
    CHECK(e.tag == "Text");
    CHECK(e.text == "hello");
}