#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "ZenitUIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;


static std::shared_ptr<Layout> makeClickTargetAt(float x, float y,
                                                  float w, float h,
                                                  int& counter)
{
    auto n = std::make_shared<Layout>(LayoutType::Stack);
    n->size(Px(w), Px(h));
    n->setInteractive(true);
    n->setBlocksRaycast(true);
    n->onClick = [&counter] { counter++; };
    n->getInlineBase().position = Position::Absolute;
    n->getInlineBase().left = Px(x);
    n->getInlineBase().top  = Px(y);
    return n;
}

static std::shared_ptr<Layout> makePortalAt(float x, float y,
                                              float w, float h,
                                              int& counter)
{
    auto n = std::make_shared<Layout>(LayoutType::Stack);
    n->size(Px(w), Px(h));
    n->setInteractive(true);
    n->setBlocksRaycast(true);
    n->setPortal(true);
    n->onClick = [&counter] { counter++; };
    n->arrange({ x, y, w, h });
    return n;
}


TEST(Portal_timing, click_spanning_registration_frame_is_invalid)
{
    Env env;

    int normalClicks = 0;
    int portalClicks = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto normal = makeClickTargetAt(50, 50, 100, 100, normalClicks);
    root->addChild(normal);

    env.frame(root);
    env.frame(root);

    auto portal = makePortalAt(50, 50, 100, 100, portalClicks);
    root->addChild(portal);

    env.platform.pressLeft({100.0f, 100.0f});
    env.frame(root);

    env.platform.releaseLeft({100.0f, 100.0f});
    env.frame(root);

    CHECK(normalClicks == 0);
    CHECK(portalClicks == 0);
}

TEST(Portal_timing, click_after_promotion_reaches_portal)
{
    Env env;

    int normalClicks = 0;
    int portalClicks = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto normal = makeClickTargetAt(50, 50, 100, 100, normalClicks);
    root->addChild(normal);

    env.frame(root);
    env.frame(root);

    auto portal = makePortalAt(50, 50, 100, 100, portalClicks);
    root->addChild(portal);

    env.frame(root);

    env.platform.pressLeft({100.0f, 100.0f});
    env.frame(root);
    env.platform.releaseLeft({100.0f, 100.0f});
    env.frame(root);

    CHECK(portalClicks == 1);
    CHECK(normalClicks == 0);
}


TEST(Portal_timing, destroyed_portal_is_skipped)
{
    Env env;

    int portalClicks = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto portal = makePortalAt(50, 50, 100, 100, portalClicks);
    root->addChild(portal);

    env.frame(root);
    env.frame(root);

    portal->removeFromParent();
    env.frame(root);
    env.frame(root);

    env.platform.pressLeft({100.0f, 100.0f});
    env.frame(root);
    env.platform.releaseLeft({100.0f, 100.0f});
    env.frame(root);

    CHECK(portalClicks == 0);
}

TEST(Portal_timing, destroyed_portal_does_not_block_underlying)
{
    Env env;

    int normalClicks = 0;
    int portalClicks = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto normal = makeClickTargetAt(50, 50, 100, 100, normalClicks);
    root->addChild(normal);

    auto portal = makePortalAt(50, 50, 100, 100, portalClicks);
    root->addChild(portal);

    env.frame(root);
    env.frame(root);

    portal->removeFromParent();
    env.frame(root);
    env.frame(root);

    env.platform.pressLeft({100.0f, 100.0f});
    env.frame(root);
    env.platform.releaseLeft({100.0f, 100.0f});
    env.frame(root);

    CHECK(normalClicks == 1);
    CHECK(portalClicks == 0);
}


TEST(Portal_timing, last_registered_portal_wins_overlap)
{
    Env env;

    int firstClicks  = 0;
    int secondClicks = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto first  = makePortalAt(50, 50, 100, 100, firstClicks);
    auto second = makePortalAt(50, 50, 100, 100, secondClicks);

    root->addChild(first);
    root->addChild(second);

    env.frame(root);
    env.frame(root);

    env.platform.pressLeft({100.0f, 100.0f});
    env.frame(root);
    env.platform.releaseLeft({100.0f, 100.0f});
    env.frame(root);

    CHECK(secondClicks == 1);
    CHECK(firstClicks == 0);
}

TEST(Popup_callbacks, onOpen_fires)
{
    Env env;
    Theme::get().clear();

    auto popup = UI::Popup::create();
    auto content = std::make_shared<Layout>(LayoutType::Stack);
    content->addChild(UI::Label("test"));
    popup->setContent(content);

    int openCount = 0;
    popup->onOpen = [&] { openCount++; };

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(popup);

    env.frame(root);

    popup->openAt({100, 100});
    CHECK(openCount == 1);

    popup->close();
    popup->openAt({200, 200});
    CHECK(openCount == 2);
}