#pragma once

/**
 * \file Utf8.hpp
 * \brief Zero-dependency UTF-8 encoding, decoding, grapheme-cluster navigation,
 *        and Unicode word/input classification utilities.
 *
 * Used by `Text`, `TextInput`, and `TextArea` so that cursor movement,
 * deletion, selection, word-wrapping, and ellipsis truncation always operate
 * on valid UTF-8 codepoint / grapheme-cluster boundaries rather than raw bytes.
 */

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

namespace ZenitUI::Utf8
{

    /** \brief `true` if \p b is a UTF-8 continuation byte (`10xxxxxx`). */
    inline constexpr bool isContinuation(unsigned char b) noexcept
    {
        return (b & 0xC0u) == 0x80u;
    }

    /** \brief `true` if \p cp is a valid Unicode scalar value (excludes UTF-16 surrogates). */
    inline constexpr bool isValidCodepoint(uint32_t cp) noexcept
    {
        return cp <= 0x10FFFFu && !(cp >= 0xD800u && cp <= 0xDFFFu);
    }

    /**
     * \brief Expected UTF-8 byte length from a leading byte (`1..4`).
     *
     * Invalid leading bytes return `1` so callers always make forward progress.
     */
    inline constexpr size_t seqLenFromLead(unsigned char b) noexcept
    {
        if (b < 0x80u)
            return 1;
        if ((b & 0xE0u) == 0xC0u)
            return 2;
        if ((b & 0xF0u) == 0xE0u)
            return 3;
        if ((b & 0xF8u) == 0xF0u)
            return 4;
        return 1;
    }

    /**
     * \brief Append the UTF-8 encoding of \p cp to \p out.
     * \return Number of bytes written (`1..4`), or `0` if \p cp is invalid.
     */
    inline size_t encode(uint32_t cp, std::string &out)
    {
        if (!isValidCodepoint(cp))
            return 0;
        if (cp <= 0x7Fu)
        {
            out.push_back(static_cast<char>(cp));
            return 1;
        }
        if (cp <= 0x7FFu)
        {
            out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
            return 2;
        }
        if (cp <= 0xFFFFu)
        {
            out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
            out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
            return 3;
        }
        out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        return 4;
    }

    /** \brief Encode \p cp into a new UTF-8 `std::string` (empty if invalid). */
    inline std::string encode(uint32_t cp)
    {
        std::string out;
        encode(cp, out);
        return out;
    }

