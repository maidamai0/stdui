// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include "stdui/render/backend_renderer.hpp"

#ifdef _WIN32

#include <d2d1.h>
#include <dwrite.h>
#include <memory>

namespace stdui::rendering {

/// Direct2D-based renderer for Windows
class direct2d_renderer : public renderer {
public:
    /// Create Direct2D renderer
    /// @param viewport_size Initial viewport dimensions
    /// @param hwnd Window handle for rendering target
    direct2d_renderer(size viewport_size, void* hwnd);
    ~direct2d_renderer() override;

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

#endif  // _WIN32
