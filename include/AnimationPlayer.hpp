#pragma once

/**
 * \file AnimationPlayer.hpp
 * \brief Per-node owner of imperative and CSS animation state.
 *
 * Every `Layout` embeds one `AnimationPlayer` (as `anim_`). It is
 * responsible for:
 *
 *  - Advancing imperative animations (`tickImperative`).
 *  - Keeping the CSS animation list in sync with the node's target style
 *    (`syncCss`, driven by `Style::animations`).
 *  - Advancing CSS animations (`tickCss`).
 *  - Sampling CSS animations onto a style at draw time
 *    (`overlayCssStyle`, `overlayCssComputed`).
 *  - Managing per-pseudo-element animations (`applyCssToPart`), which use
 *    wall-clock time (`UIContext::time`) instead of a per-frame delta.
 *
 * The `Anim` namespace in `AnimationPlayer.cpp` holds the free functions
 * that mutate `ActiveCssAnimation` lists; this header only declares them.
 * The `AnimationPlayer` struct itself is the stateful facade.
 */

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "Style.hpp"
#include "AnimPrimitives.hpp"
#include "Theme.hpp"

namespace ZenitUI
{

    class Layout;

    /** \brief Internal helpers; called from `AnimationPlayer` and from `Layout::draw`. */
    namespace Anim
    {
        /**
         * \brief Synchronize the active-animation list with a new set of references.
         *
         * Removes entries whose `name` is no longer in \p refs, updates
         * parameters on entries that are still referenced, and creates new
         * entries (starting at \p now) for names that were just added.
         */
        void syncRefs(std::vector<ActiveCssAnimation> &anims,
                      const std::vector<AnimationRef> &refs,
                      double now);

        /**
         * \brief Advance CSS animations by \p dt seconds, and drop finished non-filling ones.
         */
        void tickIncremental(std::vector<ActiveCssAnimation> &anims, float dt);

        /**
         * \brief Advance CSS animations by setting `elapsed = now - startTime`.
         */
        void tickAbsolute(std::vector<ActiveCssAnimation> &anims, double now);

        /**
         * \brief Overlay every active CSS animation onto a sparse `Style`.
         */
        void overlayCssStyle(Style &out, const std::vector<ActiveCssAnimation> &anims);

        /**
         * \brief Overlay every active CSS animation onto a dense `ComputedStyle`.
         */
        void overlayCssComputed(ComputedStyle &out, const std::vector<ActiveCssAnimation> &anims);
    }

    /**
     * \struct AnimationPlayer
     * \brief Per-node animation state and operations.
     *
     * Fields are public by design: `Layout` and `Render.cpp` read and write
     * them directly in hot paths.
     */
    struct AnimationPlayer
    {
        // ---- Imperative animations ----
        std::unordered_map<std::string, AnimState> imperative;

        /**
         * \brief Start (or restart) the imperative animation named \p name.
         */
        void play(Layout &node, const std::string &name, bool reverse);

        /**
         * \brief `true` if this node or any descendant has an active animation.
         */
        bool hasActive(const Layout &node) const;

        /**
         * \brief Advance imperative animations by \p dt seconds.
         *
         * \return `true` if any active animation has `blocksInput` set.
         */
        bool tickImperative(Layout &node, float dt);

        // ---- CSS animations (main node) ----
        std::vector<ActiveCssAnimation> css;

        /**
         * \brief Sync `css` with `node.style_.targetStyle.animations`.
         */
        void syncCss(Layout &node);

        /** \brief Advance CSS animations by \p dt seconds. */
        void tickCss(Layout &node, float dt);

        // ---- CSS animations (per-part) ----
        std::unordered_map<std::string, std::vector<ActiveCssAnimation>> parts;

        /**
         * \brief Resolve and overlay CSS animations for pseudo-element \p partName.
         */
        void applyCssToPart(Layout &node, const std::string &partName, Style &out);

        /**
         * \brief `true` if any part is currently animating.
         */
        bool hasActivePartAnims() const
        {
            for (const auto &[n, v] : parts)
                if (!v.empty())
                    return true;
            return false;
        }

        /**
         * \brief `true` if this node itself (not descendants) has any active animation.
         */
        bool hasActiveLocal() const
        {
            for (const auto &[n, s] : imperative)
                if (s.playing)
                    return true;
            if (!css.empty())
                return true;
            for (const auto &[pn, v] : parts)
                if (!v.empty())
                    return true;
            return false;
        }

        /**
         * \brief `true` if any imperative animation is currently playing.
         */
        bool hasActiveImperative() const
        {
            for (const auto &[n, s] : imperative)
                if (s.playing)
                    return true;
            return false;
        }
    };

} // namespace ZenitUI