#pragma once

/**
 * \file Common.hpp
 * \brief Centralized standard library includes used throughout ZenitUI.
 *
 * Including this header gives a translation unit access to the subset of the
 * C++ standard library that ZenitUI relies on: containers, algorithms,
 * mathematical utilities, smart pointers, variant support, and string
 * utilities. It exists so individual source files stay terse and so that a
 * future move to alternative standard facilities (e.g. `std::pmr` allocators,
 * a custom `flat_map`) only has to touch this one file.
 *
 * \note This header intentionally does not include any ZenitUI-specific
 *       declarations. Include it first, then the framework headers.
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>
#include <array>
#include <random>
#include <sstream>
#include <algorithm>