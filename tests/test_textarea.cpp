#include "TestFramework.hpp"
#include "Mocks.hpp"

#include <algorithm>

using namespace ZenitUI;
using namespace ZenitUI::UI;
using namespace ZenitUI::Test;

static void tapHeldKey(Env &env, const std::shared_ptr<Layout> &root, int key)
{
    env.platform.holdKey(key);
    env.frame(root);
    env.frame(root);
}

static bool hasDrawnText(const MockRenderer &r, std::string_view needle)
{
    return std::any_of(r.drawnTexts.begin(), r.drawnTexts.end(),
                       [needle](const std::string &s)
                       { return s == needle; });
}

TEST(TextArea, intrinsic_height_scales_with_rows)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Px(400), Px(400));

    auto ta = TextArea::create("", 3);
    root->addChild(ta);
    env.frame(root);
    float h3 = ta->getRect().height;
    CHECK(ta->getRows() == 3);
    CHECK(h3 > 40.0f);

    ta->setRows(6);
    env.frame(root);
    float h6 = ta->getRect().height;
    CHECK(ta->getRows() == 6);
    CHECK(h6 > h3);
}

TEST(TextArea, enter_inserts_newline_and_keeps_focus_escape_blurs)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(300));

    auto ta = TextArea::create("Uno", 4);
    ta->size(Px(300), Px(140));
    root->addChild(ta);

    int changeCount = 0;
    ta->onTextChanged = [&](const std::string &) { ++changeCount; };

    env.frame(root);
    UIContext::get().requestFocus(ta);

    env.platform.pressKey(Key::Enter);
    env.frame(root);

    CHECK(UIContext::get().hasFocus(ta.get()));
    CHECK(ta->getText() == "Uno\n");
    CHECK(changeCount == 1);

    env.platform.typeChar('D');
    env.platform.typeChar('u');
    env.platform.typeChar('e');
    env.frame(root);

    CHECK(ta->getText() == "Uno\nDue");
    CHECK(changeCount == 2);

    env.platform.pressKey(Key::Escape);
    env.frame(root);
    CHECK(!UIContext::get().hasFocus(ta.get()));
}

TEST(TextArea, home_end_and_vertical_navigation_across_hard_lines)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(300));

    auto ta = TextArea::create("abcd\nefghij\nkl", 4);
    ta->size(Px(300), Px(140));
    ta->getInlineBase().letterSpacing = Px(0.0f);
    root->addChild(ta);
    env.frame(root);

    UIContext::get().requestFocus(ta);
    // Cursore parte alla fine ("kl" -> pos 14)
    CHECK(ta->getCursorPos() == 14);

    // Home va all'inizio della riga corrente ("kl" inizia a indice 12)
    env.platform.pressKey(Key::Home);
    env.frame(root);
    CHECK(ta->getCursorPos() == 12);

    // End torna a fine riga corrente (14)
    env.platform.pressKey(Key::End);
    env.frame(root);
    CHECK(ta->getCursorPos() == 14);

    // Up sale alla riga "efghij" (5..11) alla stessa X (~2 caratteri -> pos 7)
    tapHeldKey(env, root, Key::Up);
    CHECK(ta->getCursorPos() == 7);

    // Up ancora sale alla riga "abcd" (0..4) alla stessa X (~2 caratteri -> pos 2)
    tapHeldKey(env, root, Key::Up);
    CHECK(ta->getCursorPos() == 2);

    // Down torna sulla seconda riga (~pos 7)
    tapHeldKey(env, root, Key::Down);
    CHECK(ta->getCursorPos() == 7);
}

