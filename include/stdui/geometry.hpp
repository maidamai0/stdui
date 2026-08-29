#pragma once

namespace stdui {

/// Logical two-dimensional position in device-independent units.
struct point {
  double x = 0.0;
  double y = 0.0;

  bool operator==(point const &) const = default;
};

/// Logical two-dimensional size in device-independent units.
struct size {
  double width = 0.0;
  double height = 0.0;

  bool operator==(size const &) const = default;
};

/// Axis-aligned logical rectangle defined by an origin and extent.
struct rect {
  point origin;
  size extent;

  bool operator==(rect const &) const = default;

  [[nodiscard]] bool contains(point const &candidate) const {
    return candidate.x >= origin.x && candidate.x <= origin.x + extent.width &&
           candidate.y >= origin.y && candidate.y <= origin.y + extent.height;
  }

  [[nodiscard]] rect inset(double amount) const {
    return {{origin.x + amount, origin.y + amount},
            {extent.width - 2.0 * amount, extent.height - 2.0 * amount}};
  }
};

} // namespace stdui
