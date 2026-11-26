#include "TextEngine.hpp"
#include "UIContext.hpp"
#include "Utf8.hpp"
#include "Logger.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ot.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <list>
#include <unordered_map>

namespace ZenitUI
{
    namespace
    {
        constexpr int kAtlasPageSize = 1024;
        constexpr int kGlyphPadding = 1;
        constexpr size_t kMaxAtlasPages = 8;

        struct TransparentStringHash
        {
            using is_transparent = void;

            size_t operator()(std::string_view sv) const noexcept
            {
                return std::hash<std::string_view>{}(sv);
            }
            size_t operator()(const std::string &s) const noexcept
            {
                return std::hash<std::string_view>{}(s);
            }
        };

        struct FontRecord
        {
            uint32_t id{0};
            std::string name;
            std::vector<uint8_t> data; // Kept immutable in place while FT/HB reference it.
            FT_Face ftFace{nullptr};
            hb_face_t *hbFace{nullptr};
            hb_font_t *hbFont{nullptr};
            int unitsPerEm{1000};
            int ascender{800};
            int descender{-200};
            int height{1000};
        };

        void destroyFontRecord(FontRecord &rec)
        {
            if (rec.hbFont)
            {
                hb_font_destroy(rec.hbFont);
                rec.hbFont = nullptr;
            }
            if (rec.hbFace)
            {
                hb_face_destroy(rec.hbFace);
                rec.hbFace = nullptr;
            }
            if (rec.ftFace)
            {
                FT_Done_Face(rec.ftFace);
                rec.ftFace = nullptr;
            }
            rec.data.clear();
        }

        struct GlyphKey
        {
            uint32_t fontId{0};
            uint32_t glyphId{0};
            uint16_t pixelSize{0};

            bool operator==(const GlyphKey &o) const noexcept
            {
                return fontId == o.fontId && glyphId == o.glyphId && pixelSize == o.pixelSize;
            }
        };

        struct GlyphKeyHash
        {
            size_t operator()(const GlyphKey &k) const noexcept
            {
                uint64_t h = (static_cast<uint64_t>(k.fontId) << 32) ^
                             (static_cast<uint64_t>(k.glyphId) << 16) ^
                             static_cast<uint64_t>(k.pixelSize);
                h ^= h >> 33;
                h *= 0xff51afd7ed558ccdULL;
                h ^= h >> 33;
                h *= 0xc4ceb9fe1a85ec53ULL;
                h ^= h >> 33;
                return static_cast<size_t>(h);
            }
        };

        struct AtlasGlyph
        {
            uint32_t pageIndex{0};
            float u0{0.0f};
            float v0{0.0f};
            float u1{0.0f};
            float v1{0.0f};
            float bearingX{0.0f}; // Physical px
            float bearingY{0.0f}; // Physical px
            float width{0.0f};    // Physical px
            float height{0.0f};   // Physical px
            bool hasBitmap{false};
        };

        struct AtlasPage
        {
            TextureHandle texture{};
            int width{kAtlasPageSize};
            int height{kAtlasPageSize};
            int cursorX{0};
            int cursorY{0};
            int shelfHeight{0};
        };

        struct LayoutCacheKey
        {
            std::string text;
            uint32_t primaryFontId{0};
            std::vector<std::string> fontStack;
            int fontSizeQ{0};
            int letterSpacingQ{0};
            int lineHeightQ{0};
            int maxWidthQ{0};
            int maxLines{0};
            uint8_t flags{0};
            std::string ellipsisText;

            bool operator==(const LayoutCacheKey &o) const noexcept = default;
        };

        struct LayoutCacheKeyHash
        {
            size_t operator()(const LayoutCacheKey &k) const noexcept
            {
                uint64_t h = 14695981039346656037ULL;
                auto mix = [&h](uint64_t v)
                {
                    h ^= v;
                    h *= 1099511628211ULL;
                };
                mix(std::hash<std::string>{}(k.text));
                mix(k.primaryFontId);
                for (const auto &s : k.fontStack)
                    mix(std::hash<std::string>{}(s));
                mix(static_cast<uint32_t>(k.fontSizeQ));
                mix(static_cast<uint32_t>(k.letterSpacingQ));
                mix(static_cast<uint32_t>(k.lineHeightQ));
                mix(static_cast<uint32_t>(k.maxWidthQ));
                mix(static_cast<uint32_t>(k.maxLines));
                mix(k.flags);
                mix(std::hash<std::string>{}(k.ellipsisText));
                return static_cast<size_t>(h);
            }
        };

        inline float alignOffsetX(float boxW, float lineW, Align align) noexcept
        {
            if (align == Align::Center)
                return (boxW - lineW) * 0.5f;
            if (align == Align::End)
                return boxW - lineW;
            return 0.0f;
        }

        inline int quantizePx(float v) noexcept
        {
            return static_cast<int>(std::lround(v * 16.0f));
        }

        bool readBinaryFile(std::string_view path, std::vector<uint8_t> &out)
        {
            std::ifstream in(std::string(path), std::ios::binary | std::ios::ate);
            if (!in)
                return false;
            const std::streamsize sz = in.tellg();
            if (sz <= 0)
                return false;
            in.seekg(0, std::ios::beg);
            out.resize(static_cast<size_t>(sz));
            return static_cast<bool>(in.read(reinterpret_cast<char *>(out.data()), sz));
        }
    } // namespace

    // =========================================================================
    // TextEngine::Impl
    // =========================================================================

    struct TextEngine::Impl
    {
        FT_Library ftLib{nullptr};
        hb_buffer_t *hbBuf{nullptr};

        uint32_t nextFontId{1};
        FontHandle defaultFont{};
        std::unordered_map<uint32_t, std::unique_ptr<FontRecord>> fontsById;
        std::unordered_map<std::string, uint32_t, TransparentStringHash, std::equal_to<>> fontsByName;

        float dpiScale{1.0f};
        float fallbackCharWidth{8.0f};

