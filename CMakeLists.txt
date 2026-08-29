cmake_minimum_required(VERSION 3.24)

project(stdui LANGUAGES CXX)

option(STDUI_ENABLE_COVERAGE "Enable llvm-cov coverage instrumentation and reporting" OFF)
set(STDUI_COVERAGE_THRESHOLD 90 CACHE STRING "Minimum required line coverage percentage")

add_library(stdui INTERFACE)
target_include_directories(stdui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(stdui INTERFACE cxx_std_20)

include(CTest)
if(BUILD_TESTING)
    include(cmake/tests.cmake)
endif()
