file(GLOB EXAMPLE_MAINS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/main.cpp"
)

foreach(MAIN_SRC IN LISTS EXAMPLE_MAINS)
    get_filename_component(EXAMPLE_DIR  "${MAIN_SRC}" DIRECTORY)
    get_filename_component(EXAMPLE_NAME "${EXAMPLE_DIR}" NAME)

    file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "${EXAMPLE_DIR}/*.cpp")

    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCES})
    target_link_libraries(${EXAMPLE_NAME}
        PRIVATE ZenitUI::ZenitUI ZenitUI::RaylibBackend
    )

    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/${EXAMPLE_NAME}"
        OUTPUT_NAME "main"
    )
endforeach()