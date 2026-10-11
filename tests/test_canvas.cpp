#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "UIComponents.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

// ============================================================
//  CanvasLayout
// ============================================================

TEST(Canvas, pushes_offset_transform_inside_target)
{
    Env env;

    auto canvas = CanvasLayout::create();
    canvas->size(Px(200), Px(150));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->addChild(canvas);

    env.frame(root);

    Rect r = canvas->getRect();

    // Il canvas deve pushare una transform di offset mentre disegna
    // dentro il target. Serve a portare il subtree in coordinate
    // locali del target (0,0 in alto a sinistra), invece che disegnarlo
    // alle coordinate schermo (bug 1.9).
    bool found = false;
    for (const auto& t : env.renderer.transformHistory)
    {
        if (t.pivot.x == 0.0f && t.pivot.y == 0.0f &&
            t.translate.x == -r.x && t.translate.y == -r.y)
        {
            found = true;
            break;
        }
    }
    CHECK(found);
}

TEST(Canvas, offset_transform_matches_canvas_rect)
{
    Env env;

    auto canvas = CanvasLayout::create();
    canvas->size(Px(180), Px(120));

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().padding = Spacing(Px(30));   // canvas non parte da (0,0)
    root->addChild(canvas);

    env.frame(root);

    Rect r = canvas->getRect();
    CHECK(r.x > 0.0f);   // sanity: il padding ha spostato il canvas
    CHECK(r.y > 0.0f);

    bool found = false;
    for (const auto& t : env.renderer.transformHistory)
    {
        if (t.pivot.x == 0.0f && t.pivot.y == 0.0f &&
            t.translate.x == -r.x && t.translate.y == -r.y)
        {
            found = true;
            break;
        }
    }
    CHECK(found);
}