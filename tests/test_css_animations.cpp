#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  CSS animations end-to-end — 3.6
// ============================================================

// A minimal widget that exposes the protected `partStyle` so tests
// can observe the resolved ::part Style.
class TestWidget : public TLayout<TestWidget>
{
public:
    TestWidget() : TLayout<TestWidget>(LayoutType::Stack)
    {
        setStyleTag("TestWidget");
    }

    Style exposePartStyle(const std::string& name)
    {
        return partStyle(name);
    }
};

// ---- hasActiveAnimations() ------------------------------------------

TEST(CssAnim_hasActive, false_without_animation)
{
    Env env;
    Theme::get().clear();

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->size(Px(100), Px(100));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    for (int i = 0; i < 5; ++i) env.frame(root);
    CHECK(!node->hasActiveAnimations());
}

TEST(CssAnim_hasActive, true_while_running)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes pulse {
            0%   { opacity: 0.0; }
            50%  { opacity: 1.0; }
            100% { opacity: 0.0; }
        }
        .pulse { animation: pulse 0.3s linear infinite; }
    )");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->cls("pulse");
    node->size(Px(100), Px(100));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    for (int i = 0; i < 5; ++i) env.frame(root);
    CHECK(node->hasActiveAnimations());
}

TEST(CssAnim_hasActive, infinite_stays_active_forever)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes spin {
            0%   { rotation: 0; }
            100% { rotation: 360; }
        }
        .spin { animation: spin 0.1s linear infinite; }
    )");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->cls("spin");
    node->size(Px(100), Px(100));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    for (int i = 0; i < 60; ++i) env.frame(root);
    CHECK(node->hasActiveAnimations());
}

TEST(CssAnim_hasActive, finite_removes_after_finish)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes fade {
            0%   { opacity: 0.0; }
            100% { opacity: 1.0; }
        }
        .fade { animation: fade 0.1s linear; }
    )");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->cls("fade");
    node->size(Px(100), Px(100));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    // 0.1s ≈ 6 frame. 30 frame per sicurezza.
    for (int i = 0; i < 30; ++i) env.frame(root);
    CHECK(!node->hasActiveAnimations());
}

TEST(CssAnim_hasActive, forwards_stays_active_after_finish)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes fade {
            0%   { opacity: 0.0; }
            100% { opacity: 1.0; }
        }
        .fade { animation: fade 0.1s linear forwards; }
    )");

    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->cls("fade");
    node->size(Px(100), Px(100));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(node);

    // L'animazione è finita, ma `forwards` la tiene nella lista
    // (visivamente il nodo resta nello stato finale).
    for (int i = 0; i < 30; ++i) env.frame(root);
    CHECK(node->hasActiveAnimations());
}

// ---- overlay of keyframe values onto a Style ------------------------

TEST(CssAnim_overlay, applies_at_midpoint)
{
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes fade {
            0%   { opacity: 0.0; }
            100% { opacity: 1.0; }
        }
    )");

    ActiveCssAnimation a;
    a.name      = "fade";
    a.duration  = 1.0f;
    a.iterations = 1;
    a.elapsed   = 0.5f;
    a.ease      = TransitionFunction::Linear;

    Style s;
    std::vector<ActiveCssAnimation> anims = { a };
    Anim::overlayCssStyle(s, anims);

    CHECK(s.opacity.is_set);
    CHECK_NEAR(s.opacity.value, 0.5f, 0.05f);
}

TEST(CssAnim_overlay, skips_finished_non_forwards)
{
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes fade {
            0%   { opacity: 0.0; }
            100% { opacity: 1.0; }
        }
    )");

    ActiveCssAnimation a;
    a.name      = "fade";
    a.duration  = 1.0f;
    a.iterations = 1;
    a.elapsed   = 2.0f;    // oltre la durata
    a.fillForwards = false;

    Style s;
    std::vector<ActiveCssAnimation> anims = { a };
    Anim::overlayCssStyle(s, anims);

    // Finita e senza forwards → lo Style resta intatto.
    CHECK(!s.opacity.is_set);
}

