#pragma once

#include "Common.hpp"
#include "UIEnums.hpp"

namespace ZenitUI
{

class Layout;

struct InputController
{
    void updateFlags(Layout& node, bool selfBlocked, bool scrolling);

    UIState computeNextState(const Layout& node) const;

    void fireCallbacks(Layout& node, UIState prev, UIState next, bool changed);

    void handleKeyInput(Layout& node);
};

} // namespace ZenitUI