TEST(TextArea, soft_wrap_splits_long_line_and_up_down_navigate_visual_lines)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(300));

    // Padding 12+12, border 1+1, margine scrollbar 8 => innerW = 114 - 34 = 80px
    // Con charWidth=8 e letterSpacing=0 stanno max 10 caratteri per riga.
    auto ta = TextArea::create("alpha beta gamma", 4);
    ta->size(Px(114), Px(140));
    ta->getInlineBase().letterSpacing = Px(0.0f);
    root->addChild(ta);
    env.frame(root);

    // Deve aver disegnato almeno 2 righe visive
    CHECK(env.renderer.drawnTexts.size() >= 2);

    UIContext::get().requestFocus(ta);
    int endPos = ta->getCursorPos();
    CHECK(endPos == (int)ta->getText().size());

    // Up da fine dell'ultima riga wrappata deve spostare il cursore su una riga precedente
    tapHeldKey(env, root, Key::Up);
    CHECK(ta->getCursorPos() < endPos);
}

TEST(TextArea, selection_clipboard_and_undo_redo)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(300));

    auto ta = TextArea::create("Uno\nDue", 4);
    ta->size(Px(300), Px(140));
    root->addChild(ta);
    env.frame(root);

    UIContext::get().requestFocus(ta);

    // Ctrl+A + Ctrl+C copia tutto
    env.platform.mods.ctrl = true;
    env.platform.pressKey(Key::A);
    env.frame(root);
    env.platform.pressKey(Key::C);
    env.frame(root);
    CHECK(env.platform.getClipboardText() == "Uno\nDue");

    // Ctrl+X taglia tutto
    env.platform.pressKey(Key::X);
    env.frame(root);
    CHECK(ta->getText().empty());

    // Ctrl+V incolla con normalizzazione CRLF -> LF
    env.platform.setClipboardText("Riga1\r\nRiga2\rRiga3");
    env.platform.pressKey(Key::V);
    env.frame(root);
    CHECK(ta->getText() == "Riga1\nRiga2\nRiga3");

    // Ctrl+Z annulla il paste -> torna vuoto
    env.platform.pressKey(Key::Z);
    env.frame(root);
    CHECK(ta->getText().empty());

    // Ctrl+Z ancora annulla il cut -> torna "Uno\nDue"
    env.platform.pressKey(Key::Z);
    env.frame(root);
    CHECK(ta->getText() == "Uno\nDue");

    // Ctrl+Y rifà il cut -> torna vuoto
    env.platform.pressKey(Key::Y);
    env.frame(root);
    CHECK(ta->getText().empty());

    // Ctrl+Shift+Z rifà il paste -> torna "Riga1\nRiga2\nRiga3"
    env.platform.mods.shift = true;
    env.platform.pressKey(Key::Z);
    env.frame(root);
    env.platform.mods.shift = false;
    env.platform.mods.ctrl = false;
    CHECK(ta->getText() == "Riga1\nRiga2\nRiga3");
}

TEST(TextArea, double_click_selects_word_and_triple_click_selects_all)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(300));

    auto ta = TextArea::create("citt\xC3\xA0 antica\nseconda riga", 4);
    ta->size(Px(300), Px(140));
    ta->getInlineBase().letterSpacing = Px(0.0f);
    root->addChild(ta);
    env.frame(root);

    // Clic sulla prima parola ("città"), tenendo conto di padding left=12+1 e top=8+1
    Vec2 clickPos = {ta->getRect().x + 13.0f + 16.0f, ta->getRect().y + 9.0f + 10.0f};

    env.platform.currentTime = 0.0;
    env.click(root, clickPos);

    // Doppio clic entro 0.4s seleziona "città" (inclusa la 'à' multi-byte)
    env.platform.currentTime = 0.1;
    env.click(root, clickPos);

    env.platform.mods.ctrl = true;
    env.platform.pressKey(Key::C);
    env.frame(root);
    env.platform.mods.ctrl = false;
    CHECK(env.platform.getClipboardText() == "citt\xC3\xA0");

    // Triplo clic seleziona l'intero contenuto
    env.platform.currentTime = 0.2;
    env.click(root, clickPos);

    env.platform.mods.ctrl = true;
    env.platform.pressKey(Key::C);
    env.frame(root);
    env.platform.mods.ctrl = false;
    CHECK(env.platform.getClipboardText() == "citt\xC3\xA0 antica\nseconda riga");
}

