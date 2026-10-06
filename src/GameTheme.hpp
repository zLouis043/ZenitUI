#pragma once

#include "UI.hpp"
#include <cstdio>

using namespace ZenitUI;

inline void InitializeGameTheme() {
    if (!ZMarkup::loadStyleFile("./assets/game.zstyle")) {
        std::fprintf(stderr, "[GameTheme] Impossibile caricare assets/game.zstyle\n");
    }
    if (!ZMarkup::loadStyleFile("./assets/settings.zstyle")) {
        std::fprintf(stderr, "[Settings] Impossibile caricare assets/settings.zstyle\n");
    }
}