// tests/Mocks.hpp
//
// Compatibility shim. The mock backend moved to backends/mock/MockBackend.hpp
// in §7.8. Kept so existing test files that `#include "Mocks.hpp"` don't
// need to change. New code should include <MockBackend.hpp> directly.
#pragma once
#include <MockBackend.hpp>