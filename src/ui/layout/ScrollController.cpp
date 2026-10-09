#include "Layout.hpp"
#include "ScrollController.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

// =========================================================================
//  SCROLL
//  Comportamento per-asse: overflow-x/y indipendenti.
// =========================================================================

static bool isBarVisible(Overflow o, float maxScroll)
{
    if (o == Overflow::Scroll) return true;
    if (o == Overflow::Auto)   return maxScroll > 0.0f;
    return false;
}

bool ScrollController::acceptsAnyInput(const ComputedStyle& style) const
{
    return acceptsInput(style.overflowX) || acceptsInput(style.overflowY);
}

void ScrollController::reset()
{
    state.reset();
    appliedOffset = {0.0f, 0.0f};
    arrangeInitialized = false;
    scrolling = false;
}

// =========================================================================
//  ARRANGE
// =========================================================================

bool ScrollController::needsFullArrange(const Layout& node, bool positionChanged) const
{
    return !arrangeInitialized || node.subtreeDirty_ || positionChanged;
}

Rect ScrollController::computeArrangeSpace(const Layout& node, Rect space) const
{
    const auto& st = node.style_.currentStyle;

    float pl = st.padding.left.resolveH(space.width, space.height);
    float pr = st.padding.right.resolveH(space.width, space.height);
    float pt = st.padding.top.resolveV(space.width, space.height);
    float pb = st.padding.bottom.resolveV(space.width, space.height);

    Rect arranged = space;

    if (node.type == LayoutType::Vertical && st.overflowY != Overflow::Visible)
    {
        float contentH = contentSize.y + pt + pb;
        arranged.height = std::max(space.height, contentH);
    }
    else if (node.type == LayoutType::Horizontal && st.overflowX != Overflow::Visible)
    {
        float contentW = contentSize.x + pl + pr;
        arranged.width = std::max(space.width, contentW);
    }

    return arranged;
}

bool ScrollController::applyOffsetDelta(Layout& node)
{
    float dx = appliedOffset.x - state.offset.x;
    float dy = appliedOffset.y - state.offset.y;
    if (dx == 0.0f && dy == 0.0f)
        return false;

    node.translateSubtree(dx, dy);
    appliedOffset = state.offset;
    return true;
}

void ScrollController::updateMaxScroll(const Layout& node, Rect space)
{
    const auto& st = node.style_.currentStyle;

    float pl = st.padding.left.resolveH(space.width, space.height);
    float pr = st.padding.right.resolveH(space.width, space.height);
    float pt = st.padding.top.resolveV(space.width, space.height);
    float pb = st.padding.bottom.resolveV(space.width, space.height);

    float contentW = contentSize.x + pl + pr;
    float contentH = contentSize.y + pt + pb;
    state.maxScroll.x = std::max(0.0f, contentW - node.rect.width);
    state.maxScroll.y = std::max(0.0f, contentH - node.rect.height);
    state.clamp();
}

// =========================================================================
//  INPUT
// =========================================================================

