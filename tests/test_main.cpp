#include "TestFramework.hpp"
#include "CoreTypes.hpp"

#include <cstring>
#include <iostream>

int main(int argc, char** argv)  {
    ZenitUI::Metrics::viewport = { 1920.0f, 1080.0f };

     for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--list") == 0) {
            Test::listMode = true;
        } else if (std::strncmp(argv[i], "--filter=", 9) == 0) {
            Test::filterPattern = argv[i] + 9;
        } else {
            std::cerr << "Unknown arg: " << argv[i] << "\n";
            return 2;
        }
    }

    return Test::run();
}