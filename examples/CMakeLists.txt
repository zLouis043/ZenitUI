set(ZENITUI_EXAMPLES_BACKEND "raylib" CACHE STRING
    "Backend used by the examples (must be listed in ZENITUI_BACKENDS).")

if(NOT ZENITUI_EXAMPLES_BACKEND IN_LIST ZENITUI_BACKENDS)
    message(FATAL_ERROR
        "Examples need backend '${ZENITUI_EXAMPLES_BACKEND}', "
        "but ZENITUI_BACKENDS = '${ZENITUI_BACKENDS}'.")
endif()

if(ZENITUI_EXAMPLES_BACKEND STREQUAL "raylib")
    set(_examples_backend_target ZenitUI::RaylibBackend)
elseif(ZENITUI_EXAMPLES_BACKEND STREQUAL "mock")
    set(_examples_backend_target ZenitUI::MockBackend)
else()
    message(FATAL_ERROR
        "No target mapping for backend '${ZENITUI_EXAMPLES_BACKEND}'. "
        "Edit examples/CMakeLists.txt to add one.")
endif()

file(GLOB EXAMPLE_MAINS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/main.cpp")

foreach(MAIN_SRC IN LISTS EXAMPLE_MAINS)
    get_filename_component(EXAMPLE_DIR  "${MAIN_SRC}" DIRECTORY)
    get_filename_component(EXAMPLE_NAME "${EXAMPLE_DIR}" NAME)
    file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "${EXAMPLE_DIR}/*.cpp")

    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCES})
    target_link_libraries(${EXAMPLE_NAME}
        PRIVATE ZenitUI::ZenitUI ${_examples_backend_target})

    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/${EXAMPLE_NAME}"
        OUTPUT_NAME "main"
    )
endforeach()