#pragma once

/**
 * \file Slider.hpp
 * \brief A horizontal value slider in `[0, 1]`.
 *
 * Drawn entirely by `renderContent`: a rounded track, a rounded fill, and a
 * circular knob. The three pieces can be styled independently through the
 * `::track`, `::fill` and `::knob` pseudo-elements.
 *
 * Interaction model:
 *  - Pointer: press-drag on the track moves the knob; the node captures the
 *    pointer on the first frame the pointer is held down on it.
 *  - Keyboard: when focused, `Left` / `Right` step by `5%`; `Shift` reduces
 *    the step to `1%`.
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI
{

    /**
     * \class Slider
     * \brief A `[0, 1]` slider with keyboard and pointer control.
     *
     * Style tag: `"Slider"`. Default intrinsic size: `120 x max(24, 1.2*fontSize)`.
     * Parts: `::track`, `::fill`, `::knob`.
     */
    class Slider : public TLayout<Slider>
    {
    public:
        /**
         * \brief Construct with an initial value.
         * \param val Initial value; clamped to `[0, 1]` on construction.
         */
        Slider(float val = 0.5f) : TLayout<Slider>(LayoutType::Stack), value(val)
        {
            setInteractive(true);
            setFocusable(true);
            setStyleTag("Slider");
        }

        /** \brief Fired whenever the value changes, with the new value in `[0, 1]`. */
        std::function<void(float)> onValueChanged = nullptr;

        /** \brief Current value in `[0, 1]`. */
        float getValue() const { return value; }

        /**
         * \brief Set the value and optionally fire `onValueChanged`.
         * \param v      New value; clamped to `[0, 1]`.
         * \param notify When `true` (default), fires `onValueChanged` if the
         *               value actually changed.
         *
         * Setting the same value is a no-op: no callback, no pending
         * transition. This makes binding idempotent.
         */
        void setValue(float v, bool notify = true)
        {
            float clamped = std::clamp(v, 0.0f, 1.0f);
            if (clamped == value)
                return;
            value = clamped;
            internal_.pendingTransition = 1;
            if (notify && onValueChanged)
                onValueChanged(value);
        }

    protected:
        /**
         * \brief Default intrinsic size: `120 x max(24, 1.2*fontSize)`.
         */
        Vec2 computeIntrinsicSize(float, float) override
        {
            float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            return {120.0f, std::max(24.0f, fs * 1.2f)};
        }

        /**
         * \brief Handle keyboard stepping and pointer dragging.
         *
         * Keyboard handling runs whenever the node has focus (regardless of
         * pointer state); pointer handling captures the pointer on first
         * `state.pressed` and releases on button-up (which happens implicitly
         * in `Layout::updateTree`).
         */
        void onUpdate(float) override
        {
            if (state.focused)
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

                    if (UIContext::get().modifiers.shift)
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

            if (state.pressed && !hasPointerCapture())
            {
                capturePointer();
            }

            if (hasPointerCapture() && rect.width > 0.0f)
            {
                float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height);
                float bwr = style_.currentStyle.borderWidth.right.resolveH(rect.width, rect.height);
                float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height) + bwl;
                float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height) + bwr;

                float innerX = rect.x + pl;
                float innerW = std::max(0.0f, rect.width - pl - pr);

                if (innerW > 0.0f)
                {
                    float px = UIContext::get().pointer.pos.x;
                    float percent = std::clamp((px - innerX) / innerW, 0.0f, 1.0f);
                    if (percent != value)
                    {
                        value = percent;
                        if (onValueChanged)
                            onValueChanged(value);
                    }
                }
            }
        }

        /**
         * \brief Draw the track, fill and knob.
         *
         * All three pieces honor their respective `::part` overrides. The
         * knob is drawn on top of the fill and the track.
         */
        void renderContent(float op, const ComputedStyle &style) override
        {
            auto r = UIContext::get().renderer;
            Style trackStyle = partStyle("track");
            Style fillStyle = partStyle("fill");
            Style knobStyle = partStyle("knob");

            float bwl = style.borderWidth.left.resolveH(rect.width, rect.height);
            float bwr = style.borderWidth.right.resolveH(rect.width, rect.height);
            float bwt = style.borderWidth.top.resolveV(rect.width, rect.height);
            float bwb = style.borderWidth.bottom.resolveV(rect.width, rect.height);

            float pl = style.padding.left.resolveH(rect.width, rect.height) + bwl;
            float pr = style.padding.right.resolveH(rect.width, rect.height) + bwr;
            float pt = style.padding.top.resolveV(rect.width, rect.height) + bwt;
            float pb = style.padding.bottom.resolveV(rect.width, rect.height) + bwb;

            float innerX = rect.x + pl;
            float innerY = rect.y + pt;
            float innerW = std::max(0.0f, rect.width - pl - pr);
            float innerH = std::max(0.0f, rect.height - pt - pb);

            float th = innerH * 0.3f;
            Rect trackRect = {innerX, innerY + (innerH - th) * 0.5f, innerW, th};

            Color trackColor = config.enabled ? Colors::DarkGray : Color{55, 55, 60, 255};
            if (trackStyle.background.is_set)
                trackColor = trackStyle.background.value;
            float trackRadius = trackStyle.radius.is_set
                                    ? trackStyle.radius.value.resolveH(th * 0.5f, th * 0.5f)
                                    : 4.0f;
            r->fillRoundedRect(trackRect, trackRadius, trackColor.withAlpha(op));

            Color fillColor = config.enabled ? style.color : Color{100, 100, 105, 255};
            if (fillStyle.color.is_set)
                fillColor = fillStyle.color.value;
            Rect fillRect = {trackRect.x, trackRect.y, trackRect.width * value, trackRect.height};
            r->fillRoundedRect(fillRect, trackRadius, fillColor.withAlpha(op));

            Color knobColor = config.enabled ? style.color : Color{130, 130, 135, 255};
            if (knobStyle.color.is_set)
                knobColor = knobStyle.color.value;
            float knobR = knobStyle.radius.is_set
                              ? knobStyle.radius.value.resolveH(innerH * 0.5f, innerH * 0.5f)
                              : innerH * 0.5f;
            r->fillCircle({innerX + innerW * value, innerY + innerH * 0.5f},
                          knobR, knobColor.withAlpha(op));
        }

    private:
        float value;
    };

} // namespace ZenitUI::UI