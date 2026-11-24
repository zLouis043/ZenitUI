#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "Utf8.hpp"

using namespace ZenitUI;
using namespace ZenitUI::UI;
using namespace ZenitUI::Test;

static bool isValidUtf8(std::string_view s)
{
    size_t pos = 0;
    while (pos < s.size())
    {
        uint32_t cp = 0;
        size_t next = Utf8::decode(s, pos, cp);
        if (cp == 0xFFFDu && s.substr(pos, next - pos) != "\xEF\xBF\xBD")
            return false;
        pos = next;
    }
    return true;
}

TEST(Utf8, encode_decode_roundtrip_1_to_4_bytes)
{
    const uint32_t samples[] = {
        0x41u,     // 'A' (1 byte)
        0x00E0u,   // 'à' (2 bytes)
        0x20ACu,   // '€' (3 bytes)
        0x1F600u,  // '😀' (4 bytes)
    };
    const size_t expectedLens[] = {1, 2, 3, 4};

    for (size_t i = 0; i < 4; ++i)
    {
        std::string enc = Utf8::encode(samples[i]);
        CHECK(enc.size() == expectedLens[i]);
        uint32_t cp = 0;
        size_t next = Utf8::decode(enc, 0, cp);
        CHECK(cp == samples[i]);
        CHECK(next == enc.size());
    }

    CHECK(Utf8::encode(0xD800u).empty());
    CHECK(Utf8::encode(0x110000u).empty());
}

TEST(Utf8, grapheme_clusters_combining_zwj_and_flags)
{
    std::string decomposed = "e\xCC\x81x";
    CHECK(Utf8::nextCluster(decomposed, 0) == 3);
    CHECK(Utf8::nextCluster(decomposed, 3) == 4);
    CHECK(Utf8::prevCluster(decomposed, 4) == 3);
    CHECK(Utf8::prevCluster(decomposed, 3) == 0);

    std::string zwj = Utf8::encode(0x1F468u) + Utf8::encode(0x200Du) + Utf8::encode(0x1F469u);
    CHECK(Utf8::nextCluster(zwj, 0) == zwj.size());
    CHECK(Utf8::prevCluster(zwj, zwj.size()) == 0);

    std::string flag = Utf8::encode(0x1F1EEu) + Utf8::encode(0x1F1F9u);
    CHECK(flag.size() == 8);
    CHECK(Utf8::nextCluster(flag, 0) == 8);
    CHECK(Utf8::prevCluster(flag, 8) == 0);

    CHECK(Utf8::nextCluster("\r\na", 0) == 2);
    CHECK(Utf8::prevCluster("\r\na", 2) == 0);
}

TEST(Utf8, textinput_typing_cursor_backspace_delete_multibyte)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(200));

    auto ti = TextInput::create("");
    ti->size(Px(300), Px(40));
    root->addChild(ti);
    env.frame(root);

    UIContext::get().requestFocus(ti);

    env.platform.typeChar('c');
    env.platform.typeChar('i');
    env.platform.typeChar(0x00E0);
    env.platform.typeChar(0x20AC);
    env.frame(root);

    CHECK(ti->getText() == "ci\xC3\xA0\xE2\x82\xAC");
    CHECK(ti->getCursorPos() == 7);

    env.platform.holdKey(Key::Left);
    env.frame(root);
    CHECK(ti->getCursorPos() == 4);

    env.platform.holdKey(Key::Backspace);
    env.frame(root);
    CHECK(ti->getText() == "ci\xE2\x82\xAC");
    CHECK(ti->getCursorPos() == 2);

    env.platform.holdKey(Key::Delete);
    env.frame(root);
    CHECK(ti->getText() == "ci");
    CHECK(ti->getCursorPos() == 2);
}

TEST(Utf8, textinput_combining_cluster_single_backspace_and_paste)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(200));

    auto ti = TextInput::create("ae\xCC\x81");
    ti->size(Px(300), Px(40));
    root->addChild(ti);
    env.frame(root);

    UIContext::get().requestFocus(ti);
    CHECK(ti->getCursorPos() == 4);

    env.platform.holdKey(Key::Backspace);
    env.frame(root);
    CHECK(ti->getText() == "a");
    CHECK(ti->getCursorPos() == 1);

    env.platform.setClipboardText("b\xC3\xA8\nc\xE2\x82\xAC");
    env.platform.mods.ctrl = true;
    env.platform.pressKey(Key::V);
    env.frame(root);
    env.platform.mods.ctrl = false;

    CHECK(ti->getText() == "ab\xC3\xA8" "c\xE2\x82\xAC");
    CHECK(isValidUtf8(ti->getText()));
}

