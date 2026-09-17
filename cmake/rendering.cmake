# Rendering implementation library
add_library(stdui_rendering STATIC
    backend/rendering/render_tree_builder.cpp
    backend/rendering/renderer_factory.cpp
    backend/rendering/frustum_culler.cpp
)

target_include_directories(stdui_rendering PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(stdui_rendering PUBLIC cxx_std_20)

# Platform-specific rendering backends
if(APPLE)
    target_sources(stdui_rendering PRIVATE
        backend/platform/macos/metal_renderer.mm
        backend/platform/macos/glyph_atlas.mm
        backend/platform/macos/path_tessellator.mm
        backend/platform/macos/image_cache.mm
    )
    target_link_libraries(stdui_rendering PUBLIC
        "-framework Metal"
        "-framework QuartzCore"
        "-framework CoreText"
    )
elseif(WIN32)
    target_sources(stdui_rendering PRIVATE
        backend/platform/windows/direct2d_renderer.cpp
    )
    target_link_libraries(stdui_rendering PUBLIC
        d2d1
        dwrite
    )
elseif(UNIX)
    # Linux: Skia/Vulkan backend (future)
    message(WARNING "Linux rendering backend not yet implemented")
endif()

# Link rendering library to main interface
target_link_libraries(stdui INTERFACE stdui_rendering)
