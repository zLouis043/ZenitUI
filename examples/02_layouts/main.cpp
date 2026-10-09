// ZenitUI — Example 02: Layouts
//
// A visual tour of the three container types (Vertical, Horizontal,
// Stack) and the flexbox-inspired properties (grow, gap, justify).
//
// Build:   make examples
// Run:     make run-02_layouts

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

// A small colored box with a centered label.
static std::shared_ptr<Layout> Box(const std::string& text, ZenitUI::Color color)
{
    auto box = std::make_shared<Layout>(LayoutType::Stack);
    box->getInlineBase().height     = Px(40);
    box->getInlineBase().minWidth   = Px(60);
    box->getInlineBase().background = color;
    box->getInlineBase().radius     = Px(4);
    box->getInlineBase().itemsH     = Align::Center;
    box->getInlineBase().itemsV     = Align::Center;
    box->getInlineBase().padding    = Spacing(Px(8), Px(12));
    box->addChild(Label(text));
    return box;
}

// A demo row: a title on the left, a content node on the right.
static std::shared_ptr<Layout> DemoRow(const std::string& title,
                                       std::shared_ptr<Layout> content)
{
    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->getInlineBase().gap        = Px(16);
    row->getInlineBase().itemsV     = Align::Center;
    row->getInlineBase().padding    = Spacing(Px(12));
    row->getInlineBase().background = ZenitUI::Color{32, 32, 38, 255};
    row->getInlineBase().radius     = Px(6);
    row->getInlineBase().width      = Percent(100);

    auto label = Label(title);
    label->size(Px(200), Auto());
    label->getInlineBase().color = ZenitUI::Color{180, 180, 190, 255};

    row->addChild(label);
    row->addChild(content);
    return row;
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(800, 600, "ZenitUI — 02 Layouts");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    // -----------------------------------------------------------------------
    //  Root: a vertical flow with padding and gap
    // -----------------------------------------------------------------------
    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().background = ZenitUI::Color{20, 20, 24, 255};
    root->getInlineBase().padding    = Spacing(Px(20));
    root->getInlineBase().gap        = Px(12);

    // --- 1. Vertical flow --------------------------------------------------
    //  Children stack top-to-bottom. The cross axis (width) is controlled
    //  by items-h; here the boxes stay at their intrinsic width.
    {
        auto content = std::make_shared<Layout>(LayoutType::Vertical);
        content->getInlineBase().gap  = Px(6);
        content->getInlineBase().grow = 1;

        content->addChild(Box("A", ZenitUI::Color{200,  80,  80, 255}));
        content->addChild(Box("B", ZenitUI::Color{ 80, 200,  80, 255}));
        content->addChild(Box("C", ZenitUI::Color{ 80,  80, 200, 255}));

        root->addChild(DemoRow("Vertical flow", content));
    }

    // --- 2. Horizontal flow ------------------------------------------------
    //  Children sit side-by-side. The main axis is horizontal, so `gap`
    //  applies between them on the x axis.
    {
        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap  = Px(6);
        content->getInlineBase().grow = 1;

        content->addChild(Box("A", ZenitUI::Color{200,  80,  80, 255}));
        content->addChild(Box("B", ZenitUI::Color{ 80, 200,  80, 255}));
        content->addChild(Box("C", ZenitUI::Color{ 80,  80, 200, 255}));

        root->addChild(DemoRow("Horizontal flow", content));
    }

    // --- 3. grow 1 : 2 : 1 -------------------------------------------------
    //  With positive free space on the main axis, `grow` distributes it
    //  proportionally. Here the middle box gets twice as much as the others.
    {
        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap   = Px(6);
        content->getInlineBase().grow  = 1;
        content->getInlineBase().width = Percent(100);

        auto a = Box("grow 1", ZenitUI::Color{200,  80,  80, 255});
        auto b = Box("grow 2", ZenitUI::Color{ 80, 200,  80, 255});
        auto c = Box("grow 1", ZenitUI::Color{ 80,  80, 200, 255});

        a->getInlineBase().grow = 1.0f;
        b->getInlineBase().grow = 2.0f;
        c->getInlineBase().grow = 1.0f;

        content->addChild(a);
        content->addChild(b);
        content->addChild(c);

        root->addChild(DemoRow("grow 1 : 2 : 1", content));
    }

    // --- 4. justify : space-between ---------------------------------------
    //  When there is free space and no child grows, `justify` decides how
    //  to arrange the group. `space-between` pushes the first to the start
    //  and the last to the end, distributing the rest between.
    {
        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap     = Px(6);
        content->getInlineBase().grow    = 1;
        content->getInlineBase().width   = Percent(100);
        content->getInlineBase().justify = Justify::SpaceBetween;

        content->addChild(Box("A", ZenitUI::Color{200,  80,  80, 255}));
        content->addChild(Box("B", ZenitUI::Color{ 80, 200,  80, 255}));
        content->addChild(Box("C", ZenitUI::Color{ 80,  80, 200, 255}));

        root->addChild(DemoRow("justify space-between", content));
    }

    // --- 5. Stack overlay --------------------------------------------------
    //  A Stack has no main axis: children overlap. Their position is
    //  controlled by absolute positioning or by margins.
    {
        auto content = std::make_shared<Layout>(LayoutType::Stack);
        content->size(Px(200), Px(70));

        auto under = Box("under", ZenitUI::Color{100, 100, 130, 255});
        under->size(Px(110), Px(45));
        under->getInlineBase().position = Position::Absolute;
        under->getInlineBase().left     = Px(0);
        under->getInlineBase().top      = Px(0);

        auto over = Box("over", ZenitUI::Color{220, 130,  80, 255});
        over->size(Px(110), Px(45));
        over->getInlineBase().position = Position::Absolute;
        over->getInlineBase().left     = Px(60);
        over->getInlineBase().top      = Px(20);

        content->addChild(under);
        content->addChild(over);

        root->addChild(DemoRow("Stack overlay", content));
    }

    // -----------------------------------------------------------------------
    //  Main loop
    // -----------------------------------------------------------------------
    while (!WindowShouldClose())
    {
        root->runFrame(GetFrameTime());

        BeginDrawing();
        ClearBackground(::BLACK);
        root->renderFrame();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}