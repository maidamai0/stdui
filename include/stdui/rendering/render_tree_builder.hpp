// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <optional>

#include "stdui/layout_tree.hpp"
#include "stdui/rendering/render_node.hpp"

namespace stdui::rendering {

/// Converts layout nodes into render trees
class render_tree_builder {
public:
    render_tree_builder() = default;

    /// Build render tree from layout node
    /// @param root The root layout node to convert
    /// @param viewport The visible viewport for culling
    /// @return The constructed render tree
    std::unique_ptr<render_tree> build(
        const layout_node& root,
        const rect& viewport
    );

private:
    /// Convert a single layout node to render node(s)
    std::shared_ptr<render_node> convert_node(
        const layout_node& node,
        const rect& viewport
    );

    /// Apply culling - check if node is visible in viewport
    bool is_visible(const rect& bounds, const rect& viewport) const;

    /// Compute transform for a node
    std::optional<mat3> compute_transform(const layout_node& node) const;

    /// Compute clip rect for a node
    std::optional<rect> compute_clip_rect(const layout_node& node) const;

    uint64_t frame_number_ = 0;
};

}  // namespace stdui::rendering