    /**
     * \brief Decode one UTF-8 codepoint starting at byte offset \p pos.
     *
     * On malformed bytes, writes `U+FFFD` (replacement character) and advances
     * by 1 byte so iteration is always safe and terminates.
     *
     * \param s     UTF-8 string view.
     * \param pos   Starting byte offset.
     * \param outCp Receives the decoded Unicode scalar value.
     * \return Byte offset immediately after the decoded sequence.
     */
    inline size_t decode(std::string_view s, size_t pos, uint32_t &outCp) noexcept
    {
        if (pos >= s.size())
        {
            outCp = 0;
            return s.size();
        }

        const unsigned char b0 = static_cast<unsigned char>(s[pos]);
        if (b0 < 0x80u)
        {
            outCp = b0;
            return pos + 1;
        }

        if ((b0 & 0xE0u) == 0xC0u && pos + 1 < s.size())
        {
            const unsigned char b1 = static_cast<unsigned char>(s[pos + 1]);
            if (isContinuation(b1))
            {
                uint32_t cp = ((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu);
                if (cp >= 0x80u)
                {
                    outCp = cp;
                    return pos + 2;
                }
            }
        }
        else if ((b0 & 0xF0u) == 0xE0u && pos + 2 < s.size())
        {
            const unsigned char b1 = static_cast<unsigned char>(s[pos + 1]);
            const unsigned char b2 = static_cast<unsigned char>(s[pos + 2]);
            if (isContinuation(b1) && isContinuation(b2))
            {
                uint32_t cp = ((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu);
                if (cp >= 0x800u && isValidCodepoint(cp))
                {
                    outCp = cp;
                    return pos + 3;
                }
            }
        }
        else if ((b0 & 0xF8u) == 0xF0u && pos + 3 < s.size())
        {
            const unsigned char b1 = static_cast<unsigned char>(s[pos + 1]);
            const unsigned char b2 = static_cast<unsigned char>(s[pos + 2]);
            const unsigned char b3 = static_cast<unsigned char>(s[pos + 3]);
            if (isContinuation(b1) && isContinuation(b2) && isContinuation(b3))
            {
                uint32_t cp = ((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) |
                              ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu);
                if (cp >= 0x10000u && isValidCodepoint(cp))
                {
                    outCp = cp;
                    return pos + 4;
                }
            }
        }

        outCp = 0xFFFDu;
        return pos + 1;
    }

    /** \brief Clamp \p pos to `s.size()` and snap down to the nearest codepoint boundary. */
    inline size_t snapFloor(std::string_view s, size_t pos) noexcept
    {
        if (pos >= s.size())
            return s.size();
        while (pos > 0 && isContinuation(static_cast<unsigned char>(s[pos])))
            --pos;
        return pos;
    }

    /** \brief Clamp \p pos to `s.size()` and snap up to the nearest codepoint boundary. */
    inline size_t snapCeil(std::string_view s, size_t pos) noexcept
    {
        if (pos >= s.size())
            return s.size();
        while (pos < s.size() && isContinuation(static_cast<unsigned char>(s[pos])))
            ++pos;
        return pos;
    }

    /** \brief Advance \p pos by one UTF-8 codepoint. */
    inline size_t nextCodepoint(std::string_view s, size_t pos) noexcept
    {
        if (pos >= s.size())
            return s.size();
        uint32_t cp = 0;
        return decode(s, pos, cp);
    }

    /** \brief Move \p pos back by one UTF-8 codepoint. */
    inline size_t prevCodepoint(std::string_view s, size_t pos) noexcept
    {
        if (pos == 0)
            return 0;
        if (pos > s.size())
            pos = s.size();
        --pos;
        while (pos > 0 && isContinuation(static_cast<unsigned char>(s[pos])))
            --pos;
        return pos;
    }

    /** \brief Count the number of UTF-8 codepoints in \p s. */
    inline size_t codepointCount(std::string_view s) noexcept
    {
        size_t count = 0;
        size_t pos = 0;
        while (pos < s.size())
        {
            pos = nextCodepoint(s, pos);
            ++count;
        }
        return count;
    }

    /**
     * \brief `true` if \p cp extends the preceding grapheme cluster.
     *
     * Covers combining diacritical marks, variation selectors, and emoji
     * skin-tone modifiers.
     */
    inline constexpr bool isClusterExtend(uint32_t cp) noexcept
    {
        return (cp >= 0x0300u && cp <= 0x036Fu) ||   // Combining Diacritical Marks
               (cp >= 0x1AB0u && cp <= 0x1AFFu) ||   // Combining Diacritical Marks Extended
               (cp >= 0x1DC0u && cp <= 0x1DFFu) ||   // Combining Diacritical Marks Supplement
               (cp >= 0x20D0u && cp <= 0x20FFu) ||   // Combining Marks for Symbols
               (cp >= 0xFE00u && cp <= 0xFE0Fu) ||   // Variation Selectors
               (cp >= 0xFE20u && cp <= 0xFE2Fu) ||   // Combining Half Marks
               (cp >= 0x1F3FBu && cp <= 0x1F3FFu) || // Emoji skin-tone modifiers
               (cp >= 0xE0100u && cp <= 0xE01EFu);   // Variation Selectors Supplement
    }

    /** \brief `true` if \p cp is a Regional Indicator symbol (`U+1F1E6..U+1F1FF`). */
    inline constexpr bool isRegionalIndicator(uint32_t cp) noexcept
    {
        return cp >= 0x1F1E6u && cp <= 0x1F1FFu;
    }

    /**
     * \brief Advance \p pos by one grapheme cluster.
     *
     * Keeps `\r\n`, base + combining marks / variation selectors / skin-tone
     * modifiers, Regional Indicator flag pairs, and ZWJ (`U+200D`) emoji
     * sequences intact as single cursor steps.
     */
    inline size_t nextCluster(std::string_view s, size_t pos) noexcept
    {
        if (pos >= s.size())
            return s.size();

        pos = snapFloor(s, pos);
        uint32_t firstCp = 0;
        size_t cur = decode(s, pos, firstCp);

        // CRLF counts as a single cluster.
        if (firstCp == '\r' && cur < s.size() && s[cur] == '\n')
            return cur + 1;

        // Regional Indicator pair (e.g. flag emojis).
        if (isRegionalIndicator(firstCp) && cur < s.size())
        {
            uint32_t nextCp = 0;
            size_t nextPos = decode(s, cur, nextCp);
            if (isRegionalIndicator(nextCp))
                cur = nextPos;
        }

        // Consume trailing combining marks, variation selectors, modifiers, and ZWJ chains.
        while (cur < s.size())
        {
            uint32_t cp = 0;
            size_t after = decode(s, cur, cp);
            if (isClusterExtend(cp))
            {
                cur = after;
                continue;
            }
            if (cp == 0x200Du && after < s.size()) // Zero-Width Joiner
            {
                uint32_t joinedCp = 0;
                size_t afterJoined = decode(s, after, joinedCp);
                cur = afterJoined;
                continue;
            }
            break;
        }
        return cur;
    }

    /**
     * \brief Move \p pos back by one grapheme cluster.
     *
     * Mirror of `nextCluster`: never lands inside a multi-byte UTF-8 sequence,
     * combining mark sequence, ZWJ emoji sequence, Regional Indicator pair, or `\r\n`.
     */
    inline size_t prevCluster(std::string_view s, size_t pos) noexcept
    {
        if (pos == 0)
            return 0;
        if (pos > s.size())
            pos = s.size();

        // If pos was mid-codepoint, snapping down already reaches a valid start < pos.
        if (pos < s.size() && isContinuation(static_cast<unsigned char>(s[pos])))
            return snapFloor(s, pos);

        size_t cur = prevCodepoint(s, pos);
        uint32_t cp = 0;
        decode(s, cur, cp);

        // CRLF as a single cluster.
        if (cp == '\n' && cur > 0 && s[cur - 1] == '\r')
            return cur - 1;

        // Step back over any trailing extenders or ZWJ sequences.
        while (cur > 0)
        {
            if (isClusterExtend(cp))
            {
                cur = prevCodepoint(s, cur);
                decode(s, cur, cp);
                continue;
            }

            // If immediately preceded by a ZWJ, step back over ZWJ + previous base.
            size_t before = prevCodepoint(s, cur);
            uint32_t beforeCp = 0;
            decode(s, before, beforeCp);
            if (beforeCp == 0x200Du && before > 0)
            {
                cur = prevCodepoint(s, before);
                decode(s, cur, cp);
                continue;
            }
            break;
        }

        // Regional Indicator pair: pair from the start of a consecutive RI run.
        if (isRegionalIndicator(cp) && cur > 0)
        {
            size_t runCount = 0;
            size_t scan = cur;
            while (scan > 0)
            {
                size_t p = prevCodepoint(s, scan);
                uint32_t pcp = 0;
                decode(s, p, pcp);
                if (!isRegionalIndicator(pcp))
                    break;
                ++runCount;
                scan = p;
            }
            if ((runCount % 2u) == 1u)
                cur = prevCodepoint(s, cur);
        }

        return cur;
    }

    /**
     * \brief Return all grapheme-cluster byte boundaries in \p s, starting at `0`
     *        and ending at `s.size()`.
     *
     * Ideal for binary searching prefix lengths (wrapping, ellipsis, hit-testing)
     * without ever splitting a UTF-8 codepoint or grapheme cluster.
     */
    inline std::vector<size_t> clusterBoundaries(std::string_view s)
    {
        std::vector<size_t> bounds;
        bounds.reserve(s.size() + 1);
        bounds.push_back(0);
        size_t pos = 0;
        while (pos < s.size())
        {
            pos = nextCluster(s, pos);
            bounds.push_back(pos);
        }
        return bounds;
    }

    /**
     * \brief `true` if \p cp is a printable Unicode scalar value accepted by text inputs.
     *
     * Rejects C0 controls (`< 0x20`), DEL (`0x7F`), C1 controls (`0x80..0x9F`),
     * surrogates, and out-of-range values.
     */
    inline constexpr bool isPrintable(uint32_t cp) noexcept
    {
        if (!isValidCodepoint(cp))
            return false;
        if (cp < 0x20u || (cp >= 0x7Fu && cp <= 0x9Fu))
            return false;
        return true;
    }

    /**
     * \brief `true` if \p cp belongs to a word (for double-click selection and Ctrl+Arrow).
     *
     * Covers ASCII alphanumerics + `_`, accented Latin-1 / Latin Extended letters,
     * combining marks, Greek, Cyrillic, and CJK / Hangul blocks.
     */
    inline constexpr bool isWordChar(uint32_t cp) noexcept
    {
        if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') ||
            (cp >= '0' && cp <= '9') || cp == '_')
            return true;
        if (cp >= 0x00C0u && cp <= 0x00FFu)
            return cp != 0x00D7u && cp != 0x00F7u; // exclude × and ÷
        if (cp >= 0x0100u && cp <= 0x024Fu)
            return true; // Latin Extended-A/B
        if (isClusterExtend(cp))
            return true;
        if ((cp >= 0x0370u && cp <= 0x052Fu) || // Greek, Cyrillic
            (cp >= 0x3040u && cp <= 0x30FFu) || // Hiragana, Katakana
            (cp >= 0x4E00u && cp <= 0x9FFFu) || // CJK Unified Ideographs
            (cp >= 0xAC00u && cp <= 0xD7AFu))   // Hangul Syllables
            return true;
        return false;
    }

    /** \brief Move backward to the start of the previous word (Ctrl+Left / Ctrl+Backspace). */
    inline size_t prevWordBoundary(std::string_view s, size_t pos) noexcept
    {
        pos = snapFloor(s, pos);
        while (pos > 0)
        {
            size_t p = prevCluster(s, pos);
            uint32_t cp = 0;
            decode(s, p, cp);
            if (isWordChar(cp))
                break;
            pos = p;
        }
        while (pos > 0)
        {
            size_t p = prevCluster(s, pos);
            uint32_t cp = 0;
            decode(s, p, cp);
            if (!isWordChar(cp))
                break;
            pos = p;
        }
        return pos;
    }

    /** \brief Move forward past the current/next word (Ctrl+Right / Ctrl+Delete). */
    inline size_t nextWordBoundary(std::string_view s, size_t pos) noexcept
    {
        pos = snapFloor(s, pos);
        while (pos < s.size())
        {
            uint32_t cp = 0;
            decode(s, pos, cp);
            if (!isWordChar(cp))
                break;
            pos = nextCluster(s, pos);
        }
        while (pos < s.size())
        {
            uint32_t cp = 0;
            decode(s, pos, cp);
            if (isWordChar(cp))
                break;
            pos = nextCluster(s, pos);
        }
        return pos;
    }

    /**
     * \brief Sanitize a pasted string for single-line text fields.
     *
     * Keeps all valid printable UTF-8 codepoints and drops control characters
     * (including `\r`, `\n`, `\t`) and malformed byte sequences.
     */
    inline std::string sanitizeSingleLine(std::string_view src)
    {
        std::string out;
        out.reserve(src.size());
        size_t pos = 0;
        while (pos < src.size())
        {
            uint32_t cp = 0;
            size_t next = decode(src, pos, cp);
            if (cp != 0xFFFDu && isPrintable(cp))
                encode(cp, out);
            pos = next;
        }
        return out;
    }

    /**
     * \brief Sanitize a pasted string for multi-line text fields.
     *
     * Normalizes `\r\n` and `\r` to `\n`, preserves all valid printable UTF-8
     * codepoints, and drops other control characters and malformed bytes.
     */
    inline std::string sanitizeMultiLine(std::string_view src)
    {
        std::string out;
        out.reserve(src.size());
        size_t pos = 0;
        while (pos < src.size())
        {
            if (src[pos] == '\r')
            {
                if (pos + 1 < src.size() && src[pos + 1] == '\n')
                    pos += 2;
                else
                    pos += 1;
                out.push_back('\n');
                continue;
            }
            if (src[pos] == '\n')
            {
                out.push_back('\n');
                ++pos;
                continue;
            }
            uint32_t cp = 0;
            size_t next = decode(src, pos, cp);
            if (cp != 0xFFFDu && isPrintable(cp))
                encode(cp, out);
            pos = next;
        }
        return out;
    }

} // namespace ZenitUI::Utf8

namespace ZenitUI
{