        // Dynamic FreeType glyph atlas
        std::vector<AtlasPage> atlasPages;
        std::unordered_map<GlyphKey, AtlasGlyph, GlyphKeyHash> glyphCache;
        std::vector<uint8_t> uploadScratch;

        // Batched draw scratch per atlas page
        struct PageBatch
        {
            std::vector<Vertex2D> vertices;
            std::vector<uint32_t> indices;
        };
        std::vector<PageBatch> drawBatches;

        // Per-context layout LRU cache
        size_t layoutCacheCap{256};
        uint64_t layoutCacheHits{0};
        uint64_t layoutCacheMisses{0};
        struct CacheEntry
        {
            LayoutCacheKey key;
            std::shared_ptr<const TextLayout> value;
        };
        std::list<CacheEntry> lruList;
        std::unordered_map<LayoutCacheKey, std::list<CacheEntry>::iterator, LayoutCacheKeyHash> lruMap;

        Impl()
        {
            if (FT_Init_FreeType(&ftLib) != 0)
            {
                ftLib = nullptr;
                logError("TextEngine", "", 0, 0, "FT_Init_FreeType failed");
            }
            hbBuf = hb_buffer_create();
        }

        ~Impl()
        {
            for (auto &[id, rec] : fontsById)
            {
                if (rec)
                    destroyFontRecord(*rec);
            }
            fontsById.clear();
            fontsByName.clear();
            if (hbBuf)
            {
                hb_buffer_destroy(hbBuf);
                hbBuf = nullptr;
            }
            if (ftLib)
            {
                FT_Done_FreeType(ftLib);
                ftLib = nullptr;
            }
        }

        void clearLayoutCache()
        {
            lruMap.clear();
            lruList.clear();
        }

        FontRecord *findFont(uint32_t id) const
        {
            if (id == 0)
                return nullptr;
            auto it = fontsById.find(id);
            return it != fontsById.end() ? it->second.get() : nullptr;
        }

        std::vector<uint32_t> buildFontChain(const TextMeasureSpec &spec) const
        {
            std::vector<uint32_t> chain;
            auto appendUnique = [&](uint32_t id)
            {
                if (id == 0 || fontsById.find(id) == fontsById.end())
                    return;
                if (std::find(chain.begin(), chain.end(), id) == chain.end())
                    chain.push_back(id);
            };

            for (const auto &name : spec.fontStack)
            {
                auto it = fontsByName.find(name);
                if (it != fontsByName.end())
                    appendUnique(it->second);
            }
            appendUnique(spec.font.id);
            appendUnique(defaultFont.id);
            return chain;
        }

        FontMetrics computeMetrics(uint32_t primaryFontId, float fontSize, float explicitLineHeight) const
        {
            const float fs = std::max(1.0f, fontSize);
            if (const auto *rec = findFont(primaryFontId))
            {
                const float scale = fs / static_cast<float>(std::max(1, rec->unitsPerEm));
                const float ascent = static_cast<float>(rec->ascender) * scale;
                const float descent = static_cast<float>(-rec->descender) * scale;
                const float naturalH = std::max(fs, ascent + descent);
                const float lineH = (explicitLineHeight > 0.0f) ? explicitLineHeight : naturalH;
                return {ascent, descent, lineH};
            }
            // Synthetic headless metrics matching MockRenderer.
            const float lineH = (explicitLineHeight > 0.0f) ? explicitLineHeight : fs;
            return {fs * 0.8f, fs * 0.2f, lineH};
        }

        static float computeBaselineOffset(const FontMetrics &m, float lineH) noexcept
        {
            const float contentH = m.ascent + m.descent;
            return (lineH - contentH) * 0.5f + m.ascent;
        }

