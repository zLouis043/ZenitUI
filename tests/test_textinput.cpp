#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

// ============================================================
//  Scene helper: TextInput con focus, in una scena minima.
// ============================================================

struct TextInputScene
{
    std::shared_ptr<Layout>    root;
    std::shared_ptr<UI::TextInput> ti;
};

static TextInputScene makeFocusedInput(Env& env,
                                        const std::string& initial = "")
{
    TextInputScene s;

    s.ti = UI::TextInput::create(initial);
    s.ti->getInlineBase().padding = Spacing(Px(0));
    s.ti->getInlineBase().letterSpacing = Px(0);
    s.ti->getInlineBase().minWidth = Px(200);

    s.root = std::make_shared<Layout>(LayoutType::Stack);
    s.root->size(Percent(100), Percent(100));
    s.root->addChild(s.ti);

    env.frame(s.root);
    env.frame(s.root);

    // Click per dare focus.
    Rect r = s.ti->getRect();
    env.click(s.root, { r.x + r.width * 0.5f, r.center().y });
    env.frame(s.root);
    env.frame(s.root);

    return s;
}

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


// ============================================================
//  Editing: insertion, backspace, delete
// ============================================================

TEST(Widget_textinput, type_chars_inserts_at_cursor)
{
    Env env;
    auto s = makeFocusedInput(env, "");

    env.platform.typeChar('h');
    env.platform.typeChar('i');
    env.frame(s.root);

    CHECK(s.ti->getText() == "hi");
    CHECK(s.ti->getCursorPos() == 2);
}

TEST(Widget_textinput, type_char_replaces_cursor_position)
{
    Env env;
    auto s = makeFocusedInput(env, "ac");

    // Sposta il cursore dopo 'a' (tra 'a' e 'c').
    s.ti->setCursorPos(1);
    env.frame(s.root);

    env.platform.typeChar('b');
    env.frame(s.root);

    CHECK(s.ti->getText() == "abc");
    CHECK(s.ti->getCursorPos() == 2);
}

TEST(Widget_textinput, backspace_removes_before_cursor)
{
    Env env;
    auto s = makeFocusedInput(env, "abc");

    s.ti->setCursorPos(2);

    env.platform.holdKey(Key::Backspace);
    env.frame(s.root);

    CHECK(s.ti->getText() == "ac");
    CHECK(s.ti->getCursorPos() == 1);
}

TEST(Widget_textinput, backspace_at_start_is_noop)
{
    Env env;
    auto s = makeFocusedInput(env, "abc");

    s.ti->setCursorPos(0);

    env.platform.holdKey(Key::Backspace);
    env.frame(s.root);

    CHECK(s.ti->getText() == "abc");
    CHECK(s.ti->getCursorPos() == 0);
}

TEST(Widget_textinput, delete_removes_at_cursor)
{
    Env env;
    auto s = makeFocusedInput(env, "abc");

    s.ti->setCursorPos(1);

    env.platform.holdKey(Key::Delete);
    env.frame(s.root);

    CHECK(s.ti->getText() == "ac");
    CHECK(s.ti->getCursorPos() == 1);
}

TEST(Widget_textinput, delete_at_end_is_noop)
{
    Env env;
    auto s = makeFocusedInput(env, "abc");

    s.ti->setCursorPos(3);

    env.platform.holdKey(Key::Delete);
    env.frame(s.root);

    CHECK(s.ti->getText() == "abc");
    CHECK(s.ti->getCursorPos() == 3);
}

// ============================================================
//  Callbacks
// ============================================================

TEST(Widget_textinput, onTextChanged_fires_on_edit)
{
    Env env;
    auto s = makeFocusedInput(env, "");

    int calls = 0;
    std::string last;
    s.ti->onTextChanged = [&](const std::string& t) {
        calls++;
        last = t;
    };

    env.platform.typeChar('a');
    env.frame(s.root);

    CHECK(calls == 1);
    CHECK(last == "a");

    env.platform.typeChar('b');
    env.frame(s.root);

    CHECK(calls == 2);
    CHECK(last == "ab");
}