TEST(TextArea, mouse_wheel_scrolls_lines_and_yields_at_bounds)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(300));

    std::string lines;
    for (int i = 0; i < 12; ++i)
    {
        if (i > 0)
            lines += '\n';
        lines += "Line" + std::to_string(i);
    }

    // Altezza piccola (~2-3 righe visibili su 12)
    auto ta = TextArea::create(lines, 2);
    ta->size(Px(220), Px(80));
    root->addChild(ta);
    env.frame(root);

    // All'inizio si vede Line0 ma non Line11
    CHECK(hasDrawnText(env.renderer, "Line0"));
    CHECK(!hasDrawnText(env.renderer, "Line11"));

    Vec2 center = {ta->getRect().x + 50.0f, ta->getRect().y + 40.0f};
    env.platform.moveMouse(center);

    // Scroll verso l'alto al limite superiore (scrollY == 0): NON deve consumare la rotella
    env.platform.scrollWheel(+1.0f);
    env.frame(root);
    CHECK(!UIContext::get().wheelConsumedThisFrame);

    // Scroll verso il basso: deve consumare la rotella
    env.renderer.drawnTexts.clear();
    env.platform.scrollWheel(-1.0f);
    env.frame(root);
    CHECK(UIContext::get().wheelConsumedThisFrame);

    // Continua a scrollare fino in fondo
    for (int i = 0; i < 10; ++i)
    {
        env.renderer.drawnTexts.clear();
        env.platform.scrollWheel(-2.0f);
        env.frame(root);
    }

    CHECK(!hasDrawnText(env.renderer, "Line0"));
    CHECK(hasDrawnText(env.renderer, "Line11"));

    // Al limite inferiore un ulteriore scroll verso il basso cede al parent
    env.platform.scrollWheel(-1.0f);
    env.frame(root);
    CHECK(!UIContext::get().wheelConsumedThisFrame);
}

TEST(TextArea, placeholder_wraps_by_default_and_hides_when_text_present)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(300));

    auto ta = TextArea::create("", 4);
    ta->size(Px(114), Px(140));
    ta->getInlineBase().letterSpacing = Px(0.0f);
    ta->setPlaceholder("alpha beta gamma delta");
    root->addChild(ta);

    env.frame(root);
    CHECK(ta->getPlaceholderWrap() == true);
    CHECK(env.renderer.drawnTexts.size() >= 2);

    // Disabilitando placeholderWrap viene disegnata una singola riga
    env.renderer.drawnTexts.clear();
    ta->setPlaceholderWrap(false);
    env.frame(root);
    CHECK(env.renderer.drawnTexts.size() == 1);
    CHECK(env.renderer.drawnTexts[0] == "alpha beta gamma delta");

    // Impostando del testo il placeholder sparisce
    env.renderer.drawnTexts.clear();
    ta->setText("Testo");
    env.frame(root);
    CHECK(hasDrawnText(env.renderer, "Testo"));
    CHECK(!hasDrawnText(env.renderer, "alpha beta gamma delta"));
}

TEST(TextArea, zmarkup_builds_textarea_with_rows_and_placeholder)
{
    Env env;
    auto ui = ZMarkup::build(R"(
        VStack {
            TextArea@notes rows=5 placeholder="Scrivi qui..." placeholder-wrap="false" "Valore iniziale"
        }
    )");
    auto ta = ui.ref<TextArea>("notes");
    CHECK(ta != nullptr);
    CHECK(ta->getRows() == 5);
    CHECK(ta->getPlaceholder() == "Scrivi qui...");
    CHECK(ta->getPlaceholderWrap() == false);
    CHECK(ta->getText() == "Valore iniziale");
}