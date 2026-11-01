#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "ZenitUIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;


struct TextScene
{
    std::shared_ptr<Layout>       root;
    std::shared_ptr<UI::Text>     text;
};

static TextScene makeWrappedText(Env& env,
                                   const std::string& content,
                                   float parentW,
                                   bool  wrap = true)
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
    auto sNarrow = makeWrappedText(env, "hello world foo bar",  80.0f, true);
    Env env2;
    auto sWide   = makeWrappedText(env2, "hello world foo bar", 200.0f, true);

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