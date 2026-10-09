#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "ScrollState.hpp"

namespace ZenitUI {

class Layout;
struct ComputedStyle;

// =========================================================================
//  ScrollController
//  Stato e comportamento dello scroll di un nodo Layout.
//  Attivo quando currentStyle.overflowX/Y != Visible.
//
//  Riceve Layout& e legge dallo stile corrente: rect, padding, opacity,
//  overflow, isInteractive. Non conosce altro di Layout.
// =========================================================================
struct ScrollController
{
    ScrollState state;              // offset, maxScroll, velocity, dragStart
    Vec2        contentSize{0, 0};  // ingombro non tagliato del contenuto
    Vec2        appliedOffset{0, 0};// offset già applicato via translateSubtree
    bool        arrangeInitialized{false};
    bool        scrolling{false};   // drag attivo o inerzia in corso
    Overflow lastOverflowX{Overflow::Visible};
    Overflow lastOverflowY{Overflow::Visible};
    bool     overflowInitialized{false};

    // --- Arrange ---
    // Ritorna true se serve un re-arrange completo (dirty, cambio posizione, primo giro).
    bool needsFullArrange(const Layout& node, bool positionChanged) const;

    // Ritorna lo spazio espanso per contenere il contenuto sull'asse scrollabile.
    Rect computeArrangeSpace(const Layout& node, Rect space) const;

    // Applica il delta di scroll come translateSubtree sui figli del nodo.
    // Ritorna true se ha traslato qualcosa.
    bool applyOffsetDelta(Layout& node);

    // Aggiorna maxScroll dopo l'arrange.
    void updateMaxScroll(const Layout& node, Rect space);

    // Reset a zero (usato quando overflow cambia).
    void reset();

    // --- Input ---
    void tickInput(Layout& node);

    // --- Render ---
    void drawScrollbar(const Layout& node, float parentOpacity) const;

    // --- Helper interni (pubblici perché usati da tickInput/drawScrollbar) ---
    Rect verticalThumb(const Layout& node) const;
    Rect horizontalThumb(const Layout& node) const;
    void resetIfOverflowChanged(const Layout& node);

private:
    static bool acceptsInput(Overflow o)
    {
        return o == Overflow::Scroll || o == Overflow::Auto;
    }
    bool acceptsAnyInput(const ComputedStyle& style) const;
};

} // namespace ZenitUI