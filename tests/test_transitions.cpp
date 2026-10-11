#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  State transitions end-to-end — 3.5
// ============================================================

// Helper: quanto vale un float dopo N frame (60fps)?
// Con transitionTime = 0.15s (default), servono 9 frame.
// Usiamo 20 frame per essere sicuri.
static void runFrames(Env& env,
                       std::shared_ptr<Layout> root,
                       int n)
{
    for (int i = 0; i < n; ++i)
        env.frame(root);
}

// ---- :hover transition ------------------------------------------------

TEST(Transitions_hover, interpolates_background_over_frames)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .btn {
            background: #000000ff;
            transition: background 0.2s ease-out;
        }
        .btn:hover { background: #ffffffff; }
    )");

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->cls("btn");
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(btn);

    // Mouse FUORI dal bottone, così il setup non triggera :hover.
    env.platform.moveMouse({500.0f, 500.0f});

    runFrames(env, root, 20);
    Color initial = btn->getStyle().background;
    CHECK(initial.r == 0x00);
    CHECK(initial.g == 0x00);
    CHECK(initial.b == 0x00);

    // Ora sposta il mouse dentro.
    env.platform.moveMouse({50.0f, 25.0f});
    env.frame(root);

    runFrames(env, root, 3);
    Color mid = btn->getStyle().background;
    CHECK(mid.r > 0x00);
    CHECK(mid.r < 0xFF);

    runFrames(env, root, 30);
    Color final = btn->getStyle().background;
    CHECK(final.r == 0xFF);
    CHECK(final.g == 0xFF);
    CHECK(final.b == 0xFF);
}

TEST(Transitions_hover, default_transition_time_applies)
{
    // Anche senza `transition:` dichiarato, il framework ha un
    // transitionTime di default (0.15s). Il cambio di stato è
    // quindi sempre interpolato, mai istantaneo.
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .btn { background: #000000ff; }
        .btn:hover { background: #ff0000ff; }
    )");

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->cls("btn");
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(btn);

    env.platform.moveMouse({500.0f, 500.0f});
    runFrames(env, root, 5);
    CHECK(btn->getStyle().background.r == 0x00);

    // Hover: con transitionTime default 0.15s, il colore è intermedio
    // dopo pochi frame.
    env.platform.moveMouse({50.0f, 25.0f});
    env.frame(root);
    runFrames(env, root, 3);

    CHECK(btn->getStyle().background.r > 0x00);
    CHECK(btn->getStyle().background.r < 0xFF);

    // Dopo abbastanza frame, arriva al target.
    runFrames(env, root, 30);
    CHECK(btn->getStyle().background.r == 0xFF);
}

TEST(Transitions_hover, snap_when_transition_time_zero)
{
    // Con `transition-time: 0s`, il cambio è istantaneo anche con
    // :hover dichiarato.
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .btn {
            background: #000000ff;
            transition-time: 0s;
        }
        .btn:hover { background: #ff0000ff; }
    )");

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->cls("btn");
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(btn);

    env.platform.moveMouse({500.0f, 500.0f});
    runFrames(env, root, 5);
    CHECK(btn->getStyle().background.r == 0x00);

    env.platform.moveMouse({50.0f, 25.0f});
    env.frame(root);

    // Snap: nessun frame extra.
    CHECK(btn->getStyle().background.r == 0xFF);
}

// ---- :checked transition (Toggle) ------------------------------------

TEST(Transitions_toggle, knob_transitions_on_checked)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        Toggle {
            width: 100px;
            height: 50px;
        }
        .toggle-knob {
            position: absolute;
            width: 30px;
            height: 30px;
            top: 10px;
            left: 10px;
            transition: left 0.2s ease-out;
        }
        Toggle:checked .toggle-knob {
            left: 60px;
        }
    )");

    auto ui = ZMarkup::build(R"(Toggle#t)");
    auto toggle = ui.find("t");
    auto knob   = toggle->children[0];

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(toggle);

    runFrames(env, root, 20);
    CHECK_NEAR(knob->getRect().x, 10.0f, 1.0f);

    // Cambia checked: il knob deve iniziare a muoversi.
    toggle->setChecked(true);
    env.frame(root);

    runFrames(env, root, 3);
    float midX = knob->getRect().x;
    CHECK(midX > 10.0f);
    CHECK(midX < 60.0f);

    // Dopo abbastanza frame, è arrivato.
    runFrames(env, root, 30);
    CHECK_NEAR(knob->getRect().x, 60.0f, 1.0f);
}

// ---- :focus transition -----------------------------------------------

TEST(Transitions_focus, border_color_changes_on_focus)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .input {
            width: 200px;
            height: 40px;
            border-width: 2px;
            border-color: #404040ff;
            transition: border-color 0.2s ease-out;
        }
        .input:focus { border-color: #4a9effff; }
    )");

    auto input = std::make_shared<Layout>(LayoutType::Stack);
    input->cls("input");
    input->setInteractive(true);
    input->setBlocksRaycast(true);
    input->setFocusable(true);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(input);

    runFrames(env, root, 20);
    CHECK(input->getStyle().borderColor.r == 0x40);

    // Dai focus.
    UIContext::get().requestFocus(input);
    env.frame(root);
    runFrames(env, root, 30);

    CHECK(input->getStyle().borderColor.r == 0x4a);
    CHECK(input->getStyle().borderColor.g == 0x9e);
}

// ---- :disabled transition --------------------------------------------

TEST(Transitions_disabled, opacity_changes_on_disable)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .btn {
            background: #4a9effff;
            opacity: 1.0;
            transition: opacity 0.2s ease-out;
        }
        .btn:disabled { opacity: 0.3; }
    )");

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->cls("btn");
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(btn);

    runFrames(env, root, 20);
    CHECK_NEAR(btn->getStyle().opacity, 1.0f, 1e-3);

    // Disabilita.
    btn->setEnabled(false);
    env.frame(root);

    runFrames(env, root, 3);
    float midOp = btn->getStyle().opacity;
    CHECK(midOp < 1.0f);
    CHECK(midOp > 0.3f);

    runFrames(env, root, 30);
    CHECK_NEAR(btn->getStyle().opacity, 0.3f, 1e-3);
}

// ---- multiple properties with different timings ---------------------
TEST(Transitions_multi, faster_property_settles_first)
{
    Env env;
    Theme::get().clear();
    ZMarkup::loadStyleString(R"(
        .btn {
            background: #000000ff;
            scale: 1.0;
            transition: background 0.5s linear,
                        scale      0.1s linear;
        }
        .btn:hover {
            background: #ffffffff;
            scale: 2.0;
        }
    )");

    auto btn = std::make_shared<Layout>(LayoutType::Stack);
    btn->cls("btn");
    btn->size(Px(100), Px(50));
    btn->setInteractive(true);
    btn->setBlocksRaycast(true);

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(btn);

    env.platform.moveMouse({500.0f, 500.0f});
    runFrames(env, root, 20);
    CHECK_NEAR(btn->getStyle().scale, 1.0f, 1e-3);

    env.platform.moveMouse({50.0f, 25.0f});
    env.frame(root);

    // Dopo ~9 frame (0.15s), scale è già completata (duration 0.1s),
    // ma background è a metà (duration 0.5s).
    runFrames(env, root, 9);

    CHECK_NEAR(btn->getStyle().scale, 2.0f, 0.05f);
    CHECK(btn->getStyle().background.r > 0x00);
    CHECK(btn->getStyle().background.r < 0xFF);

    runFrames(env, root, 30);
    CHECK(btn->getStyle().background.r == 0xFF);
}