        // Shape a single contiguous line slice (no '\n') into `outGlyphs`, returning total logical width.
        float shapeSlice(std::string_view slice,
                         size_t baseByteOffset,
                         const std::vector<uint32_t> &fontChain,
                         float fontSize,
                         float letterSpacing,
                         std::vector<ShapedGlyph> &outGlyphs) const
        {
            outGlyphs.clear();
            if (slice.empty())
                return 0.0f;

            // Synthetic headless path when no real fonts are available.
            if (fontChain.empty())
            {
                float penX = 0.0f;
                size_t pos = 0;
                while (pos < slice.size())
                {
                    const size_t next = Utf8::nextCluster(slice, pos);
                    const size_t cpCount = std::max<size_t>(1, Utf8::codepointCount(slice.substr(pos, next - pos)));
                    float adv = fallbackCharWidth * static_cast<float>(cpCount);
                    if (next < slice.size())
                        adv += letterSpacing;

                    ShapedGlyph g;
                    g.fontId = 0;
                    g.glyphId = 0;
                    g.cluster = static_cast<uint32_t>(baseByteOffset + pos);
                    g.x = penX;
                    g.xAdvance = adv;
                    outGlyphs.push_back(g);

                    penX += adv;
                    pos = next;
                }
                return penX;
            }

            // Pick a font per grapheme cluster so combining marks stay with their base glyph.
            struct FontRun
            {
                size_t start{0};
                size_t end{0};
                uint32_t fontId{0};
            };
            std::vector<FontRun> runs;

            auto pickFontForCodepoint = [&](uint32_t cp) -> uint32_t
            {
                for (uint32_t fid : fontChain)
                {
                    auto *rec = findFont(fid);
                    if (!rec || !rec->hbFont)
                        continue;
                    hb_codepoint_t gid = 0;
                    if (hb_font_get_nominal_glyph(rec->hbFont, cp, &gid) && gid != 0)
                        return fid;
                }
                return fontChain.front();
            };

            size_t pos = 0;
            while (pos < slice.size())
            {
                uint32_t baseCp = 0;
                Utf8::decode(slice, pos, baseCp);
                const size_t next = Utf8::nextCluster(slice, pos);
                const uint32_t chosenFont = pickFontForCodepoint(baseCp);

                if (!runs.empty() && runs.back().fontId == chosenFont)
                    runs.back().end = next;
                else
                    runs.push_back({pos, next, chosenFont});
                pos = next;
            }

            const int scale266 = static_cast<int>(std::lround(std::max(1.0f, fontSize) * 64.0f));
            const unsigned int ppem = static_cast<unsigned int>(std::max<long>(1L, std::lround(fontSize)));

            for (const auto &run : runs)
            {
                auto *rec = findFont(run.fontId);
                if (!rec || !rec->hbFont || !hbBuf)
                    continue;

                hb_font_set_scale(rec->hbFont, scale266, scale266);
                hb_font_set_ppem(rec->hbFont, ppem, ppem);

                hb_buffer_clear_contents(hbBuf);
                hb_buffer_add_utf8(hbBuf,
                                   slice.data(),
                                   static_cast<int>(slice.size()),
                                   static_cast<unsigned int>(run.start),
                                   static_cast<int>(run.end - run.start));
                hb_buffer_guess_segment_properties(hbBuf);
                hb_shape(rec->hbFont, hbBuf, nullptr, 0);

                unsigned int count = 0;
                const hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(hbBuf, &count);
                const hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(hbBuf, &count);

                outGlyphs.reserve(outGlyphs.size() + count);
                for (unsigned int i = 0; i < count; ++i)
                {
                    ShapedGlyph g;
                    g.fontId = run.fontId;
                    g.glyphId = infos[i].codepoint;
                    g.cluster = static_cast<uint32_t>(baseByteOffset + infos[i].cluster);
                    g.xAdvance = static_cast<float>(positions[i].x_advance) / 64.0f;
                    g.xOffset = static_cast<float>(positions[i].x_offset) / 64.0f;
                    g.yOffset = static_cast<float>(positions[i].y_offset) / 64.0f;
                    outGlyphs.push_back(g);
                }
            }

            // Compute cumulative pen positions and apply letterSpacing between distinct clusters.
            float penX = 0.0f;
            for (size_t i = 0; i < outGlyphs.size(); ++i)
            {
                outGlyphs[i].x = penX;
                if (letterSpacing != 0.0f &&
                    i + 1 < outGlyphs.size() &&
                    outGlyphs[i + 1].cluster != outGlyphs[i].cluster)
                {
                    outGlyphs[i].xAdvance += letterSpacing;
                }
                penX += outGlyphs[i].xAdvance;
            }
            return penX;
        }

        // Build an ellipsized single line that fits inside `maxWidth`.
        std::string ellipsizeSlice(std::string_view slice,
                                   size_t baseByteOffset,
                                   const std::vector<uint32_t> &fontChain,
                                   float fontSize,
                                   float letterSpacing,
                                   float maxWidth,
                                   std::string_view ellipsisText,
                                   TextLine &outLine) const
        {
            std::vector<ShapedGlyph> probe;
            const float ellW = shapeSlice(ellipsisText, 0, fontChain, fontSize, letterSpacing, probe);

            if (ellW > maxWidth)
            {
                // Shrink bare ellipsis ("..." -> ".." -> "." -> "") when box is tiny.
                std::string shrunk;
                auto eBounds = Utf8::clusterBoundaries(ellipsisText);
                for (size_t k = eBounds.size() - 1; k > 0; --k)
                {
                    std::string_view cand = ellipsisText.substr(0, eBounds[k]);
                    const float w = shapeSlice(cand, baseByteOffset, fontChain, fontSize, letterSpacing, probe);
                    if (w <= maxWidth)
                    {
                        shrunk.assign(cand);
                        outLine.width = w;
                        outLine.glyphs = std::move(probe);
                        outLine.ellipsized = true;
                        return shrunk;
                    }
                }
                outLine.width = 0.0f;
                outLine.glyphs.clear();
                outLine.ellipsized = true;
                return {};
            }

            // Binary search longest cluster prefix such that (prefix + ellipsisText) fits.
            auto bounds = Utf8::clusterBoundaries(slice);
            size_t lo = 0;
            size_t hi = bounds.size() - 1;
            size_t bestIdx = 0;
            while (lo <= hi)
            {
                const size_t mid = lo + (hi - lo) / 2;
                std::string candidate(slice.substr(0, bounds[mid]));
                candidate.append(ellipsisText);
                const float w = shapeSlice(candidate, baseByteOffset, fontChain, fontSize, letterSpacing, probe);
                if (w <= maxWidth)
                {
                    bestIdx = mid;
                    lo = mid + 1;
                }
                else
                {
                    if (mid == 0)
                        break;
                    hi = mid - 1;
                }
            }

            size_t cut = bounds[bestIdx];
            while (cut > 0 && slice[cut - 1] == ' ')
                --cut;

            std::string finalStr(slice.substr(0, cut));
            finalStr.append(ellipsisText);
            outLine.width = shapeSlice(finalStr, baseByteOffset, fontChain, fontSize, letterSpacing, outLine.glyphs);
            outLine.ellipsized = true;
            return finalStr;
        }

        // Split one paragraph `[pStart, pEnd)` into wrapped byte ranges.
        struct LineSlice
        {
            size_t start{0};
            size_t end{0};
            bool hardBreak{true};
        };

