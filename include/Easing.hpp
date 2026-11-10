#pragma once

/**
 * \file Easing.hpp
 * \brief Easing curves used by transitions and CSS animations.
 */

#include "Common.hpp"
#include "CoreTypes.hpp"

namespace ZenitUI
{

    /**
     * \enum TransitionFunction
     * \brief Timing function applied between two keyframe values.
     *
     * The set matches the subset of the CSS `cubic-bezier()` presets that
     * ZenitUI supports out of the box. Custom cubic-bezier curves are not
     * exposed; use the closest preset or a `Custom` filter for arbitrary
     * motion.
     */
    enum class TransitionFunction
    {
        Linear,
        EaseInQuad,
        EaseOutQuad,
        EaseInOutQuad,
        EaseInCubic,
        EaseOutCubic,
        EaseInOutCubic,
        EaseInBack,
        EaseOutBack,
        EaseOutElastic,
        EaseOutBounce
    };

    /**
     * \brief Map a normalized time \p t in `[0, 1]` through the given easing.
     * \param t       Progress, clamped to `[0, 1]` by the function.
     * \param fn      Easing curve to apply.
     * \return Eased progress. May exceed `[0, 1]` for overshooting curves
     *         (`EaseInBack`, `EaseOutBack`, `EaseOutElastic`, `EaseOutBounce`).
     */
    inline float getRatio(float t, TransitionFunction fn)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;
        constexpr float pi = 3.14159265358979323846f;
        switch (fn)
        {
        case TransitionFunction::Linear:
            return t;
        case TransitionFunction::EaseInQuad:
            return t * t;
        case TransitionFunction::EaseOutQuad:
            return t * (2.0f - t);
        case TransitionFunction::EaseInOutQuad:
            return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        case TransitionFunction::EaseInCubic:
            return t * t * t;
        case TransitionFunction::EaseOutCubic:
            return 1.0f - std::pow(1.0f - t, 3.0f);
        case TransitionFunction::EaseInOutCubic:
            return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        case TransitionFunction::EaseInBack:
        {
            const float c1 = 1.70158f, c3 = c1 + 1.0f;
            return c3 * t * t * t - c1 * t * t;
        }
        case TransitionFunction::EaseOutBack:
        {
            const float c1 = 1.70158f, c3 = c1 + 1.0f;
            float f = t - 1.0f;
            return 1.0f + c3 * f * f * f + c1 * f * f;
        }
        case TransitionFunction::EaseOutElastic:
        {
            const float c4 = (2.0f * pi) / 3.0f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
        }
        case TransitionFunction::EaseOutBounce:
        {
            const float n1 = 7.5625f, d1 = 2.75f;
            if (t < 1.0f / d1)
                return n1 * t * t;
            if (t < 2.0f / d1)
            {
                t -= 1.5f / d1;
                return n1 * t * t + 0.75f;
            }
            if (t < 2.5f / d1)
            {
                t -= 2.25f / d1;
                return n1 * t * t + 0.9375f;
            }
            t -= 2.625f / d1;
            return n1 * t * t + 0.984375f;
        }
        }
        return t;
    }

    /**
     * \brief Parse an easing name (CSS-like) into a `TransitionFunction`.
     *
     * Case-insensitive; separators `-`, `_` and space are ignored, so
     * `"ease-in-out-quad"`, `"EaseInOutQuad"` and `"ease in out quad"` all
     * parse to `TransitionFunction::EaseInOutQuad`.
     *
     * \param name Input string view.
     * \param[out] out Set on success.
     * \return `true` if the name matched a known curve.
     */
    inline bool parseEasing(std::string_view name, TransitionFunction &out)
    {
        char buf[32];
        size_t n = 0;
        for (char c : name)
        {
            if (c == '-' || c == '_' || c == ' ')
                continue;
            if (n >= sizeof(buf) - 1)
                return false;
            buf[n++] = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        std::string_view normalized(buf, n);

        struct Entry
        {
            std::string_view key;
            TransitionFunction fn;
        };
        static const Entry table[] = {
            {"linear", TransitionFunction::Linear},
            {"easeinquad", TransitionFunction::EaseInQuad},
            {"easeoutquad", TransitionFunction::EaseOutQuad},
            {"easeinoutquad", TransitionFunction::EaseInOutQuad},
            {"easeincubic", TransitionFunction::EaseInCubic},
            {"easeoutcubic", TransitionFunction::EaseOutCubic},
            {"easeinoutcubic", TransitionFunction::EaseInOutCubic},
            {"easeinback", TransitionFunction::EaseInBack},
            {"easeoutback", TransitionFunction::EaseOutBack},
            {"easeoutelastic", TransitionFunction::EaseOutElastic},
            {"easeoutbounce", TransitionFunction::EaseOutBounce},
            {"ease", TransitionFunction::EaseInOutQuad},
            {"easein", TransitionFunction::EaseInQuad},
            {"easeout", TransitionFunction::EaseOutQuad},
            {"easeinout", TransitionFunction::EaseInOutQuad},
        };
        for (const auto &e : table)
        {
            if (normalized == e.key)
            {
                out = e.fn;
                return true;
            }
        }
        return false;
    }
}