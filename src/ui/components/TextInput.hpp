#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

class TextInput : public TLayout<TextInput>
{
public:
    TextInput(std::string initial = "")
        : TLayout<TextInput>(LayoutType::Stack), text(std::move(initial))
    {
        setInteractive(true);
        setFocusable(true);
        setStyleTag("TextInput");
        cursorPos = (int)text.size();

        onClick = [this]()
        {
            if (!isEnabled)
                return;
            UIContext::get().requestFocus(shared_from_this());
        };

        style_.inlineBase.background = Color{30, 30, 36, 255};
        style_.inlineBase.color = Colors::White;
        style_.inlineBase.radius = Px(6.0f);
        style_.inlineBase.padding = Spacing(VH(1.2f), VW(1.5f));
        style_.inlineBase.borderColor = Color{60, 60, 70, 255};
        style_.inlineBase.borderWidth = Px(1.0f);
        pendingTransition = true;
    }

    void setText(std::string t)
    {
        text = std::move(t);
        cursorPos = std::min(cursorPos, (int)text.size());
        text_dirty = true;
        pendingTransition = true;
    }
    const std::string &getText() const { return text; }
    void clear() { setText(""); }

    void setFont(FontHandle f)
    {
        defaultFont = f;
        pendingTransition = true;
    }

    std::function<void(const std::string &)> onTextChanged;
    std::function<void(const std::string &)> onSubmit;

protected:
    Vec2 computeIntrinsicSize(float, float) override
    {
        FontHandle f = resolveFont(style_.currentStyle, defaultFont);
        float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);

        float charW = 0.0f;
        if (auto r = UIContext::get().renderer)
        {
            charW = r->measureText(f, "M", fs, 0.0f).x;
        }
        if (charW <= 0.0f)
            charW = fs * 0.6f;

