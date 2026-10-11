#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  Text wrapping
// ============================================================

// Helper: crea un Text con wrap dentro un parent di larghezza fissa.
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
    // Il mock misura 8px per char + spacing. Con spacing 0 il calcolo
    // è prevedibile: una parola da N char misura N*8 px.
    s.text->getInlineBase().letterSpacing = Px(0);
    s.text->getInlineBase().fontSize = Px(16);   // non influenza la larghezza nel mock
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

// ---- misura intrinseca ----------------------------------------------

TEST(Text_wrap, single_line_no_wrap)
{
    Env env;
    // "hello" misura 5*8 = 40px. Parent 200px → sta in una riga.
    auto s = makeWrappedText(env, "hello", 200.0f, /*wrap=*/false);

    Rect r = s.text->getRect();
    CHECK(r.width > 0.0f);
    CHECK(r.height > 0.0f);
    // Con una sola riga, l'altezza è quella del font (dal mock).
    CHECK(r.height <= 40.0f);   // tolleranza ampia per il mock
}

TEST(Text_wrap, wrap_disabled_keeps_single_line)
{
    Env env;
    // Testo lungo, wrap disabilitato: una sola riga, anche se overflow.
    auto s = makeWrappedText(env, "this is a long text that should not wrap",
                              200.0f, /*wrap=*/false);

    Rect r = s.text->getRect();
    // Non ci sono controlli diretti sul numero di righe, ma con wrap
    // disabilitato l'altezza deve essere piccola (una riga).
    CHECK(r.height <= 40.0f);
}

TEST(Text_wrap, wrap_enabled_wraps_long_text)
{
    Env env1;
    auto narrow = makeWrappedText(env1, "hello world foo bar", 100.0f, true);

    Env env2;
    auto wide = makeWrappedText(env2, "hello world foo bar", 500.0f, true);

    // Con parent largo, il testo sta in una riga. Con parent stretto,
    // va in più righe → altezza maggiore.
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

    // Il cambio di size triggera una transizione di width sul parent.
    // Aspetta che si completi (transitionTime default = 0.15s).
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

    // Un parent più largo produce meno righe → altezza minore.
    CHECK(sWide.text->getRect().height <
          sNarrow.text->getRect().height);
}

TEST(Text_wrap, no_wrap_on_unbounded_parent)
{
    Env env;
    // Parent senza vincolo di larghezza: il testo non deve mai
    // mandare a capo.
    auto text = UI::Text::create("this is a long string that would wrap");
    text->setWrap(true);
    text->getInlineBase().letterSpacing = Px(0);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(text);

    env.frame(root);
    env.frame(root);

    // Il testo ha una larghezza intrinseca e una sola riga.
    Rect r = text->getRect();
    CHECK(r.width > 100.0f);
    CHECK(r.height <= 40.0f);
}

TEST(Text_wrap, empty_text_has_minimal_height)
{
    Env env;
    auto s = makeWrappedText(env, "", 100.0f, true);

    Rect r = s.text->getRect();
    // Non deve crashare e l'altezza è ragionevole (0 o piccola).
    CHECK(r.height >= 0.0f);
}

TEST(Text_wrap, setText_rewraps)
{
    Env env;
    auto s = makeWrappedText(env, "short", 100.0f, true);

    float h1 = s.text->getRect().height;

    // Sostituisci con testo lungo: deve re-wrappare.
    s.text->setText("this is a much longer string that will need wrapping");
    env.frame(s.root);
    env.frame(s.root);

    float h2 = s.text->getRect().height;
    CHECK(h2 > h1);
}