    /**
     * \class CodepointSet
     * \brief Composable set of Unicode codepoints for static font-atlas baking.
     *
     * Provides common language/symbol presets (`ascii`, `defaultLatin`,
     * `latinExtendedA`, `greek`, `cyrillic`, `symbols`) plus fluent builders
     * (`addRange`, `addCodepoint`, `addFromUtf8`, `merge`) that produce a sorted,
     * deduplicated `std::vector<int>` suitable for backends like Raylib's
     * `LoadFontEx`.
     */
    class CodepointSet
    {
    public:
        CodepointSet() = default;

        /** \brief Add a single valid Unicode scalar value. */
        CodepointSet &addCodepoint(uint32_t cp)
        {
            if (Utf8::isValidCodepoint(cp))
                cps_.push_back(static_cast<int>(cp));
            return *this;
        }

        /** \brief Add an inclusive range `[first, last]`, skipping UTF-16 surrogates. */
        CodepointSet &addRange(uint32_t first, uint32_t last)
        {
            if (first > last)
                return *this;
            if (last > 0x10FFFFu)
                last = 0x10FFFFu;
            cps_.reserve(cps_.size() + static_cast<size_t>(last - first + 1));
            for (uint32_t cp = first; cp <= last; ++cp)
            {
                if (Utf8::isValidCodepoint(cp))
                    cps_.push_back(static_cast<int>(cp));
            }
            return *this;
        }

