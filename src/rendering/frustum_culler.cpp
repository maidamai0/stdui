// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#include "stdui/rendering/frustum_culler.hpp"

namespace stdui::rendering {

void frustum_culler::set_viewport(const rect& viewport_rect) {
    viewport_ = viewport_rect;
}

bool frustum_culler::is_visible(const rect& bounds) const {
    // Check if rectangles intersect
    if (bounds.origin.x + bounds.extent.width < viewport_.origin.x) return false;
    if (bounds.origin.x > viewport_.origin.x + viewport_.extent.width) return false;
    if (bounds.origin.y + bounds.extent.height < viewport_.origin.y) return false;
    if (bounds.origin.y > viewport_.origin.y + viewport_.extent.height) return false;
    return true;
}

bool frustum_culler::should_render(const render_node& node) const {
    if (!node.is_visible()) {
        return false;
    }

    // Get bounds based on node type
    rect bounds;
    switch (node.type()) {
        case render_node_type::rectangle: {
            const auto& rect_node = static_cast<const rectangle_node&>(node);
            bounds = rect_node.properties().bounds;
            break;
        }
        case render_node_type::text: {
            const auto& txt_node = static_cast<const text_node&>(node);
            const auto& props = txt_node.properties();
            // Approximate text bounds (conservative)
            bounds = rect{{props.position.x, props.position.y}, {1000, 100}};
            break;
        }
        case render_node_type::image: {
            const auto& img_node = static_cast<const image_node&>(node);
            bounds = img_node.properties().bounds;
            break;
        }
        case render_node_type::scene_view: {
            const auto& scene_node = static_cast<const scene_view_node&>(node);
            bounds = scene_node.viewport();
            break;
        }
        case render_node_type::group:
        case render_node_type::path:
        case render_node_type::effect:
            // For groups, paths, and effects, always render (conservative)
            return true;
    }

    return is_visible(bounds);
}

}  // namespace stdui::rendering
