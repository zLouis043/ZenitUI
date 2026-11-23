// @doc   End-to-end showcase combining Raylib setup, custom FilterRegistry shaders, ZStyle, ZMarkup, widgets, portals, and animations.
// @group grp_core grp_backends

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "ZenitUI.hpp"

#include "SettingsScreen.hpp"
#include "DemoWindow.hpp"
#include "GameTheme.hpp"

using namespace ZenitUI;

#include <exception>
#include <cstdio>
#include <cstdlib>

int main(void)
{
    InitWindow(800, 600, "TestUI");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    // 1. Bind the Raylib backend implementations to the global UIContext.
    RaylibRenderer backendRenderer;
    RaylibPlatform backendPlatform;
    RaylibAssetProvider backendAssets(backendRenderer);

    UIContext::get().renderer = &backendRenderer;
    UIContext::get().platform = &backendPlatform;
    UIContext::get().assets = &backendAssets;

    // 2. Register fonts, textures, and fragment shaders referenced by .zstyle rules.
    backendAssets.loadFont("calibri", "assets/calibrib.ttf", 64);
    backendAssets.loadFont("mont", "assets/mont.otf", 64);
    backendAssets.loadFont("congose", "assets/congose.ttf", 64);
    backendAssets.loadFont("designer", "assets/Designer.otf", 64);
    backendAssets.loadFont("highrise", "assets/highrise.otf", 64);
    backendAssets.loadTexture("bubble", "assets/bubble.png");
    backendAssets.loadTexture("npatches", "assets/npatches_y.png");
    backendAssets.loadEffect("hueShift", "assets/hue_shift.fs");
    backendAssets.loadEffect("blur", "assets/blur.fs");
    backendAssets.loadEffect("dropShadow", "assets/drop_shadow.fs");
    backendAssets.loadEffect("sepia", "assets/sepia.fs");

    // 3. Register a custom single-pass post-processing filter usable via `filter: sepia(...)`.
    ZenitUI::FilterRegistry::get().add(ZenitUI::FilterDef{
        .name = "sepia",
        .shader = "sepia",
        .pattern = ZenitUI::FilterPattern::SinglePass,
        .params = {
            {"amount", ZenitUI::FilterParamType::Float, 0, 0.7f}}});

    // 4. Load the application's .zstyle stylesheets into the global Theme.
    InitializeGameTheme();

    // 5. Create a full-viewport Stack root and center the launcher buttons.
    auto rootLayer = std::make_shared<Layout>(LayoutType::Stack);
    rootLayer->getInlineBase().itemsH = Align::Center;
    rootLayer->getInlineBase().itemsV = Align::Center;
    rootLayer->getInlineBase().width = Percent(100.0f);
    rootLayer->getInlineBase().height = Percent(100.0f);

    auto launchRow = HStack({})->cls("launch-row");

    // Clicking either button spawns a self-contained TLayout overlay as a child of rootLayer.
    auto openDemoBtn = UI::Btn("APRI DEMO", [rootLayer]()
                               { rootLayer->addChild(DemoWindow::create()); });
    openDemoBtn->cls("btn-primary");

    auto openSettingsBtn = UI::Btn("APRI IMPOSTAZIONI", [rootLayer]()
                                   { rootLayer->addChild(SettingsScreen::create()); });
    openSettingsBtn->cls("btn-secondary");

    launchRow->addChild(openDemoBtn);
    launchRow->addChild(openSettingsBtn);
    rootLayer->addChild(launchRow);

    // 6. Standard ZenitUI frame loop: runFrame() updates/measures/arranges, renderFrame() draws.
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        rootLayer->runFrame(dt);

        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        rootLayer->renderFrame();
        EndDrawing();
    }

    CloseWindow();
}