        void wrapParagraph(std::string_view fullText,
                           size_t pStart,
                           size_t pEnd,
                           const std::vector<uint32_t> &fontChain,
                           float fontSize,
                           float letterSpacing,
                           float maxWidth,
                           bool preserveSpaces,
                           std::vector<LineSlice> &outSlices) const
        {
            if (pStart == pEnd)
            {
                outSlices.push_back({pStart, pEnd, true});
                return;
            }

            const std::string_view para = fullText.substr(pStart, pEnd - pStart);
            std::vector<ShapedGlyph> paraGlyphs;
            const float fullW = shapeSlice(para, pStart, fontChain, fontSize, letterSpacing, paraGlyphs);

            if (fullW <= maxWidth)
            {
                outSlices.push_back({pStart, pEnd, true});
                return;
            }

            // Aggregate shaped glyphs into grapheme clusters for O(N) break scanning.
            struct ClusterSpan
            {
                size_t byteStart{0};
                size_t byteEnd{0};
                float width{0.0f};
                bool isSpace{false};
            };

            auto bounds = Utf8::clusterBoundaries(para);
            const size_t numClusters = bounds.size() - 1;
            std::vector<ClusterSpan> clusters;
            clusters.reserve(numClusters);

            size_t gIdx = 0;
            for (size_t ci = 0; ci < numClusters; ++ci)
            {
                const size_t bStart = pStart + bounds[ci];
                const size_t bEnd = pStart + bounds[ci + 1];
                float w = 0.0f;
                while (gIdx < paraGlyphs.size() && paraGlyphs[gIdx].cluster < bEnd)
                {
                    if (paraGlyphs[gIdx].cluster >= bStart)
                        w += paraGlyphs[gIdx].xAdvance;
                    ++gIdx;
                }
                const bool isSpace = (bEnd - bStart == 1 && fullText[bStart] == ' ');
                clusters.push_back({bStart, bEnd, w, isSpace});
            }

            size_t lineStartIdx = 0;
            while (lineStartIdx < numClusters)
            {
                // In non-preserving mode (static Text), skip leading spaces on wrapped lines.
                if (!preserveSpaces && lineStartIdx > 0)
                {
                    while (lineStartIdx < numClusters && clusters[lineStartIdx].isSpace)
                        ++lineStartIdx;
                    if (lineStartIdx >= numClusters)
                        break;
                }

                float curW = 0.0f;
                size_t lastBreakAfter = lineStartIdx; // 0 = no space seen yet on this line
                bool hasSpaceBreak = false;
                size_t i = lineStartIdx;

                while (i < numClusters)
                {
                    const float nextW = curW + clusters[i].width;
                    // Subtract trailing letterSpacing when testing if the last cluster fits.
                    const float effectiveW = (letterSpacing > 0.0f && i + 1 < numClusters)
                                                 ? (nextW - letterSpacing)
                                                 : nextW;

                    if (i > lineStartIdx && effectiveW > maxWidth)
                        break;

                    curW = nextW;
                    if (clusters[i].isSpace)
                    {
                        lastBreakAfter = i + 1;
                        hasSpaceBreak = true;
                    }
                    ++i;
                }

                if (i >= numClusters)
                {
                    size_t sliceEnd = clusters.back().byteEnd;
                    if (!preserveSpaces)
                    {
                        while (sliceEnd > clusters[lineStartIdx].byteStart && fullText[sliceEnd - 1] == ' ')
                            --sliceEnd;
                    }
                    outSlices.push_back({clusters[lineStartIdx].byteStart, sliceEnd, true});
                    break;
                }

                // Overflow occurred at cluster `i`.
                size_t breakIdx = i;
                if (preserveSpaces)
                {
                    if (clusters[i].isSpace)
                        breakIdx = i + 1;
                    else if (hasSpaceBreak)
                        breakIdx = lastBreakAfter;
                }
                else
                {
                    if (hasSpaceBreak)
                        breakIdx = lastBreakAfter;
                }

                if (breakIdx <= lineStartIdx)
                    breakIdx = lineStartIdx + 1; // Always make forward progress.

                size_t sliceEnd = clusters[breakIdx - 1].byteEnd;
                if (!preserveSpaces)
                {
                    while (sliceEnd > clusters[lineStartIdx].byteStart && fullText[sliceEnd - 1] == ' ')
                        --sliceEnd;
                }

                outSlices.push_back({clusters[lineStartIdx].byteStart, sliceEnd, false});
                lineStartIdx = breakIdx;
            }
        }

        TextLayout computeLayout(std::string_view text,
                                 const TextMeasureSpec &spec,
                                 const TextLayoutOptions &options) const
        {
            TextLayout result;
            const auto fontChain = buildFontChain(spec);
            result.synthetic = fontChain.empty();

            const uint32_t primaryId = fontChain.empty() ? 0u : fontChain.front();
            result.metrics = computeMetrics(primaryId, spec.fontSize, spec.lineHeight);

            const float lineH = result.metrics.lineHeight;
            const float baselineOff = computeBaselineOffset(result.metrics, lineH);

            // 1. Collect line byte slices across hard paragraphs.
            std::vector<LineSlice> slices;
            size_t pStart = 0;
            while (pStart <= text.size())
            {
                const size_t nl = text.find('\n', pStart);
                const size_t pEnd = (nl == std::string_view::npos) ? text.size() : nl;

                if (options.wrap && options.maxWidth > 0.0f)
                {
                    wrapParagraph(text, pStart, pEnd, fontChain, spec.fontSize,
                                  spec.letterSpacing, options.maxWidth,
                                  options.preserveSpaces, slices);
                }
                else
                {
                    slices.push_back({pStart, pEnd, true});
                }

                if (nl == std::string_view::npos)
                    break;
                pStart = nl + 1;
            }

            if (slices.empty())
                slices.push_back({0, 0, true});

            // 2. Enforce maxLines if requested.
            bool cutByMaxLines = false;
            if (options.maxLines > 0 && slices.size() > static_cast<size_t>(options.maxLines))
            {
                slices.resize(static_cast<size_t>(options.maxLines));
                cutByMaxLines = true;
                result.truncated = true;
            }

            // 3. Shape each resulting line and apply ellipsis when needed.
            result.lines.reserve(slices.size());
            float maxLineW = 0.0f;
            float curY = 0.0f;

            for (size_t li = 0; li < slices.size(); ++li)
            {
                const auto &sl = slices[li];
                TextLine line;
                line.byteStart = sl.start;
                line.byteEnd = sl.end;
                line.height = lineH;
                line.yOffset = curY;
                line.baselineOffset = baselineOff;
                line.hardBreak = sl.hardBreak;

                const std::string_view lineView = text.substr(sl.start, sl.end - sl.start);
                line.width = shapeSlice(lineView, sl.start, fontChain, spec.fontSize,
                                        spec.letterSpacing, line.glyphs);

                const bool isLastLine = (li + 1 == slices.size());
                const bool needsEllipsis =
                    options.ellipsis && options.maxWidth > 0.0f &&
                    ((!options.wrap && line.width > options.maxWidth) ||
                     (isLastLine && cutByMaxLines));

                if (needsEllipsis)
                {
                    result.ellipsizedText = ellipsizeSlice(
                        lineView, sl.start, fontChain, spec.fontSize, spec.letterSpacing,
                        options.maxWidth, options.ellipsisText, line);
                    result.truncated = true;
                }

                maxLineW = std::max(maxLineW, line.width);
                curY += lineH;
                result.lines.push_back(std::move(line));
            }

            result.size = {maxLineW, curY};
            return result;
        }

