#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;

// ============================================================
//  Portal hit-test timing — 3.8
// ============================================================
//
// Portals are hit-tested against `activePortals`, which is the
// previous frame's `framePortals`. The consequence is:
//
//   - A portal that renders in frame N is registered in framePortals.
//   - At the start of frame N+1, framePortals becomes activePortals.
//   - So the portal becomes hit-testable starting from frame N+1.
//
// These tests verify this one-frame delay, and that destroyed
// portals are skipped.

// Helper: a clickable normal node at a known position.
static std::shared_ptr<Layout> makeClickTargetAt(float x, float y,
                                                  float w, float h,
                                                  int& counter)
{
    auto n = std::make_shared<Layout>(LayoutType::Stack);
    n->size(Px(w), Px(h));
    n->setInteractive(true);
    n->setBlocksRaycast(true);
    n->onClick = [&counter] { counter++; };
    // Forziamo la posizione via absolute positioning.
    n->getInlineBase().position = Position::Absolute;
    n->getInlineBase().left = Px(x);
    n->getInlineBase().top  = Px(y);
    return n;
}

// Helper: a clickable portal at a known position.
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
    // Per i portal l'arrange è gestito diversamente; forziamo la rect.
    n->arrange({ x, y, w, h });
    return n;
}

// ---- hit-test delay --------------------------------------------------

// Se press e release cadono a cavallo del frame in cui il portal
// viene registrato, il click è invalido:
//   - al press, il portal NON è ancora in activePortals → pressTarget = normal
//   - al release, il portal è stato promosso → releaseTarget = portal
//   - pressTarget != releaseTarget → nessuno riceve onClick
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

    // Aggiungi il portal senza farlo ancora promuovere.
    auto portal = makePortalAt(50, 50, 100, 100, portalClicks);
    root->addChild(portal);

    // Frame N: press. activePortals NON contiene il portal
    // (il draw di questo frame lo registrerà in framePortals).
    env.platform.pressLeft({100.0f, 100.0f});
    env.frame(root);

    // Frame N+1: release. beginFrame ha promosso framePortals →
    // activePortals, quindi releaseTarget = portal.
    env.platform.releaseLeft({100.0f, 100.0f});
    env.frame(root);

    // pressTarget (normal) != releaseTarget (portal) → no click.
    CHECK(normalClicks == 0);
    CHECK(portalClicks == 0);
}

// Un click che inizia DOPO la promozione raggiunge il portal.
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

    // Promuovi il portal in activePortals.
    env.frame(root);

    // Ora un click completo va al portal.
    env.platform.pressLeft({100.0f, 100.0f});
    env.frame(root);
    env.platform.releaseLeft({100.0f, 100.0f});
    env.frame(root);

    CHECK(portalClicks == 1);
    CHECK(normalClicks == 0);
}

// ---- destroyed portals are skipped ----------------------------------

TEST(Portal_timing, destroyed_portal_is_skipped)
{
    Env env;

    int portalClicks = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto portal = makePortalAt(50, 50, 100, 100, portalClicks);
    root->addChild(portal);

    // Promuovi ad activePortals.
    env.frame(root);
    env.frame(root);

    // Rimuovi il portal.
    portal->removeFromParent();
    env.frame(root);
    env.frame(root);

    // Click dove c'era il portal: non deve crashare, né chiamare
    // la callback.
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

    // Distruggi il portal: ora il normale deve ricevere i click.
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

// ---- multiple portals: order ----------------------------------------

TEST(Portal_timing, last_registered_portal_wins_overlap)
{
    Env env;

    int firstClicks  = 0;
    int secondClicks = 0;

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto first  = makePortalAt(50, 50, 100, 100, firstClicks);
    auto second = makePortalAt(50, 50, 100, 100, secondClicks);

    // Aggiungi nell'ordine: first, poi second. Nella iterazione
    // reverse, `second` viene testato per primo (è l'ultimo della
    // lista, quindi il primo in reverse).
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