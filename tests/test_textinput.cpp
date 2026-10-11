#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

// ============================================================
//  TextInput: cursor position
// ============================================================

// ---- getCursorPos / setCursorPos -------------------------------------

TEST(Widget_textinput, initial_cursor_at_end)
{
    Env env;
    auto ti = TextInput::create("hello");
    CHECK(ti->getCursorPos() == 5);
}

TEST(Widget_textinput, setCursorPos_stores_position)
{
    Env env;
    auto ti = TextInput::create("hello");
    ti->setCursorPos(2);
    CHECK(ti->getCursorPos() == 2);
}

TEST(Widget_textinput, setCursorPos_clamps_above_size)
{
    Env env;
    auto ti = TextInput::create("hello");
    ti->setCursorPos(100);
    CHECK(ti->getCursorPos() == 5);
}

TEST(Widget_textinput, setCursorPos_clamps_below_zero)
{
    Env env;
    auto ti = TextInput::create("hello");
    ti->setCursorPos(-3);
    CHECK(ti->getCursorPos() == 0);
}

TEST(Widget_textinput, setText_clamps_cursor)
{
    Env env;
    auto ti = TextInput::create("hello");
    ti->setCursorPos(5);        // fine del testo
    ti->setText("hi");          // testo più corto
    CHECK(ti->getCursorPos() == 2);
}

// ---- click-to-position -----------------------------------------------

TEST(Widget_textinput, click_at_left_edge_sets_cursor_to_zero)
{
    Env env;

    auto ti = TextInput::create("hello");
    ti->getInlineBase().padding = Spacing(Px(0));   // semplifica il calcolo
    ti->getInlineBase().letterSpacing = Px(0);
    ti->getInlineBase().minWidth = Px(200);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(ti);

    env.frame(root);
    env.frame(root);   // secondo frame: layout stabile

    // Click nell'angolo in alto a sinistra del widget.
    Rect r = ti->getRect();
    env.click(root, { r.x + 1.0f, r.center().y });

    CHECK(ti->getCursorPos() == 0);
}

TEST(Widget_textinput, click_at_right_edge_sets_cursor_to_end)
{
    Env env;

    auto ti = TextInput::create("hello");
    ti->getInlineBase().padding = Spacing(Px(0));
    ti->getInlineBase().letterSpacing = Px(0);
    ti->getInlineBase().minWidth = Px(200);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(ti);

    env.frame(root);
    env.frame(root);

    Rect r = ti->getRect();
    // Click oltre la fine del testo: il cursore va all'ultimo index.
    env.click(root, { r.x + r.width - 1.0f, r.center().y });

    CHECK(ti->getCursorPos() == 5);
}

TEST(Widget_textinput, click_at_letter_boundary)
{
    Env env;

    // MockRenderer: charWidth = 8.0, spacing default 0 (passiamo 0).
    // "ab" misura 16px, "abc" 24px. Un click a x=20 cade tra la
    // seconda e la terza lettera: il cursore va a 2 o 3, il più
    // vicino — 20 è equidistante (16 vs 24), scegliamo il primo
    // che risulta più vicino.
    auto ti = TextInput::create("abc");
    ti->getInlineBase().padding = Spacing(Px(0));
    ti->getInlineBase().letterSpacing = Px(0);
    ti->getInlineBase().minWidth = Px(200);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(ti);

    env.frame(root);
    env.frame(root);

    Rect r = ti->getRect();
    // click a x = r.x + 22 → più vicino a 24 (index 3) che a 16 (index 2)
    env.click(root, { r.x + 22.0f, r.center().y });
    CHECK(ti->getCursorPos() == 3);
}

// ---- integrazione con setText dopo setCursorPos ----------------------

TEST(Widget_textinput, cursor_moves_with_keyboard)
{
    Env env;

    auto ti = TextInput::create("hello");
    ti->getInlineBase().padding = Spacing(Px(0));
    ti->getInlineBase().minWidth = Px(200);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(ti);

    env.frame(root);
    env.frame(root);

    // Focus via click al centro (non importa dove — mettiamo il
    // cursore al punto giusto dopo).
    Rect r = ti->getRect();
    env.click(root, { r.x + r.width * 0.5f, r.center().y });

    // Forza il cursore all'inizio per un test deterministico.
    ti->setCursorPos(0);

    // Right arrow sposta di 1.
    env.platform.holdKey(Key::Right);
    env.frame(root);
    CHECK(ti->getCursorPos() == 1);

    env.platform.holdKey(Key::Left);
    env.frame(root);
    CHECK(ti->getCursorPos() == 0);
}

TEST(Widget_textinput, home_end_move_cursor)
{
    Env env;

    auto ti = TextInput::create("hello world");
    ti->getInlineBase().padding = Spacing(Px(0));
    ti->getInlineBase().minWidth = Px(200);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(ti);

    env.frame(root);
    env.frame(root);

    // Focus.
    Rect r = ti->getRect();
    env.click(root, { r.x + r.width * 0.5f, r.center().y });

    ti->setCursorPos(5);

    env.platform.pressKey(Key::End);
    env.frame(root);
    CHECK(ti->getCursorPos() == 11);

    env.platform.pressKey(Key::Home);
    env.frame(root);
    CHECK(ti->getCursorPos() == 0);
}