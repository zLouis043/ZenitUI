#pragma once

/**
 * \file ScrollView.hpp
 * \brief A generic scrolling container.
 *
 * `ScrollView` is a very thin wrapper around `Layout`: it just sets
 * `overflow: scroll` on both axes via `getInlineDefaults()` and enables
 * interaction. The actual scroll logic lives in `ScrollController`, embedded
 * in `Layout`.
 *
 * The inline defaults (rather than inline base) are used so that application
 * CSS can override them without needing `!important`-style hacks — theme
 * rules win over `inlineDefaults` but lose to `inlineBase`.
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

/**
 * \class ScrollView
 * \brief A `Layout` with `overflow: scroll` on both axes by default.
 *
 * \note `setBlocksRaycast(true)` means that clicks in empty scroll area do
 *       not fall through to siblings. This is almost always what you want,
 *       but be aware of it when composing stacked scroll views.
 */
class ScrollView : public TLayout<ScrollView>
{
public:
    /**
     * \brief Construct a scroll view with the given flow direction.
     * \param t Layout strategy for children (default: Vertical).
     */
    explicit ScrollView(LayoutType t = LayoutType::Vertical) : TLayout<ScrollView>(t)
    {
        setInteractive(true);
        setBlocksRaycast(true);
        setStyleTag("ScrollView");
        getInlineDefaults().overflowX = Overflow::Scroll;
        getInlineDefaults().overflowY = Overflow::Scroll;
    }
};

} // namespace ZenitUI::UI