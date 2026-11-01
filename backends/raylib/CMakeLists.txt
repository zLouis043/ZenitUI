if(NOT TARGET raylib)
    include(FetchContent)
    set(BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES     OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_library(ZenitRaylibBackend STATIC RaylibBackend.cpp)
add_library(ZenitUI::RaylibBackend ALIAS ZenitRaylibBackend)

target_compile_features(ZenitRaylibBackend PUBLIC cxx_std_20)
target_link_libraries(ZenitRaylibBackend PUBLIC ZenitUI::ZenitUI raylib)
target_include_directories(ZenitRaylibBackend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ZenitRaylibBackend PRIVATE
        -Wall -Wextra -Wno-missing-field-initializers)
endif()