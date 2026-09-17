// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "stdui/core/color.hpp"
#include "stdui/core/effects.hpp"
#include "stdui/core/geometry.hpp"

namespace stdui::rendering {

/// 2D vector for offsets
struct vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

/// 3x3 transformation matrix (for 2D transforms)
struct mat3 {
    float m[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};  // Identity by default

    static mat3 identity() {
        return mat3{};
    }
};

/// Render node types
enum class render_node_type {
    rectangle,
    text,
    path,
    image,
    group,
    effect,
    scene_view,
};

/// Rectangle rendering properties
struct rectangle_properties {
    rect bounds;
    color fill_color;
    std::optional<color> stroke_color;
    float stroke_width = 0.0f;
    float corner_radius = 0.0f;
};

/// Text rendering properties
struct text_properties {
    std::string content;
    point position;
    std::string font_family;
    float font_size = 14.0f;
    color text_color;
};

/// Path rendering properties (for complex shapes)
struct path_properties {
    // Path commands (moveTo, lineTo, bezierTo, etc.)
    std::vector<uint8_t> commands;
    std::vector<point> points;
    color fill_color;
    std::optional<color> stroke_color;
    float stroke_width = 0.0f;
};

/// Image rendering properties
struct image_properties {
    rect bounds;
    uint32_t texture_id;  // Backend-specific texture handle
    float opacity = 1.0f;
};

/// Effect types
enum class effect_type {
    blur,
    shadow,
    opacity,
    clip,
};

using stdui::blur_effect;
using stdui::clip_effect;
using stdui::opacity_effect;
using stdui::shadow_effect;
using effect_params = stdui::visual_effect;

/// Render node - represents a single drawable element
class render_node {
public:
    render_node(render_node_type type) : type_(type) {}
    virtual ~render_node() = default;

    render_node_type type() const { return type_; }

    /// Transform from parent space to this node's space
    const std::optional<mat3>& transform() const { return transform_; }
    void set_transform(const mat3& t) { transform_ = t; }

    /// Clip rect in this node's space
    const std::optional<rect>& clip_rect() const { return clip_rect_; }
    void set_clip_rect(const rect& r) { clip_rect_ = r; }

    /// Opacity (0.0 - 1.0)
    float opacity() const { return opacity_; }
    void set_opacity(float o) { opacity_ = o; }

    /// Whether this node is visible (for culling)
    bool is_visible() const { return is_visible_; }
    void set_visible(bool v) { is_visible_ = v; }

private:
    render_node_type type_;
    std::optional<mat3> transform_;
    std::optional<rect> clip_rect_;
    float opacity_ = 1.0f;
    bool is_visible_ = true;
};

/// Rectangle render node
class rectangle_node : public render_node {
public:
    rectangle_node() : render_node(render_node_type::rectangle) {}
    explicit rectangle_node(const rectangle_properties& props)
        : render_node(render_node_type::rectangle), props_(props) {}

    const rectangle_properties& properties() const { return props_; }
    rectangle_properties& properties() { return props_; }

private:
    rectangle_properties props_;
};

/// Text render node
class text_node : public render_node {
public:
    text_node() : render_node(render_node_type::text) {}
    explicit text_node(const text_properties& props)
        : render_node(render_node_type::text), props_(props) {}

    const text_properties& properties() const { return props_; }
    text_properties& properties() { return props_; }

private:
    text_properties props_;
};

/// Path render node
class path_node : public render_node {
public:
    path_node() : render_node(render_node_type::path) {}
    explicit path_node(const path_properties& props)
        : render_node(render_node_type::path), props_(props) {}

    const path_properties& properties() const { return props_; }
    path_properties& properties() { return props_; }

private:
    path_properties props_;
};

/// Image render node
class image_node : public render_node {
public:
    image_node() : render_node(render_node_type::image) {}
    explicit image_node(const image_properties& props)
        : render_node(render_node_type::image), props_(props) {}

    const image_properties& properties() const { return props_; }
    image_properties& properties() { return props_; }

private:
    image_properties props_;
};

/// Group node - contains child nodes
class group_node : public render_node {
public:
    group_node() : render_node(render_node_type::group) {}

    const std::vector<std::shared_ptr<render_node>>& children() const {
        return children_;
    }

    void add_child(std::shared_ptr<render_node> child) {
        children_.push_back(std::move(child));
    }

private:
    std::vector<std::shared_ptr<render_node>> children_;
};

/// Effect node - applies an effect to its child
class effect_node : public render_node {
public:
    effect_node() : render_node(render_node_type::effect) {}

    effect_type effect() const { return effect_; }
    void set_effect(effect_type e) { effect_ = e; }

    const effect_params& parameters() const { return params_; }
    void set_parameters(effect_params p) { params_ = std::move(p); }

    const std::shared_ptr<render_node>& child() const { return child_; }
    void set_child(std::shared_ptr<render_node> c) { child_ = std::move(c); }

private:
    effect_type effect_;
    effect_params params_;
    std::shared_ptr<render_node> child_;
};

/// Scene view node - placeholder for user's 3D rendering
class scene_view_node : public render_node {
public:
    scene_view_node() : render_node(render_node_type::scene_view) {}

    const rect& viewport() const { return viewport_; }
    void set_viewport(const rect& v) { viewport_ = v; }

    uint32_t scene_id() const { return scene_id_; }
    void set_scene_id(uint32_t id) { scene_id_ = id; }

private:
    rect viewport_;
    uint32_t scene_id_ = 0;
};

/// Render tree - root of the rendering hierarchy
class render_tree {
public:
    render_tree() = default;

    const std::shared_ptr<render_node>& root() const { return root_; }
    void set_root(std::shared_ptr<render_node> r) { root_ = std::move(r); }

    /// Frame number (increments each update)
    uint64_t frame_number() const { return frame_number_; }
    void set_frame_number(uint64_t n) { frame_number_ = n; }

private:
    std::shared_ptr<render_node> root_;
    uint64_t frame_number_ = 0;
};

}  // namespace stdui::rendering
