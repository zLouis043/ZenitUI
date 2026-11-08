#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "ZenitUIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

struct TextScene
{
    std::shared_ptr<Layout> root;
    std::shared_ptr<UI::Text> text;
};

static TextScene makeAutoWidthText(Env &env,
                                   const std::string &content,
                                   float maxParentW)
{
    TextScene s;

    s.text = UI::Text::create(content);
    s.text->setWrap(false);
    s.text->getInlineBase().letterSpacing = Px(0);
    s.text->getInlineBase().fontSize = Px(16);
    s.text->size(Auto(), Auto());

    auto parent = std::make_shared<Layout>(LayoutType::Stack);
    parent->size(Px(maxParentW), Px(500));

    s.root = std::make_shared<Layout>(LayoutType::Stack);
    s.root->size(Percent(100), Percent(100));
    parent->addChild(s.text);
    s.root->addChild(parent);

    env.frame(s.root);
    env.frame(s.root);

    return s;
}

static TextScene makeWrappedText(Env &env,
                                 const std::string &content,
                                 float parentW,
                                 bool wrap = true)
{
    TextScene s;

    s.text = UI::Text::create(content);
    s.text->setWrap(wrap);
    s.text->getInlineBase().letterSpacing = Px(0);
    s.text->getInlineBase().fontSize = Px(16);
    s.text->size(Percent(100), Auto());

    auto parent = std::make_shared<Layout>(LayoutType::Stack);
    parent->size(Px(parentW), Px(500));

    s.root = std::make_shared<Layout>(LayoutType::Stack);
    s.root->size(Percent(100), Percent(100));
    parent->addChild(s.text);
    s.root->addChild(parent);

    env.frame(s.root);
    env.frame(s.root);

    return s;
}

TEST(Text_wrap, single_line_no_wrap)
{
    Env env;
    auto s = makeWrappedText(env, "hello", 200.0f, /*wrap=*/false);

    Rect r = s.text->getRect();
    CHECK(r.width > 0.0f);
    CHECK(r.height > 0.0f);
    CHECK(r.height <= 40.0f);
}

TEST(Text_wrap, wrap_disabled_keeps_single_line)
{
    Env env;
    auto s = makeWrappedText(env, "this is a long text that should not wrap",
                             200.0f, /*wrap=*/false);

    Rect r = s.text->getRect();
    CHECK(r.height <= 40.0f);
}

TEST(Text_wrap, wrap_enabled_wraps_long_text)
{
    Env env1;
    auto narrow = makeWrappedText(env1, "hello world foo bar", 100.0f, true);

    Env env2;
    auto wide = makeWrappedText(env2, "hello world foo bar", 500.0f, true);

    CHECK(narrow.text->getRect().height > wide.text->getRect().height);
}

TEST(Text_wrap, narrow_parent_forces_more_lines)
{
    Env env1;
    auto narrow = makeWrappedText(env1, "hello world foo bar", 50.0f, true);

    Env env2;
    auto medium = makeWrappedText(env2, "hello world foo bar", 150.0f, true);

    CHECK(narrow.text->getRect().height > medium.text->getRect().height);
}

TEST(Text_wrap, rewraps_on_width_change)
{
    Env env;
    auto s = makeWrappedText(env, "hello world foo bar baz qux", 200.0f, true);

    float h1 = s.text->getRect().height;

    s.text->getParent()->size(Px(80), Px(500));

    for (int i = 0; i < 20; ++i)
        env.frame(s.root);

    float h2 = s.text->getRect().height;

    CHECK(h2 > h1);
}

TEST(Text_wrap, wider_parent_fits_fewer_lines)
{
    Env env;
    auto sNarrow = makeWrappedText(env, "hello world foo bar", 80.0f, true);
    Env env2;
    auto sWide = makeWrappedText(env2, "hello world foo bar", 200.0f, true);

    CHECK(sWide.text->getRect().height <
          sNarrow.text->getRect().height);
}

TEST(Text_wrap, no_wrap_on_unbounded_parent)
{
    Env env;
    auto text = UI::Text::create("this is a long string that would wrap");
    text->setWrap(true);
    text->getInlineBase().letterSpacing = Px(0);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(text);

    env.frame(root);
    env.frame(root);

    Rect r = text->getRect();
    CHECK(r.width > 100.0f);
    CHECK(r.height <= 40.0f);
}

TEST(Text_wrap, empty_text_has_minimal_height)
{
    Env env;
    auto s = makeWrappedText(env, "", 100.0f, true);

    Rect r = s.text->getRect();
    CHECK(r.height >= 0.0f);
}

TEST(Text_wrap, setText_rewraps)
{
    Env env;
    auto s = makeWrappedText(env, "short", 100.0f, true);

    float h1 = s.text->getRect().height;

    s.text->setText("this is a much longer string that will need wrapping");
    env.frame(s.root);
    env.frame(s.root);

    float h2 = s.text->getRect().height;
    CHECK(h2 > h1);
}

TEST(Text_wrap, preserves_explicit_newlines)
{
    Env env1;
    auto withNewlines = makeWrappedText(env1, "foo\nbar\nbaz", 400.0f, true);

    Env env2;
    auto withSpaces = makeWrappedText(env2, "foo bar baz", 400.0f, true);

    // Three logical lines vs one.
    // Without the '\n' fix, both render as a single line and the
    // heights are equal (within a pixel).
    CHECK(withNewlines.text->getRect().height >
          withSpaces.text->getRect().height * 1.5f);
}

