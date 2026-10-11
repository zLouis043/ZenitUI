#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  Scroll system — 3.1
// ============================================================

// Scene: vertical scroll container with fixed size, containing
// content that overflows. Returns root, sv, and content height.
struct VScrollScene
{
    std::shared_ptr<Layout> root;
    std::shared_ptr<Layout> sv;
    float contentH{0.0f};
};

static VScrollScene makeVScroll(Env& env,
                                 float svW = 200.0f,
                                 float svH = 100.0f,
                                 float contentH = 500.0f)
{
    VScrollScene s;
    s.contentH = contentH;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow-y: scroll; }
    )");

    s.root = std::make_shared<Layout>(LayoutType::Stack);
    s.root->size(Percent(100), Percent(100));

    s.sv = std::make_shared<Layout>(LayoutType::Vertical);
    s.sv->size(Px(svW), Px(svH));
    s.sv->cls("scroller");

    auto content = std::make_shared<Layout>(LayoutType::Stack);
    content->size(Auto(), Px(contentH));
    s.sv->addChild(content);

    s.root->addChild(s.sv);
    env.frame(s.root);
    env.frame(s.root);
    return s;
}

// ---- scrollTo / scrollToX / scrollToY --------------------------------

TEST(Scroll_offset, scrollTo_sets_absolute_position)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollTo(0.0f, 250.0f);
    CHECK_NEAR(s.sv->getScrollY(), 250.0f, 1e-3);
}

TEST(Scroll_offset, scrollToX_sets_only_x)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollTo(0.0f, 100.0f);
    s.sv->scrollToX(0.0f);
    // scrollToX con valore 0 non deve toccare l'asse Y.
    CHECK_NEAR(s.sv->getScrollY(), 100.0f, 1e-3);
}

TEST(Scroll_offset, scrollToY_sets_only_y)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToY(300.0f);
    CHECK_NEAR(s.sv->getScrollY(), 300.0f, 1e-3);
}

TEST(Scroll_offset, scrollToTop_snaps_to_zero)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToY(200.0f);
    s.sv->scrollToTop();
    CHECK_NEAR(s.sv->getScrollY(), 0.0f, 1e-3);
}

TEST(Scroll_offset, scrollToBottom_snaps_to_max)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToBottom();
    CHECK_NEAR(s.sv->getScrollY(), s.sv->getMaxScrollY(), 1e-3);
    CHECK(s.sv->getScrollY() > 0.0f);
}

TEST(Scroll_offset, scroll_is_clamped_to_max)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToY(99999.0f);
    CHECK_NEAR(s.sv->getScrollY(), s.sv->getMaxScrollY(), 1e-3);
}

TEST(Scroll_offset, scroll_is_clamped_to_zero)
{
    Env env;
    auto s = makeVScroll(env);

    s.sv->scrollToY(-100.0f);
    CHECK_NEAR(s.sv->getScrollY(), 0.0f, 1e-3);
}

// ---- max scroll ------------------------------------------------------

TEST(Scroll_max, max_is_content_minus_viewport)
{
    Env env;
    // content 500, viewport 100 → max 400
    auto s = makeVScroll(env, 200.0f, 100.0f, 500.0f);

    CHECK_NEAR(s.sv->getMaxScrollY(), 400.0f, 1.0f);
}

TEST(Scroll_max, zero_when_content_fits)
{
    Env env;
    // content 50, viewport 100 → max 0
    auto s = makeVScroll(env, 200.0f, 100.0f, 50.0f);

    CHECK_NEAR(s.sv->getMaxScrollY(), 0.0f, 1e-3);
    CHECK_NEAR(s.sv->getMaxScrollX(), 0.0f, 1e-3);
}

// ---- wheel ----------------------------------------------------------

TEST(Scroll_wheel, adds_velocity_not_offset_directly)
{
    Env env;
    auto s = makeVScroll(env);

    // Posiziona il mouse dentro la scroll view.
    Rect r = s.sv->getRect();
    env.platform.moveMouse({r.x + r.width * 0.5f,
                            r.y + r.height * 0.5f});
    env.frame(s.root);   // il mouse è ora sopra la sv

    // Un tick di wheel.
    env.platform.scrollWheel(-1.0f);
    env.frame(s.root);

    // L'offset cambia solo dopo l'inertia, ma dovrebbe già essere > 0
    // perché tickInertia gira nello stesso frame.
    CHECK(s.sv->getScrollY() > 0.0f);

    // Applica lo scroll al layout per far avanzare lo stato.
    env.frame(s.root);
    CHECK(s.sv->getScrollY() > 0.0f);
}

