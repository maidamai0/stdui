// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include "stdui/render/node.hpp"
#include "stdui/core/geometry.hpp"

#ifdef __APPLE__
#include <Metal/Metal.h>
#endif

namespace stdui::rendering {

#ifdef __APPLE__

/// Manages effect rendering (blur, shadow, etc.)
class effect_renderer {
public:
    effect_renderer(id<MTLDevice> device);
    ~effect_renderer();

    /// Apply blur effect to a texture
    /// @param command_buffer Current command buffer
    /// @param source_texture Input texture
    /// @param blur_radius Blur radius in pixels
    /// @return Blurred texture
    id<MTLTexture> apply_blur(
        id<MTLCommandBuffer> command_buffer,
        id<MTLTexture> source_texture,
        float blur_radius
    );

    /// Apply shadow effect
    /// @param command_buffer Current command buffer
    /// @param source_texture Input texture (alpha mask)
    /// @param offset Shadow offset
    /// @param blur_radius Shadow blur radius
    /// @param shadow_color Shadow color
    /// @return Shadow texture
    id<MTLTexture> apply_shadow(
        id<MTLCommandBuffer> command_buffer,
        id<MTLTexture> source_texture,
        vec2 offset,
        float blur_radius,
        color shadow_color
    );

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

#endif  // __APPLE__

}  // namespace stdui::rendering
