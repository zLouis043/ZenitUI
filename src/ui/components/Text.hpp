#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

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
        pendingTransition = true;
    }

    void setFont(FontHandle f)
    {
        defaultFont = f;
        text_dirty = true;
        pendingTransition = true;
    }

    void setWrap(bool w)
    {
        wrap_ = w;
        text_dirty = true;
        pendingTransition = true;
    }
    bool getWrap() const { return wrap_; }

protected:
    Vec2 computeIntrinsicSize(float availW, float /*availH*/) override
    {
        FontHandle f = resolveFont(style_.currentStyle, defaultFont);
        if (wrap_ && availW > 0.0f)
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

        float pl = style.padding.left.resolveH(rect.width, rect.height);
        float pr = style.padding.right.resolveH(rect.width, rect.height);
        float pt = style.padding.top.resolveV(rect.width, rect.height);
        float pb = style.padding.bottom.resolveV(rect.width, rect.height);

        float availW = std::max(0.0f, rect.width - pl - pr);
        float availH = std::max(0.0f, rect.height - pt - pb);

        float totalTextH = 0.0f;
        for (auto &ln : lines)
            totalTextH += ln.size.y;

        Vec2 pos = {rect.x + pl, rect.y + pt};
        Align ha = (style.textAlign == Align::Auto) ? style.itemsH : style.textAlign;

        if (style.itemsV == Align::Center)
            pos.y += (availH - totalTextH) * 0.5f;
        else if (style.itemsV == Align::End)
            pos.y += availH - totalTextH;

        auto r = UIContext::get().renderer;
        r->pushClip(rect);
        float curY = pos.y;
        for (auto &ln : lines)
        {
            float x = pos.x;
            if (ha == Align::Center)
                x += (availW - ln.size.x) * 0.5f;
            else if (ha == Align::End)
                x += availW - ln.size.x;
            r->drawText(f, ln.text, {x, curY}, fs, sp, style.color.withAlpha(op));
            curY += ln.size.y;
        }
        r->popClip();
    }

    void onLayout() override
    {
        if (!wrap_)
            return;

        float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height);
        float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height);
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

        std::string cur;
        std::istringstream iss(text);
        std::string word;

        auto flush = [&]()
        {
            if (cur.empty())
                return;
            Vec2 sz = r->measureText(f, cur, fs, sp);
            lines.push_back({cur, sz});
            cur.clear();
        };

        while (iss >> word)
        {
            std::string candidate = cur.empty() ? word : (cur + " " + word);
            Vec2 sz = r->measureText(f, candidate, fs, sp);
            if (sz.x <= availW || cur.empty())
            {
                cur = candidate;
            }
            else
            {
                flush();
                cur = word;
            }
        }
        flush();
        if (lines.empty())
        {
            Vec2 sz = r->measureText(f, text, fs, sp);
            lines.push_back({text, sz});
        }
    }

    Vec2 measureSingleLine(FontHandle f)
    {
        float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
        float sp = style_.currentStyle.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);

        if (text_dirty || f.id != cachedFontId ||
            fs != cachedFs || sp != cachedSp || cachedSize.y == 0.0f)
        {
            cachedSize = UIContext::get().renderer->measureText(f, text, fs, sp);
            cachedFs = fs;
            cachedSp = sp;
            cachedFontId = f.id;
            text_dirty = false;
        }
        lines.clear();
        lines.push_back({text, cachedSize});
        return cachedSize;
    }

    Vec2 measureWrapped(float availW, FontHandle f)
    {
        float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
        float sp = style_.currentStyle.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);

        if (text_dirty || f.id != cachedFontId ||
            fs != cachedFs || sp != cachedSp || availW != cachedWrapW)
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