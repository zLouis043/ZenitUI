#include "TestFramework.hpp"
#include "CoreTypes.hpp"

int main() {
    // I test di Unit/Easing usano VW/VH e quindi Metrics::viewport.
    // Mettiamo un valore realistico, non-zero, all'avvio.
    ZenitUI::Metrics::viewport = { 1920.0f, 1080.0f };
    return Test::run();
}