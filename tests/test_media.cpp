#include "TestFramework.hpp"
#include "Media.hpp"

using namespace ZenitUI;

// ---------- evaluateMedia ----------

TEST(Media_evaluate, empty_is_always_true) {
    MediaQuery q;
    CHECK(evaluateMedia(q, {800, 600}));
    CHECK(evaluateMedia(q, {0, 0}));
}

TEST(Media_evaluate, min_width_pass) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MinWidth, 600.0f});
    CHECK(evaluateMedia(q, {800, 600}));
    CHECK(evaluateMedia(q, {600, 600}));   // boundary incluso
}

TEST(Media_evaluate, min_width_fail) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MinWidth, 800.0f});
    CHECK(!evaluateMedia(q, {799, 600}));
}

TEST(Media_evaluate, max_width_pass_fail) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MaxWidth, 800.0f});
    CHECK(evaluateMedia(q, {800, 600}));   // boundary incluso
    CHECK(evaluateMedia(q, {500, 600}));
    CHECK(!evaluateMedia(q, {801, 600}));
}

TEST(Media_evaluate, min_height) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MinHeight, 500.0f});
    CHECK(evaluateMedia(q, {100, 500}));
    CHECK(!evaluateMedia(q, {100, 499}));
}

TEST(Media_evaluate, max_height) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MaxHeight, 600.0f});
    CHECK(evaluateMedia(q, {100, 600}));
    CHECK(!evaluateMedia(q, {100, 601}));
}

TEST(Media_evaluate, orientation_landscape) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::OrientationLandscape, 0.0f});
    CHECK(evaluateMedia(q, {800, 600}));
    CHECK(evaluateMedia(q, {600, 600}));   // quadrato: x >= y → landscape
    CHECK(!evaluateMedia(q, {600, 800}));
}

TEST(Media_evaluate, orientation_portrait) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::OrientationPortrait, 0.0f});
    CHECK(evaluateMedia(q, {600, 800}));
    CHECK(!evaluateMedia(q, {800, 600}));
    CHECK(!evaluateMedia(q, {600, 600}));
}

TEST(Media_evaluate, min_aspect_ratio) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MinAspectRatio, 16.0f / 9.0f});
    CHECK(evaluateMedia(q, {1920, 1080}));       // 16:9 = esattamente
    CHECK(evaluateMedia(q, {2000, 1000}));       // > 16:9
    CHECK(!evaluateMedia(q, {1000, 1000}));      // 1:1 < 16:9
}

TEST(Media_evaluate, max_aspect_ratio) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MaxAspectRatio, 1.0f});
    CHECK(evaluateMedia(q, {1000, 1000}));       // 1:1
    CHECK(evaluateMedia(q, {500, 1000}));        // 0.5:1 < 1
    CHECK(!evaluateMedia(q, {2000, 1000}));      // 2:1 > 1
}

TEST(Media_evaluate, and_all_pass) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MinWidth, 600.0f});
    q.conditions.push_back({MediaCondition::Kind::MaxWidth, 1000.0f});
    q.conditions.push_back({MediaCondition::Kind::OrientationLandscape, 0.0f});
    CHECK(evaluateMedia(q, {800, 600}));
}

TEST(Media_evaluate, and_one_fails) {
    MediaQuery q;
    q.conditions.push_back({MediaCondition::Kind::MinWidth, 600.0f});
    q.conditions.push_back({MediaCondition::Kind::MaxWidth, 700.0f});
    CHECK(!evaluateMedia(q, {800, 600}));   // min ok, max no
}