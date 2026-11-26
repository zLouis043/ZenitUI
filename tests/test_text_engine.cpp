#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "TextEngine.hpp"

#include <filesystem>

using namespace ZenitUI;
using namespace ZenitUI::Test;

namespace
{
    std::string findInterFontPath()
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

TEST(TextEngine_fonts, load_metrics_and_unload_inter)
{
    TextEngine engine;
    const std::string path = findInterFontPath();

    FontHandle h = engine.loadFont("inter", path);
    CHECK(h.id != 0);
    CHECK(engine.getFont("inter").id == h.id);
    CHECK(engine.defaultFont().id == h.id);

    FontMetrics m = engine.fontMetrics(h, 20.0f);
    CHECK(m.ascent > 10.0f);
    CHECK(m.descent > 1.0f);
    CHECK(m.lineHeight >= 20.0f);

    engine.unloadFont("inter");
    CHECK(engine.getFont("inter").id == 0);
    CHECK(engine.defaultFont().id == 0);
}

TEST(TextEngine_shaping, harfbuzz_kerning_and_utf8_clusters)
{
    TextEngine engine;
    FontHandle h = engine.loadFont("inter", findInterFontPath());
    CHECK(h.id != 0);

    TextMeasureSpec spec;
    spec.font = h;
    spec.fontSize = 32.0f;

    const float wA = engine.measureText("A", spec).x;
    const float wV = engine.measureText("V", spec).x;
    const float wAV = engine.measureText("AV", spec).x;
    CHECK(wA > 0.0f && wV > 0.0f);
    // OpenType GPOS kerning tightens the "AV" pair compared to independent glyphs.
    CHECK(wAV < wA + wV);

    const std::string utf8 = "caf\xC3\xA9"; // "café" (5 bytes, 4 clusters)
    TextLayout lo = engine.layout(utf8, spec);
    CHECK(!lo.synthetic);
    CHECK(lo.lines.size() == 1);
    CHECK(lo.lines[0].glyphs.size() == 4);
    CHECK(lo.lines[0].glyphs[0].cluster == 0);
    CHECK(lo.lines[0].glyphs[3].cluster == 3); // 'é' starts at byte 3
}

TEST(TextEngine_layout, wrap_long_word_max_lines_and_ellipsis)
{
    TextEngine engine;
    FontHandle h = engine.loadFont("inter", findInterFontPath());
    CHECK(h.id != 0);

    TextMeasureSpec spec;
    spec.font = h;
    spec.fontSize = 16.0f;

    // 1. Soft word wrap
    TextLayoutOptions wrapOpts;
    wrapOpts.wrap = true;
    wrapOpts.maxWidth = 100.0f;
    TextLayout wrapped = engine.layout("Hello world from ZenitUI text engine", spec, wrapOpts);
    CHECK(wrapped.lines.size() >= 2);
    for (const auto &ln : wrapped.lines)
        CHECK(ln.width <= 100.5f);

    // 2. Single-line ellipsis
    TextLayoutOptions ellOpts;
    ellOpts.wrap = false;
    ellOpts.ellipsis = true;
    ellOpts.maxWidth = 70.0f;
    TextLayout ell = engine.layout("Very long title that must truncate", spec, ellOpts);
    CHECK(ell.truncated);
    CHECK(ell.lines.size() == 1);
    CHECK(ell.lines[0].ellipsized);
    CHECK(ell.lines[0].width <= 70.5f);
    CHECK(ell.ellipsizedText.size() >= 3);
    CHECK(ell.ellipsizedText.substr(ell.ellipsizedText.size() - 3) == "...");

    // 3. maxLines + ellipsis
    TextLayoutOptions maxOpts;
    maxOpts.wrap = true;
    maxOpts.ellipsis = true;
    maxOpts.maxWidth = 90.0f;
    maxOpts.maxLines = 2;
    TextLayout clamped = engine.layout("One two three four five six seven eight nine", spec, maxOpts);
    CHECK(clamped.truncated);
    CHECK(clamped.lines.size() == 2);
    CHECK(clamped.lines[1].ellipsized);
}

TEST(TextEngine_editing, hit_test_caret_and_selection_rects)
{
    TextEngine engine;
    FontHandle h = engine.loadFont("inter", findInterFontPath());
    CHECK(h.id != 0);

    TextMeasureSpec spec;
    spec.font = h;
    spec.fontSize = 20.0f;
    spec.lineHeight = 28.0f;

    const std::string text = "Hello\ncaf\xC3\xA9"; // line 0: [0..5), line 1: [6..11)
    TextLayout lo = engine.layout(text, spec);
    CHECK(lo.lines.size() == 2);

    // Start and end hit-testing on line 0
    CHECK(TextEngine::hitTestPoint(text, lo, {0.0f, 5.0f}) == 0);
    CHECK(TextEngine::hitTestPoint(text, lo, {500.0f, 5.0f}) == 5);

    // Hit-testing on line 1 (UTF-8 "café") lands on valid cluster boundaries
    const size_t midLine1 = TextEngine::hitTestPoint(text, lo, {lo.lines[1].width - 2.0f, 35.0f});
    CHECK(midLine1 == 11 || midLine1 == 9);

    // Caret rect for start of line 1
    Rect c = TextEngine::caretRect(lo, 6, 2.0f);
    CHECK_NEAR(c.x, 0.0f, 0.01f);
    CHECK_NEAR(c.y, 28.0f, 0.01f);
    CHECK_NEAR(c.height, 28.0f, 0.01f);

    // Multi-line selection from byte 2 ('l') to byte 9 ('é') spans both lines
    auto rects = TextEngine::selectionRects(lo, 2, 9);
    CHECK(rects.size() == 2);
    CHECK(rects[0].width > 0.0f);
    CHECK(rects[1].width > 0.0f);
    CHECK_NEAR(rects[0].y, 0.0f, 0.01f);
    CHECK_NEAR(rects[1].y, 28.0f, 0.01f);
}

TEST(TextEngine_atlas, rasterizes_glyphs_and_batches_draw_geometry)
{
    Env env;
    TextEngine engine;
    FontHandle h = engine.loadFont("inter", findInterFontPath());
    CHECK(h.id != 0);

    TextMeasureSpec spec;
    spec.font = h;
    spec.fontSize = 24.0f;

    TextLayout lo = engine.layout("ZenitUI 2026!", spec);
    engine.draw(env.renderer, lo, {10.0f, 20.0f, 400.0f, 40.0f}, Colors::White, spec);

    CHECK(env.renderer.createTextureCount == 1);
    CHECK(env.renderer.liveTextures == 1);
    CHECK(env.renderer.updateTextureCount > 0);
    CHECK(env.renderer.drawGeometryCalls == 1);
    CHECK(env.renderer.verticesSubmitted > 0);
    CHECK(env.renderer.indicesSubmitted == (env.renderer.verticesSubmitted / 4) * 6);

    // Verify FreeType actually uploaded non-zero alpha coverage into the dynamic atlas.
    bool hasNonZeroAlpha = false;
    for (const auto &[id, tex] : env.renderer.dynamicTextures)
    {
        for (size_t i = 3; i < tex.pixels.size(); i += 4)
        {
            if (tex.pixels[i] > 0)
            {
                hasNonZeroAlpha = true;
                break;
            }
        }
    }
    CHECK(hasNonZeroAlpha);

    // Drawing the same string a second time must hit the glyph cache with zero new uploads.
    const int uploadsBefore = env.renderer.updateTextureCount;
    engine.draw(env.renderer, lo, {10.0f, 20.0f, 400.0f, 40.0f}, Colors::White, spec);
    CHECK(env.renderer.updateTextureCount == uploadsBefore);
    CHECK(env.renderer.drawGeometryCalls == 2);

    engine.clearAtlas(env.renderer);
    CHECK(env.renderer.liveTextures == 0);
    CHECK(engine.stats().pageCount == 0);
    CHECK(engine.stats().cachedGlyphs == 0);
}

TEST(TextEngine_fallback, synthetic_metrics_without_loaded_fonts)
{
    TextEngine engine;
    engine.setFallbackCharWidth(8.0f);

    TextMeasureSpec spec;
    spec.fontSize = 16.0f;
    spec.letterSpacing = 2.0f;

    // 4 codepoints: 4 * 8 + 3 * 2 = 38px
    Vec2 sz = engine.measureText("abcd", spec);
    CHECK_NEAR(sz.x, 38.0f, 0.01f);
    CHECK_NEAR(sz.y, 16.0f, 0.01f);

    TextLayout lo = engine.layout("abcd", spec);
    CHECK(lo.synthetic);
    CHECK(TextEngine::hitTestPoint("abcd", lo, {11.0f, 8.0f}) == 1);
}

TEST(TextEngine_cache, lru_hits_and_invalidates_on_dpi_change)
{
    TextEngine engine;
    FontHandle h = engine.loadFont("inter", findInterFontPath());
    CHECK(h.id != 0);

    TextMeasureSpec spec;
    spec.font = h;
    spec.fontSize = 18.0f;

    auto first = engine.layoutCached("Cached line", spec);
    auto second = engine.layoutCached("Cached line", spec);
    CHECK(first.get() == second.get());
    CHECK(engine.stats().layoutCacheMisses == 1);
    CHECK(engine.stats().layoutCacheHits == 1);

    engine.setDpiScale(2.0f);
    CHECK(engine.stats().layoutCacheSize == 0);
    auto third = engine.layoutCached("Cached line", spec);
    CHECK(third.get() != first.get());
    CHECK(engine.stats().layoutCacheMisses == 2);
}