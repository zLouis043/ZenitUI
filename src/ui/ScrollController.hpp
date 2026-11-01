#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "ScrollState.hpp"

namespace ZenitUI {

class Layout;
struct ComputedStyle;

struct ScrollController
{
    ScrollState state;
    Vec2        contentSize{0, 0};
    Vec2        appliedOffset{0, 0};
    bool        arrangeInitialized{false};
    bool        scrolling{false};
    Overflow lastOverflowX{Overflow::Visible};
    Overflow lastOverflowY{Overflow::Visible};
    bool     overflowInitialized{false};

    bool needsFullArrange(const Layout& node, bool positionChanged) const;

    Rect computeArrangeSpace(const Layout& node, Rect space) const;

    bool applyOffsetDelta(Layout& node);

    void updateMaxScroll(const Layout& node, Rect space);

    void reset();

    void tickInput(Layout& node);

    void drawScrollbar(const Layout& node, float parentOpacity) const;

    Rect verticalThumb(const Layout& node) const;
    Rect horizontalThumb(const Layout& node) const;
    void resetIfOverflowChanged(const Layout& node);

private:
    static bool acceptsInput(Overflow o)
    {
        return o == Overflow::Scroll || o == Overflow::Auto;
    }
    bool acceptsAnyInput(const ComputedStyle& style) const;
};

} // namespace ZenitUI