#pragma once

/**
 * \file Media.hpp
 * \brief `@media` query representation and evaluation.
 */

#include "Common.hpp"
#include "CoreTypes.hpp"

namespace ZenitUI
{

    /** \brief A single condition inside a `@media` query. */
    struct MediaCondition
    {
        enum class Kind
        {
            MinWidth,
            MaxWidth,
            MinHeight,
            MaxHeight,
            OrientationLandscape,
            OrientationPortrait,
            MinAspectRatio,
            MaxAspectRatio,
        };
        Kind kind;
        float value{0.0f};
    };

    /** \brief A conjunction of media conditions. Empty means "always matches". */
    struct MediaQuery
    {
        std::vector<MediaCondition> conditions;

        bool empty() const { return conditions.empty(); }
    };

    /**
     * \brief Evaluate a `MediaQuery` against a viewport size.
     * \param q        The query to evaluate.
     * \param viewport Current viewport in logical pixels.
     * \return `true` if every condition holds (vacuously true for an empty query).
     */
    inline bool evaluateMedia(const MediaQuery &q, Vec2 viewport)
    {
        for (const auto &c : q.conditions)
        {
            switch (c.kind)
            {
            case MediaCondition::Kind::MinWidth:
                if (viewport.x < c.value)
                    return false;
                break;
            case MediaCondition::Kind::MaxWidth:
                if (viewport.x > c.value)
                    return false;
                break;
            case MediaCondition::Kind::MinHeight:
                if (viewport.y < c.value)
                    return false;
                break;
            case MediaCondition::Kind::MaxHeight:
                if (viewport.y > c.value)
                    return false;
                break;
            case MediaCondition::Kind::OrientationLandscape:
                if (viewport.x < viewport.y)
                    return false;
                break;
            case MediaCondition::Kind::OrientationPortrait:
                if (viewport.x >= viewport.y)
                    return false;
                break;
            case MediaCondition::Kind::MinAspectRatio:
                if (viewport.y <= 0.0f || (viewport.x / viewport.y) < c.value)
                    return false;
                break;
            case MediaCondition::Kind::MaxAspectRatio:
                if (viewport.y <= 0.0f || (viewport.x / viewport.y) > c.value)
                    return false;
                break;
            }
        }
        return true;
    }

} // namespace ZenitUI