set(ZENITUI_EXAMPLES_BACKEND "raylib" CACHE STRING
    "Backend used by tutorial examples (must be listed in ZENITUI_BACKENDS).")

function(zenitui_resolve_backend_target backend_name out_target out_define)
    if(backend_name STREQUAL "raylib")
        set(${out_target} ZenitUI::RaylibBackend PARENT_SCOPE)
        set(${out_define} ZENITUI_DEMO_BACKEND_RAYLIB PARENT_SCOPE)
    elseif(backend_name STREQUAL "sdl3")
        set(${out_target} ZenitUI::SDL3Backend PARENT_SCOPE)
        set(${out_define} ZENITUI_DEMO_BACKEND_SDL3 PARENT_SCOPE)
    elseif(backend_name STREQUAL "mock")
        set(${out_target} ZenitUI::MockBackend PARENT_SCOPE)
        set(${out_define} ZENITUI_DEMO_BACKEND_MOCK PARENT_SCOPE)
    else()
        message(FATAL_ERROR
            "No target mapping for backend '${backend_name}'. "
            "Edit examples/CMakeLists.txt to add one.")
    endif()
endfunction()

if(NOT ZENITUI_EXAMPLES_BACKEND IN_LIST ZENITUI_BACKENDS)
    message(FATAL_ERROR
        "Examples need backend '${ZENITUI_EXAMPLES_BACKEND}', "
        "but ZENITUI_BACKENDS = '${ZENITUI_BACKENDS}'.")
endif()
if(NOT ZENITUI_DEMO_BACKEND IN_LIST ZENITUI_BACKENDS)
    message(FATAL_ERROR
        "00_demo needs backend '${ZENITUI_DEMO_BACKEND}', "
        "but ZENITUI_BACKENDS = '${ZENITUI_BACKENDS}'.")
endif()

zenitui_resolve_backend_target("${ZENITUI_EXAMPLES_BACKEND}" _examples_backend_target _examples_backend_define)
zenitui_resolve_backend_target("${ZENITUI_DEMO_BACKEND}"     _demo_backend_target     _demo_backend_define)

file(GLOB EXAMPLE_MAINS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/main.cpp")

foreach(MAIN_SRC IN LISTS EXAMPLE_MAINS)
    get_filename_component(EXAMPLE_DIR  "${MAIN_SRC}" DIRECTORY)
    get_filename_component(EXAMPLE_NAME "${EXAMPLE_DIR}" NAME)
    file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "${EXAMPLE_DIR}/*.cpp")

    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCES})

    if(EXAMPLE_NAME STREQUAL "00_demo")
        target_link_libraries(${EXAMPLE_NAME}
            PRIVATE ZenitUI::ZenitUI ${_demo_backend_target})
        target_compile_definitions(${EXAMPLE_NAME}
            PRIVATE ${_demo_backend_define})
    else()
        target_link_libraries(${EXAMPLE_NAME}
            PRIVATE ZenitUI::ZenitUI ${_examples_backend_target})
    endif()

    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/${EXAMPLE_NAME}"
        OUTPUT_NAME "main"
    )
endforeach()