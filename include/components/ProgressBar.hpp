#pragma once

/**
 * \file ProgressBar.hpp
 * \brief A horizontal progress bar.
 *
 * The visual is drawn directly by `renderContent`: a rounded track plus a
 * rounded fill whose width is `innerW * value`. Theme rules can override
 * both through the `::track` and `::fill` pseudo-elements.
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI
{

    /**
     * \class ProgressBar
     * \brief A value-in-`[0, 1]` progress indicator.
     *
     * Style tag: `"ProgressBar"`. Default intrinsic size: `120 x max(16, 0.8*fontSize)`.
     * Not interactive. Parts: `::track` (background), `::fill` (foreground).
     */
    class ProgressBar : public TLayout<ProgressBar>
    {
    public:
        /**
         * \brief Construct with an initial value, clamped to `[0, 1]`.
         */
        ProgressBar(float v = 0.0f) : TLayout<ProgressBar>(LayoutType::Stack), value(std::clamp(v, 0.0f, 1.0f))
        {
            setInteractive(false);
            setStyleTag("ProgressBar");
            getInlineBase().background = Colors::DarkGray;
            getInlineBase().radius = Px(4.0f);
        }

        /** \brief Set the value, clamped to `[0, 1]`. */
        void setValue(float v)
        {
            value = std::clamp(v, 0.0f, 1.0f);
            internal_.pendingTransition = 1;
        }
        /** \brief Current value in `[0, 1]`. */
        float getValue() const { return value; }

    protected:
        /**
         * \brief Default intrinsic size: `120 x max(16, 0.8 * fontSize)`.
         *
         * The height tracks the font size so that a progress bar sits nicely
         * inside a text-heavy layout by default.
         */
        Vec2 computeIntrinsicSize(float, float) override
        {
            float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            return {120.0f, std::max(16.0f, fs * 0.8f)};
        }

        /**
         * \brief Draw the track and the fill.
         *
         * The fill width is `innerW * value`, so a value of `0` or `1`
         * produces a fully collapsed or fully filled fill respectively. The
         * fill is skipped entirely below `0.5` pixel to avoid degenerate
         * geometry on backends that dislike zero-width rectangles.
         */
        void renderContent(float op, const ComputedStyle &style) override
        {
            auto r = UIContext::get().renderer;
            Style trackStyle = partStyle("track");
            Style fillStyle = partStyle("fill");

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
            Rect innerRect = {innerX, innerY, innerW, innerH};

            float maxR = std::min(innerW, innerH) * 0.5f;
            Color trackColor = style.background;
            if (trackStyle.background.is_set)
                trackColor = trackStyle.background.value;

            float rPx = trackStyle.radius.is_set
                            ? std::clamp(trackStyle.radius.value.resolveH(maxR * 2.0f, maxR * 2.0f), 0.0f, maxR)
                            : std::clamp(style.radius.resolveH(maxR * 2.0f, maxR * 2.0f), 0.0f, maxR);

            r->fillRoundedRect(innerRect, rPx, trackColor.withAlpha(op));

            float w = innerW * value;
            if (w > 0.5f)
            {
                Color fillColor = config.enabled ? style.color : Color{90, 90, 95, 255};
                if (fillStyle.color.is_set)
                    fillColor = fillStyle.color.value;
                Rect fill = {innerX, innerY, w, innerH};
                r->fillRoundedRect(fill, rPx, fillColor.withAlpha(op));
            }
        }

    private:
        float value;
    };

} // namespace ZenitUI::UI