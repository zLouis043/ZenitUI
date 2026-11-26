#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "StyleParser.hpp"

using namespace ZenitUI;
using namespace ZenitUI::UI;
using namespace ZenitUI::Test;

TEST(MultiContext, independent_themes_and_viewports)
{
    Env envA;
    Env envB;

    envA.platform.viewport = {1000.0f, 600.0f};
    envB.platform.viewport = {400.0f, 300.0f};

    ZMarkup::loadStyleString(R"(
        .box { width: 50vw; height: 40px; background: #FF0000FF; }
    )", envA.ctx.theme);

    ZMarkup::loadStyleString(R"(
        .box { width: 50vw; height: 40px; background: #0000FFFF; }
    )", envB.ctx.theme);

    auto rootA = std::make_shared<Layout>(LayoutType::Stack);
    rootA->size(Percent(100), Percent(100));
    auto boxA = std::make_shared<Layout>(LayoutType::Stack);
    boxA->addClass("box");
    rootA->addChild(boxA);

    auto rootB = std::make_shared<Layout>(LayoutType::Stack);
    rootB->size(Percent(100), Percent(100));
    auto boxB = std::make_shared<Layout>(LayoutType::Stack);
    boxB->addClass("box");
    rootB->addChild(boxB);

    envA.frame(rootA);
    envB.frame(rootB);

    CHECK(boxA->context() == &envA.ctx);
    CHECK(boxB->context() == &envB.ctx);
    CHECK_NEAR(boxA->getRect().width, 500.0f, 0.5f);
    CHECK_NEAR(boxB->getRect().width, 200.0f, 0.5f);
    CHECK((boxA->getStyle().background == Color{255, 0, 0, 255}));
    CHECK((boxB->getStyle().background == Color{0, 0, 255, 255}));
}

TEST(MultiContext, isolated_focus_hover_and_pointer_capture)
{
    Env envA;
    Env envB;

    auto makeTree = []() {
        auto root = std::make_shared<Layout>(LayoutType::Stack);
        root->size(Px(200), Px(200));
        auto btn = std::make_shared<Layout>(LayoutType::Stack);
        btn->size(Px(100), Px(50));
        btn->setInteractive(true);
        btn->setFocusable(true);
        root->addChild(btn);
        return std::pair{root, btn};
    };

    auto [rootA, btnA] = makeTree();
    auto [rootB, btnB] = makeTree();

    envA.frame(rootA);
    envB.frame(rootB);

    envA.platform.pressLeft({50.0f, 25.0f});
    envA.frame(rootA);
    btnA->capturePointer();

    envB.platform.moveMouse({180.0f, 180.0f});
    envB.frame(rootB);

    CHECK(envA.ctx.hasFocus(btnA.get()));
    CHECK(!envB.ctx.hasFocus(btnA.get()));
    CHECK(!envB.ctx.hasFocus(btnB.get()));
    CHECK(btnA->hasPointerCapture());
    CHECK(!btnB->hasPointerCapture());
    CHECK(btnA->isHoveredState());
    CHECK(!btnB->isHoveredState());

    envB.ctx.requestFocus(btnB);
    CHECK(envA.ctx.hasFocus(btnA.get()));
    CHECK(envB.ctx.hasFocus(btnB.get()));
}

TEST(MultiContext, isolated_filters_and_target_pools)
{
    Env envA;
    Env envB;

    envA.renderer.supportsEffects = true;
    envA.renderer.supportsNestedTargets = true;
    envA.renderer.supportsTargetSubRect = true;
    envB.renderer.supportsEffects = true;
    envB.renderer.supportsNestedTargets = true;
    envB.renderer.supportsTargetSubRect = true;

    int callsA = 0;
    int callsB = 0;
    envA.ctx.filters.addCustom("onlyA", [&](const FilterContext &c) {
        CHECK(c.ui == &envA.ctx);
        ++callsA;
    });
    envB.ctx.filters.addCustom("onlyB", [&](const FilterContext &c) {
        CHECK(c.ui == &envB.ctx);
        ++callsB;
    });

    ZMarkup::loadStyleString(".fx { width: 100px; height: 100px; filter: onlyA; }", envA.ctx.theme);
    ZMarkup::loadStyleString(".fx { width: 100px; height: 100px; filter: onlyA; }", envB.ctx.theme);

    auto rootA = std::make_shared<Layout>(LayoutType::Stack);
    rootA->size(Px(200), Px(200));
    auto nodeA = std::make_shared<Layout>(LayoutType::Stack);
    nodeA->addClass("fx");
    rootA->addChild(nodeA);

    auto rootB = std::make_shared<Layout>(LayoutType::Stack);
    rootB->size(Px(200), Px(200));
    auto nodeB = std::make_shared<Layout>(LayoutType::Stack);
    nodeB->addClass("fx");
    rootB->addChild(nodeB);

    envA.frame(rootA);
    envB.frame(rootB);

    CHECK(callsA == 1);
    CHECK(callsB == 0);
    CHECK(envA.ctx.filters.find("onlyB") == nullptr);
    CHECK(envB.ctx.filters.find("onlyA") == nullptr);
    CHECK(envA.ctx.targetPool.liveCount() > 0);
    CHECK(envB.ctx.targetPool.liveCount() > 0);

    envA.ctx.targetPool.clear(envA.renderer, &envA.ctx.renderStats);
    CHECK(envA.ctx.targetPool.liveCount() == 0);
    CHECK(envB.ctx.targetPool.liveCount() > 0);
}

TEST(MultiContext, reparenting_subtree_switches_context_and_clears_old_focus)
{
    Env envA;
    Env envB;

    ZMarkup::loadStyleString(".card { background: #112233FF; }", envA.ctx.theme);
    ZMarkup::loadStyleString(".card { background: #445566FF; }", envB.ctx.theme);

    auto rootA = std::make_shared<Layout>(LayoutType::Stack);
    rootA->size(Px(200), Px(200));
    auto rootB = std::make_shared<Layout>(LayoutType::Stack);
    rootB->size(Px(200), Px(200));

    auto card = std::make_shared<Layout>(LayoutType::Stack);
    card->addClass("card");
    card->size(Px(80), Px(40));
    card->setFocusable(true);
    auto child = std::make_shared<Layout>(LayoutType::Stack);
    card->addChild(child);
    rootA->addChild(card);

    envA.frame(rootA);
    envB.frame(rootB);

    envA.ctx.requestFocus(card);
    CHECK(envA.ctx.hasFocus(card.get()));
    CHECK(card->context() == &envA.ctx);
    CHECK(child->context() == &envA.ctx);
    CHECK((card->getStyle().background == Color{0x11, 0x22, 0x33, 0xFF}));

    rootA->removeChild(card.get());
    CHECK(card->context() == nullptr);
    CHECK(child->context() == nullptr);
    CHECK(!envA.ctx.hasFocus(card.get()));

    rootB->addChild(card);
    CHECK(card->context() == &envB.ctx);
    CHECK(child->context() == &envB.ctx);

    envB.frame(rootB);
    CHECK((card->getStyle().background == Color{0x44, 0x55, 0x66, 0xFF}));
}

TEST(MultiContext, context_destroyed_before_layout_is_safe)
{
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(100), Px(100));
    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    child->setFocusable(true);
    root->addChild(child);

    {
        Env env;
        env.frame(root);
        env.ctx.requestFocus(child);
        CHECK(root->context() == &env.ctx);
        CHECK(child->context() == &env.ctx);
    }

    CHECK(root->context() == nullptr);
    CHECK(child->context() == nullptr);

    // Mutating and destroying detached nodes after UIContext teardown must remain safe.
    child->setVisible(false);
    child->setEnabled(false);
    root->removeAllChildren();
}

TEST(MultiContext, backend_and_context_teardown_order_independent)
{
    // Order 1: Backend destroyed before UIContext.
    {
        UIContext ctx;
        {
            MockBackend backend;
            WindowConfig cfg;
            cfg.width = 640;
            cfg.height = 480;
            CHECK(backend.initAndBind(cfg, ctx));
            CHECK(backend.boundContext() == &ctx);
            CHECK(ctx.renderer == &backend.renderer());
            backend.shutdown();
            CHECK(backend.boundContext() == nullptr);
            CHECK(ctx.renderer == nullptr);
        }
        CHECK(ctx.platform == nullptr);
    }

    // Order 2: UIContext destroyed before Backend.
    {
        MockBackend backend;
        WindowConfig cfg;
        cfg.width = 640;
        cfg.height = 480;
        CHECK(backend.init(cfg));
        {
            UIContext ctx;
            backend.bindTo(ctx);
            CHECK(backend.boundContext() == &ctx);
            CHECK(ctx.renderer == &backend.renderer());
        }
        CHECK(backend.boundContext() == nullptr);
        backend.shutdown();
    }
}

TEST(MultiContext, mounted_primary_root_short_overloads)
{
    Env env;
    env.platform.viewport = {640.0f, 480.0f};

    {
        auto root = std::make_shared<Layout>(LayoutType::Stack);
        root->size(Percent(100), Percent(100));
        root->getInlineBase().background = Colors::Red;

        CHECK(env.ctx.mountedRoot() == nullptr);
        env.ctx.mount(root);
        CHECK(env.ctx.mountedRoot() == root.get());

        env.ctx.runFrame(1.0f / 60.0f);
        env.ctx.renderFrame();

        CHECK_NEAR(root->getRect().width, 640.0f, 0.5f);
        CHECK_NEAR(root->getRect().height, 480.0f, 0.5f);
        CHECK(env.renderer.fillRectCount > 0);

        env.ctx.unmount();
        CHECK(env.ctx.mountedRoot() == nullptr);
        CHECK(root->context() == nullptr);

        // No-op when no root is mounted.
        env.renderer.fillRectCount = 0;
        env.ctx.runFrame(1.0f / 60.0f);
        env.ctx.renderFrame();
        CHECK(env.renderer.fillRectCount == 0);

        // Re-mount and let root go out of scope: primaryRoot_ must clear automatically.
        env.ctx.mount(root);
        CHECK(env.ctx.mountedRoot() == root.get());
    }

    CHECK(env.ctx.mountedRoot() == nullptr);
    env.ctx.runFrame(1.0f / 60.0f);
    env.ctx.renderFrame();
}