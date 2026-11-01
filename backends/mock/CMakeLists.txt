# backends/mock/CMakeLists.txt
#
# Header-only mock backend for headless testing. Implements IRenderer,
# IPlatform, and IAssetProvider without touching a GPU or a window.
# Intended for CI and for consumers who want to unit-test ZenitUI code
# without pulling Raylib (or any other real backend).

add_library(ZenitMockBackend INTERFACE)
add_library(ZenitUI::MockBackend ALIAS ZenitMockBackend)

target_compile_features(ZenitMockBackend INTERFACE cxx_std_20)
target_link_libraries(ZenitMockBackend INTERFACE ZenitUI::ZenitUI)

target_include_directories(ZenitMockBackend
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# If you ever want the mock to be consumable via find_package, add:
#   install(TARGETS ZenitMockBackend EXPORT ZenitUITargets ...)
# But we deliberately don't, matching the "backends are source-only"
# policy documented in README.