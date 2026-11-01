#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "Style.hpp"
#include "AnimPrimitives.hpp"
#include "Theme.hpp"

namespace ZenitUI
{

    class Layout;

    namespace Anim
    {
        void syncRefs(std::vector<ActiveCssAnimation> &anims,
                      const std::vector<AnimationRef> &refs,
                      double now);

        void tickIncremental(std::vector<ActiveCssAnimation> &anims, float dt);

        void tickAbsolute(std::vector<ActiveCssAnimation> &anims, double now);

        void overlayCssStyle(Style &out, const std::vector<ActiveCssAnimation> &anims);
        void overlayCssComputed(ComputedStyle &out, const std::vector<ActiveCssAnimation> &anims);
    }

    struct AnimationPlayer
    {
        std::unordered_map<std::string, AnimState> imperative;
        void play(Layout &node, const std::string &name, bool reverse);
        bool hasActive(const Layout &node) const;
        bool tickImperative(Layout &node, float dt);

        std::vector<ActiveCssAnimation> css;
        void syncCss(Layout &node);
        void tickCss(Layout &node, float dt);

        std::unordered_map<std::string, std::vector<ActiveCssAnimation>> parts;

        void applyCssToPart(Layout &node, const std::string &partName, Style &out);

        bool hasActivePartAnims() const
        {
            for (const auto &[n, v] : parts)
                if (!v.empty())
                    return true;
            return false;
        }

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

        bool hasActiveImperative() const
        {
            for (const auto &[n, s] : imperative)
                if (s.playing)
                    return true;
            return false;
        }
    };

} // namespace ZenitUI