        // ---- FreeType rasterization & shelf-packed atlas ----

        AtlasPage *allocateAtlasSlot(IRenderer &renderer, int reqW, int reqH, int &outX, int &outY, uint32_t &outPageIdx)
        {
            const int cellW = reqW + kGlyphPadding * 2;
            const int cellH = reqH + kGlyphPadding * 2;
            if (cellW > kAtlasPageSize || cellH > kAtlasPageSize)
                return nullptr;

            for (size_t pi = 0; pi < atlasPages.size(); ++pi)
            {
                auto &page = atlasPages[pi];
                if (page.cursorX + cellW <= page.width && page.cursorY + cellH <= page.height)
                {
                    outX = page.cursorX;
                    outY = page.cursorY;
                    page.cursorX += cellW;
                    page.shelfHeight = std::max(page.shelfHeight, cellH);
                    outPageIdx = static_cast<uint32_t>(pi);
                    return &page;
                }
                if (page.cursorY + page.shelfHeight + cellH <= page.height)
                {
                    page.cursorY += page.shelfHeight;
                    page.cursorX = cellW;
                    page.shelfHeight = cellH;
                    outX = 0;
                    outY = page.cursorY;
                    outPageIdx = static_cast<uint32_t>(pi);
                    return &page;
                }
            }

            // Reset atlas if page budget is exceeded.
            if (atlasPages.size() >= kMaxAtlasPages)
            {
                for (auto &p : atlasPages)
                {
                    if (p.texture.valid())
                        renderer.destroyTexture(p.texture);
                }
                atlasPages.clear();
                glyphCache.clear();
            }

            TextureHandle tex = renderer.createTexture(kAtlasPageSize, kAtlasPageSize, nullptr);
            if (!tex.valid())
                return nullptr;

            AtlasPage page;
            page.texture = tex;
            page.width = kAtlasPageSize;
            page.height = kAtlasPageSize;
            page.cursorX = cellW;
            page.cursorY = 0;
            page.shelfHeight = cellH;
            atlasPages.push_back(page);

            outX = 0;
            outY = 0;
            outPageIdx = static_cast<uint32_t>(atlasPages.size() - 1);
            return &atlasPages.back();
        }

        const AtlasGlyph *ensureGlyph(IRenderer &renderer, const GlyphKey &key)
        {
            if (auto it = glyphCache.find(key); it != glyphCache.end())
                return &it->second;

            auto *rec = findFont(key.fontId);
            if (!rec || !rec->ftFace)
                return nullptr;

            FT_Set_Pixel_Sizes(rec->ftFace, 0, key.pixelSize);
            if (FT_Load_Glyph(rec->ftFace, key.glyphId, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT) != 0)
                return nullptr;

            FT_GlyphSlot slot = rec->ftFace->glyph;
            const FT_Bitmap &bm = slot->bitmap;
            const int bw = static_cast<int>(bm.width);
            const int bh = static_cast<int>(bm.rows);

            AtlasGlyph entry{};
            entry.bearingX = static_cast<float>(slot->bitmap_left);
            entry.bearingY = static_cast<float>(slot->bitmap_top);
            entry.width = static_cast<float>(bw);
            entry.height = static_cast<float>(bh);

            if (bw == 0 || bh == 0 || !bm.buffer)
            {
                entry.hasBitmap = false;
                auto [it, _] = glyphCache.emplace(key, entry);
                return &it->second;
            }

            int slotX = 0;
            int slotY = 0;
            uint32_t pageIdx = 0;
            AtlasPage *page = allocateAtlasSlot(renderer, bw, bh, slotX, slotY, pageIdx);
            if (!page)
                return nullptr;

            // Expand grayscale coverage to white RGBA8 so the standard textured shader tints cleanly.
            uploadScratch.resize(static_cast<size_t>(bw) * static_cast<size_t>(bh) * 4u);
            const int pitch = bm.pitch;
            for (int y = 0; y < bh; ++y)
            {
                const uint8_t *srcRow = (pitch >= 0)
                                            ? (bm.buffer + static_cast<size_t>(y) * static_cast<size_t>(pitch))
                                            : (bm.buffer + static_cast<size_t>(bh - 1 - y) * static_cast<size_t>(-pitch));
                uint8_t *dstRow = uploadScratch.data() + static_cast<size_t>(y) * static_cast<size_t>(bw) * 4u;
                for (int x = 0; x < bw; ++x)
                {
                    dstRow[x * 4 + 0] = 255;
                    dstRow[x * 4 + 1] = 255;
                    dstRow[x * 4 + 2] = 255;
                    dstRow[x * 4 + 3] = srcRow[x];
                }
            }

            const int dstX = slotX + kGlyphPadding;
            const int dstY = slotY + kGlyphPadding;
            renderer.updateTexture(
                page->texture,
                Rect{static_cast<float>(dstX), static_cast<float>(dstY),
                     static_cast<float>(bw), static_cast<float>(bh)},
                uploadScratch.data());

            const float invW = 1.0f / static_cast<float>(page->width);
            const float invH = 1.0f / static_cast<float>(page->height);
            entry.pageIndex = pageIdx;
            entry.u0 = static_cast<float>(dstX) * invW;
            entry.v0 = static_cast<float>(dstY) * invH;
            entry.u1 = static_cast<float>(dstX + bw) * invW;
            entry.v1 = static_cast<float>(dstY + bh) * invH;
            entry.hasBitmap = true;

            auto [it, _] = glyphCache.emplace(key, entry);
            return &it->second;
        }
    };

