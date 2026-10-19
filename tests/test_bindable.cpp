#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "Bindable.hpp"
#include "UIComponents.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

// ============================================================
//  Construction and basic access
// ============================================================

TEST(Bindable_construct, default_is_value_initialized)
{
    Bindable<int> b;
    CHECK(b.get() == 0);
}

TEST(Bindable_construct, initial_value)
{
    Bindable<int> b{42};
    CHECK(b.get() == 42);
}

TEST(Bindable_construct, string_value)
{
    Bindable<std::string> b{std::string("hello")};
    CHECK(b.get() == "hello");
}

// ============================================================
//  set() and notification
// ============================================================

TEST(Bindable_set, same_value_does_not_fire_callback)
{
    Bindable<int> b{10};
    int calls = 0;
    auto sub = b.onChanged([&calls](const int&){ calls++; });

    b.set(10);
    CHECK(calls == 0);
}

TEST(Bindable_set, different_value_fires_once)
{
    Bindable<int> b{10};
    int calls = 0;
    int last  = 0;
    auto sub = b.onChanged([&](const int& v){ calls++; last = v; });

    b.set(20);
    CHECK(calls == 1);
    CHECK(last == 20);
    CHECK(b.get() == 20);
}

TEST(Bindable_set, two_subscribers_fire_in_registration_order)
{
    Bindable<int> b{0};
    std::vector<int> order;
    auto sub1 = b.onChanged([&](const int&){ order.push_back(1); });
    auto sub2 = b.onChanged([&](const int&){ order.push_back(2); });

    b.set(1);
    CHECK((int)order.size() == 2);
    CHECK(order[0] == 1);
    CHECK(order[1] == 2);
}

// ============================================================
//  Subscription RAII
// ============================================================

TEST(Bindable_subscription, raii_destroy_unsubscribes)
{
    Bindable<int> b{0};
    int calls = 0;

    {
        auto sub = b.onChanged([&](const int&){ calls++; });
        b.set(1);
        CHECK(calls == 1);
    }   // sub destroyed here

    b.set(2);
    CHECK(calls == 1);   // no additional call
}

TEST(Bindable_subscription, explicit_unsubscribe)
{
    Bindable<int> b{0};
    int calls = 0;
    auto sub = b.onChanged([&](const int&){ calls++; });

    b.set(1);
    CHECK(calls == 1);

    sub.unsubscribe();
    b.set(2);
    CHECK(calls == 1);
}

TEST(Bindable_subscription, unsubscribe_is_idempotent)
{
    Bindable<int> b{0};
    int calls = 0;
    auto sub = b.onChanged([&](const int&){ calls++; });

    sub.unsubscribe();
    sub.unsubscribe();   // no crash
    sub.unsubscribe();   // no crash

    b.set(1);
    CHECK(calls == 0);
}

TEST(Bindable_subscription, active_reflects_state)
{
    Bindable<int> b{0};
    auto sub = b.onChanged([](const int&){});
    CHECK(sub.active());

    sub.unsubscribe();
    CHECK(!sub.active());
}

TEST(Bindable_subscription, move_construction)
{
    Bindable<int> b{0};
    int calls = 0;
    auto sub1 = b.onChanged([&](const int&){ calls++; });
    auto sub2 = std::move(sub1);

    // Old sub should no longer own the subscription.
    CHECK(!sub1.active());
    CHECK(sub2.active());

    b.set(1);
    CHECK(calls == 1);   // not 2
}

// ============================================================
//  Recursion / unsubscribe during iteration
// ============================================================

TEST(Bindable_recursion, unsubscribe_in_own_callback_does_not_crash)
{
    Bindable<int> b{0};
    int calls_a = 0;
    int calls_b = 0;

    // Declare the Subscription first so the lambda can capture it by
    // reference. Assign after, so the lambda body sees the populated
    // Subscription once the callback fires.
    Bindable<int>::Subscription subA;
    subA = b.onChanged([&](const int&){
        calls_a++;
        subA.unsubscribe();   // remove self during notification
    });
    auto subB = b.onChanged([&](const int&){ calls_b++; });

    b.set(1);
    CHECK(calls_a == 1);
    CHECK(calls_b == 1);

    b.set(2);
    CHECK(calls_a == 1);   // A unsubscribed itself
    CHECK(calls_b == 2);
}

TEST(Bindable_recursion, set_same_value_in_callback_no_infinite_loop)
{
    Bindable<int> b{0};
    int calls = 0;
    auto sub = b.onChanged([&](const int& v){
        calls++;
        b.set(v);   // echo the same value — no recursion
    });

    b.set(1);
    CHECK(calls == 1);
    CHECK(b.get() == 1);
}