        /**
         * \brief Extract and add every valid printable codepoint from a UTF-8 string.
         *
         * Useful to include specific symbols or localized strings directly, e.g.
         * `set.addFromUtf8("★☆♥♦♠♣✓✗→←")`.
         */
        CodepointSet &addFromUtf8(std::string_view text)
        {
            size_t pos = 0;
            while (pos < text.size())
            {
                uint32_t cp = 0;
                size_t next = Utf8::decode(text, pos, cp);
                if (cp != 0xFFFDu && Utf8::isPrintable(cp))
                    cps_.push_back(static_cast<int>(cp));
                pos = next;
            }
            return *this;
        }

        /** \brief Merge all codepoints from \p other into this set. */
        CodepointSet &merge(const CodepointSet &other)
        {
            cps_.insert(cps_.end(), other.cps_.begin(), other.cps_.end());
            return *this;
        }

        /** \brief Remove all codepoints. */
        void clear() { cps_.clear(); }

        /** \brief `true` if no codepoints have been added. */
        bool empty() const { return cps_.empty(); }

        /** \brief Build a sorted, deduplicated codepoint vector for font loading. */
        std::vector<int> build() const
        {
            std::vector<int> out = cps_;
            std::sort(out.begin(), out.end());
            out.erase(std::unique(out.begin(), out.end()), out.end());
            return out;
        }