TEST(CssAnim_overlay, applies_finished_with_forwards)
{
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes fade {
            0%   { opacity: 0.0; }
            100% { opacity: 1.0; }
        }
    )");

    ActiveCssAnimation a;
    a.name      = "fade";
    a.duration  = 1.0f;
    a.iterations = 1;
    a.elapsed   = 2.0f;
    a.fillForwards = true;

    Style s;
    std::vector<ActiveCssAnimation> anims = { a };
    Anim::overlayCssStyle(s, anims);

    // Forwards: il valore finale (1.0) è applicato anche dopo la fine.
    CHECK(s.opacity.is_set);
    CHECK_NEAR(s.opacity.value, 1.0f, 0.05f);
}

TEST(CssAnim_overlay, alternate_reverses_at_odd_iterations)
{
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes fade {
            0%   { opacity: 0.0; }
            100% { opacity: 1.0; }
        }
    )");

    ActiveCssAnimation a;
    a.name      = "fade";
    a.duration  = 1.0f;
    a.iterations = 4;
    a.alternate = true;
    a.elapsed   = 1.75f;   // iterazione 1 (dispari), localT = 0.75
    a.ease      = TransitionFunction::Linear;

    Style s;
    std::vector<ActiveCssAnimation> anims = { a };
    Anim::overlayCssStyle(s, anims);

    // Iterazione dispari → t invertito: 1 - 0.75 = 0.25.
    // Linear 0→1 a t=0.25 → opacity 0.25.
    CHECK(s.opacity.is_set);
    CHECK_NEAR(s.opacity.value, 0.25f, 0.05f);
}

TEST(CssAnim_overlay, alternate_does_not_reverse_at_even_iterations)
{
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes fade {
            0%   { opacity: 0.0; }
            100% { opacity: 1.0; }
        }
    )");

    ActiveCssAnimation a;
    a.name      = "fade";
    a.duration  = 1.0f;
    a.iterations = 4;
    a.alternate = true;
    a.elapsed   = 0.75f;   // iterazione 0 (pari), localT = 0.75
    a.ease      = TransitionFunction::Linear;

    Style s;
    std::vector<ActiveCssAnimation> anims = { a };
    Anim::overlayCssStyle(s, anims);

    // Iterazione pari → nessun reverse. Linear a t=0.75 → opacity 0.75.
    CHECK(s.opacity.is_set);
    CHECK_NEAR(s.opacity.value, 0.75f, 0.05f);
}

// ---- ::part animation ------------------------------------------------

TEST(CssAnim_part, animation_overlays_part_style)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        @keyframes glow {
            0%   { color: #ff0000ff; }
            100% { color: #00ff00ff; }
        }
        TestWidget::star {
            color: #000000ff;
            animation: glow 1s linear infinite;
        }
    )");

    auto w = TestWidget::create();
    w->size(Px(100), Px(100));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(w);

    // Primo frame: la part animation viene inizializzata con
    // startTime = 0.
    env.frame(root);
    Style initial = w->exposePartStyle("star");

    CHECK(initial.color.is_set);
    // Il colore è dominato dal rosso (inizio dell'animazione).
    CHECK(initial.color.value.r > 100);
    CHECK(initial.color.value.g < 100);

    // Avanza di ~0.5s (30 frame a 60fps).
    for (int i = 0; i < 30; ++i)
        env.frame(root);

    Style mid = w->exposePartStyle("star");
    CHECK(mid.color.is_set);
    // A metà strada: rosso e verde entrambi medi.
    CHECK(mid.color.value.r > 50);
    CHECK(mid.color.value.g > 50);

    // E il colore è *diverso* dal base nero.
    CHECK(!(mid.color.value.r == 0 &&
            mid.color.value.g == 0 &&
            mid.color.value.b == 0));
}