cmake_minimum_required(VERSION 3.20)
project(ZenitUI VERSION 0.1.0 LANGUAGES CXX)

# --- Opzioni -------------------------------------------------------------
option(ZENITUI_BUILD_TESTS    "Build ZenitUI unit tests"   ON)  # come 'make all'
option(ZENITUI_BUILD_BACKEND  "Build the Raylib backend"   OFF)
option(ZENITUI_BUILD_EXAMPLES "Build ZenitUI examples"     OFF)
option(ZENITUI_BUILD_BENCHMARKS "Build ZenitUI benchmarks" OFF)
option(ZENITUI_INSTALL        "Generate install rules"     ON)

# Default build type = Release, come 'BUILD ?= release' nel Makefile
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# --- Sorgenti core (NON ricorsivi, come il Makefile) ---------------------
# $(wildcard src/layout/*.cpp) $(wildcard src/internal/*.cpp)
file(GLOB ZENITUI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/layout/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/internal/*.cpp"
)

# --- Libreria statica ----------------------------------------------------
add_library(ZenitUI STATIC ${ZENITUI_SOURCES})
add_library(ZenitUI::ZenitUI ALIAS ZenitUI)

target_compile_features(ZenitUI PUBLIC cxx_std_20)

# Stesse warnings del Makefile
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ZenitUI PRIVATE
        -Wall -Wextra -Wno-missing-field-initializers)
elseif(MSVC)
    target_compile_options(ZenitUI PRIVATE /W4)
endif()

# Stessi include paths del Makefile: -I./include -I./include/components
target_include_directories(ZenitUI
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/components>
        $<INSTALL_INTERFACE:include>
)

# --- Test (equivalenti a bin/tests) --------------------------------------
if(ZENITUI_BUILD_TESTS)
    file(GLOB ZENITUI_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    )

    add_executable(ZenitTests ${ZENITUI_TEST_SOURCES})
    target_link_libraries(ZenitTests PRIVATE ZenitUI::ZenitUI)

    # Il Makefile definisce -DZENITUI_DEBUG per i test
    target_compile_definitions(ZenitTests PRIVATE ZENITUI_DEBUG)

    # Il Makefile aggiunge -I./tests
    target_include_directories(ZenitTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

    enable_testing()
    add_test(NAME ZenitTests COMMAND ZenitTests)
endif()

# --- Backend Raylib (opzionale, richiede Raylib) -------------------------
if(ZENITUI_BUILD_BACKEND OR ZENITUI_BUILD_EXAMPLES)
    set(ZENITUI_BUILD_BACKEND ON)
    add_subdirectory(backends/raylib)
endif()

# --- Esempi --------------------------------------------------------------
if(ZENITUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# --- Benchmarks ----------------------------------------------------------
if(ZENITUI_BUILD_BENCHMARKS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
endif()

# --- Install / export ----------------------------------------------------
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