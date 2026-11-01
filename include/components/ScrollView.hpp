#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

class ScrollView : public TLayout<ScrollView>
{
public:
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