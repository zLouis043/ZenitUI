#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

// ============================================================
//  Slider::setValue
// ============================================================

TEST(Widget_slider, setValue_stores_value)
{
    Env env;

    auto s = Slider::create(0.5f);
    CHECK_NEAR(s->getValue(), 0.5f, 1e-6);

    s->setValue(0.8f);
    CHECK_NEAR(s->getValue(), 0.8f, 1e-6);
}

TEST(Widget_slider, setValue_clamps_above_one)
{
    Env env;
    auto s = Slider::create(0.5f);
    s->setValue(2.5f);
    CHECK_NEAR(s->getValue(), 1.0f, 1e-6);
}

TEST(Widget_slider, setValue_clamps_below_zero)
{
    Env env;
    auto s = Slider::create(0.5f);
    s->setValue(-3.0f);
    CHECK_NEAR(s->getValue(), 0.0f, 1e-6);
}

TEST(Widget_slider, setValue_fires_callback_when_changed)
{
    Env env;

    float last = -1.0f;
    int calls = 0;

    auto s = Slider::create(0.2f);
    s->onValueChanged = [&](float v){ last = v; calls++; };

    s->setValue(0.7f);

    CHECK(calls == 1);
    CHECK_NEAR(last, 0.7f, 1e-6);
}

TEST(Widget_slider, setValue_does_not_fire_when_unchanged)
{
    Env env;

    int calls = 0;
    auto s = Slider::create(0.4f);
    s->onValueChanged = [&](float){ calls++; };

    s->setValue(0.4f);   // stesso valore
    CHECK(calls == 0);
}

TEST(Widget_slider, setValue_with_notify_false_suppresses_callback)
{
    Env env;

    int calls = 0;
    auto s = Slider::create(0.1f);
    s->onValueChanged = [&](float){ calls++; };

    s->setValue(0.9f, /*notify=*/false);

    CHECK(calls == 0);
    CHECK_NEAR(s->getValue(), 0.9f, 1e-6);
}

TEST(Widget_slider, setValue_after_clamp_still_notifies_once)
{
    // Chiamare setValue(2.0) deve clampare a 1.0 e notificare una sola
    // volta (non una per il clamp, una per il valore finale).
    Env env;

    int calls = 0;
    auto s = Slider::create(0.0f);
    s->onValueChanged = [&](float){ calls++; };

    s->setValue(2.0f);
    CHECK(calls == 1);
    CHECK_NEAR(s->getValue(), 1.0f, 1e-6);
}