        // ---- Presets ----

        /** \brief Printable ASCII (`U+0020..U+007E`). */
        static CodepointSet ascii()
        {
            CodepointSet s;
            s.addRange(0x0020u, 0x007Eu);
            return s;
        }

        /** \brief Latin-1 Supplement (`U+00A0..U+00FF`: Western European accents, `°`, `×`, `÷`). */
        static CodepointSet latin1()
        {
            CodepointSet s;
            s.addRange(0x00A0u, 0x00FFu);
            return s;
        }

        /** \brief Latin Extended-A (`U+0100..U+017F`: Central/Eastern European letters, `Œ`, `œ`). */
        static CodepointSet latinExtendedA()
        {
            CodepointSet s;
            s.addRange(0x0100u, 0x017Fu);
            return s;
        }

        /** \brief Common European typography (`– — ‘ ’ “ ” • … € ™`). */
        static CodepointSet typography()
        {
            CodepointSet s;
            for (uint32_t cp : {0x2013u, 0x2014u, 0x2018u, 0x2019u, 0x201Cu,
                                0x201Du, 0x2022u, 0x2026u, 0x20ACu, 0x2122u})
                s.addCodepoint(cp);
            return s;
        }

        /** \brief Default preset: ASCII + Latin-1 + `Œ`/`œ` + common European typography. */
        static CodepointSet defaultLatin()
        {
            return ascii()
                .merge(latin1())
                .addCodepoint(0x0152u)
                .addCodepoint(0x0153u)
                .merge(typography());
        }

        /** \brief Greek and Coptic (`U+0370..U+03FF`). */
        static CodepointSet greek()
        {
            CodepointSet s;
            s.addRange(0x0370u, 0x03FFu);
            return s;
        }

        /** \brief Cyrillic (`U+0400..U+04FF`). */
        static CodepointSet cyrillic()
        {
            CodepointSet s;
            s.addRange(0x0400u, 0x04FFu);
            return s;
        }

        /** \brief Common UI, math, arrow, and card/game symbols (`←↑→↓ ≤≥≠∞ ▲▼◀▶ ★☆ ♠♥♦♣ ✓✗`). */
        static CodepointSet symbols()
        {
            CodepointSet s;
            s.addRange(0x2190u, 0x2199u); // Arrows
            for (uint32_t cp : {0x2212u, 0x221Eu, 0x2248u, 0x2260u, 0x2264u, 0x2265u,
                                0x25B2u, 0x25B6u, 0x25BCu, 0x25C0u,
                                0x2605u, 0x2606u,
                                0x2660u, 0x2663u, 0x2665u, 0x2666u,
                                0x2713u, 0x2717u})
                s.addCodepoint(cp);
            return s;
        }

    private:
        std::vector<int> cps_;
    };

} // namespace ZenitUI