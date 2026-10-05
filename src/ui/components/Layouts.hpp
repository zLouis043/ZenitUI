#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

inline std::shared_ptr<Layout> VStack(std::initializer_list<std::shared_ptr<Layout>> children = {})
{
    auto l = std::make_shared<Layout>(LayoutType::Vertical);
    l->setStyleTag("VStack");
    for (auto &c : children)
        l->addChild(c);
    return l;
}

inline std::shared_ptr<Layout> HStack(std::initializer_list<std::shared_ptr<Layout>> children = {})
{
    auto l = std::make_shared<Layout>(LayoutType::Horizontal);
    l->setStyleTag("HStack");
    for (auto &c : children)
        l->addChild(c);
    return l;
}

// ---------- Panel ----------
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
inline std::shared_ptr<Panel> Pan() { return Panel::create(); }

} // namespace ZenitUI::UI