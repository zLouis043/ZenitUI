#pragma once

/**
 * \file TextEngine.hpp
 * \brief Backend-independent text shaping (HarfBuzz), glyph rasterization
 *        (FreeType), dynamic atlas packing, layout, hit-testing, and batched
 *        rendering.
 *
 * Designed to live as a value member inside `UIContext` (zero global state).
 * FreeType and HarfBuzz headers are strictly private to `src/TextEngine.cpp`
 * via PIMPL.
 */

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "UIEnums.hpp"
#include "Style.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ZenitUI
{
    /** \addtogroup grp_core
     *  @{
     */

    class IRenderer;

    /**
     * \struct FontMetrics
     * \brief Scaled vertical metrics for a font at a given logical pixel size.
     */
    struct FontMetrics
    {
        float ascent{0.0f};     ///< Distance from baseline to top of ascenders (positive, logical px).
        float descent{0.0f};    ///< Distance from baseline to bottom of descenders (positive, logical px).
        float lineHeight{0.0f}; ///< Recommended line advance (logical px).
    };

    /**
     * \struct TextMeasureSpec
     * \brief Typographic parameters governing shaping and measurement.
     */
    struct TextMeasureSpec
    {
        FontHandle font{};            ///< Explicit primary font handle (optional when `fontStack` is set).
        FontStack fontStack{};        ///< Ordered CSS-style font fallback names.
        float fontSize{16.0f};        ///< Logical font size in pixels.
        float letterSpacing{0.0f};    ///< Additional horizontal tracking between grapheme clusters (logical px).
        float lineHeight{0.0f};       ///< Explicit line height in logical px (`<= 0` uses font's natural height).
    };

    /**
     * \struct TextLayoutOptions
     * \brief Wrapping, truncation, and whitespace options for `TextEngine::layout`.
     */
    struct TextLayoutOptions
    {
        float maxWidth{0.0f};              ///< Wrapping / ellipsis width in logical px (`<= 0` = unbounded).
        bool wrap{false};                  ///< Enable soft word/cluster wrapping at `maxWidth`.
        bool ellipsis{false};              ///< Append `ellipsisText` when overflowing `maxWidth` or `maxLines`.
        int maxLines{0};                   ///< Maximum number of visual lines (`<= 0` = unlimited).
        bool preserveSpaces{false};        ///< Keep spaces contiguous across soft breaks (for editable fields).
        std::string ellipsisText{"..."};   ///< Suffix used when truncating overflowing lines.
    };

    /**
     * \struct ShapedGlyph
     * \brief Single shaped glyph positioned along a laid-out line.
     */
    struct ShapedGlyph
    {
        uint32_t fontId{0};   ///< `FontHandle::id` that owns `glyphId` (`0` in synthetic fallback mode).
        uint32_t glyphId{0};  ///< Font-specific glyph index (GID).
        uint32_t cluster{0};  ///< Byte offset in the original UTF-8 input string.
        float x{0.0f};        ///< Pen X offset from the start of the line (logical px).
        float xAdvance{0.0f}; ///< Logical advance including tracking on cluster boundaries.
        float xOffset{0.0f};  ///< HarfBuzz horizontal glyph offset (logical px).
        float yOffset{0.0f};  ///< HarfBuzz vertical glyph offset (positive upward, logical px).
    };

    /**
     * \struct TextLine
     * \brief One visual line produced by `TextEngine::layout`.
     */
    struct TextLine
    {
        size_t byteStart{0};            ///< Inclusive UTF-8 byte start in original text.
        size_t byteEnd{0};              ///< Exclusive UTF-8 byte end in original text.
        float width{0.0f};              ///< Measured logical width of this line.
        float height{0.0f};             ///< Logical height of this line box.
        float yOffset{0.0f};            ///< Top Y offset from the start of the text block.
        float baselineOffset{0.0f};     ///< Distance from `yOffset` down to the typographic baseline.
        bool hardBreak{true};           ///< `true` if terminated by `\n` or end-of-string; `false` on soft wrap.
        bool ellipsized{false};         ///< `true` if this line was truncated with `ellipsisText`.
        std::vector<ShapedGlyph> glyphs;
    };

    /**
     * \struct TextLayout
     * \brief Complete laid-out text block ready for sizing, hit-testing, and rendering.
     */
    struct TextLayout
    {
        std::vector<TextLine> lines;
        Vec2 size{0.0f, 0.0f};
        FontMetrics metrics{};
        std::string ellipsizedText; ///< Populated with the truncated last line when `truncated` is `true`.
        bool truncated{false};
        bool synthetic{false};      ///< `true` when measured via synthetic fallback (no loaded fonts).
    };

    /**
     * \struct TextAtlasStats
     * \brief Diagnostic counters for the dynamic FreeType glyph atlas.
     */
    struct TextAtlasStats
    {
        size_t pageCount{0};
        size_t cachedGlyphs{0};
        size_t layoutCacheSize{0};
        uint64_t layoutCacheHits{0};
        uint64_t layoutCacheMisses{0};
    };

    /**
     * \class TextEngine
     * \brief Per-context font store, HarfBuzz shaper, FreeType glyph atlas, and layout engine.
     */
    class TextEngine
    {
    public:
        TextEngine();
        ~TextEngine();

        TextEngine(const TextEngine &) = delete;
        TextEngine &operator=(const TextEngine &) = delete;
        TextEngine(TextEngine &&) noexcept;
        TextEngine &operator=(TextEngine &&) noexcept;

        // ---- Font lifecycle ----

        /** \brief Load a TTF/OTF font file from disk under logical \p name. */
        FontHandle loadFont(std::string_view name, std::string_view path);

        /** \brief Load a TTF/OTF font from an in-memory buffer under logical \p name (copies bytes). */
        FontHandle loadFontFromMemory(std::string_view name, const void *data, size_t size);

        /** \brief Unload a font by logical name and invalidate layout caches. */
        void unloadFont(std::string_view name);

        /** \brief Look up a loaded font by logical name; returns `{0}` if not found. */
        FontHandle getFont(std::string_view name) const;

        /** \brief Resolve the first loaded font matching \p stack; falls back to the default font if set. */
        FontHandle resolveFont(const FontStack &stack, FontHandle fallback = {}) const;

        /** \brief Set the fallback font used when a node specifies no font or an unknown font name. */
        void setDefaultFont(FontHandle h);

        /** \brief Return the current default font handle (`{0}` if none). */
        FontHandle defaultFont() const;

        /** \brief Query scaled vertical metrics for \p font at logical \p fontSize. */
        FontMetrics fontMetrics(FontHandle font, float fontSize) const;

        // ---- Configuration & cache control ----

        /** \brief Update the active logical-to-physical DPI scale used when rasterizing glyphs. */
        void setDpiScale(float scale);
        float dpiScale() const;

        /** \brief Set the fixed character advance used when no TTF/OTF font is loaded (headless fallback). */
        void setFallbackCharWidth(float width);
        float fallbackCharWidth() const;

        /** \brief Configure the maximum number of entries kept in the layout LRU cache. */
        void setLayoutCacheCapacity(size_t maxEntries);

        /** \brief Destroy all GPU atlas pages on \p renderer and clear the glyph cache. */
        void clearAtlas(IRenderer &renderer);

        /** \brief Clear the cached `TextLayout` entries without touching GPU atlas textures. */
        void clearLayoutCache();

        /** \brief Return current atlas and layout-cache diagnostics. */
        TextAtlasStats stats() const;

        // ---- Measurement & Layout ----

        /** \brief Shape and lay out \p text according to \p spec and \p options (uncached). */
        TextLayout layout(std::string_view text,
                          const TextMeasureSpec &spec,
                          const TextLayoutOptions &options = {}) const;

        /** \brief Cached variant of `layout()` backed by the per-engine LRU cache. */
        std::shared_ptr<const TextLayout> layoutCached(std::string_view text,
                                                       const TextMeasureSpec &spec,
                                                       const TextLayoutOptions &options = {});

        /** \brief Measure a single line (no wrapping, splits on `\n`) and return its `{width, height}`. */
        Vec2 measureText(std::string_view text, const TextMeasureSpec &spec) const;

        // ---- Hit-testing, Caret & Selection ----

        /**
         * \brief Map a line-local X coordinate to the closest UTF-8 cluster byte offset.
         * \param clampSoftBreak When `true` and `!line.hardBreak`, avoids returning `line.byteEnd`
         *                       (which belongs to the next wrapped line).
         */
        static size_t hitTestLine(std::string_view fullText,
                                  const TextLine &line,
                                  float localX,
                                  bool clampSoftBreak = false);

        /** \brief Map a layout-local `(x, y)` point to the closest UTF-8 cluster byte offset. */
        static size_t hitTestPoint(std::string_view fullText,
                                   const TextLayout &layout,
                                   Vec2 localPos,
                                   Align textAlign = Align::Start,
                                   float boxWidth = 0.0f,
                                   bool clampSoftBreak = true);

        /** \brief Return the line index containing \p byteIndex. */
        static size_t lineIndexForByte(const TextLayout &layout, size_t byteIndex);

        /** \brief Return the line-local X coordinate of \p byteIndex on \p line. */
        static float caretXOnLine(const TextLine &line, size_t byteIndex);

        /** \brief Compute the caret rectangle in layout-local coordinates. */
        static Rect caretRect(const TextLayout &layout,
                              size_t byteIndex,
                              float caretWidth = 2.0f,
                              Align textAlign = Align::Start,
                              float boxWidth = 0.0f);

        /** \brief Compute selection highlight rectangles in layout-local coordinates for `[selStart, selEnd)`. */
        static std::vector<Rect> selectionRects(const TextLayout &layout,
                                                size_t selStart,
                                                size_t selEnd,
                                                Align textAlign = Align::Start,
                                                float boxWidth = 0.0f);

        // ---- Batched Rendering ----

        /**
         * \brief Rasterize any missing glyphs into the atlas and emit batched `drawGeometry` calls.
         * \param renderer Destination renderer.
         * \param layout   Previously computed `TextLayout`.
         * \param bounds   Logical bounding box for alignment (x, y, width, height).
         * \param color    Text color (including opacity).
         * \param spec     Font size used to select physical glyph pixel size.
         * \param align    Horizontal alignment within `bounds.width`.
         */
        void draw(IRenderer &renderer,
                  const TextLayout &layout,
                  Rect bounds,
                  Color color,
                  const TextMeasureSpec &spec,
                  Align align = Align::Start);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    /** @} */
} // namespace ZenitUI