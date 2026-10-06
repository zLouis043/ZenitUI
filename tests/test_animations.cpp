#include "TestFramework.hpp"
#include "AnimPrimitives.hpp"

using namespace ZenitUI;

// ---------- evaluateKeyframes ----------

TEST(Anim_evaluateKeyframes, empty) {
    KeyframeAnimation anim;
    anim.name = "empty";
    auto s = evaluateKeyframes(anim, 0.5f, TransitionFunction::Linear);
    CHECK(!s.opacity.is_set);
}

TEST(Anim_evaluateKeyframes, single_keyframe) {
    KeyframeAnimation anim;
    Keyframe k; k.t = 0.5f; k.delta.opacity = 0.7f;
    anim.keyframes = { k };
    auto s = evaluateKeyframes(anim, 0.0f, TransitionFunction::Linear);
    CHECK(s.opacity.is_set);
    CHECK_NEAR(s.opacity.value, 0.7f, 1e-6);
}

TEST(Anim_evaluateKeyframes, two_keyframes_midpoint) {
    KeyframeAnimation anim;
    Keyframe k0; k0.t = 0.0f; k0.delta.opacity = 0.0f;
    Keyframe k1; k1.t = 1.0f; k1.delta.opacity = 1.0f;
    anim.keyframes = { k0, k1 };
    auto s = evaluateKeyframes(anim, 0.5f, TransitionFunction::Linear);
    CHECK(s.opacity.is_set);
    CHECK_NEAR(s.opacity.value, 0.5f, 1e-4);
}

TEST(Anim_evaluateKeyframes, boundaries) {
    KeyframeAnimation anim;
    Keyframe k0; k0.t = 0.0f; k0.delta.opacity = 0.0f;
    Keyframe k1; k1.t = 1.0f; k1.delta.opacity = 1.0f;
    anim.keyframes = { k0, k1 };
    CHECK_NEAR(evaluateKeyframes(anim, 0.0f, TransitionFunction::Linear).opacity.value, 0.0f, 1e-6);
    CHECK_NEAR(evaluateKeyframes(anim, 1.0f, TransitionFunction::Linear).opacity.value, 1.0f, 1e-6);
}

TEST(Anim_evaluateKeyframes, three_keyframes_segments) {
    KeyframeAnimation anim;
    Keyframe k0; k0.t = 0.0f; k0.delta.opacity = 0.0f;
    Keyframe k1; k1.t = 0.5f; k1.delta.opacity = 1.0f;
    Keyframe k2; k2.t = 1.0f; k2.delta.opacity = 0.0f;
    anim.keyframes = { k0, k1, k2 };

    CHECK_NEAR(evaluateKeyframes(anim, 0.25f, TransitionFunction::Linear).opacity.value, 0.5f, 1e-4);
    CHECK_NEAR(evaluateKeyframes(anim, 0.50f, TransitionFunction::Linear).opacity.value, 1.0f, 1e-4);
    CHECK_NEAR(evaluateKeyframes(anim, 0.75f, TransitionFunction::Linear).opacity.value, 0.5f, 1e-4);
}

TEST(Anim_evaluateKeyframes, multiple_props) {
    KeyframeAnimation anim;
    Keyframe k0; k0.t = 0.0f; k0.delta.opacity = 0.0f; k0.delta.scale = 1.0f;
    Keyframe k1; k1.t = 1.0f; k1.delta.opacity = 1.0f; k1.delta.scale = 2.0f;
    anim.keyframes = { k0, k1 };
    auto s = evaluateKeyframes(anim, 0.5f, TransitionFunction::Linear);
    CHECK_NEAR(s.opacity.value, 0.5f, 1e-4);
    CHECK_NEAR(s.scale.value,   1.5f, 1e-4);
}

TEST(Anim_evaluateKeyframes, prop_only_in_one_keyframe) {
    KeyframeAnimation anim;
    Keyframe k0; k0.t = 0.0f; k0.delta.opacity = 0.5f;
    Keyframe k1; k1.t = 1.0f; k1.delta.opacity = 1.0f; k1.delta.scale = 2.0f;
    anim.keyframes = { k0, k1 };
    auto s = evaluateKeyframes(anim, 0.5f, TransitionFunction::Linear);
    CHECK_NEAR(s.opacity.value, 0.75f, 1e-4);
    CHECK(s.scale.is_set);
    CHECK_NEAR(s.scale.value, 2.0f, 1e-4);
}

// ---------- sampleActive ----------

TEST(Anim_sampleActive, before_delay) {
    ActiveCssAnimation a;
    a.duration = 1.0f;
    a.delay    = 0.5f;
    a.elapsed  = 0.3f;
    bool fin = false;
    CHECK_NEAR(sampleActive(a, fin), 0.0f, 1e-6);
    CHECK(!fin);
}

TEST(Anim_sampleActive, duration_zero_is_finished) {
    ActiveCssAnimation a;
    a.duration = 0.0f;
    a.elapsed  = 0.0f;
    bool fin = false;
    CHECK_NEAR(sampleActive(a, fin), 1.0f, 1e-6);
    CHECK(fin);
}

TEST(Anim_sampleActive, single_iteration_midpoint) {
    ActiveCssAnimation a;
    a.duration = 1.0f; a.iterations = 1; a.elapsed = 0.5f;
    bool fin = false;
    CHECK_NEAR(sampleActive(a, fin), 0.5f, 1e-4);
    CHECK(!fin);
}

TEST(Anim_sampleActive, single_iteration_end) {
    ActiveCssAnimation a;
    a.duration = 1.0f; a.iterations = 1; a.elapsed = 1.0f;
    bool fin = false;
    CHECK_NEAR(sampleActive(a, fin), 1.0f, 1e-4);
    CHECK(fin);
}

TEST(Anim_sampleActive, overrun_capped) {
    ActiveCssAnimation a;
    a.duration = 1.0f; a.iterations = 1; a.elapsed = 3.0f;
    bool fin = false;
    CHECK_NEAR(sampleActive(a, fin), 1.0f, 1e-4);
    CHECK(fin);
}

TEST(Anim_sampleActive, three_iterations_second_half) {
    ActiveCssAnimation a;
    a.duration = 1.0f; a.iterations = 3; a.elapsed = 1.5f;
    bool fin = false;
    CHECK_NEAR(sampleActive(a, fin), 0.5f, 1e-4);
    CHECK(!fin);
}

TEST(Anim_sampleActive, alternate_reverses_odd_iterations) {
    ActiveCssAnimation a;
    a.duration = 1.0f; a.iterations = 4; a.alternate = true; a.elapsed = 1.25f;
    bool fin = false;
    // iter=1 (dispari), local=0.25 → alternate → 0.75
    CHECK_NEAR(sampleActive(a, fin), 0.75f, 1e-4);
    CHECK(!fin);
}

TEST(Anim_sampleActive, infinite_never_finishes) {
    ActiveCssAnimation a;
    a.duration = 1.0f; a.iterations = -1; a.elapsed = 100.0f;
    bool fin = false;
    CHECK_NEAR(sampleActive(a, fin), 0.0f, 1e-4);
    CHECK(!fin);
}