#pragma once

/**
 * \file Checkbox.hpp
 * \brief A boolean checkbox with a drawn check mark.
 *
 * The visual is drawn directly by `renderContent` (a filled rounded rect for
 * the box, plus a small white square for the check mark when checked). Theme
 * rules can override both through the `::box` and `::mark` pseudo-elements.
 *
 * \warning This component does **not** call `setChecked` in its `onClick`
 *          handler, unlike `Toggle`. As a result, the `:checked` pseudo-class
 *          is never set from user interaction; only an explicit
 *          `setChecked()` call from application code will trigger it. If you
 *          want `Checkbox:checked { ... }` to work, use `Toggle` or patch
 *          the component to call `setChecked(isChecked)` after the state
 *          flip. See `Toggle` for the intended pattern.
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI
{

    /**
     * \class Checkbox
     * \brief A checkbox with a custom-painted check mark.
     *
     * Style tag: `"Checkbox"`. Default intrinsic size: `1.4 * fontSize` square.
     * Parts: `::box` (the outer square), `::mark` (the check mark glyph —
     * currently drawn as a white square, not an actual tick).
     */
    class Checkbox : public TLayout<Checkbox>
    {
    public:
        /**
         * \brief Construct with an initial state.
         * \param state Initial checked state.
         */
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

        /** \brief Fired after the checked state changes, with the new value. */
        std::function<void(bool)> onToggle = nullptr;
        /** \brief Current checked state. */
        bool isChecked;

    protected:
        /**
         * \brief Default intrinsic size: `1.4 * fontSize` on each side.
         *
         * Means a checkbox tracks the font size unless the caller overrides
         * `width` / `height`.
         */
        Vec2 computeIntrinsicSize(float, float) override
        {
            float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            return {fs * 1.4f, fs * 1.4f};
        }

        /**
         * \brief Draw the box and (if checked) the mark.
         *
         * Respects `::box.background` / `::box.radius` overrides and
         * `::mark.color`. When the node is disabled, uses a muted palette
         * that does not depend on theme rules — so disabled appearance is
         * consistent across themes.
         */
        void renderContent(float op, const ComputedStyle &style) override
        {
            auto r = UIContext::get().renderer;
            Style boxStyle = partStyle("box");
            Style markStyle = partStyle("mark");

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

            Color box = isChecked ? Colors::Green : style.background;
            if (!config.enabled)
                box = Color{70, 70, 75, 255};
            if (boxStyle.background.is_set)
                box = boxStyle.background.value;

            float maxR = std::min(innerW, innerH) * 0.5f;
            float rPx = boxStyle.radius.is_set
                            ? std::clamp(boxStyle.radius.value.resolveH(maxR * 2.0f, maxR * 2.0f), 0.0f, maxR)
                            : std::clamp(style.radius.resolveH(maxR * 2.0f, maxR * 2.0f), 0.0f, maxR);

            r->fillRoundedRect(innerRect, rPx, box.withAlpha(op));

            if (isChecked)
            {
                Color mark = Colors::White;
                if (!config.enabled)
                    mark = Color{150, 150, 155, 255};
                if (markStyle.color.is_set)
                    mark = markStyle.color.value;

                float innerMark = innerH * 0.35f;
                Rect m = {
                    innerX + innerW * 0.5f - innerMark * 0.5f,
                    innerY + innerH * 0.5f - innerMark * 0.5f,
                    innerMark, innerMark};
                r->fillRoundedRect(m, 3.0f, mark.withAlpha(op));
            }
        }
    };

} // namespace ZenitUI::UI