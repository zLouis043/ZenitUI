#pragma once

#include "Common.hpp"
#include "UIEnums.hpp"

namespace ZenitUI
{

class Layout;

// =========================================================================
//  InputController
//  FSM hover/pressed/focused + dispatch callback + attivazione da tastiera.
//  Nessuno stato proprio: opera interamente su Layout (che possiede i flag
//  isHovered/isPressed/isFocused usati anche dal matching CSS).
// =========================================================================
struct InputController
{
    // Aggiorna isHovered/isPressed/isFocused di `node`.
    // `scrolling` congela l'hover durante lo scroll attivo.
    void updateFlags(Layout& node, bool selfBlocked, bool scrolling);

    // Calcola lo stato FSM corrente (Idle/Hover/Pressed/Disabled).
    UIState computeNextState(const Layout& node) const;

    // Dispatcha onHoverEnter/Exit, onPress, onRelease, onClick, onRightClick.
    void fireCallbacks(Layout& node, UIState prev, UIState next, bool changed);

    // Attiva onClick via Enter/Space quando il nodo è focused.
    void handleKeyInput(Layout& node);
};

} // namespace ZenitUI