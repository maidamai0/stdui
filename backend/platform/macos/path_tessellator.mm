// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef __APPLE__

#include "stdui/rendering/path_tessellator.hpp"
#include <algorithm>
#include <cmath>

namespace stdui::rendering {

std::vector<path_tessellator::triangle> path_tessellator::tessellate(
    const std::vector<uint8_t>& commands,
    const std::vector<point>& points,
    color fill_color
) {
    std::vector<triangle> triangles;

    if (points.empty()) {
        return triangles;
    }

    // Simple convex polygon triangulation (fan triangulation from first vertex)
    // TODO: Implement proper ear-clipping for concave polygons
    triangulate_polygon(points, fill_color, triangles);

    return triangles;
}

std::vector<path_tessellator::triangle> path_tessellator::create_stroke(
    const std::vector<uint8_t>& commands,
    const std::vector<point>& points,
    color stroke_color,
    float stroke_width
) {
    std::vector<triangle> triangles;

    if (points.size() < 2) {
        return triangles;
    }

    // Simple line strip to quad strip conversion
    float half_width = stroke_width * 0.5f;

    for (size_t i = 0; i < points.size() - 1; ++i) {
        const point& p0 = points[i];
        const point& p1 = points[i + 1];

        // Calculate perpendicular vector
        double dx = p1.x - p0.x;
        double dy = p1.y - p0.y;
        double len = std::sqrt(dx * dx + dy * dy);

        if (len < 0.001) {
            continue;
        }

        double nx = -dy / len * half_width;
        double ny = dx / len * half_width;

        // Create quad as two triangles
        simd_float2 v0 = {static_cast<float>(p0.x + nx), static_cast<float>(p0.y + ny)};
        simd_float2 v1 = {static_cast<float>(p0.x - nx), static_cast<float>(p0.y - ny)};
        simd_float2 v2 = {static_cast<float>(p1.x + nx), static_cast<float>(p1.y + ny)};
        simd_float2 v3 = {static_cast<float>(p1.x - nx), static_cast<float>(p1.y - ny)};

        simd_float4 color_vec = {stroke_color.red, stroke_color.green, stroke_color.blue, stroke_color.alpha};

        // First triangle
        triangles.push_back({v0, v1, v2, color_vec});

        // Second triangle
        triangles.push_back({v1, v3, v2, color_vec});
    }

    return triangles;
}

void path_tessellator::triangulate_polygon(
    const std::vector<point>& vertices,
    color fill_color,
    std::vector<triangle>& out_triangles
) {
    if (vertices.size() < 3) {
        return;
    }

    simd_float4 color_vec = {fill_color.red, fill_color.green, fill_color.blue, fill_color.alpha};

    // Fan triangulation from first vertex
    for (size_t i = 1; i < vertices.size() - 1; ++i) {
        simd_float2 v0 = {static_cast<float>(vertices[0].x), static_cast<float>(vertices[0].y)};
        simd_float2 v1 = {static_cast<float>(vertices[i].x), static_cast<float>(vertices[i].y)};
        simd_float2 v2 = {static_cast<float>(vertices[i + 1].x), static_cast<float>(vertices[i + 1].y)};

        out_triangles.push_back({v0, v1, v2, color_vec});
    }
}

}  // namespace stdui::rendering

#endif  // __APPLE__
