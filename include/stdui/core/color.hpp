// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace stdui {

/// Logical RGBA color with channels in the range [0, 1].
struct color {
  float red = 0.0f;
  float green = 0.0f;
  float blue = 0.0f;
  float alpha = 1.0f;

  constexpr color() = default;
  constexpr color(float red_value, float green_value, float blue_value, float alpha_value = 1.0f)
      : red(red_value), green(green_value), blue(blue_value), alpha(alpha_value) {}

  /// Creates a color from 8-bit channels.
  static constexpr auto from_rgb(uint8_t red_value, uint8_t green_value, uint8_t blue_value,
                                 uint8_t alpha_value = 255) -> color {
    return {red_value / 255.0f, green_value / 255.0f, blue_value / 255.0f, alpha_value / 255.0f};
  }

  /// Packs the color as 0xRRGGBBAA.
  constexpr auto to_rgba() const -> uint32_t {
    return (static_cast<uint32_t>(red * 255.0f) << 24U) |
           (static_cast<uint32_t>(green * 255.0f) << 16U) |
           (static_cast<uint32_t>(blue * 255.0f) << 8U) | static_cast<uint32_t>(alpha * 255.0f);
  }

  bool operator==(color const &) const = default;

  static constexpr auto black() -> color { return {0.0f, 0.0f, 0.0f, 1.0f}; }
  static constexpr auto white() -> color { return {1.0f, 1.0f, 1.0f, 1.0f}; }
  static constexpr auto red_color() -> color { return {1.0f, 0.0f, 0.0f, 1.0f}; }
  static constexpr auto green_color() -> color { return {0.0f, 1.0f, 0.0f, 1.0f}; }
  static constexpr auto blue_color() -> color { return {0.0f, 0.0f, 1.0f, 1.0f}; }
  static constexpr auto transparent() -> color { return {0.0f, 0.0f, 0.0f, 0.0f}; }
};

} // namespace stdui
