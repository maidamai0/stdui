// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include "stdui/rendering/render_node.hpp"
#include "stdui/geometry.hpp"
#include <vector>

#ifdef __APPLE__
#include <Metal/Metal.h>
#include <simd/simd.h>
#endif

namespace stdui::rendering {

#ifdef __APPLE__

/// Tessellates paths into triangles for GPU rendering
class path_tessellator {
public:
    struct triangle {
        simd_float2 v0, v1, v2;
        simd_float4 color;
    };

    path_tessellator() = default;

    /// Tessellate a path into triangles
    /// @param commands Path commands
    /// @param points Path points
    /// @param fill_color Fill color
    /// @return Vector of triangles
    std::vector<triangle> tessellate(
        const std::vector<uint8_t>& commands,
        const std::vector<point>& points,
        color fill_color
    );

    /// Create stroke triangles for a path
    /// @param commands Path commands
    /// @param points Path points
    /// @param stroke_color Stroke color
    /// @param stroke_width Stroke width
    /// @return Vector of triangles
    std::vector<triangle> create_stroke(
        const std::vector<uint8_t>& commands,
        const std::vector<point>& points,
        color stroke_color,
        float stroke_width
    );

private:
    // Simple ear-clipping triangulation for convex polygons
    void triangulate_polygon(
        const std::vector<point>& vertices,
        color fill_color,
        std::vector<triangle>& out_triangles
    );
};

#endif  // __APPLE__

}  // namespace stdui::rendering