        return {std::max(120.0f, charW * 12.0f), fs * 1.6f};
    }

    void onEnabledChanged(bool nowEnabled) override
    {
        if (!nowEnabled)
        {
            hasFocusCache = false;
            repeatStates.clear();
            cursorBlink = 0.0f;
        }
    }

    void onUpdate(float dt) override
    {
        hasFocusCache = UIContext::get().hasFocus(this);
        cursorBlink += dt;
        if (cursorBlink > 1.0f)
            cursorBlink -= 1.0f;

        if (!hasFocusCache)
        {
            repeatStates.clear();
            return;
        }

        auto &ev = UIContext::get().inputEvents;
        bool changed = false;

        for (int c : ev.chars)
        {
            if (c < 32 || c > 126)
                continue;
            text.insert(text.begin() + cursorPos, (char)c);
            cursorPos++;
            changed = true;
        }

        auto doAction = [&](int key)
        {
            switch (key)
            {
            case Key::Backspace:
                if (cursorPos > 0)
                {
                    text.erase(text.begin() + cursorPos - 1);
                    cursorPos--;
                    changed = true;
                }
                break;
            case Key::Delete:
                if (cursorPos < (int)text.size())
                {
                    text.erase(text.begin() + cursorPos);
                    changed = true;
                }
                break;
            case Key::Left:
                if (cursorPos > 0)
                {
                    cursorPos--;
                    changed = true;
                }
                break;
            case Key::Right:
                if (cursorPos < (int)text.size())
                {
                    cursorPos++;
                    changed = true;
                }
                break;
            }
        };

        for (int k : ev.keys)
        {
            switch (k)
            {
            case Key::Enter:
                if (onSubmit)
                    onSubmit(text);
                UIContext::get().releaseFocus();
                hasFocusCache = false;
                repeatStates.clear();
                return;
            case Key::Escape:
                UIContext::get().releaseFocus();
                hasFocusCache = false;
                repeatStates.clear();
                return;
            case Key::Home:
                cursorPos = 0;
                changed = true;
                break;
            case Key::End:
                cursorPos = (int)text.size();
                changed = true;
                break;
            default:
                break;
            }
        }

        constexpr float REPEAT_DELAY = 0.40f;
        constexpr float REPEAT_INTERVAL = 0.04f;

        auto handleRepeat = [&](int key)
        {
            bool isHeld = std::find(ev.held.begin(), ev.held.end(), key) != ev.held.end();
            auto it = repeatStates.find(key);

            if (!isHeld)
            {
                if (it != repeatStates.end())
                    repeatStates.erase(it);
                return;
            }
            if (it == repeatStates.end())
            {
                doAction(key);
                RepeatState st;
                st.holdTime = 0.0f;
                st.nextFireTime = REPEAT_DELAY;
                repeatStates[key] = st;
            }
            else
            {
                it->second.holdTime += dt;
                while (it->second.holdTime >= it->second.nextFireTime)
                {
                    doAction(key);
                    it->second.nextFireTime += REPEAT_INTERVAL;
                }
            }
        };

        handleRepeat(Key::Backspace);
        handleRepeat(Key::Delete);
        handleRepeat(Key::Left);
        handleRepeat(Key::Right);

        if (changed)
        {
            if (onTextChanged)
                onTextChanged(text);
            pendingTransition = true;
        }

        {
            auto r = UIContext::get().renderer;
            if (!r)
                return;

            FontHandle f = resolveFont(style_.currentStyle, defaultFont);
            float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            float sp = style_.currentStyle.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);
            float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height);
            float availW = std::max(0.0f, rect.width - pl * 2.0f);

            std::string prefix = text.substr(0, cursorPos);
            Vec2 prefixSize = r->measureText(f, prefix, fs, sp);
            Vec2 fullSize = r->measureText(f, text, fs, sp);

            if (prefixSize.x - scrollX > availW)
                scrollX = prefixSize.x - availW;
            if (prefixSize.x - scrollX < 0.0f)
                scrollX = prefixSize.x;

            float maxScroll = std::max(0.0f, fullSize.x - availW);
            scrollX = std::clamp(scrollX, 0.0f, maxScroll);
        }
    }

    void renderContent(float op, const ComputedStyle &style) override
    {
        auto r = UIContext::get().renderer;
        FontHandle f = resolveFont(style, defaultFont);
        float fs = style.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
        float sp = style.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);
        float pl = style.padding.left.resolveH(rect.width, rect.height);

        r->pushClip(rect);

        Vec2 pos = {rect.x + pl - scrollX, rect.y + (rect.height - fs) * 0.5f - fs * 0.1f};

        Color textColor = isEnabled ? style.color : Color{120, 120, 125, 255};
        r->drawText(f, text, pos, fs, sp, textColor.withAlpha(op));

        if (hasFocusCache && isEnabled && cursorBlink < 0.5f)
        {
            Style cursorStyle = partStyle("cursor");
            Color cursorColor = cursorStyle.color.is_set
                                    ? cursorStyle.color.value
                                    : textColor;

            float cursorW = cursorStyle.borderWidth.is_set
                                ? cursorStyle.borderWidth.value.resolveH(fs, fs)
                                : 2.0f;

            std::string prefix = text.substr(0, cursorPos);
            Vec2 prefixSize = r->measureText(f, prefix, fs, sp);
            float cx = pos.x + prefixSize.x;
            float cy = pos.y;
            float ch = fs;
            r->fillRect({cx, cy, cursorW, ch}, cursorColor.withAlpha(op));
        }

        r->popClip();
    }

private:
    std::string text;
    int cursorPos{0};
    bool text_dirty{true};
    bool hasFocusCache{false};
    float cursorBlink{0.0f};
    float scrollX{0.0f};

    FontHandle defaultFont{0};

    struct RepeatState
    {
        float holdTime{0.0f};
        float nextFireTime{0.0f};
    };
    std::unordered_map<int, RepeatState> repeatStates;
};

} // namespace ZenitUI::UI