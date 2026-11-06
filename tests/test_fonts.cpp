#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "StyleAttr.hpp"
#include "Style.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

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
    MockAssetProvider assets;
    assets.registerFont("calibri", FontHandle{1});
    assets.registerFont("sans",    FontHandle{2});

    FontStack stack{"mont", "calibri", "sans"};
    FontHandle h = assets.getFont(stack);
    CHECK(h.id == 1);  // calibri, perché mont non c'è
}

TEST(FontStack, fallback_none_match)
{
    MockAssetProvider assets;
    assets.registerFont("calibri", FontHandle{1});

    FontStack stack{"mont", "sans"};
    FontHandle h = assets.getFont(stack);
    CHECK(h.id == 0);
}