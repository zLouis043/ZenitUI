include(FetchContent)

if(NOT TARGET SDL3::SDL3-static)
    set(SDL_SHARED           OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC           ON  CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY     OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS            OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES         OFF CACHE BOOL "" FORCE)
    set(SDL_DISABLE_INSTALL  ON  CACHE BOOL "" FORCE)

    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.2.8
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(SDL3)
endif()

if(NOT DEFINED stb_SOURCE_DIR)
    FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(stb)
endif()

find_package(OpenGL REQUIRED)

add_library(ZenitSDL3Backend STATIC SDL3Backend.cpp)
add_library(ZenitUI::SDL3Backend ALIAS ZenitSDL3Backend)

target_compile_features(ZenitSDL3Backend PUBLIC cxx_std_20)
target_link_libraries(ZenitSDL3Backend
    PUBLIC
        ZenitUI::ZenitUI
        SDL3::SDL3-static
    PRIVATE
        OpenGL::GL
)
target_include_directories(ZenitSDL3Backend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_include_directories(ZenitSDL3Backend SYSTEM
    PRIVATE
        "${stb_SOURCE_DIR}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ZenitSDL3Backend PRIVATE
        -Wall -Wextra -Wno-missing-field-initializers)
elseif(MSVC)
    target_compile_options(ZenitSDL3Backend PRIVATE /W4)
endif()