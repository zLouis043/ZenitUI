#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

class Checkbox : public TLayout<Checkbox>
{
public:
    Checkbox(bool state = false) : TLayout<Checkbox>(LayoutType::Stack), isChecked(state)
    {
        setInteractive(true);
        setFocusable(true);
        setKeyboardActivates(true);
        setStyleTag("Checkbox");

        getInlineBase().background = Colors::DarkGray;
        getInlineBase().radius = Px(6.0f);

        onClick = [this]()
        {
            isChecked = !isChecked;
            if (onToggle)
                onToggle(isChecked);
        };
    }

    std::function<void(bool)> onToggle = nullptr;
    bool isChecked;

protected:
    Vec2 computeIntrinsicSize(float, float) override
    {
        float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
        return {fs * 1.4f, fs * 1.4f};
    }

    void renderContent(float op, const ComputedStyle &style) override
    {
        auto r = UIContext::get().renderer;

        Style boxStyle = partStyle("box");
        Style markStyle = partStyle("mark");

        Color box = isChecked ? Colors::Green : style.background;
        if (!isEnabled)
            box = Color{70, 70, 75, 255};
        if (boxStyle.background.is_set)
            box = boxStyle.background.value;

        float maxR = std::min(rect.width, rect.height) * 0.5f;
        float rPx = boxStyle.radius.is_set
                        ? std::clamp(boxStyle.radius.value.resolveH(maxR * 2.0f, maxR * 2.0f), 0.0f, maxR)
                        : std::clamp(style.radius.resolveH(maxR * 2.0f, maxR * 2.0f), 0.0f, maxR);

        r->fillRoundedRect(rect, rPx, box.withAlpha(op));

        if (isChecked)
        {
            Color mark = Colors::White;
            if (!isEnabled)
                mark = Color{150, 150, 155, 255};
            if (markStyle.color.is_set)
                mark = markStyle.color.value;

            float inner = rect.height * 0.35f;
            Rect m = {
                rect.center().x - inner * 0.5f,
                rect.center().y - inner * 0.5f,
                inner, inner};
            r->fillRoundedRect(m, 3.0f, mark.withAlpha(op));
        }
    }
};

} // namespace ZenitUI::UI