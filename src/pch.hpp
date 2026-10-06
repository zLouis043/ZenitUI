#pragma once

// =========================================================================
//  Precompiled header.
//  Contiene STL + header del framework che vengono parsati di continuo.
//  NON include raylib (RaylibBackend.cpp gestisce i suoi #define).
// =========================================================================

// --- STL ---
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

// --- Framework core (macro-heavy: Style.hpp espande BUBBLE_STYLE_PROPS) ---
#include "Common.hpp"
#include "CoreTypes.hpp"
#include "Easing.hpp"
#include "Unit.hpp"
#include "Style.hpp"
#include "Theme.hpp"
#include "AnimPrimitives.hpp"
#include "StyleResolver.hpp"
#include "AnimationPlayer.hpp"
#include "UIContext.hpp"
#include "Logger.hpp"
#include "ScrollState.hpp"
#include "UIEnums.hpp"
#include "Layout.hpp"