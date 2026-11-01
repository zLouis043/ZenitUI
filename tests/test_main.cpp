#include "TestFramework.hpp"
#include "CoreTypes.hpp"

int main() {
    ZenitUI::Metrics::viewport = { 1920.0f, 1080.0f };
    return Test::run();
}