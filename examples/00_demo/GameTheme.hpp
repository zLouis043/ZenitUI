#pragma once

#include "ZenitUI.hpp"
#include <cstdio>

using namespace ZenitUI;

// Loads the demo stylesheets into the global ZenitUI::Theme singleton.
// Rules from multiple files are merged in load order.
inline void InitializeGameTheme()
{
    if (!ZMarkup::loadStyleFile("./assets/game.zstyle"))
    {
        std::fprintf(stderr, "[GameTheme] Failed to load assets/game.zstyle\n");
    }
    if (!ZMarkup::loadStyleFile("./assets/settings.zstyle"))
    {
        std::fprintf(stderr, "[Settings] Failed to load assets/settings.zstyle\n");
    }
}