#pragma once

#include "ZenitUI.hpp"
#include <cstdio>

using namespace ZenitUI;

inline void InitializeGameTheme() {
    if (!ZMarkup::loadStyleFile("./assets/game.zstyle")) {
        std::fprintf(stderr, "[GameTheme] Failed to load assets/game.zstyle\n");
    }
    if (!ZMarkup::loadStyleFile("./assets/settings.zstyle")) {
        std::fprintf(stderr, "[Settings] Failed to load assets/settings.zstyle\n");
    }
}