set(ZENITUI_BENCH_HEADERS
    bench_framework.hpp
)

add_executable(ZenitBenchmarks
    main.cpp
    benches.cpp
    ${ZENITUI_BENCH_HEADERS}
)

target_link_libraries(ZenitBenchmarks PRIVATE ZenitUI::ZenitUI)
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