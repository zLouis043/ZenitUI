#pragma once

/**
 * \file ScrollController.hpp
 * \brief Overflow handling, scrolling input, and scrollbar rendering for a node.
 *
 * Every `Layout` embeds one `ScrollController` (as `scroll_`). The controller
 * only does work when the node's style has at least one non-visible overflow
 * axis. Its responsibilities:
 *
 *  - Track the content size measured by `Layout::measure`.
 *  - Grow the arrange space beyond the viewport when content overflows.
 *  - Translate the subtree by `(appliedOffset - state.offset)` whenever the
 *    offset changes, avoiding a full re-arrange.
 *  - Translate wheel input into inertial velocity; drag on the scrollbar
 *    thumb into direct offset changes.
 *  - Draw the scrollbars, when `Overflow::Scroll` or (`Overflow::Auto` and
 *    content overflows).
 *
 * The division of labor between the controller and `Layout` is:
 *
 *  - `Layout::measure` sets `contentSize` via `scroll_.contentSize`.
 *  - `Layout::arrange` calls `needsFullArrange`, `computeArrangeSpace`,
 *    `applyOffsetDelta`, and `updateMaxScroll`.
 *  - `Layout::update` calls `tickInput` and `resetIfOverflowChanged`.
 *  - `Layout::draw` calls `drawScrollbar`.
 */

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "ScrollState.hpp"

namespace ZenitUI {

class Layout;
struct ComputedStyle;

/**
 * \struct ScrollController
 * \brief Per-node overflow, scrolling and scrollbar state machine.
 *
 * Fields are public by design: `Layout` reads and writes them directly, and
 * `Render.cpp` reads `contentSize` for hit-test-relevant calculations.
 */
struct ScrollController
{
    ScrollState state;
    Vec2        contentSize{0, 0};
    Vec2        appliedOffset{0, 0};
    bool        arrangeInitialized{false};
    bool        scrolling{false};
    Overflow    lastOverflowX{Overflow::Visible};
    Overflow    lastOverflowY{Overflow::Visible};
    bool        overflowInitialized{false};

    /**
     * \brief `true` if a full re-arrange is needed before applying an offset delta.
     */
    bool needsFullArrange(const Layout& node, bool positionChanged) const;

    /**
     * \brief Compute the space to pass to `Layout::arrangeInto`.
     */
    Rect computeArrangeSpace(const Layout& node, Rect space) const;

    /**
     * \brief Translate the subtree by the delta between `state.offset` and
     *        `appliedOffset`.
     */
    bool applyOffsetDelta(Layout& node);

    /**
     * \brief Recompute `state.maxScroll` from `contentSize` and the node's rect.
     */
    void updateMaxScroll(const Layout& node, Rect space);

    /** \brief Reset offset, bounds, velocity and scroll-activity flags. */
    void reset();

    /**
     * \brief Process pointer and wheel input for this scroll container.
     */
    void tickInput(Layout& node);

    /**
     * \brief Draw the vertical and/or horizontal scrollbars.
     */
    void drawScrollbar(const Layout& node, float parentOpacity) const;

    /**
     * \brief Compute the vertical scrollbar thumb rectangle in screen space.
     */
    Rect verticalThumb(const Layout& node) const;

    /** \brief Compute the horizontal scrollbar thumb rectangle in screen space. */
    Rect horizontalThumb(const Layout& node) const;

    /**
     * \brief Detect `overflow` style changes and reset scroll state if any.
     */
    void resetIfOverflowChanged(const Layout& node);

private:
    /** \brief `true` if the overflow mode accepts pointer/wheel input. */
    static bool acceptsInput(Overflow o)
    {
        return o == Overflow::Scroll || o == Overflow::Auto;
    }
    /** \brief `true` if either axis accepts input. */
    bool acceptsAnyInput(const ComputedStyle& style) const;
};

} // namespace ZenitUI