// ZenitUI — Example 03: Widgets
//
// A gallery of the built-in widgets: Button, Toggle, Checkbox, Slider,
// ProgressBar, TextInput, Dropdown. Each row is interactive.
//
// The widgets ship with no default appearance beyond their base layout;
// the small .zstyle block below is what makes them visible and clickable.
//
// Build:   make examples
// Run:     make run-03_widgets

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

// A row: label on the left, widget on the right.
static std::shared_ptr<Layout> Row(const std::string &title,
                                   std::shared_ptr<Layout> widget)
{
    auto row = std::make_shared<Layout>(LayoutType::Horizontal);
    row->getInlineBase().gap = Px(16);
    row->getInlineBase().itemsV = Align::Center;
    row->getInlineBase().padding = Spacing(Px(10), Px(14));
    row->getInlineBase().background = ZenitUI::Color{32, 32, 38, 255};
    row->getInlineBase().radius = Px(6);
    row->getInlineBase().width = Percent(100);

    auto label = Label(title);
    label->size(Px(140), Auto());
    label->getInlineBase().color = ZenitUI::Color{180, 180, 190, 255};

    row->addChild(label);
    row->addChild(widget);
    return row;
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(700, 600, "ZenitUI — 03 Widgets");
    SetTargetFPS(60);

    RaylibRenderer renderer;
    RaylibPlatform platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets = &assets;

    // -----------------------------------------------------------------------
    //  Minimal styling for the built-in widgets.
    //  This is inline (not a file) so the example stays self-contained.
    // -----------------------------------------------------------------------
    ZMarkup::loadStyleString(R"(
        Button {
            background: #4a9eff;
            color: #ffffff;
            radius: 6px;
            padding: 8px 16px;
        }
        Button:hover   { background: #5aafff; }
        Button:pressed { background: #3a8eef; }

        Toggle {
            width: 50px;
            height: 26px;
            background: #404040;
            radius: 13px;
            transition: background 0.15s linear;
        }
        Toggle:checked { background: #4a9eff; }

        Toggle .toggle-knob {
            position: absolute;
            width: 22px;
            height: 22px;
            top: 2px;
            left: 2px;
            radius: 11px;
            background: #ffffff;
            transition: left 0.18s ease-out-cubic;
        }
        Toggle:checked .toggle-knob { left: 26px; }

        Checkbox {
            width: 26px;
            height: 26px;
            background: #303030;
            radius: 4px;
        }
        Checkbox:checked { background: #4a9eff; }

        Slider {
            width: 200px;
            height: 24px;
        }
        Slider::track { background: #303030; radius: 4px; }
        Slider::fill  { color: #4a9eff; }
        Slider::knob  { color: #ffffff; }

        ProgressBar {
            width: 200px;
            height: 20px;
            background: #303030;
            color: #4a9eff;
            radius: 4px;
        }

        TextInput {
            background: #1a1a1e;
            color: #ffffff;
            radius: 4px;
            padding: 8px 12px;
            border-width: 1px;
            border-color: #404040;
        }
        TextInput:focus { border-color: #4a9eff; }

        .dropdown-trigger {
            background: #303030;
            color: #ffffff;
            radius: 4px;
            padding: 8px 14px;
            height: auto;
            font-size: 16px;
        }
    )");

    // -----------------------------------------------------------------------
    //  Root: a ScrollView so the gallery is scrollable if it grows.
    // -----------------------------------------------------------------------
    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Percent(100), Percent(100));
    root->getInlineBase().background = ZenitUI::Color{20, 20, 24, 255};
    root->getInlineBase().padding = Spacing(Px(16));

    auto scroller = ScrollView::create(LayoutType::Vertical);
    scroller->getInlineBase().grow = 1;
    scroller->getInlineBase().gap = Px(8);
    scroller->getInlineBase().width = Percent(100);
    scroller->getInlineBase().overflowX = Overflow::Hidden;
    scroller->getInlineBase().overflowY = Overflow::Auto;
    root->addChild(scroller);

    // --- Button ------------------------------------------------------------
    {
        auto btn = Btn("Click me", []
                       { std::printf("button: clicked\n"); });
        scroller->addChild(Row("Button", btn));
    }

    // --- Toggle ------------------------------------------------------------
    {
        auto tgl = Toggle::create(false);
        tgl->onToggle = [](bool v)
        { std::printf("toggle: %d\n", v); };
        scroller->addChild(Row("Toggle", tgl));
    }

    // --- Checkbox ----------------------------------------------------------
    {
        auto cb = Checkbox::create(false);
        cb->onToggle = [](bool v)
        { std::printf("checkbox: %d\n", v); };
        scroller->addChild(Row("Checkbox", cb));
    }

    // --- Slider + ProgressBar ---------------------------------------------
    //  The slider drives the progress bar via onValueChanged.
    {
        auto slider = Slider::create(0.5f);
        auto progress = ProgressBar::create(0.5f);
        slider->onValueChanged = [progress](float v)
        {
            progress->setValue(v);
        };

        auto stacked = std::make_shared<Layout>(LayoutType::Horizontal);
        stacked->getInlineBase().gap = Px(12);
        stacked->getInlineBase().itemsV = Align::Center;
        stacked->addChild(slider);
        stacked->addChild(progress);

        scroller->addChild(Row("Slider", stacked));
    }

    // --- TextInput ---------------------------------------------------------
    {
        auto input = TextInput::create("Type here...");
        input->getInlineBase().minWidth = Px(220);
        input->onTextChanged = [](const std::string &s)
        {
            std::printf("text: %s\n", s.c_str());
        };
        input->onSubmit = [](const std::string &s)
        {
            std::printf("submit: %s\n", s.c_str());
        };
        scroller->addChild(Row("TextInput", input));
    }

    // --- Dropdown ----------------------------------------------------------
    {
        auto dd = Dropdown::create(
            std::vector<std::string>{"Easy", "Normal", "Hard"}, 1);
        dd->onChange = [](int idx, const std::string &name)
        {
            std::printf("dropdown: %d -> %s\n", idx, name.c_str());
        };

        auto wrapper = std::make_shared<Layout>(LayoutType::Stack);
        wrapper->size(Px(180), Auto());
        wrapper->addChild(dd);

        scroller->addChild(Row("Dropdown", wrapper));
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