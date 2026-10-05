#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

class ScrollView : public TLayout<ScrollView>
{
public:
    ScrollView(LayoutType t = LayoutType::Vertical) : TLayout<ScrollView>(t)
    {
        setInteractive(true);
        setBlocksRaycast(true);
        setStyleTag("ScrollView");
        inlineBase.overflow = Overflow::Scroll;
        pendingTransition = true;
    }

    void scrollTo(float x, float y)
    {
        scrollX = std::max(0.0f, x);
        scrollY = std::max(0.0f, y);
        clampScroll();
        pendingTransition = true;
    }
    void scrollToY(float y) { scrollTo(scrollX, y); }
    void scrollToX(float x) { scrollTo(x, scrollY); }
    void scrollToTop() { scrollToY(0.0f); }
    void scrollToBottom() { scrollToY(maxScrollY); }
    void scrollToLeft() { scrollToX(0.0f); }
    void scrollToRight() { scrollToX(maxScrollX); }

    float getScrollX() const { return scrollX; }
    float getScrollY() const { return scrollY; }
    float getMaxScrollX() const { return maxScrollX; }
    float getMaxScrollY() const { return maxScrollY; }

    void arrange(Rect space) override
    {
        rect = space;

        float pl = currentStyle.padding.left.resolveH(rect.width, rect.height);
        float pr = currentStyle.padding.right.resolveH(rect.width, rect.height);
        float pt = currentStyle.padding.top.resolveV(rect.width, rect.height);
        float pb = currentStyle.padding.bottom.resolveV(rect.width, rect.height);

        Rect offsetSpace = space;
        offsetSpace.x -= scrollX;
        offsetSpace.y -= scrollY;

        if (type == LayoutType::Vertical)
        {
            float contentH = scrollContentSize.y + pt + pb;
            offsetSpace.height = std::max(space.height, contentH);
        }
        else if (type == LayoutType::Horizontal)
        {
            float contentW = scrollContentSize.x + pl + pr;
            offsetSpace.width = std::max(space.width, contentW);
        }

        arrangeInto(offsetSpace);

        float contentW = scrollContentSize.x + pl + pr;
        float contentH = scrollContentSize.y + pt + pb;
        maxScrollX = std::max(0.0f, contentW - rect.width);
        maxScrollY = std::max(0.0f, contentH - rect.height);
        clampScroll();
        onLayout();
    }

protected:
    void onUpdate(float dt) override
    {
        auto &ctx = UIContext::get();

        if (!hasPointerCapture())
        {
            if (ctx.pointer.pressed && ctx.topmostConsumer == this && maxScrollY > 0.0f)
            {
                Rect thumb = getThumbRect();
                if (thumb.contains(ctx.pointer.pos))
                {
                    capturePointer();
                    dragStartMouseY = ctx.pointer.pos.y;
                    dragStartScrollY = scrollY;
                    velocityX = velocityY = 0.0f;
                }
                else
                {
                    float ratio = (ctx.pointer.pos.y - rect.y) / rect.height;
                    scrollY = std::clamp(ratio * scrollContentSize.y - rect.height * 0.5f,
                                         0.0f, maxScrollY);
                    pendingTransition = true;
                }
            }
        }
        else if (ctx.pointer.down)
        {
            float dy = ctx.pointer.pos.y - dragStartMouseY;
            float ratio = (rect.height > 0.0f) ? (scrollContentSize.y / rect.height) : 1.0f;
            scrollY = std::clamp(dragStartScrollY + dy * ratio, 0.0f, maxScrollY);
            pendingTransition = true;
        }

        if (!ctx.wheelConsumedThisFrame && isInteractive &&
            !hasPointerCapture() && rect.contains(ctx.pointer.pos))
        {
            if (ctx.pointer.wheelY != 0.0f)
            {
                if (IsShiftHeld())
                    velocityX -= ctx.pointer.wheelY * WHEEL_IMPULSE;
                else
                    velocityY -= ctx.pointer.wheelY * WHEEL_IMPULSE;
                ctx.wheelConsumedThisFrame = true;
                pendingTransition = true;
            }
        }

        if (std::abs(velocityX) > VELOCITY_MIN)
        {
            scrollX += velocityX * dt;
            velocityX *= std::pow(DECAY, dt * 60.0f);
        }
        else
            velocityX = 0.0f;

        if (std::abs(velocityY) > VELOCITY_MIN)
        {
            scrollY += velocityY * dt;
            velocityY *= std::pow(DECAY, dt * 60.0f);
        }
        else
            velocityY = 0.0f;

        clampScroll();
    }

    void draw(float parentOpacity) override
    {
        Layout::draw(parentOpacity);

        auto r = UIContext::get().renderer;
        if (!r || maxScrollY <= 0.0f || rect.height <= 10.0f)
            return;

        const float MARGIN = 4.0f;
        const float TRACK_W = 6.0f;
        float op = currentStyle.opacity * parentOpacity;
        if (op <= 0.001f)
            return;

        r->pushClip(rect);

        Rect track = {
            rect.x + rect.width - TRACK_W - MARGIN,
            rect.y + MARGIN,
            TRACK_W,
            rect.height - MARGIN * 2};
        r->fillRoundedRect(track, TRACK_W * 0.5f, Color{40, 40, 40, 180}.withAlpha(op));

        Rect thumb = getThumbRect();
        Color thumbColor = hasPointerCapture()
                               ? Color{180, 180, 180, 255}
                               : Color{130, 130, 130, 220};
        r->fillRoundedRect(thumb, TRACK_W * 0.5f, thumbColor.withAlpha(op));

        r->popClip();
    }

private:
    float scrollX{0.0f};
    float scrollY{0.0f};
    float maxScrollX{0.0f};
    float maxScrollY{0.0f};

    float velocityX{0.0f};
    float velocityY{0.0f};

    static constexpr float WHEEL_IMPULSE = 400.0f;
    static constexpr float DECAY = 0.90f;
    static constexpr float VELOCITY_MIN = 5.0f;

    float dragStartMouseY{0.0f};
    float dragStartScrollY{0.0f};

    void clampScroll()
    {
        scrollX = std::clamp(scrollX, 0.0f, maxScrollX);
        scrollY = std::clamp(scrollY, 0.0f, maxScrollY);
    }

    Rect getThumbRect() const
    {
        const float TRACK_W = 6.0f;
        const float MARGIN = 4.0f;

        float trackH = std::max(0.0f, rect.height - MARGIN * 2.0f);
        float contentH = scrollContentSize.y;
        float viewportRatio = (contentH > 0.0f) ? (rect.height / contentH) : 1.0f;
        float thumbH = std::min(trackH, std::max(30.0f, trackH * viewportRatio));
        float scrollRatio = (maxScrollY > 0.0f) ? (scrollY / maxScrollY) : 0.0f;
        float available = std::max(0.0f, trackH - thumbH);
        float thumbY = rect.y + MARGIN + std::clamp(scrollRatio * available, 0.0f, available);

        return {rect.x + rect.width - TRACK_W - MARGIN, thumbY, TRACK_W, thumbH};
    }

    static bool IsShiftHeld() { return UIContext::get().shiftHeld; }
};

} // namespace ZenitUI::UI