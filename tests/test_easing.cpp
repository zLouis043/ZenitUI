#include "TestFramework.hpp"
#include "Easing.hpp"

using namespace ZenitUI;

TEST(Easing, linear_passthrough) {
    CHECK_NEAR(getRatio(0.00f, TransitionFunction::Linear), 0.00f, 1e-6);
    CHECK_NEAR(getRatio(0.25f, TransitionFunction::Linear), 0.25f, 1e-6);
    CHECK_NEAR(getRatio(0.50f, TransitionFunction::Linear), 0.50f, 1e-6);
    CHECK_NEAR(getRatio(1.00f, TransitionFunction::Linear), 1.00f, 1e-6);
}

TEST(Easing, boundaries_all_curves) {
    for (auto fn : {
        TransitionFunction::Linear,
        TransitionFunction::EaseInQuad,
        TransitionFunction::EaseOutQuad,
        TransitionFunction::EaseInOutQuad,
        TransitionFunction::EaseInCubic,
        TransitionFunction::EaseOutCubic,
        TransitionFunction::EaseInOutCubic,
        TransitionFunction::EaseInBack,
        TransitionFunction::EaseOutBack,
        TransitionFunction::EaseOutElastic,
        TransitionFunction::EaseOutBounce,
    }) {
        CHECK_NEAR(getRatio(0.0f, fn), 0.0f, 1e-5);
        CHECK_NEAR(getRatio(1.0f, fn), 1.0f, 1e-5);
    }
}

TEST(Easing, quad_out_curve_shape) {
    // EaseOutQuad è simmetrico rispetto al centro:
    // f(0.25) = 1 - (1-0.25)^2 = 0.4375
    CHECK_NEAR(getRatio(0.25f, TransitionFunction::EaseOutQuad), 0.4375f, 1e-4);
}

TEST(Easing, in_out_quad_is_symmetric) {
    float a = getRatio(0.25f, TransitionFunction::EaseInOutQuad);
    float b = getRatio(0.75f, TransitionFunction::EaseInOutQuad);
    CHECK_NEAR(a + b, 1.0f, 1e-5);
}

TEST(Easing, out_back_overshoots) {
    // EaseOutBack supera 1 nel mezzo per poi rientrare.
    float mid = getRatio(0.7f, TransitionFunction::EaseOutBack);
    CHECK(mid > 1.0f);
}

// ------------------------------------------------------------
//  parseEasing
// ------------------------------------------------------------

TEST(Easing, parse_linear) {
    TransitionFunction fn;
    CHECK(parseEasing("linear", fn));
    CHECK(fn == TransitionFunction::Linear);
}

TEST(Easing, parse_aliases_are_normalized) {
    TransitionFunction fn;

    CHECK(parseEasing("ease-out-back", fn));
    CHECK(fn == TransitionFunction::EaseOutBack);

    CHECK(parseEasing("easeoutback", fn));
    CHECK(fn == TransitionFunction::EaseOutBack);

    CHECK(parseEasing("EaseOutBack", fn));
    CHECK(fn == TransitionFunction::EaseOutBack);

    CHECK(parseEasing("EASE_OUT_BACK", fn));
    CHECK(fn == TransitionFunction::EaseOutBack);
}

TEST(Easing, parse_generic_ease) {
    TransitionFunction fn;
    CHECK(parseEasing("ease", fn));
    CHECK(fn == TransitionFunction::EaseInOutQuad);
    CHECK(parseEasing("ease-in", fn));
    CHECK(fn == TransitionFunction::EaseInQuad);
    CHECK(parseEasing("ease-out", fn));
    CHECK(fn == TransitionFunction::EaseOutQuad);
}

TEST(Easing, parse_unknown_fails) {
    TransitionFunction fn;
    CHECK(!parseEasing("mario", fn));
    CHECK(!parseEasing("", fn));
}