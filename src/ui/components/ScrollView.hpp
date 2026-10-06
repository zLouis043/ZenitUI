#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

// Zucchero: produce un Layout con overflow:scroll impostato inline.
// Tutto il comportamento (measure, arrange, input, scrollbar, draw)
// vive in Layout ed è guidato da `currentStyle.overflow`.
class ScrollView : public TLayout<ScrollView>
{
public:
    explicit ScrollView(LayoutType t = LayoutType::Vertical) : TLayout<ScrollView>(t)
    {
        setInteractive(true);
        setBlocksRaycast(true);
        setStyleTag("ScrollView");
        getInlineBase().overflow = Overflow::Scroll;
    }
};

} // namespace ZenitUI::UI