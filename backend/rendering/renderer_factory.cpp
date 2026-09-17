// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#include "stdui/rendering/renderer.hpp"

#ifdef __APPLE__
// Forward declare Metal renderer to avoid including Objective-C++ headers
namespace stdui::rendering {
    class metal_renderer;
}
#endif

namespace stdui::rendering {

#ifdef __APPLE__
// Declare factory function implemented in metal_renderer.mm
extern std::unique_ptr<renderer> create_metal_renderer(size viewport_size, void* metal_layer);
#endif

std::unique_ptr<renderer> renderer_factory::create(
    size viewport_size,
    void* native_handle
) {
#ifdef __APPLE__
    // On Apple platforms, use Metal renderer
    return create_metal_renderer(viewport_size, native_handle);
#else
    // TODO: Implement Direct2D renderer for Windows
    // TODO: Implement Skia/Vulkan renderer for Linux
    throw std::runtime_error("No renderer implementation available for this platform");
#endif
}

}  // namespace stdui::rendering
