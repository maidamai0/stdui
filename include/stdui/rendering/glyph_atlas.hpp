// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include "stdui/geometry.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

#ifdef __APPLE__
#include <Metal/Metal.h>
#include <CoreText/CoreText.h>
#endif

namespace stdui::rendering {

/// Glyph metrics and atlas position
struct glyph_info {
    rect atlas_rect;      // Position in texture atlas
    point offset;         // Offset from baseline
    double advance;       // Horizontal advance
};

/// Manages a texture atlas for text rendering
class glyph_atlas {
public:
    glyph_atlas(uint32_t width, uint32_t height);
    ~glyph_atlas();

    /// Get or create glyph info for a character
    /// @param codepoint Unicode codepoint
    /// @param font_family Font family name
    /// @param font_size Font size in points
    /// @return Glyph info with atlas coordinates
    const glyph_info* get_glyph(
        uint32_t codepoint,
        const std::string& font_family,
        float font_size
    );

#ifdef __APPLE__
    /// Get Metal texture for the atlas
    id<MTLTexture> metal_texture() const;

    /// Update atlas texture with new glyph data
    void update_texture();

    /// Set Metal device for texture creation
    void set_metal_device(id<MTLDevice> device);
#endif

    /// Get atlas dimensions
    size atlas_size() const { return size{static_cast<double>(width_), static_cast<double>(height_)}; }

private:
    struct impl;
    std::unique_ptr<impl> impl_;

    uint32_t width_;
    uint32_t height_;

    // Packing state
    uint32_t cursor_x_ = 0;
    uint32_t cursor_y_ = 0;
    uint32_t row_height_ = 0;

    // Cache key: font_family + font_size + codepoint
    struct font_key {
        std::string family;
        float size;
        uint32_t codepoint;

        bool operator==(const font_key& other) const {
            return family == other.family &&
                   size == other.size &&
                   codepoint == other.codepoint;
        }
    };

    struct font_key_hash {
        size_t operator()(const font_key& k) const {
            size_t h1 = std::hash<std::string>{}(k.family);
            size_t h2 = std::hash<float>{}(k.size);
            size_t h3 = std::hash<uint32_t>{}(k.codepoint);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    std::unordered_map<font_key, glyph_info, font_key_hash> glyph_cache_;

#ifdef __APPLE__
    /// Rasterize a glyph using Core Text
    bool rasterize_glyph(
        uint32_t codepoint,
        const std::string& font_family,
        float font_size,
        glyph_info& info
    );
#endif
};

}  // namespace stdui::rendering
