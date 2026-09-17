cmake_minimum_required(VERSION 3.24)

project(stdui VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

if(APPLE)
    enable_language(OBJCXX)
endif()

option(STDUI_ENABLE_COVERAGE "Enable llvm-cov coverage instrumentation and reporting" OFF)
set(STDUI_COVERAGE_THRESHOLD 90 CACHE STRING "Minimum required line coverage percentage")

add_library(stdui_core INTERFACE)
add_library(stdui::core ALIAS stdui_core)
set_target_properties(stdui_core PROPERTIES EXPORT_NAME core)

target_include_directories(stdui_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(stdui_core INTERFACE cxx_std_20)

add_library(stdui_platform INTERFACE)
add_library(stdui::platform ALIAS stdui_platform)
set_target_properties(stdui_platform PROPERTIES EXPORT_NAME platform)
target_link_libraries(stdui_platform INTERFACE stdui_core)

add_library(stdui_runtime INTERFACE)
add_library(stdui::runtime ALIAS stdui_runtime)
set_target_properties(stdui_runtime PROPERTIES EXPORT_NAME runtime)
target_link_libraries(stdui_runtime INTERFACE stdui_core stdui_platform)

# Platform-specific rendering (dispatched to separate file)
include(cmake/rendering.cmake)

add_library(stdui INTERFACE)
add_library(stdui::stdui ALIAS stdui)
set_target_properties(stdui PROPERTIES EXPORT_NAME stdui)
target_link_libraries(stdui INTERFACE
    stdui_core
    stdui_runtime
    stdui_platform
    stdui_render)

include(CTest)
if(BUILD_TESTING)
    include(cmake/tests.cmake)
endif()

include(cmake/docs.cmake)
include(cmake/install.cmake)
