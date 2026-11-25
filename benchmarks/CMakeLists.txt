set(ZENITUI_BENCH_HEADERS
    bench_framework.hpp
)

add_executable(ZenitBenchmarks
    main.cpp
    benches.cpp
    ${ZENITUI_BENCH_HEADERS}
)

target_link_libraries(ZenitBenchmarks
    PRIVATE
        ZenitUI::ZenitUI
        ZenitUI::MockBackend
)
target_compile_features(ZenitBenchmarks PUBLIC cxx_std_20)

target_include_directories(ZenitBenchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/tests
)

if(EXISTS "${PROJECT_SOURCE_DIR}/src")
    target_include_directories(ZenitBenchmarks PRIVATE
        ${PROJECT_SOURCE_DIR}/include)
endif()
if(EXISTS "${PROJECT_SOURCE_DIR}/src/ui")
    target_include_directories(ZenitBenchmarks PRIVATE
        ${PROJECT_SOURCE_DIR}/include/components)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ZenitBenchmarks PRIVATE
        -O2 -Wall -Wextra -Wno-missing-field-initializers)
elseif(MSVC)
    target_compile_options(ZenitBenchmarks PRIVATE /O2 /W4)
endif()

set_target_properties(ZenitBenchmarks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    OUTPUT_NAME              "bench"
)

if(NOT ZENITUI_BENCH_BACKEND STREQUAL "")
    if(ZENITUI_BENCH_BACKEND STREQUAL "raylib")
        set(_bench_backend_target ZenitUI::RaylibBackend)
        set(_bench_backend_define ZENITUI_BENCH_BACKEND_RAYLIB)
    elseif(ZENITUI_BENCH_BACKEND STREQUAL "sdl3")
        set(_bench_backend_target ZenitUI::SDL3Backend)
        set(_bench_backend_define ZENITUI_BENCH_BACKEND_SDL3)
    else()
        message(FATAL_ERROR
            "Unsupported ZENITUI_BENCH_BACKEND='${ZENITUI_BENCH_BACKEND}'. "
            "Supported: raylib, sdl3.")
    endif()

    add_executable(ZenitBenchmarksBackend
        backend_bench.cpp
        ${ZENITUI_BENCH_HEADERS}
    )

    target_link_libraries(ZenitBenchmarksBackend
        PRIVATE
            ZenitUI::ZenitUI
            ${_bench_backend_target}
    )
    target_compile_definitions(ZenitBenchmarksBackend
        PRIVATE
            ${_bench_backend_define}
    )
    target_compile_features(ZenitBenchmarksBackend PUBLIC cxx_std_20)
    target_include_directories(ZenitBenchmarksBackend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(ZenitBenchmarksBackend PRIVATE
            -O2 -Wall -Wextra -Wno-missing-field-initializers)
    elseif(MSVC)
        target_compile_options(ZenitBenchmarksBackend PRIVATE /O2 /W4)
    endif()

    set_target_properties(ZenitBenchmarksBackend PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
        OUTPUT_NAME              "bench_${ZENITUI_BENCH_BACKEND}"
    )
endif()