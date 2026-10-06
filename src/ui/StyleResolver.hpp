#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "UIEnums.hpp" 
#include "Style.hpp"
#include "Theme.hpp"
#include "UIAnimations.hpp"

namespace ZenitUI
{

class Layout;

// =========================================================================
//  StyleResolver
//  Sistema autonomo che gestisce:
//   - cascata CSS + inline + defaults + stato
//   - transizioni tra stati (beginStateTransition / resolve / tick)
//   - overlay dei ::part con propria transizione (Slider::knob, ecc.)
//   - propagazione delle proprietà ereditate ai figli
//
//  Non conosce Layout in modo forte: riceve Layout& e ne legge lo stato
//  pubblico (hover/pressed/focused/enabled/checked, tag, classi, parent,
//  children). Scrive solo nei propri campi.
// =========================================================================
struct StyleResolver
{
    // --- Livelli della cascata ---
    Style inlineDefaults;   // "user agent" del widget: batteribile dal CSS
    Style inlineBase;       // inline "autore": vince su qualsiasi regola
    Style inlineHover, inlinePressed, inlineDisabled, inlineFocus, inlineChecked;

    // --- Stato di stile corrente ---
    ComputedStyle currentStyle, targetStyle, transitionStartStyle;
    UIState       currentState{UIState::Idle};
    float         transitionTimer{1.0f};

    // --- ::part (Slider::knob, ecc.) ---
    struct PartTransition
    {
        bool initialized{false};
        Style target;
        Style current;
        double startTime{0.0};
        float duration{0.0f};
        TransitionFunction ease{TransitionFunction::Linear};
        bool active{false};
    };
    std::unordered_map<std::string, PartTransition> partTransitions;

    std::unordered_map<std::string, std::vector<ActiveCssAnimation>> activePartAnimations;

    // --- Snapshot per la propagazione dell'ereditarietà ---
    //  Contiene sia i flag di stato (influenzano i discendenti via selettori
    //  tipo `.card:hover .card-title`) sia le proprietà realmente ereditate.
    struct InheritedSnapshot
    {
        bool hovered{false};
        bool pressed{false};
        bool focused{false};
        bool enabled{true};
        bool checked{false};
        std::string font;
        Value fontSize;
        Color color;
        Value letterSpacing;
        Align textAlign{Align::Auto};
        Overflow overflowX{Overflow::Visible};
        Overflow overflowY{Overflow::Visible};
    };
    InheritedSnapshot lastInherited{};

    // --- API ---
    ComputedStyle resolveFor(Layout& node);
    Style         partFor(Layout& node, const std::string& partName);

    void beginStateTransition(Layout& node, UIState newState);
    void resolvePendingTransition(Layout& node);
    void tick(Layout& node, float dt);
    void propagateInheritance(Layout& node);
};

} // namespace ZenitUI