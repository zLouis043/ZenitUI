cmake_minimum_required(VERSION 3.20)
project(ZenitUI VERSION 0.1.0 LANGUAGES C CXX)

option(ZENITUI_BUILD_TESTS    "Build ZenitUI unit tests"   ON)
option(ZENITUI_BUILD_BACKEND  "Build the Raylib backend"   OFF)
option(ZENITUI_BUILD_EXAMPLES "Build ZenitUI examples"     OFF)
option(ZENITUI_BUILD_BENCHMARKS "Build ZenitUI benchmarks" OFF)
option(ZENITUI_INSTALL        "Generate install rules"     ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

file(GLOB ZENITUI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/layout/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/internal/*.cpp"
)

add_library(ZenitUI STATIC ${ZENITUI_SOURCES})
add_library(ZenitUI::ZenitUI ALIAS ZenitUI)

include(FetchContent)

# ---- FreeType (minimal rasterizer & metrics, no circular HB / image deps) ----
set(FT_DISABLE_ZLIB     ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON  CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL    ON  CACHE BOOL "" FORCE)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(freetype)

# ---- HarfBuzz (minimal hb-ot OpenType shaper, cross-platform deterministic) ----
set(HB_HAVE_FREETYPE    OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GLIB        OFF CACHE BOOL "" FORCE)
set(HB_HAVE_ICU         OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GOBJECT     OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GRAPHITE2   OFF CACHE BOOL "" FORCE)
set(HB_HAVE_CORETEXT    OFF CACHE BOOL "" FORCE)
set(HB_HAVE_DIRECTWRITE OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GDI         OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET     OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS      OFF CACHE BOOL "" FORCE)
set(HB_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(HB_BUILD_BENCHMARK  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG        10.4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(harfbuzz)

target_link_libraries(ZenitUI PRIVATE freetype harfbuzz)

target_compile_features(ZenitUI PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ZenitUI PRIVATE
        -Wall -Wextra -Wno-missing-field-initializers)
elseif(MSVC)
    target_compile_options(ZenitUI PRIVATE /W4)
endif()

target_include_directories(ZenitUI
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/components>
        $<INSTALL_INTERFACE:include>
)

set(ZENITUI_BACKENDS "" CACHE STRING
    "Semicolon-separated list of backends to build (raylib, mock, ...).")

if(ZENITUI_BUILD_BACKEND AND ZENITUI_BACKENDS STREQUAL "")
    set(ZENITUI_BACKENDS "raylib" CACHE STRING "" FORCE)
endif()

if((ZENITUI_BUILD_TESTS OR ZENITUI_BUILD_BENCHMARKS)
        AND NOT "mock" IN_LIST ZENITUI_BACKENDS)
    list(APPEND ZENITUI_BACKENDS "mock")
    list(REMOVE_DUPLICATES ZENITUI_BACKENDS)
    set(ZENITUI_BACKENDS "${ZENITUI_BACKENDS}" CACHE STRING "" FORCE)
endif()

if(ZENITUI_BUILD_TESTS)
    file(GLOB ZENITUI_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

    add_executable(ZenitTests ${ZENITUI_TEST_SOURCES})
    target_link_libraries(ZenitTests
        PRIVATE ZenitUI::ZenitUI ZenitUI::MockBackend)
    target_compile_definitions(ZenitTests PRIVATE ZENITUI_DEBUG)
    target_include_directories(ZenitTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests")

    enable_testing()

    set(_zenit_tests_file "${CMAKE_CURRENT_BINARY_DIR}/ZenitTests_tests.cmake")

    if(NOT EXISTS "${_zenit_tests_file}")
        file(WRITE "${_zenit_tests_file}" "# placeholder\n")
    endif()

    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
                 "${_zenit_tests_file}")

    add_custom_command(TARGET ZenitTests POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -D "TEST_EXECUTABLE=$<TARGET_FILE:ZenitTests>"
            -D "TEST_OUTPUT_FILE=${_zenit_tests_file}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateZenitTests.cmake"
        VERBATIM
    )
endif()

set(ZENITUI_DEMO_BACKEND "raylib" CACHE STRING
    "Backend used by 00_demo (raylib, sdl3, ...).")

if(ZENITUI_BUILD_EXAMPLES)
    foreach(_needed IN ITEMS raylib "${ZENITUI_DEMO_BACKEND}")
        if(NOT _needed IN_LIST ZENITUI_BACKENDS)
            list(APPEND ZENITUI_BACKENDS "${_needed}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES ZENITUI_BACKENDS)
    set(ZENITUI_BACKENDS "${ZENITUI_BACKENDS}" CACHE STRING "" FORCE)
endif()

set(ZENITUI_BENCH_BACKEND "" CACHE STRING
    "Optional real backend for GPU/rendering benchmarks (raylib, sdl3).")

if(ZENITUI_BUILD_BENCHMARKS AND NOT ZENITUI_BENCH_BACKEND STREQUAL "")
    if(NOT ZENITUI_BENCH_BACKEND IN_LIST ZENITUI_BACKENDS)
        list(APPEND ZENITUI_BACKENDS "${ZENITUI_BENCH_BACKEND}")
        list(REMOVE_DUPLICATES ZENITUI_BACKENDS)
        set(ZENITUI_BACKENDS "${ZENITUI_BACKENDS}" CACHE STRING "" FORCE)
    endif()
endif()

foreach(_backend IN LISTS ZENITUI_BACKENDS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/backends/${_backend}")
        message(FATAL_ERROR
            "Unknown backend '${_backend}'. "
            "Available: raylib, sdl3, mock.")
    endif()
    add_subdirectory("backends/${_backend}")
endforeach()

if(ZENITUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ZENITUI_BUILD_BENCHMARKS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
endif()

if(ZENITUI_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS ZenitUI
        EXPORT ZenitUITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(TARGETS freetype harfbuzz
        EXPORT ZenitUITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ZenitUITargets
        FILE      ZenitUITargets.cmake
        NAMESPACE ZenitUI::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZenitUI
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZenitUIConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ZenitUIConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZenitUI
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ZenitUIConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ZenitUIConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ZenitUIConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZenitUI
    )
endif()