    // =========================================================================
    // TextEngine Public API
    // =========================================================================

    TextEngine::TextEngine() : impl_(std::make_unique<Impl>()) {}
    TextEngine::~TextEngine() = default;
    TextEngine::TextEngine(TextEngine &&) noexcept = default;
    TextEngine &TextEngine::operator=(TextEngine &&) noexcept = default;

    FontHandle TextEngine::loadFont(std::string_view name, std::string_view path)
    {
        if (name.empty() || path.empty())
            return {};
        if (auto it = impl_->fontsByName.find(name); it != impl_->fontsByName.end())
            return FontHandle{it->second};

        std::vector<uint8_t> bytes;
        if (!readBinaryFile(path, bytes))
        {
            logError("TextEngine", "", 0, 0,
                     "failed to read font '" + std::string(name) + "' from '" + std::string(path) + "'");
            return {};
        }
        return loadFontFromMemory(name, bytes.data(), bytes.size());
    }

    FontHandle TextEngine::loadFontFromMemory(std::string_view name, const void *data, size_t size)
    {
        if (!impl_->ftLib || name.empty() || !data || size == 0)
            return {};
        if (auto it = impl_->fontsByName.find(name); it != impl_->fontsByName.end())
            return FontHandle{it->second};

        auto rec = std::make_unique<FontRecord>();
        rec->id = impl_->nextFontId++;
        rec->name = std::string(name);
        rec->data.assign(static_cast<const uint8_t *>(data),
                         static_cast<const uint8_t *>(data) + size);

        if (FT_New_Memory_Face(impl_->ftLib,
                               rec->data.data(),
                               static_cast<FT_Long>(rec->data.size()),
                               0,
                               &rec->ftFace) != 0 ||
            !rec->ftFace)
        {
            logError("TextEngine", "", 0, 0, "FreeType failed to parse font '" + rec->name + "'");
            return {};
        }

        hb_blob_t *blob = hb_blob_create(
            reinterpret_cast<const char *>(rec->data.data()),
            static_cast<unsigned int>(rec->data.size()),
            HB_MEMORY_MODE_READONLY,
            nullptr,
            nullptr);
        rec->hbFace = hb_face_create(blob, 0);
        hb_blob_destroy(blob);

        if (!rec->hbFace)
        {
            destroyFontRecord(*rec);
            logError("TextEngine", "", 0, 0, "HarfBuzz failed to create face for '" + std::string(name) + "'");
            return {};
        }

        rec->hbFont = hb_font_create(rec->hbFace);
        hb_ot_font_set_funcs(rec->hbFont);

        rec->unitsPerEm = rec->ftFace->units_per_EM > 0 ? static_cast<int>(rec->ftFace->units_per_EM) : 1000;
        rec->ascender = static_cast<int>(rec->ftFace->ascender);
        rec->descender = static_cast<int>(rec->ftFace->descender);
        rec->height = static_cast<int>(rec->ftFace->height);

        const uint32_t id = rec->id;
        impl_->fontsByName.emplace(rec->name, id);
        impl_->fontsById.emplace(id, std::move(rec));

        if (impl_->defaultFont.id == 0)
            impl_->defaultFont = FontHandle{id};

        impl_->clearLayoutCache();
        return FontHandle{id};
    }

    void TextEngine::unloadFont(std::string_view name)
    {
        auto it = impl_->fontsByName.find(name);
        if (it == impl_->fontsByName.end())
            return;

        const uint32_t id = it->second;
        impl_->fontsByName.erase(it);

        if (auto fit = impl_->fontsById.find(id); fit != impl_->fontsById.end())
        {
            if (fit->second)
                destroyFontRecord(*fit->second);
            impl_->fontsById.erase(fit);
        }

        if (impl_->defaultFont.id == id)
        {
            impl_->defaultFont = impl_->fontsById.empty()
                                     ? FontHandle{}
                                     : FontHandle{impl_->fontsById.begin()->first};
        }

        // Drop glyph cache entries referencing the unloaded font.
        for (auto git = impl_->glyphCache.begin(); git != impl_->glyphCache.end();)
        {
            if (git->first.fontId == id)
                git = impl_->glyphCache.erase(git);
            else
                ++git;
        }

        impl_->clearLayoutCache();
    }

    FontHandle TextEngine::getFont(std::string_view name) const
    {
        auto it = impl_->fontsByName.find(name);
        return it != impl_->fontsByName.end() ? FontHandle{it->second} : FontHandle{};
    }

    FontHandle TextEngine::resolveFont(const FontStack &stack, FontHandle fallback) const
    {
        for (const auto &name : stack)
        {
            auto it = impl_->fontsByName.find(name);
            if (it != impl_->fontsByName.end())
                return FontHandle{it->second};
        }
        if (fallback.id != 0 && impl_->fontsById.find(fallback.id) != impl_->fontsById.end())
            return fallback;
        return impl_->defaultFont;
    }

    void TextEngine::setDefaultFont(FontHandle h)
    {
        if (impl_->defaultFont.id == h.id)
            return;
        impl_->defaultFont = h;
        impl_->clearLayoutCache();
    }

    FontHandle TextEngine::defaultFont() const
    {
        return impl_->defaultFont;
    }

    FontMetrics TextEngine::fontMetrics(FontHandle font, float fontSize) const
    {
        const uint32_t id = (font.id != 0) ? font.id : impl_->defaultFont.id;
        return impl_->computeMetrics(id, fontSize, 0.0f);
    }

