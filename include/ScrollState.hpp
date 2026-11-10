#pragma once

/**
 * \file ScrollState.hpp
 * \brief Pure data: the scroll offset, bounds, inertia, and drag anchors.
 *
 * `ScrollState` contains no rendering or input logic; it only owns the
 * numeric state that `ScrollController` mutates. Keeping it separate lets
 * `Layout` expose `getScrollX()` / `scrollTo()` without pulling in the whole
 * controller, and makes it trivial to test.
 */

#include "CoreTypes.hpp"

namespace ZenitUI
{

    /**
     * \struct ScrollState
     * \brief Offset, bounds, velocity and drag anchors for one scroll container.
     *
     * The offset is in content space (pixels scrolled). `maxScroll` is the
     * maximum valid offset, computed by `ScrollController::updateMaxScroll`
     * from the content size and the viewport size.
     *
     * Inertia uses an exponential decay tuned for 60 Hz but made
     * frame-rate-independent by raising the decay to `dt * 60`.
     */
    struct ScrollState
    {
        Vec2 offset{0.0f, 0.0f};
        Vec2 maxScroll{0.0f, 0.0f};
        Vec2 velocity{0.0f, 0.0f};

        float dragStartMouseY{0.0f};
        float dragStartOffsetY{0.0f};
        float dragStartMouseX{0.0f};
        float dragStartOffsetX{0.0f};

        static constexpr float WHEEL_IMPULSE = 400.0f;
        static constexpr float DECAY = 0.90f;
        static constexpr float VELOCITY_MIN = 5.0f;

        /** \brief Clamp `offset` to `[0, maxScroll]` on both axes. */
        void clamp()
        {
            offset.x = std::clamp(offset.x, 0.0f, maxScroll.x);
            offset.y = std::clamp(offset.y, 0.0f, maxScroll.y);
        }

        /** \brief Reset offset, bounds and velocity to zero. */
        void reset()
        {
            offset = {0.0f, 0.0f};
            maxScroll = {0.0f, 0.0f};
            velocity = {0.0f, 0.0f};
        }

        /**
         * \brief Advance the inertial offset by \p dt seconds.
         *
         * Applies the current velocity to `offset`, then decays the velocity
         * by `pow(DECAY, dt * 60)` so that behavior is frame-rate-independent.
         * Velocity below `VELOCITY_MIN` snaps to zero to avoid infinite
         * asymptotic motion. Clamps the result to `[0, maxScroll]`.
         */
        void tickInertia(float dt)
        {
            if (std::abs(velocity.x) > VELOCITY_MIN)
            {
                offset.x += velocity.x * dt;
                velocity.x *= std::pow(DECAY, dt * 60.0f);
            }
            else
            {
                velocity.x = 0.0f;
            }

            if (std::abs(velocity.y) > VELOCITY_MIN)
            {
                offset.y += velocity.y * dt;
                velocity.y *= std::pow(DECAY, dt * 60.0f);
            }
            else
            {
                velocity.y = 0.0f;
            }

            clamp();
        }
    };

} // namespace ZenitUI