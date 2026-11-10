#pragma once

/**
 * \file InputController.hpp
 * \brief Hover / press / focus state and click-callback dispatch for one node.
 *
 * Every `Layout` embeds one `InputController` (as `input_`). The controller
 * is stateless: it only mutates the node's public state fields (`state.hovered`,
 * `state.pressed`, `state.focused`) and fires the node's callbacks. The
 * per-node state itself lives in `Layout`, which is what the style resolver
 * reads to select pseudo-class overlays.
 *
 * Per-frame responsibilities:
 *
 *  - `updateFlags`: recompute hovered / pressed from the current pointer.
 *  - `computeNextState`: derive the `UIState` enum consumed by the resolver.
 *  - `fireCallbacks`: dispatch `onPress` / `onRelease` / `onClick` and
 *    `onRightClick` based on the current frame's pointer transitions.
 *  - `handleKeyInput`: translate Enter / Space into `onClick` for the focused
 *    keyboard-activatable node.
 *
 * Click routing uses a press-then-release model: `pressTarget` is set on
 * button-down, `releaseTarget` on button-up, and `onClick` fires only if both
 * agree. This prevents clicks from firing when the pointer is released over
 * a different node (e.g. drag off a button).
 */

#include "Common.hpp"
#include "UIEnums.hpp"

namespace ZenitUI
{

class Layout;

/**
 * \struct InputController
 * \brief Stateless per-node input dispatcher.
 */
struct InputController
{
    /**
     * \brief Recompute the node's `hovered` / `pressed` / `focused` flags.
     *
     * \param node        Node whose flags are updated.
     * \param selfBlocked `true` when input to this node should be suppressed.
     * \param scrolling   `true` while any ancestor is scrolling or the node
     *                    itself is scrolling. In this mode, only focus is
     *                    updated: hover / press are frozen so that a drag
     *                    does not flicker buttons under the pointer.
     */
    void updateFlags(Layout& node, bool selfBlocked, bool scrolling);

    /**
     * \brief Derive the `UIState` enum from the node's current state flags.
     *
     * Order of precedence: `Disabled` > `Pressed` > `Hover` > `Idle`.
     */
    UIState computeNextState(const Layout& node) const;

    /**
     * \brief Fire callbacks for state transitions and pointer events.
     */
    void fireCallbacks(Layout& node, UIState prev, UIState next, bool changed);

    /**
     * \brief Fire `onClick` on Enter or Space for the focused, keyboard-
     *        activatable node.
     */
    void handleKeyInput(Layout& node);
};

} // namespace ZenitUI