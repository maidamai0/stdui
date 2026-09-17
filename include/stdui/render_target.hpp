#pragma once

#include <stdui/geometry.hpp>

namespace stdui {

/// Describes what kind of output a renderer is targeting.
enum class render_target_kind {
  image,
  texture,
  window_surface,
};

/// Logical and physical properties of a rendering destination.
struct render_target_descriptor {
  render_target_kind kind = render_target_kind::image;
  size logical_size;
  double scale = 1.0;

  auto physical_size() const -> size {
    return {logical_size.width * scale, logical_size.height * scale};
  }

  bool operator==(render_target_descriptor const &) const = default;
};

/// Backend-neutral rendering destination.
class render_target {
public:
  virtual ~render_target() = default;
  virtual auto descriptor() const -> render_target_descriptor const & = 0;
};

} // namespace stdui
