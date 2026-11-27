// @doc   Reusable ZMarkup prefabs with ZMarkup::registerPrefab and Slot child injection.
// @group grp_zmarkup

// ZenitUI — Example 13: UIPrefabs and Slots
//
// Demonstrates registering a reusable ZMarkup prefab (`SettingRow`) with a
// `Slot` placeholder, then instantiating it multiple times inside a parent
// markup layout while injecting caller-provided children into the slot.
//
// Build:   make examples
// Run:     make run-13_prefabs

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include <raylib.h>
#include "RaylibBackend.hpp"
#include "ZenitUI.hpp"
#include <cstdio>

using namespace ZenitUI;

int main(void)
{
    InitWindow(800, 600, "ZenitUI - UIPrefabs and Slots");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    // Bind the Raylib backend to a local UIContext.
    RaylibRenderer backendRenderer;
    RaylibPlatform backendPlatform;
    RaylibAssetProvider backendAssets(backendRenderer);

    UIContext ctx;
    ctx.attachBackend(&backendRenderer, &backendPlatform, &backendAssets);

    ctx.textEngine.loadFont("default", "assets/fonts/Inter_18pt-Regular.ttf");

    // Inline styling for Toggle (its intrinsic size is 0x0 until styled).
    ZMarkup::loadStyleString(R"(
        Toggle {
            width: 50px;
            height: 26px;
            background: #505050;
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
    )", ctx.theme);

    // Register a reusable prefab: children passed to `SettingRow { ... }`
    // are injected where `Slot` appears.
    ZMarkup::registerPrefab("SettingRow", R"(
        HStack width="100%" justify="space-between" items-v="center" padding="10px" background="#2A2A32" radius="6px" {
            Slot
        }
    )");

    // Use the registered prefab just like a built-in tag.
    const char *layout = R"(
        VStack width="100%" height="100%" background="#181818" padding="40px" gap="10px" {
            Text color="#FDF900" font-size="32px" "Settings Menu"

            SettingRow {
                Text.label-text color="#A0A0A0" "Fullscreen"
                Toggle#toggle-fs checked=true
            }

            SettingRow {
                Text.label-text color="#A0A0A0" "Master Volume"
                Slider#slider-vol value=0.7 width="200px"
            }

            SettingRow {
                Text.label-text color="#A0A0A0" "Graphics Quality"
                Dropdown#drop-qual options="Low,Medium,High,Ultra" width="150px"
            }
        }
    )";

    auto ui = ZMarkup::build(layout);
    auto root = ui.root();

    // Wire callbacks for controls injected inside the prefab slots.
    ui.onToggle("toggle-fs", [](bool v) { std::printf("Fullscreen set to: %d\n", v); });
    ui.onValueChanged("slider-vol", [](float v) { std::printf("Volume set to: %.2f\n", v); });

    ctx.mount(root);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        ctx.runFrame(*root, dt);

        BeginDrawing();
        ClearBackground(::BLACK);
        ctx.renderFrame();
        EndDrawing();
    }

    ctx.detachBackend();
    CloseWindow();
    return 0;
}