# =============================================================================
#  ZenitUI benchmarks
#
#  Mirrors benchmarks/Makefile:
#     SRCS := main.cpp benches.cpp
#     HDR  := bench_framework.hpp
#     OUT  := bin/bench[.exe]
#     INC  := -I$(ROOT)/src -I$(ROOT)/src/ui -I$(ROOT)/tests -I.
#     LINK := libzenit.a
# =============================================================================

# Headers aggiunti al target solo per visibilità nell'IDE (non serve al build)
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

# --- Include paths -------------------------------------------------------
# -I. e -I$(ROOT)/tests
target_include_directories(ZenitBenchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # -I.
    ${PROJECT_SOURCE_DIR}/tests          # -I$(ROOT)/tests
)

# -I$(ROOT)/src e -I$(ROOT)/src/ui sono percorsi EFFETTIVAMENTE PRESENTI
# nel benchmarks/Makefile attuale, ma la struttura del repo al commit
# aaa1e19 usa src/layout/ e src/internal/. Se i benchmark includono
# ancora header con prefisso "src/ui/..." queste due dir servono; se
# sono già stati migrati puoi rimuoverle. Le teniamo solo se esistono,
# così il build non si rompe su un path inesistente.
if(EXISTS "${PROJECT_SOURCE_DIR}/src")
    target_include_directories(ZenitBenchmarks PRIVATE
        ${PROJECT_SOURCE_DIR}/include)       # -I$(ROOT)/src
endif()
if(EXISTS "${PROJECT_SOURCE_DIR}/src/ui")
    target_include_directories(ZenitBenchmarks PRIVATE
        ${PROJECT_SOURCE_DIR}/include/components)    # -I$(ROOT)/src/ui
endif()

# --- Flags ---------------------------------------------------------------
# Il Makefile forza -O2 indipendentemente dal build type.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ZenitBenchmarks PRIVATE
        -O2 -Wall -Wextra -Wno-missing-field-initializers)
elseif(MSVC)
    target_compile_options(ZenitBenchmarks PRIVATE /O2 /W4)
endif()

# --- Output: build/benchmarks/bin/bench[.exe] ----------------------------
set_target_properties(ZenitBenchmarks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    OUTPUT_NAME              "bench"
)