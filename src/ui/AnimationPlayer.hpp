#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "Style.hpp"
#include "AnimPrimitives.hpp"     // (ex UIAnimations.hpp)
#include "Theme.hpp"

namespace ZenitUI
{

class Layout;

// =========================================================================
//  Primitive condivise per animazioni CSS.
//  Usate da AnimationPlayer sia per i nodi che per i ::part.
// =========================================================================
namespace Anim
{
    // Allinea `anims` a `refs` (rimuove orfani, aggiorna esistenti, aggiunge
    // nuove con startTime = now).
    void syncRefs(std::vector<ActiveCssAnimation>& anims,
                  const std::vector<AnimationRef>& refs,
                  double now);

    // Tick incrementale (nodi): elapsed += dt, marca finished, rimuove.
    void tickIncremental(std::vector<ActiveCssAnimation>& anims, float dt);

    // Tick assoluto (part): elapsed = now - startTime, marca finished, rimuove.
    void tickAbsolute(std::vector<ActiveCssAnimation>& anims, double now);

    // Applica i frame correnti dei keyframe a `out`.
    void overlayCssStyle(Style& out, const std::vector<ActiveCssAnimation>& anims);
    void overlayCssComputed(ComputedStyle& out, const std::vector<ActiveCssAnimation>& anims);
}

// =========================================================================
//  AnimationPlayer
//  - Imperative: UIAnimation con track programmatiche.
//  - CSS nodo: sync dal targetStyle + tick incrementale.
//  - CSS part: sync + tick assoluto + overlay al partStyle.
// =========================================================================
struct AnimationPlayer
{
    // --- Imperative ---
    std::unordered_map<std::string, AnimState> imperative;
    void play(Layout& node, const std::string& name, bool reverse);
    bool hasActive(const Layout& node) const;
    bool tickImperative(Layout& node, float dt);

    // --- CSS nodo ---
    std::vector<ActiveCssAnimation> css;
    void syncCss(Layout& node);
    void tickCss(Layout& node, float dt);

    // --- CSS part ---
    std::unordered_map<std::string, std::vector<ActiveCssAnimation>> parts;

    // Sync + tick + overlay in una chiamata (chiamato da StyleResolver::partFor).
    void applyCssToPart(Layout& node, const std::string& partName, Style& out);

    bool hasActivePartAnims() const
    {
        for (const auto& [n, v] : parts)
            if (!v.empty())
                return true;
        return false;
    }
};

} // namespace ZenitUI