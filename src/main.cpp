#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

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

    RaylibRenderer backendRenderer;
    RaylibPlatform backendPlatform;
    RaylibAssetProvider backendAssets(backendRenderer);

    UIContext::get().renderer = &backendRenderer;
    UIContext::get().platform = &backendPlatform;
    UIContext::get().assets = &backendAssets;

    backendAssets.loadFont("calibri", "assets/calibrib.ttf", 64);
    backendAssets.loadFont("mont", "assets/mont.otf", 64);
    backendAssets.loadFont("congose", "assets/congose.ttf", 64);
    backendAssets.loadFont("designer", "assets/Designer.otf", 64);
    backendAssets.loadFont("highrise", "assets/highrise.otf", 64);
    backendAssets.loadTexture("bubble", "assets/bubble.png");
    backendAssets.loadTexture("npatches", "assets/npatches_y.png");
    backendAssets.loadEffect("hueShift", "assets/hue_shift.fs");
    backendAssets.loadEffect("blur", "assets/blur.fs");

    InitializeGameTheme();

    auto rootLayer = std::make_shared<Layout>(LayoutType::Stack);
    rootLayer->getInlineBase().itemsH = Align::Center;
    rootLayer->getInlineBase().itemsV = Align::Center;
    rootLayer->getInlineBase().width = Percent(100.0f);
    rootLayer->getInlineBase().height = Percent(100.0f);

    auto launchRow = HStack({})->cls("launch-row");

    auto openDemoBtn = UI::Btn("APRI DEMO", [rootLayer]()
                               { rootLayer->addChild(DemoWindow::create()); });
    openDemoBtn->cls("btn-primary");

    auto openSettingsBtn = UI::Btn("APRI IMPOSTAZIONI", [rootLayer]()
                                   { rootLayer->addChild(SettingsScreen::create()); });
    openSettingsBtn->cls("btn-secondary");

    launchRow->addChild(openDemoBtn);
    launchRow->addChild(openSettingsBtn);
    rootLayer->addChild(launchRow);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        UIContext::get().beginFrame(dt);

        rootLayer->updateTree(dt);
        rootLayer->measure((float)GetScreenWidth(), (float)GetScreenHeight());
        rootLayer->arrange({0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()});

        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        rootLayer->draw();
        EndDrawing();

        UIContext::get().renderer->endFrame();
    }

    CloseWindow();
}