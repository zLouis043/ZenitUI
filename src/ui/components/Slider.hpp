#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

class Slider : public TLayout<Slider>
{
public:
    Slider(float val = 0.5f) : TLayout<Slider>(LayoutType::Stack), value(val)
    {
        setInteractive(true);
        setFocusable(true);
        setStyleTag("Slider");
    }

    std::function<void(float)> onValueChanged = nullptr;

    float getValue() const { return value; }

protected:
    Vec2 computeIntrinsicSize(float, float) override
    {
        float fs = currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
        return {120.0f, std::max(24.0f, fs * 1.2f)};
    }

    void onUpdate(float) override
    {
        if (isFocused)
        {
            auto &ev = UIContext::get().inputEvents;
            constexpr float STEP = 0.05f;
            constexpr float STEP_FINE = 0.01f;

            for (int k : ev.keys)
            {
                float delta = 0.0f;
                if (k == Key::Left)
                    delta = -STEP;
                if (k == Key::Right)
                    delta = +STEP;
                if (delta == 0.0f)
                    continue;

                if (UIContext::get().shiftHeld)
                    delta *= (STEP_FINE / STEP);
                float nv = std::clamp(value + delta, 0.0f, 1.0f);
                if (nv != value)
                {
                    value = nv;
                    if (onValueChanged)
                        onValueChanged(value);
                }
            }
        }

        if (isPressed && !hasPointerCapture())
        {
            capturePointer();
        }

        if (hasPointerCapture() && rect.width > 0.0f)
        {
            float px = UIContext::get().pointer.pos.x;
            float percent = std::clamp((px - rect.x) / rect.width, 0.0f, 1.0f);
            if (percent != value)
            {
                value = percent;
                if (onValueChanged)
                    onValueChanged(value);
            }
        }
    }

    void renderContent(float op, const ComputedStyle &style) override
    {
        auto r = UIContext::get().renderer;

        Style trackStyle = partStyle("track");
        Style fillStyle = partStyle("fill");
        Style knobStyle = partStyle("knob");

        float th = rect.height * 0.3f;
        Rect trackRect = {rect.x, rect.y + (rect.height - th) * 0.5f, rect.width, th};

        Color trackColor = isEnabled ? Colors::DarkGray : Color{55, 55, 60, 255};
        if (trackStyle.background.is_set)
            trackColor = trackStyle.background.value;

        float trackRadius = trackStyle.radius.is_set
                                ? trackStyle.radius.value.resolveH(th * 0.5f, th * 0.5f)
                                : 4.0f;

        r->fillRoundedRect(trackRect, trackRadius, trackColor.withAlpha(op));

        Color fillColor = isEnabled ? style.color : Color{100, 100, 105, 255};
        if (fillStyle.color.is_set)
            fillColor = fillStyle.color.value;

        Rect fillRect = {trackRect.x, trackRect.y, trackRect.width * value, trackRect.height};
        r->fillRoundedRect(fillRect, trackRadius, fillColor.withAlpha(op));

        Color knobColor = isEnabled ? style.color : Color{130, 130, 135, 255};
        if (knobStyle.color.is_set)
            knobColor = knobStyle.color.value;

        float knobR = knobStyle.radius.is_set
                          ? knobStyle.radius.value.resolveH(rect.height * 0.5f, rect.height * 0.5f)
                          : rect.height * 0.5f;

        r->fillCircle({rect.x + rect.width * value, rect.center().y},
                      knobR, knobColor.withAlpha(op));
    }

private:
    float value;
};

} // namespace ZenitUI::UI