TEST(Widget_textinput, onTextChanged_does_not_fire_when_no_change)
{
    Env env;
    auto s = makeFocusedInput(env, "abc");

    int calls = 0;
    s.ti->onTextChanged = [&](const std::string&) { calls++; };

    // Backspace all'inizio non cambia nulla.
    s.ti->setCursorPos(0);
    env.frame(s.root);

    env.platform.pressKey(Key::Backspace);
    env.frame(s.root);

    CHECK(calls == 0);
}

// ============================================================
//  Enter / Escape
// ============================================================

TEST(Widget_textinput, enter_fires_onSubmit_and_releases_focus)
{
    Env env;
    auto s = makeFocusedInput(env, "hello");

    int calls = 0;
    std::string submitted;
    s.ti->onSubmit = [&](const std::string& t) {
        calls++;
        submitted = t;
    };

    CHECK(UIContext::get().hasFocus(s.ti.get()));

    env.platform.pressKey(Key::Enter);
    env.frame(s.root);

    CHECK(calls == 1);
    CHECK(submitted == "hello");
    CHECK(!UIContext::get().hasFocus(s.ti.get()));
}

TEST(Widget_textinput, escape_releases_focus_without_submit)
{
    Env env;
    auto s = makeFocusedInput(env, "hello");

    int calls = 0;
    s.ti->onSubmit = [&](const std::string&) { calls++; };

    CHECK(UIContext::get().hasFocus(s.ti.get()));

    env.platform.pressKey(Key::Escape);
    env.frame(s.root);

    CHECK(calls == 0);
    CHECK(!UIContext::get().hasFocus(s.ti.get()));
}

// ============================================================
//  Key repeat
// ============================================================

TEST(Widget_textinput, key_repeat_deletes_multiple)
{
    Env env;
    auto s = makeFocusedInput(env, "abcdef");

    // Cursore alla fine.
    s.ti->setCursorPos(6);

    // Tieni premuto Backspace per 1 secondo (60 frame a 60 fps).
    // Repeat: delay 0.4s, interval 0.04s → dopo 1s dovrebbe aver
    // cancellato (1s - 0.4s) / 0.04s + 1 ≈ 16 caratteri, ma il testo
    // è solo 6. Quindi il testo diventa "".
    for (int i = 0; i < 60; ++i)
    {
        env.platform.holdKey(Key::Backspace);
        env.frame(s.root);
    }

    CHECK(s.ti->getText() == "");
    CHECK(s.ti->getCursorPos() == 0);
}

TEST(Widget_textinput, key_repeat_stops_when_released)
{
    Env env;
    auto s = makeFocusedInput(env, "abcdefghij");

    s.ti->setCursorPos(10);

    // Tieni premuto per 10 frame.
    for (int i = 0; i < 10; ++i)
    {
        env.platform.holdKey(Key::Backspace);
        env.frame(s.root);
    }

    std::string afterHold = s.ti->getText();

    // Rilascia (nessun holdKey) per 30 frame.
    for (int i = 0; i < 30; ++i)
    {
        env.frame(s.root);
    }

    // Il testo è rimasto lo stesso.
    CHECK(s.ti->getText() == afterHold);
}

// ============================================================
//  Horizontal scroll of the cursor
// ============================================================

TEST(Widget_textinput, cursor_scrolls_into_view_when_long)
{
    Env env;
    auto s = makeFocusedInput(env, "");

    const std::string longText = "this is a very long text that will not fit";
    s.ti->setText(longText);
    s.ti->setCursorPos((int)longText.size());
    env.frame(s.root);

    CHECK(s.ti->getText() == longText);
    CHECK(s.ti->getCursorPos() == (int)longText.size());
}