TEST(Scroll_wheel, wheel_outside_does_nothing)
{
    Env env;
    auto s = makeVScroll(env);

     // Mouse fuori dalla sv (che è a (0,0), 200x100).
    // Il viewport mock è 800x600, quindi (600, 400) è sicuramente
    // fuori dalla sv.
    env.platform.moveMouse({600.0f, 400.0f});
    env.frame(s.root);

    env.platform.scrollWheel(-1.0f);
    env.frame(s.root);

    CHECK_NEAR(s.sv->getScrollY(), 0.0f, 1e-3);
}

TEST(Scroll_wheel, shift_switches_to_horizontal)
{
    Env env;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .hscroller { overflow-x: scroll; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto sv = std::make_shared<Layout>(LayoutType::Horizontal);
    sv->size(Px(100), Px(80));
    sv->cls("hscroller");

    auto content = std::make_shared<Layout>(LayoutType::Stack);
    content->size(Px(500), Auto());
    sv->addChild(content);

    root->addChild(sv);
    env.frame(root);
    env.frame(root);

    Rect r = sv->getRect();
    env.platform.moveMouse({r.x + r.width * 0.5f,
                            r.y + r.height * 0.5f});
    env.platform.shiftDown = true;
    env.frame(root);

    env.platform.scrollWheel(-1.0f);
    env.frame(root);

    CHECK(sv->getScrollX() > 0.0f);
    CHECK_NEAR(sv->getScrollY(), 0.0f, 1e-3);
}

// ---- inerzia --------------------------------------------------------

TEST(Scroll_inertia, velocity_decays_over_time)
{
    Env env;
    auto s = makeVScroll(env);

    Rect r = s.sv->getRect();
    env.platform.moveMouse({r.x + r.width * 0.5f,
                            r.y + r.height * 0.5f});
    env.frame(s.root);

    env.platform.scrollWheel(-3.0f);   // forte impulso
    env.frame(s.root);

    float y0 = s.sv->getScrollY();

    // Dopo qualche frame, l'offset è cresciuto ancora.
    for (int i = 0; i < 3; ++i)
        env.frame(s.root);

    float y1 = s.sv->getScrollY();
    CHECK(y1 > y0);

    // Dopo un bel po' di frame, l'inerzia si è fermata.
    for (int i = 0; i < 60; ++i)
        env.frame(s.root);

    float y2 = s.sv->getScrollY();
    for (int i = 0; i < 10; ++i)
        env.frame(s.root);

    float y3 = s.sv->getScrollY();
    CHECK_NEAR(y3, y2, 0.5f);   // fermo

    // E non ha superato il max.
    CHECK(y3 <= s.sv->getMaxScrollY() + 0.5f);
}

TEST(Scroll_inertia, clamps_at_max_during_fling)
{
    Env env;
    // Content piccolo e scroller piccolo: max piccolo, così
    // un singolo tick di wheel supera facilmente il fondo.
    auto s = makeVScroll(env, 200.0f, 100.0f, 200.0f);

    CHECK_NEAR(s.sv->getMaxScrollY(), 100.0f, 1.0f);

    Rect r = s.sv->getRect();
    env.platform.moveMouse({r.x + r.width * 0.5f,
                            r.y + r.height * 0.5f});
    env.frame(s.root);

    env.platform.scrollWheel(-10.0f);
    env.frame(s.root);

    // L'inertia non supera mai il max.
    for (int i = 0; i < 30; ++i)
        env.frame(s.root);

    CHECK(s.sv->getScrollY() <= s.sv->getMaxScrollY() + 0.5f);
}

// ---- nested scroll --------------------------------------------------

TEST(Scroll_nested, inner_consumes_wheel)
{
    Env env;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow-y: scroll; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    // SV esterna: 300x300, content 800.
    auto outer = std::make_shared<Layout>(LayoutType::Vertical);
    outer->size(Px(300), Px(300));
    outer->cls("scroller");

    auto outerContent = std::make_shared<Layout>(LayoutType::Stack);
    outerContent->size(Auto(), Px(800));
    outer->addChild(outerContent);

    // SV interna: 200x100, dentro outerContent.
    auto inner = std::make_shared<Layout>(LayoutType::Vertical);
    inner->size(Px(200), Px(100));
    inner->cls("scroller");

    auto innerContent = std::make_shared<Layout>(LayoutType::Stack);
    innerContent->size(Auto(), Px(500));
    inner->addChild(innerContent);
    outerContent->addChild(inner);

    root->addChild(outer);
    env.frame(root);
    env.frame(root);

    // Mouse dentro l'inner.
    Rect ri = inner->getRect();
    env.platform.moveMouse({ri.x + ri.width * 0.5f,
                            ri.y + ri.height * 0.5f});
    env.frame(root);

    env.platform.scrollWheel(-3.0f);
    env.frame(root);
    env.frame(root);

    // L'inner ha scrollato, l'outer no.
    CHECK(inner->getScrollY() > 0.0f);
    CHECK_NEAR(outer->getScrollY(), 0.0f, 1e-3);
}

TEST(Scroll_nested, outer_consumes_wheel_outside_inner)
{
    Env env;

    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .scroller { overflow-y: scroll; }
    )");

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto outer = std::make_shared<Layout>(LayoutType::Vertical);
    outer->size(Px(300), Px(300));
    outer->cls("scroller");

    auto outerContent = std::make_shared<Layout>(LayoutType::Stack);
    outerContent->size(Auto(), Px(800));
    outer->addChild(outerContent);

    // Inner piccola, in alto a sinistra.
    auto inner = std::make_shared<Layout>(LayoutType::Vertical);
    inner->size(Px(100), Px(50));
    inner->cls("scroller");

    auto innerContent = std::make_shared<Layout>(LayoutType::Stack);
    innerContent->size(Auto(), Px(500));
    inner->addChild(innerContent);
    outerContent->addChild(inner);

    root->addChild(outer);
    env.frame(root);
    env.frame(root);

    // Mouse dentro l'outer ma FUORI dall'inner (che è 100x50 in alto).
    Rect ro = outer->getRect();
    env.platform.moveMouse({ro.x + 250.0f, ro.y + 200.0f});
    env.frame(root);

    env.platform.scrollWheel(-3.0f);
    env.frame(root);
    env.frame(root);

    // L'outer ha scrollato, l'inner no.
    CHECK(outer->getScrollY() > 0.0f);
    CHECK_NEAR(inner->getScrollY(), 0.0f, 1e-3);
}

