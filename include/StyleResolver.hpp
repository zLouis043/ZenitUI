#pragma once

/**
 * \file StyleResolver.hpp
 * \brief Per-node style resolution, state transitions and per-part animations.
 *
 * A `StyleResolver` lives inside every `Layout` (as `style_`). It exposes
 * three responsibilities:
 *
 *  1. **Resolve** sparse `Style` inputs (theme rules + inline overlays) into
 *     a dense `ComputedStyle`, with caching keyed by theme version, viewport
 *     generation, and the full ancestor-state vector.
 *  2. **Transition** between two `ComputedStyle` when the node's interaction
 *     state changes, interpolating only the properties that actually differ
 *     (tracked via `transitionDirtyMask_`).
 *  3. **Part styles**: resolve `Slider::track`, `Checkbox::mark`, etc., with
 *     their own independent transition state per part name.
 *
 * The struct deliberately keeps its fields public: `Layout`, `Measure.cpp`
 * and `Render.cpp` are friend structs and read/write them directly to avoid
 * accessor overhead in hot paths.
 */

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

    /**
     * \struct StyleResolver
     * \brief Resolves, caches, and transitions the style of a single node.
     *
     * \note The resolver does not own the node: it is embedded in `Layout`
     *       and receives `*this` as an argument to every method that needs
     *       node state (tag, classes, ancestry, enabled flags, interaction
     *       flags). This keeps it stateless w.r.t. the tree shape, which is
     *       what allows the cache key to be small and stable.
     */
    struct StyleResolver
    {
        // ---- Inline (per-node) style inputs ----
        Style inlineDefaults;   ///< Lowest-priority inline: applied first, overridable by rules.
        Style inlineBase;       ///< Highest-priority inline: applied last, after all overlays.
        Style inlineHover;      ///< Applied when the node is hovered or pressed.
        Style inlinePressed;    ///< Applied on top of `inlineHover` when pressed.
        Style inlineDisabled;   ///< Applied instead of state overlays when `enabled == false`.
        Style inlineFocus;      ///< Applied when the node has keyboard focus.
        Style inlineChecked;    ///< Applied when the node is in the `:checked` state.

        // ---- Current / target state ----
        ComputedStyle currentStyle;          ///< The value consumed by layout and render.
        ComputedStyle targetStyle;           ///< The value the transition is heading toward.
        ComputedStyle transitionStartStyle;  ///< The value at `transitionTimer == 0`.
        UIState currentState{UIState::Idle}; ///< Last observed interaction state.
        float transitionTimer{1.0f};         ///< Normalized `[0, 1]`; 1 means "settled".
        uint64_t transitionDirtyMask_{0};    ///< Bit per property, 1 = currently transitioning.

        /**
         * \struct PartTransition
         * \brief Independent transition state for one pseudo-element (`::part`).
         *
         * Parts are resolved lazily via `partFor`. They live outside the
         * node's main style and are updated only on demand, so a component
         * that never draws a given part pays no cost for it.
         */
        struct PartTransition
        {
            bool initialized{false};
            Style target;                              ///< The most recent part style from the cascade.
            Style current;                             ///< Interpolated value between `target` changes.
            double startTime{0.0};                     ///< `UIContext::time` at the last target change.
            float duration{0.0f};                      ///< Duration of the currently-running transition.
            TransitionFunction ease{TransitionFunction::Linear};
            bool active{false};
        };
        std::unordered_map<std::string, PartTransition> partTransitions;  ///< One entry per accessed part.

        /**
         * \struct InheritedSnapshot
         * \brief Last-seen inherited values, used to detect changes worth propagating.
         *
         * `propagateInheritance` compares the node's current inherited values
         * with this snapshot; if any differ, every child is marked
         * inheritance-dirty. This avoids an O(tree) walk on every frame.
         */
        struct InheritedSnapshot
        {
            bool hovered{false};
            bool pressed{false};
            bool focused{false};
            bool enabled{true};
            bool checked{false};
            FontStack font;
            Value fontSize;
            Color color;
            Value letterSpacing;
            Align textAlign{Align::Auto};
            Overflow overflowX{Overflow::Visible};
            Overflow overflowY{Overflow::Visible};
        };
        InheritedSnapshot lastInherited{};

        // ---- Cache of the last `resolveFor` result ----
        ComputedStyle cachedThemeStyle_;   ///< Result including theme rules but *before* inline overlays.
        uint64_t cachedKey_{0};            ///< Hash of every input that can change `cachedThemeStyle_`.
        bool cachedValid_{false};
        bool cachedAlignItemsSet_{false};
        bool cachedItemsHSet_{false};
        bool cachedItemsVSet_{false};

        /**
         * \brief Compute the fully resolved `ComputedStyle` for \p node.
         *
         * Algorithm:
         *  1. Build a cache key from theme version, viewport generation,
         *     node identity, ancestor state flags, style tag, and class list.
         *  2. On cache hit, skip the stylesheet walk and apply the state /
         *     inline overlays directly on top of `cachedThemeStyle_`.
         *  3. On miss, gather matching rules, sort by specificity then order,
         *     overlay them, resolve `var()` references, then call
         *     `ComputedStyle::from` with parent and theme root.
         *  4. Refresh `node.relevantSymbols_` for `invalidateIfRelevant`.
         *
         * \return The resolved `ComputedStyle`. The node's `style_` is *not*
         *         modified; callers decide whether to assign it to
         *         `currentStyle`/`targetStyle`.
         */
        ComputedStyle resolveFor(Layout &node);

        /**
         * \brief Resolve the style for pseudo-element \p partName on \p node.
         *
         * Runs an independent cascade, then advances that part's transition
         * state (lazily initialized on first call). The returned `Style` is a
         * live value: callers should apply it, not hold references.
         */
        Style partFor(Layout &node, const std::string &partName);

        /**
         * \brief Start a transition from the current style to \p newState's style.
         *
         * Called by `Layout::update` when `computeNextState` differs from
         * `currentState`. Records the pre-transition style, resolves the
         * post-transition target, and marks every differing property in
         * `transitionDirtyMask_`.
         */
        void beginStateTransition(Layout &node, UIState newState);

        /**
         * \brief Re-resolve the target style after a theme or ancestor change.
         *
         * Unlike `beginStateTransition`, this preserves an in-flight
         * transition: if the new target differs from the current target and
         * the node has an active imperative animation, the transition is
         * short-circuited. Otherwise only the properties that changed are
         * marked dirty and the timer is reset.
         */
        void resolvePendingTransition(Layout &node);

        /**
         * \brief Advance the state transition by \p dt seconds.
         *
         * Called every frame while `transitionTimer < 1`. Interpolates only
         * the properties whose bit is set in `transitionDirtyMask_`, using
         * per-property durations from `targetStyle.transitions` (falling back
         * to `transitionTime` / `ease` for properties not listed).
         *
         * \param node Unused today; reserved for future per-node easing.
         * \param dt   Frame delta in seconds.
         */
        void tick(Layout &node, float dt);

        /**
         * \brief Propagate inherited property changes to children.
         *
         * Compares the current inherited values against `lastInherited`; on
         * any change, marks every child as inheritance-dirty so they
         * re-resolve on the next frame. Also updates the snapshot.
         */
        void propagateInheritance(Layout &node);

        /** \brief Invalidate `cachedValid_`; called on inline setter changes. */
        void invalidateCache() noexcept { cachedValid_ = false; }

    private:
        /**
         * \brief Derive `alignItems` from deprecated `items-h` / `items-v`.
         *
         * Priority order:
         *  1. Explicit `align-items` (or inline `alignItems`) → nothing to derive.
         *  2. `itemsH` set on a Stack container → alignItems = itemsH.
         *  3. `itemsV` set on a Stack container (and itemsH not set) → itemsV.
         *  4. `itemsH` on a Vertical container (cross axis is horizontal).
         *  5. `itemsV` on a Horizontal container (cross axis is vertical).
         */
        void deriveAlignAliases(Layout &node,
                                const Style &inlineBase,
                                bool alignItemsSet, bool itemsHSet, bool itemsVSet,
                                Align styleItemsH, Align styleItemsV,
                                ComputedStyle &result);
    };

} // namespace ZenitUI