void ScrollController::tickInput(Layout& node)
{
    const auto& st = node.style_.currentStyle;
    if (!acceptsAnyInput(st) || node.type == LayoutType::Stack)
        return;

    auto &ctx = UIContext::get();
    auto &pointer = ctx.pointer;
    const Rect& rect = node.rect;

    const bool canScrollY = state.maxScroll.y > 0.0f && st.overflowY != Overflow::Hidden;
    const bool canScrollX = state.maxScroll.x > 0.0f && st.overflowX != Overflow::Hidden;

    // --- Inizio drag o click sul track ---
    if (!node.hasPointerCapture())
    {
        if (pointer.pressed && ctx.topmostConsumer == &node)
        {
            bool handled = false;

            if (canScrollY)
            {
                Rect thumb = verticalThumb(node);
                if (thumb.contains(pointer.pos))
                {
                    node.capturePointer();
                    state.dragStartMouseY  = pointer.pos.y;
                    state.dragStartOffsetY = state.offset.y;
                    state.velocity = {0.0f, 0.0f};
                    handled = true;
                }
            }
            if (!handled && canScrollX)
            {
                Rect thumb = horizontalThumb(node);
                if (thumb.contains(pointer.pos))
                {
                    node.capturePointer();
                    state.dragStartMouseX  = pointer.pos.x;
                    state.dragStartOffsetX = state.offset.x;
                    state.velocity = {0.0f, 0.0f};
                    handled = true;
                }
            }

            if (!handled && canScrollY && !canScrollX)
            {
                float ratio = (rect.height > 0.0f)
                                  ? ((pointer.pos.y - rect.y) / rect.height)
                                  : 0.0f;
                state.offset.y = std::clamp(
                    ratio * contentSize.y - rect.height * 0.5f,
                    0.0f, state.maxScroll.y);
            }
            else if (!handled && canScrollX && !canScrollY)
            {
                float ratio = (rect.width > 0.0f)
                                  ? ((pointer.pos.x - rect.x) / rect.width)
                                  : 0.0f;
                state.offset.x = std::clamp(
                    ratio * contentSize.x - rect.width * 0.5f,
                    0.0f, state.maxScroll.x);
            }
        }
    }
    else if (pointer.down)
    {
        if (canScrollY)
        {
            float dy = pointer.pos.y - state.dragStartMouseY;
            float ratio = (rect.height > 0.0f) ? (contentSize.y / rect.height) : 1.0f;
            state.offset.y = std::clamp(
                state.dragStartOffsetY + dy * ratio, 0.0f, state.maxScroll.y);
        }
        if (canScrollX)
        {
            float dx = pointer.pos.x - state.dragStartMouseX;
            float ratio = (rect.width > 0.0f) ? (contentSize.x / rect.width) : 1.0f;
            state.offset.x = std::clamp(
                state.dragStartOffsetX + dx * ratio, 0.0f, state.maxScroll.x);
        }
    }

    // --- Wheel ---
    if (!ctx.wheelConsumedThisFrame && node.isInteractive &&
        !node.hasPointerCapture() && rect.contains(pointer.pos))
    {
        if (pointer.wheelY != 0.0f)
        {
            if (ctx.shiftHeld && canScrollX)
            {
                state.velocity.x -= pointer.wheelY * ScrollState::WHEEL_IMPULSE;
                ctx.wheelConsumedThisFrame = true;
            }
            else if (!ctx.shiftHeld && canScrollY)
            {
                state.velocity.y -= pointer.wheelY * ScrollState::WHEEL_IMPULSE;
                ctx.wheelConsumedThisFrame = true;
            }
        }
    }

    state.tickInertia(ctx.dt);

    scrolling = node.hasPointerCapture()
             || std::abs(state.velocity.x) > ScrollState::VELOCITY_MIN
             || std::abs(state.velocity.y) > ScrollState::VELOCITY_MIN;
}

// =========================================================================
//  RENDER
// =========================================================================

