#pragma once

#include "CoreTypes.hpp"

namespace ZenitUI {

// Stato e operazioni puramente meccaniche dello scroll.
// Non conosce Layout: riceve/restituisce solo numeri. L'orchestrazione
// (quando chiamare clamp, quando leggere il wheel, ecc.) vive nel
// proprietario (ScrollView oggi, Layout in Fase 2).
struct ScrollState
{
    Vec2  offset{0.0f, 0.0f};
    Vec2  maxScroll{0.0f, 0.0f};
    Vec2  velocity{0.0f, 0.0f};

    float dragStartMouseY{0.0f};
    float dragStartOffsetY{0.0f};

    static constexpr float WHEEL_IMPULSE = 400.0f;
    static constexpr float DECAY         = 0.90f;
    static constexpr float VELOCITY_MIN  = 5.0f;

    void clamp()
    {
        offset.x = std::clamp(offset.x, 0.0f, maxScroll.x);
        offset.y = std::clamp(offset.y, 0.0f, maxScroll.y);
    }

    void reset()
    {
        offset    = {0.0f, 0.0f};
        maxScroll = {0.0f, 0.0f};
        velocity  = {0.0f, 0.0f};
    }

    // Inerzia: applica velocity e la fa decadere. Chiama clamp() in coda.
    void tickInertia(float dt)
    {
        if (std::abs(velocity.x) > VELOCITY_MIN) {
            offset.x += velocity.x * dt;
            velocity.x *= std::pow(DECAY, dt * 60.0f);
        } else {
            velocity.x = 0.0f;
        }

        if (std::abs(velocity.y) > VELOCITY_MIN) {
            offset.y += velocity.y * dt;
            velocity.y *= std::pow(DECAY, dt * 60.0f);
        } else {
            velocity.y = 0.0f;
        }

        clamp();
    }
};

} // namespace ZenitUI