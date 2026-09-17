// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include "stdui/core/geometry.hpp"
#include "stdui/render/node.hpp"

namespace stdui::rendering {

/// Frustum culling utility for rendering optimization
class frustum_culler {
public:
    /// Set the visible viewport rectangle
    void set_viewport(const rect& viewport_rect);

    /// Check if a rectangle is visible (intersects viewport)
    bool is_visible(const rect& bounds) const;

    /// Check if a node should be rendered (visibility + culling)
    bool should_render(const render_node& node) const;

private:
    rect viewport_;
};

}  // namespace stdui::rendering