TEST(Utf8, textinput_setCursorPos_snaps_mid_sequence_to_boundary)
{
    Env env;
    auto ti = TextInput::create("\xC3\xA0\xE2\x82\xAC");
    ti->setCursorPos(1);
    CHECK(ti->getCursorPos() == 0);
    ti->setCursorPos(4);
    CHECK(ti->getCursorPos() == 2);
}

TEST(Utf8, textarea_utf8_paste_wrap_and_navigation)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(300));

    auto ta = TextArea::create("", 4);
    ta->size(Px(90), Px(120));
    root->addChild(ta);
    env.frame(root);

    UIContext::get().requestFocus(ta);

    env.platform.setClipboardText("Citt\xC3\xA0\r\nPerch\xC3\xA9 \xE2\x82\xAC");
    env.platform.mods.ctrl = true;
    env.platform.pressKey(Key::V);
    env.frame(root);
    env.platform.mods.ctrl = false;

    CHECK(ta->getText() == "Citt\xC3\xA0\nPerch\xC3\xA9 \xE2\x82\xAC");
    CHECK(isValidUtf8(ta->getText()));

    env.platform.holdKey(Key::Backspace);
    env.frame(root);
    CHECK(ta->getText() == "Citt\xC3\xA0\nPerch\xC3\xA9 ");
}

TEST(Utf8, text_wrap_and_ellipsis_never_split_multibyte_chars)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(400), Px(200));

    std::string accentedWord;
    for (int i = 0; i < 12; ++i)
        accentedWord += "\xC3\xA0";

    auto wrapped = Text::create(accentedWord);
    wrapped->setWrap(true);
    wrapped->size(Px(50), Auto());
    root->addChild(wrapped);

    auto ell = Text::create("Citt\xC3\xA0 meravigliosa con \xE2\x82\xAC");
    ell->size(Px(60), Px(30));
    ell->getInlineBase().whiteSpace = WhiteSpace::Nowrap;
    ell->getInlineBase().textOverflow = TextOverflow::Ellipsis;
    root->addChild(ell);

    env.frame(root);

    CHECK(!env.renderer.drawnTexts.empty());
    for (const auto &drawn : env.renderer.drawnTexts)
    {
        CHECK(isValidUtf8(drawn));
    }
}

TEST(Utf8, renderer_measureCodepoint_default_works)
{
    Env env;
    Vec2 mAscii = env.renderer.measureCodepoint({}, 'A', 20.0f, 0.0f);
    Vec2 mEuro = env.renderer.measureCodepoint({}, 0x20ACu, 20.0f, 0.0f);
    CHECK_NEAR(mAscii.x, 8.0f, 0.01f);
    CHECK_NEAR(mEuro.x, 8.0f, 0.01f);
}

TEST(Utf8, codepoint_set_presets_dedup_and_utf8_extraction)
{
    auto def = CodepointSet::defaultLatin().build();
    CHECK(!def.empty());
    for (size_t i = 1; i < def.size(); ++i)
        CHECK(def[i - 1] < def[i]);

    CHECK(std::binary_search(def.begin(), def.end(), 'A'));
    CHECK(std::binary_search(def.begin(), def.end(), 0x00E0));
    CHECK(std::binary_search(def.begin(), def.end(), 0x20AC));

    auto custom = CodepointSet::ascii()
                      .merge(CodepointSet::symbols())
                      .addFromUtf8("A\n\xE2\x98\x85\xE2\x98\x85\xC3\xA0") // A, \n, ★, ★, à
                      .build();
    for (size_t i = 1; i < custom.size(); ++i)
        CHECK(custom[i - 1] < custom[i]);
    CHECK(!std::binary_search(custom.begin(), custom.end(), '\n'));
    CHECK(std::binary_search(custom.begin(), custom.end(), 0x2605)); // ★
    CHECK(std::binary_search(custom.begin(), custom.end(), 0x00E0)); // à
}