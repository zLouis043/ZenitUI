// ZenitUI — Example 06: ZMarkup
//
// The declarative alternative to building the tree in C++. A single
// ZMarkup string describes the UI; a UINode handle is used to bind
// callbacks and to look up nodes by id.
//
// Concepts covered:
//   - ZMarkup::build(string) returning a UINode
//   - tags, classes (.cls), ids (#id), attributes (key=value)
//   - children blocks { ... }
//   - binding callbacks via ui.onClick / onToggle / onValueChanged
//   - typed lookup via ui.find<T>(id)
//
// The styling lives in ui.zstyle, loaded separately.
//
// Build:   make examples
// Run:     make run-06_zmarkup

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ---------------------------------------------------------------------------
//  The UI, described declaratively.
// ---------------------------------------------------------------------------

static const char* kMarkup = R"(
    VStack#page {
        VStack.card {
            Text.card-title "Account settings"
            Text.card-desc wrap "This entire tree is described by a single ZMarkup string. The styling lives in ui.zstyle."

            HStack.row {
                Text.row-label "Username"
                TextInput#username value="player_one" width="60%"
            }

            HStack.row {
                Text.row-label "Volume"
                Slider#volume value=0.65 width="40%"
                Text.value-label#volume-value "65%"
            }

            HStack.row {
                Text.row-label "Audio"
                Toggle#audio checked=true
            }

            HStack.row {
                Text.row-label ""
                Button.btn-primary#save "Save"
                Button.btn-primary#reset "Reset"
            }
        }
    }
)";

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main()
{
    InitWindow(800, 540, "ZenitUI — 06 ZMarkup");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    ZMarkup::loadStyleFile("examples/06_zmarkup/ui.zstyle");

    // -----------------------------------------------------------------------
    //  Build the tree. ZMarkup::build parses the string, constructs every
    //  node via the registry, and returns a UINode handle.
    // -----------------------------------------------------------------------
    auto ui = ZMarkup::build(kMarkup);
    auto root = ui.root();

    if (!root) {
        std::fprintf(stderr, "Failed to build markup\n");
        return 1;
    }

    // -----------------------------------------------------------------------
    //  Bind behavior. Callbacks are wired by id, after the tree exists.
    // -----------------------------------------------------------------------

    // Live feedback between the slider and its label.
    auto volumeLabel = ui.find<Text>("volume-value");
    ui.onValueChanged("volume", [volumeLabel](float v) {
        int pct = (int)(v * 100.0f + 0.5f);
        volumeLabel->setText(std::to_string(pct) + "%");
    });

    // Toggle.
    ui.onToggle("audio", [](bool on) {
        std::printf("audio: %s\n", on ? "on" : "off");
    });

    // Buttons.
    ui.onClick("save", [] { std::printf("save clicked\n"); });
    ui.onClick("reset", [volumeLabel] {
        std::printf("reset clicked\n");
        volumeLabel->setText("65%");
    });

    // Typed lookup: the TextInput's onSubmit fires on Enter.
    if (auto input = ui.find<TextInput>("username"))
    {
        input->onSubmit = [](const std::string& s) {
            std::printf("username submitted: %s\n", s.c_str());
        };
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