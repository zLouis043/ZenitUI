// ZenitUI — Example 09: Custom widget
//
// Walks through writing a widget from scratch. The Rating widget lives in
// Rating.hpp next to this source, and demonstrates the full extension API:
// TLayout, onBuild, computeIntrinsicSize, renderContent with ::part,
// onUpdate with pointer and keyboard handling, and an onValueChanged
// callback.
//
// The example also shows how to use a custom widget from C++ and (at the
// bottom) how to register it as a ZMarkup tag.
//
// Build:   make examples
// Run:     make run-09_custom_widget

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"
#include "Rating.hpp"

#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<Layout> Row(const std::string& label,
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

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(800, 480, "ZenitUI — 09 Custom widget");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    ZMarkup::loadStyleFile("examples/09_custom_widget/theme.zstyle");

    // -----------------------------------------------------------------------
    //  Root + page
    // -----------------------------------------------------------------------
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->setStyleTag("Stack");
    root->id("root");

    auto page = std::make_shared<Layout>(LayoutType::Vertical);
    page->cls("page");

    auto panel = std::make_shared<Layout>(LayoutType::Vertical);
    panel->cls("panel");

    auto title = Label("Custom widget: Rating");
    title->cls("title");
    panel->addChild(title);

    // -----------------------------------------------------------------------
    //  One Rating, five stars, initially set to 3.
    // -----------------------------------------------------------------------
    auto valueLabel = Label("3 / 5");
    valueLabel->cls("value-label");

    auto rating = Rating::create(5, 3);
    rating->onValueChanged = [valueLabel](int v) {
        valueLabel->setText(std::to_string(v) + " / 5");
    };

    auto content = std::make_shared<Layout>(LayoutType::Horizontal);
    content->getInlineBase().gap    = Px(16);
    content->getInlineBase().itemsV = Align::Center;
    content->addChild(rating);
    content->addChild(valueLabel);

    panel->addChild(Row("Click, or arrow keys", content));

    // -----------------------------------------------------------------------
    //  A second Rating, ten stars, to show that the widget scales.
    // -----------------------------------------------------------------------
    {
        auto r2 = Rating::create(10, 7);
        r2->getInlineBase().fontSize = Px(20);   // smaller stars
        panel->addChild(Row("Ten stars", r2));
    }

    page->addChild(panel);
    root->addChild(page);

    // -----------------------------------------------------------------------
    //  ZMarkup registration
    //
    //  The built-in ZMarkup::build uses a private registry, so to use a
    //  custom tag you build the tree manually with your own Registry.
    //  The block below is commented out to keep the example focused, but
    //  it shows the pattern.
    // -----------------------------------------------------------------------
    //
    //  ZMarkup::Registry reg;
    //  ZMarkup::registerBuiltins(reg);
    //  reg.reg("Rating", [](const ZMarkup::Element& e) {
    //      return Rating::create(
    //          (int)e.attrFloat("max",   5.0f),
    //          (int)e.attrFloat("value", 0.0f)
    //      );
    //  });
    //
    //  auto el  = ZMarkup::parse(R"(Rating#stars max=5 value=3)");
    //  auto ctx = std::make_shared<ZMarkup::BuildContext>();
    //  auto node = reg.create(el, *ctx);
    //  ZMarkup::UINode ui(node, ctx);
    //
    //  ui.find<Rating>("stars")->onValueChanged = [](int v){ /* ... */ };
    //  panel->addChild(ui.root());

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