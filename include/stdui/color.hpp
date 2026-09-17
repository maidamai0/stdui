// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace stdui {

/// RGBA color representation
struct color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    constexpr color() = default;
    constexpr color(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}

    /// Create color from 8-bit RGB values
    static constexpr color from_rgb(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) {
        return color{
            red / 255.0f,
            green / 255.0f,
            blue / 255.0f,
            alpha / 255.0f
        };
    }

    /// Convert to 32-bit RGBA (0xRRGGBBAA)
    constexpr uint32_t to_rgba() const {
        return (static_cast<uint32_t>(r * 255) << 24) |
               (static_cast<uint32_t>(g * 255) << 16) |
               (static_cast<uint32_t>(b * 255) << 8) |
               static_cast<uint32_t>(a * 255);
    }

    /// Common color constants
    static constexpr color black() { return color{0.0f, 0.0f, 0.0f, 1.0f}; }
    static constexpr color white() { return color{1.0f, 1.0f, 1.0f, 1.0f}; }
    static constexpr color red() { return color{1.0f, 0.0f, 0.0f, 1.0f}; }
    static constexpr color green() { return color{0.0f, 1.0f, 0.0f, 1.0f}; }
    static constexpr color blue() { return color{0.0f, 0.0f, 1.0f, 1.0f}; }
    static constexpr color transparent() { return color{0.0f, 0.0f, 0.0f, 0.0f}; }
};

}  // namespace stdui
