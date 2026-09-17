// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include "stdui/geometry.hpp"
#include <cstdint>
#include <memory>
#include <string>

#ifdef __APPLE__
#include <Metal/Metal.h>
#endif

namespace stdui::rendering {

/// Image data format
enum class image_format {
    rgba8,      // 8-bit RGBA
    bgra8,      // 8-bit BGRA
    rgb8,       // 8-bit RGB
    gray8,      // 8-bit grayscale
};

/// Image data wrapper
struct image_data {
    const void* pixels;
    uint32_t width;
    uint32_t height;
    image_format format;
    size_t stride;  // bytes per row
};

#ifdef __APPLE__

/// Manages image textures for Metal rendering
class image_cache {
public:
    image_cache(id<MTLDevice> device);
    ~image_cache();

    /// Load image into texture cache
    /// @param image_id Unique identifier for the image
    /// @param data Image pixel data
    /// @return Metal texture handle
    id<MTLTexture> load_image(uint32_t image_id, const image_data& data);

    /// Get cached texture
    /// @param image_id Image identifier
    /// @return Cached texture or nullptr
    id<MTLTexture> get_texture(uint32_t image_id) const;

    /// Remove texture from cache
    void unload_image(uint32_t image_id);

    /// Clear all cached textures
    void clear();

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

#endif  // __APPLE__

}  // namespace stdui::rendering