// ============================================================
//  bindTo — widget method binding
// ============================================================

TEST(Bindable_bindTo, float_setter)
{
    Bindable<float> vol{0.3f};
    auto pb = ProgressBar::create(0.0f);

    auto sub = vol.bindTo(pb, &ProgressBar::setValue);
    CHECK_NEAR(pb->getValue(), 0.0f, 1e-6);   // not called yet

    vol.set(0.7f);
    CHECK_NEAR(pb->getValue(), 0.7f, 1e-6);
}

TEST(Bindable_bindTo, string_setter)
{
    Bindable<std::string> name{std::string("initial")};
    auto lbl = Label("_");

    auto sub = name.bindTo(lbl, &Text::setText);
    CHECK(lbl->getText() == "_");   // not called yet

    name.set(std::string("updated"));
    CHECK(lbl->getText() == "updated");
}

TEST(Bindable_bindTo, bool_setter)
{
    Bindable<bool> enabled{true};
    auto w = std::make_shared<Layout>(LayoutType::Stack);

    auto sub = enabled.bindTo(w, &Layout::setEnabled);
    CHECK(w->getEnabled());   // initial, not from bindable

    enabled.set(false);
    CHECK(!w->getEnabled());
}

TEST(Bindable_bindTo, expired_weakptr_does_not_call)
{
    Bindable<float> vol{0.5f};
    std::weak_ptr<ProgressBar> weakPb;

    {
        auto pb = ProgressBar::create(0.0f);
        weakPb = pb;
        auto sub = vol.bindTo(weakPb, &ProgressBar::setValue);
        // pb destroyed here
    }

    // No crash, no call.
    vol.set(0.9f);
    CHECK(true);
}

TEST(Bindable_bindTo, fire_immediately_on_float)
{
    Bindable<float> vol{0.42f};
    auto pb = ProgressBar::create(0.0f);

    auto sub = vol.bindTo(pb, &ProgressBar::setValue, /*fireImmediately=*/true);
    CHECK_NEAR(pb->getValue(), 0.42f, 1e-6);
}

TEST(Bindable_bindTo, fire_immediately_on_string)
{
    Bindable<std::string> name{std::string("start")};
    auto lbl = Label("_");

    auto sub = name.bindTo(lbl, &Text::setText, /*fireImmediately=*/true);
    CHECK(lbl->getText() == "start");
}

// ============================================================
//  Lifetime: Subscription vs Bindable
// ============================================================

TEST(Bindable_lifetime, subscription_outlives_bindable)
{
    Bindable<float>::Subscription sub;

    {
        Bindable<float> b{0.0f};
        sub = b.onChanged([](const float&){});
        CHECK(sub.active());
    }   // b destroyed

    CHECK(!sub.active());
    sub.unsubscribe();   // no-op, no crash
}

TEST(Bindable_lifetime, bindable_destroyed_then_subscription_destructed)
{
    Bindable<float>::Subscription sub;
    {
        Bindable<float> b{0.0f};
        sub = b.onChanged([](const float&){});
    }
    // sub destructs at end of scope, b already gone.
    // No crash = pass.
    CHECK(true);
}

TEST(Bindable_lifetime, bindable_destroyed_then_set_not_possible)
{
    auto holder = std::make_unique<Bindable<int>>(0);
    auto sub = holder->onChanged([](const int&){});

    holder.reset();   // Bindable destroyed
    CHECK(!sub.active());
    // sub destructor runs at end of scope, must be safe
}

// ============================================================
//  Type constraints
// ============================================================

TEST(Bindable_constraints, not_copyable_not_movable)
{
    static_assert(!std::is_copy_constructible_v<Bindable<int>>);
    static_assert(!std::is_copy_assignable_v<Bindable<int>>);
    static_assert(!std::is_move_constructible_v<Bindable<int>>);
    static_assert(!std::is_move_assignable_v<Bindable<int>>);
    CHECK(true);
}

TEST(Bindable_constraints, subscription_is_move_only)
{
    static_assert(std::is_move_constructible_v<Bindable<int>::Subscription>);
    static_assert(std::is_move_assignable_v<Bindable<int>::Subscription>);
    static_assert(!std::is_copy_constructible_v<Bindable<int>::Subscription>);
    static_assert(!std::is_copy_assignable_v<Bindable<int>::Subscription>);
    CHECK(true);
}