void ScrollController::drawScrollbar(const Layout& node, float parentOpacity) const
{
    const auto& st = node.style_.currentStyle;
    const Rect& rect = node.rect;

    const bool wantV = isBarVisible(st.overflowY, state.maxScroll.y);
    const bool wantH = isBarVisible(st.overflowX, state.maxScroll.x);
    if (!wantV && !wantH)
        return;

    auto r = UIContext::get().renderer;
    if (!r) return;

    float op = st.opacity * parentOpacity;
    if (op <= 0.001f) return;

    const float MARGIN  = 4.0f;
    const float TRACK_W = 6.0f;

    r->pushClip(rect);

    const float reservedV = wantV ? (TRACK_W + MARGIN) : 0.0f;
    const float reservedH = wantH ? (TRACK_W + MARGIN) : 0.0f;

    if (wantV)
    {
        Rect track = {
            rect.x + rect.width - TRACK_W - MARGIN,
            rect.y + MARGIN,
            TRACK_W,
            rect.height - MARGIN * 2 - reservedH};
        r->fillRoundedRect(track, TRACK_W * 0.5f, Color{40, 40, 40, 180}.withAlpha(op));

        Rect thumb = verticalThumb(node);
        Color thumbColor = node.hasPointerCapture()
                               ? Color{180, 180, 180, 255}
                               : Color{130, 130, 130, 220};
        r->fillRoundedRect(thumb, TRACK_W * 0.5f, thumbColor.withAlpha(op));
    }

    if (wantH)
    {
        Rect track = {
            rect.x + MARGIN,
            rect.y + rect.height - TRACK_W - MARGIN,
            rect.width - MARGIN * 2 - reservedV,
            TRACK_W};
        r->fillRoundedRect(track, TRACK_W * 0.5f, Color{40, 40, 40, 180}.withAlpha(op));

        Rect thumb = horizontalThumb(node);
        Color thumbColor = node.hasPointerCapture()
                               ? Color{180, 180, 180, 255}
                               : Color{130, 130, 130, 220};
        r->fillRoundedRect(thumb, TRACK_W * 0.5f, thumbColor.withAlpha(op));
    }

    r->popClip();
}

Rect ScrollController::verticalThumb(const Layout& node) const
{
    const auto& st = node.style_.currentStyle;
    const Rect& rect = node.rect;

    const float TRACK_W = 6.0f;
    const float MARGIN  = 4.0f;

    const bool wantH = isBarVisible(st.overflowX, state.maxScroll.x);
    const float reservedH = wantH ? (TRACK_W + MARGIN) : 0.0f;

    float trackH = std::max(0.0f, rect.height - MARGIN * 2.0f - reservedH);
    float contentH = contentSize.y;
    float viewportRatio = (contentH > 0.0f) ? (rect.height / contentH) : 1.0f;
    float thumbH = std::min(trackH, std::max(30.0f, trackH * viewportRatio));
    float scrollRatio = (state.maxScroll.y > 0.0f)
                            ? (state.offset.y / state.maxScroll.y)
                            : 0.0f;
    float available = std::max(0.0f, trackH - thumbH);
    float thumbY = rect.y + MARGIN + std::clamp(scrollRatio * available, 0.0f, available);

    return {rect.x + rect.width - TRACK_W - MARGIN, thumbY, TRACK_W, thumbH};
}

Rect ScrollController::horizontalThumb(const Layout& node) const
{
    const auto& st = node.style_.currentStyle;
    const Rect& rect = node.rect;

    const float TRACK_H = 6.0f;
    const float MARGIN  = 4.0f;

    const bool wantV = isBarVisible(st.overflowY, state.maxScroll.y);
    const float reservedV = wantV ? (TRACK_H + MARGIN) : 0.0f;

    float trackW = std::max(0.0f, rect.width - MARGIN * 2.0f - reservedV);
    float contentW = contentSize.x;
    float viewportRatio = (contentW > 0.0f) ? (rect.width / contentW) : 1.0f;
    float thumbW = std::min(trackW, std::max(30.0f, trackW * viewportRatio));
    float scrollRatio = (state.maxScroll.x > 0.0f)
                            ? (state.offset.x / state.maxScroll.x)
                            : 0.0f;
    float available = std::max(0.0f, trackW - thumbW);
    float thumbX = rect.x + MARGIN + std::clamp(scrollRatio * available, 0.0f, available);

    return {thumbX, rect.y + rect.height - TRACK_H - MARGIN, thumbW, TRACK_H};
}

void ScrollController::resetIfOverflowChanged(const Layout& node)
{
    const auto& cur = node.style_.currentStyle;
    const auto& prev = node.style_.lastInherited;
    if (cur.overflowX != prev.overflowX || cur.overflowY != prev.overflowY)
        reset();
}

} // namespace ZenitUI