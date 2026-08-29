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
};

} // namespace stdui