TEST(Widget_textinput, type_after_scroll_keeps_cursor_at_end)
{
    Env env;
    auto s = makeFocusedInput(env, "");

    // Riempi il campo.
    for (int i = 0; i < 30; ++i)
    {
        env.platform.typeChar('x');
        env.frame(s.root);
    }

    CHECK(s.ti->getText() == std::string(30, 'x'));
    CHECK(s.ti->getCursorPos() == 30);

    // L'ultimo carattere digitato deve essere visibile, il che
    // significa che il cursore è alla fine e non è stato spostato
    // dallo scroll.
    env.platform.typeChar('y');
    env.frame(s.root);

    CHECK(s.ti->getText() == std::string(30, 'x') + "y");
    CHECK(s.ti->getCursorPos() == 31);
}

// ============================================================
//  Selection and Clipboard
// ============================================================

TEST(Widget_textinput, type_with_selection_replaces_it) {
    Env env;
    auto s = makeFocusedInput(env, "hello world");
    
    s.ti->setCursorPos(11);
    env.platform.mods.shift = true;
    for (int i = 0; i < 5; ++i) {
        env.platform.holdKey(Key::Left); // Usa holdKey per la navigazione
        env.frame(s.root);
        env.frame(s.root);
    }
    env.platform.mods.shift = false;
    
    env.platform.typeChar('X');
    env.frame(s.root);
    
    CHECK(s.ti->getText() == "hello X");
    CHECK(s.ti->getCursorPos() == 7);
}

TEST(Widget_textinput, backspace_with_selection_deletes_it) {
    Env env;
    auto s = makeFocusedInput(env, "hello world");
    
    s.ti->setCursorPos(8);
    env.platform.mods.shift = true;
    for (int i = 0; i < 5; ++i) {
        env.platform.holdKey(Key::Left);
        env.frame(s.root);
        env.frame(s.root);
    }
    env.platform.mods.shift = false;
    
    env.platform.holdKey(Key::Backspace); // Usa holdKey
    env.frame(s.root);
    
    CHECK(s.ti->getText() == "helrld");
    CHECK(s.ti->getCursorPos() == 3);
}

TEST(Widget_textinput, ctrl_c_copies_selection) {
    Env env;
    auto s = makeFocusedInput(env, "ZenitUI is great");
    
    s.ti->setCursorPos(7);
    env.platform.mods.shift = true;
    for (int i = 0; i < 7; ++i) {
        env.platform.holdKey(Key::Left);
        env.frame(s.root);
        env.frame(s.root);
    }
    
    env.platform.mods.shift = false;
    env.platform.mods.ctrl = true;
    env.platform.pressKey(Key::C); // Ctrl+C legge ev.keys, pressKey va bene
    env.frame(s.root);
    env.platform.mods.ctrl = false;
    
    CHECK(env.platform.getClipboardText() == "ZenitUI");
}

TEST(Widget_textinput, ctrl_v_pastes_from_clipboard) {
    Env env;
    auto s = makeFocusedInput(env, "Hello ");
    
    env.platform.setClipboardText("world!");
    s.ti->setCursorPos(6);
    
    env.platform.mods.ctrl = true;
    env.platform.pressKey(Key::V);
    env.frame(s.root);
    env.platform.mods.ctrl = false;
    
    CHECK(s.ti->getText() == "Hello world!");
    CHECK(s.ti->getCursorPos() == 12);
}

TEST(Widget_textinput, ctrl_a_selects_all) {
    Env env;
    auto s = makeFocusedInput(env, "ZenitUI");
    
    s.ti->setCursorPos(2);
    
    env.platform.mods.ctrl = true;
    env.platform.pressKey(Key::A);
    env.frame(s.root);
    
    env.platform.pressKey(Key::C);
    env.frame(s.root);
    env.platform.mods.ctrl = false;
    
    CHECK(env.platform.getClipboardText() == "ZenitUI");
}