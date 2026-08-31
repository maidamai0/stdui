cmake_minimum_required(VERSION 3.24)

project(stdui VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

option(STDUI_ENABLE_COVERAGE "Enable llvm-cov coverage instrumentation and reporting" OFF)
set(STDUI_COVERAGE_THRESHOLD 90 CACHE STRING "Minimum required line coverage percentage")

add_library(stdui INTERFACE)
add_library(stdui::stdui ALIAS stdui)

target_include_directories(stdui INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(stdui INTERFACE cxx_std_20)

include(CTest)
if(BUILD_TESTING)
    include(cmake/tests.cmake)
endif()

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

include(cmake/install.cmake)