TEST(Text_wrap, breaks_long_word_without_spaces)
{
    Env env1;
    auto longWord = makeWrappedText(env1, "supercalifragilistichespiralidoso", 80.0f, true);

    Env env2;
    auto shortWord = makeWrappedText(env2, "hello", 80.0f, true);

    // Without word-break the long word stays on one line (very tall, one
    // line height). With word-break it spans multiple lines.
    CHECK(longWord.text->getRect().height >
          shortWord.text->getRect().height * 2.0f);
}

TEST(Text_wrap, consecutive_newlines_preserve_empty_lines)
{
    Env env1;
    auto withEmpty = makeWrappedText(env1, "a\n\nb", 400.0f, true);

    Env env2;
    auto flat = makeWrappedText(env2, "ab", 400.0f, true);

    // "a\n\nb" = 3 line heights (the middle line is empty but still
    // occupies vertical space); "ab" = 1 line height.
    CHECK(withEmpty.text->getRect().height >
          flat.text->getRect().height * 2.0f);
}

TEST(Text_wrap, ellipsis_truncates_overflowing_single_line)
{
    Env env1;
    auto s = makeWrappedText(env1, "this is a very long text that will not fit",
                             80.0f, /*wrap=*/false);
    s.text->getInlineBase().textOverflow = TextOverflow::Ellipsis;

    env1.frame(s.root);
    env1.frame(s.root);

    // With ellipsis the Text must not overflow its parent.
    CHECK(s.text->getRect().width <= 80.0f + 0.5f);
    // And it must be a single line tall.
    CHECK(s.text->getRect().height <= 40.0f);
}

TEST(Text_wrap, ellipsis_does_not_change_layout_box)
{
    Env env1;
    auto a = makeAutoWidthText(env1, "short", 400.0f);
    a.text->getInlineBase().textOverflow = TextOverflow::Ellipsis;
    a.text->getInlineBase().transitionTime = 0;
    env1.frame(a.root);
    env1.frame(a.root);

    Env env2;
    auto b = makeAutoWidthText(env2, "short", 400.0f);
    b.text->getInlineBase().transitionTime = 0;
    env2.frame(b.root);
    env2.frame(b.root);

    // Ellipsis is a render-time transformation only; it must not change
    // the layout box. Both nodes should size to the natural text width,
    // which for "short" is well under 400.
    CHECK_NEAR(a.text->getRect().width, b.text->getRect().width, 1.0f);
    CHECK(a.text->getRect().width < 400.0f);
}

TEST(Text_wrap, white_space_nowrap_disables_wrapping)
{
    Env env1;
    auto s = makeWrappedText(env1, "hello world foo bar baz qux", 80.0f, true);
    s.text->getInlineBase().whiteSpace = WhiteSpace::Nowrap;
    s.text->getInlineBase().transitionTime = 0;

    for (int i = 0; i < 20; ++i)
        env1.frame(s.root);

    // Without nowrap the text would wrap to multiple lines and be tall.
    // With nowrap it stays a single line.
    CHECK(s.text->getRect().height <= 40.0f);
}

TEST(Text_wrap, newlines_work_without_wrap)
{
    Env env1;
    auto withNewlines = makeWrappedText(env1, "foo\nbar\nbaz", 400.0f, false);

    Env env2;
    auto singleLine = makeWrappedText(env2, "foobarbaz", 400.0f, false);

    // Three logical lines vs one, even without wrapping.
    CHECK(withNewlines.text->getRect().height >
          singleLine.text->getRect().height * 1.5f);
}

TEST(Text_wrap, newlines_work_with_nowrap)
{
    Env env1;
    auto withNewlines = makeWrappedText(env1, "foo\nbar\nbaz", 400.0f, true);
    withNewlines.text->getInlineBase().whiteSpace = WhiteSpace::Nowrap;
    withNewlines.text->getInlineBase().transitionTime = 0;

    Env env2;
    auto flat = makeWrappedText(env2, "foobarbaz", 400.0f, true);
    flat.text->getInlineBase().whiteSpace = WhiteSpace::Nowrap;
    flat.text->getInlineBase().transitionTime = 0;

    for (int i = 0; i < 20; ++i)
    {
        env1.frame(withNewlines.root);
        env2.frame(flat.root);
    }

    CHECK(withNewlines.text->getRect().height >
          flat.text->getRect().height * 1.5f);
}

TEST(Text_wrap, ellipsis_not_applied_to_multiline_text)
{
    Env env;
    auto s = makeWrappedText(env, "foo\nbar", 400.0f, false);
    s.text->getInlineBase().textOverflow = TextOverflow::Ellipsis;
    s.text->getInlineBase().transitionTime = 0;

    for (int i = 0; i < 5; ++i)
        env.frame(s.root);

    // Two logical lines: text-overflow is a single-line feature, so
    // both lines must be laid out without truncation. If ellipsis were
    // wrongly applied, the second line would be empty or the height
    // would collapse to one line.
    CHECK(s.text->getRect().height > 20.0f);
}