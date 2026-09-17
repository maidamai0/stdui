// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include "stdui/render/backend_renderer.hpp"

#ifdef __APPLE__

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

namespace stdui::rendering {

/// Metal-based renderer for macOS/iOS
class metal_renderer : public renderer {
public:
    /// Create Metal renderer
    /// @param viewport_size Initial viewport dimensions
    /// @param metal_layer CAMetalLayer for rendering target
    metal_renderer(size viewport_size, CAMetalLayer* metal_layer);
    ~metal_renderer() override;

    void begin_frame() override;
    void render(const render_tree& tree) override;
    void end_frame() override;
    void resize(size new_size) override;
    size viewport_size() const override;

private:
    struct impl;
    std::unique_ptr<impl> impl_;

    // Rendering methods
    void render_node_internal(const render_node& node);
    void render_rectangle(const rectangle_node& node);
    void render_text(const text_node& node);
    void render_path(const path_node& node);
    void render_image(const image_node& node);
    void render_group(const group_node& node);
    void render_effect(const effect_node& node);
    void render_scene_view(const scene_view_node& node);
};

}  // namespace stdui::rendering

#endif  // __APPLE__
