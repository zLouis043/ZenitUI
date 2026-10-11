#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

// ---------------------------------------------------------------------------
//  Rating — a custom widget that shows N stars and lets the user pick one.
//
//  This file is a template for writing your own widget. It demonstrates:
//
//    - TLayout<Derived>       the CRTP factory: create() runs onBuild()
//    - onBuild()              one-time setup (style tag, flags, children)
//    - computeIntrinsicSize() natural size when width/height are auto
//    - renderContent()        drawing the widget using ::part styles
//    - onUpdate()             per-frame logic: read the pointer, respond
//    - ::part                 a named styling surface (see theme.zstyle)
//
//  Build:   make examples
//  Run:     make run-09_custom_widget
// ---------------------------------------------------------------------------

class Rating : public TLayout<Rating>
{
public:
    Rating(int maxStars = 5, int value = 0)
        : TLayout<Rating>(LayoutType::Horizontal),
          maxStars_(maxStars),
          value_(std::clamp(value, 0, maxStars))
    {
        setStyleTag("Rating");
        setInteractive(true);
        setBlocksRaycast(true);
        setFocusable(true);
        setKeyboardActivates(false);   // we handle keys ourselves
        getInlineBase().gap = Px(4.0f);
    }

    // ---- Public API ------------------------------------------------------

    int value()    const { return value_; }
    int maxStars() const { return maxStars_; }

    void setValue(int v)
    {
        int clamped = std::clamp(v, 0, maxStars_);
        if (clamped == value_)
            return;
        value_ = clamped;
        pendingTransition = true;
        if (onValueChanged)
            onValueChanged(value_);
    }

    std::function<void(int)> onValueChanged;

protected:
    // ---- Intrinsic size --------------------------------------------------
    //  Called during measure, before children are laid out. Returns the
    //  size the widget would like if it has no explicit width/height.
    Vec2 computeIntrinsicSize(float, float) override
    {
        float fs = style_.currentStyle.fontSize.resolveV(
            Metrics::viewport.x, Metrics::viewport.y);
        float starSize = fs * 1.4f;
        float gap = getStyle().gap.resolveH(0, 0);
        float totalW = starSize * maxStars_ + gap * (maxStars_ - 1);
        return { totalW, starSize };
    }

    // ---- Drawing ---------------------------------------------------------
    //  Called during draw, after the background (renderChrome) and before
    //  the children (there are none here).
    void renderContent(float op, const ComputedStyle& style) override
    {
        auto r = UIContext::get().renderer;
        if (!r) return;

        float fs = style.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
        float starSize = fs * 1.4f;
        float gap = style.gap.resolveH(rect.width, rect.height);
        float totalW = starSize * maxStars_ + gap * (maxStars_ - 1);
        float startX = rect.x + (rect.width - totalW) * 0.5f;
        float centerY = rect.center().y;

        // partStyle() resolves a ::part from the theme, with its own
        // transitions and animations. Every part is a Style with optional
        // fields: check is_set before using.
        Style starStyle = partStyle("star");

        Color filledColor = starStyle.color.is_set
            ? starStyle.color.value
            : Color{255, 200, 60, 255};

        Color emptyColor = starStyle.background.is_set
            ? starStyle.background.value
            : Color{70, 70, 80, 255};

        for (int i = 0; i < maxStars_; ++i)
        {
            float cx = startX + i * (starSize + gap) + starSize * 0.5f;
            Color c = (i < value_) ? filledColor : emptyColor;
            r->fillCircle({cx, centerY}, starSize * 0.5f, c.withAlpha(op));
        }
    }

    // ---- Per-frame logic -------------------------------------------------
    //  Called after the state machine, every frame the node is enabled.
    void onUpdate(float) override
    {
        auto& ctx = UIContext::get();

        // Keyboard: Left / Right adjust, when focused.
        if (isFocusedState())
        {
            for (int k : ctx.inputEvents.keys)
            {
                if (k == Key::Left)  setValue(value_ - 1);
                if (k == Key::Right) setValue(value_ + 1);
            }
        }

        // Mouse: click picks the star under the pointer.
        // Using topmostConsumer (set by hitTest) instead of a geometric
        // rect.contains check ensures only the node that actually
        // received the click reacts, even if some other node's rect
        // overlaps the same point.
        if (!ctx.pointer.pressed) return;
        if (ctx.topmostConsumer != this) return;
        if (rect.width <= 0.0f) return;

        float fs = style_.currentStyle.fontSize.resolveV(
            Metrics::viewport.x, Metrics::viewport.y);
        float starSize = fs * 1.4f;
        float gap = style_.currentStyle.gap.resolveH(rect.width, rect.height);
        float totalW = starSize * maxStars_ + gap * (maxStars_ - 1);
        float startX = rect.x + (rect.width - totalW) * 0.5f;

        float localX = ctx.pointer.pos.x - startX;
        int idx = (int)(localX / (starSize + gap));
        idx = std::clamp(idx, 0, maxStars_ - 1);
        setValue(idx + 1);
    }

private:
    int maxStars_;
    int value_;
};

} // namespace ZenitUI::UI