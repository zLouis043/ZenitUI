#pragma once

/**
 * \file Layouts.hpp
 * \brief Basic layout containers: VStack, HStack, Panel.
 *
 * These are the simplest building blocks: they wrap `Layout` with a style tag
 * so that themes can target them by name, and expose convenient factory
 * functions (`VStack`, `HStack`, `Pan`).
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

/**
 * \brief Create a vertical-flow container (`LayoutType::Vertical`).
 * \param children Optional initial children; ownership is transferred.
 * \return The container as a `shared_ptr<Layout>`.
 */
inline std::shared_ptr<Layout> VStack(std::initializer_list<std::shared_ptr<Layout>> children = {})
{
    auto l = std::make_shared<Layout>(LayoutType::Vertical);
    l->setStyleTag("VStack");
    for (auto &c : children)
        l->addChild(c);
    return l;
}

/**
 * \brief Create a horizontal-flow container (`LayoutType::Horizontal`).
 * \param children Optional initial children; ownership is transferred.
 * \return The container as a `shared_ptr<Layout>`.
 */
inline std::shared_ptr<Layout> HStack(std::initializer_list<std::shared_ptr<Layout>> children = {})
{
    auto l = std::make_shared<Layout>(LayoutType::Horizontal);
    l->setStyleTag("HStack");
    for (auto &c : children)
        l->addChild(c);
    return l;
}

/**
 * \class Panel
 * \brief A generic `Stack` container that blocks raycasts.
 *
 * Panels are the default container for grouping content and receiving clicks
 * as a unit. They do not consume clicks themselves (no `onClick` by default),
 * but because `blocksRaycast` is set, a click on a Panel that has no
 * interactive child stops at the Panel — it does not leak to siblings below.
 *
 * Style tag: `"Panel"`.
 */
class Panel : public TLayout<Panel>
{
public:
    Panel() : TLayout<Panel>(LayoutType::Stack)
    {
        setBlocksRaycast(true);
        setInteractive(false);
        setStyleTag("Panel");
    }
};

/** \brief Convenience factory for `Panel::create()`. */
inline std::shared_ptr<Panel> Pan() { return Panel::create(); }

} // namespace ZenitUI::UI