#pragma once

#include <stdui/color.hpp>
#include <stdui/geometry.hpp>

#include <variant>

namespace stdui {

/// Opacity applied to a subtree.
struct opacity_effect {
  float opacity = 1.0f;
};

/// Rectangular clipping applied to a subtree.
struct clip_effect {
  rect bounds;
};

/// Gaussian-like blur request. The renderer chooses the implementation.
struct blur_effect {
  float radius = 0.0f;
};

/// Shadow request. The renderer chooses the compositing technique.
struct shadow_effect {
  point offset;
  float blur_radius = 0.0f;
  color shadow_color = color::black();
};

/// Backend-neutral visual effect description.
using visual_effect = std::variant<opacity_effect, clip_effect, blur_effect, shadow_effect>;

} // namespace stdui