    void TextEngine::setDpiScale(float scale)
    {
        const float clamped = (scale > 0.01f) ? scale : 1.0f;
        if (std::abs(clamped - impl_->dpiScale) <= 0.001f)
            return;
        impl_->dpiScale = clamped;
        impl_->clearLayoutCache();
    }

    float TextEngine::dpiScale() const
    {
        return impl_->dpiScale;
    }

    void TextEngine::setFallbackCharWidth(float width)
    {
        if (width <= 0.0f || std::abs(width - impl_->fallbackCharWidth) <= 1e-4f)
            return;
        impl_->fallbackCharWidth = width;
        impl_->clearLayoutCache();
    }

    float TextEngine::fallbackCharWidth() const
    {
        return impl_->fallbackCharWidth;
    }

    void TextEngine::setLayoutCacheCapacity(size_t maxEntries)
    {
        impl_->layoutCacheCap = maxEntries;
        while (impl_->lruList.size() > impl_->layoutCacheCap)
        {
            impl_->lruMap.erase(impl_->lruList.back().key);
            impl_->lruList.pop_back();
        }
    }

    void TextEngine::clearAtlas(IRenderer &renderer)
    {
        for (auto &page : impl_->atlasPages)
        {
            if (page.texture.valid())
                renderer.destroyTexture(page.texture);
        }
        impl_->atlasPages.clear();
        impl_->glyphCache.clear();
    }

    void TextEngine::clearLayoutCache()
    {
        impl_->clearLayoutCache();
    }

    TextAtlasStats TextEngine::stats() const
    {
        return {
            impl_->atlasPages.size(),
            impl_->glyphCache.size(),
            impl_->lruList.size(),
            impl_->layoutCacheHits,
            impl_->layoutCacheMisses};
    }

    TextLayout TextEngine::layout(std::string_view text,
                                  const TextMeasureSpec &spec,
                                  const TextLayoutOptions &options) const
    {
        return impl_->computeLayout(text, spec, options);
    }

    std::shared_ptr<const TextLayout> TextEngine::layoutCached(std::string_view text,
                                                               const TextMeasureSpec &spec,
                                                               const TextLayoutOptions &options)
    {
        if (impl_->layoutCacheCap == 0)
            return std::make_shared<const TextLayout>(impl_->computeLayout(text, spec, options));

        LayoutCacheKey key;
        key.text.assign(text);
        key.primaryFontId = spec.font.id;
        key.fontStack.assign(spec.fontStack.begin(), spec.fontStack.end());
        key.fontSizeQ = quantizePx(spec.fontSize);
        key.letterSpacingQ = quantizePx(spec.letterSpacing);
        key.lineHeightQ = quantizePx(spec.lineHeight);
        key.maxWidthQ = quantizePx(options.maxWidth);
        key.maxLines = options.maxLines;
        key.flags = static_cast<uint8_t>(
            (options.wrap ? 1u : 0u) |
            (options.ellipsis ? 2u : 0u) |
            (options.preserveSpaces ? 4u : 0u));
        key.ellipsisText = options.ellipsisText;

        if (auto it = impl_->lruMap.find(key); it != impl_->lruMap.end())
        {
            ++impl_->layoutCacheHits;
            impl_->lruList.splice(impl_->lruList.begin(), impl_->lruList, it->second);
            return it->second->value;
        }

        ++impl_->layoutCacheMisses;
        auto computed = std::make_shared<const TextLayout>(impl_->computeLayout(text, spec, options));

        impl_->lruList.push_front({key, computed});
        impl_->lruMap[key] = impl_->lruList.begin();

        while (impl_->lruList.size() > impl_->layoutCacheCap)
        {
            impl_->lruMap.erase(impl_->lruList.back().key);
            impl_->lruList.pop_back();
        }

        return computed;
    }

    Vec2 TextEngine::measureText(std::string_view text, const TextMeasureSpec &spec) const
    {
        return impl_->computeLayout(text, spec, {}).size;
    }

    // ---- Hit-testing, Caret & Selection ----

    size_t TextEngine::lineIndexForByte(const TextLayout &layout, size_t byteIndex)
    {
        if (layout.lines.empty())
            return 0;
        for (size_t i = 0; i < layout.lines.size(); ++i)
        {
            const auto &ln = layout.lines[i];
            const bool isLast = (i + 1 == layout.lines.size());
            if (byteIndex >= ln.byteStart &&
                (byteIndex < ln.byteEnd || (byteIndex == ln.byteEnd && (ln.hardBreak || isLast))))
                return i;
        }
        return layout.lines.size() - 1;
    }

    float TextEngine::caretXOnLine(const TextLine &line, size_t byteIndex)
    {
        if (byteIndex <= line.byteStart)
            return 0.0f;
        if (byteIndex >= line.byteEnd)
            return line.width;

        for (const auto &g : line.glyphs)
        {
            if (g.cluster >= byteIndex)
                return g.x;
        }
        return line.width;
    }

    size_t TextEngine::hitTestLine(std::string_view fullText,
                                   const TextLine &line,
                                   float localX,
                                   bool clampSoftBreak)
    {
        if (line.byteEnd <= line.byteStart || localX <= 0.0f)
            return line.byteStart;

        const size_t maxEditable =
            (clampSoftBreak && !line.hardBreak)
                ? std::max(line.byteStart, Utf8::prevCluster(fullText, line.byteEnd))
                : line.byteEnd;

        if (maxEditable <= line.byteStart)
            return line.byteStart;

        size_t i = 0;
        while (i < line.glyphs.size())
        {
            const uint32_t cluster = line.glyphs[i].cluster;
            if (cluster >= maxEditable)
                return maxEditable;

            const float startX = line.glyphs[i].x;
            float endX = startX;
            while (i < line.glyphs.size() && line.glyphs[i].cluster == cluster)
            {
                endX += line.glyphs[i].xAdvance;
                ++i;
            }

            const float midX = (startX + endX) * 0.5f;
            if (localX < midX)
                return static_cast<size_t>(cluster);
        }

        return maxEditable;
    }

