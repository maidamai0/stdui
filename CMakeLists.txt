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

# Rendering subsystem (compiled implementation)
set(STDUI_RENDERING_SOURCES
    src/rendering/render_tree_builder.cpp
    src/rendering/renderer_factory.cpp
)

# Add Metal renderer on Apple platforms
if(APPLE)
    list(APPEND STDUI_RENDERING_SOURCES
        src/rendering/metal_renderer.mm
        src/rendering/glyph_atlas.mm
        src/rendering/path_tessellator.mm
    )
endif()

add_library(stdui_rendering STATIC
    ${STDUI_RENDERING_SOURCES}
)

target_include_directories(stdui_rendering PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(stdui_rendering PUBLIC cxx_std_20)

# Link Metal framework on Apple platforms
if(APPLE)
    target_link_libraries(stdui_rendering PUBLIC
        "-framework Metal"
        "-framework QuartzCore"
        "-framework CoreText"
    )
endif()

# Link rendering library to main interface
target_link_libraries(stdui INTERFACE stdui_rendering)

include(CTest)
if(BUILD_TESTING)
    include(cmake/tests.cmake)
endif()

include(cmake/docs.cmake)
include(cmake/install.cmake)