// ---- scrollbar thumb drag -------------------------------------------

TEST(Scroll_thumb, click_on_thumb_starts_capture)
{
    Env env;
    auto s = makeVScroll(env);   // 200x100, content 500 → max = 400

    // Geometria del thumb verticale:
    //   MARGIN=4, TRACK_W=6
    //   trackH = 100 - 8 = 92 (no horizontal bar)
    //   viewportRatio = 100/500 = 0.2
    //   thumbH = max(30, 92*0.2) = 30
    //   thumbRect = {200-6-4, 4, 6, 30} = {190, 4, 6, 30}
    // Il centro del thumb è a (193, 19).
    env.platform.pressLeft({193.0f, 19.0f});
    env.frame(s.root);

    CHECK(s.sv->hasPointerCapture());

    // Rilascio pulito.
    env.platform.releaseLeft({193.0f, 19.0f});
    env.frame(s.root);
    CHECK(!s.sv->hasPointerCapture());
}

TEST(Scroll_thumb, drag_down_scrolls_content)
{
    Env env;
    auto s = makeVScroll(env);

    // Press sul centro del thumb (193, 19).
    env.platform.pressLeft({193.0f, 19.0f});
    env.frame(s.root);
    CHECK(s.sv->getScrollY() == 0.0f);

    // Muovi il mouse in giù di 20px. Il rapporto di scala:
    //   contentSize.y / rect.height = 500 / 100 = 5
    // Quindi 20px di mouse → 100px di scroll.
    env.platform.holdLeft({193.0f, 39.0f});
    env.frame(s.root);

    CHECK(s.sv->getScrollY() > 50.0f);
    CHECK(s.sv->getScrollY() <= s.sv->getMaxScrollY());

    // Rilascio.
    env.platform.releaseLeft({193.0f, 39.0f});
    env.frame(s.root);
    CHECK(!s.sv->hasPointerCapture());
}

TEST(Scroll_thumb, click_outside_thumb_does_not_capture)
{
    Env env;
    auto s = makeVScroll(env);

    // Click sul corpo della sv, ma non sul thumb (thumb è a x ≥ 190).
    env.platform.pressLeft({50.0f, 50.0f});
    env.frame(s.root);

    CHECK(!s.sv->hasPointerCapture());

    env.platform.releaseLeft({50.0f, 50.0f});
    env.frame(s.root);
}

TEST(Scroll_thumb, click_on_track_jumps_to_position)
{
    Env env;
    // Solo scroll verticale: la logica di "jump" si attiva solo quando
    // un solo asse è scrollabile.
    auto s = makeVScroll(env);

    // Clicca a x=100 (dentro la sv), y=80 (vicino al fondo della view).
    // La formula: ratio = 80 / 100 = 0.8
    //             offset.y = 0.8 * contentSize.y - rect.height/2
    //                      = 0.8 * 500 - 50 = 350
    // Clampato a maxScroll.y = 400 → 350.
    env.platform.pressLeft({100.0f, 80.0f});
    env.frame(s.root);

    CHECK_NEAR(s.sv->getScrollY(), 350.0f, 1.0f);

    env.platform.releaseLeft({100.0f, 80.0f});
    env.frame(s.root);
}