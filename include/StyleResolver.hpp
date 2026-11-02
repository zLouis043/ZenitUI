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
        Style inlineDefaults;
        Style inlineBase;
        Style inlineHover, inlinePressed, inlineDisabled, inlineFocus, inlineChecked;

        ComputedStyle currentStyle, targetStyle, transitionStartStyle;
        UIState currentState{UIState::Idle};
        float transitionTimer{1.0f};
        uint64_t      transitionDirtyMask_{0};

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

        ComputedStyle cachedThemeStyle_;
        uint64_t cachedKey_{0};
        bool cachedValid_{false};

        ComputedStyle resolveFor(Layout &node);
        Style partFor(Layout &node, const std::string &partName);

        void beginStateTransition(Layout &node, UIState newState);
        void resolvePendingTransition(Layout &node);
        void tick(Layout &node, float dt);
        void propagateInheritance(Layout &node);
        void invalidateCache() noexcept { cachedValid_ = false; }
    };

} // namespace ZenitUI