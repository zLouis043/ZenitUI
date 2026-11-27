#include "TestFramework.hpp"
#include "TextEngine.hpp"
#include "StyleAttr.hpp"
#include "Style.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

using namespace ZenitUI;

namespace
{
    std::string findTestFontPath()
    {
        for (const char *candidate : {
                 "assets/fonts/Inter_18pt-Regular.ttf",
                 "../assets/fonts/Inter_18pt-Regular.ttf",
                 "../../assets/fonts/Inter_18pt-Regular.ttf"})
        {
            if (std::filesystem::exists(candidate))
                return candidate;
        }
        return "assets/fonts/Inter_18pt-Regular.ttf";
    }
} // namespace

TEST(FontStack, parse_single)
{
    auto s = ZenitUI::detail::parseFontStack("calibri");
    CHECK(s.size() == 1);
    CHECK(s[0] == "calibri");
}

TEST(FontStack, parse_multi)
{
    auto s = ZenitUI::detail::parseFontStack("mont, calibri, sans");
    CHECK(s.size() == 3);
    CHECK(s[0] == "mont");
    CHECK(s[1] == "calibri");
    CHECK(s[2] == "sans");
}

TEST(FontStack, parse_quoted_with_spaces)
{
    auto s = ZenitUI::detail::parseFontStack("\"Comic Sans\", mont");
    CHECK(s.size() == 2);
    CHECK(s[0] == "Comic Sans");
    CHECK(s[1] == "mont");
}

TEST(FontStack, parse_extra_whitespace)
{
    auto s = ZenitUI::detail::parseFontStack("  mont ,  calibri  ");
    CHECK(s.size() == 2);
    CHECK(s[0] == "mont");
    CHECK(s[1] == "calibri");
}

TEST(FontStack, parse_empty)
{
    auto s = ZenitUI::detail::parseFontStack("");
    CHECK(s.empty());
}

TEST(FontStack, fallback_first_match_wins)
{
    TextEngine engine;
    const std::string path = findTestFontPath();
    FontHandle h1 = engine.loadFont("calibri", path);
    FontHandle h2 = engine.loadFont("sans", path);
    CHECK(h1.id != 0 && h2.id != 0 && h1.id != h2.id);

    FontStack stack{"mont", "calibri", "sans"};
    CHECK(engine.resolveFont(stack).id == h1.id);
}

TEST(FontStack, fallback_none_match_uses_default)
{
    TextEngine engine;
    FontHandle h1 = engine.loadFont("calibri", findTestFontPath());
    CHECK(h1.id != 0);

    FontStack stack{"mont", "sans"};
    CHECK(engine.resolveFont(stack).id == h1.id);

    engine.setDefaultFont({});
    CHECK(engine.resolveFont(stack).id == 0);
}

TEST(FontStack, load_and_unload)
{
    TextEngine engine;
    FontHandle h = engine.loadFont("test", findTestFontPath());
    CHECK(h.id != 0);

    FontHandle again = engine.getFont("test");
    CHECK(again.id == h.id);

    engine.unloadFont("test");
    FontHandle after = engine.getFont("test");
    CHECK(after.id == 0);
}

TEST(FontStack, load_from_memory)
{
    std::ifstream in(findTestFontPath(), std::ios::binary | std::ios::ate);
    CHECK(static_cast<bool>(in));
    const std::streamsize sz = in.tellg();
    CHECK(sz > 0);
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<size_t>(sz));
    in.read(reinterpret_cast<char *>(bytes.data()), sz);

    TextEngine engine;
    FontHandle h = engine.loadFontFromMemory("mem", bytes.data(), bytes.size());
    CHECK(h.id != 0);
    CHECK(engine.getFont("mem").id == h.id);
}

TEST(FontStack, fallback_uses_runtime_loaded)
{
    TextEngine engine;
    FontHandle h = engine.loadFont("mont", findTestFontPath());
    CHECK(h.id != 0);

    FontStack stack{"mont", "calibri", "sans"};
    CHECK(engine.resolveFont(stack).id == h.id);
}

TEST(FontStack, load_empty_name_is_noop)
{
    TextEngine engine;
    CHECK(engine.loadFont("", findTestFontPath()).id == 0);
    CHECK(engine.loadFontFromMemory("", nullptr, 0).id == 0);
}

TEST(FontStack, unload_unknown_is_noop)
{
    TextEngine engine;
    engine.unloadFont("nonexistent");
    CHECK(engine.getFont("nonexistent").id == 0);
}