#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include <raylib.h>
#include "RaylibBackend.hpp"
#include "UI.hpp"
#include <cstdio>

using namespace ZenitUI;

int main(void)
{
    InitWindow(800, 600, "ZenitUI - UIPrefabs and Slots");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    RaylibRenderer backendRenderer;
    RaylibPlatform backendPlatform;
    RaylibAssetProvider backendAssets(backendRenderer);

    UIContext::get().renderer = &backendRenderer;
    UIContext::get().platform = &backendPlatform;
    UIContext::get().assets = &backendAssets;

    backendAssets.loadFont("calibri", "./assets/calibrib.ttf", 64);

    // Stili CSS essenziali per i widget! (Senza questi, il Toggle è 0x0 invisibile)
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
    )");

    ZMarkup::registerPrefab("SettingRow", R"(
        HStack width="100%" justify="space-between" items-v="center" padding="10px" background="#2A2A32" radius="6px" {
            Slot
        }
    )");

    const char* layout = R"(
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

    ui.onToggle("toggle-fs", [](bool v) { std::printf("Fullscreen set to: %d\n", v); });
    ui.onValueChanged("slider-vol", [](float v) { std::printf("Volume set to: %.2f\n", v); });

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        root->runFrame(dt);

        BeginDrawing();
        ClearBackground(::BLACK);
        root->renderFrame();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}