    size_t TextEngine::hitTestPoint(std::string_view fullText,
                                    const TextLayout &layout,
                                    Vec2 localPos,
                                    Align textAlign,
                                    float boxWidth,
                                    bool clampSoftBreak)
    {
        if (layout.lines.empty())
            return 0;

        size_t lineIdx = layout.lines.size() - 1;
        if (localPos.y <= 0.0f)
        {
            lineIdx = 0;
        }
        else
        {
            for (size_t i = 0; i < layout.lines.size(); ++i)
            {
                const auto &ln = layout.lines[i];
                if (localPos.y < ln.yOffset + ln.height)
                {
                    lineIdx = i;
                    break;
                }
            }
        }

        const auto &ln = layout.lines[lineIdx];
        const float effectiveW = (boxWidth > 0.0f) ? boxWidth : layout.size.x;
        const float offX = alignOffsetX(effectiveW, ln.width, textAlign);
        return hitTestLine(fullText, ln, localPos.x - offX, clampSoftBreak);
    }

    Rect TextEngine::caretRect(const TextLayout &layout,
                               size_t byteIndex,
                               float caretWidth,
                               Align textAlign,
                               float boxWidth)
    {
        if (layout.lines.empty())
            return {0.0f, 0.0f, caretWidth, 0.0f};

        const size_t idx = lineIndexForByte(layout, byteIndex);
        const auto &ln = layout.lines[idx];
        const float effectiveW = (boxWidth > 0.0f) ? boxWidth : layout.size.x;
        const float offX = alignOffsetX(effectiveW, ln.width, textAlign);
        const float cx = offX + caretXOnLine(ln, byteIndex);
        return {cx, ln.yOffset, caretWidth, ln.height};
    }

    std::vector<Rect> TextEngine::selectionRects(const TextLayout &layout,
                                                 size_t selStart,
                                                 size_t selEnd,
                                                 Align textAlign,
                                                 float boxWidth)
    {
        std::vector<Rect> rects;
        if (layout.lines.empty() || selStart == selEnd)
            return rects;
        if (selStart > selEnd)
            std::swap(selStart, selEnd);

        const float effectiveW = (boxWidth > 0.0f) ? boxWidth : layout.size.x;
        for (const auto &ln : layout.lines)
        {
            if (selEnd < ln.byteStart || selStart > ln.byteEnd)
                continue;
            if (selStart == ln.byteEnd && selEnd > ln.byteEnd && !ln.hardBreak)
                continue;

            const size_t a = std::max(selStart, ln.byteStart);
            const size_t b = std::min(selEnd, ln.byteEnd);
            if (a == b && selEnd <= ln.byteEnd)
                continue;

            const float offX = alignOffsetX(effectiveW, ln.width, textAlign);
            const float x0 = offX + caretXOnLine(ln, a);
            float x1 = offX + caretXOnLine(ln, b);
            if (selEnd > ln.byteEnd && (x1 - x0) < 4.0f)
                x1 = x0 + 6.0f;

            rects.push_back({x0, ln.yOffset, std::max(0.0f, x1 - x0), ln.height});
        }
        return rects;
    }

    // ---- Batched Rendering ----

    void TextEngine::draw(IRenderer &renderer,
                          const TextLayout &layout,
                          Rect bounds,
                          Color color,
                          const TextMeasureSpec &spec,
                          Align align)
    {
        if (color.a == 0 || layout.lines.empty() || layout.synthetic)
            return;

        const float invDpi = 1.0f / impl_->dpiScale;
        const uint16_t physPx = static_cast<uint16_t>(
            std::clamp(std::lround(std::max(1.0f, spec.fontSize) * impl_->dpiScale), 1L, 512L));

        for (auto &b : impl_->drawBatches)
        {
            b.vertices.clear();
            b.indices.clear();
        }

        const float boxW = (bounds.width > 0.0f) ? bounds.width : layout.size.x;

        for (const auto &ln : layout.lines)
        {
            const float lineX = std::round(bounds.x + alignOffsetX(boxW, ln.width, align));
            const float baselineY = std::round(bounds.y + ln.yOffset + ln.baselineOffset);

            for (const auto &sg : ln.glyphs)
            {
                if (sg.fontId == 0)
                    continue;

                const AtlasGlyph *ag = impl_->ensureGlyph(renderer, GlyphKey{sg.fontId, sg.glyphId, physPx});
                if (!ag || !ag->hasBitmap)
                    continue;

                if (ag->pageIndex >= impl_->drawBatches.size())
                    impl_->drawBatches.resize(static_cast<size_t>(ag->pageIndex) + 1u);

                auto &batch = impl_->drawBatches[ag->pageIndex];

                const float gx = lineX + sg.x + sg.xOffset + ag->bearingX * invDpi;
                const float gy = baselineY - sg.yOffset - ag->bearingY * invDpi;
                const float gw = ag->width * invDpi;
                const float gh = ag->height * invDpi;

                const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
                batch.vertices.push_back(Vertex2D{{gx, gy}, {ag->u0, ag->v0}, color});
                batch.vertices.push_back(Vertex2D{{gx + gw, gy}, {ag->u1, ag->v0}, color});
                batch.vertices.push_back(Vertex2D{{gx + gw, gy + gh}, {ag->u1, ag->v1}, color});
                batch.vertices.push_back(Vertex2D{{gx, gy + gh}, {ag->u0, ag->v1}, color});

                batch.indices.push_back(base + 0);
                batch.indices.push_back(base + 3);
                batch.indices.push_back(base + 2);
                batch.indices.push_back(base + 0);
                batch.indices.push_back(base + 2);
                batch.indices.push_back(base + 1);
            }
        }

        for (size_t pi = 0; pi < impl_->drawBatches.size() && pi < impl_->atlasPages.size(); ++pi)
        {
            const auto &batch = impl_->drawBatches[pi];
            if (!batch.indices.empty())
                renderer.drawGeometry(batch.vertices, batch.indices, impl_->atlasPages[pi].texture);
        }
    }

} // namespace ZenitUI