# Documentation build configuration

find_package(Doxygen)
option(STDUI_BUILD_DOCS "Build Doxygen documentation" ${DOXYGEN_FOUND})

if(STDUI_BUILD_DOCS)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "STDUI_BUILD_DOCS requires Doxygen")
    endif()

    set(STDUI_DOCS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs")
    file(MAKE_DIRECTORY "${STDUI_DOCS_OUTPUT_DIR}")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
        ${STDUI_DOCS_OUTPUT_DIR}/Doxyfile
        @ONLY)

    add_custom_target(stdui_docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${STDUI_DOCS_OUTPUT_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating stdui API documentation")
endif()
