# Render-layer implementation library
add_library(stdui_render STATIC
    backend/rendering/render_tree_builder.cpp
    backend/rendering/renderer_factory.cpp
    backend/rendering/frustum_culler.cpp
)
add_library(stdui::render ALIAS stdui_render)
add_library(stdui_rendering ALIAS stdui_render)
set_target_properties(stdui_render PROPERTIES EXPORT_NAME render)

target_link_libraries(stdui_render PUBLIC stdui_core)
target_compile_features(stdui_render PUBLIC cxx_std_20)

# Platform-specific rendering backends
if(APPLE)
    target_sources(stdui_render PRIVATE
        backend/platform/macos/metal_renderer.mm
        backend/platform/macos/glyph_atlas.mm
        backend/platform/macos/path_tessellator.mm
        backend/platform/macos/image_cache.mm
    )
    target_link_libraries(stdui_render PUBLIC
        "-framework Metal"
        "-framework QuartzCore"
        "-framework CoreText"
    )
elseif(WIN32)
    target_sources(stdui_render PRIVATE
        backend/platform/windows/direct2d_renderer.cpp
    )
    target_link_libraries(stdui_render PUBLIC
        d2d1
        dwrite
    )
elseif(UNIX)
    # Linux: Skia/Vulkan backend (future)
    message(WARNING "Linux rendering backend not yet implemented")
endif()
