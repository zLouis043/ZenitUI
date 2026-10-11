// ZenitUI — Example 08: Portals
//
// A portal is a node that renders and hit-tests as if it lived at the
// root of the tree, regardless of its logical parent. This is how
// tooltips, popups, dropdown menus and context menus escape the
// clipping and z-order of their container.
//
// The example shows four portal-based widgets:
//
//   1. Tooltip       — attached to a button via Tooltip::attach
//   2. Popup         — opened below a trigger with openBelow
//   3. ContextMenu   — opened at the mouse position on right-click
//   4. Modal         — full-screen overlay with an intro animation
//
// Build:   make examples
// Run:     make run-08_portals

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<Layout> Row(const std::string &label,
                                   std::shared_ptr<Layout> content)
{
    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->cls("row");

    auto l = Label(label);
    l->cls("row-label");

    row->addChild(l);
    row->addChild(content);
    return row;
}

static std::shared_ptr<Button> ActionButton(const std::string &text,
                                            std::function<void()> cb)
{
    auto b = Btn(text, std::move(cb));
    b->cls("btn");
    return b;
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(800, 560, "ZenitUI — 08 Portals");
    SetTargetFPS(60);

    RaylibRenderer renderer;
    RaylibPlatform platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets = &assets;

    ZMarkup::loadStyleFile("examples/08_portals/theme.zstyle");

    // -----------------------------------------------------------------------
    //  Root: a Stack that fills the viewport. Two children — the page and
    //  the modal — overlap. The modal is declared after the page, so it
    //  draws on top.
    // -----------------------------------------------------------------------
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->setStyleTag("Stack");
    root->id("root");

    // -----------------------------------------------------------------------
    //  Page: the actual content, with padding and a Vertical layout.
    // -----------------------------------------------------------------------
    auto page = std::make_shared<Layout>(LayoutType::Vertical);
    page->cls("page");

    auto panel = std::make_shared<Layout>(LayoutType::Vertical);
    panel->cls("panel");

    auto title = Label("Portals demo");
    title->cls("title");
    panel->addChild(title);

    // =======================================================================
    //  1. Tooltip
    // =======================================================================
    {
        auto btn = ActionButton("Hover me", [] {});
        Tooltip::attach(btn, "This tooltip is a portal — it's not clipped by anything.", 0.3f);
        panel->addChild(Row("1. Tooltip", btn));
    }

    // =======================================================================
    //  2. Popup
    // =======================================================================
    {
        auto trigger = ActionButton("Open popup", nullptr);

        auto popup = Popup::create();

        auto content = std::make_shared<Layout>(LayoutType::Vertical);
        content->getInlineBase().padding = Spacing(Px(4));
        content->getInlineBase().gap = Px(4);
        content->addChild(Label("This is a Popup"));
        content->addChild(Label("It positions itself below the trigger."));
        content->addChild(Label("Click outside or press Esc to close."));
        popup->setContent(content);

        trigger->addChild(popup);

        trigger->onClick = [trigger, popup]
        {
            popup->openBelow(trigger);
        };

        panel->addChild(Row("2. Popup", trigger));
    }

    // =======================================================================
    //  3. ContextMenu
    // =======================================================================
    {
        auto target = std::make_shared<Layout>(LayoutType::Stack);
        target->cls("target-box");
        target->setInteractive(true);
        target->setBlocksRaycast(true);
        target->addChild(Label("Right-click anywhere in this box"));

        auto menu = ContextMenu({
            {"Action A", []
             { std::printf("menu: action A\n"); }},
            {"Action B", []
             { std::printf("menu: action B\n"); }},
            {"Action C", []
             { std::printf("menu: action C\n"); }},
        });

        target->addChild(menu);
        target->onRightClick = [menu]
        {
            Vec2 pos = UIContext::get().pointer.pos;
            menu->openAt(pos);
        };

        panel->addChild(Row("3. ContextMenu", target));
    }

    // =======================================================================
    //  4. Modal trigger
    // =======================================================================
    std::shared_ptr<Modal> modal;

    {
        auto trigger = ActionButton("Open modal", nullptr);

        modal = Modal::create();

        auto content = modal->children[0];
        content->cls("modal-content");

        auto mTitle = Label("Confirm");
        mTitle->cls("modal-title");
        content->addChild(mTitle);
        content->addChild(Label("This modal is a regular child of the root,"));
        content->addChild(Label("covered by a full-screen overlay."));

        auto actions = std::make_shared<Layout>(LayoutType::Horizontal);
        actions->cls("modal-actions");
        actions->addChild(ActionButton("Cancel", [modal]
                                       { modal->hide(); }));
        actions->addChild(ActionButton("OK", [modal]
                                       {
            std::printf("modal: OK pressed\n");
            modal->hide(); }));
        content->addChild(actions);

        trigger->onClick = [modal]
        {
            std::printf("modal trigger clicked\n");
            modal->show();
        };
        panel->addChild(Row("4. Modal", trigger));
    }

    page->addChild(panel);

    // -----------------------------------------------------------------------
    //  Assemble the tree: page first (bottom), modal second (top).
    // -----------------------------------------------------------------------
    root->addChild(page);
    root->addChild(modal);

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