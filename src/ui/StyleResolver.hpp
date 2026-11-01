#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "UIEnums.hpp" 
#include "Style.hpp"
#include "Theme.hpp"
#include "AnimPrimitives.hpp"
#include "StyleAttr.hpp"

namespace ZenitUI
{

class Layout;

struct StyleResolver
{
    Style inlineDefaults;   // "user agent" del widget: batteribile dal CSS
    Style inlineBase;       // inline "autore": vince su qualsiasi regola
    Style inlineHover, inlinePressed, inlineDisabled, inlineFocus, inlineChecked;

    ComputedStyle currentStyle, targetStyle, transitionStartStyle;
    UIState       currentState{UIState::Idle};
    float         transitionTimer{1.0f};

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

    ComputedStyle resolveFor(Layout& node);
    Style         partFor(Layout& node, const std::string& partName);

    void beginStateTransition(Layout& node, UIState newState);
    void resolvePendingTransition(Layout& node);
    void tick(Layout& node, float dt);
    void propagateInheritance(Layout& node);
};

} // namespace ZenitUI