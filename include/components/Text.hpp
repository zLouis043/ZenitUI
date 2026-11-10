#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI
{

    class Text : public TLayout<Text>
    {
    public:
        Text(std::string text) : TLayout<Text>(LayoutType::Stack), text(std::move(text))
        {
            setInteractive(false);
            setStyleTag("Text");
        }

        void setText(std::string new_text)
        {
            text = std::move(new_text);
            text_dirty = true;
            internal_.pendingTransition = 1;
        }

        const std::string &getText() const { return text; }

        void setFont(FontHandle f)
        {
            defaultFont = f;
            text_dirty = true;
            internal_.pendingTransition = 1;
        }

        void setWrap(bool w)
        {
            wrap_ = w;
            text_dirty = true;
            internal_.pendingTransition = 1;
        }
        bool getWrap() const { return wrap_; }

    protected:
        Vec2 computeIntrinsicSize(float availW, float /*availH*/) override
        {
            FontHandle f = resolveFont(style_.currentStyle, defaultFont);
            const bool effectiveWrap =
                wrap_ && style_.currentStyle.whiteSpace != WhiteSpace::Nowrap;
            if (effectiveWrap && availW > 0.0f)
                return measureWrapped(availW, f);
            return measureSingleLine(f);
        }

        void renderContent(float op, const ComputedStyle &style) override
        {
            if (lines.empty())
                return;

            FontHandle f = resolveFont(style, defaultFont);

            float fs = style.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            float sp = style.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);

            float bwl = style.borderWidth.left.resolveH(rect.width, rect.height);
            float bwr = style.borderWidth.right.resolveH(rect.width, rect.height);
            float bwt = style.borderWidth.top.resolveV(rect.width, rect.height);
            float bwb = style.borderWidth.bottom.resolveV(rect.width, rect.height);

            float pl = style.padding.left.resolveH(rect.width, rect.height) + bwl;
            float pr = style.padding.right.resolveH(rect.width, rect.height) + bwr;
            float pt = style.padding.top.resolveV(rect.width, rect.height) + bwt;
            float pb = style.padding.bottom.resolveV(rect.width, rect.height) + bwb;

            float availW = std::max(0.0f, rect.width - pl - pr);
            float availH = std::max(0.0f, rect.height - pt - pb);

            // Ellipsis: only meaningful for single-line text (wrap disabled or
            // white-space: nowrap) that overflows the available width.
            const bool isSingleLine = lines.size() == 1;
            const bool applyEllipsis =
                isSingleLine &&
                style.textOverflow == TextOverflow::Ellipsis &&
                lines[0].size.x > availW;

            std::string truncatedText;
            Vec2 truncatedSize = lines[0].size;
            if (applyEllipsis)
            {
                truncatedText = buildEllipsisText(lines[0].text, availW, f, fs, sp);
                truncatedSize = UIContext::get().renderer->measureText(
                    f, truncatedText, fs, sp);
            }

            float totalTextH = 0.0f;
            if (applyEllipsis)
                totalTextH = truncatedSize.y;
            else
                for (auto &ln : lines)
                    totalTextH += ln.size.y;

            Vec2 pos = {rect.x + pl, rect.y + pt};
            Align ha = (style.textAlign == Align::Auto) ? style.alignItems : style.textAlign;

            if (style.verticalAlign == Align::Center)
                pos.y += (availH - totalTextH) * 0.5f;
            else if (style.verticalAlign == Align::End)
                pos.y += availH - totalTextH;

            auto r = UIContext::get().renderer;
            r->pushClip(rect);

            if (applyEllipsis)
            {
                float x = pos.x;
                if (ha == Align::Center)
                    x += (availW - truncatedSize.x) * 0.5f;
                else if (ha == Align::End)
                    x += availW - truncatedSize.x;
                r->drawText(f, truncatedText, {x, pos.y}, fs, sp,
                            style.color.withAlpha(op));
            }
            else
            {
                float curY = pos.y;
                for (auto &ln : lines)
                {
                    float x = pos.x;
                    if (ha == Align::Center)
                        x += (availW - ln.size.x) * 0.5f;
                    else if (ha == Align::End)
                        x += availW - ln.size.x;
                    r->drawText(f, ln.text, {x, curY}, fs, sp,
                                style.color.withAlpha(op));
                    curY += ln.size.y;
                }
            }

            r->popClip();
        }

        void onLayout() override
        {
            if (!wrap_ || style_.currentStyle.whiteSpace == WhiteSpace::Nowrap)
                return;

            float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height);
            float bwr = style_.currentStyle.borderWidth.right.resolveH(rect.width, rect.height);
            float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height) + bwl;
            float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height) + bwr;
            float newAvailW = std::max(0.0f, rect.width - pl - pr);

            if (newAvailW > 0.0f && newAvailW != cachedWrapW)
            {
                FontHandle f = resolveFont(style_.currentStyle, defaultFont);
                float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
                float sp = style_.currentStyle.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);

                wrapText(newAvailW, fs, sp, f);
                cachedFs = fs;
                cachedSp = sp;
                cachedWrapW = newAvailW;
                cachedFontId = f.id;

                float maxW = 0.0f, totalH = 0.0f;
                for (auto &ln : lines)
                {
                    maxW = std::max(maxW, ln.size.x);
                    totalH += ln.size.y;
                }
                cachedSize = {maxW, totalH};
            }
        }

    private:
        struct Line
        {
            std::string text;
            Vec2 size;
        };

        void wrapText(float availW, float fs, float sp, FontHandle f)
        {
            lines.clear();
            auto r = UIContext::get().renderer;
            if (!r)
                return;

            auto measure = [&](std::string_view s) -> Vec2
            {
                return r->measureText(f, s, fs, sp);
            };

            // Append `word` to `cur`, or flush and start a new line, or break the
            // word into chunks when it is wider than `availW`. No recursion.
            auto emitWord = [&](std::string_view word, std::string &cur)
            {
                if (word.empty())
                    return;

                // Fast path: try appending " word" to the current line.
                if (!cur.empty())
                {
                    std::string candidate = cur + " " + std::string(word);
                    if (measure(candidate).x <= availW)
                    {
                        cur = std::move(candidate);
                        return;
                    }
                    // Does not fit: flush and fall through with cur cleared.
                    Vec2 s = measure(cur);
                    lines.push_back({cur, s});
                    cur.clear();
                }

                // Fast path: word fits on an empty line.
                if (measure(word).x <= availW)
                {
                    cur.assign(word);
                    return;
                }

                // Slow path: break the word into chunks that fit.
                std::string_view remaining = word;
                while (!remaining.empty())
                {
                    // Longest prefix that still fits in availW (binary search).
                    size_t lo = 1;
                    size_t hi = remaining.size();
                    size_t best = 0;
                    while (lo <= hi)
                    {
                        size_t mid = (lo + hi) / 2;
                        if (measure(remaining.substr(0, mid)).x <= availW)
                        {
                            best = mid;
                            lo = mid + 1;
                        }
                        else
                        {
                            hi = mid - 1;
                        }
                    }
                    if (best == 0)
                        best = 1; // always make progress

                    if (best >= remaining.size())
                    {
                        // The whole remaining tail fits: park it in cur so the
                        // next word can try to continue on this line.
                        cur.assign(remaining);
                        return;
                    }

                    std::string chunk(remaining.substr(0, best));
                    Vec2 s = measure(chunk);
                    lines.push_back({chunk, s});
                    remaining.remove_prefix(best);
                }
            };

            // Split the text on explicit '\n' into logical lines, then wrap each one.
            size_t lineStart = 0;
            while (lineStart <= text.size())
            {
                size_t lineEnd = text.find('\n', lineStart);
                std::string_view logicalLine = (lineEnd == std::string::npos)
                                                   ? std::string_view(text).substr(lineStart)
                                                   : std::string_view(text).substr(lineStart, lineEnd - lineStart);

                std::string cur;
                size_t i = 0;
                while (i < logicalLine.size())
                {
                    while (i < logicalLine.size() && logicalLine[i] == ' ')
                        ++i;
                    if (i >= logicalLine.size())
                        break;

                    size_t wordStart = i;
                    while (i < logicalLine.size() && logicalLine[i] != ' ')
                        ++i;
                    emitWord(logicalLine.substr(wordStart, i - wordStart), cur);
                }

                if (!cur.empty())
                {
                    Vec2 s = measure(cur);
                    lines.push_back({cur, s});
                }
                else if (logicalLine.empty())
                {
                    // Preserve empty lines produced by consecutive '\n'.
                    lines.push_back({"", measure("")});
                }

                if (lineEnd == std::string::npos)
                    break;
                lineStart = lineEnd + 1;
            }

            if (lines.empty())
            {
                Vec2 sz = measure(text);
                lines.push_back({text, sz});
            }
        }

        std::string buildEllipsisText(std::string_view src, float availW,
                                      FontHandle f, float fs, float sp)
        {
            auto r = UIContext::get().renderer;
            if (!r)
                return std::string(src);

            // If even the bare ellipsis does not fit, shrink it.
            Vec2 ellipsisOnly = r->measureText(f, "...", fs, sp);
            if (ellipsisOnly.x > availW)
            {
                if (r->measureText(f, "..", fs, sp).x <= availW)
                    return "..";
                if (r->measureText(f, ".", fs, sp).x <= availW)
                    return ".";
                return "";
            }

            // Longest prefix such that (prefix + "...") fits in availW.
            size_t lo = 0, hi = src.size(), best = 0;
            while (lo <= hi)
            {
                size_t mid = (lo + hi) / 2;
                std::string candidate = std::string(src.substr(0, mid)) + "...";
                if (r->measureText(f, candidate, fs, sp).x <= availW)
                {
                    best = mid;
                    lo = mid + 1;
                }
                else
                {
                    hi = mid - 1;
                }
            }
            return std::string(src.substr(0, best)) + "...";
        }

        Vec2 measureSingleLine(FontHandle f)
        {
            float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            float sp = style_.currentStyle.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);

            if (text_dirty || f.id != cachedFontId ||
                fs != cachedFs || sp != cachedSp || cachedSize.y == 0.0f ||
                cachedWrapW >= 0.0f)
            {
                auto r = UIContext::get().renderer;
                lines.clear();

                // Split on explicit '\n' even without wrapping. This makes
                // '\n' produce a hard line break in every mode (wrap:true or
                // wrap:false), matching CSS.
                size_t lineStart = 0;
                while (lineStart <= text.size())
                {
                    size_t lineEnd = text.find('\n', lineStart);
                    std::string_view part = (lineEnd == std::string::npos)
                                                ? std::string_view(text).substr(lineStart)
                                                : std::string_view(text).substr(lineStart, lineEnd - lineStart);

                    Vec2 sz = r->measureText(f, part, fs, sp);
                    lines.push_back({std::string(part), sz});

                    if (lineEnd == std::string::npos)
                        break;
                    lineStart = lineEnd + 1;
                }

                float maxW = 0.0f, totalH = 0.0f;
                for (auto &ln : lines)
                {
                    maxW = std::max(maxW, ln.size.x);
                    totalH += ln.size.y;
                }
                cachedSize = {maxW, totalH};
                cachedFs = fs;
                cachedSp = sp;
                cachedFontId = f.id;
                cachedWrapW = -1.0f;
                text_dirty = false;
            }
            return cachedSize;
        }

        Vec2 measureWrapped(float availW, FontHandle f)
        {
            float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            float sp = style_.currentStyle.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);

            if (text_dirty || f.id != cachedFontId ||
                fs != cachedFs || sp != cachedSp || availW != cachedWrapW ||
                cachedWrapW < 0.0f)
            {
                wrapText(availW, fs, sp, f);
                cachedFs = fs;
                cachedSp = sp;
                cachedWrapW = availW;
                cachedFontId = f.id;
                text_dirty = false;
            }

            float maxW = 0.0f, totalH = 0.0f;
            for (auto &ln : lines)
            {
                maxW = std::max(maxW, ln.size.x);
                totalH += ln.size.y;
            }
            cachedSize = {maxW, totalH};
            return cachedSize;
        }

        std::string text;
        FontHandle defaultFont{0};
        bool text_dirty{true};
        bool wrap_{false};
        Vec2 cachedSize;
        float cachedFs{-1.0f};
        float cachedSp{-1.0f};
        float cachedWrapW{-1.0f};
        uint32_t cachedFontId{0};
        std::vector<Line> lines;
    };
    inline std::shared_ptr<Text> Label(std::string t) { return Text::create(std::move(t)); }

} // namespace ZenitUI::UI