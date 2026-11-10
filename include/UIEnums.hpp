#pragma once

/**
 * \file UIEnums.hpp
 * \brief Small enumerations describing layout strategy and interaction state.
 *
 * Kept in their own header so that components and the style resolver can both
 * reference these enums without pulling in the full `Layout.hpp`.
 */

namespace ZenitUI
{
    /**
     * \enum LayoutType
     * \brief Positioning strategy applied to a Layout node's direct children.
     *
     * Determines how `Layout::measure`, `Layout::arrange` and the flex-flow
     * helpers interpret their children.
     *
     * \sa Layout
     */
    enum class LayoutType
    {
        Stack,      ///< Children overlap each other; Z-order controls paint order.
        Vertical,   ///< Children flow top-to-bottom (column flex semantics).
        Horizontal  ///< Children flow left-to-right (row flex semantics).
    };

    /**
     * \enum UIState
     * \brief Interaction state computed per node and consumed by the style
     *        resolver to select pseudo-class overlays.
     *
     * The state is recomputed every frame in `InputController::computeNextState`.
     * Transitions between states drive the `:hover`, `:pressed` and `:disabled`
     * pseudo-classes in the theme stylesheet.
     *
     * \sa InputController
     * \sa StyleResolver
     */
    enum class UIState
    {
        Idle,       ///< No interaction is active.
        Hover,      ///< Pointer is currently over the node.
        Pressed,    ///< A pointer button is held down on the node.
        Disabled    ///< The node is logically disabled (`config.enabled == 0`).
    };
}