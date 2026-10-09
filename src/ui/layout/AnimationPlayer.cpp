#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

// =========================================================================
//  PRIMITIVE (namespace Anim)
// =========================================================================

void Anim::syncRefs(std::vector<ActiveCssAnimation>& anims,
                    const std::vector<AnimationRef>& refs,
                    double now)
{
    // 1) Rimuovi orfani (anims il cui nome non è in refs).
    anims.erase(
        std::remove_if(anims.begin(), anims.end(),
                       [&](const ActiveCssAnimation& a)
                       {
                           for (const auto& r : refs)
                               if (r.name == a.name)
                                   return false;
                           return true;
                       }),
        anims.end());

    // 2) Aggiorna esistenti / aggiungi nuove.
    for (const auto& r : refs)
    {
        bool found = false;
        for (auto& a : anims)
        {
            if (a.name == r.name)
            {
                a.duration = r.duration;
                a.delay = r.delay;
                a.iterations = r.iterations;
                a.alternate = r.alternate;
                a.fillForwards = r.fillForwards;
                a.ease = r.ease;
                found = true;
                break;
            }
        }
        if (!found)
        {
            ActiveCssAnimation a;
            a.name = r.name;
            a.startTime = now;
            a.duration = r.duration;
            a.delay = r.delay;
            a.iterations = r.iterations;
            a.alternate = r.alternate;
            a.fillForwards = r.fillForwards;
            a.ease = r.ease;
            anims.push_back(std::move(a));
        }
    }
}

void Anim::tickIncremental(std::vector<ActiveCssAnimation>& anims, float dt)
{
    for (auto& anim : anims)
    {
        if (anim.finished)
            continue;
        anim.elapsed += dt;
        auto it = Theme::get().keyframes.find(anim.name);
        if (it == Theme::get().keyframes.end())
        {
            anim.finished = true;
            continue;
        }
        bool fin = false;
        (void)sampleActive(anim, fin);
        if (fin)
            anim.finished = true;
    }

    anims.erase(
        std::remove_if(anims.begin(), anims.end(),
                       [](const ActiveCssAnimation& a)
                       { return a.finished && !a.fillForwards; }),
        anims.end());
}

void Anim::tickAbsolute(std::vector<ActiveCssAnimation>& anims, double now)
{
    for (auto& anim : anims)
    {
        if (anim.finished)
            continue;
        anim.elapsed = (float)(now - anim.startTime);
        auto it = Theme::get().keyframes.find(anim.name);
        if (it == Theme::get().keyframes.end())
        {
            anim.finished = true;
            continue;
        }
        bool fin = false;
        (void)sampleActive(anim, fin);
        if (fin)
            anim.finished = true;
    }

    anims.erase(
        std::remove_if(anims.begin(), anims.end(),
                       [](const ActiveCssAnimation& a)
                       { return a.finished && !a.fillForwards; }),
        anims.end());
}

void Anim::overlayCssStyle(Style& out, const std::vector<ActiveCssAnimation>& anims)
{
    for (const auto& anim : anims)
    {
        auto it = Theme::get().keyframes.find(anim.name);
        if (it == Theme::get().keyframes.end())
            continue;
        bool fin = false;
        float localT = sampleActive(anim, fin);
        if (fin && !anim.fillForwards)
            continue;
        Style frame = evaluateKeyframes(it->second, localT, anim.ease);
        out.overlay(frame);
    }
}

void Anim::overlayCssComputed(ComputedStyle& out, const std::vector<ActiveCssAnimation>& anims)
{
    for (const auto& anim : anims)
    {
        auto it = Theme::get().keyframes.find(anim.name);
        if (it == Theme::get().keyframes.end())
            continue;
        bool fin = false;
        float localT = sampleActive(anim, fin);
        if (fin && !anim.fillForwards)
            continue;
        Style frame = evaluateKeyframes(it->second, localT, anim.ease);
        overlayComputed(out, frame);
    }
}

// =========================================================================
//  IMPERATIVE
// =========================================================================

void AnimationPlayer::play(Layout& node, const std::string& name, bool playReverse)
{
    (void)node;
    auto it = imperative.find(name);
    if (it == imperative.end())
        return;

    auto& state = it->second;
    state.reverse = playReverse;
    state.playing = true;

    if (!state.reverse && state.elapsed >= state.anim->duration)
        state.elapsed = 0.0f;
    else if (state.reverse && state.elapsed <= 0.0f)
        state.elapsed = state.anim->duration;

    state.delayElapsed = state.anim->delay;
}

bool AnimationPlayer::hasActive(const Layout& node) const
{
    for (const auto& [n, s] : imperative)
        if (s.playing)
            return true;
    if (!css.empty())
        return true;
    if (hasActivePartAnims())
        return true;

    for (const auto& c : node.children)
        if (c->hasActiveAnimations())
            return true;
    return false;
}

bool AnimationPlayer::tickImperative(Layout& node, float dt)
{
    bool localBlocks = false;

    for (auto& [n, s] : imperative)
    {
        if (!s.playing)
            continue;

        if (s.anim->blocksInput)
            localBlocks = true;

        if (s.delayElapsed > 0.0f)
        {
            s.delayElapsed -= dt;
            continue;
        }

        s.elapsed += (s.reverse ? -dt : dt);
        bool justFinished = false;

        if (s.elapsed <= 0.0f)
        {
            s.elapsed = 0.0f;
            s.playing = false;
        }
        if (s.elapsed >= s.anim->duration)
        {
            s.elapsed = s.anim->duration;
             if (s.playing && !s.reverse)
                justFinished = true;
            s.playing = false;
        }

        float t = (s.anim->duration > 0.0f) ? (s.elapsed / s.anim->duration) : 1.0f;
        for (auto& track : s.anim->tracks)
            track->apply(t, &node);

        node.pendingTransition = true;

        if (justFinished)
        {
            // Copia il callback: potrebbe invalidare la mappa imperative
            // (es. removeFromParent lanciato dal callback stesso).
            auto cb = s.anim->onFinished;
            if (cb) cb();
        }
    }

    return localBlocks;
}

// =========================================================================
//  CSS NODO
// =========================================================================

void AnimationPlayer::syncCss(Layout& node)
{
    const auto& refs = node.style_.targetStyle.animations;
    Anim::syncRefs(css, refs, UIContext::get().time);
}

void AnimationPlayer::tickCss(Layout& /*node*/, float dt)
{
    Anim::tickIncremental(css, dt);
}

// =========================================================================
//  CSS PART
// =========================================================================

void AnimationPlayer::applyCssToPart(Layout& /*node*/, const std::string& partName, Style& out)
{
    const auto& refs = out.animations.is_set
        ? out.animations.value
        : std::vector<AnimationRef>{};

    auto& anims = parts[partName];
    const double now = UIContext::get().time;

    Anim::syncRefs(anims, refs, now);
    Anim::tickAbsolute(anims, now);
    Anim::overlayCssStyle(out, anims);
}

} // namespace ZenitUI