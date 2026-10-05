#include "TestFramework.hpp"
#include "CoreTypes.hpp"

using namespace ZenitUI;

// ---------- Vec2 ----------
TEST(CoreTypes_Vec2, add) {
    Vec2 a{1, 2}, b{3, 4};
    Vec2 c = a + b;
    CHECK_NEAR(c.x, 4.0f, 1e-6);
    CHECK_NEAR(c.y, 6.0f, 1e-6);
}

TEST(CoreTypes_Vec2, sub) {
    Vec2 a{5, 5}, b{2, 3};
    Vec2 c = a - b;
    CHECK_NEAR(c.x, 3.0f, 1e-6);
    CHECK_NEAR(c.y, 2.0f, 1e-6);
}

TEST(CoreTypes_Vec2, scalar_mul) {
    Vec2 a{2, 3};
    Vec2 c = a * 2.0f;
    CHECK_NEAR(c.x, 4.0f, 1e-6);
    CHECK_NEAR(c.y, 6.0f, 1e-6);
}

// ---------- Rect ----------
TEST(CoreTypes_Rect, contains_inside) {
    Rect r{10, 20, 100, 50};
    CHECK(r.contains({50, 40}));
}

TEST(CoreTypes_Rect, contains_edges_inclusive) {
    Rect r{0, 0, 100, 100};
    CHECK(r.contains({0, 0}));
    CHECK(r.contains({100, 100}));
    CHECK(r.contains({0, 100}));
    CHECK(r.contains({100, 0}));
}

TEST(CoreTypes_Rect, contains_outside) {
    Rect r{0, 0, 100, 100};
    CHECK(!r.contains({-1, 50}));
    CHECK(!r.contains({101, 50}));
    CHECK(!r.contains({50, -1}));
    CHECK(!r.contains({50, 101}));
}

TEST(CoreTypes_Rect, center_and_size) {
    Rect r{10, 20, 100, 50};
    Vec2 c = r.center();
    CHECK_NEAR(c.x, 60.0f, 1e-6);
    CHECK_NEAR(c.y, 45.0f, 1e-6);
    Vec2 s = r.size();
    CHECK_NEAR(s.x, 100.0f, 1e-6);
    CHECK_NEAR(s.y, 50.0f,  1e-6);
}

// ---------- Color ----------
TEST(CoreTypes_Color, withAlpha) {
    Color c{100, 150, 200, 255};
    Color half = c.withAlpha(0.5f);
    CHECK(half.r == 100);
    CHECK(half.g == 150);
    CHECK(half.b == 200);
    CHECK(half.a == 128);
}

TEST(CoreTypes_Color, withAlpha_clamps) {
    Color c{100, 100, 100, 200};
    CHECK(c.withAlpha(2.0f).a == 200);
    CHECK(c.withAlpha(-1.0f).a == 0);
}

TEST(CoreTypes_Color, eq) {
    CHECK((Color{1,2,3,4}) == (Color{1,2,3,4}));
    CHECK((Color{1,2,3,4}) != (Color{1,2,3,5}));
}

// ---------- Transform2D ----------
TEST(CoreTypes_Transform, identity_apply) {
    Transform2D t;
    t.pivot = {0, 0};
    t.translate = {0, 0};
    t.rotationDeg = 0;
    t.scale = 1;
    Vec2 p = applyTransform(t, {10, 20});
    CHECK_NEAR(p.x, 10.0f, 1e-5);
    CHECK_NEAR(p.y, 20.0f, 1e-5);
}

TEST(CoreTypes_Transform, translate) {
    Transform2D t;
    t.pivot = {0, 0};
    t.translate = {5, -3};
    Vec2 p = applyTransform(t, {10, 20});
    CHECK_NEAR(p.x, 15.0f, 1e-5);
    CHECK_NEAR(p.y, 17.0f, 1e-5);
}

TEST(CoreTypes_Transform, scale_around_pivot) {
    Transform2D t;
    t.pivot = {10, 10};
    t.scale = 2;
    Vec2 p = applyTransform(t, {15, 15});
    CHECK_NEAR(p.x, 20.0f, 1e-5);
    CHECK_NEAR(p.y, 20.0f, 1e-5);
}

TEST(CoreTypes_Transform, rotation_90deg) {
    Transform2D t;
    t.pivot = {0, 0};
    t.rotationDeg = 90;
    Vec2 p = applyTransform(t, {1, 0});
    CHECK_NEAR(p.x, 0.0f, 1e-4);
    CHECK_NEAR(p.y, 1.0f, 1e-4);
}

TEST(CoreTypes_Transform, inverse_roundtrip) {
    Transform2D t;
    t.pivot       = {30, 40};
    t.translate   = {10, -5};
    t.rotationDeg = 33;
    t.scale       = 1.7f;

    Vec2 original    = {12.0f, 7.0f};
    Vec2 transformed = applyTransform(t, original);
    Vec2 back        = applyInverseTransform(t, transformed);
    CHECK_NEAR(back.x, original.x, 1e-3);
    CHECK_NEAR(back.y, original.y, 1e-3);
}