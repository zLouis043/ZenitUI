// ZenitUI — Example 02: Layouts
//
// A visual tour of the three container types (Vertical, Horizontal,
// Stack) and the flexbox-inspired properties (grow, gap, justify, wrap).
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

static std::shared_ptr<Layout> Box(const std::string &text, ZenitUI::Color color)
{
    auto box = std::make_shared<Layout>(LayoutType::Stack);
    box->getInlineBase().height = Px(40);
    box->getInlineBase().minWidth = Px(60);
    box->getInlineBase().background = color;
    box->getInlineBase().radius = Px(4);
    box->getInlineBase().itemsH = Align::Center;
    box->getInlineBase().itemsV = Align::Center;
    box->getInlineBase().padding = Spacing(Px(8), Px(12));
    box->addChild(Label(text));
    return box;
}

static std::shared_ptr<Layout> DemoRow(const std::string &title,
                                       std::shared_ptr<Layout> content)
{
    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->getInlineBase().gap = Px(16);
    row->getInlineBase().itemsV = Align::Center;
    row->getInlineBase().padding = Spacing(Px(12));
    row->getInlineBase().background = ZenitUI::Color{32, 32, 38, 255};
    row->getInlineBase().radius = Px(6);
    row->getInlineBase().width = Percent(100);

    auto label = Label(title);
    label->size(Px(180), Auto());
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
    InitWindow(800, 750, "ZenitUI — 02 Layouts");
    SetTargetFPS(60);

    RaylibRenderer renderer;
    RaylibPlatform platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets = &assets;

    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().background = ZenitUI::Color{20, 20, 24, 255};
    root->getInlineBase().padding = Spacing(Px(20));
    root->getInlineBase().gap = Px(12);

    // --- 1. Vertical flow ---
    {
        auto content = std::make_shared<Layout>(LayoutType::Vertical);
        content->getInlineBase().gap = Px(6);
        content->getInlineBase().grow = 1;

        content->addChild(Box("A", ZenitUI::Color{200, 80, 80, 255}));
        content->addChild(Box("B", ZenitUI::Color{80, 200, 80, 255}));
        content->addChild(Box("C", ZenitUI::Color{80, 80, 200, 255}));

        root->addChild(DemoRow("Vertical flow", content));
    }

    // --- 2. Horizontal flow ---
    {
        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap = Px(6);
        content->getInlineBase().grow = 1;

        content->addChild(Box("A", ZenitUI::Color{200, 80, 80, 255}));
        content->addChild(Box("B", ZenitUI::Color{80, 200, 80, 255}));
        content->addChild(Box("C", ZenitUI::Color{80, 80, 200, 255}));

        root->addChild(DemoRow("Horizontal flow", content));
    }

    // --- 3. grow 1 : 2 : 1 ---
    {
        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap = Px(6);
        content->getInlineBase().grow = 1;
        content->getInlineBase().width = Percent(100);

        auto a = Box("grow 1", ZenitUI::Color{200, 80, 80, 255});
        auto b = Box("grow 2", ZenitUI::Color{80, 200, 80, 255});
        auto c = Box("grow 1", ZenitUI::Color{80, 80, 200, 255});

        a->getInlineBase().grow = 1.0f;
        b->getInlineBase().grow = 2.0f;
        c->getInlineBase().grow = 1.0f;

        content->addChild(a);
        content->addChild(b);
        content->addChild(c);

        root->addChild(DemoRow("grow 1 : 2 : 1", content));
    }

    // --- 4. justify : space-between ---
    {
        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->getInlineBase().gap = Px(6);
        content->getInlineBase().grow = 1;
        content->getInlineBase().width = Percent(100);
        content->getInlineBase().justify = Justify::SpaceBetween;

        content->addChild(Box("A", ZenitUI::Color{200, 80, 80, 255}));
        content->addChild(Box("B", ZenitUI::Color{80, 200, 80, 255}));
        content->addChild(Box("C", ZenitUI::Color{80, 80, 200, 255}));

        root->addChild(DemoRow("justify space-between", content));
    }

    // --- 5. Stack overlay ---
    {
        auto content = std::make_shared<Layout>(LayoutType::Stack);
        content->size(Px(200), Px(70));

        auto under = Box("under", ZenitUI::Color{100, 100, 130, 255});
        under->size(Px(110), Px(45));
        under->getInlineBase().position = Position::Absolute;
        under->getInlineBase().left = Px(0);
        under->getInlineBase().top = Px(0);

        auto over = Box("over", ZenitUI::Color{220, 130, 80, 255});
        over->size(Px(110), Px(45));
        over->getInlineBase().position = Position::Absolute;
        over->getInlineBase().left = Px(60);
        over->getInlineBase().top = Px(20);

        content->addChild(under);
        content->addChild(over);

        root->addChild(DemoRow("Stack overlay", content));
    }

    // --- 6. Wrap flow (Tag Cloud Reale) ---
    {
        auto title = Label("Wrap flow (Tag Cloud natively working)");
        title->getInlineBase().color = ZenitUI::Color{180, 180, 190, 255};
        title->getInlineBase().margin = Spacing(Px(10), Px(0), Px(5), Px(0));
        root->addChild(title);

        auto content = std::make_shared<Layout>(LayoutType::Horizontal);
        content->size(Percent(100), Auto());
        content->getInlineBase().gap = Px(8);
        content->getInlineBase().wrap = true;
        content->getInlineBase().justify = Justify::Center;

        content->addChild(Box("Action", ZenitUI::Color{200, 80, 80, 255}));
        content->addChild(Box("Adventure", ZenitUI::Color{80, 200, 80, 255}));
        content->addChild(Box("RPG", ZenitUI::Color{80, 80, 200, 255}));
        content->addChild(Box("Strategy", ZenitUI::Color{200, 160, 80, 255}));
        content->addChild(Box("Simulation", ZenitUI::Color{200, 80, 200, 255}));
        content->addChild(Box("Sports", ZenitUI::Color{80, 200, 200, 255}));
        content->addChild(Box("Puzzle", ZenitUI::Color{140, 140, 140, 255}));
        content->addChild(Box("Racing", ZenitUI::Color{200, 100, 100, 255}));
        content->addChild(Box("Fighting", ZenitUI::Color{100, 200, 100, 255}));

        root->addChild(content);
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