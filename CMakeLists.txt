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

include(cmake/docs.cmake)
include(cmake/install.cmake)
