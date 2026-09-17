// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#include "stdui/render/tree_builder.hpp"

namespace stdui::rendering {

std::unique_ptr<render_tree> render_tree_builder::build(
    const layout_node& root,
    const rect& viewport
) {
    auto tree = std::make_unique<render_tree>();
    tree->set_frame_number(frame_number_++);

    auto root_node = convert_node(root, viewport);
    tree->set_root(std::move(root_node));

    return tree;
}

std::shared_ptr<render_node> render_tree_builder::convert_node(
    const layout_node& node,
    const rect& viewport
) {
    // For Phase 4.1, we need to arrange the node to get its bounds
    // Since layout_node doesn't store its frame, we need to compute it
    // For now, create a placeholder that covers the entire viewport
    rect bounds = viewport;

    // Cull nodes outside viewport
    if (!is_visible(bounds, viewport)) {
        return nullptr;
    }

    // Create appropriate render node based on layout node type
    std::shared_ptr<render_node> render;

    // For now, create a simple rectangle for all nodes
    auto rect_node = std::make_shared<rectangle_node>();
    auto& props = rect_node->properties();
    props.bounds = bounds;
    props.fill_color = color{0.9f, 0.9f, 0.9f, 1.0f};  // Light gray default
    props.corner_radius = 0.0f;

    render = rect_node;

    // Set common properties
    if (auto transform = compute_transform(node)) {
        render->set_transform(*transform);
    }

    if (auto clip = compute_clip_rect(node)) {
        render->set_clip_rect(*clip);
    }

    // Handle children for container nodes
    if (!node.children.empty()) {
        auto group = std::make_shared<group_node>();

        // Copy properties from the rectangle node
        if (auto transform = render->transform()) {
            group->set_transform(*transform);
        }
        if (auto clip = render->clip_rect()) {
            group->set_clip_rect(*clip);
        }

        // Add the background rectangle as first child
        group->add_child(render);

        // Convert children
        for (const auto& child : node.children) {
            if (auto child_render = convert_node(child, viewport)) {
                group->add_child(child_render);
            }
        }

        return group;
    }

    return render;
}

bool render_tree_builder::is_visible(
    const rect& bounds,
    const rect& viewport
) const {
    // Simple rectangle intersection test
    return !(bounds.origin.x + bounds.extent.width < viewport.origin.x ||
             bounds.origin.x > viewport.origin.x + viewport.extent.width ||
             bounds.origin.y + bounds.extent.height < viewport.origin.y ||
             bounds.origin.y > viewport.origin.y + viewport.extent.height);
}

std::optional<mat3> render_tree_builder::compute_transform(
    const layout_node& node
) const {
    // TODO: Extract transform from layout node if it has one
    // For now, no transforms
    return std::nullopt;
}

std::optional<rect> render_tree_builder::compute_clip_rect(
    const layout_node& node
) const {
    // TODO: Extract clip rect from layout node if it has one
    // For now, no clipping
    return std::nullopt;
}

}  // namespace stdui::rendering
